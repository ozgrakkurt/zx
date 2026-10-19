import zx;

using namespace zx;

struct Ctx {
    SliceMut<u8> mem;
};

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    assert(fuzz_input != nullptr);

    Ctx const *ctx = (Ctx const *)usr_ctx;
    Alloc alloc = Alloc::init(ctx->mem);

    constexpr u64 allocs_cap = 1024;
    SliceMut<u8> allocs[allocs_cap];
    __builtin_memset(allocs, 0, sizeof(allocs));

    u64 const n_allocs = rand_mod_u64(fuzz_input->next(), allocs_cap);

    u64 num_allocs = 0;
    for (u64 i = 0; i < n_allocs; i++) {
        u64 const len = rand_mod_u64(fuzz_input->next(),
                                     add_unsafe_u64(alloc.capacity_left(), 1));
        u8 const align_kind =
            u64_to_u8_unsafe(rand_mod_u64(fuzz_input->next(), 8));
        u16 const align = shl_unsafe_u16(1, align_kind);
        SliceMut<u8> slice;
        AllocStatus const status =
            alloc.aligned_alloc<u8>(nullptr, align, len, &slice);
        if (AllocStatus::Ok == status) {
            u16 const expected_align = shl_unsafe_u16(1, align_kind);
            assert(__builtin_is_aligned(slice.ptr(), expected_align));
            allocs[num_allocs] = slice;
            num_allocs = add_unsafe_u64(num_allocs, 1);
        }
    }

    // make sure no overlap
    for (u64 i = 0; i < num_allocs; i++) {
        for (u64 j = 0; j < i; j++) {
            assert(i != j);
            assert(!allocs[i].as_const().is_overlapping(allocs[j].as_const()));
        }
        for (u64 j = i + 1; j < num_allocs; j++) {
            assert(i != j);
            assert(!allocs[i].as_const().is_overlapping(allocs[j].as_const()));
        }
    }
}

static SliceMut<u8> make_alloc(Backtrace *err_bt, u64 capacity) {
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

    constexpr u64 fuzz_mem_cap = u64{1} << 27;
    u8 *fuzz_mem = nullptr;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc::alloc_4kb(&err_bt, fuzz_mem_cap, &fuzz_mem),
        &err_bt);
    Alloc alloc =
        Alloc::init(SliceMut<u8>::init_unsafe(fuzz_mem, fuzz_mem_cap));
    Ctx const ctx = Ctx{
        .mem = make_alloc(&err_bt, 8192),
    };
    constexpr u64 n_iter = u64{1} << 15;
    fuzz(FuzzParams{.alloc = &alloc,
                    .usr_ctx = (void *)&ctx,
                    .fuzz_one = fuzz_one,
                    .iter_limit = n_iter});
    return 0;
}
