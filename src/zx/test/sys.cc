import zx;

using namespace zx;

static i32 open_temporary_file(Backtrace *err_bt) {
    u8 fmt_buf[64];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(fmt_buf, sizeof(fmt_buf)));

    bool fmt_overflow = false;

    fmt.push_str(CString::init_unsafe("/tmp/").as_slice(), &fmt_overflow);

    u64 seed = 0;
    u64 n_read = 0;
    testing::assert_with_trace(
        sys::GetRandomStatus::Ok ==
            sys::getrandom(err_bt, (u8 *)&seed, sizeof(seed), 0, &n_read),
        err_bt);
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
    testing::assert_with_trace(sys::OpenStatus::Ok ==
                                   sys::open(err_bt, path,
                                             sys::O_RDWR | sys::O_CREAT,
                                             sys::S_IWUSR | sys::S_IRUSR, &fd),
                               err_bt);
    assert(fd != -1);

    testing::assert_with_trace(
        sys::UnlinkAtStatus::Ok ==
            sys::unlink_at(err_bt, sys::AT_FDCWD, path, 0),
        err_bt);

    return fd;
}

static void test_mmap(Backtrace *err_bt) {
    i32 const fd = open_temporary_file(err_bt);

    u64 const len = 4096;

    testing::assert_with_trace(
        sys::FTruncateStatus::Ok == sys::ftruncate(err_bt, fd, len), err_bt);

    u8 *addr = nullptr;
    testing::assert_with_trace(
        sys::MMapStatus::Ok == sys::mmap(err_bt, nullptr, len, sys::PROT_NONE,
                                         sys::MAP_SHARED, fd, 0, &addr),
        err_bt);
    assert(addr != nullptr);

    testing::assert_with_trace(
        sys::MProtectStatus::Ok ==
            sys::mprotect(err_bt, addr, len, sys::PROT_READ | sys::PROT_WRITE),
        err_bt);

    SliceMut<u8> slice = SliceMut<u8>::init_unsafe(addr, len);
    slice.write_unsafe(3069, 69);

    assert(slice.read_unsafe(3069) == 69);

    testing::assert_with_trace(
        sys::MMapStatus::Ok == sys::munmap(err_bt, addr, len), err_bt);
}

static void test_file(Backtrace *err_bt) {
    i32 const fd = open_temporary_file(err_bt);

    u64 const file_size = 5000;

    testing::assert_with_trace(sys::FTruncateStatus::Ok ==
                                   sys::ftruncate(err_bt, fd, file_size),
                               err_bt);

    constexpr u64 data_len = 1020;

    u8 data_buf[data_len];
    SliceMut<u8> data = SliceMut<u8>::init_unsafe(data_buf, sizeof(data_buf));

    Xoshiro256 rand = Xoshiro256::init(1131);
    rand.fill(data);

    i64 offset = 0;
    testing::assert_with_trace(
        sys::LSeekStatus::Ok ==
            sys::lseek(err_bt, fd, 2000, sys::SEEK_SET, &offset),
        err_bt);
    assert(offset == 2000);

    u64 n = 0;
    testing::assert_with_trace(
        sys::WriteStatus::Ok ==
            sys::write(err_bt, fd, data.ptr(), data.len(), &n),
        err_bt);
    assert(n == data.len());

    u8 out_buf[data_len];
    SliceMut<u8> out = SliceMut<u8>::init_unsafe(out_buf, sizeof(out_buf));

    // read from improper position, the cursor on the file is pointing to 2000 +
    // out.len() since we just did a write that advanced the cursor.
    n = 0;
    testing::assert_with_trace(
        sys::ReadStatus::Ok == sys::read(err_bt, fd, out.ptr(), out.len(), &n),
        err_bt);
    assert(n == out.len());

    assert(!out.equals(data));

    // seek to proper position
    testing::assert_with_trace(
        sys::LSeekStatus::Ok ==
            sys::lseek(err_bt, fd, 2000, sys::SEEK_SET, &offset),
        err_bt);
    assert(offset == 2000);

    n = 0;
    testing::assert_with_trace(
        sys::ReadStatus::Ok == sys::read(err_bt, fd, out.ptr(), out.len(), &n),
        err_bt);
    assert(n == out.len());

    assert(out.equals(data));

    sys::StatX statx;
    __builtin_memset(&statx, 0, sizeof(statx));
    testing::assert_with_trace(sys::StatXStatus::Ok ==
                                   sys::statx(err_bt, fd, (u8 const *)"",
                                              sys::AT_EMPTY_PATH,
                                              sys::STATX_MODE, &statx),
                               err_bt);
    assert(statx.stx_size == file_size);
    assert((statx.stx_mask & sys::STATX_MODE) != 0);

    rand.fill(data);
    rand.fill(out);

    u64 const p_offset = 1269;

    n = 0;
    testing::assert_with_trace(
        sys::PWriteStatus::Ok ==
            sys::pwrite(err_bt, fd, data.ptr(), data.len(), p_offset, &n),
        err_bt);
    assert(n == data.len());

    // read from wrong offset
    n = 0;
    testing::assert_with_trace(
        sys::PReadStatus::Ok ==
            sys::pread(err_bt, fd, out.ptr(), out.len(), p_offset + 3, &n),
        err_bt);
    assert(n == out.len());

    assert(!out.equals(data));

    // read from right offset
    n = 0;
    testing::assert_with_trace(
        sys::PReadStatus::Ok ==
            sys::pread(err_bt, fd, out.ptr(), out.len(), p_offset, &n),
        err_bt);
    assert(n == out.len());

    assert(out.equals(data));
}

int main() {
    Backtrace err_bt = Backtrace::init();

    test_mmap(&err_bt);
    test_file(&err_bt);
}
