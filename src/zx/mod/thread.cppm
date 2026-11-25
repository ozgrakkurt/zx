// Thread spawning on top of the raw clone syscall.
//
// The spawned thread shares the spawning thread's address space (CLONE_VM) but
// none of the allocators in this codebase (Alloc, Slab, ..) are thread safe.
// Allocate everything a thread needs before spawning it and pass it in `arg`.

export module zx:thread;
import :sys;
import :signal;
import :page_alloc;
import :def;
import :backtrace;
import :assert;

export namespace zx {

namespace thread {

// Size of the execution stack of a spawned thread. Has to be a power of two
// page multiple since it is mmap-ed through page_alloc.
constexpr u64 STACK_SIZE = u64{1} << 20;

// Size of the signal (alternate) stack of a spawned thread. This is the same
// 1MB size start.cc uses for the main thread's signal stack. A spawned thread
// doesn't inherit the main thread's sigaltstack setting since sigaltstack is a
// per thread setting, so we set it up for every thread we spawn.
constexpr u64 SIGNAL_STACK_SIZE = u64{1} << 20;

typedef void (*Fn)(void *arg);

struct Thread {
    // the gettid of the spawned thread
    i32 tid;
};

// Private to this module.
//
// Written by the spawning thread right before the clone syscall and read by
// the spawned thread at its start. It sits at the base (lowest addresses) of
// the spawned thread's execution stack, which the stack can only overwrite by
// growing a full STACK_SIZE deep.
struct ChildCtx_ {
    Fn fn;
    void *arg;
    signal::Stack sig_stack;
};

// First thing that runs on the spawned thread's brand new stack. Never
// returns.
[[noreturn]] inline void child_entry_(void *child_ctx_) {
    ChildCtx_ const *const child_ctx = (ChildCtx_ const *)child_ctx_;

    // sigaltstack is a per thread setting and a freshly spawned thread starts
    // without one. Set it up exactly the way start.cc sets it up for the main
    // thread so the SA_ONSTACK signal handlers (the SIGILL/SIGSEGV stack
    // trace printers in dev mode) work on this thread too.
    assert(sys::SigAltStackStatus::Ok ==
           sys::sigaltstack(nullptr, &child_ctx->sig_stack, nullptr));

    child_ctx->fn(child_ctx->arg);

    // the thread function is done, only this thread exits, the rest of the
    // process keeps running. there is no join implementation (yet), so the
    // thread's stacks stay mapped for the rest of the process' life.
    sys::exit(0);
}

enum class [[nodiscard]] SpawnStatus : u8_c {
    Ok = 0,
    Nullptr,
    // the mmap for the spawned thread's execution stack failed.
    StackAlloc,
    // the mmap for the spawned thread's signal stack failed.
    SignalStackAlloc,
    // the clone syscall itself failed.
    Clone,
};

inline SpawnStatus spawn(Backtrace *err_bt, Fn fn, void *arg, Thread *out) {
    if (fn == nullptr || out == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return SpawnStatus::Nullptr;
    }

    // the thread stacks are mmap-ed instead of being carved out of the arena
    // allocators since the allocators are not thread safe, and because these
    // pages have to stay mapped for the spawned thread's whole life, which
    // outlives this function call.
    u8 *stack = nullptr;
    if (page_alloc::AllocStatus const status =
            page_alloc::alloc_4kb(err_bt, STACK_SIZE, &stack);
        page_alloc::AllocStatus::Ok != status) [[unlikely]] {
        return SpawnStatus::StackAlloc;
    }

    u8 *sig_stack = nullptr;
    if (page_alloc::AllocStatus const status =
            page_alloc::alloc_4kb(err_bt, SIGNAL_STACK_SIZE, &sig_stack);
        page_alloc::AllocStatus::Ok != status) [[unlikely]] {
        return SpawnStatus::SignalStackAlloc;
    }

    // stage the child context at the base of the child's execution stack.
    // the spawning thread is still the only thread running so there is no
    // race writing to it.
    ChildCtx_ *const child_ctx = (ChildCtx_ *)stack;
    child_ctx->fn = fn;
    child_ctx->arg = arg;
    child_ctx->sig_stack = signal::Stack{
        .ss_sp = sig_stack,
        .ss_flags = 0,
        .ss_size = SIGNAL_STACK_SIZE,
    };

    // stage the child entry function and the child context pointer on the
    // very top of the child's execution stack. sys::clone's child path pops
    // both of them and calls the entry function with the context pointer.
    u8 *const child_sp = &stack[STACK_SIZE - 16];
    *(Fn *)child_sp = child_entry_;
    *(ChildCtx_ **)&child_sp[8] = child_ctx;

    // spawn the thread as part of the same process (thread group) instead of
    // a separate process:
    //   - CLONE_VM: share the address space.
    //   - CLONE_FS: share the filesystem info (cwd, root, umask).
    //   - CLONE_FILES: share the file descriptor table.
    //   - CLONE_SIGHAND: share the signal handlers, so the handlers set up in
    //     start.cc apply to this thread too. required by CLONE_THREAD.
    //   - CLONE_THREAD: same thread group. the spawning thread stays alive
    //     even if the spawned thread dies.
    //   - CLONE_SYSVSEM: share the system v semaphore undo state.
    u64 const flags = sys::CLONE_VM | sys::CLONE_FS | sys::CLONE_FILES |
                      sys::CLONE_SIGHAND | sys::CLONE_THREAD |
                      sys::CLONE_SYSVSEM;

    i32 tid = -1;
    if (sys::CloneStatus const status =
            sys::clone(err_bt, flags, child_sp, nullptr, nullptr, 0, &tid);
        sys::CloneStatus::Ok != status) [[unlikely]] {
        // the child never came to life so the stacks are only ours, unmap
        // them
        assert(page_alloc::FreeStatus::Ok ==
               page_alloc::free(err_bt, stack, STACK_SIZE));
        assert(page_alloc::FreeStatus::Ok ==
               page_alloc::free(err_bt, sig_stack, SIGNAL_STACK_SIZE));

        return SpawnStatus::Clone;
    }

    out->tid = tid;
    return SpawnStatus::Ok;
}

} // namespace thread

} // namespace zx
