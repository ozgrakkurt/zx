import zx;

using namespace zx;

constexpr u64 MEMORY_SIZE = u64{1} << 16;

static void do_test(Backtrace *err_bt, DEAlloc *alloc) {
    SliceMut<i32 *> left_ptrs;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->left_alloc<i32 *>(err_bt, 100, &left_ptrs),
        err_bt);

    SliceMut<i32 *> right_ptrs;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->right_alloc<i32 *>(err_bt, 100, &right_ptrs),
        err_bt);

    assert(left_ptrs.len() == 100);
    assert(right_ptrs.len() == 100);

    for (u64 i = 0; i < 100; i++) {
        testing::assert_with_trace(
            AllocStatus::Ok ==
                alloc->left_create<i32>(err_bt, left_ptrs.offset_mut_unsafe(i)),
            err_bt);
        *left_ptrs.read_unsafe(i) = (i32)i;

        testing::assert_with_trace(
            AllocStatus::Ok == alloc->right_create<i32>(
                                   err_bt, right_ptrs.offset_mut_unsafe(i)),
            err_bt);
        *right_ptrs.read_unsafe(i) = (i32)(100 + i);
    }

    for (u64 i = 0; i < 100; i++) {
        assert(*left_ptrs.read_unsafe(i) == (i32)i);
        assert(*right_ptrs.read_unsafe(i) == (i32)(100 + i));
    }

    // allocations from the two ends must not overlap
    assert(!left_ptrs.as_const().is_overlapping(right_ptrs.as_const()));

    // the page allocated memory is page aligned so these allocations don't
    // need any alignment padding.
    u64 const used_per_side = add_unsafe_u64(mul_unsafe_u64(100, sizeof(i32 *)),
                                             mul_unsafe_u64(100, sizeof(i32)));
    assert(alloc->left_offset() == used_per_side);
    assert(alloc->right_offset() == sub_unsafe_u64(MEMORY_SIZE, used_per_side));
    assert(alloc->capacity_left() ==
           sub_unsafe_u64(MEMORY_SIZE, mul_unsafe_u64(2, used_per_side)));

    // zero length allocations from both ends
    //
    // initialize with a sentinel so we can tell that the allocator actually
    // wrote to the out parameter.
    SliceMut<u256> left_zero =
        SliceMut<u256>::init_unsafe((u256 *)u64{0x1000}, 4);
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->left_alloc<u256>(err_bt, 0, &left_zero),
        err_bt);
    assert(left_zero.len() == 0);
    assert(left_zero.ptr() == (u256 *)alignof(u256));

    SliceMut<u256> right_zero =
        SliceMut<u256>::init_unsafe((u256 *)u64{0x1000}, 4);
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->right_alloc<u256>(err_bt, 0, &right_zero),
        err_bt);
    assert(right_zero.len() == 0);
    assert(right_zero.ptr() == (u256 *)alignof(u256));

    // alignment must be honored on both ends even when align > sizeof(T)
    SliceMut<u8> left_aligned;
    testing::assert_with_trace(
        AllocStatus::Ok ==
            alloc->left_aligned_alloc<u8>(err_bt, 64, 3, &left_aligned),
        err_bt);
    assert(left_aligned.len() == 3);
    assert(__builtin_is_aligned(left_aligned.ptr(), 64));

    SliceMut<u8> right_aligned;
    testing::assert_with_trace(
        AllocStatus::Ok ==
            alloc->right_aligned_alloc<u8>(err_bt, 64, 3, &right_aligned),
        err_bt);
    assert(right_aligned.len() == 3);
    assert(__builtin_is_aligned(right_aligned.ptr(), 64));

    // invalid arguments are rejected on both ends
    SliceMut<u8> tmp;
    assert(AllocStatus::InvalidAlign ==
           alloc->left_aligned_alloc<u8>(err_bt, 3, 1, &tmp));
    assert(AllocStatus::InvalidAlign ==
           alloc->right_aligned_alloc<u8>(err_bt, 3, 1, &tmp));
    assert(AllocStatus::InvalidAlign ==
           alloc->left_aligned_alloc<u8>(err_bt, 256, 1, &tmp));
    assert(AllocStatus::InvalidAlign ==
           alloc->right_aligned_alloc<u8>(err_bt, 256, 1, &tmp));
    assert(AllocStatus::Nullptr ==
           alloc->left_aligned_alloc<u8>(err_bt, 8, 1, nullptr));
    assert(AllocStatus::Nullptr ==
           alloc->right_aligned_alloc<u8>(err_bt, 8, 1, nullptr));

    SliceMut<u64> tmp_u64;
    assert(AllocStatus::InvalidAlign ==
           alloc->left_aligned_alloc<u64>(err_bt, 4, 1, &tmp_u64));
    assert(AllocStatus::InvalidAlign ==
           alloc->right_aligned_alloc<u64>(err_bt, 4, 1, &tmp_u64));

    // not enough memory from either end
    u8 *const dangling = (u8 *)alignof(u8);

    {
        SliceMut<u8> big = SliceMut<u8>::init_dangling();
        assert(AllocStatus::OutOfMemory ==
               alloc->left_alloc<u8>(err_bt, MEMORY_SIZE, &big));
        assert(big.ptr() == dangling);
        assert(big.len() == 0);
    }

    {
        SliceMut<u8> big = SliceMut<u8>::init_dangling();
        assert(AllocStatus::OutOfMemory ==
               alloc->right_alloc<u8>(err_bt, MEMORY_SIZE, &big));
        assert(big.ptr() == dangling);
        assert(big.len() == 0);
    }

    // resetting the right end must not invalidate left side allocations
    alloc->right_reset();
    assert(alloc->right_offset() == MEMORY_SIZE);
    for (u64 i = 0; i < 100; i++) {
        assert(*left_ptrs.read_unsafe(i) == (i32)i);
    }

    SliceMut<u8> right_buf;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->right_alloc<u8>(err_bt, 64, &right_buf),
        err_bt);
    for (u64 i = 0; i < 64; i++) {
        right_buf.write_unsafe(i, (u8)(i + 1));
    }

    // resetting the left end must not invalidate right side allocations
    alloc->left_reset();
    assert(alloc->left_offset() == 0);
    for (u64 i = 0; i < 64; i++) {
        assert(right_buf.read_unsafe(i) == (u8)(i + 1));
    }

    // full reset frees both ends
    alloc->full_reset();
    assert(alloc->left_offset() == 0);
    assert(alloc->right_offset() == MEMORY_SIZE);
    assert(alloc->capacity_left() == MEMORY_SIZE);

    {
        SliceMut<u8> big = SliceMut<u8>::init_dangling();
        testing::assert_with_trace(
            AllocStatus::Ok == alloc->left_alloc<u8>(err_bt, MEMORY_SIZE, &big),
            err_bt);
        assert(big.ptr() != dangling);
        assert(big.len() == MEMORY_SIZE);
    }

    SliceMut<u8> small_left = SliceMut<u8>::init_dangling();
    assert(AllocStatus::OutOfMemory ==
           alloc->left_alloc<u8>(err_bt, 1, &small_left));
    assert(small_left.ptr() == dangling);
    assert(small_left.len() == 0);

    SliceMut<u8> small_right = SliceMut<u8>::init_dangling();
    assert(AllocStatus::OutOfMemory ==
           alloc->right_alloc<u8>(err_bt, 1, &small_right));
    assert(small_right.ptr() == dangling);
    assert(small_right.len() == 0);

    SliceMut<u256> left_zero2 =
        SliceMut<u256>::init_unsafe((u256 *)u64{0x1000}, 4);
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->left_alloc<u256>(err_bt, 0, &left_zero2),
        err_bt);
    assert(left_zero2.ptr() == (u256 *)alignof(u256));
    assert(left_zero2.len() == 0);

    SliceMut<u256> right_zero2 =
        SliceMut<u256>::init_unsafe((u256 *)u64{0x1000}, 4);
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->right_alloc<u256>(err_bt, 0, &right_zero2),
        err_bt);
    assert(right_zero2.ptr() == (u256 *)alignof(u256));
    assert(right_zero2.len() == 0);
}

