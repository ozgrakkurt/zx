import zx;

using namespace zx;

void test_empty(Backtrace *err_bt) {
    Cursor empty = Cursor::init(Slice<u8>::init_dangling());

    u8 val8 = 69;
    i32 val32 = -1131;

    assert(empty.is_finished());

    assert(Cursor::Status::OutOfBounds == empty.read<u8>(err_bt, &val8));
    assert(Cursor::Status::OutOfBounds == empty.read<i32>(err_bt, &val32));

    assert(empty.is_finished());
}

void test_empty_write(Backtrace *err_bt) {
    WriteCursor empty = WriteCursor::init(SliceMut<u8>::init_dangling());

    assert(WriteCursor::Status::OutOfBounds == empty.write<u8>(err_bt, 69));
    assert(WriteCursor::Status::OutOfBounds == empty.write<i32>(err_bt, 69));
}

void test_write_cursor(Backtrace *err_bt) {
    u8 buf[100];
    SliceMut<u8> mut_slice = SliceMut<u8>::init_unsafe(buf, 100);
    WriteCursor wc = WriteCursor::init(mut_slice);

    assert(wc.offset() == 0);
    assert(wc.buf().len() == 100);
    assert(!wc.is_finished());
    assert(wc.written().len() == 0);

    // write
    u32 const v1 = 0x12345678;
    testing::assert_with_trace(WriteCursor::Status::Ok == wc.write(err_bt, v1),
                               err_bt);
    assert(wc.offset() == 4);

    i32 const v2 = -42;
    testing::assert_with_trace(WriteCursor::Status::Ok == wc.write(err_bt, v2),
                               err_bt);
    assert(wc.offset() == 8);

    u16 const v3 = 0xabcd;
    testing::assert_with_trace(WriteCursor::Status::Ok == wc.write(err_bt, v3),
                               err_bt);
    assert(wc.offset() == 10);

    // written
    assert(wc.written().len() == 10);

    // OutOfBounds
    for (int i = 0; i < 90; i++) {
        testing::assert_with_trace(
            WriteCursor::Status::Ok == wc.write<u8>(err_bt, (u8)i), err_bt);
    }
    assert(wc.offset() == 100);
    assert(wc.is_finished());
    assert(WriteCursor::Status::OutOfBounds == wc.write(err_bt, (u8)0));

    // IntegerOverflow
    WriteCursor wc2 = WriteCursor::init(mut_slice);
    wc2.m_offset = ~0ull - 2;
    assert(WriteCursor::Status::IntegerOverflow == wc2.write(err_bt, u32{0}));
}

