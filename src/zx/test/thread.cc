import zx;

using namespace zx;

namespace {

constexpr u64 N_THREADS = 4;

// the spawned thread always eventually runs, so the spin loop below always
// terminates. the limit is only there so that a broken implementation fails
// the test instead of hanging it forever. it is unreachable in practice.
constexpr u64 SPIN_LIMIT = u64{1} << 40;

// written by a spawned thread, read by the main thread. volatile so the
// accesses actually hit memory instead of being cached in registers.
struct ThreadResult {
    volatile u64 done;
    volatile u64 sig_stack_ptr;
    volatile u64 sig_stack_size;
    volatile u64 sig_stack_flags;
};

static void thread_fn(void *arg) {
    ThreadResult volatile *const result = (ThreadResult volatile *)arg;

    // query this thread's signal altstack, which also makes a syscall from
    // the spawned thread. sigaltstack is a per thread setting and a fresh
    // thread starts without one, so what we get back has to be the altstack
    // thread::spawn's entry set up for this thread.
    signal::Stack cur = signal::Stack{};
    assert(sys::SigAltStackStatus::Ok ==
           sys::sigaltstack(nullptr, nullptr, &cur));

    result->sig_stack_ptr = (u64)cur.ss_sp;
    result->sig_stack_size = cur.ss_size;
    result->sig_stack_flags = (u64)cur.ss_flags;

    // last write, signals that the thread is done
    result->done = 1;
}

} // namespace

int main() {
    Backtrace err_bt = Backtrace::init();

    // the main thread's own altstack (set up by start.cc in dev mode, unset
    // in release mode). every spawned thread must get its own altstack,
    // distinct from this one and from each other's.
    signal::Stack main_stack = signal::Stack{};
    assert(sys::SigAltStackStatus::Ok ==
           sys::sigaltstack(nullptr, nullptr, &main_stack));
    u64 const main_sig_stack_ptr = (u64)main_stack.ss_sp;

    ThreadResult results[N_THREADS];
    __builtin_memset(results, 0, sizeof(results));

    thread::Thread threads[N_THREADS];
    __builtin_memset(threads, 0, sizeof(threads));

    for (u64 i = 0; i < N_THREADS; i++) {
        testing::assert_with_trace(
            thread::SpawnStatus::Ok ==
                thread::spawn(&err_bt, thread_fn, (void *)&results[i],
                              &threads[i]),
            &err_bt);
        assert(threads[i].tid > 0);

        // every spawned thread gets its own thread id
        for (u64 j = 0; j < i; j++) {
            assert(threads[i].tid != threads[j].tid);
        }
    }

    for (u64 i = 0; i < N_THREADS; i++) {
        ThreadResult volatile *const result =
            (ThreadResult volatile *)&results[i];

        // the thread reports back by setting done. a store from a thread is
        // always eventually visible to the other threads' loads on the x86
        // memory model, so this is guaranteed to terminate.
        u64 spins = 0;
        while (result->done == 0) {
            spins++;
            if (spins >= SPIN_LIMIT) {
                panic();
            }
        }

        // the thread ran and reported back. check its sigaltstack:
        // enabled (a thread without an altstack reports SS_DISABLE)
        assert(result->sig_stack_flags == 0);
        // same size as the main thread's altstack in start.cc
        assert(result->sig_stack_size == thread::SIGNAL_STACK_SIZE);
        assert(result->sig_stack_ptr != 0);
        // distinct from the main thread's altstack
        assert(result->sig_stack_ptr != main_sig_stack_ptr);
        // distinct from every other spawned thread's altstack
        for (u64 j = 0; j < i; j++) {
            ThreadResult volatile const *const other =
                (ThreadResult volatile const *)&results[j];
            assert(result->sig_stack_ptr != other->sig_stack_ptr);
        }
    }

    return 0;
}
