import zx;

using namespace zx;

int main() {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    i32 fd = -1;
    testing::assert_with_trace(
        tmp_file::OpenStatus::Ok == tmp_file::open(&err_ctx, &fd), &err_ctx);
    assert(fd != -1);

    u64 data = 69;
    u64 n = 0;
    testing::assert_with_trace(
        sys::PWriteStatus::Ok ==
            sys::pwrite(&err_ctx, fd, (u8 const *)&data, sizeof(data), 0, &n),
        &err_ctx);
    assert(n == sizeof(data));

    data = 1131;

    testing::assert_with_trace(
        sys::PReadStatus::Ok ==
            sys::pread(&err_ctx, fd, (u8 *)&data, sizeof(data), 0, &n),
        &err_ctx);
    assert(n == sizeof(data));
    assert(data == 69);

    return 0;
}
