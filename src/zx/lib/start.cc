import zx;

using namespace zx;

#ifdef ZX_DEBUG_INFO

elf::EhFrame ZX_ELF_EH_FRAME_;
elf::DebugLine ZX_ELF_DEBUG_LINE_;

static Slice<u8> read_self_exe_panic_(Alloc *alloc) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    i32 fd = -1;
    assert(sys::OpenStatus::Ok == sys::open(&err_ctx,
                                            (u8 const *)"/proc/self/exe",
                                            sys::O_RDONLY, 0, &fd));
    assert(fd != -1);

    sys::StatX statx;
    __builtin_memset(&statx, 0, sizeof(statx));
    assert(sys::StatXStatus::Ok == sys::statx(&err_ctx, fd, (u8 const *)"",
                                              sys::AT_EMPTY_PATH,
                                              sys::STATX_SIZE, &statx));
    assert((statx.stx_mask & sys::STATX_SIZE) != 0);

    u64 const file_size = statx.stx_size;

    SliceMut<u8> buf;
    // Need the explicit alignment since we cast pointers into this buffer into
    // struct pointers while parsing the ELF format.
    assert(AllocStatus::Ok ==
           alloc->aligned_alloc<8, u8>(&err_ctx, file_size, &buf));

    u64 n_read = 0;
    assert(sys::ReadStatus::Ok ==
           sys::read(&err_ctx, fd, buf.ptr(), buf.len(), &n_read));
    assert(n_read == file_size);

    return buf.as_const();
}

static void print_stack_trace_panic_(Backtrace const *bt) {
    assert(bt != nullptr);

    constexpr u64 FMT_SIZE = u64{1} << 14;
    u8 fmt_mem[FMT_SIZE];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(fmt_mem, FMT_SIZE));

    bool fmt_overflow = false;

    fmt.push_str(CString::init_unsafe("STACK TRACE:\n").as_slice(),
                 &fmt_overflow);

    constexpr u64 N_LINE_INFO = 8;
    elf::DebugLine::LineInfo line_info_mem[N_LINE_INFO];
    List<elf::DebugLine::LineInfo> line_infos =
        List<elf::DebugLine::LineInfo>::init(
            SliceMut<elf::DebugLine::LineInfo>::init_unsafe(line_info_mem,
                                                            N_LINE_INFO));

    for (u64 i = 0; i < Backtrace::LEN; i++) {
        if (bt->m_data[i] == 0) {
            break;
        }
        line_infos.reset();
        elf::DebugLine::GetLineInfosStatus const gl_status =
            ZX_ELF_DEBUG_LINE_.get_line_infos(bt->m_data[i] - 1, &line_infos);

        if (gl_status == elf::DebugLine::GetLineInfosStatus::NotFound) {
            assert(line_infos.n_written() == 0);
            fmt.push_str(CString::init_unsafe("-??\n").as_slice(),
                         &fmt_overflow);
        } else if (gl_status ==
                   elf::DebugLine::GetLineInfosStatus::OutOfCapacity) {
            fmt.push_str(CString::init_unsafe("-??OUT_OF_CAP\n").as_slice(),
                         &fmt_overflow);
        }

        for (u64 j = 0; j < line_infos.n_written(); j++) {
            elf::DebugLine::LineInfo const line_info =
                line_infos.written().read_unsafe(j);
            fmt.push_char('-', &fmt_overflow);
            fmt.push_str(line_info.file_path.as_slice(), &fmt_overflow);
            fmt.push_char(':', &fmt_overflow);
            fmt.push_u64(line_info.line_no, &fmt_overflow);
            fmt.push_char(':', &fmt_overflow);
            fmt.push_u64(line_info.column_no, &fmt_overflow);
            fmt.push_char('\n', &fmt_overflow);
        }
    }

    assert(!fmt_overflow);

    u64 n_written = 0;
    assert(sys::WriteStatus::Ok == sys::write(nullptr, sys::STDERR,
                                              fmt.written().ptr(),
                                              fmt.n_written(), &n_written));
    assert(n_written == fmt.n_written());
}

static void signal_handler(i32_c signum, sys::SigInfo *info, void *ucontext) {
    (void)info;
    (void)signum;

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

    Backtrace bt;
    // ignore truncation error
    (void)ZX_ELF_EH_FRAME_.backtrace_panic(&cpu_ctx, &bt);

    print_stack_trace_panic_(&bt);

    // control will fall back to the instruction that raised the signal,
    // it will raise the same signal again and the default handler for that
    // signal will run, which should cause the kernel to end the process with
    // the correspoding error code.
}

extern "C" void zx_setup_() {
    constexpr u64 MEM_SIZE = u64{1} << 25;
    u8 *mem = nullptr;
    assert(page_alloc::AllocStatus::Ok ==
           page_alloc::alloc_4kb(nullptr, MEM_SIZE, &mem));
    assert(mem != nullptr);

    constexpr u64 ALLOC_SIZE = u64{1} << 22;
    static_assert(MEM_SIZE > ALLOC_SIZE);
    constexpr u64 TMP_ALLOC_SIZE = MEM_SIZE - ALLOC_SIZE;

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

    sys::SigAction act = sys::SigAction{
        .sigaction = signal_handler,
        .flags = sys::SA_SIGINFO | sys::SA_RESETHAND | sys::SA_NODEFER,
        .mask = sys::SigSet::init(),
    };

    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(nullptr, sys::SIG::ILL, &act, nullptr));
    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(nullptr, sys::SIG::SEGV, &act, nullptr));
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
