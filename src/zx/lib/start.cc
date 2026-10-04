import zx;

using namespace zx;

#ifdef ZX_DEBUG_INFO

static elf::EhFrame ZX_EH_FRAME_;
static Alloc ZX_TMP_ALLOC_;

static void signal_handler(i32_c signum, sys::SigInfo *info, void *ucontext) {
    (void)info;
    (void)signum;
    sys::UContext const *uctx = (sys::UContext const *)ucontext;
    elf::UnwindContext ctx = elf::UnwindContext::init_from_ucontext(uctx);

    u8 fmt_buf[1024];
    SliceMut<u8> fmt_slice =
        SliceMut<u8>::init_unsafe(fmt_buf, sizeof(fmt_buf));
    Fmt fmt = Fmt::init(fmt_slice);

    bool fmt_overflow = false;

    fmt.push_str(CString::init_unsafe("Crashed with signal:\n").as_slice(),
                 &fmt_overflow);

    while (ZX_EH_FRAME_.step_panic(&ZX_TMP_ALLOC_, &ctx)) {
        if (ctx.rip() != 0) {
            fmt.push_u64_hex(ctx.rip() - 1, &fmt_overflow);
            fmt.push_char('\n', &fmt_overflow);
        }
    }

    assert(!fmt_overflow);

    u64 n_written = 0;
    assert(sys::WriteStatus::Ok == sys::write(sys::STDERR, fmt.written().ptr(),
                                              fmt.n_written(), &n_written));
    assert(n_written == fmt.n_written());
}

static Slice<u8> read_self_bin(Alloc *alloc) {
    i32 fd = -1;
    assert(sys::OpenStatus::Ok ==
           sys::open((u8 const *)"/proc/self/exe", sys::O_RDONLY, 0, &fd));
    assert(fd != -1);

    sys::StatX statx;
    __builtin_memset(&statx, 0, sizeof(statx));
    assert(sys::StatXStatus::Ok == sys::statx(fd, (u8 const *)"",
                                              sys::AT_EMPTY_PATH,
                                              sys::STATX_SIZE, &statx));
    assert((statx.stx_mask & sys::STATX_SIZE) != 0);

    u64 const file_size = statx.stx_size;

    SliceMut<u8> buf;
    assert(AllocStatus::Ok == alloc->aligned_alloc<64, u8>(file_size, &buf));

    u64 n_read = 0;
    assert(sys::ReadStatus::Ok == sys::read(fd, buf.ptr(), buf.len(), &n_read));
    assert(n_read == file_size);

    return buf.as_const();
}

extern "C" void zx_setup_() {
    constexpr u64 mem_size = 1 << 21;
    u8 *mem = nullptr;
    assert(page_alloc::AllocStatus::Ok ==
           page_alloc::alloc_4kb(mem_size, &mem));
    assert(mem != nullptr);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, mem_size / 2));
    Alloc tmp_alloc = Alloc::init(
        SliceMut<u8>::init_unsafe(&mem[mem_size / 2], mem_size / 2));

    Slice<u8> buf = read_self_bin(&alloc);
    elf::File const file = elf::File::init_panic(&alloc, buf);

    u64 const section_idx = file.get_section_index_panic(
        CString::init_unsafe(".eh_frame").as_slice());

    Slice<u8> eh_frame_raw = file.get_section_data_panic(section_idx);
    elf::SectionHeader const *eh_frame_header =
        file.get_section_header_panic(section_idx);

    ZX_EH_FRAME_ = elf::EhFrame::init_panic(&alloc, &tmp_alloc, eh_frame_header,
                                            eh_frame_raw);
    ZX_TMP_ALLOC_ = tmp_alloc;

    sys::SigAction act = sys::SigAction{
        .sigaction = signal_handler,
        .flags = 0,
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
        ".cfi_undefined rip\n\t"
        "xorq %rbp, %rbp\n\t"
        "popq %rdi\n\t"
        "movq %rsp, %rsi\n\t"
        "andq $-16, %rsp\n\t"
        "callq zx_setup_\n\t"
        "callq main\n\t"
        "movq %rax, %rdi\n\t"
        "movq $60, %rax\n\t"
        "syscall\n\t"
        ".cfi_endproc\n\t");
