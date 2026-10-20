import zx;

using namespace zx;

typedef u256 key_type;

using QueueT = Queue<key_type>;

static void smoke_test_inner(Backtrace *err_bt, QueueT *q, u32 cap) {
    assert(q->capacity_left() == cap);
    assert(q->len() == 0);

    key_type key = 69;
    assert(QueuePopStatus::Empty == q->pop(nullptr, &key));
    assert(69 == key);

    for (u32 i = 0; i < cap; i++) {
        key = i;
        testing::assert_with_trace(QueuePushStatus::Ok == q->push(err_bt, &key),
                                   err_bt);
        assert(q->len() == add_unsafe_u32(1, i));
        assert(q->capacity_left() == sub_unsafe_u32(sub_unsafe_u32(cap, i), 1));
    }

    assert(q->capacity_left() == 0);
    key = 69;
    assert(QueuePushStatus::OutOfCapacity == q->push(nullptr, &key));

    for (u32 i = 0; i < cap; i++) {
        testing::assert_with_trace(QueuePopStatus::Ok == q->pop(err_bt, &key),
                                   err_bt);
        assert(key == i);
        assert(q->len() == sub_unsafe_u32(sub_unsafe_u32(cap, i), 1));
        assert(q->capacity_left() == add_unsafe_u32(1, i));
    }

    assert(q->capacity_left() == cap);
    assert(q->len() == 0);
    key = 69;
    assert(QueuePopStatus::Empty == q->pop(nullptr, &key));
    assert(69 == key);
}

static void smoke_test(Backtrace *err_bt, Alloc *alloc, u32 cap) {
    QueueT queue;
    testing::assert_with_trace(QueueInitStatus::Ok ==
                                   QueueT::init(err_bt, alloc, cap, &queue),
                               err_bt);

    smoke_test_inner(err_bt, &queue, cap);
    queue.reset();
    smoke_test_inner(err_bt, &queue, cap);
    queue.reset();
    smoke_test_inner(err_bt, &queue, cap);
}

static void zero_size_test(Backtrace *err_bt) {
    Alloc noalloc = Alloc::init(SliceMut<u8>::init_dangling());

    QueueT queue;
    assert(QueueInitStatus::OutOfMemory ==
           QueueT::init(nullptr, &noalloc, 69, &queue));
    assert(QueueInitStatus::OutOfMemory ==
           QueueT::init(nullptr, &noalloc, 1, &queue));

    testing::assert_with_trace(QueueInitStatus::Ok ==
                                   QueueT::init(err_bt, &noalloc, 0, &queue),
                               err_bt);

    // should be empty
    key_type key = 69;
    assert(QueuePopStatus::Empty == queue.pop(nullptr, &key));

    // should be full
    key = 69;
    assert(QueuePushStatus::OutOfCapacity == queue.push(nullptr, &key));

    // should be empty
    key = 69;
    assert(QueuePopStatus::Empty == queue.pop(nullptr, &key));

    // should be full
    key = 69;
    assert(QueuePushStatus::OutOfCapacity == queue.push(nullptr, &key));
}

int main(void) {
    constexpr u64 MEMORY_SIZE = 1 << 20;

    Backtrace err_bt = Backtrace::init();

    u8 *mem;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_bt, MEMORY_SIZE, &mem),
        &err_bt);

    zero_size_test(&err_bt);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, MEMORY_SIZE));

    constexpr u32 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                               65, 63, 64,   128,  127, 129,  3169};
    constexpr u32 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

    for (u32 i = 0; i < n_lengths; i++) {
        u32 const len = lengths[i];
        alloc.reset();
        smoke_test(&err_bt, &alloc, len);
    }

    testing::assert_with_trace(page_alloc::FreeStatus::Ok ==
                                   page_alloc::free(&err_bt, mem, MEMORY_SIZE),
                               &err_bt);

    return 0;
}
