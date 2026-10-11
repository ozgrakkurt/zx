import zx;

using namespace zx;

#ifdef ZX_DEBUG_INFO

static void signal_handler(i32_c signum, sys::SigInfo *info, void *ucontext) {
    (void)info;

    assert(signum == (i32)sys::SIG::ILL || signum == (i32)sys::SIG::SEGV);

    sys::UContext const *uctx = (sys::UContext const *)ucontext;

    CPUContext cpu_ctx = CPUContext{
        .r12 = uctx->uc_mcontext.r12,
        .r13 = uctx->uc_mcontext.r13,
        .r14 = uctx->uc_mcontext.r14,
        .r15 = uctx->uc_mcontext.r15,
        .rbp = uctx->uc_mcontext.rbp,
        .rbx = uctx->uc_mcontext.rbx,
        .rsp = uctx->uc_mcontext.rsp,
        // do this since this is the exact address of the instruction that
        // panicked but the stack trace printer will subtract 1 from RIP since
        // it expects the RIP to be the return address.
        //
        // If RIP is a return address then subtracting 1 from it will give the
        // call instruction which is what the stack trace printer wants, but
        // this case is special since RIP isn't a return address, it is the
        // exact instruction that panicked.
        .rip = add_unsafe_u64(uctx->uc_mcontext.rip, 1),
    };

    elf::print_stack_trace_panic(&cpu_ctx);

    // control will fall back to the instruction that raised the signal,
    // it will raise the same signal again and the default handler for that
    // signal will run, which should cause the kernel to end the process with
    // the correspoding error code.
}

extern "C" void zx_setup_() {
    sys::SigAction act = sys::SigAction{
        .sigaction = signal_handler,
        .flags = sys::SA_SIGINFO | sys::SA_RESETHAND | sys::SA_NODEFER,
        .mask = sys::SigSet::init(),
    };

    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(sys::SIG::ILL, &act, nullptr));
    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(sys::SIG::SEGV, &act, nullptr));
}

#else // ifdef ZX_DEBUG_INFO

extern "C" void zx_setup_() {}

#endif

__asm__(".text\n"
        ".type _start, @function\n"
        ".global _start\n"
        "_start:\n\t"
        ".cfi_startproc\n\t"
        // stop eh_frame based unwinder from going past this function frame
        // since there is nothing above.
        ".cfi_undefined %rip\n\t"
        "xorq %rbp, %rbp\n\t"
        "popq %rdi\n\t"
        "movq %rsp, %rsi\n\t"
        "andq $-16, %rsp\n\t"
        "pushq %rdi\n\t"
        "pushq %rsi\n\t"
        "callq zx_setup_\n\t"
        "popq %rsi\n\t"
        "popq %rdi\n\t"
        "callq main\n\t"
        "movq %rax, %rdi\n\t"
        "movq $60, %rax\n\t"
        "syscall\n\t"
        ".cfi_endproc\n\t");
