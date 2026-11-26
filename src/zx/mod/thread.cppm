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

typedef void (*thread_fn)(void *usr_ctx);

constexpr u64 RED_ZONE_SIZE = 16;
constexpr u64 MIN_STACK_SIZE = u64{1} << 17;
constexpr u64 STACK_ALIGN = 16;

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
    StackTooSmall,
    StackUnaligned,
    UnexpectedErr,
    EAgain,
    EInval,
    ENoMem,
    EPerm,
};

inline SpawnStatus spawn(thread_fn fn, void *usr_ctx, SliceMut<u8> stack) {
    if (fn == nullptr) [[unlikely]] {
        return SpawnStatus::Nullptr;
    }

    if (!__builtin_is_aligned(stack.ptr(), STACK_ALIGN)) {
        return SpawnStatus::StackUnaligned;
    }

    if (MIN_STACK_SIZE > stack.len()) {
        return SpawnStatus::StackTooSmall;
    }

    // the stack grows down, use the highest 16 aligned address as the initial
    // stack pointer and reserve RED_ZONE_SIZE at the very top. the child's
    // entry function jumps to the user function without a return address on
    // the stack so the stack pointer has to be 16 aligned here.
    u64 const stack_len = stack.len() & ~(u64)(STACK_ALIGN - 1);
    u8 *stack_ptr =
        stack.offset_mut_unsafe(sub_unsafe_u64(stack_len, RED_ZONE_SIZE));

    // share the address space, filesystem info, file descriptor table and
    // signal handlers with the spawning thread and put the new thread in the
    // same thread group.
    u64 const flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
                      CLONE_THREAD | CLONE_SYSVSEM;

    register u64_c child_tid __asm__("r10") = 0;
    register u64_c tls __asm__("r8") = 0;

    u64_c res;
    __asm__ volatile(
        "syscall\n\t"
        // The parent gets the tid of the new thread, the child gets 0.
        "testq %%rax, %%rax\n\t"
        "jnz 1f\n\t"
        // Jump to the thread entry function implemented with assembly in
        // lib/thread.cc. It wraps the user function and provides the
        // necessary setup for the thread to run. It expects the user context
        // in rdi and the user function in rsi.
        "movq %[fn], %%rsi\n\t"
        "movq %[usr_ctx], %%rdi\n\t"
        "jmp zx_thread_entry_\n\t"
        "1:\n\t"
        : [res] "=a"(res)
        : "a"((u64_c)NR_CLONE), "D"((u64_c)flags), "S"((u64_c)stack_ptr),
          "d"(0), "r"(child_tid),
          "r"(tls), [fn] "r"((u64_c)fn), [usr_ctx] "r"((u64_c)usr_ctx)
        : "rcx", "r11", "memory");

    u64 const errno = sys::errno_(res);
    if (errno != sys::SUCCESS) [[unlikely]] {
        switch (errno) {
        case sys::EAGAIN: {
            return SpawnStatus::EAgain;
        }
        case sys::EINVAL: {
            return SpawnStatus::EInval;
        }
        case sys::ENOMEM: {
            return SpawnStatus::ENoMem;
        }
        case sys::EPERM: {
            return SpawnStatus::EPerm;
        }
        default: {
            return SpawnStatus::UnexpectedErr;
        }
        }
    }

    return SpawnStatus::Ok;
}

} // namespace thread

} // namespace zx
