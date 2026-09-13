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
    // open close read write statx pread pwrite
}

static void test_perf_event_open() {}

static void test_io_uring() {
    // setup enter register
}

int main() {
    test_mmap();
    test_file();
    test_perf_event_open();
    test_io_uring();
}
