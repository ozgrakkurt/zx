// Measures the current cost of the ELF unwind and debug-line queries.
//
// The benchmark tries to stay representative:
//   - eh_frame is exercised through real stack traces with distinct functions
//     per frame, so different FDEs are looked up.
//   - debug_line is exercised with PCs sampled across the whole line table,
//     not just one repeated address.
//
// The *_linear / *_binary sections are local estimates of the lookup cost
// itself, so we can estimate how much the binary-search changes will help.
import zx;

using namespace zx;

#ifdef ZX_DEBUG_INFO

namespace zx {

extern "C" elf_eh_frame::EhFrame ZX_ELF_EH_FRAME_;
extern "C" elf_debug_line::DebugLine ZX_ELF_DEBUG_LINE_;

} // namespace zx

namespace {

constexpr u64 CHAIN_DEPTH = 28;
constexpr u64 ITERS = 20'000;
constexpr u64 WARMUP_ITERS = 1'000;
constexpr u64 LINE_INFO_CAP = 8;
constexpr u64 MAX_PC_SAMPLES = 1024;
constexpr u64 LINE_TARGET_QUERIES = 1'000'000;

using Fde = elf_eh_frame::EhFrame::FDE;
using Seq = elf_debug_line::DebugLine::Sequence;
using LineInfo = elf_debug_line::DebugLine::LineInfo;

static void keep_alive(u64 value) {
    u64_c asm_value = (u64_c)value;
    __asm__ volatile("" : "+r"(asm_value));
}

static bool fde_contains_pc(u64 pc) {
    Slice<Fde> const fdes = ZX_ELF_EH_FRAME_.m_fd_entries;

    for (u64 i = 0; i < fdes.len(); i++) {
        Fde const *const fde = fdes.offset_unsafe(i);

        bool overflow = false;
        u64 const end = add_u64(fde->pc_begin, fde->pc_range, &overflow);
        assert(!overflow);

        if (fde->pc_begin <= pc && pc < end) {
            return true;
        }
    }

    return false;
}

// Sample PCs from real code rows only. Dead line-table entries are filtered
// out by checking whether the PC is covered by an FDE.
static u64 build_pc_samples(u64 *out, u64 cap) {
    Slice<elf_debug_line::DebugLine::Row> const rows = ZX_ELF_DEBUG_LINE_.rows;

    u64 n_valid = 0;
    for (u64 i = 0; i < rows.len(); i++) {
        if (fde_contains_pc(rows.read_unsafe(i).pc)) {
            n_valid++;
        }
    }

    if (n_valid == 0 || cap == 0) {
        return 0;
    }

    bool overflow = false;
    u64 stride = div_ceil_u64(n_valid, cap, &overflow);
    assert(!overflow);
    stride = max_u64(1, stride);

    u64 seen = 0;
    u64 n = 0;
    u64 last_pc = 0;

    for (u64 i = 0; i < rows.len(); i++) {
        u64 const pc = rows.read_unsafe(i).pc;
        if (!fde_contains_pc(pc)) {
            continue;
        }

        last_pc = pc;
        if (seen % stride == 0) {
            out[n] = pc;
            n++;
            if (n == cap) {
                break;
            }
        }
        seen++;
    }

    // Make sure the end of the line table is represented too.
    if (n < cap && (n == 0 || out[n - 1] != last_pc)) {
        out[n] = last_pc;
        n++;
    }

    return n;
}

static u64 count_frames(Backtrace const *bt) {
    for (u64 i = 0; i < Backtrace::LEN; i++) {
        if (bt->program_counters[i] == 0) {
            return i;
        }
    }
    return Backtrace::LEN;
}

// Distinct function per recursion level, so each frame belongs to a different
// FDE instead of repeating one recursive return address.
template <u64 N>
__attribute__((noinline)) void collect_chain(Backtrace *bt, u64 iters) {
    collect_chain<N - 1>(bt, iters);
    __asm__ volatile("" ::: "memory");
}

template <>
__attribute__((noinline)) void collect_chain<0>(Backtrace *bt, u64 iters) {
    for (u64 i = 0; i < iters; i++) {
        Backtrace::collect(bt);
    }
}

static u64 run_line_queries(Slice<u64> pcs, List<LineInfo> *line_infos,
                            u64 iters, u64 *checksum) {
    u64 queries = 0;

    for (u64 i = 0; i < iters; i++) {
        for (u64 j = 0; j < pcs.len(); j++) {
            line_infos->reset();
            (void)ZX_ELF_DEBUG_LINE_.get_line_infos(pcs.read_unsafe(j),
                                                    line_infos);
            *checksum = add_unsafe_u64(*checksum, line_infos->n_written());
            queries = add_unsafe_u64(queries, 1);
        }
    }

    return queries;
}

__attribute__((noinline)) static Fde const *find_fde_linear(u64 rip) {
    Slice<Fde> const fdes = ZX_ELF_EH_FRAME_.m_fd_entries;

    for (u64 i = 0; i < fdes.len(); i++) {
        Fde const *const fde = fdes.offset_unsafe(i);

        bool overflow = false;
        u64 const end = add_u64(fde->pc_begin, fde->pc_range, &overflow);
        assert(!overflow);

        if (fde->pc_begin <= rip && rip < end) {
            return fde;
        }
    }

    return nullptr;
}

__attribute__((noinline)) static Fde const *find_fde_binary(u64 rip) {
    Slice<Fde> const fdes = ZX_ELF_EH_FRAME_.m_fd_entries;

    u64 lo = 0;
    u64 hi = fdes.len();
    while (lo < hi) {
        u64 const mid =
            add_unsafe_u64(lo, div_unsafe_u64(sub_unsafe_u64(hi, lo), 2));

        Fde const *const fde = fdes.offset_unsafe(mid);
        if (fde->pc_begin <= rip) {
            lo = add_unsafe_u64(mid, 1);
        } else {
            hi = mid;
        }
    }

    if (lo == 0) {
        return nullptr;
    }

    Fde const *const fde = fdes.offset_unsafe(sub_unsafe_u64(lo, 1));
    bool overflow = false;
    u64 const end = add_u64(fde->pc_begin, fde->pc_range, &overflow);
    assert(!overflow);

    return (rip < end) ? fde : nullptr;
}

__attribute__((noinline)) static u64 find_seq_linear(u64 pc) {
    Slice<Seq> const seqs = ZX_ELF_DEBUG_LINE_.sequences;

    for (u64 i = 0; i < seqs.len(); i++) {
        Seq const seq = seqs.read_unsafe(i);
        if (seq.pc_begin <= pc && pc < seq.pc_end) {
            return i;
        }
    }

    return U64_MAX;
}

__attribute__((noinline)) static u64 find_seq_binary(u64 pc) {
    Slice<u32> const order = ZX_ELF_DEBUG_LINE_.sequence_index;
    Slice<Seq> const seqs = ZX_ELF_DEBUG_LINE_.sequences;

    u64 lo = 0;
    u64 hi = order.len();
    while (lo < hi) {
        u64 const mid =
            add_unsafe_u64(lo, div_unsafe_u64(sub_unsafe_u64(hi, lo), 2));

        u64 const seq_idx = order.read_unsafe(mid);
        Seq const seq = seqs.read_unsafe(seq_idx);
        if (seq.pc_begin <= pc) {
            lo = add_unsafe_u64(mid, 1);
        } else {
            hi = mid;
        }
    }

    if (lo == 0) {
        return U64_MAX;
    }

    u64 const seq_idx = order.read_unsafe(sub_unsafe_u64(lo, 1));
    Seq const seq = seqs.read_unsafe(seq_idx);
    return (pc < seq.pc_end) ? seq_idx : U64_MAX;
}

static u64 run_fde_lookups(Slice<u64> rips, bool binary, u64 iters,
                           u64 *checksum) {
    u64 queries = 0;

    for (u64 i = 0; i < iters; i++) {
        for (u64 j = 0; j < rips.len(); j++) {
            Fde const *fde = binary ? find_fde_binary(rips.read_unsafe(j))
                                    : find_fde_linear(rips.read_unsafe(j));
            *checksum = add_unsafe_u64(*checksum, fde != nullptr);
            queries = add_unsafe_u64(queries, 1);
        }
    }

    return queries;
}

static u64 run_seq_lookups(Slice<u64> pcs, bool binary, u64 iters,
                           u64 *checksum) {
    u64 queries = 0;

    for (u64 i = 0; i < iters; i++) {
        for (u64 j = 0; j < pcs.len(); j++) {
            u64 const idx = binary ? find_seq_binary(pcs.read_unsafe(j))
                                   : find_seq_linear(pcs.read_unsafe(j));
            *checksum = add_unsafe_u64(*checksum, idx != U64_MAX);
            queries = add_unsafe_u64(queries, 1);
        }
    }

    return queries;
}

static void write_fmt(Fmt *fmt) {
    u64 n_written = 0;
    assert(sys::WriteStatus::Ok ==
           sys::write(nullptr, sys::STDOUT, fmt->written().ptr(),
                      fmt->written().len(), &n_written));
    assert(n_written == fmt->written().len());
    fmt->reset();
}

static void push_cstr(Fmt *fmt, char const *str, bool *fail) {
    fmt->push_str(CString::init_unsafe(str).as_slice(), fail);
}

static void print_result(char const *name, u64 total_ns, u64 units,
                         char const *unit) {
    u8 buf[320];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));
    bool fail = false;

    u64 const per_unit = (units == 0) ? 0 : div_unsafe_u64(total_ns, units);

    push_cstr(&fmt, name, &fail);
    push_cstr(&fmt, ": ", &fail);
    fmt.push_u64(total_ns, &fail);
    push_cstr(&fmt, " ns total, ", &fail);
    fmt.push_u64(per_unit, &fail);
    push_cstr(&fmt, " ns/", &fail);
    push_cstr(&fmt, unit, &fail);
    push_cstr(&fmt, ", ", &fail);
    fmt.push_u64(units, &fail);
    push_cstr(&fmt, " ", &fail);
    push_cstr(&fmt, unit, &fail);
    push_cstr(&fmt, "s\n", &fail);

    assert(!fail);
    write_fmt(&fmt);
}

} // namespace

