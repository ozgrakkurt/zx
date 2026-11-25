// Dummy fuzz target for benchmarking the fuzzing engine.
//
// The target validates a fake fixed-size header one 12-bit field at a time.
// Every field that matches opens a new coverage edge, so the fuzzer can make
// progress incrementally instead of having to guess the whole header at once.
// The last field is an unvalidated command value. Reaching it means the
// fuzzer found the intentionally inserted bug.
//
// Run it with:
//   zxbs --fuzz --fuzz-unlimited --opt-level=2 run zx:fuzz_bench
//
// The target logs iterations/sec periodically. When the bug is found it logs
// the total elapsed time and the overall iterations/sec, then exits.

import zx;

using namespace zx;

namespace {

struct Ctx {
    Backtrace m_err_bt;
    Timer m_total_timer;
    Timer m_log_timer;
    u64 m_iterations;
    u64 m_last_log_iterations;
};

__attribute__((no_builtin, no_sanitize("coverage"))) static void
write_bytes_(Backtrace *err_bt, Slice<u8> bytes) {
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

#ifdef ZX_FUZZ

constexpr u64 LOG_INTERVAL_NANOS = u64{1} * 1000__wbu * 1000__wbu * 1000__wbu;

// The timer is read once every 1024 iterations so it doesn't dominate the
// benchmark. The value only has to be small enough to comfortably hit the
// one second log interval.
constexpr u64 LOG_CHECK_MASK = 0x3ff;

// Keep each field small enough that random mutation can match it quickly.
// The field values are deliberately not 0 or 0xfff because those are magic
// values that the fuzzing engine tries on every mutation.
constexpr u64 FIELD_MASK = u64{0xfff};

// Keep the compiler from collapsing the field checks into a single branch.
// If it did that, matching a prefix would stop producing new coverage and the
// fuzzer would lose the feedback that lets it find the bug incrementally.
inline void stage_barrier_() { __asm__ volatile("" ::: "memory"); }

__attribute__((no_builtin, no_sanitize("coverage"))) static void
push_str_(Fmt *fmt, char const *str, bool *fail) {
    fmt->push_str(CString::init_unsafe(str).as_slice(), fail);
}

__attribute__((no_builtin, no_sanitize("coverage"))) static u64
iterations_per_sec_(u64 iterations, u64 nanos) {
    if (nanos == 0) {
        return 0;
    }

    // Use microseconds as the unit so the multiplication below doesn't
    // overflow for any run that is short enough to reach this benchmark's
    // bug. Avoids 128 bit division, which pulls in a compiler runtime helper.
    u64 const micros = nanos / 1'000;
    if (micros == 0) {
        return mul_unsafe_u64(iterations, 1'000'000'000) / nanos;
    }
    return mul_unsafe_u64(iterations, 1'000'000) / micros;
}

__attribute__((no_builtin, no_sanitize("coverage"))) static void
log_progress_(Ctx *ctx) {
    u64 const total_nanos = ctx->m_total_timer.elapsed();
    u64 const interval_nanos = ctx->m_log_timer.lap();
    u64 const iterations = ctx->m_iterations;
    u64 const interval_iterations =
        sub_unsafe_u64(iterations, ctx->m_last_log_iterations);
    ctx->m_last_log_iterations = iterations;

    u64 const rate = iterations_per_sec_(interval_iterations, interval_nanos);

    u8 buf[256];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));
    bool fail = false;
    push_str_(&fmt, "fuzz_bench: elapsed_ms=", &fail);
    fmt.push_u64(total_nanos / 1'000'000, &fail);
    push_str_(&fmt, " iterations=", &fail);
    fmt.push_u64(iterations, &fail);
    push_str_(&fmt, " iterations/sec=", &fail);
    fmt.push_u64(rate, &fail);
    push_str_(&fmt, "\n", &fail);
    testing::assert_with_trace(!fail, &ctx->m_err_bt);
    write_bytes_(&ctx->m_err_bt, fmt.written());
}

__attribute__((no_builtin, no_sanitize("coverage"))) static void
maybe_log_(Ctx *ctx) {
    if (ctx->m_log_timer.elapsed() < LOG_INTERVAL_NANOS) {
        return;
    }
    log_progress_(ctx);
}

[[noreturn]] __attribute__((no_builtin, no_sanitize("coverage"))) static void
report_bug_(Ctx *ctx) {
    u64 const total_nanos = ctx->m_total_timer.elapsed();
    u64 const iterations = ctx->m_iterations;
    u64 const rate = iterations_per_sec_(iterations, total_nanos);

    u8 buf[256];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));
    bool fail = false;
    push_str_(&fmt, "fuzz_bench: BUG FOUND elapsed_ms=", &fail);
    fmt.push_u64(total_nanos / 1'000'000, &fail);
    push_str_(&fmt, " elapsed_ns=", &fail);
    fmt.push_u64(total_nanos, &fail);
    push_str_(&fmt, " iterations=", &fail);
    fmt.push_u64(iterations, &fail);
    push_str_(&fmt, " iterations/sec=", &fail);
    fmt.push_u64(rate, &fail);
    push_str_(&fmt, "\n", &fail);
    testing::assert_with_trace(!fail, &ctx->m_err_bt);
    write_bytes_(&ctx->m_err_bt, fmt.written());

    sys::exit_group(0);
}

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    Ctx *const ctx = (Ctx *)usr_ctx;

    u64 const iterations = add_unsafe_u64(ctx->m_iterations, 1);
    ctx->m_iterations = iterations;
    if ((iterations & LOG_CHECK_MASK) == 0) {
        maybe_log_(ctx);
    }

    u64 v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xe37}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0x9b9}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xf4a}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xc15}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0x39c}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xb8e}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0x4d1}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xc6f}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xd3b}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xe27}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0x3a9}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xf0d}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0x58c}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xe61}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0x7a2}) {
        return;
    }
    stage_barrier_();

    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0x6d5}) {
        return;
    }
    stage_barrier_();

    // The header was valid. The buggy path accepts this command without
    // checking it against the valid command range.
    v = fuzz_input->next();
    if ((v & FIELD_MASK) != u64{0xad1}) {
        return;
    }

    report_bug_(ctx);
}

