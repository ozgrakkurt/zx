import zx;

using namespace zx;

void test_empty() {
    Cursor empty = Cursor::init(Slice<u8>::init_dangling());

    u8 val8 = 69;
    i32 val32 = -1131;

    assert(empty.is_finished());

    assert(Cursor::Status::OutOfBounds == empty.read<u8>(&val8));
    assert(Cursor::Status::OutOfBounds == empty.read<i32>(&val32));

    assert(empty.is_finished());
}

void test_empty_write() {
    WriteCursor empty = WriteCursor::init(SliceMut<u8>::init_dangling());

    assert(WriteCursor::Status::OutOfBounds == empty.write<u8>(69));
    assert(WriteCursor::Status::OutOfBounds == empty.write<i32>(69));
}

void test_write_cursor() {
    u8 buf[100];
    SliceMut<u8> mut_slice = SliceMut<u8>::init_unsafe(buf, 100);
    WriteCursor wc = WriteCursor::init(mut_slice);

    assert(wc.offset() == 0);
    assert(wc.buf().len() == 100);
    assert(!wc.is_finished());
    assert(wc.written().len() == 0);

    // write
    u32 v1 = 0x12345678;
    assert(WriteCursor::Status::Ok == wc.write(v1));
    assert(wc.offset() == 4);

    // write_unsafe
    i32 v2 = -42;
    wc.write_unsafe(v2);
    assert(wc.offset() == 8);

    // write_panic
    u16 v3 = 0xabcd;
    wc.write_panic(v3);
    assert(wc.offset() == 10);

    // written
    assert(wc.written().len() == 10);

    // OutOfBounds
    for (int i = 0; i < 90; i++) {
        wc.write_panic((u8)i);
    }
    assert(wc.offset() == 100);
    assert(wc.is_finished());
    assert(WriteCursor::Status::OutOfBounds == wc.write((u8)0));

    // IntegerOverflow
    WriteCursor wc2 = WriteCursor::init(mut_slice);
    wc2.m_offset = ~0ull - 2;
    assert(WriteCursor::Status::IntegerOverflow == wc2.write(u32{0}));
}

void test_cursor() {
    u8 buf[100];
    SliceMut<u8> mut_slice = SliceMut<u8>::init_unsafe(buf, 100);
    WriteCursor wc = WriteCursor::init(mut_slice);

    // Prepare a known byte sequence
    assert(WriteCursor::Status::Ok == wc.write(u32{0x12345678})); // 4 bytes
    assert(WriteCursor::Status::Ok == wc.write(i32{-42}));        // 4 bytes
    assert(WriteCursor::Status::Ok == wc.write(u16{0xabcd}));     // 2 bytes
    assert(WriteCursor::Status::Ok == wc.write(u8{0x55}));        // 1 byte
    assert(WriteCursor::Status::Ok == wc.write(u8{0x80}));
    assert(WriteCursor::Status::Ok ==
           wc.write(u8{0x01})); // uleb128 128 (2 bytes)
    assert(WriteCursor::Status::Ok ==
           wc.write(u8{0x7f})); // sleb128 -1 (1 byte)
    assert(WriteCursor::Status::Ok == wc.write(u8{0xAC}));
    assert(WriteCursor::Status::Ok ==
           wc.write(u8{0x02})); // uleb128 300 (2 bytes)
    assert(WriteCursor::Status::Ok ==
           wc.write(u8{0x7e})); // sleb128 -2 (1 byte)
    for (int i = 0; i < 10; i++) {
        assert(WriteCursor::Status::Ok == wc.write(u8{0xaa}));
    }

    Slice<u8> written = wc.written();
    Cursor cur = Cursor::init(written);

    assert(cur.offset() == 0);
    assert(cur.buf().len() == written.len());
    assert(!cur.is_finished());

    // read
    u32 v1;
    assert(Cursor::Status::Ok == cur.read(&v1));
    assert(v1 == 0x12345678);
    assert(cur.offset() == 4);

    // read_unsafe
    i32 v2 = cur.read_unsafe<i32>();
    assert(v2 == -42);
    assert(cur.offset() == 8);

    // read_panic
    u16 v3 = cur.read_panic<u16>();
    assert(v3 == 0xabcd);
    assert(cur.offset() == 10);

    u8 v4 = cur.read_panic<u8>();
    assert(v4 == 0x55);
    assert(cur.offset() == 11);

    // read_uleb128
    u64 v5;
    assert(leb128::DecodeStatus::Ok == cur.read_uleb128(&v5));
    assert(v5 == 128);
    assert(cur.offset() == 13);

    // read_sleb128
    i64 v6;
    assert(leb128::DecodeStatus::Ok == cur.read_sleb128(&v6));
    assert(v6 == -1);
    assert(cur.offset() == 14);

    // read_uleb128_panic
    u64 v7 = cur.read_uleb128_panic();
    assert(v7 == 300);
    assert(cur.offset() == 16);

    // read_sleb128_panic
    i64 v8 = cur.read_sleb128_panic();
    assert(v8 == -2);
    assert(cur.offset() == 17);

    // Nullptr
    assert(Cursor::Status::Nullptr == cur.read<u32>(nullptr));

    // skip
    assert(Cursor::Status::Ok == cur.skip(5));
    assert(cur.offset() == 22);

    // OutOfBounds on read
    u64 dummy;
    assert(Cursor::Status::OutOfBounds == cur.read(&dummy));

    // seek
    assert(Cursor::Status::Ok == cur.seek(10));
    assert(cur.offset() == 10);
    assert(Cursor::Status::OutOfBounds == cur.seek(100));

    // IntegerOverflow on skip
    Cursor cur2 = Cursor::init(written);
    cur2.m_offset = ~0ull - 2;
    assert(Cursor::Status::IntegerOverflow == cur2.skip(10));

    // is_finished
    Cursor cur3 = Cursor::init(written);
    assert(Cursor::Status::Ok == cur3.seek(written.len()));
    assert(cur3.is_finished());
}

int main() {
    test_write_cursor();
    test_cursor();
    test_empty();
    test_empty_write();
    return 0;
}
