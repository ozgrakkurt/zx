import zx;

using namespace zx;

struct Ctx {
    SliceMut<u8> mem;
};

__attribute__((no_builtin)) static void fuzz_one(void *usr_ctx,
                                                 FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    SliceMut<u8> mem = ((Ctx const *)usr_ctx)->mem;
    assert(mem.len() > 0);

    bool const left_align = fuzz_input->next() & u64{1};
    u64 const len = max_u64(1, mod_unsafe_u64(fuzz_input->next(), mem.len()));
    assert(len > 0);

    u64 const first = mod_unsafe_u64(fuzz_input->next(), len);
    u64 const second = mod_unsafe_u64(fuzz_input->next(), len);
    u64 const third = mod_unsafe_u64(fuzz_input->next(), len);

    SliceMut<u8> const str =
        left_align ? mem.slice_unsafe(0, len)
                   : mem.slice_at_unsafe(sub_unsafe_u64(mem.len(), len));
    assert(str.len() == len);
    assert(str.len() > 0);

    str.fill(69);
    str.write_unsafe(first, 0);
    str.write_unsafe(second, 0);
    str.write_unsafe(third, 0);

    u64 const slen = strlen((char const *)str.ptr());

    u64 const expected = min_u64(third, min_u64(first, second));
    assert(slen == expected);
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
