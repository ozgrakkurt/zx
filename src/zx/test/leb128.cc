import zx;

using namespace zx;

static void testcase_unsigned(Backtrace *err_bt, Slice<u8> buf, u64 num) {
    u64 out_num = U64_MAX;
    u64 offset = 0;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok ==
            leb128::decode_u64(err_bt, buf, &offset, &out_num),
        err_bt);
    assert(offset == buf.len());
    assert(out_num == num);

    u8 out_buf[16];
    __builtin_memset(out_buf, 0xaa, sizeof(out_buf));

    SliceMut<u8> out = SliceMut<u8>::init_unsafe(out_buf, sizeof(out_buf));

    offset = 0;
    testing::assert_with_trace(
        leb128::EncodeStatus::Ok ==
            leb128::encode_u64(err_bt, num, out, &offset),
        err_bt);
    assert(offset == buf.len());
    assert(out.as_const().slice_unsafe(0, buf.len()).equals(buf));
}

static void testcase_signed(Backtrace *err_bt, Slice<u8> buf, i64 num) {
    i64 out_num = I64_MAX;
    u64 offset = 0;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok ==
            leb128::decode_i64(err_bt, buf, &offset, &out_num),
        err_bt);
    assert(offset == buf.len());
    assert(out_num == num);

    u8 out_buf[16];
    __builtin_memset(out_buf, 0xaa, sizeof(out_buf));

    SliceMut<u8> out = SliceMut<u8>::init_unsafe(out_buf, sizeof(out_buf));

    offset = 0;
    testing::assert_with_trace(
        leb128::EncodeStatus::Ok ==
            leb128::encode_i64(err_bt, num, out, &offset),
        err_bt);
    assert(offset == buf.len());
    assert(out.as_const().slice_unsafe(0, buf.len()).equals(buf));
}

static void test_unsigned(Backtrace *err_bt) {
    testcase_unsigned(err_bt, Slice<u8>::init_unsafe((u8[]){2__wbu}, 1), 2);
    testcase_unsigned(err_bt, Slice<u8>::init_unsafe((u8[]){127__wbu}, 1), 127);
    testcase_unsigned(
        err_bt, Slice<u8>::init_unsafe((u8[]){0x80__wbu, 1__wbu}, 2), 128);
    testcase_unsigned(
        err_bt, Slice<u8>::init_unsafe((u8[]){1__wbu | 0x80__wbu, 1__wbu}, 2),
        129);
    testcase_unsigned(
        err_bt,
        Slice<u8>::init_unsafe((u8[]){57__wbu | 0x80__wbu, 100__wbu}, 2),
        12857__wbu);
}

static void test_signed(Backtrace *err_bt) {
    testcase_signed(err_bt, Slice<u8>::init_unsafe((u8[]){2__wbu}, 1), 2);
    testcase_signed(err_bt, Slice<u8>::init_unsafe((u8[]){0x7e__wbu}, 1), -2);
    testcase_signed(err_bt,
                    Slice<u8>::init_unsafe((u8[]){127__wbu | 0x80__wbu, 0}, 2),
                    127);
    testcase_signed(
        err_bt,
        Slice<u8>::init_unsafe((u8[]){1__wbu | 0x80__wbu, 0x7f__wbu}, 2), -127);
    testcase_signed(err_bt,
                    Slice<u8>::init_unsafe((u8[]){0x80__wbu, 1__wbu}, 2), 128);
    testcase_signed(
        err_bt, Slice<u8>::init_unsafe((u8[]){0x80__wbu, 0x7f__wbu}, 2), -128);
    testcase_signed(
        err_bt, Slice<u8>::init_unsafe((u8[]){1__wbu | 0x80__wbu, 1__wbu}, 2),
        129);
    testcase_signed(
        err_bt,
        Slice<u8>::init_unsafe((u8[]){0x7f__wbu | 0x80__wbu, 0x7e__wbu}, 2),
        -129);
}

static void signed_roundtrip(Backtrace *err_bt, SliceMut<u8> slice, i64 i) {
    u64 write_offset = 0;
    testing::assert_with_trace(
        leb128::EncodeStatus::Ok ==
            leb128::encode_i64(err_bt, i, slice, &write_offset),
        err_bt);

    u64 read_offset = 0;
    i64 num = I64_MAX;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok ==
            leb128::decode_i64(err_bt, slice.as_const(), &read_offset, &num),
        err_bt);

    assert(num == i);
    assert(read_offset == write_offset);
}

