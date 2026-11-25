import zx;

using namespace zx;

struct Ctx {
    SliceMut<u8> mem;
};

static void verify_pattern(SliceMut<u8> slice, u8 value) {
    for (u64 i = 0; i < slice.len(); i++) {
        assert(slice.read_unsafe(i) == value);
    }
}

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    assert(fuzz_input != nullptr);

    Ctx const *ctx = (Ctx const *)usr_ctx;
    DEAlloc alloc = DEAlloc::init(ctx->mem);

    constexpr u64 allocs_cap = 512;
    SliceMut<u8> left_allocs[allocs_cap];
    SliceMut<u8> right_allocs[allocs_cap];
    __builtin_memset(left_allocs, 0, sizeof(left_allocs));
    __builtin_memset(right_allocs, 0, sizeof(right_allocs));

    u64 const n_allocs = rand_mulshift_mod_u64(fuzz_input->next(), allocs_cap);

    u64 num_left = 0;
    u64 num_right = 0;
    for (u64 i = 0; i < n_allocs; i++) {
        u64 const len = rand_mulshift_mod_u64(
            fuzz_input->next(), add_unsafe_u64(alloc.capacity_left(), 1));
        u8 const align_kind =
            u64_to_u8_unsafe(rand_mulshift_mod_u64(fuzz_input->next(), 8));
        u16 const align = shl_unsafe_u16(1, align_kind);
        bool const from_left = mod_unsafe_u64(fuzz_input->next(), 2) == 0;
        SliceMut<u8> slice;
        AllocStatus const status =
            from_left
                ? alloc.left_aligned_alloc<u8>(nullptr, align, len, &slice)
                : alloc.right_aligned_alloc<u8>(nullptr, align, len, &slice);
        if (AllocStatus::Ok == status) {
            assert(__builtin_is_aligned(slice.ptr(), align));
            if (from_left) {
                left_allocs[num_left] = slice;
                num_left = add_unsafe_u64(num_left, 1);
            } else {
                right_allocs[num_right] = slice;
                num_right = add_unsafe_u64(num_right, 1);
            }
        }
    }

    // make sure allocations from the same end and across the two ends don't
    // overlap
    for (u64 i = 0; i < num_left; i++) {
        for (u64 j = add_unsafe_u64(i, 1); j < num_left; j++) {
            assert(!left_allocs[i].as_const().is_overlapping(
                left_allocs[j].as_const()));
        }
        for (u64 j = 0; j < num_right; j++) {
            assert(!left_allocs[i].as_const().is_overlapping(
                right_allocs[j].as_const()));
        }
    }
    for (u64 i = 0; i < num_right; i++) {
        for (u64 j = add_unsafe_u64(i, 1); j < num_right; j++) {
            assert(!right_allocs[i].as_const().is_overlapping(
                right_allocs[j].as_const()));
        }
    }

    for (u64 i = 0; i < num_left; i++) {
        left_allocs[i].memset((u8)(i + 1));
    }
    for (u64 i = 0; i < num_right; i++) {
        right_allocs[i].memset((u8)(i + 1));
    }

    // resetting the right end must not invalidate left side allocations
    alloc.right_reset();
    assert(alloc.right_offset() == ctx->mem.len());
    assert(alloc.capacity_left() ==
           sub_unsafe_u64(ctx->mem.len(), alloc.left_offset()));
    for (u64 i = 0; i < num_left; i++) {
        verify_pattern(left_allocs[i], (u8)(i + 1));
    }

    // allocate from the right after the reset
    u64 const new_right_len = rand_mulshift_mod_u64(
        fuzz_input->next(), add_unsafe_u64(alloc.capacity_left(), 1));
    SliceMut<u8> new_right;
    assert(AllocStatus::Ok ==
           alloc.right_alloc<u8>(nullptr, new_right_len, &new_right));
    new_right.memset(0x77);

    // resetting the left end must not invalidate right side allocations
    alloc.left_reset();
    assert(alloc.left_offset() == 0);
    assert(alloc.capacity_left() == alloc.right_offset());
    verify_pattern(new_right, 0x77);

    alloc.full_reset();
    assert(alloc.left_offset() == 0);
    assert(alloc.right_offset() == ctx->mem.len());
    assert(alloc.capacity_left() == ctx->mem.len());
}

Alloc make_alloc(Backtrace *err_bt, u64 size) {
    u8 *mem = nullptr;
    testing::assert_with_trace(page_alloc::AllocStatus::Ok ==
                                   page_alloc::alloc_4kb(err_bt, size, &mem),
                               err_bt);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, size));

    return alloc;
}

static SliceMut<u8> make_guarded_alloc(Backtrace *err_bt, u64 capacity) {
    assert(is_pow_two_u64(capacity));
    assert(capacity >= 4096);

    u64 const mem_size = mul_unsafe_u64(capacity, 2);
    u8 *mem = nullptr;
    page_alloc::AllocStatus const alloc_status =
        page_alloc::alloc_4kb(err_bt, mem_size, &mem);
    testing::assert_with_trace(alloc_status == page_alloc::AllocStatus::Ok,
                               err_bt);

    sys::MProtectStatus sys_status =
        sys::mprotect(err_bt, &mem[capacity], capacity, sys::PROT_NONE);
    testing::assert_with_trace(sys_status == sys::MProtectStatus::Ok, err_bt);

    SliceMut<u8> ret = SliceMut<u8>::init_unsafe(mem, capacity);
    ret.memset(0);

    return ret;
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    Alloc alloc = make_alloc(&err_bt, FUZZ_MEM_CAP);
    Ctx const ctx = Ctx{
        .mem = make_guarded_alloc(&err_bt, 8192),
    };
    fuzz(FuzzParams{
        .alloc = &alloc,
        .usr_ctx = (void *)&ctx,
        .fuzz_one = fuzz_one,
    });
    return 0;
}