static void do_test_nocap(Backtrace *err_bt) {
    DEAlloc alloc = DEAlloc::init(SliceMut<u8>::init_dangling());

    assert(alloc.capacity_left() == 0);

    SliceMut<u64> left_slice;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.left_alloc<u64>(err_bt, 0, &left_slice),
        err_bt);

    SliceMut<u64> right_slice;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.right_alloc<u64>(err_bt, 0, &right_slice),
        err_bt);

    assert(AllocStatus::OutOfMemory ==
           alloc.left_alloc<u64>(err_bt, 1, &left_slice));
    assert(AllocStatus::OutOfMemory ==
           alloc.left_alloc<u64>(err_bt, 69, &left_slice));

    assert(AllocStatus::OutOfMemory ==
           alloc.right_alloc<u64>(err_bt, 1, &right_slice));
    assert(AllocStatus::OutOfMemory ==
           alloc.right_alloc<u64>(err_bt, 69, &right_slice));

    alloc.full_reset();
    assert(alloc.capacity_left() == 0);
}

static void do_test_resize(Backtrace *err_bt, SliceMut<u8> mem) {
    constexpr u64 CAP = 256;
    constexpr u64 BUF_LEN = CAP / 4;

    assert(mem.len() >= CAP);
    DEAlloc alloc = DEAlloc::init(mem.slice_unsafe(0, CAP));

    SliceMut<u8> left_nums;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.left_alloc<u8>(err_bt, BUF_LEN, &left_nums),
        err_bt);

    for (u8 i = 0; i < BUF_LEN; i++) {
        left_nums.write_unsafe(i, i);
    }

    u64 const left_resize_offset = alloc.left_offset();
    assert(left_resize_offset == left_nums.len());

    SliceMut<u8> right_nums;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.right_alloc<u8>(err_bt, BUF_LEN, &right_nums),
        err_bt);

    for (u8 i = 0; i < BUF_LEN; i++) {
        right_nums.write_unsafe(i, ~i);
    }

    u64 const right_resize_offset = alloc.right_offset();
    assert(right_resize_offset == sub_unsafe_u64(CAP, right_nums.len()));

    SliceMut<u8> left_tmp;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.left_alloc<u8>(err_bt, BUF_LEN, &left_tmp),
        err_bt);

    for (u8 i = 0; i < BUF_LEN; i++) {
        left_tmp.write_unsafe(i, 69);
    }

    SliceMut<u8> right_tmp;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.right_alloc<u8>(err_bt, BUF_LEN, &right_tmp),
        err_bt);

    for (u8 i = 0; i < BUF_LEN; i++) {
        right_tmp.write_unsafe(i, 42);
    }

    SliceMut<u8> no_fit;
    assert(AllocStatus::OutOfMemory ==
           alloc.left_alloc<u8>(err_bt, 1, &no_fit));
    assert(AllocStatus::OutOfMemory ==
           alloc.right_alloc<u8>(err_bt, 1, &no_fit));

    // free the temporary allocations on both ends
    testing::assert_with_trace(
        AllocResizeStatus::Ok ==
            alloc.right_resize(err_bt, right_resize_offset),
        err_bt);
    testing::assert_with_trace(
        AllocResizeStatus::Ok == alloc.left_resize(err_bt, left_resize_offset),
        err_bt);

    assert(alloc.left_offset() == left_resize_offset);
    assert(alloc.right_offset() == right_resize_offset);
    assert(alloc.capacity_left() == mul_unsafe_u64(2, BUF_LEN));

    // either end can claim the whole shared middle, starving the other end
    testing::assert_with_trace(
        AllocResizeStatus::Ok == alloc.left_resize(err_bt, right_resize_offset),
        err_bt);
    assert(alloc.capacity_left() == 0);
    assert(AllocStatus::OutOfMemory ==
           alloc.left_alloc<u8>(err_bt, 1, &no_fit));
    assert(AllocStatus::OutOfMemory ==
           alloc.right_alloc<u8>(err_bt, 1, &no_fit));

    testing::assert_with_trace(
        AllocResizeStatus::Ok == alloc.left_resize(err_bt, left_resize_offset),
        err_bt);
    testing::assert_with_trace(
        AllocResizeStatus::Ok == alloc.right_resize(err_bt, left_resize_offset),
        err_bt);
    assert(alloc.capacity_left() == 0);
    assert(AllocStatus::OutOfMemory ==
           alloc.left_alloc<u8>(err_bt, 1, &no_fit));
    assert(AllocStatus::OutOfMemory ==
           alloc.right_alloc<u8>(err_bt, 1, &no_fit));

    testing::assert_with_trace(
        AllocResizeStatus::Ok ==
            alloc.right_resize(err_bt, right_resize_offset),
        err_bt);
    assert(alloc.capacity_left() == mul_unsafe_u64(2, BUF_LEN));

    // reallocate the freed middle from both ends
    SliceMut<u8> left_again;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.left_alloc<u8>(err_bt, BUF_LEN, &left_again),
        err_bt);

    SliceMut<u8> right_again;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc.right_alloc<u8>(err_bt, BUF_LEN, &right_again),
        err_bt);

    for (u64 i = 0; i < BUF_LEN; i++) {
        assert(left_nums.read_unsafe(i) == (u8)i);
        assert(right_nums.read_unsafe(i) == (u8)~i);
    }

    for (u64 i = 0; i < BUF_LEN; i++) {
        left_again.write_unsafe(i, 0xaa);
        right_again.write_unsafe(i, 0xbb);
    }

    for (u64 i = 0; i < BUF_LEN; i++) {
        assert(left_nums.read_unsafe(i) == (u8)i);
        assert(right_nums.read_unsafe(i) == (u8)~i);
        assert(left_again.read_unsafe(i) == 0xaa);
        assert(right_again.read_unsafe(i) == 0xbb);
    }

    // resizing past the other end or out of the buffer must fail and must not
    // change either offset
    u64 const left_before_failed_resizes = alloc.left_offset();
    u64 const right_before_failed_resizes = alloc.right_offset();

    assert(AllocResizeStatus::OutOfBounds ==
           alloc.left_resize(err_bt, add_unsafe_u64(alloc.right_offset(), 1)));
    assert(alloc.left_offset() == left_before_failed_resizes);
    assert(alloc.right_offset() == right_before_failed_resizes);

    assert(AllocResizeStatus::OutOfBounds ==
           alloc.right_resize(err_bt, sub_unsafe_u64(alloc.left_offset(), 1)));
    assert(alloc.left_offset() == left_before_failed_resizes);
    assert(alloc.right_offset() == right_before_failed_resizes);

    assert(AllocResizeStatus::OutOfBounds ==
           alloc.right_resize(err_bt, add_unsafe_u64(CAP, 1)));
    assert(alloc.left_offset() == left_before_failed_resizes);
    assert(alloc.right_offset() == right_before_failed_resizes);

    assert(AllocResizeStatus::OutOfBounds ==
           alloc.left_resize(err_bt, add_unsafe_u64(CAP, 1)));
    assert(alloc.left_offset() == left_before_failed_resizes);
    assert(alloc.right_offset() == right_before_failed_resizes);
}

