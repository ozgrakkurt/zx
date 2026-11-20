import zx;

using namespace zx;

typedef void (*init_fn_t)(void);

extern init_fn_t __init_array_start[] __attribute__((weak));
extern init_fn_t __init_array_end[] __attribute__((weak));

static void run_init_array_() {
    init_fn_t const *start = __init_array_start;
    init_fn_t const *end = __init_array_end;

    if (start == nullptr) {
        assert(end == nullptr);
        return;
    } else {
        assert(end != nullptr);
    }

    assert(start <= end);

    for (init_fn_t const *p = start; p < end; ++p) {
        if (*p != nullptr)
            (*p)();
    }
}

#ifdef ZX_DEBUG_INFO

// The curlies are to indicate that these are actual definitions are the
// variables and not just declarations.
//
// We don't run global constructors since we don't use constructors at all in
// this codebase.
//
// These values are expected to be just 0 initialized at the point of program
// entry into _start.
extern "C" elf::EhFrame ZX_ELF_EH_FRAME_{};
extern "C" elf::DebugLine ZX_ELF_DEBUG_LINE_{};

static Slice<u8> read_self_exe_panic_(Alloc *alloc) {
    i32 fd = -1;
    assert(sys::OpenStatus::Ok == sys::open(nullptr,
                                            (u8 const *)"/proc/self/exe",
                                            sys::O_RDONLY, 0, &fd));
    assert(fd != -1);

    sys::StatX statx;
    __builtin_memset(&statx, 0, sizeof(statx));
    assert(sys::StatXStatus::Ok == sys::statx(nullptr, fd, (u8 const *)"",
                                              sys::AT_EMPTY_PATH,
                                              sys::STATX_SIZE, &statx));
    assert((statx.stx_mask & sys::STATX_SIZE) != 0);

    u64 const file_size = statx.stx_size;

    SliceMut<u8> buf;
    // Need the explicit alignment since we cast pointers into this buffer into
    // struct pointers while parsing the ELF format.
    assert(AllocStatus::Ok ==
           alloc->aligned_alloc<u8>(nullptr, 8, file_size, &buf));

    u64 n_read = 0;
    assert(sys::ReadStatus::Ok ==
           sys::read(nullptr, fd, buf.ptr(), buf.len(), &n_read));
    assert(n_read == file_size);

    assert(sys::CloseStatus::Ok == sys::close(nullptr, fd));

    return buf.as_const();
}

static void print_panic_(char const *message) {
    assert(message != nullptr);

    u64 const len = __builtin_strlen(message);
    u64 n_written = 0;
    assert(sys::WriteStatus::Ok == sys::write(nullptr, sys::STDERR,
                                              (u8 const *)message, len,
                                              &n_written));
    assert(n_written == len);
}

static void signal_handler(i32_c signum, sys::SigInfo *info, void *ucontext) {
    (void)info;

    if (signum == (i32)sys::SIG::ILL) {
        print_panic_("Printing stack trace for SIGILL(4) in signal handler.\n");
    } else if (signum == (i32)sys::SIG::SEGV) {
        print_panic_(
            "Printing stack trace for SIGSEGV(11) in signal handler.\n");
    } else {
        print_panic_("Printing stack trace for signal with unexpected signum. "
                     "THIS SHOULD NEVER HAPPEN, check how the signal handler "
                     "is setup in start.cc\n");
    }

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
        .return_address = add_unsafe_u64(uctx->uc_mcontext.rip, 1),
    };

    Backtrace bt;
    elf::EhFrame::BacktraceStatus const status =
        ZX_ELF_EH_FRAME_.backtrace_panic(&cpu_ctx, &bt);
    switch (status) {
    case elf::EhFrame::BacktraceStatus::Ok: {
        break;
    }
    case elf::EhFrame::BacktraceStatus::OutOfCapacity: {
        // we don't care if it truncated.
        break;
    }
    default: {
        __builtin_memset(&bt, 0, sizeof(Backtrace));
        break;
    }
    }

    elf::print_stack_trace_panic(&bt);

    // control will fall back to the instruction that raised the signal,
    // it will raise the same signal again and the default handler for that
    // signal will run, which should cause the kernel to end the process with
    // the correspoding error code.
}