static Alloc make_alloc(Backtrace *err_bt, u64 size) {
    u8 *mem = nullptr;
    testing::assert_with_trace(page_alloc::AllocStatus::Ok ==
                                   page_alloc::alloc_4kb(err_bt, size, &mem),
                               err_bt);

    return Alloc::init(SliceMut<u8>::init_unsafe(mem, size));
}

#endif // ZX_FUZZ

} // namespace

int main(void) {
    Ctx ctx;
    ctx.m_err_bt = Backtrace::init();
    ctx.m_iterations = 0;
    ctx.m_last_log_iterations = 0;

#ifdef ZX_FUZZ
    testing::assert_with_trace(
        TimeInfo::InitStatus::Ok ==
            TimeInfo::init(&ctx.m_err_bt, &ctx.m_total_timer.m_info),
        &ctx.m_err_bt);

    Alloc fuzz_alloc = make_alloc(&ctx.m_err_bt, FUZZ_MEM_CAP);

    // Start timing right before entering the fuzzing engine so the reported
    // time is the time spent fuzzing.
    ctx.m_total_timer = Timer::init(ctx.m_total_timer.m_info);
    ctx.m_log_timer = Timer::init(ctx.m_total_timer.m_info);

    fuzz(FuzzParams{
        .alloc = &fuzz_alloc,
        .usr_ctx = (void *)&ctx,
        .fuzz_one = fuzz_one,
    });

    u64 const total_nanos = ctx.m_total_timer.elapsed();
    u64 const rate = iterations_per_sec_(ctx.m_iterations, total_nanos);

    u8 buf[256];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));
    bool fail = false;
    push_str_(&fmt,
              "fuzz_bench: fuzz time limit reached without finding the bug. "
              "Use --fuzz-unlimited to keep running. elapsed_ms=",
              &fail);
    fmt.push_u64(total_nanos / 1'000'000, &fail);
    push_str_(&fmt, " iterations=", &fail);
    fmt.push_u64(ctx.m_iterations, &fail);
    push_str_(&fmt, " iterations/sec=", &fail);
    fmt.push_u64(rate, &fail);
    push_str_(&fmt, "\n", &fail);
    testing::assert_with_trace(!fail, &ctx.m_err_bt);
    write_bytes_(&ctx.m_err_bt, fmt.written());
#else
    static char const NOTICE[] =
        "fuzz_bench: fuzzing is disabled. Run this target with "
        "`zxbs --fuzz --fuzz-unlimited --opt-level=2 run zx:fuzz_bench`.\n";
    write_bytes_(&ctx.m_err_bt, Slice<u8>::init_unsafe((u8 const *)NOTICE,
                                                       sizeof(NOTICE) - 1));
#endif

    return 0;
}