static void do_test_right_align_padding_oom(Backtrace *err_bt,
                                            SliceMut<u8> mem) {
    constexpr u64 CAP = 256;
    assert(mem.len() >= CAP);
    DEAlloc alloc = DEAlloc::init(mem.slice_unsafe(0, CAP));

    // leave 9 bytes in the middle. the right allocation is 8 bytes so its size
    // would fit, but it needs 64 byte alignment which pushes it below the left
    // end.
    SliceMut<u8> left;
    testing::assert_with_trace(AllocStatus::Ok ==
                                   alloc.left_alloc<u8>(err_bt, CAP - 9, &left),
                               err_bt);
    assert(alloc.left_offset() == CAP - 9);
    assert(alloc.right_offset() == CAP);
    assert(alloc.capacity_left() == 9);

    u64 *const sentinel = (u64 *)u64{0x1000};
    SliceMut<u64> right = SliceMut<u64>::init_unsafe(sentinel, 4);
    assert(AllocStatus::OutOfMemory ==
           alloc.right_aligned_alloc<u64>(err_bt, 64, 1, &right));
    assert(right.ptr() == sentinel);
    assert(right.len() == 4);

    // failed allocations must not change the allocator state
    assert(alloc.left_offset() == CAP - 9);
    assert(alloc.right_offset() == CAP);
    assert(alloc.capacity_left() == 9);
}

