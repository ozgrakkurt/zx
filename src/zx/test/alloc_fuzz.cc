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

static SliceMut<u8> make_alloc(u64 capacity) {
    assert(is_pow_two_u64(capacity));
    assert(capacity >= 4096);

    SliceMut<u8> alloc;
    PageAllocStatus alloc_status =
        page_alloc_4kb(mul_unsafe_u64(capacity, 2), &alloc);
    assert(alloc_status == PageAllocStatus::Ok);
    SysStatus sys_status =
        sys_mprotect(alloc.slice_at_unsafe(capacity), SYS_PROT_NONE);
    assert(sys_status == SysStatus::Ok);

    SliceMut<u8> ret = alloc.slice_unsafe(0, capacity);
    ret.memset(0);

    return ret;
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    constexpr u64 fuzz_mem_cap = u64{1} << 27;
    SliceMut<u8> fuzz_mem;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok ==
            page_alloc_4kb(&err_bt, fuzz_mem_cap, &fuzz_mem),
        &err_bt);
    Alloc alloc = Alloc::init(fuzz_mem);
    Ctx const ctx = Ctx{
        .mem = make_alloc(8192),
    };
    constexpr u64 n_iter = u64{1} << 15;
    fuzz(&alloc, (void *)&ctx, fuzz_one, n_iter);
    return 0;
}
