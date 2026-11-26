module zx;

namespace zx {

#ifdef ZX_DEBUG_INFO
extern "C" void zx_setup_sigaltstack_() {
    Backtrace err_bt = Backtrace::init();

    constexpr u64 STACK_SIZE = u64{1} << 20;

    u8 *mem = nullptr;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_bt, STACK_SIZE, &mem),
        &err_bt);

    assert(mem != nullptr);
    assert(__builtin_is_aligned(mem, STACK_ALIGN));

    signal::Stack const signal_stack = signal::Stack{
        .ss_sp = mem,
        .ss_flags = 0,
        .ss_size = STACK_SIZE,
    };
    testing::assert_with_trace(
        sys::SigAltStackStatus::Ok ==
            sys::sigaltstack(&err_bt, &signal_stack, nullptr),
        &err_bt);
}
#else  // ifdef ZX_DEBUG_INFO
extern "C" void zx_setup_sigaltstack_() {}
#endif // ifdef ZX_DEBUG_INFO

__asm__(
    ".text\n"
    ".type zx_thread_entry_, @function\n"
    ".global zx_thread_entry_\n"
    "zx_thread_entry_:\n\t"
    ".cfi_startproc\n\t"
    // stop eh_frame based unwinder from going past this function frame
    // since there is nothing above.
    ".cfi_undefined %rip\n\t"
    // mark the first frame by setting rbp to 0
    "xorq %rbp, %rbp\n\t"
    // save these for calling user function later.
    // we are pushing 16 bytes here so we will keep the stack alignment for setup call below.
    "pushq %rdi\n\t"
    "pushq %rsi\n\t"
    // setup sigaltstack
    "callq zx_setup_sigaltstack_\n\t"
    // poping values to prep for calling user function, we pop 16 bytes so we keep stack alignment valid.
    "popq %rsi\n\t"
    "popq %rdi\n\t"
    // call user function, the user context argument should already be in rdi so don't need to touch that.
    "callq *%rsi\n\t"
    // set exit() code to 0
    "xorq %rdi, %rdi\n\t"
    // exit(), syscall number for exit is 60
    "movq $60, %rax\n\t"
    "syscall\n\t"
    // will never reach here becasue of the exit() above but just put it here as a safeguard
    "ud2\n\t"
    // mark function boundary for eh_frame debug info
    ".cfi_endproc\n\t"
    ".size zx_thread_entry_, .-zx_thread_entry_\n\t");

constexpr u64 RED_ZONE_SIZE = 16;
constexpr u64 MIN_STACK_SIZE = u64{1} << 17;

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

inline SpawnStatus spawn(thread_fn, void *usr_ctx, SliceMut<u8> stack) {
    if (!__builtin_is_aligned(stack.ptr(), STACK_ALIGN)) {
        return SpawnStatus::StackUnaligned;
    }

    if (MIN_STACK_SIZE > stack.len()) {
        return SpawnStatus::StackTooSmall;
    }

    u64 const stack_offset = sub_unsafe_u64(stack.len(), RED_ZONE_SIZE + sizeof(u64));
    u8* stack_ptr = stack.offset_unsafe(stack_offset);

    // // echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -E "__NR_(clone|exit)\b"
    // constexpr u64 NR_CLONE = 56;
    // constexpr u64 NR_EXIT = 60;

    u64_c res;
    __asm__ volatile(
        "syscall\n\t"
        "testq %rax, %rax\n\t"
        "jnz 1f\n\t"
        "callq %[thread_fn]\n\t"
        "1:\n\t"
        : [res]"=a"(res)
        : "a"(NR_CLONE), [stack_ptr] "r"(stack_ptr)
        : "memory"
    );

    return SpawnStatus::Ok;
}

} // namespace zx
