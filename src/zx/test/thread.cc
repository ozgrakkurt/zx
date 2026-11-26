import zx;

using namespace zx;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -E
// "__NR_sched_yield\b"
constexpr u64 NR_SCHED_YIELD = 24;

// 256KiB, has to be a power of two for page_alloc::alloc_4kb.
constexpr u64 STACK_SIZE = u64{1} << 18;

// a spawned thread should start running almost immediately, if it doesn't run
// within this many yields then something is wrong.
constexpr u64 SPIN_LIMIT = 100000;

// wait until value becomes expected without burning cpu. panics instead of
// hanging forever if the thread never runs.
static void wait_value(u64_c const *value, u64_c expected) {
    for (u64 i = 0; i < SPIN_LIMIT; i++) {
        if (__atomic_load_n(value, __ATOMIC_ACQUIRE) == expected) {
            return;
        }

        // yield so a spawned thread gets cpu time even on a machine with fewer
        // cpus than threads.
        syscall0(NR_SCHED_YIELD);
    }

    assert(false);
}

struct BasicCtx {
    u64_c done;
    u64 self_seen;
    u64 stack_probe;
    u64 ppid;
#ifdef ZX_DEBUG_INFO
    u64 altstack_ok;
#endif
};

static void basic_fn(void *usr_ctx) {
    BasicCtx *ctx = (BasicCtx *)usr_ctx;

    u8 probe;
    ctx->self_seen = (u64)usr_ctx;
    ctx->stack_probe = (u64)&probe;
    ctx->ppid = sys::getppid();

#ifdef ZX_DEBUG_INFO
    // thread entry should have installed a per-thread alternate signal stack.
    signal::Stack old_stack = {};
    Backtrace err_bt = Backtrace::init();
    ctx->altstack_ok = (sys::SigAltStackStatus::Ok ==
                        sys::sigaltstack(&err_bt, nullptr, &old_stack)) &&
                       old_stack.ss_sp != nullptr && old_stack.ss_flags == 0 &&
                       old_stack.ss_size >= 4096;
#endif

    __atomic_store_n(&ctx->done, 1, __ATOMIC_RELEASE);
}

static void test_invalid_args() {
    Backtrace err_bt = Backtrace::init();

    u8 *mem = nullptr;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_bt, STACK_SIZE, &mem),
        &err_bt);

    // a null thread function is rejected.
    assert(thread::SpawnStatus::Nullptr ==
           thread::spawn(&err_bt, nullptr, nullptr,
                         SliceMut<u8>::init_unsafe(mem, STACK_SIZE)));

    // an unaligned stack start is rejected, keep the slice inside the mapping.
    assert(thread::SpawnStatus::StackUnaligned ==
           thread::spawn(&err_bt, basic_fn, nullptr,
                         SliceMut<u8>::init_unsafe(
                             mem + 1, sub_unsafe_u64(STACK_SIZE, 1))));

    // anything smaller than MIN_STACK_SIZE is rejected.
    assert(thread::SpawnStatus::StackTooSmall ==
           thread::spawn(&err_bt, basic_fn, nullptr,
                         SliceMut<u8>::init_unsafe(
                             mem, sub_unsafe_u64(thread::MIN_STACK_SIZE,
                                                 thread::STACK_ALIGN))));

    // no thread was created by any of the calls above so it is safe to unmap.
    testing::assert_with_trace(page_alloc::FreeStatus::Ok ==
                                   page_alloc::free(&err_bt, mem, STACK_SIZE),
                               &err_bt);
}

static void test_basic_spawn() {
    Backtrace err_bt = Backtrace::init();

    u8 *mem = nullptr;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_bt, STACK_SIZE, &mem),
        &err_bt);

    // use a length that is not a multiple of STACK_ALIGN so spawn has to round
    // it down without going past the end of the mapping.
    u64 const stack_len = sub_unsafe_u64(STACK_SIZE, 8);

    BasicCtx ctx = {};

    testing::assert_with_trace(
        thread::SpawnStatus::Ok ==
            thread::spawn(&err_bt, basic_fn, &ctx,
                          SliceMut<u8>::init_unsafe(mem, stack_len)),
        &err_bt);

    wait_value(&ctx.done, 1);

    // the thread got the exact context pointer that was passed in.
    assert(ctx.self_seen == (u64)&ctx);

    // the thread is in the same process so it sees the same parent.
    assert(ctx.ppid == sys::getppid());

    // the thread ran inside the stack slice that was passed in. the stack grows
    // down so the entry frame has to be near the top.
    assert(ctx.stack_probe >= (u64)mem);
    assert(ctx.stack_probe < (u64)mem + stack_len);
    assert(ctx.stack_probe >= (u64)mem + stack_len - (u64{1} << 16));

#ifdef ZX_DEBUG_INFO
    assert(ctx.altstack_ok != 0);
#endif

    // intentionally not unmapping the stack: there is no join() yet so the
    // thread could still be between the flag store and its exit syscall. The
    // process tears everything down when the tests are done anyway.
}

constexpr u64 N_THREADS = 3;

struct MultiCtx {
    u64_c counter;
    u64_c done;
};

struct MultiItem {
    MultiCtx *shared;
    u64 self_seen;
    u64 stack_probe;
};

static void multi_fn(void *usr_ctx) {
    MultiItem *item = (MultiItem *)usr_ctx;

    __atomic_fetch_add(&item->shared->counter, 1, __ATOMIC_ACQ_REL);

    u8 probe;
    item->self_seen = (u64)usr_ctx;
    item->stack_probe = (u64)&probe;

    __atomic_fetch_add(&item->shared->done, 1, __ATOMIC_ACQ_REL);
}

static void test_multiple_threads() {
    Backtrace err_bt = Backtrace::init();

    MultiCtx shared = {};
    MultiItem items[N_THREADS] = {};
    u8 *stacks[N_THREADS] = {};

    for (u64 i = 0; i < N_THREADS; i++) {
        testing::assert_with_trace(
            page_alloc::AllocStatus::Ok ==
                page_alloc::alloc_4kb(&err_bt, STACK_SIZE, &stacks[i]),
            &err_bt);

        items[i].shared = &shared;

        testing::assert_with_trace(
            thread::SpawnStatus::Ok ==
                thread::spawn(&err_bt, multi_fn, &items[i],
                              SliceMut<u8>::init_unsafe(stacks[i], STACK_SIZE)),
            &err_bt);
    }

    wait_value(&shared.done, N_THREADS);

    // every thread incremented the shared counter exactly once.
    assert(__atomic_load_n(&shared.counter, __ATOMIC_ACQUIRE) == N_THREADS);

    for (u64 i = 0; i < N_THREADS; i++) {
        // each thread got its own context.
        assert(items[i].self_seen == (u64)&items[i]);

        // and ran on the stack that was passed for it.
        assert(items[i].stack_probe >= (u64)stacks[i]);
        assert(items[i].stack_probe < (u64)stacks[i] + STACK_SIZE);
    }

    // stacks are not unmapped for the same reason as in test_basic_spawn.
}

int main() {
    test_invalid_args();
    test_basic_spawn();
    test_multiple_threads();
}
