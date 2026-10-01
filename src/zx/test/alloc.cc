import zx;

constexpr zx::u64 MEMORY_SIZE = zx::u64{1} << 16;

static void do_test(zx::Alloc *alloc) {
    zx::SliceMut<zx::i32 *> slice;
    zx::assert(zx::AllocStatus::Ok == alloc->alloc<zx::i32 *>(100, &slice));
    zx::assert(slice.len() == 100);

    for (zx::u64 i = 0; i < 100; i++) {
        zx::assert(zx::AllocStatus::Ok ==
                   alloc->create<zx::i32>(slice.offset_mut_unsafe(i)));
        *slice.read_unsafe(i) = (zx::i32)i;
    }

    for (zx::u64 i = 0; i < 100; i++) {
        zx::assert(*slice.read_unsafe(i) == (zx::i32)i);
    }

    zx::SliceMut<zx::u256> zero_len;
    zx::assert(zx::AllocStatus::Ok == alloc->alloc<zx::u256>(0, &zero_len));
    zx::assert(zero_len.len() == 0);

    // not nullptr and also not a valid address
    zx::u8 *const dangling = (zx::u8 *)alignof(zx::u8);

    {
        zx::SliceMut<zx::u8> big = zx::SliceMut<zx::u8>::init_dangling();
        zx::assert(zx::AllocStatus::OutOfMemory ==
                   alloc->alloc<zx::u8>(MEMORY_SIZE, &big));
        zx::assert(big.ptr() == dangling);
        zx::assert(big.len() == 0);
    }

    alloc->reset();

    {
        zx::SliceMut<zx::u8> big = zx::SliceMut<zx::u8>::init_dangling();
        zx::assert(zx::AllocStatus::Ok ==
                   alloc->alloc<zx::u8>(MEMORY_SIZE, &big));
        zx::assert(big.ptr() != dangling);
        zx::assert(big.len() == MEMORY_SIZE);
    }

    zx::SliceMut<zx::u8> small = zx::SliceMut<zx::u8>::init_dangling();
    zx::assert(zx::AllocStatus::OutOfMemory == alloc->alloc<zx::u8>(1, &small));
    zx::assert(small.ptr() == dangling);
    zx::assert(small.len() == 0);

    zx::SliceMut<zx::u256> zero2 = zx::SliceMut<zx::u256>::init_dangling();
    zx::assert(zx::AllocStatus::Ok == alloc->alloc<zx::u256>(0, &zero2));
    zx::assert(zero2.ptr() != (zx::u256 *)dangling);
    zx::assert(__builtin_is_aligned(zero2.ptr(), alignof(zx::u256)));
}

static void do_test_nocap() {
    zx::Alloc alloc = zx::Alloc::init(zx::SliceMut<zx::u8>::init_dangling());

    zx::SliceMut<zx::u64> slice;
    zx::AllocStatus const status = alloc.alloc<zx::u64>(0, &slice);
    zx::assert(zx::AllocStatus::Ok == status);
}

static void do_test_resize(SliceMut<u8> mem) {
    constexpr u64 CAP = 256;
    constexpr u64 BUF_LEN = CAP / 2;

    assert(mem.len() >= CAP);
    zx::Alloc alloc = zx::Alloc::init(mem.slice_unsafe(0, CAP));

    SliceMut<u8> nums;
    assert(AllocStatus::Ok == alloc.alloc<u8>(BUF_LEN, &nums));

    for (u8 i = 0; i < BUF_LEN; i++) {
        nums.write_unsafe(i, i);
    }

    u64 const resize_offset = alloc.offset();
    assert(resize_offset == nums.len());

    SliceMut<u8> nums2;
    assert(AllocStatus::Ok == alloc.alloc<u8>(BUF_LEN, &nums2));

    for (u8 i = 0; i < BUF_LEN; i++) {
        nums2.write_unsafe(i, ~i);
    }

    SliceMut<u8> nums3;
    assert(AllocStatus::OutOfMemory == alloc.alloc<u8>(BUF_LEN, &nums3));

    alloc.resize(resize_offset);

    assert(AllocStatus::Ok == alloc.alloc<u8>(BUF_LEN, &nums3));

    for (u8 i = 0; i < BUF_LEN; i++) {
        nums3.write_unsafe(i, 69);
    }

    for (u8 i = 0; i < BUF_LEN; i++) {
        assert(nums.read_unsafe(i) == i);
    }
}

int main() {
    zx::u8 *mem = nullptr;
    zx::assert(zx::page_alloc::AllocStatus::Ok ==
               zx::page_alloc::alloc_4kb(MEMORY_SIZE, &mem));

    zx::SliceMut<zx::u8> slice =
        zx::SliceMut<zx::u8>::init_unsafe(mem, MEMORY_SIZE);

    do_test_nocap();

    zx::Alloc alloc = zx::Alloc::init(slice);
    do_test(&alloc);

    do_test_resize(slice);

    zx::assert(zx::page_alloc::FreeStatus::Ok ==
               zx::page_alloc::free(mem, MEMORY_SIZE));

    return 0;
}
