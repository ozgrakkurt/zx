import zx;

using namespace zx;

using val_type = u256;
using SlabT = Slab<val_type>;

static void smoke_test_inner(Backtrace *err_bt, SlabT *slab,
                             SliceMut<u32> slab_keys, u32 cap) {
    assert(slab->capacity() == cap);
    assert(slab->n_occupied() == 0);

    for (u32 i = 0; i < cap; i++) {
        val_type const val = i;
        u32 idx = U32_MAX;
        assert(SlabInsertStatus::Ok == slab->insert(err_bt, &val, &idx));
        slab_keys.write_unsafe(i, idx);
        val_type const *got_val_p = nullptr;
        assert(SlabAccessStatus::Found == slab->get(err_bt, idx, &got_val_p));
        assert(got_val_p != nullptr);
        assert(*got_val_p == val);

        // backwards test
        val_type *old_val_p = nullptr;
        assert(SlabAccessStatus::Found ==
               slab->get_mut(err_bt, slab_keys.read_unsafe(i / 2), &old_val_p));
        assert(old_val_p != nullptr);
        assert(*old_val_p == val / 2);
    }

    for (u32 i = 0; i < cap / 2; i++) {
        val_type val;
        testing::assert_with_trace(
            SlabAccessStatus::Found ==
                slab->remove(err_bt, slab_keys.read_unsafe(i), &val),
            err_bt);
        assert(val == i);

        val_type const *val_p = nullptr;
        assert(SlabAccessStatus::NotFound ==
               slab->get(nullptr, slab_keys.read_unsafe(i), &val_p));
        assert(val_p == nullptr);

        val_type *val_p_mut = nullptr;
        assert(SlabAccessStatus::NotFound ==
               slab->get_mut(nullptr, slab_keys.read_unsafe(i), &val_p_mut));
        assert(val_p_mut == nullptr);

        assert(SlabAccessStatus::NotFound ==
               slab->remove(nullptr, slab_keys.read_unsafe(i), &val));
    }

    for (u32 i = cap / 2; i < cap; i++) {
        val_type const *val_p = nullptr;
        testing::assert_with_trace(
            SlabAccessStatus::Found ==
                slab->get(err_bt, slab_keys.read_unsafe(i), &val_p),
            err_bt);
        assert(val_p != nullptr);
        assert(*val_p == i);
    }
}

static void smoke_test(Backtrace *err_bt, Alloc *alloc, u32 cap) {
    SlabT slab;
    testing::assert_with_trace(
        SlabInitStatus::Ok == SlabT::init(err_bt, alloc, cap, &slab), err_bt);

    SliceMut<u32> slab_keys;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->alloc<u32>(err_bt, cap, &slab_keys), err_bt);

    slab_keys.memset(0xaa);
    smoke_test_inner(err_bt, &slab, slab_keys, cap);
    slab.reset();
    slab_keys.memset(0xaa);
    smoke_test_inner(err_bt, &slab, slab_keys, cap);
    slab.reset();
    slab_keys.memset(0xaa);
    smoke_test_inner(err_bt, &slab, slab_keys, cap);
}

static void noalloc_test(Backtrace *err_bt) {
    Alloc noalloc = Alloc::init(SliceMut<u8>::init_dangling());
    SlabT slab;
    assert(SlabInitStatus::OutOfMemory ==
           SlabT::init(nullptr, &noalloc, 69, &slab));
    assert(SlabInitStatus::OutOfMemory ==
           SlabT::init(nullptr, &noalloc, 1, &slab));
    testing::assert_with_trace(
        SlabInitStatus::Ok == SlabT::init(err_bt, &noalloc, 0, &slab), err_bt);

    u32 idx = U32_MAX;
    val_type val = 100;
    assert(SlabInsertStatus::OutOfCapacity == slab.insert(nullptr, &val, &idx));

    val_type *mut_p = nullptr;
    assert(SlabAccessStatus::OutOfBounds == slab.get_mut(nullptr, 0, &mut_p));

    val_type const *immut_p = nullptr;
    assert(SlabAccessStatus::OutOfBounds == slab.get(nullptr, 0, &immut_p));
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    constexpr u64 MEMORY_SIZE = 1 << 20;

    u8 *mem = nullptr;
    testing::assert_with_trace(
        page_alloc::alloc_4kb(&err_bt, MEMORY_SIZE, &mem) ==
            page_alloc::AllocStatus::Ok,
        &err_bt);

    noalloc_test(&err_bt);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, MEMORY_SIZE));

    constexpr u32 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                               65, 63, 64,   128,  127, 129,  3169};
    constexpr u32 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

    for (u32 i = 0; i < n_lengths; i++) {
        u32 const len = lengths[i];
        alloc.reset();
        smoke_test(&err_bt, &alloc, len);
    }

    testing::assert_with_trace(page_alloc::free(&err_bt, mem, MEMORY_SIZE) ==
                                   page_alloc::FreeStatus::Ok,
                               &err_bt);
    return 0;
}
