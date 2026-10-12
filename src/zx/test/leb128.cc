import zx;

using namespace zx;

static void testcase_unsigned(Slice<u8> buf, u64 num) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    u64 out_num = U64_MAX;
    u64 offset = 0;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok ==
            leb128::decode_u64(&err_ctx, buf, &offset, &out_num),
        &err_ctx);
    assert(offset == buf.len());
    assert(out_num == num);

    u8 out_buf[16];
    __builtin_memset(out_buf, 0xaa, sizeof(out_buf));

    SliceMut<u8> out = SliceMut<u8>::init_unsafe(out_buf, sizeof(out_buf));

    offset = 0;
    testing::assert_with_trace(
        leb128::EncodeStatus::Ok ==
            leb128::encode_u64(&err_ctx, num, out, &offset),
        &err_ctx);
    assert(offset == buf.len());
    assert(out.as_const().slice_unsafe(0, buf.len()).equals(buf));
}

static void testcase_signed(Slice<u8> buf, i64 num) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    i64 out_num = I64_MAX;
    u64 offset = 0;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok ==
            leb128::decode_i64(&err_ctx, buf, &offset, &out_num),
        &err_ctx);
    assert(offset == buf.len());
    assert(out_num == num);

    u8 out_buf[16];
    __builtin_memset(out_buf, 0xaa, sizeof(out_buf));

    SliceMut<u8> out = SliceMut<u8>::init_unsafe(out_buf, sizeof(out_buf));

    offset = 0;
    testing::assert_with_trace(
        leb128::EncodeStatus::Ok ==
            leb128::encode_i64(&err_ctx, num, out, &offset),
        &err_ctx);
    assert(offset == buf.len());
    assert(out.as_const().slice_unsafe(0, buf.len()).equals(buf));
}

static void test_unsigned() {
    testcase_unsigned(Slice<u8>::init_unsafe((u8[]){2__wbu}, 1), 2);
    testcase_unsigned(Slice<u8>::init_unsafe((u8[]){127__wbu}, 1), 127);
    testcase_unsigned(Slice<u8>::init_unsafe((u8[]){0x80__wbu, 1__wbu}, 2),
                      128);
    testcase_unsigned(
        Slice<u8>::init_unsafe((u8[]){1__wbu | 0x80__wbu, 1__wbu}, 2), 129);
    testcase_unsigned(
        Slice<u8>::init_unsafe((u8[]){57__wbu | 0x80__wbu, 100__wbu}, 2),
        12857__wbu);
}

static void test_signed() {
    testcase_signed(Slice<u8>::init_unsafe((u8[]){2__wbu}, 1), 2);
    testcase_signed(Slice<u8>::init_unsafe((u8[]){0x7e__wbu}, 1), -2);
    testcase_signed(Slice<u8>::init_unsafe((u8[]){127__wbu | 0x80__wbu, 0}, 2),
                    127);
    testcase_signed(
        Slice<u8>::init_unsafe((u8[]){1__wbu | 0x80__wbu, 0x7f__wbu}, 2), -127);
    testcase_signed(Slice<u8>::init_unsafe((u8[]){0x80__wbu, 1__wbu}, 2), 128);
    testcase_signed(Slice<u8>::init_unsafe((u8[]){0x80__wbu, 0x7f__wbu}, 2),
                    -128);
    testcase_signed(
        Slice<u8>::init_unsafe((u8[]){1__wbu | 0x80__wbu, 1__wbu}, 2), 129);
    testcase_signed(
        Slice<u8>::init_unsafe((u8[]){0x7f__wbu | 0x80__wbu, 0x7e__wbu}, 2),
        -129);
}

static void signed_roundtrip(SliceMut<u8> slice, i64 i) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    u64 write_offset = 0;
    testing::assert_with_trace(
        leb128::EncodeStatus::Ok ==
            leb128::encode_i64(&err_ctx, i, slice, &write_offset),
        &err_ctx);

    u64 read_offset = 0;
    i64 num = I64_MAX;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok ==
            leb128::decode_i64(&err_ctx, slice.as_const(), &read_offset, &num),
        &err_ctx);

    assert(num == i);
    assert(read_offset == write_offset);
}

static void unsigned_roundtrip(SliceMut<u8> slice, u64 i) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    u64 write_offset = 0;
    testing::assert_with_trace(
        leb128::EncodeStatus::Ok ==
            leb128::encode_u64(&err_ctx, i, slice, &write_offset),
        &err_ctx);

    u64 read_offset = 0;
    u64 num = U64_MAX;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok ==
            leb128::decode_u64(&err_ctx, slice.as_const(), &read_offset, &num),
        &err_ctx);

    assert(num == i);
    assert(read_offset == write_offset);
}

static void test_limits() {
    u8 buf_buf[15];
    SliceMut<u8> buf = SliceMut<u8>::init_unsafe(buf_buf, sizeof(buf_buf));
    buf.memset(0xaa);

    unsigned_roundtrip(buf, U64_MAX);
    unsigned_roundtrip(buf, 0);
    unsigned_roundtrip(buf, 1);

    signed_roundtrip(buf, I64_MAX);
    signed_roundtrip(buf, I64_MIN);
    signed_roundtrip(buf, -1);
    signed_roundtrip(buf, 1);
}

static void test_roundtrip() {
    u8 buf_buf[15];
    SliceMut<u8> buf = SliceMut<u8>::init_unsafe(buf_buf, sizeof(buf_buf));
    buf.memset(0xaa);

    for (u64 i = 0; i < 1025; i++) {
        unsigned_roundtrip(buf, i);
    }

    for (i64 i = -1025; i < 1024; i++) {
        signed_roundtrip(buf, i);
    }
}

static void test_read_with_overflow() {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    u8 buf_buf[] = {
        0xff__wbu, 0xff__wbu, 0xff__wbu,       0xff__wbu,
        0xff__wbu, 0xff__wbu, 0xff__wbu,       0xff__wbu,
        0xff__wbu, 0xff__wbu, 0b01111111__wbu, 0b11100100__wbu,
    };

    SliceMut<u8> buf = SliceMut<u8>::init_unsafe(buf_buf, sizeof(buf_buf));

    u64 offset = 0;
    u64 num = 0;
    assert(leb128::DecodeStatus::InvalidInput ==
           leb128::decode_u64(&err_ctx, buf.as_const(), &offset, &num));

    offset = 0;
    i64 inum = 0;
    assert(leb128::DecodeStatus::InvalidInput ==
           leb128::decode_i64(&err_ctx, buf.as_const(), &offset, &inum));
}

int main(void) {
    test_unsigned();
    test_signed();
    test_limits();
    test_read_with_overflow();
    test_roundtrip();

    return 0;
}
