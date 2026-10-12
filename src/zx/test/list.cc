import zx;

using namespace zx;

void test_empty() {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    List<u8> list = List<u8>::init(SliceMut<u8>::init_dangling());

    testing::assert_with_trace(
        ListPushStatus::OutOfCapacity == list.push(&err_ctx, 69), &err_ctx);

    u8 val = 69;
    testing::assert_with_trace(ListPopStatus::Empty == list.pop(&err_ctx, &val),
                               &err_ctx);
}

void test_basic() {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    i32 buf[100];
    SliceMut<i32> slice =
        SliceMut<i32>::init_unsafe(buf, sizeof(buf) / sizeof(buf[0]));
    assert(slice.len() == 100);
    List<i32> list = List<i32>::init(slice);

    assert(list.n_written() == 0);

    testing::assert_with_trace(ListPushStatus::Ok == list.push(&err_ctx, 105),
                               &err_ctx);

    i32 val = -5;

    testing::assert_with_trace(ListPopStatus::Ok == list.pop(&err_ctx, &val),
                               &err_ctx);

    assert(val == 105);

    assert(ListPopStatus::Empty == list.pop(&err_ctx, &val));

    for (i32 i = 0; i < 100; i++) {
        testing::assert_with_trace(
            ListPushStatus::Ok == list.push(&err_ctx, -i), &err_ctx);
        assert(list.written().len() == (u64)(i + 1));
    }

    assert(list.written().len() == 100);
    assert(list.is_full());

    for (i32 i = 99; i >= 0; i--) {
        i32 out = 0;
        testing::assert_with_trace(
            ListPopStatus::Ok == list.pop(&err_ctx, &out), &err_ctx);
        assert(-i == out);
        assert(list.written().len() == (u64)i);
        assert(!list.is_full());
    }

    assert(list.n_written() == 0);

    for (i32 i = 0; i < 100; i++) {
        testing::assert_with_trace(
            ListPushStatus::Ok == list.push(&err_ctx, -i), &err_ctx);
        assert(list.written().len() == (u64)(i + 1));
    }

    assert(ListPushStatus::OutOfCapacity == list.push(&err_ctx, 105));

    list.reset();
    assert(list.n_written() == 0);

    for (i32 i = 0; i < 100; i++) {
        testing::assert_with_trace(
            ListPushStatus::Ok == list.push(&err_ctx, -i), &err_ctx);
        assert(list.written().len() == (u64)(i + 1));
    }
    assert(list.n_written() == 100);
}

int main() {
    test_empty();
    test_basic();

    return 0;
}
