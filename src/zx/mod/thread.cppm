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

enum class [[nodiscard]] SpawnStatus : u8_c {
    Ok = 0,
    StackTooSmall,
    StackUnaligned,
    UnexpectedErr,
    EAgain,
    EInval,
    ENoMem,
    EPerm,
};

inline SpawnStatus spawn(thread_fn, void *usr_ctx, SliceMut<u8> stack) {
    if (!__builtin_is_aligned(stack.ptr(), STACK_ALIGN)) {
        return SpawnStatus::StackUnaligned;
    }

    if (MIN_STACK_SIZE > stack.len()) {
        return SpawnStatus::StackTooSmall;
    }

    u64 const stack_offset = sub_unsafe_u64(stack.len(), RED_ZONE_SIZE);
    u8 *stack_ptr = stack.offset_unsafe(stack_offset);

    u64_c res;
    __asm__ volatile("syscall\n\t"
                     "testq %rax, %rax\n\t"
                     "jnz 1f\n\t"
                     // jumpt to thread entry function implemented with assembly
                     // in lib/thread.cc this wraps the user function and
                     // provides necessary setup for the thread to run.
                     "jmpq zx_thread_entry_\n\t"
                     "1:\n\t"
                     : [res] "=a"(res)
                     : "a"(NR_CLONE), [stack_ptr] "r"(stack_ptr)
                     : "memory");

    return SpawnStatus::Ok;
}

} // namespace thread

} // namespace zx
