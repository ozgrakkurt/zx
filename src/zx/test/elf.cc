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
    (void)eh_frame;

    assert(page_alloc::FreeStatus::Ok == page_alloc::free(mem, mem_size));

    return 0;
}

#else // ifdef ZX_DEBUG_INFO

int main() { return 0; }

#endif // ifdef ZX_DEBUG_INFO