int main(void) {
    Backtrace err_bt = Backtrace::init();

    TimeInfo time_info;
    assert(TimeInfo::InitStatus::Ok == TimeInfo::init(&err_bt, &time_info));

    // eh_frame: time real stack-trace collection with distinct frames.
    Backtrace bt = Backtrace::init();
    collect_chain<CHAIN_DEPTH>(&bt, WARMUP_ITERS);

    Timer eh_timer = Timer::init(time_info);
    collect_chain<CHAIN_DEPTH>(&bt, ITERS);
    u64 const eh_ns = eh_timer.elapsed();
    u64 const n_frames = count_frames(&bt);

    u64 rips[Backtrace::LEN];
    for (u64 i = 0; i < n_frames; i++) {
        rips[i] = bt.program_counters[i];
    }
    Slice<u64> const rip_slice = Slice<u64>::init_unsafe(rips, n_frames);

    // debug_line: query a spread of real PCs from the whole line table.
    u64 sample_pcs[MAX_PC_SAMPLES];
    u64 const n_samples = build_pc_samples(sample_pcs, MAX_PC_SAMPLES);
    assert(n_samples > 0);
    Slice<u64> const pc_slice = Slice<u64>::init_unsafe(sample_pcs, n_samples);

    u64 const line_iters =
        max_u64(1, div_unsafe_u64(LINE_TARGET_QUERIES, n_samples));
    u64 const line_warmup = min_u64(WARMUP_ITERS, line_iters);

    LineInfo line_info_mem[LINE_INFO_CAP];
    List<LineInfo> line_infos = List<LineInfo>::init(
        SliceMut<LineInfo>::init_unsafe(line_info_mem, LINE_INFO_CAP));

    u64 checksum = 0;
    (void)run_line_queries(pc_slice, &line_infos, line_warmup, &checksum);

    checksum = 0;
    Timer line_timer = Timer::init(time_info);
    u64 const n_queries =
        run_line_queries(pc_slice, &line_infos, line_iters, &checksum);
    u64 const line_ns = line_timer.elapsed();
    keep_alive(checksum);

    // Estimate the sequence-lookup part of debug_line.
    checksum = 0;
    (void)run_seq_lookups(pc_slice, false, line_warmup, &checksum);

    checksum = 0;
    Timer seq_linear_timer = Timer::init(time_info);
    u64 const n_seq_queries =
        run_seq_lookups(pc_slice, false, line_iters, &checksum);
    u64 const seq_linear_ns = seq_linear_timer.elapsed();
    keep_alive(checksum);

    checksum = 0;
    (void)run_seq_lookups(pc_slice, true, line_warmup, &checksum);

    checksum = 0;
    Timer seq_binary_timer = Timer::init(time_info);
    (void)run_seq_lookups(pc_slice, true, line_iters, &checksum);
    u64 const seq_binary_ns = seq_binary_timer.elapsed();
    keep_alive(checksum);

    // Estimate the FDE-lookup part of eh_frame.
    checksum = 0;
    (void)run_fde_lookups(rip_slice, false, WARMUP_ITERS, &checksum);

    checksum = 0;
    Timer fde_linear_timer = Timer::init(time_info);
    u64 const n_fde_queries =
        run_fde_lookups(rip_slice, false, ITERS, &checksum);
    u64 const fde_linear_ns = fde_linear_timer.elapsed();
    keep_alive(checksum);

    checksum = 0;
    (void)run_fde_lookups(rip_slice, true, WARMUP_ITERS, &checksum);

    checksum = 0;
    Timer fde_binary_timer = Timer::init(time_info);
    (void)run_fde_lookups(rip_slice, true, ITERS, &checksum);
    u64 const fde_binary_ns = fde_binary_timer.elapsed();
    keep_alive(checksum);

    print_result("eh_frame backtrace", eh_ns, ITERS, "backtrace");
    print_result("eh_frame FDE lookup linear", fde_linear_ns, n_fde_queries,
                 "lookup");
    print_result("eh_frame FDE lookup binary", fde_binary_ns, n_fde_queries,
                 "lookup");

    print_result("debug_line lookup", line_ns, n_queries, "lookup");
    print_result("debug_line sequence scan linear", seq_linear_ns,
                 n_seq_queries, "lookup");
    print_result("debug_line sequence scan binary", seq_binary_ns,
                 n_seq_queries, "lookup");

    u8 info_buf[320];
    Fmt info_fmt =
        Fmt::init(SliceMut<u8>::init_unsafe(info_buf, sizeof(info_buf)));
    bool fail = false;
    push_cstr(&info_fmt, "frames=", &fail);
    info_fmt.push_u64(n_frames, &fail);
    push_cstr(&info_fmt, ", chain_depth=", &fail);
    info_fmt.push_u64(CHAIN_DEPTH, &fail);
    push_cstr(&info_fmt, ", line_rows=", &fail);
    info_fmt.push_u64(ZX_ELF_DEBUG_LINE_.rows.len(), &fail);
    push_cstr(&info_fmt, ", sample_pcs=", &fail);
    info_fmt.push_u64(n_samples, &fail);
    push_cstr(&info_fmt, ", sequences=", &fail);
    info_fmt.push_u64(ZX_ELF_DEBUG_LINE_.sequences.len(), &fail);
    push_cstr(&info_fmt, ", fdes=", &fail);
    info_fmt.push_u64(ZX_ELF_EH_FRAME_.m_fd_entries.len(), &fail);
    push_cstr(&info_fmt, "\n", &fail);
    assert(!fail);
    write_fmt(&info_fmt);

    return 0;
}

#else // ifdef ZX_DEBUG_INFO

int main(void) {
    char const message[] = "elf_query_bench requires debug info\n";
    u64 n_written = 0;
    assert(sys::WriteStatus::Ok == sys::write(nullptr, sys::STDOUT,
                                              (u8 const *)message,
                                              sizeof(message) - 1, &n_written));
    assert(n_written == sizeof(message) - 1);
    return 0;
}

#endif // ifdef ZX_DEBUG_INFO
