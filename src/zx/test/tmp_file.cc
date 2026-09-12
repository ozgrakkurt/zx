import zx;

using namespace zx;

int main() {
    i32 fd = -1;
    assert(tmp_file::OpenStatus::Ok == tmp_file::open(&fd));
    assert(fd != -1);

    u64 data = 69;
    u64 n = 0;
    assert(sys::PWriteStatus::Ok ==
           sys::pwrite(fd, (u8 const *)&data, sizeof(data), 0, &n));
    assert(n == sizeof(data));

    data = 1131;

    assert(sys::PReadStatus::Ok ==
           sys::pread(fd, (u8 *)&data, sizeof(data), 0, &n));
    assert(n == sizeof(data));
    assert(data == 69);

    return 0;
}
