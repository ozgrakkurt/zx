module zx;

using namespace zx;

#ifdef ZX_DEBUG_INFO
extern "C" void zx_setup_sigaltstack_() {
    Backtrace err_bt = Backtrace::init();

    u8 *stack_mem = nullptr;
    u8 *stack_ptr = nullptr;
    testing::assert_with_trace(
        thread::SetupStackStatus::Ok ==
            thread::setup_stack(&err_bt, &stack_mem, &stack_ptr),
        &err_bt);
    (void)stack_mem;

    signal::Stack const signal_stack = signal::Stack{
        .ss_sp = (void *)sub_unsafe_u64((u64)stack_ptr, thread::STACK_SIZE),
        .ss_flags = 0,
        .ss_size = thread::STACK_SIZE,
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
        // rbx is callee-saved so preserve the caller's value. the child never
        // returns to the caller so it doesn't need to pop it.
        "pushq %rbx\n\t"
        ".cfi_adjust_cfa_offset 8\n\t"
        // the entry CFA is rsp + 8 because rsp points at the return address
        // that the call pushed. after this push rsp moves down by 8 so the
        // CFA becomes rsp + 16 and the saved rbx is at CFA - 16.
        ".cfi_offset %rbx, -16\n\t"
        // clone(2) expects (flags, stack, parent_tid, child_tid, tls) in
        // (rdi, rsi, rdx, r10, r8). flags is already in rdi and stack is
        // already in rsi. save usr_fn and usr_ctx because rdx/rcx are needed
        // for the clone arguments and syscall clobbers rcx.
        "movq %rdx, %r9\n\t"
        "movq %rcx, %rbx\n\t"
        // parent_tid = nullptr
        "xorq %rdx, %rdx\n\t"
        // child_tid = nullptr
        "xorq %r10, %r10\n\t"
        // tls = 0
        "xorq %r8, %r8\n\t"
        // syscall number for clone is 56
        "movq $56, %rax\n\t"
        "syscall\n\t"
        // The parent gets the tid of the new thread, the child gets 0.
        "testq %rax, %rax\n\t"
        "jz 1f\n\t"
        // parent: return the tid (or -errno) to the caller.
        "popq %rbx\n\t"
        ".cfi_adjust_cfa_offset -8\n\t"
        ".cfi_restore %rbx\n\t"
        "retq\n\t"
        "1:\n\t"
        // the child never returns through this frame and its stack does not
        // contain the return address and saved rbx described above, so mark
        // this as the outermost frame. otherwise an unwinder would read past
        // the child's stack into the top guard page.
        ".cfi_undefined %rip\n\t"
        // child: enter the thread trampoline with the user context in rdi and
        // the user function in rsi.
        "movq %r9, %rsi\n\t"
        "movq %rbx, %rdi\n\t"
        "jmp zx_thread_entry_\n\t"
        ".cfi_endproc\n\t"
        ".size zx_spawn_thread_, .-zx_spawn_thread_\n\t");
