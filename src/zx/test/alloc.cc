import zx;

using namespace zx;

constexpr u64 MEMORY_SIZE = u64{1} << 16;

static void do_test(Backtrace *err_bt, Alloc *alloc) {
    SliceMut<i32 *> slice;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->alloc<i32 *>(err_bt, 100, &slice), err_bt);
    assert(slice.len() == 100);

    for (u64 i = 0; i < 100; i++) {
        testing::assert_with_trace(
            AllocStatus::Ok ==
                alloc->create<i32>(err_bt, slice.offset_mut_unsafe(i)),
            err_bt);
        *slice.read_unsafe(i) = (i32)i;
    }

    for (u64 i = 0; i < 100; i++) {
        assert(*slice.read_unsafe(i) == (i32)i);
    }

    SliceMut<u256> zero_len;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->alloc<u256>(err_bt, 0, &zero_len), err_bt);
    assert(zero_len.len() == 0);

    // not nullptr and also not a valid address
    u8 *const dangling = (u8 *)alignof(u8);

    {
        SliceMut<u8> big = SliceMut<u8>::init_dangling();
        assert(AllocStatus::OutOfMemory ==
               alloc->alloc<u8>(err_bt, MEMORY_SIZE, &big));
        assert(big.ptr() == dangling);
        assert(big.len() == 0);
    }

    alloc->reset();

    {
        SliceMut<u8> big = SliceMut<u8>::init_dangling();
        testing::assert_with_trace(
            AllocStatus::Ok == alloc->alloc<u8>(err_bt, MEMORY_SIZE, &big),
            err_bt);
        assert(big.ptr() != dangling);
        assert(big.len() == MEMORY_SIZE);
    }

    SliceMut<u8> small = SliceMut<u8>::init_dangling();
    assert(AllocStatus::OutOfMemory == alloc->alloc<u8>(err_bt, 1, &small));
    assert(small.ptr() == dangling);
    assert(small.len() == 0);

    SliceMut<u256> zero2 = SliceMut<u256>::init_dangling();
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->alloc<u256>(err_bt, 0, &zero2), err_bt);
    assert(zero2.ptr() != (u256 *)dangling);
    assert(__builtin_is_aligned(zero2.ptr(), alignof(u256)));
}

static void do_test_nocap(Backtrace *err_bt) {
    Alloc alloc = Alloc::init(SliceMut<u8>::init_dangling());

    SliceMut<u64> slice;
    AllocStatus const status = alloc.alloc<u64>(err_bt, 0, &slice);
    testing::assert_with_trace(AllocStatus::Ok == status, err_bt);

    assert(AllocStatus::OutOfMemory == alloc.alloc<u64>(err_bt, 1, &slice));
    assert(AllocStatus::OutOfMemory == alloc.alloc<u64>(err_bt, 69, &slice));
}

static void do_test_resize(Backtrace *err_bt, SliceMut<u8> mem) {
    constexpr u64 CAP = 256;
    constexpr u64 BUF_LEN = CAP / 2;

    assert(mem.len() >= CAP);
    Alloc alloc = Alloc::init(mem.slice_unsafe(0, CAP));

    SliceMut<u8> nums;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.alloc<u8>(err_bt, BUF_LEN, &nums), err_bt);

    for (u8 i = 0; i < BUF_LEN; i++) {
        nums.write_unsafe(i, i);
    }

    u64 const resize_offset = alloc.offset();
    assert(resize_offset == nums.len());

    SliceMut<u8> nums2;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.alloc<u8>(err_bt, BUF_LEN, &nums2), err_bt);

    for (u8 i = 0; i < BUF_LEN; i++) {
        nums2.write_unsafe(i, ~i);
    }

    SliceMut<u8> nums3;
    assert(AllocStatus::OutOfMemory ==
           alloc.alloc<u8>(err_bt, BUF_LEN, &nums3));

    testing::assert_with_trace(
        Alloc::ResizeStatus::Ok == alloc.resize(err_bt, resize_offset), err_bt);

    testing::assert_with_trace(
        AllocStatus::Ok == alloc.alloc<u8>(err_bt, BUF_LEN, &nums3), err_bt);

    for (u8 i = 0; i < BUF_LEN; i++) {
        nums3.write_unsafe(i, 69);
    }

    for (u8 i = 0; i < BUF_LEN; i++) {
        assert(nums.read_unsafe(i) == i);
    }
}

int main() {
    Backtrace err_bt = Backtrace::init();

    u8 *mem = nullptr;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_bt, MEMORY_SIZE, &mem),
        &err_bt);

    SliceMut<u8> slice = SliceMut<u8>::init_unsafe(mem, MEMORY_SIZE);

    do_test_nocap(&err_bt);

    Alloc alloc = Alloc::init(slice);
    do_test(&err_bt, &alloc);

    do_test_resize(&err_bt, slice);

    testing::assert_with_trace(page_alloc::FreeStatus::Ok ==
                                   page_alloc::free(&err_bt, mem, MEMORY_SIZE),
                               &err_bt);

    return 0;
}
