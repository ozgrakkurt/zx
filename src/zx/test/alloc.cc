import zx;

constexpr zx::u64 MEMORY_SIZE = zx::u64{1} << 16;

static void do_test(zx::Alloc *alloc) {
    zx::SliceMut<zx::i32 *> slice;
    zx::assert(zx::AllocStatus::Ok == alloc->alloc<zx::i32 *>(100, &slice));
    zx::assert(slice.len() == 100);

    for (zx::u64 i = 0; i < 100; i++) {
        zx::assert(zx::AllocStatus::Ok ==
                   alloc->create<zx::i32>(slice.offset_unsafe(i)));
        slice.write_unsafe(i, (zx::i32)i);
    }

    for (zx::u64 i = 0; i < 100; i++) {
        zx::assert(*slice.read_unsafe(i) == (zx::i32)i);
    }

    zx::u256 *zero_len = nullptr;
    zx::assert(zx::AllocStatus::Ok == alloc->alloc<zx::u256>(0, &zero_len));
    zx::assert(zero_len.len() == 0);

    // not nullptr and also not a valid address
    zx::u8 *const dangling = (zx::u8 *)alignof(zx::u8);

    {
        zx::u8 *big = dangling;
        zx::assert(zx::RawAllocStatus::OutOfMemory ==
                   alloc->alloc<zx::u8>(MEMORY_SIZE, &big));
        zx::assert(big == dangling);
    }

    alloc->reset();

    {
        zx::u8 *big = dangling;
        zx::assert(zx::RawAllocStatus::Ok ==
                   alloc->alloc<zx::u8>(MEMORY_SIZE, &big));
        zx::assert(big != dangling);
    }

    zx::u8 *small = dangling;
    zx::assert(zx::RawAllocStatus::OutOfMemory ==
               alloc->alloc<zx::u8>(1, &small));
    zx::assert(small == dangling);

    zx::u256 *zero2 = nullptr;
    zx::assert(zx::RawAllocStatus::Ok == alloc->alloc<zx::u256>(0, &zero2));
    zx::assert(zero2 != nullptr);
    zx::assert(__builtin_is_aligned(zero2, alignof(zx::u256)));
}

static void do_test_nocap() {
    zx::Alloc alloc = zx::Alloc::init(zx::SliceMut<zx::u8>::init_dangling());

    zx::SliceMut<zx::u64> slice;
    zx::AllocStatus const status = alloc.alloc<zx::u64>(0, &slice);
    zx::assert(zx::AllocStatus::Ok == status);
}

int main() {
    zx::u8 *mem = nullptr;
    zx::assert(zx::page_alloc::AllocStatus::Ok ==
               zx::page_alloc::alloc_4kb(MEMORY_SIZE, &mem));

    zx::SliceMut<zx::u8> slice =
        zx::SliceMut<zx::u8>::init_unsafe(mem, MEMORY_SIZE);

    do_test_nocap();

    zx::RawAlloc alloc = zx::RawAlloc::init(slice);
    do_test(&alloc);

    zx::assert(zx::page_alloc::FreeStatus::Ok ==
               zx::page_alloc::free(mem, MEMORY_SIZE));

    return 0;
}
