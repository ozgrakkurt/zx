// Benchmark for compression::noz.
//
// The benchmark is split into two phases:
//
//   1. Size/cache/ratio sweep: all sizes with naturally aligned buffers.
//   2. Alignment sweep: a few buffer alignments on both input and output.
//
// The cold cache cases use the full arenas and pick a random portion of the
// arena for every iteration. Since the arenas are much larger than the CPU
// cache, most of the accesses miss the cache. The hot cache cases reuse the
// same slices for every iteration.
//
// The memcpy baseline copies the same number of bytes as the compression
// input (and the decompression output).

import compression;
import zx;

namespace noz = compression::noz;

using namespace zx;

namespace {

constexpr u64 KIB = 1024;
constexpr u64 MIB = 1024 * KIB;
constexpr u64 GIB = 1024 * MIB;

constexpr u64 MAX_INPUT_SIZE = GIB;
constexpr u64 MAX_ALIGN = 63;

// Input/output arenas have a little extra room so we can offset the slices
// by up to 63 bytes without going out of bounds.
constexpr u64 INPUT_ARENA_SIZE = MAX_INPUT_SIZE + MAX_ALIGN + 1;
constexpr u64 OUTPUT_ARENA_SIZE = MAX_INPUT_SIZE + MAX_ALIGN + 1;

// The hot cache cases target this many bytes per measurement. Small sizes are
// also bounded by the iteration cap so the benchmark doesn't spend forever
// there.
constexpr u64 HOT_TARGET_BYTES = 1 * GIB;
constexpr u64 COLD_TARGET_BYTES = 1 * GIB;
constexpr u64 ALIGN_TARGET_BYTES = 16 * MIB;

// This caps the number of loop iterations for tiny inputs. It still gives
// more than enough work for a stable measurement.
constexpr u64 MAX_ITERS = 100'000;

// Two offset arrays (source and destination) of MAX_ITERS u64s.
constexpr u64 OFFSET_SCRATCH_SIZE = 2 * MAX_ITERS * sizeof(u64);

enum class Pattern : u8_c {
    Zero,   // Best case for noz: every control bit is zero.
    Half,   // Average case: about half of the bytes are zero.
    Random, // Random data: virtually no zero bytes.
    Ones,   // Worst case for noz: absolutely no zero bytes.
};

enum class CacheMode : u8_c {
    Hot,
    Cold,
};

struct SizeCase {
    u64 m_bytes;
    char const *m_name;
};

struct PatternCase {
    Pattern m_pattern;
    char const *m_name;
};

struct AlignCase {
    u64 m_in;
    u64 m_out;
};

struct PointResult {
    u64 m_compressed_size;
    u64 m_iters;
    u64 m_compress_ns;
    u64 m_decompress_ns;
    u64 m_memcpy_ns;
};

struct BenchCtx {
    SliceMut<u8> m_input;
    SliceMut<u8> m_compressed;
    SliceMut<u8> m_output;
    SliceMut<u8> m_scratch;
    Backtrace m_err_bt;
    TimeInfo m_time_info;
    Timer m_timer;
    Xoshiro256 m_rand;
};

constexpr SizeCase SIZE_CASES[] = {
    {1 * KIB, "1KiB"},   {64 * KIB, "64KiB"}, {1 * MIB, "1MiB"},
    {16 * MIB, "16MiB"}, {64 * MIB, "64MiB"},
};

constexpr u64 N_SIZE_CASES = sizeof(SIZE_CASES) / sizeof(SIZE_CASES[0]);

constexpr PatternCase PATTERN_CASES[] = {
    {Pattern::Half, "r50"},
};

constexpr u64 N_PATTERN_CASES =
    sizeof(PATTERN_CASES) / sizeof(PATTERN_CASES[0]);

// This doesn't cover every number between 1 and 63, but it covers the
// interesting classes: odd byte offsets, 64-byte cache line boundaries,
// AVX512 register boundaries and offsets close to 63 bytes.
constexpr AlignCase ALIGN_CASES[] = {
    {0, 0},
};

constexpr u64 N_ALIGN_CASES = sizeof(ALIGN_CASES) / sizeof(ALIGN_CASES[0]);

// The alignment sweep is done on a few representative sizes. Doing it for
// all sizes would make the benchmark take much longer without telling us
// much more.
constexpr SizeCase ALIGN_SIZE_CASES[] = {
    {1 * MIB, "1MiB"},
};

constexpr u64 N_ALIGN_SIZE_CASES =
    sizeof(ALIGN_SIZE_CASES) / sizeof(ALIGN_SIZE_CASES[0]);

static void print_bytes(Backtrace *err_bt, Slice<u8> bytes) {
    u64 offset = 0;
    while (offset < bytes.len()) {
        u64 n_written = 0;
        testing::assert_with_trace(
            sys::WriteStatus::Ok ==
                sys::write(err_bt, sys::STDOUT, bytes.ptr() + offset,
                           sub_unsafe_u64(bytes.len(), offset), &n_written),
            err_bt);
        testing::assert_with_trace(n_written > 0, err_bt);
        offset = add_unsafe_u64(offset, n_written);
    }
}

static void print_line(Backtrace *err_bt, Fmt *fmt) {
    print_bytes(err_bt, fmt->written());
    fmt->reset();
}

static void push_str(Fmt *fmt, char const *str, bool *fail) {
    fmt->push_str(CString::init_unsafe(str).as_slice(), fail);
}

static void push_spaces(Fmt *fmt, u64 count, bool *fail) {
    constexpr char SPACES[] = "                                ";
    constexpr u64 N_SPACES = sizeof(SPACES) - 1;

    while (count > 0) {
        u64 const n = min_u64(count, N_SPACES);
        fmt->push_str(Slice<u8>::init_unsafe((u8 const *)SPACES, n), fail);
        count = sub_unsafe_u64(count, n);
    }
}

static u64 n_digits_u64(u64 val) {
    u64 digits = 1;
    while (val >= 10) {
        val /= 10;
        digits = add_unsafe_u64(digits, 1);
    }
    return digits;
}

static void push_padded_u64(Fmt *fmt, u64 val, u64 width, bool *fail) {
    u64 const digits = n_digits_u64(val);
    if (width > digits) {
        push_spaces(fmt, sub_unsafe_u64(width, digits), fail);
    }
    fmt->push_u64(val, fail);
}

static void push_padded_str(Fmt *fmt, char const *str, u64 width, bool *fail) {
    u64 const len = __builtin_strlen(str);
    fmt->push_str(Slice<u8>::init_unsafe((u8 const *)str, len), fail);
    if (width > len) {
        push_spaces(fmt, sub_unsafe_u64(width, len), fail);
    }
}

static void push_two_digits(Fmt *fmt, u64 val, bool *fail) {
    assert(val < 100);
    if (val < 10) {
        fmt->push_char((u8)'0', fail);
    }
    fmt->push_u64(val, fail);
}

// Pushes a fixed point number with two fractional digits.
static void push_padded_fixed2(Fmt *fmt, u64 val_x100, u64 width, bool *fail) {
    u64 const int_part = val_x100 / 100;
    u64 const frac_part = val_x100 % 100;
    u64 const digits = add_unsafe_u64(n_digits_u64(int_part), 3);
    if (width > digits) {
        push_spaces(fmt, sub_unsafe_u64(width, digits), fail);
    }
    fmt->push_u64(int_part, fail);
    fmt->push_char((u8)'.', fail);
    push_two_digits(fmt, frac_part, fail);
}

// Pushes a percentage with one fractional digit.
static void push_padded_percent(Fmt *fmt, u64 pct_x10, u64 width, bool *fail) {
    u64 const int_part = pct_x10 / 10;
    u64 const frac_part = pct_x10 % 10;
    u64 const digits = add_unsafe_u64(n_digits_u64(int_part), 3);
    if (width > digits) {
        push_spaces(fmt, sub_unsafe_u64(width, digits), fail);
    }
    fmt->push_u64(int_part, fail);
    fmt->push_char((u8)'.', fail);
    fmt->push_u64(frac_part, fail);
    fmt->push_char((u8)'%', fail);
}

static u64 gbps_x100(u64 bytes, u64 nanos) {
    if (nanos == 0) {
        return 0;
    }

    // bytes is at most a few GiB so this multiplication can't overflow.
    return mul_unsafe_u64(bytes, 100) / nanos;
}

static void print_table_header(Backtrace *err_bt, u8 *buf, u64 buf_len) {
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, buf_len));
    bool fail = false;

    push_padded_str(&fmt, "size", 9, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "pattern", 8, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "in", 3, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "out", 4, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "cache", 6, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "comp_bytes", 12, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "comp%", 8, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "iters", 9, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "compress GB/s", 13, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "decompress GB/s", 15, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, "memcpy GB/s", 13, &fail);
    push_str(&fmt, "\n", &fail);

    testing::assert_with_trace(!fail, err_bt);
    print_line(err_bt, &fmt);
}