static void unsigned_roundtrip(Backtrace *err_bt, SliceMut<u8> slice, u64 i) {
    u64 write_offset = 0;
    testing::assert_with_trace(
        leb128::EncodeStatus::Ok ==
            leb128::encode_u64(err_bt, i, slice, &write_offset),
        err_bt);

    u64 read_offset = 0;
    u64 num = U64_MAX;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok ==
            leb128::decode_u64(err_bt, slice.as_const(), &read_offset, &num),
        err_bt);

    assert(num == i);
    assert(read_offset == write_offset);
}

static void test_limits(Backtrace *err_bt) {
    u8 buf_buf[15];
    SliceMut<u8> buf = SliceMut<u8>::init_unsafe(buf_buf, sizeof(buf_buf));
    buf.memset(0xaa);

    unsigned_roundtrip(err_bt, buf, U64_MAX);
    unsigned_roundtrip(err_bt, buf, 0);
    unsigned_roundtrip(err_bt, buf, 1);

    signed_roundtrip(err_bt, buf, I64_MAX);
    signed_roundtrip(err_bt, buf, I64_MIN);
    signed_roundtrip(err_bt, buf, -1);
    signed_roundtrip(err_bt, buf, 1);
}

static void test_roundtrip(Backtrace *err_bt) {
    u8 buf_buf[15];
    SliceMut<u8> buf = SliceMut<u8>::init_unsafe(buf_buf, sizeof(buf_buf));
    buf.memset(0xaa);

    for (u64 i = 0; i < 1025; i++) {
        unsigned_roundtrip(err_bt, buf, i);
    }

    for (i64 i = -1025; i < 1024; i++) {
        signed_roundtrip(err_bt, buf, i);
    }
}

static void test_read_with_overflow(Backtrace *err_bt) {
    u8 buf_buf[] = {
        0xff__wbu, 0xff__wbu, 0xff__wbu,       0xff__wbu,
        0xff__wbu, 0xff__wbu, 0xff__wbu,       0xff__wbu,
        0xff__wbu, 0xff__wbu, 0b01111111__wbu, 0b11100100__wbu,
    };

    SliceMut<u8> buf = SliceMut<u8>::init_unsafe(buf_buf, sizeof(buf_buf));

    u64 offset = 0;
    u64 num = 0;
    assert(leb128::DecodeStatus::InvalidInput ==
           leb128::decode_u64(err_bt, buf.as_const(), &offset, &num));

    offset = 0;
    i64 inum = 0;
    assert(leb128::DecodeStatus::InvalidInput ==
           leb128::decode_i64(err_bt, buf.as_const(), &offset, &inum));
}

static void test_terminating_overlong(Backtrace *err_bt) {
    // Ten-byte LEB128 value whose tenth byte terminates but has high bits
    // set. This does not fit in u64/i64 and must be rejected. Nine leading
    // zero-payload continuation bytes followed by 0x02 encodes 2^64.
    u8 buf_buf[] = {
        0x80__wbu, 0x80__wbu, 0x80__wbu, 0x80__wbu, 0x80__wbu,
        0x80__wbu, 0x80__wbu, 0x80__wbu, 0x80__wbu, 0x02__wbu,
    };

    Slice<u8> buf = Slice<u8>::init_unsafe(buf_buf, sizeof(buf_buf));

    u64 offset = 0;
    u64 num = 0;
    assert(leb128::DecodeStatus::InvalidInput ==
           leb128::decode_u64(err_bt, buf, &offset, &num));

    offset = 0;
    i64 inum = 0;
    assert(leb128::DecodeStatus::InvalidInput ==
           leb128::decode_i64(err_bt, buf, &offset, &inum));
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    test_unsigned(&err_bt);
    test_signed(&err_bt);
    test_limits(&err_bt);
    test_read_with_overflow(&err_bt);
    test_terminating_overlong(&err_bt);
    test_roundtrip(&err_bt);

    return 0;
}
