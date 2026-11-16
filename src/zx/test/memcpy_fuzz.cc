import zx;

using namespace zx;

struct Ctx {
    SliceMut<u8> mem;
};

__attribute__((no_builtin)) static bool is_eq(Slice<u8> left, Slice<u8> right) {
    if (left.len() != right.len()) {
        return false;
    }

    for (u64 i = 0; i < left.len(); i++) {
        if (left.read_unsafe(i) != right.read_unsafe(i)) {
            return false;
        }
    }

    return true;
}

__attribute__((no_builtin)) static void fuzz_one(void *usr_ctx,
                                                 FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    SliceMut<u8> mem = ((Ctx const *)usr_ctx)->mem;
    assert(mem.len() > 0);

    u64 const len = max_u64(
        1, mod_unsafe_u64(fuzz_input->next(), div_unsafe_u64(mem.len(), 2)));

    Xoshiro256 rand = Xoshiro256::init(fuzz_input->next());
    rand.fill(mem);

    SliceMut<u8> const left = mem.slice_unsafe(0, len);
    SliceMut<u8> const right =
        mem.slice_at_unsafe(sub_unsafe_u64(mem.len(), len));

    assert(left.len() == len);
    assert(right.len() == len);
    assert(!left.as_const().is_overlapping(right.as_const()));

    while (is_eq(left.as_const(), right.as_const())) {
        rand.fill(mem);
    }
    assert(!is_eq(left.as_const(), right.as_const()));
    memcpy(left.ptr(), right.ptr(), len);
    assert(is_eq(left.as_const(), right.as_const()));
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
        .mem = make_guarded_alloc(&err_bt, 4096),
    };
    constexpr u64 n_iter = u64{1} << 12;
    fuzz(FuzzParams{.alloc = &alloc,
                    .usr_ctx = (void *)&ctx,
                    .fuzz_one = fuzz_one,
                    .iter_limit = n_iter});
    return 0;
}