static void print_result(Backtrace *err_bt, u8 *buf, u64 buf_len,
                         SizeCase size_case, PatternCase pattern_case,
                         AlignCase align_case, CacheMode cache,
                         PointResult result) {
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, buf_len));
    bool fail = false;

    u64 const compressed_pct_x10 =
        mul_unsafe_u64(result.m_compressed_size, 1000) / size_case.m_bytes;
    u64 const total_bytes = mul_unsafe_u64(size_case.m_bytes, result.m_iters);

    push_padded_str(&fmt, size_case.m_name, 9, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, pattern_case.m_name, 8, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_u64(&fmt, align_case.m_in, 3, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_u64(&fmt, align_case.m_out, 4, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_str(&fmt, cache == CacheMode::Hot ? "hot" : "cold", 6, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_u64(&fmt, result.m_compressed_size, 12, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_percent(&fmt, compressed_pct_x10, 8, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_u64(&fmt, result.m_iters, 9, &fail);
    push_str(&fmt, " ", &fail);
    push_padded_fixed2(&fmt, gbps_x100(total_bytes, result.m_compress_ns), 13,
                       &fail);
    push_str(&fmt, " ", &fail);
    push_padded_fixed2(&fmt, gbps_x100(total_bytes, result.m_decompress_ns), 15,
                       &fail);
    push_str(&fmt, " ", &fail);
    push_padded_fixed2(&fmt, gbps_x100(total_bytes, result.m_memcpy_ns), 13,
                       &fail);
    push_str(&fmt, "\n", &fail);

    testing::assert_with_trace(!fail, err_bt);
    print_line(err_bt, &fmt);
}

static u64 compress_bound_or_panic(Backtrace *err_bt, u64 size) {
    u64 bound = 0;
    testing::assert_with_trace(noz::CompressBoundStatus::Ok ==
                                   noz::compress_bound(err_bt, size, &bound),
                               err_bt);
    return bound;
}

static SliceMut<u8> alloc_arena(Backtrace *err_bt, u64 len) {
    u8 *mem = nullptr;

    i32 const prot = sys::PROT_READ | sys::PROT_WRITE;
    i32 const flags = sys::MAP_PRIVATE | sys::MAP_ANONYMOUS | sys::MAP_LOCKED;

    testing::assert_with_trace(
        sys::MMapStatus::Ok ==
            sys::mmap(err_bt, nullptr, len, prot, flags, -1, 0, &mem),
        err_bt);
    assert(mem != nullptr);

    return SliceMut<u8>::init_unsafe(mem, len);
}

static void fill_pattern(SliceMut<u8> dst, Pattern pattern, Xoshiro256 *rand) {
    switch (pattern) {
    case Pattern::Zero: {
        dst.memset(0x00);
        break;
    }
    case Pattern::Ones: {
        dst.memset(0xff);
        break;
    }
    case Pattern::Random: {
        rand->fill(dst);
        break;
    }
    case Pattern::Half: {
        // Keep three random bytes out of every eight, giving about 50%
        // compressed size with the noz format.
        constexpr u64 MASK = u64{0x0000000000ffffff};

        u64 *const words = (u64 *)dst.ptr();
        u64 const n_words = dst.len() / sizeof(u64);
        for (u64 i = 0; i < n_words; i++) {
            words[i] = rand->next() & MASK;
        }

        u64 const words_end = mul_unsafe_u64(n_words, sizeof(u64));
        if (words_end < dst.len()) {
            u64 const remainder = sub_unsafe_u64(dst.len(), words_end);
            u64 const last = rand->next() & MASK;
            SliceMut<u8> const tail = dst.slice_unsafe(words_end, remainder);
            tail.memcpy_from_unsafe(
                Slice<u8>::init_unsafe((u8 const *)&last, remainder));
        }
        break;
    }
    default: {
        break;
    }
    }
}

static u64 iteration_count(u64 size, u64 target_bytes) {
    u64 iters = target_bytes / size;
    if (iters == 0) {
        iters = 1;
    }
    if (iters > MAX_ITERS) {
        iters = MAX_ITERS;
    }
    return iters;
}

// Picks a random cache line aligned offset in [0, max_offset]. Keeping the
// random offsets cache line aligned means the cold cases keep the requested
// input/output alignment while randomizing which cache lines/pages are
// touched.
static u64 random_aligned_offset(Xoshiro256 *rand, u64 max_offset) {
    u64 const max_line = max_offset / 64;
    u64 const line =
        rand_mulshift_mod_u64(rand->next(), add_unsafe_u64(max_line, 1));
    return mul_unsafe_u64(line, 64);
}

static PointResult run_point(BenchCtx *ctx, SizeCase size_case,
                             AlignCase align_case, CacheMode cache,
                             u64 target_bytes) {
    Backtrace *const err_bt = &ctx->m_err_bt;

    u64 const size = size_case.m_bytes;
    u64 const bound = compress_bound_or_panic(err_bt, size);
    u64 const comp_arena_len = ctx->m_compressed.len();

    testing::assert_with_trace(
        add_unsafe_u64(size, align_case.m_in) <= ctx->m_input.len(), err_bt);
    testing::assert_with_trace(
        add_unsafe_u64(bound, align_case.m_out) <= comp_arena_len, err_bt);
    testing::assert_with_trace(
        add_unsafe_u64(size, align_case.m_out) <= ctx->m_output.len(), err_bt);

    u64 iters = iteration_count(size, target_bytes);
    if (cache == CacheMode::Cold && iters < 2) {
        iters = 2;
    }

    u64 const max_in_offset = sub_unsafe_u64(
        ctx->m_input.len(), add_unsafe_u64(size, align_case.m_in));
    u64 const max_out_offset = sub_unsafe_u64(
        ctx->m_output.len(), add_unsafe_u64(size, align_case.m_out));
    u64 const max_comp_offset =
        sub_unsafe_u64(comp_arena_len, add_unsafe_u64(bound, align_case.m_out));

    // max_out_offset is also used for the compressed output in the compress
    // benchmark. The compressed arena is always at least as large as the
    // output arena, but be defensive about it here.
    u64 const max_shared_out_offset = min_u64(max_out_offset, max_comp_offset);

    u64 *const in_offsets = (u64 *)ctx->m_scratch.ptr();
    u64 *const out_offsets = &in_offsets[MAX_ITERS];

    for (u64 i = 0; i < iters; i++) {
        if (cache == CacheMode::Hot) {
            in_offsets[i] = 0;
            out_offsets[i] = 0;
        } else {
            in_offsets[i] = random_aligned_offset(&ctx->m_rand, max_in_offset);
            out_offsets[i] =
                random_aligned_offset(&ctx->m_rand, max_shared_out_offset);
        }
    }

    // The first iteration is also the verification run and it serves as the
    // warmup for the hot cache measurements.
    u64 const verify_in_off = add_unsafe_u64(align_case.m_in, in_offsets[0]);
    u64 const verify_out_off = add_unsafe_u64(align_case.m_out, out_offsets[0]);

    Slice<u8> const verify_in =
        ctx->m_input.as_const().slice_unsafe(verify_in_off, size);
    // Keep the compressed stream at the same byte offset as the input so
    // decompression is also exercised with the requested input alignment.
    testing::assert_with_trace(add_unsafe_u64(bound, verify_in_off) <=
                                   ctx->m_compressed.len(),
                               err_bt);
    SliceMut<u8> const verify_comp =
        ctx->m_compressed.slice_unsafe(verify_in_off, bound);

    u64 compressed_size = 0;
    testing::assert_with_trace(
        noz::CompressStatus::Ok ==
            noz::compress(err_bt, verify_in, verify_comp, &compressed_size),
        err_bt);

    Slice<u8> const compressed =
        verify_comp.as_const().slice_unsafe(0, compressed_size);

    SliceMut<u8> const verify_out =
        ctx->m_output.slice_unsafe(verify_out_off, size);
    u64 n_read = 0;
    testing::assert_with_trace(
        noz::DecompressStatus::Ok ==
            noz::decompress(err_bt, compressed, verify_out, &n_read),
        err_bt);
    testing::assert_with_trace(n_read == compressed_size, err_bt);
    testing::assert_with_trace(
        verify_in.memcmp_unsafe(verify_out.as_const()) == 0, err_bt);

    // Decompression is measured before compression so that the compressed
    // stream prepared above is still intact.
    u64 decompress_ns = 0;
    {
        (void)ctx->m_timer.lap();

        noz::DecompressStatus status = noz::DecompressStatus::Ok;
        u64 last_n_read = 0;

        for (u64 i = 0; i < iters; i++) {
            u64 const out_off =
                add_unsafe_u64(align_case.m_out, out_offsets[i]);
            SliceMut<u8> const out = ctx->m_output.slice_unsafe(out_off, size);
            status = noz::decompress(err_bt, compressed, out, &last_n_read);
        }

        decompress_ns = ctx->m_timer.lap();

        testing::assert_with_trace(status == noz::DecompressStatus::Ok, err_bt);
        testing::assert_with_trace(last_n_read == compressed_size, err_bt);
    }

    u64 compress_ns = 0;
    {
        (void)ctx->m_timer.lap();

        noz::CompressStatus status = noz::CompressStatus::Ok;
        u64 last_written = 0;

        for (u64 i = 0; i < iters; i++) {
            u64 const in_off = add_unsafe_u64(align_case.m_in, in_offsets[i]);
            u64 const out_off =
                add_unsafe_u64(align_case.m_out, out_offsets[i]);
            Slice<u8> const in =
                ctx->m_input.as_const().slice_unsafe(in_off, size);
            SliceMut<u8> const out =
                ctx->m_compressed.slice_unsafe(out_off, bound);
            status = noz::compress(err_bt, in, out, &last_written);
        }

        compress_ns = ctx->m_timer.lap();

        testing::assert_with_trace(status == noz::CompressStatus::Ok, err_bt);
    }

    // Warm up the memcpy destination path before timing it.
    {
        Slice<u8> const src =
            ctx->m_input.as_const().slice_unsafe(verify_in_off, size);
        SliceMut<u8> const dst =
            ctx->m_output.slice_unsafe(verify_out_off, size);
        dst.memcpy_from_unsafe(src);
        __asm__ volatile("" ::: "memory");
    }

    u64 memcpy_ns = 0;
    {
        (void)ctx->m_timer.lap();

        for (u64 i = 0; i < iters; i++) {
            u64 const in_off = add_unsafe_u64(align_case.m_in, in_offsets[i]);
            u64 const out_off =
                add_unsafe_u64(align_case.m_out, out_offsets[i]);
            Slice<u8> const src =
                ctx->m_input.as_const().slice_unsafe(in_off, size);
            SliceMut<u8> const dst = ctx->m_output.slice_unsafe(out_off, size);
            dst.memcpy_from_unsafe(src);
            // Prevent the compiler from removing repeated copies that write
            // to the same destination.
            __asm__ volatile("" ::: "memory");
        }

        memcpy_ns = ctx->m_timer.lap();
    }

    return PointResult{
        .m_compressed_size = compressed_size,
        .m_iters = iters,
        .m_compress_ns = compress_ns,
        .m_decompress_ns = decompress_ns,
        .m_memcpy_ns = memcpy_ns,
    };
}

static void print_phase_header(Backtrace *err_bt, char const *title) {
    u8 buf[1024];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));
    bool fail = false;

    push_str(&fmt, "\n== ", &fail);
    push_str(&fmt, title, &fail);
    push_str(&fmt, " ==\n", &fail);

    testing::assert_with_trace(!fail, err_bt);
    print_line(err_bt, &fmt);
}

} // namespace

int main(void) {
    BenchCtx ctx;
    ctx.m_err_bt = Backtrace::init();
    ctx.m_rand = Xoshiro256::init(0x6e6f7a31);

    Backtrace *const err_bt = &ctx.m_err_bt;

    testing::assert_with_trace(TimeInfo::InitStatus::Ok ==
                                   TimeInfo::init(err_bt, &ctx.m_time_info),
                               err_bt);
    ctx.m_timer = Timer::init(ctx.m_time_info);

    u64 const max_compressed_bound =
        compress_bound_or_panic(err_bt, MAX_INPUT_SIZE);
    u64 const compressed_arena_size =
        add_unsafe_u64(max_compressed_bound, add_unsafe_u64(MAX_ALIGN, 1));

    ctx.m_input = alloc_arena(err_bt, INPUT_ARENA_SIZE);
    ctx.m_compressed = alloc_arena(err_bt, compressed_arena_size);
    ctx.m_output = alloc_arena(err_bt, OUTPUT_ARENA_SIZE);
    ctx.m_scratch = alloc_arena(err_bt, OFFSET_SCRATCH_SIZE);

    // Pre-fault the arenas so the measurements don't include page faults.
    ctx.m_compressed.memset(0);
    ctx.m_output.memset(0);
    ctx.m_scratch.memset(0);

    u8 line_buf[1024];

    {
        Fmt fmt =
            Fmt::init(SliceMut<u8>::init_unsafe(line_buf, sizeof(line_buf)));
        bool fail = false;
        push_str(&fmt, "noz compression benchmark\n", &fail);
        push_str(&fmt, "input arena: ", &fail);
        fmt.push_u64(ctx.m_input.len(), &fail);
        push_str(&fmt, " bytes, compressed arena: ", &fail);
        fmt.push_u64(ctx.m_compressed.len(), &fail);
        push_str(&fmt, " bytes, output arena: ", &fail);
        fmt.push_u64(ctx.m_output.len(), &fail);
        push_str(&fmt, " bytes\n", &fail);
        testing::assert_with_trace(!fail, err_bt);
        print_line(err_bt, &fmt);
    }

    print_phase_header(err_bt, "size sweep, input/output aligned to 0");

    for (u64 p = 0; p < N_PATTERN_CASES; p++) {
        PatternCase const pattern_case = PATTERN_CASES[p];

        // The input arena is always fully populated with the current
        // pattern. Cold cases pick random slices from it and hot cases keep
        // reusing the same slice.
        fill_pattern(ctx.m_input, pattern_case.m_pattern, &ctx.m_rand);

        print_table_header(err_bt, line_buf, sizeof(line_buf));

        for (u64 s = 0; s < N_SIZE_CASES; s++) {
            SizeCase const size_case = SIZE_CASES[s];

            for (u64 c = 0; c < 2; c++) {
                CacheMode const cache =
                    (c == 0) ? CacheMode::Hot : CacheMode::Cold;
                u64 const target = (cache == CacheMode::Hot)
                                       ? HOT_TARGET_BYTES
                                       : COLD_TARGET_BYTES;

                PointResult const result =
                    run_point(&ctx, size_case, AlignCase{0, 0}, cache, target);
                print_result(err_bt, line_buf, sizeof(line_buf), size_case,
                             pattern_case, AlignCase{0, 0}, cache, result);
            }
        }
    }

    print_phase_header(err_bt,
                       "alignment sweep, representative sizes, hot and cold");

    for (u64 p = 0; p < N_PATTERN_CASES; p++) {
        PatternCase const pattern_case = PATTERN_CASES[p];
        fill_pattern(ctx.m_input, pattern_case.m_pattern, &ctx.m_rand);

        print_table_header(err_bt, line_buf, sizeof(line_buf));

        for (u64 s = 0; s < N_ALIGN_SIZE_CASES; s++) {
            SizeCase const size_case = ALIGN_SIZE_CASES[s];

            for (u64 a = 0; a < N_ALIGN_CASES; a++) {
                AlignCase const align_case = ALIGN_CASES[a];

                for (u64 c = 0; c < 2; c++) {
                    CacheMode const cache =
                        (c == 0) ? CacheMode::Hot : CacheMode::Cold;

                    PointResult const result = run_point(
                        &ctx, size_case, align_case, cache, ALIGN_TARGET_BYTES);
                    print_result(err_bt, line_buf, sizeof(line_buf), size_case,
                                 pattern_case, align_case, cache, result);
                }
            }
        }
    }

    return 0;
}
