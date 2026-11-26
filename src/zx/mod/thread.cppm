export module zx:thread;
import :signal;
import :page_alloc;
import :def;
import :sys;
import :slice;
import :backtrace;
import :testing;

export namespace zx {

namespace thread {

constexpr u64 RED_ZONE_SIZE = 16;
constexpr u64 STACK_SIZE = u64{1} << 20;
constexpr u64 STACK_ALIGN = 16;
constexpr u64 GUARD_PAGE_SIZE = u64{1} << 10;

// echo '#include <linux/sched.h>' | gcc -E -dM -xc - | grep -E "#define CLONE_"
constexpr u64 CLONE_VM = 0x00000100;
constexpr u64 CLONE_FS = 0x00000200;
constexpr u64 CLONE_FILES = 0x00000400;
constexpr u64 CLONE_SIGHAND = 0x00000800;
constexpr u64 CLONE_THREAD = 0x00010000;
constexpr u64 CLONE_SYSVSEM = 0x00040000;
constexpr u64 CLONE_SETTLS = 0x00080000;
constexpr u64 CLONE_PARENT_SETTID = 0x00100000;
constexpr u64 CLONE_CHILD_CLEARTID = 0x00200000;
constexpr u64 CLONE_CHILD_SETTID = 0x01000000;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -E "__NR_clone\b"
constexpr u64 NR_CLONE = 56;

enum class [[nodiscard]] SpawnStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAgain,
    EInval,
    ENoMem,
    EPerm,
    StackAllocFail,
};

// This is marked noreturn since we expect spawned threads to always run until
// the program ends.
typedef void (*thread_fn)(void *usr_ctx) __attribute__((noreturn));

// This is implemented in lib/thread.cc
extern "C" u64 zx_spawn_thread_(u64 flags, u64 stack_ptr, u64 usr_fn,
                                u64 usr_ctx);

// It is expected that spawned threads will run until the program ends.
inline SpawnStatus spawn(Backtrace *err_bt, thread_fn usr_fn, void *usr_ctx) {

    if (usr_fn == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return SpawnStatus::Nullptr;
    }

    u8 *stack_mem = nullptr;
    if (page_alloc::AllocStatus::Ok !=
        page_alloc::alloc_4kb(err_bt, STACK_SIZE, &stack_mem)) [[unlikely]] {
        return SpawnStatus::StackAllocFail;
    }

    if (stack_mem == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return SpawnStatus::UnexpectedErr;
    }

    if (!__builtin_is_aligned(stack_mem, STACK_ALIGN)) {
        Backtrace::collect(err_bt);
        return SpawnStatus::UnexpectedErr;
    }

    // [guard page] ---- stack ---- [red zone]
    //                <-- stack grows down

    if (sys::MProtectStatus::Ok !=
        sys::mprotect(err_bt, stack_mem, GUARD_PAGE_SIZE, sys::PROT_NONE)) {
        return SpawnStatus::UnexpectedErr;
    }

    u64 const stack_offset = sub_unsafe_u64(STACK_SIZE, RED_ZONE_SIZE);
    u8 *stack_ptr = &stack_mem[stack_offset];

    if (!__builtin_is_aligned(stack_ptr, STACK_ALIGN)) {
        Backtrace::collect(err_bt);
        return SpawnStatus::UnexpectedErr;
    }

    // share the address space, filesystem info, file descriptor table and
    // signal handlers with the spawning thread and put the new thread in the
    // same thread group.
    u64 const flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
                      CLONE_THREAD | CLONE_SYSVSEM;

    u64 const res =
        zx_spawn_thread_(flags, (u64)stack_ptr, (u64)usr_fn, (u64)usr_ctx);
    u64 const errno = sys::errno_(res);
    if (errno != sys::SUCCESS) [[unlikely]] {
        switch (errno) {
        case sys::EAGAIN: {
            Backtrace::collect(err_bt);
            return SpawnStatus::EAgain;
        }
        case sys::EINVAL: {
            Backtrace::collect(err_bt);
            return SpawnStatus::EInval;
        }
        case sys::ENOMEM: {
            Backtrace::collect(err_bt);
            return SpawnStatus::ENoMem;
        }
        case sys::EPERM: {
            Backtrace::collect(err_bt);
            return SpawnStatus::EPerm;
        }
        default: {
            Backtrace::collect(err_bt);
            return SpawnStatus::UnexpectedErr;
        }
        }
    }

    return SpawnStatus::Ok;
}

} // namespace thread

} // namespace zx
