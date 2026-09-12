import zx;

using namespace zx;

static i32 open_temporary_file() {
    u8 fmt_buf[64];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(fmt_buf, sizeof(fmt_buf)));

    bool fmt_overflow = false;

    fmt.push_str(CString::init_unsafe("/tmp/").as_slice(), &fmt_overflow);

    // TODO: should be system random
    Xoshiro256 rand = Xoshiro256::init(69);
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
    ;
}

static void test_mmap() {
    // mmap munmap mprotect
    i32 const fd = open_temporary_file();
    (void)fd;
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
