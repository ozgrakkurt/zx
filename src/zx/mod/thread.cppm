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

constexpr u64 STACK_ALLOC_SIZE = u64{1} << 20;
constexpr u64 STACK_ALIGN = 16;
constexpr u64 GUARD_PAGE_SIZE = u64{1} << 12;
// usable stack size
constexpr u64 STACK_SIZE = STACK_ALLOC_SIZE - 2__wbu * GUARD_PAGE_SIZE;

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

enum class [[nodiscard]] SetupStackStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    AllocFail,
};

inline void free_stack_mem_(u8 *stack_mem) {
    if (stack_mem != nullptr) {
        page_alloc::FreeStatus const status =
            page_alloc::free(nullptr, stack_mem, STACK_ALLOC_SIZE);
        assert_dev(page_alloc::FreeStatus::Ok == status);
    }
}

// Sets up a stack with this layout:
// [guard page] ---- stack ---- [guard page]
//                <-- stack grows down
//
// Size of the usable stack space is STACK_SIZE
// Size of the total allocation is STACK_ALLOC_SIZE
inline SetupStackStatus setup_stack(Backtrace *err_bt, u8 **stack_mem_out,
                                    u8 **stack_ptr_out) {
    if (stack_ptr_out == nullptr || stack_mem_out == nullptr) {
        Backtrace::collect(err_bt);
        return SetupStackStatus::Nullptr;
    }

    u8 *stack_mem = nullptr;
    if (page_alloc::AllocStatus::Ok !=
        page_alloc::alloc_4kb(err_bt, STACK_ALLOC_SIZE, &stack_mem))
        [[unlikely]] {
        return SetupStackStatus::AllocFail;
    }

    if (stack_mem == nullptr) [[unlikely]] {
        free_stack_mem_(stack_mem);
        Backtrace::collect(err_bt);
        return SetupStackStatus::UnexpectedErr;
    }

    if (!__builtin_is_aligned(stack_mem, STACK_ALIGN)) {
        free_stack_mem_(stack_mem);
        Backtrace::collect(err_bt);
        return SetupStackStatus::UnexpectedErr;
    }

    if (sys::MProtectStatus::Ok !=
        sys::mprotect(err_bt, stack_mem, GUARD_PAGE_SIZE, sys::PROT_NONE)) {
        free_stack_mem_(stack_mem);
        return SetupStackStatus::UnexpectedErr;
    }

    u64 const stack_offset = sub_unsafe_u64(STACK_ALLOC_SIZE, GUARD_PAGE_SIZE);
    u8 *stack_ptr = &stack_mem[stack_offset];

    if (sys::MProtectStatus::Ok !=
        sys::mprotect(err_bt, stack_ptr, GUARD_PAGE_SIZE, sys::PROT_NONE)) {
        free_stack_mem_(stack_mem);
        return SetupStackStatus::UnexpectedErr;
    }

    if (!__builtin_is_aligned(stack_ptr, STACK_ALIGN)) {
        free_stack_mem_(stack_mem);
        Backtrace::collect(err_bt);
        return SetupStackStatus::UnexpectedErr;
    }

    *stack_ptr_out = stack_ptr;
    *stack_mem_out = stack_mem;
    return SetupStackStatus::Ok;
}

enum class [[nodiscard]] SpawnStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAgain,
    EInval,
    ENoMem,
    EPerm,
    SetupStackFail,
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
    u8 *stack_ptr = nullptr;
    if (SetupStackStatus::Ok != setup_stack(err_bt, &stack_mem, &stack_ptr)) {
        free_stack_mem_(stack_mem);
        return SpawnStatus::SetupStackFail;
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

        // cleanup the allocated memory since spawning failed
        free_stack_mem_(stack_mem);

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
