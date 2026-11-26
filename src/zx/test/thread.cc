import zx;

using namespace zx;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -E
// "__NR_sched_yield\b"
constexpr u64 NR_SCHED_YIELD = 24;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -E "__NR_exit\b"
constexpr u64 NR_EXIT = 60;

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

// terminate only the calling thread, exit_group would kill the whole test
// process.
[[noreturn]] static void exit_thread() {
    syscall1(NR_EXIT, 0);

    // the syscall never returns, this is only here for the noreturn
    // annotation.
    __builtin_trap();
}

// touch a large part of the stack to make sure the stack that spawn allocated
// is actually usable.
static bool touch_deep_stack() {
    volatile u8 buf[thread::STACK_SIZE / 2];

    for (u64 i = 0; i < (u64)sizeof(buf); i += 4096) {
        buf[i] = (u8)i;
    }

    for (u64 i = 0; i < (u64)sizeof(buf); i += 4096) {
        if (buf[i] != (u8)i) {
            return false;
        }
    }

    return true;
}

struct BasicCtx {
    u64_c done;
    u64 self_seen;
    u64 stack_probe;
    u64 deep_stack_ok;
    u64 ppid;
#ifdef ZX_DEBUG_INFO
    u64 altstack_ok;
#endif
};

// has to use the GNU attribute instead of [[noreturn]] because the noreturn
// attribute is part of the thread_fn function pointer type.
__attribute__((noreturn)) static void basic_fn(void *usr_ctx) {
    BasicCtx *ctx = (BasicCtx *)usr_ctx;

    u8 probe;
    ctx->self_seen = (u64)usr_ctx;
    ctx->stack_probe = (u64)&probe;
    ctx->deep_stack_ok = touch_deep_stack() ? 1 : 0;
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

    exit_thread();
}

static void test_invalid_args() {
    Backtrace err_bt = Backtrace::init();

    // a null thread function is rejected.
    assert(thread::SpawnStatus::Nullptr ==
           thread::spawn(&err_bt, nullptr, nullptr));
}

static void test_basic_spawn() {
    Backtrace err_bt = Backtrace::init();

    BasicCtx ctx = {};

    testing::assert_with_trace(thread::SpawnStatus::Ok ==
                                   thread::spawn(&err_bt, basic_fn, &ctx),
                               &err_bt);

    wait_value(&ctx.done, 1);

    // the thread got the exact context pointer that was passed in.
    assert(ctx.self_seen == (u64)&ctx);

    // the thread is in the same process so it sees the same parent.
    assert(ctx.ppid == sys::getppid());

    // the thread should have been able to use most of the stack that spawn
    // allocated for it.
    assert(ctx.deep_stack_ok != 0);

    // the thread has to run on its own stack, not on the stack of the thread
    // that spawned it.
    u8 main_probe;
    assert(ctx.stack_probe != (u64)&main_probe);

#ifdef ZX_DEBUG_INFO
    assert(ctx.altstack_ok != 0);
#endif

    // intentionally not freeing the stack: there is no join() yet so the
    // thread could still be between the flag store and its exit syscall, and
    // spawn owns the stack anyway. The process tears everything down when the
    // tests are done.
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

__attribute__((noreturn)) static void multi_fn(void *usr_ctx) {
    MultiItem *item = (MultiItem *)usr_ctx;

    __atomic_fetch_add(&item->shared->counter, 1, __ATOMIC_ACQ_REL);

    u8 probe;
    item->self_seen = (u64)usr_ctx;
    item->stack_probe = (u64)&probe;

    __atomic_fetch_add(&item->shared->done, 1, __ATOMIC_ACQ_REL);

    exit_thread();
}

static void test_multiple_threads() {
    Backtrace err_bt = Backtrace::init();

    MultiCtx shared = {};
    MultiItem items[N_THREADS] = {};

    for (u64 i = 0; i < N_THREADS; i++) {
        items[i].shared = &shared;

        testing::assert_with_trace(
            thread::SpawnStatus::Ok ==
                thread::spawn(&err_bt, multi_fn, &items[i]),
            &err_bt);
    }

    wait_value(&shared.done, N_THREADS);

    // every thread incremented the shared counter exactly once.
    assert(__atomic_load_n(&shared.counter, __ATOMIC_ACQUIRE) == N_THREADS);

    for (u64 i = 0; i < N_THREADS; i++) {
        // each thread got its own context.
        assert(items[i].self_seen == (u64)&items[i]);

        // and each one ran on its own stack.
        for (u64 j = 0; j < i; j++) {
            assert(items[i].stack_probe != items[j].stack_probe);
        }
    }

    // stacks are not freed for the same reason as in test_basic_spawn.
}

int main() {
    test_invalid_args();
    test_basic_spawn();
    test_multiple_threads();
}