extern "C" void zx_setup_() {
    run_init_array_();

    constexpr u64 MEM_SIZE = u64{1} << 25;
    u8 *mem = nullptr;
    assert(page_alloc::AllocStatus::Ok ==
           page_alloc::alloc_4kb(nullptr, MEM_SIZE, &mem));
    assert(mem != nullptr);

    constexpr u64 ALLOC_SIZE = MEM_SIZE / 2;
    constexpr u64 TMP_ALLOC_SIZE = MEM_SIZE / 2;

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, ALLOC_SIZE));
    Alloc tmp_alloc = Alloc::init(
        SliceMut<u8>::init_unsafe(&mem[ALLOC_SIZE], TMP_ALLOC_SIZE));

    Slice<u8> buf = read_self_exe_panic_(&alloc);
    elf::File const file = elf::File::init_panic(&alloc, buf);

    elf::Section const eh_frame_section =
        file.get_section_panic(CString::init_unsafe(".eh_frame").as_slice());
    elf::Section const debug_line_section =
        file.get_section_panic(CString::init_unsafe(".debug_line").as_slice());
    elf::Section const debug_line_str_section = file.get_section_panic(
        CString::init_unsafe(".debug_line_str").as_slice());

    ZX_ELF_EH_FRAME_ = elf::EhFrame::init_panic(&alloc, &tmp_alloc,
                                                eh_frame_section.header->addr,
                                                eh_frame_section.body);
    ZX_ELF_DEBUG_LINE_ =
        elf::DebugLine::init_panic(&alloc, &tmp_alloc, debug_line_section.body,
                                   debug_line_str_section.body);

    tmp_alloc.reset();

    SliceMut<u8> signal_stack_mem;
    assert(AllocStatus::Ok == alloc.aligned_alloc<u8>(nullptr, 16, u64{1} << 20,
                                                      &signal_stack_mem));

    sys::Stack const signal_stack = sys::Stack{
        .ss_sp = signal_stack_mem.ptr(),
        .ss_flags = 0,
        .ss_size = signal_stack_mem.len(),
    };
    assert(sys::SigAltStackStatus::Ok ==
           sys::sigaltstack(nullptr, &signal_stack, nullptr));

    sys::SigAction act = sys::SigAction{
        .sigaction = signal_handler,
        .flags = sys::SA_SIGINFO | sys::SA_RESETHAND | sys::SA_NODEFER |
                 sys::SA_ONSTACK,
        .mask = sys::SigSet::init(),
    };

    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(nullptr, sys::SIG::ILL, &act, nullptr));
    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(nullptr, sys::SIG::SEGV, &act, nullptr));
}

#else // ifdef ZX_DEBUG_INFO

extern "C" void zx_setup_() { run_init_array_(); }

#endif // ifdef ZX_DEBUG_INFO

__asm__(
    ".text\n"
    ".type _start, @function\n"
    ".global _start\n"
    "_start:\n\t"
    ".cfi_startproc\n\t"
    // stop eh_frame based unwinder from going past this function frame
    // since there is nothing above.
    ".cfi_undefined %rip\n\t"
    // mark the first frame by setting rbp to 0
    "xorq %rbp, %rbp\n\t"
    // argc
    "movq (%rsp), %r15\n\t"
    // argv
    "leaq 8(%rsp), %r14\n\t"
    // envp
    "leaq 16(%rsp, %r15, 8), %r13\n\t"
    // call debug info setup function
    "callq zx_setup_\n\t"
    // restore saved argc, argv and envp
    "movq %r15, %rdi\n\t"
    "movq %r14, %rsi\n\t"
    "movq %r13, %rdx\n\t"
    // call main function to actually execute the program
    "callq main\n\t"
    // mov return code of main to syscall argument
    "movl %eax, %edi\n\t"
    // syscall code for exit_group is 231
    "movq $231, %rax\n\t"
    // call exit_group with the single argument set to the return code of main
    "syscall\n\t"
    // should never be reached but ..
    "ud2\n\t"
    // mark function boundary for eh_frame debug info
    ".cfi_endproc\n\t"
    ".size _start, .-_start\n\t");