static void do_test_misaligned_base(Backtrace *err_bt, SliceMut<u8> mem) {
    constexpr u64 CAP = 256;
    assert(mem.len() > CAP);

    // shift the base by one byte so it is not aligned to 64
    SliceMut<u8> const misaligned = mem.slice_unsafe(1, CAP);

    {
        DEAlloc alloc = DEAlloc::init(misaligned);

        SliceMut<u8> left;
        testing::assert_with_trace(
            AllocStatus::Ok ==
                alloc.left_aligned_alloc<u8>(err_bt, 64, 1, &left),
            err_bt);
        assert(left.len() == 1);
        assert(__builtin_is_aligned(left.ptr(), 64));
        // the first 64 aligned address is at offset 63
        assert(alloc.left_offset() == 64);
    }

    {
        DEAlloc alloc = DEAlloc::init(misaligned);

        SliceMut<u8> right;
        testing::assert_with_trace(
            AllocStatus::Ok ==
                alloc.right_aligned_alloc<u8>(err_bt, 64, 192, &right),
            err_bt);
        assert(right.len() == 192);
        assert(__builtin_is_aligned(right.ptr(), 64));
        // the slice starts at offset 63 and ends at the end of the buffer
        assert(alloc.right_offset() == 63);
    }

    {
        // the size equals the whole capacity so it would fit without alignment
        // padding. with a misaligned base there is no 64 byte aligned start,
        // and computing the padding underflows.
        DEAlloc alloc = DEAlloc::init(misaligned);

        u8 *const sentinel = (u8 *)u64{0x1000};
        SliceMut<u8> whole = SliceMut<u8>::init_unsafe(sentinel, 4);
        assert(AllocStatus::OutOfMemory ==
               alloc.right_aligned_alloc<u8>(err_bt, 64, CAP, &whole));
        assert(whole.ptr() == sentinel);
        assert(whole.len() == 4);

        assert(alloc.left_offset() == 0);
        assert(alloc.right_offset() == CAP);
        assert(alloc.capacity_left() == CAP);
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

    DEAlloc alloc = DEAlloc::init(slice);
    do_test(&err_bt, &alloc);

    do_test_resize(&err_bt, slice);
    do_test_right_align_padding_oom(&err_bt, slice);
    do_test_misaligned_base(&err_bt, slice);

    testing::assert_with_trace(page_alloc::FreeStatus::Ok ==
                                   page_alloc::free(&err_bt, mem, MEMORY_SIZE),
                               &err_bt);

    return 0;
}
