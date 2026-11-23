import zx;

using namespace zx;

struct Ctx {
    Backtrace *err_bt;
    Alloc *alloc;
};

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    Ctx ctx = *(Ctx *)usr_ctx;
    Alloc *alloc = ctx.alloc;
    Backtrace *err_bt = ctx.err_bt;

    alloc->reset();

    constexpr u32 MAX_CAPACITY = 128;
    u32 const capacity = max_u32(
        u64_to_u32_unsafe(mod_unsafe_u64(fuzz_input->next(), MAX_CAPACITY)), 1);

    u32 const max_n_ops = mul_unsafe_u32(MAX_CAPACITY, 4);
    u32 const n_ops =
        u64_to_u32_unsafe(mod_unsafe_u64(fuzz_input->next(), max_n_ops));

    Slab<u64> slab;
    testing::assert_with_trace(
        SlabInitStatus::Ok == Slab<u64>::init(err_bt, alloc, capacity, &slab),
        err_bt);

    SliceMut<u32> slab_keys;
    testing::assert_with_trace(
        AllocStatus::Ok == alloc->alloc<u32>(err_bt, capacity, &slab_keys),
        err_bt);
    slab_keys.memset(0xff);
    u32 n_slab_keys = 0;

    for (u32 i = 0; i < n_ops; i++) {
        assert(n_slab_keys <= capacity);

        u8 const op = u64_to_u8_unsafe(mod_unsafe_u64(fuzz_input->next(), 3));
        bool const rand_key = n_slab_keys == 0 || (fuzz_input->next() & 1) == 1;
        u32 const key_idx =
            rand_mulshift_mod_u32((u32)fuzz_input->next(), n_slab_keys);
        u32 const key = (rand_key) ? (u32)fuzz_input->next()
                                   : slab_keys.read_unsafe(key_idx);
        switch (op) {
        // insert
        case 0: {
            u64 const to_insert = U64_MAX;
            u32 new_key = U32_MAX;
            bool const should_succeed = slab.capacity_left() > 0;
            SlabInsertStatus const status =
                slab.insert(err_bt, &to_insert, &new_key);
            if (should_succeed) {
                testing::assert_with_trace(SlabInsertStatus::Ok == status,
                                           err_bt);
                u64 *pp = nullptr;
                testing::assert_with_trace(
                    SlabAccessStatus::Found ==
                        slab.get_mut(err_bt, new_key, &pp),
                    err_bt);
                assert(pp != nullptr);
                assert(*pp == U64_MAX);
                *pp = new_key;
                slab_keys.write_unsafe(n_slab_keys, new_key);
                n_slab_keys = add_unsafe_u32(n_slab_keys, 1);
            } else {
                assert(SlabInsertStatus::OutOfCapacity == status);
            }
            break;
        }
        // remove
        case 1: {
            u64 val = U64_MAX;
            SlabAccessStatus const status = slab.remove(err_bt, key, &val);
            if (!rand_key) {
                testing::assert_with_trace(SlabAccessStatus::Found == status,
                                           err_bt);
                assert(val == key);
                n_slab_keys = sub_unsafe_u32(n_slab_keys, 1);
                slab_keys.write_unsafe(key_idx,
                                       slab_keys.read_unsafe(n_slab_keys));
                slab_keys.write_unsafe(n_slab_keys, U32_MAX);
            } else if (SlabAccessStatus::Found == status) {
                assert(val == key);
                bool found = false;
                for (u32 j = 0; j < n_slab_keys; j++) {
                    if (slab_keys.read_unsafe(j) == key) {
                        n_slab_keys = sub_unsafe_u32(n_slab_keys, 1);
                        slab_keys.write_unsafe(
                            j, slab_keys.read_unsafe(n_slab_keys));
                        slab_keys.write_unsafe(n_slab_keys, U32_MAX);
                        found = true;
                        break;
                    }
                }
                assert(found);
            }
            break;
        }
        // get
        case 2: {
            u64 const *val_p = nullptr;
            SlabAccessStatus const status = slab.get(err_bt, key, &val_p);
            if (!rand_key) {
                testing::assert_with_trace(SlabAccessStatus::Found == status,
                                           err_bt);
                assert(val_p != nullptr);
                assert(*val_p == key);
            }
            break;
        }
        default: {
            panic();
        }
        }
        for (u32 j = 0; j < n_slab_keys; j++) {
            u64 const *val_p = nullptr;
            u32 const keyk = slab_keys.read_unsafe(j);
            switch (slab.get(nullptr, keyk, &val_p)) {
            case SlabAccessStatus::Found: {
                break;
            }
            case SlabAccessStatus::NotFound: {
                if (op == 0) {
                    panic();
                } else if (op == 1) {
                    panic();

                } else if (op == 2) {
                    panic();
                } else {
                    panic();
                }
                break;
            }
            case SlabAccessStatus::OutOfBounds: {
                panic();
                break;
            }
            default: {
                panic();
                break;
            }
            }
            assert(*val_p == keyk);
        }
    }

    for (u32 i = 0; i < n_slab_keys; i++) {
        u64 val = U64_MAX;
        u32 const key = slab_keys.read_unsafe(i);
        testing::assert_with_trace(
            SlabAccessStatus::Found == slab.remove(err_bt, key, &val), err_bt);
        assert(val == key);
    }

    assert(slab.n_occupied() == 0);
}

Alloc make_alloc(Backtrace *err_bt, u64 size) {
    u8 *mem = nullptr;
    testing::assert_with_trace(page_alloc::AllocStatus::Ok ==
                                   page_alloc::alloc_4kb(err_bt, size, &mem),
                               err_bt);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, size));

    return alloc;
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    constexpr u64 MEM_CAP = u64{1} << 18;
    Alloc alloc = make_alloc(&err_bt, MEM_CAP);

    Alloc fuzz_alloc = make_alloc(&err_bt, FUZZ_MEM_CAP);

    Ctx ctx = Ctx{
        .alloc = &alloc,
        .err_bt = &err_bt,
    };
    fuzz(FuzzParams{
        .alloc = &fuzz_alloc,
        .usr_ctx = (void *)&ctx,
        .fuzz_one = fuzz_one,
    });
    return 0;
}
