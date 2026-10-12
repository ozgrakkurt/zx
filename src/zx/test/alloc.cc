import zx;

using namespace zx;

constexpr u64 MEMORY_SIZE = u64{1} << 16;

static void do_test(Alloc *alloc) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    SliceMut<i32 *> slice;
    testing::assert_with_trace(AllocStatus::Ok ==
                                   alloc->alloc<i32 *>(&err_ctx, 100, &slice),
                               &err_ctx);
    assert(slice.len() == 100);

    for (u64 i = 0; i < 100; i++) {
        testing::assert_with_trace(
            AllocStatus::Ok ==
                alloc->create<i32>(&err_ctx, slice.offset_mut_unsafe(i)),
            &err_ctx);
        *slice.read_unsafe(i) = (i32)i;
    }

    for (u64 i = 0; i < 100; i++) {
        assert(*slice.read_unsafe(i) == (i32)i);
    }

    SliceMut<u256> zero_len;
    testing::assert_with_trace(AllocStatus::Ok ==
                                   alloc->alloc<u256>(&err_ctx, 0, &zero_len),
                               &err_ctx);
    assert(zero_len.len() == 0);

    // not nullptr and also not a valid address
    u8 *const dangling = (u8 *)alignof(u8);

    {
        SliceMut<u8> big = SliceMut<u8>::init_dangling();
        assert(AllocStatus::OutOfMemory ==
               alloc->alloc<u8>(&err_ctx, MEMORY_SIZE, &big));
        assert(big.ptr() == dangling);
        assert(big.len() == 0);
    }

    alloc->reset();

    {
        SliceMut<u8> big = SliceMut<u8>::init_dangling();
        testing::assert_with_trace(
            AllocStatus::Ok == alloc->alloc<u8>(&err_ctx, MEMORY_SIZE, &big),
            &err_ctx);
        assert(big.ptr() != dangling);
        assert(big.len() == MEMORY_SIZE);
    }

    SliceMut<u8> small = SliceMut<u8>::init_dangling();
    assert(AllocStatus::OutOfMemory == alloc->alloc<u8>(&err_ctx, 1, &small));
    assert(small.ptr() == dangling);
    assert(small.len() == 0);

    SliceMut<u256> zero2 = SliceMut<u256>::init_dangling();
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->alloc<u256>(&err_ctx, 0, &zero2), &err_ctx);
    assert(zero2.ptr() != (u256 *)dangling);
    assert(__builtin_is_aligned(zero2.ptr(), alignof(u256)));
}

static void do_test_nocap() {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    Alloc alloc = Alloc::init(SliceMut<u8>::init_dangling());

    SliceMut<u64> slice;
    AllocStatus const status = alloc.alloc<u64>(&err_ctx, 0, &slice);
    testing::assert_with_trace(AllocStatus::Ok == status, &err_ctx);

    assert(AllocStatus::OutOfMemory == alloc.alloc<u64>(&err_ctx, 1, &slice));
    assert(AllocStatus::OutOfMemory == alloc.alloc<u64>(&err_ctx, 69, &slice));
}

static void do_test_resize(SliceMut<u8> mem) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    constexpr u64 CAP = 256;
    constexpr u64 BUF_LEN = CAP / 2;

    assert(mem.len() >= CAP);
    Alloc alloc = Alloc::init(mem.slice_unsafe(0, CAP));

    SliceMut<u8> nums;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.alloc<u8>(&err_ctx, BUF_LEN, &nums), &err_ctx);

    for (u8 i = 0; i < BUF_LEN; i++) {
        nums.write_unsafe(i, i);
    }

    u64 const resize_offset = alloc.offset();
    assert(resize_offset == nums.len());

    SliceMut<u8> nums2;
    testing::assert_with_trace(AllocStatus::Ok ==
                                   alloc.alloc<u8>(&err_ctx, BUF_LEN, &nums2),
                               &err_ctx);

    for (u8 i = 0; i < BUF_LEN; i++) {
        nums2.write_unsafe(i, ~i);
    }

    SliceMut<u8> nums3;
    assert(AllocStatus::OutOfMemory ==
           alloc.alloc<u8>(&err_ctx, BUF_LEN, &nums3));

    testing::assert_with_trace(Alloc::ResizeStatus::Ok ==
                                   alloc.resize(&err_ctx, resize_offset),
                               &err_ctx);

    testing::assert_with_trace(AllocStatus::Ok ==
                                   alloc.alloc<u8>(&err_ctx, BUF_LEN, &nums3),
                               &err_ctx);

    for (u8 i = 0; i < BUF_LEN; i++) {
        nums3.write_unsafe(i, 69);
    }

    for (u8 i = 0; i < BUF_LEN; i++) {
        assert(nums.read_unsafe(i) == i);
    }
}

int main() {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    u8 *mem = nullptr;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_ctx, MEMORY_SIZE, &mem),
        &err_ctx);

    SliceMut<u8> slice = SliceMut<u8>::init_unsafe(mem, MEMORY_SIZE);

    do_test_nocap();

    Alloc alloc = Alloc::init(slice);
    do_test(&alloc);

    do_test_resize(slice);

    testing::assert_with_trace(page_alloc::FreeStatus::Ok ==
                                   page_alloc::free(&err_ctx, mem, MEMORY_SIZE),
                               &err_ctx);

    return 0;
}