void test_cursor(Backtrace *err_bt) {
    u8 buf[100];
    SliceMut<u8> mut_slice = SliceMut<u8>::init_unsafe(buf, 100);
    WriteCursor wc = WriteCursor::init(mut_slice);

    // Prepare a known byte sequence
    testing::assert_with_trace(WriteCursor::Status::Ok ==
                                   wc.write(err_bt, u32{0x12345678}),
                               err_bt); // 4 bytes
    testing::assert_with_trace(WriteCursor::Status::Ok ==
                                   wc.write(err_bt, i32{-42}),
                               err_bt); // 4 bytes
    testing::assert_with_trace(WriteCursor::Status::Ok ==
                                   wc.write(err_bt, u16{0xabcd}),
                               err_bt); // 2 bytes
    testing::assert_with_trace(WriteCursor::Status::Ok ==
                                   wc.write(err_bt, u8{0x55}),
                               err_bt); // 1 byte
    testing::assert_with_trace(
        WriteCursor::Status::Ok == wc.write(err_bt, u8{0x80}), err_bt);
    testing::assert_with_trace(WriteCursor::Status::Ok ==
                                   wc.write(err_bt, u8{0x01}),
                               err_bt); // uleb128 128 (2 bytes)
    testing::assert_with_trace(WriteCursor::Status::Ok ==
                                   wc.write(err_bt, u8{0x7f}),
                               err_bt); // sleb128 -1 (1 byte)
    testing::assert_with_trace(
        WriteCursor::Status::Ok == wc.write(err_bt, u8{0xAC}), err_bt);
    testing::assert_with_trace(WriteCursor::Status::Ok ==
                                   wc.write(err_bt, u8{0x02}),
                               err_bt); // uleb128 300 (2 bytes)
    testing::assert_with_trace(WriteCursor::Status::Ok ==
                                   wc.write(err_bt, u8{0x7e}),
                               err_bt); // sleb128 -2 (1 byte)
    for (int i = 0; i < 10; i++) {
        testing::assert_with_trace(
            WriteCursor::Status::Ok == wc.write(err_bt, u8{0xaa}), err_bt);
    }

    Slice<u8> written = wc.written();
    Cursor cur = Cursor::init(written);

    assert(cur.offset() == 0);
    assert(cur.buf().len() == written.len());
    assert(!cur.is_finished());

    // read
    u32 v1 = 0;
    testing::assert_with_trace(Cursor::Status::Ok == cur.read<u32>(err_bt, &v1),
                               err_bt);
    assert(v1 == 0x12345678);
    assert(cur.offset() == 4);

    i32 v2 = 0;
    testing::assert_with_trace(Cursor::Status::Ok == cur.read<i32>(err_bt, &v2),
                               err_bt);
    assert(v2 == -42);
    assert(cur.offset() == 8);

    u16 v3 = 0;
    testing::assert_with_trace(Cursor::Status::Ok == cur.read<u16>(err_bt, &v3),
                               err_bt);
    assert(v3 == 0xabcd);
    assert(cur.offset() == 10);

    u8 v4 = 0;
    testing::assert_with_trace(Cursor::Status::Ok == cur.read<u8>(err_bt, &v4),
                               err_bt);
    assert(v4 == 0x55);
    assert(cur.offset() == 11);

    // read_uleb128
    u64 v5;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok == cur.read_uleb128(err_bt, &v5), err_bt);
    assert(v5 == 128);
    assert(cur.offset() == 13);

    // read_sleb128
    i64 v6;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok == cur.read_sleb128(err_bt, &v6), err_bt);
    assert(v6 == -1);
    assert(cur.offset() == 14);

    // read_uleb128
    u64 v7 = 0;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok == cur.read_uleb128(err_bt, &v7), err_bt);
    assert(v7 == 300);
    assert(cur.offset() == 16);

    // read_sleb128
    i64 v8 = 0;
    testing::assert_with_trace(
        leb128::DecodeStatus::Ok == cur.read_sleb128(err_bt, &v8), err_bt);
    assert(v8 == -2);
    assert(cur.offset() == 17);

    // Nullptr
    assert(Cursor::Status::Nullptr == cur.read<u32>(err_bt, nullptr));

    // skip
    testing::assert_with_trace(Cursor::Status::Ok == cur.skip(err_bt, 5),
                               err_bt);
    assert(cur.offset() == 22);

    // OutOfBounds on read
    u64 dummy;
    assert(Cursor::Status::OutOfBounds == cur.read(err_bt, &dummy));

    // seek
    testing::assert_with_trace(Cursor::Status::Ok == cur.seek(err_bt, 10),
                               err_bt);
    assert(cur.offset() == 10);
    assert(Cursor::Status::OutOfBounds == cur.seek(err_bt, 100));

    // IntegerOverflow on skip
    Cursor cur2 = Cursor::init(written);
    cur2.m_offset = ~0ull - 2;
    assert(Cursor::Status::IntegerOverflow == cur2.skip(err_bt, 10));

    // is_finished
    Cursor cur3 = Cursor::init(written);
    testing::assert_with_trace(
        Cursor::Status::Ok == cur3.seek(err_bt, written.len()), err_bt);
    assert(cur3.is_finished());
}

int main() {
    Backtrace err_bt = Backtrace::init();

    test_write_cursor(&err_bt);
    test_cursor(&err_bt);
    test_empty(&err_bt);
    test_empty_write(&err_bt);
    return 0;
}
