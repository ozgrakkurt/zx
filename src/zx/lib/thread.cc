module zx;

using namespace zx;

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
    assert(__builtin_is_aligned(mem, thread::STACK_ALIGN));

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

__asm__(".text\n"
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
        // we are pushing 16 bytes here so we will keep the stack alignment for
        // setup call below.
        "pushq %rdi\n\t"
        "pushq %rsi\n\t"
        // setup sigaltstack
        "callq zx_setup_sigaltstack_\n\t"
        // poping values to prep for calling user function, we pop 16 bytes so
        // we keep stack alignment valid.
        "popq %rsi\n\t"
        "popq %rdi\n\t"
        // call user function, the user context argument should already be in
        // rdi so don't need to touch that.
        "callq *%rsi\n\t"
        // will never reach here because the user thread function has to be
        // noreturn.
        "ud2\n\t"
        // mark function boundary for eh_frame debug info
        ".cfi_endproc\n\t"
        ".size zx_thread_entry_, .-zx_thread_entry_\n\t");

// extern "C" u64 zx_spawn_thread_(u64 flags, u64 stack_ptr, u64 usr_fn, u64
// usr_ctx);
__asm__(".text\n"
        ".type zx_spawn_thread_, @function\n"
        ".global zx_spawn_thread_\n"
        "zx_spawn_thread_:\n\t"
        ".cfi_startproc\n\t"
        // child_tid
        "xorq %r10, %r10\n\t"
        // tls
        "xorq %r8, %r8\n\t"
        // syscall number for clone is 56
        "movq $56, %rax\n\t"
        "syscall\n\t"
        // The parent gets the tid of the new thread, the child gets 0.
        "testq %%rax, %%rax\n\t"
        "jnz 1f\n\t"
        ""
        "1:\n\t"
        "retq\n\t"
        ".cfi_endproc\n\t"
        ".size zx_spawn_thread_, .-zx_spawn_thread_\n\t");
);
