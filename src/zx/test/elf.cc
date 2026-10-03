import zx;

using namespace zx;

#ifdef ZX_DEBUG_INFO

Slice<u8> read_self_bin(Alloc *alloc) {
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

void cmmb(Alloc *tmp_alloc, elf::EhFrame const *dbg_info) {
    elf::UnwindContext ctx = elf::UnwindContext::init();

    u8 fmt_buf[1024];
    SliceMut<u8> fmt_slice =
        SliceMut<u8>::init_unsafe(fmt_buf, sizeof(fmt_buf));
    Fmt fmt = Fmt::init(fmt_slice);

    bool fmt_overflow = false;

    fmt.push_str(CString::init_unsafe("cmmb:\n").as_slice(), &fmt_overflow);

    do {
        if (ctx.rip() != 0) {
            fmt.push_u64_hex(ctx.rip() - 1, &fmt_overflow);
            fmt.push_char('\n', &fmt_overflow);
        }
    } while (dbg_info->step_panic(tmp_alloc, &ctx));

    assert(!fmt_overflow);

    u64 n_written = 0;
    assert(sys::WriteStatus::Ok == sys::write(sys::STDERR, fmt.written().ptr(),
                                              fmt.n_written(), &n_written));
    assert(n_written == fmt.n_written());
    panic();
}

void callme_maybe(Alloc *tmp_alloc, elf::EhFrame const *dbg_info, u64 iter) {
    if (iter > 5) {
        u8 fmt_buf[1024];
        SliceMut<u8> fmt_slice =
            SliceMut<u8>::init_unsafe(fmt_buf, sizeof(fmt_buf));
        Fmt fmt = Fmt::init(fmt_slice);

        bool fmt_overflow = false;

        elf::UnwindContext ctx = elf::UnwindContext::init();

        fmt.push_str(CString::init_unsafe("callme_maybe:\n").as_slice(),
                     &fmt_overflow);

        do {
            if (ctx.rip() != 0) {
                fmt.push_u64_hex(ctx.rip() - 1, &fmt_overflow);
                fmt.push_char('\n', &fmt_overflow);
            }
        } while (dbg_info->step_panic(tmp_alloc, &ctx));

        assert(!fmt_overflow);

        // u64 n_written = 0;
        // assert(sys::WriteStatus::Ok == sys::write(sys::STDERR,
        //                                           fmt.written().ptr(),
        //                                           fmt.n_written(),
        //                                           &n_written));
        // assert(n_written == fmt.n_written());
        // panic();

        return;
    } else {
        callme_maybe(tmp_alloc, dbg_info, iter + 1);
        return;
    }
}

int main() {
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

    elf::EhFrame eh_frame = elf::EhFrame::init_panic(
        &alloc, &tmp_alloc, eh_frame_header, eh_frame_raw);

    callme_maybe(&tmp_alloc, &eh_frame, 0);
    cmmb(&tmp_alloc, &eh_frame);

    assert(page_alloc::FreeStatus::Ok == page_alloc::free(mem, mem_size));

    return 0;
}

#else // ifdef ZX_DEBUG_INFO

int main() { return 0; }

#endif // ifdef ZX_DEBUG_INFO
