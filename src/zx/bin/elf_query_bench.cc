// Measures the current cost of the ELF unwind and debug-line queries.
//
// eh_frame is exercised through real stack-trace collection. debug_line is
// exercised by querying every PC from one collected stack trace.
//
// This is intentionally small: it is a baseline for the upcoming binary-search
// changes, not a full benchmark framework.
import zx;

using namespace zx;

#ifdef ZX_DEBUG_INFO

namespace zx {

extern "C" elf_eh_frame::EhFrame ZX_ELF_EH_FRAME_;
extern "C" elf_debug_line::DebugLine ZX_ELF_DEBUG_LINE_;

} // namespace zx

namespace {

constexpr u64 DEPTH = 28;
constexpr u64 ITERS = 20'000;
constexpr u64 WARMUP_ITERS = 1'000;
constexpr u64 LINE_INFO_CAP = 8;

__attribute__((noinline)) static void touch(volatile u64 *value) {
    *value = *value + 1;
}

// Keep one real stack frame per recursion level and run the loop at the
// deepest frame so every collection sees a full stack.
__attribute__((noinline)) static void collect_at_depth(u64 depth, Backtrace *bt,
                                                       u64 iters) {
    volatile u64 frame_marker = depth;

    if (depth == 0) {
        for (u64 i = 0; i < iters; i++) {
            Backtrace::collect(bt);
        }
        return;
    }

    collect_at_depth(sub_unsafe_u64(depth, 1), bt, iters);
    touch(&frame_marker);
}

static u64 count_frames(Backtrace const *bt) {
    for (u64 i = 0; i < Backtrace::LEN; i++) {
        if (bt->program_counters[i] == 0) {
            return i;
        }
    }
    return Backtrace::LEN;
}

static u64
run_line_queries(Backtrace const *bt,
                 List<elf_debug_line::DebugLine::LineInfo> *line_infos,
                 u64 iters, u64 *checksum) {
    u64 queries = 0;

    for (u64 i = 0; i < iters; i++) {
        for (u64 j = 0; j < Backtrace::LEN; j++) {
            u64 const pc = bt->program_counters[j];
            if (pc == 0) {
                break;
            }

            line_infos->reset();
            (void)ZX_ELF_DEBUG_LINE_.get_line_infos(pc, line_infos);
            *checksum =
                add_unsafe_u64(*checksum, line_infos->n_written());
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
    u8 buf[256];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));
    bool fail = false;

    u64 const per_unit =
        (units == 0) ? 0 : div_unsafe_u64(total_ns, units);

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
    assert(TimeInfo::InitStatus::Ok ==
           TimeInfo::init(&err_bt, &time_info));

    // eh_frame: time real stack-trace collection.
    Backtrace bt = Backtrace::init();
    collect_at_depth(DEPTH, &bt, WARMUP_ITERS);

    Timer eh_timer = Timer::init(time_info);
    collect_at_depth(DEPTH, &bt, ITERS);
    u64 const eh_ns = eh_timer.elapsed();
    u64 const n_frames = count_frames(&bt);

    // debug_line: query all PCs from one full stack trace.
    Backtrace line_bt = Backtrace::init();
    collect_at_depth(DEPTH, &line_bt, 1);

    elf_debug_line::DebugLine::LineInfo line_info_mem[LINE_INFO_CAP];
    List<elf_debug_line::DebugLine::LineInfo> line_infos =
        List<elf_debug_line::DebugLine::LineInfo>::init(
            SliceMut<elf_debug_line::DebugLine::LineInfo>::init_unsafe(
                line_info_mem, LINE_INFO_CAP));

    u64 checksum = 0;
    (void)run_line_queries(&line_bt, &line_infos, WARMUP_ITERS, &checksum);

    checksum = 0;
    Timer line_timer = Timer::init(time_info);
    u64 const n_queries =
        run_line_queries(&line_bt, &line_infos, ITERS, &checksum);
    u64 const line_ns = line_timer.elapsed();

    // Keep the compiler from removing the queries whose results we do not
    // otherwise look at.
    u64_c checksum_asm = (u64_c)checksum;
    __asm__ volatile("" : "+r"(checksum_asm));

    print_result("eh_frame backtrace", eh_ns, ITERS, "backtrace");
    print_result("debug_line lookup", line_ns, n_queries, "lookup");

    u8 info_buf[256];
    Fmt info_fmt = Fmt::init(SliceMut<u8>::init_unsafe(info_buf, sizeof(info_buf)));
    bool fail = false;
    push_cstr(&info_fmt, "frames in benchmark stack: ", &fail);
    info_fmt.push_u64(n_frames, &fail);
    push_cstr(&info_fmt, ", depth: ", &fail);
    info_fmt.push_u64(DEPTH, &fail);
    push_cstr(&info_fmt, "\n", &fail);
    assert(!fail);
    write_fmt(&info_fmt);

    return 0;
}

#else // ifdef ZX_DEBUG_INFO

int main(void) {
    char const message[] = "elf_query_bench requires debug info\n";
    u64 n_written = 0;
    assert(sys::WriteStatus::Ok ==
           sys::write(nullptr, sys::STDOUT, (u8 const *)message,
                      sizeof(message) - 1, &n_written));
    assert(n_written == sizeof(message) - 1);
    return 0;
}

#endif // ifdef ZX_DEBUG_INFO
