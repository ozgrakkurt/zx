import zx;

using namespace zx;

static i32 open_temporary_file() {
    u8 fmt_buf[64];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(fmt_buf, sizeof(fmt_buf)));

    bool fmt_overflow = false;

    fmt.push_str(CString::init_unsafe("/tmp/").as_slice(), &fmt_overflow);

    u64 seed = 0;
    u64 n_read = 0;
    assert(sys::GetRandomStatus::Ok ==
           sys::getrandom((u8 *)&seed, sizeof(seed), 0, &n_read));
    assert(n_read == sizeof(seed));
    Xoshiro256 rand = Xoshiro256::init(seed);
    u128 uuid = uuid_v4::generate(&rand);

    uuid_v4::print(uuid, false, &fmt, &fmt_overflow);
    fmt.push_char(0, &fmt_overflow);

    assert(!fmt_overflow);

    u8 const *path = fmt.written().ptr();
    u64 const path_len = __builtin_strlen((char const *)path);
    assert(sub_unsafe_u64(fmt.written().len(), 1) == path_len);

    i32 fd = -1;
    assert(sys::OpenStatus::Ok == sys::open(path, sys::O_RDWR | sys::O_CREAT,
                                            sys::S_IWUSR | sys::S_IRUSR, &fd));
    assert(fd != -1);

    assert(sys::UnlinkAtStatus::Ok == sys::unlink_at(0, path, 0));

    return fd;
}

static void test_mmap() {
    i32 const fd = open_temporary_file();

    u64 const len = 4096;

    assert(sys::FTruncateStatus::Ok == sys::ftruncate(fd, len));

    u8 *addr = nullptr;
    assert(sys::MMapStatus::Ok == sys::mmap(nullptr, len, sys::PROT_NONE,
                                            sys::MAP_SHARED, fd, 0, &addr));
    assert(addr != nullptr);

    assert(sys::MProtectStatus::Ok ==
           sys::mprotect(addr, len, sys::PROT_READ | sys::PROT_WRITE));

    SliceMut<u8> slice = SliceMut<u8>::init_unsafe(addr, len);
    slice.write_unsafe(3069, 69);

    assert(slice.read_unsafe(3069) == 69);

    assert(sys::MMapStatus::Ok == sys::munmap(addr, len));
}

static void test_file() {
    // open close read write lseek statx pread pwrite
    i32 const fd = open_temporary_file();

    u64 const file_size = 5000;

    assert(sys::FTruncateStatus::Ok == sys::ftruncate(fd, file_size));

    constexpr u64 data_len = 1020;

    u8 data_buf[data_len];
    SliceMut<u8> data = SliceMut<u8>::init_unsafe(data_buf, sizeof(data_buf));

    Xoshiro256 rand = Xoshiro256::init(1131);
    rand.fill(data);

    u64 offset = 0;
    assert(sys::LSeekStatus::Ok ==
           sys::lseek(fd, 2000, sys::SEEK_SET, &offset));
    assert(offset == 2000);

    u64 n = 0;
    assert(sys::WriteStatus::Ok == sys::write(fd, data.ptr(), data.len(), &n));
    assert(n == data.len());

    u8 out_buf[data_len];
    SliceMut<u8> out = SliceMut<u8>::init_unsafe(out_buf, sizeof(out_buf));

    // read from improper position, the cursor on the file is pointing to 2000 +
    // out.len() since we just did a write that advanced the cursor.
    n = 0;
    assert(sys::ReadStatus::Ok == sys::read(fd, out.ptr(), out.len(), &n));
    assert(n == out.len());

    assert(!out.equals(data));

    // seek to proper position
    assert(sys::LSeekStatus::Ok ==
           sys::lseek(fd, 2000, sys::SEEK_SET, &offset));
    assert(offset == 2000);

    n = 0;
    assert(sys::ReadStatus::Ok == sys::read(fd, out.ptr(), out.len(), &n));
    assert(n == out.len());

    assert(out.equals(data));

    sys::StatX statx;
    __builtin_memset(&statx, 0, sizeof(statx));
    assert(sys::StatXStatus::Ok == sys::statx(fd, (u8 const *)"",
                                              sys::AT_EMPTY_PATH,
                                              sys::STATX_MODE, &statx));
    assert(statx.stx_size == file_size);
    assert((statx.stx_mask & sys::STATX_MODE) != 0);

    rand.fill(data);
    rand.fill(out);

    u64 const p_offset = 1269;

    n = 0;
    assert(sys::PWriteStatus::Ok ==
           sys::pwrite(fd, data.ptr(), data.len(), p_offset, &n));
    assert(n == data.len());

    // read from wrong offset
    n = 0;
    assert(sys::PReadStatus::Ok ==
           sys::pread(fd, out.ptr(), out.len(), p_offset + 3, &n));
    assert(n == out.len());

    assert(!out.equals(data));

    // read from right offset
    n = 0;
    assert(sys::PReadStatus::Ok ==
           sys::pread(fd, out.ptr(), out.len(), p_offset, &n));
    assert(n == out.len());

    assert(out.equals(data));
}

int main() {
    test_mmap();
    test_file();
}
