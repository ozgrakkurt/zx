import zx;

using namespace zx;

void test_empty(Backtrace *err_bt) {
    List<u8> list = List<u8>::init(SliceMut<u8>::init_dangling());

    testing::assert_with_trace(
        ListPushStatus::OutOfCapacity == list.push(err_bt, 69), err_bt);

    u8 val = 69;
    testing::assert_with_trace(ListPopStatus::Empty == list.pop(err_bt, &val),
                               err_bt);
}

void test_basic(Backtrace *err_bt) {
    i32 buf[100];
    SliceMut<i32> slice =
        SliceMut<i32>::init_unsafe(buf, sizeof(buf) / sizeof(buf[0]));
    assert(slice.len() == 100);
    List<i32> list = List<i32>::init(slice);

    assert(list.n_written() == 0);

    testing::assert_with_trace(ListPushStatus::Ok == list.push(err_bt, 105),
                               err_bt);

    i32 val = -5;

    testing::assert_with_trace(ListPopStatus::Ok == list.pop(err_bt, &val),
                               err_bt);

    assert(val == 105);

    assert(ListPopStatus::Empty == list.pop(err_bt, &val));

    for (i32 i = 0; i < 100; i++) {
        testing::assert_with_trace(ListPushStatus::Ok == list.push(err_bt, -i),
                                   err_bt);
        assert(list.written().len() == (u64)(i + 1));
    }

    assert(list.written().len() == 100);
    assert(list.is_full());

    for (i32 i = 99; i >= 0; i--) {
        i32 out = 0;
        testing::assert_with_trace(ListPopStatus::Ok == list.pop(err_bt, &out),
                                   err_bt);
        assert(-i == out);
        assert(list.written().len() == (u64)i);
        assert(!list.is_full());
    }

    assert(list.n_written() == 0);

    for (i32 i = 0; i < 100; i++) {
        testing::assert_with_trace(ListPushStatus::Ok == list.push(err_bt, -i),
                                   err_bt);
        assert(list.written().len() == (u64)(i + 1));
    }

    assert(ListPushStatus::OutOfCapacity == list.push(err_bt, 105));

    list.reset();
    assert(list.n_written() == 0);

    for (i32 i = 0; i < 100; i++) {
        testing::assert_with_trace(ListPushStatus::Ok == list.push(err_bt, -i),
                                   err_bt);
        assert(list.written().len() == (u64)(i + 1));
    }
    assert(list.n_written() == 100);
}

int main() {
    Backtrace err_bt = Backtrace::init();

    test_empty(&err_bt);
    test_basic(&err_bt);

    return 0;
}
