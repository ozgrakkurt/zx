import zx;

using namespace zx;

struct Ctx {
    SliceMut<u8> mem;
};

// __attribute__((no_builtin)) static inline i32_c simple_memcmp(void const*
// lhs,
//                                                              void const* rhs,
//                                                              u64_c count)
//                                                              {
//   if (count == 0) {
//     return 0;
//   }
//
//   u8 const* left = (u8 const*)lhs;
//   u8 const* right = (u8 const*)rhs;
//
//   for (u64 i=0; i<count; i++) {
//     if (left[i] != right[i]) {
//       return sub_unsafe_i32(i32{left[i]}, i32{right[i]});
//     }
//   }
//
//   return 0;
// }

__attribute__((no_builtin)) static void fuzz_one(void *usr_ctx,
                                                 FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    SliceMut<u8> mem = ((Ctx const *)usr_ctx)->mem;
    assert(mem.len() > 0);

    u64 const len = max_u64(
        1, mod_unsafe_u64(fuzz_input->next(), div_unsafe_u64(mem.len(), 2)));
    assert(len > 0);

    mem.memset(0);

    SliceMut<u8> const left = mem.slice_unsafe(0, len);
    SliceMut<u8> const right =
        mem.slice_at_unsafe(sub_unsafe_u64(mem.len(), len));

    u64 const idx = mod_unsafe_u64(fuzz_input->next(), len);

    u8 const left_val = (u8)fuzz_input->next();
    u8 const right_val = wrapping_add_u8(left_val, (u8)fuzz_input->next());

    left.write_unsafe(idx, left_val);
    right.write_unsafe(idx, right_val);

    Xoshiro256 rand = Xoshiro256::init(fuzz_input->next());
    if (left_val != right_val) {
        rand.fill(left.slice_at_unsafe(add_unsafe_u64(idx, 1)));
        rand.fill(right.slice_at_unsafe(add_unsafe_u64(idx, 1)));
    }

    assert(left.len() == len);
    assert(right.len() == len);
    assert(!left.as_const().is_overlapping(right.as_const()));
    i32 const res = memcmp(left.ptr(), right.ptr(), len);
    if (left_val == right_val) {
        assert(res == 0);
    } else if (left_val > right_val) {
        assert(res > 0);
    } else {
        assert(res < 0);
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
        .mem = make_alloc(&err_bt, 4096),
    };
    constexpr u64 n_iter = u64{1} << 12;
    fuzz(FuzzParams{.alloc = &alloc,
                    .usr_ctx = (void *)&ctx,
                    .fuzz_one = fuzz_one,
                    .iter_limit = n_iter});
    return 0;
}
