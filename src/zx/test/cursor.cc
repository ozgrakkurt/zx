import zx;

using namespace zx;

void test_cursor() {
    Cursor empty = Cursor::init(Slice<u8>::init_dangling());

    u8 val8 = 69;
    i32 val32 = -1131;

    assert(empty.is_finished());

    assert(Cursor::Status::OutOfBounds == empty.read<u8>(&val8));
    assert(Cursor::Status::OutOfBounds == empty.read<i32>(&val32));

    assert(empty.is_finished());
}

int main() {
    test_cursor();

    return 0;
}
