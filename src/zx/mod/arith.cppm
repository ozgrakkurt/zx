export module zx:arith;
import :def;
import :assert;

export namespace zx {

u8 wrapping_add_u8(u8 a, u8 b) { return a + b; }

u16 wrapping_add_u16(u16 a, u16 b) { return a + b; }

u32 wrapping_add_u32(u32 a, u32 b) { return a + b; }

u64 wrapping_add_u64(u64 a, u64 b) { return a + b; }

i8 wrapping_add_i8(i8 a, i8 b) { return a + b; }

i16 wrapping_add_i16(i16 a, i16 b) { return a + b; }

i32 wrapping_add_i32(i32 a, i32 b) { return a + b; }

i64 wrapping_add_i64(i64 a, i64 b) { return a + b; }

u8 saturating_add_u8(u8 a, u8 b) { return __builtin_elementwise_add_sat(a, b); }

u16 saturating_add_u16(u16 a, u16 b) {
    return __builtin_elementwise_add_sat(a, b);
}

u32 saturating_add_u32(u32 a, u32 b) {
    return __builtin_elementwise_add_sat(a, b);
}

u64 saturating_add_u64(u64 a, u64 b) {
    return __builtin_elementwise_add_sat(a, b);
}

i8 saturating_add_i8(i8 a, i8 b) { return __builtin_elementwise_add_sat(a, b); }

i16 saturating_add_i16(i16 a, i16 b) {
    return __builtin_elementwise_add_sat(a, b);
}

i32 saturating_add_i32(i32 a, i32 b) {
    return __builtin_elementwise_add_sat(a, b);
}

i64 saturating_add_i64(i64 a, i64 b) {
    return __builtin_elementwise_add_sat(a, b);
}

u8 add_u8(u8 a, u8 b, bool *overflow) {
    u8 res;
    *overflow |= __builtin_add_overflow(a, b, &res);
    return res;
}

u16 add_u16(u16 a, u16 b, bool *overflow) {
    u16 res;
    *overflow |= __builtin_add_overflow(a, b, &res);
    return res;
}

u32 add_u32(u32 a, u32 b, bool *overflow) {
    u32 res;
    *overflow |= __builtin_add_overflow(a, b, &res);
    return res;
}

u64 add_u64(u64 a, u64 b, bool *overflow) {
    u64 res;
    *overflow |= __builtin_add_overflow(a, b, &res);
    return res;
}

i8 add_i8(i8 a, i8 b, bool *overflow) {
    i8 res;
    *overflow |= __builtin_add_overflow(a, b, &res);
    return res;
}

i16 add_i16(i16 a, i16 b, bool *overflow) {
    i16 res;
    *overflow |= __builtin_add_overflow(a, b, &res);
    return res;
}

i32 add_i32(i32 a, i32 b, bool *overflow) {
    i32 res;
    *overflow |= __builtin_add_overflow(a, b, &res);
    return res;
}

i64 add_i64(i64 a, i64 b, bool *overflow) {
    i64 res;
    *overflow |= __builtin_add_overflow(a, b, &res);
    return res;
}

u8 add_unsafe_u8(u8 a, u8 b) {
    u8 res;
    assert_dev(!__builtin_add_overflow(a, b, &res));
    return res;
}

u16 add_unsafe_u16(u16 a, u16 b) {
    u16 res;
    assert_dev(!__builtin_add_overflow(a, b, &res));
    return res;
}

u32 add_unsafe_u32(u32 a, u32 b) {
    u32 res;
    assert_dev(!__builtin_add_overflow(a, b, &res));
    return res;
}

u64 add_unsafe_u64(u64 a, u64 b) {
    u64 res;
    assert_dev(!__builtin_add_overflow(a, b, &res));
    return res;
}

i8 add_unsafe_i8(i8 a, i8 b) {
    i8 res;
    assert_dev(!__builtin_add_overflow(a, b, &res));
    return res;
}

i16 add_unsafe_i16(i16 a, i16 b) {
    i16 res;
    assert_dev(!__builtin_add_overflow(a, b, &res));
    return res;
}

i32 add_unsafe_i32(i32 a, i32 b) {
    i32 res;
    assert_dev(!__builtin_add_overflow(a, b, &res));
    return res;
}

i64 add_unsafe_i64(i64 a, i64 b) {
    i64 res;
    assert_dev(!__builtin_add_overflow(a, b, &res));
    return res;
}

u8 wrapping_sub_u8(u8 a, u8 b) { return a - b; }

u16 wrapping_sub_u16(u16 a, u16 b) { return a - b; }

u32 wrapping_sub_u32(u32 a, u32 b) { return a - b; }

u64 wrapping_sub_u64(u64 a, u64 b) { return a - b; }

i8 wrapping_sub_i8(i8 a, i8 b) { return a - b; }

i16 wrapping_sub_i16(i16 a, i16 b) { return a - b; }

i32 wrapping_sub_i32(i32 a, i32 b) { return a - b; }

i64 wrapping_sub_i64(i64 a, i64 b) { return a - b; }

u8 saturating_sub_u8(u8 a, u8 b) { return __builtin_elementwise_sub_sat(a, b); }

u16 saturating_sub_u16(u16 a, u16 b) {
    return __builtin_elementwise_sub_sat(a, b);
}

u32 saturating_sub_u32(u32 a, u32 b) {
    return __builtin_elementwise_sub_sat(a, b);
}

u64 saturating_sub_u64(u64 a, u64 b) {
    return __builtin_elementwise_sub_sat(a, b);
}

i8 saturating_sub_i8(i8 a, i8 b) { return __builtin_elementwise_sub_sat(a, b); }

i16 saturating_sub_i16(i16 a, i16 b) {
    return __builtin_elementwise_sub_sat(a, b);
}

i32 saturating_sub_i32(i32 a, i32 b) {
    return __builtin_elementwise_sub_sat(a, b);
}

i64 saturating_sub_i64(i64 a, i64 b) {
    return __builtin_elementwise_sub_sat(a, b);
}

u8 sub_u8(u8 a, u8 b, bool *overflow) {
    u8 res;
    *overflow |= __builtin_sub_overflow(a, b, &res);
    return res;
}

u16 sub_u16(u16 a, u16 b, bool *overflow) {
    u16 res;
    *overflow |= __builtin_sub_overflow(a, b, &res);
    return res;
}

u32 sub_u32(u32 a, u32 b, bool *overflow) {
    u32 res;
    *overflow |= __builtin_sub_overflow(a, b, &res);
    return res;
}

u64 sub_u64(u64 a, u64 b, bool *overflow) {
    u64 res;
    *overflow |= __builtin_sub_overflow(a, b, &res);
    return res;
}

i8 sub_i8(i8 a, i8 b, bool *overflow) {
    i8 res;
    *overflow |= __builtin_sub_overflow(a, b, &res);
    return res;
}

i16 sub_i16(i16 a, i16 b, bool *overflow) {
    i16 res;
    *overflow |= __builtin_sub_overflow(a, b, &res);
    return res;
}

i32 sub_i32(i32 a, i32 b, bool *overflow) {
    i32 res;
    *overflow |= __builtin_sub_overflow(a, b, &res);
    return res;
}

i64 sub_i64(i64 a, i64 b, bool *overflow) {
    i64 res;
    *overflow |= __builtin_sub_overflow(a, b, &res);
    return res;
}

u8 sub_unsafe_u8(u8 a, u8 b) {
    u8 res;
    assert_dev(!__builtin_sub_overflow(a, b, &res));
    return res;
}

u16 sub_unsafe_u16(u16 a, u16 b) {
    u16 res;
    assert_dev(!__builtin_sub_overflow(a, b, &res));
    return res;
}

u32 sub_unsafe_u32(u32 a, u32 b) {
    u32 res;
    assert_dev(!__builtin_sub_overflow(a, b, &res));
    return res;
}

u64 sub_unsafe_u64(u64 a, u64 b) {
    u64 res;
    assert_dev(!__builtin_sub_overflow(a, b, &res));
    return res;
}

i8 sub_unsafe_i8(i8 a, i8 b) {
    i8 res;
    assert_dev(!__builtin_sub_overflow(a, b, &res));
    return res;
}

i16 sub_unsafe_i16(i16 a, i16 b) {
    i16 res;
    assert_dev(!__builtin_sub_overflow(a, b, &res));
    return res;
}

i32 sub_unsafe_i32(i32 a, i32 b) {
    i32 res;
    assert_dev(!__builtin_sub_overflow(a, b, &res));
    return res;
}

i64 sub_unsafe_i64(i64 a, i64 b) {
    i64 res;
    assert_dev(!__builtin_sub_overflow(a, b, &res));
    return res;
}

u8 wrapping_mul_u8(u8 a, u8 b) { return a * b; }

u16 wrapping_mul_u16(u16 a, u16 b) { return a * b; }

u32 wrapping_mul_u32(u32 a, u32 b) { return a * b; }

u64 wrapping_mul_u64(u64 a, u64 b) { return a * b; }

u8 saturating_mul_u8(u8 a, u8 b) {
    u8 res;
    if (__builtin_mul_overflow(a, b, &res)) {
        return U8_MAX;
    } else {
        return res;
    }
}

u16 saturating_mul_u16(u16 a, u16 b) {
    u16 res;
    if (__builtin_mul_overflow(a, b, &res)) {
        return U16_MAX;
    } else {
        return res;
    }
}

u32 saturating_mul_u32(u32 a, u32 b) {
    u32 res;
    if (__builtin_mul_overflow(a, b, &res)) {
        return U32_MAX;
    } else {
        return res;
    }
}

u64 saturating_mul_u64(u64 a, u64 b) {
    u64 res;
    if (__builtin_mul_overflow(a, b, &res)) {
        return U64_MAX;
    } else {
        return res;
    }
}

u8 mul_u8(u8 a, u8 b, bool *overflow) {
    u8 res;
    *overflow |= __builtin_mul_overflow(a, b, &res);
    return res;
}

u16 mul_u16(u16 a, u16 b, bool *overflow) {
    u16 res;
    *overflow |= __builtin_mul_overflow(a, b, &res);
    return res;
}

u32 mul_u32(u32 a, u32 b, bool *overflow) {
    u32 res;
    *overflow |= __builtin_mul_overflow(a, b, &res);
    return res;
}

u64 mul_u64(u64 a, u64 b, bool *overflow) {
    u64 res;
    *overflow |= __builtin_mul_overflow(a, b, &res);
    return res;
}

i8 mul_i8(i8 a, i8 b, bool *overflow) {
    i8 res;
    *overflow |= __builtin_mul_overflow(a, b, &res);
    return res;
}

i16 mul_i16(i16 a, i16 b, bool *overflow) {
    i16 res;
    *overflow |= __builtin_mul_overflow(a, b, &res);
    return res;
}

i32 mul_i32(i32 a, i32 b, bool *overflow) {
    i32 res;
    *overflow |= __builtin_mul_overflow(a, b, &res);
    return res;
}

i64 mul_i64(i64 a, i64 b, bool *overflow) {
    i64 res;
    *overflow |= __builtin_mul_overflow(a, b, &res);
    return res;
}

u8 mul_unsafe_u8(u8 a, u8 b) {
    u8 res;
    assert_dev(!__builtin_mul_overflow(a, b, &res));
    return res;
}

u16 mul_unsafe_u16(u16 a, u16 b) {
    u16 res;
    assert_dev(!__builtin_mul_overflow(a, b, &res));
    return res;
}

u32 mul_unsafe_u32(u32 a, u32 b) {
    u32 res;
    assert_dev(!__builtin_mul_overflow(a, b, &res));
    return res;
}

u64 mul_unsafe_u64(u64 a, u64 b) {
    u64 res;
    assert_dev(!__builtin_mul_overflow(a, b, &res));
    return res;
}

i8 mul_unsafe_i8(i8 a, i8 b) {
    i8 res;
    assert_dev(!__builtin_mul_overflow(a, b, &res));
    return res;
}

i16 mul_unsafe_i16(i16 a, i16 b) {
    i16 res;
    assert_dev(!__builtin_mul_overflow(a, b, &res));
    return res;
}

i32 mul_unsafe_i32(i32 a, i32 b) {
    i32 res;
    assert_dev(!__builtin_mul_overflow(a, b, &res));
    return res;
}

i64 mul_unsafe_i64(i64 a, i64 b) {
    i64 res;
    assert_dev(!__builtin_mul_overflow(a, b, &res));
    return res;
}

u8 div_u8(u8 a, u8 b, bool *div_by_zero) {
    bool const dbz = b == 0;
    *div_by_zero |= dbz;
    u8 const res = dbz ? 0 : (a / b);
    return res;
}

u16 div_u16(u16 a, u16 b, bool *div_by_zero) {
    bool const dbz = b == 0;
    *div_by_zero |= dbz;
    u16 const res = dbz ? 0 : (a / b);
    return res;
}

u32 div_u32(u32 a, u32 b, bool *div_by_zero) {
    bool const dbz = b == 0;
    *div_by_zero |= dbz;
    u32 const res = dbz ? 0 : (a / b);
    return res;
}

u64 div_u64(u64 a, u64 b, bool *div_by_zero) {
    bool const dbz = b == 0;
    *div_by_zero |= dbz;
    u64 const res = dbz ? 0 : (a / b);
    return res;
}

u8 div_unsafe_u8(u8 a, u8 b) {
    assert_dev(b != 0);
    return a / b;
}

u16 div_unsafe_u16(u16 a, u16 b) {
    assert_dev(b != 0);
    return a / b;
}

u32 div_unsafe_u32(u32 a, u32 b) {
    assert_dev(b != 0);
    return a / b;
}

u64 div_unsafe_u64(u64 a, u64 b) {
    assert_dev(b != 0);
    return a / b;
}

u8 mod_u8(u8 a, u8 b, bool *div_by_zero) {
    bool const dbz = b == 0;
    *div_by_zero |= dbz;
    u8 const res = dbz ? 0 : (a % b);
    return res;
}

u16 mod_u16(u16 a, u16 b, bool *div_by_zero) {
    bool const dbz = b == 0;
    *div_by_zero |= dbz;
    u16 const res = dbz ? 0 : (a % b);
    return res;
}

u32 mod_u32(u32 a, u32 b, bool *div_by_zero) {
    bool const dbz = b == 0;
    *div_by_zero |= dbz;
    u32 const res = dbz ? 0 : (a % b);
    return res;
}

u64 mod_u64(u64 a, u64 b, bool *div_by_zero) {
    bool const dbz = b == 0;
    *div_by_zero |= dbz;
    u64 const res = dbz ? 0 : (a % b);
    return res;
}

u8 mod_unsafe_u8(u8 a, u8 b) {
    assert_dev(b != 0);
    return a % b;
}

u16 mod_unsafe_u16(u16 a, u16 b) {
    assert_dev(b != 0);
    return a % b;
}

u32 mod_unsafe_u32(u32 a, u32 b) {
    assert_dev(b != 0);
    return a % b;
}

u64 mod_unsafe_u64(u64 a, u64 b) {
    assert_dev(b != 0);
    return a % b;
}

// This maps random number `a` to another random number which is `[0, b)`.
// The result is uniform only if the `a` is uniform.
//
// This function is intended to be used as a faster alternative to doing modulo
// on random numbers.
u32 rand_mulshift_mod_u32(u32 a, u32 b) {
    u64 const mul = u64{a} * u64{b};
    return (u32)(mul >> 32);
}

// This maps random number `a` to another random number which is `[0, b)`
// The result is uniform only if the `a` is uniform.
//
// This function is intended to be used as a faster alternative to doing modulo
// on random numbers.
u64 rand_mulshift_mod_u64(u64 a, u64 b) {
    u128 const mul = u128{a} * u128{b};
    return (u64)(mul >> 64);
}

u8 div_ceil_u8(u8 a, u8 b, bool *fail) {
    if (b == 0) [[unlikely]] {
        *fail = true;
        return 0;
    }
    u8 const q = a / b;
    u8 const r = a % b;
    return q + ((r != 0) ? u8{1} : u8{0});
}

u16 div_ceil_u16(u16 a, u16 b, bool *fail) {
    if (b == 0) [[unlikely]] {
        *fail = true;
        return 0;
    }
    u16 const q = a / b;
    u16 const r = a % b;
    return q + ((r != 0) ? u16{1} : u16{0});
}

u32 div_ceil_u32(u32 a, u32 b, bool *fail) {
    if (b == 0) [[unlikely]] {
        *fail = true;
        return 0;
    }
    u32 const q = a / b;
    u32 const r = a % b;
    return q + ((r != 0) ? u32{1} : u32{0});
}

u64 div_ceil_u64(u64 a, u64 b, bool *fail) {
    if (b == 0) [[unlikely]] {
        *fail = true;
        return 0;
    }
    u64 const q = a / b;
    u64 const r = a % b;
    return q + ((r != 0) ? u64{1} : u64{0});
}

u8 div_ceil_unsafe_u8(u8 a, u8 b) {
    assert_dev(b != 0);
    u8 const q = a / b;
    u8 const r = a % b;
    return q + ((r != 0) ? u8{1} : u8{0});
}

u16 div_ceil_unsafe_u16(u16 a, u16 b) {
    assert_dev(b != 0);
    u16 const q = a / b;
    u16 const r = a % b;
    return q + ((r != 0) ? u16{1} : u16{0});
}

u32 div_ceil_unsafe_u32(u32 a, u32 b) {
    assert_dev(b != 0);
    u32 const q = a / b;
    u32 const r = a % b;
    return q + ((r != 0) ? u32{1} : u32{0});
}

u64 div_ceil_unsafe_u64(u64 a, u64 b) {
    assert_dev(b != 0);
    u64 const q = a / b;
    u64 const r = a % b;
    return q + ((r != 0) ? u64{1} : u64{0});
}

u8 next_multiple_of_u8(u8 a, u8 b, bool *fail) {
    return mul_u8(div_ceil_u8(a, b, fail), b, fail);
}

u16 next_multiple_of_u16(u16 a, u16 b, bool *fail) {
    return mul_u16(div_ceil_u16(a, b, fail), b, fail);
}

u32 next_multiple_of_u32(u32 a, u32 b, bool *fail) {
    return mul_u32(div_ceil_u32(a, b, fail), b, fail);
}

u64 next_multiple_of_u64(u64 a, u64 b, bool *fail) {
    return mul_u64(div_ceil_u64(a, b, fail), b, fail);
}

u8 next_multiple_of_unsafe_u8(u8 a, u8 b) {
    return mul_unsafe_u8(div_ceil_unsafe_u8(a, b), b);
}

u16 next_multiple_of_unsafe_u16(u16 a, u16 b) {
    return mul_unsafe_u16(div_ceil_unsafe_u16(a, b), b);
}

u32 next_multiple_of_unsafe_u32(u32 a, u32 b) {
    return mul_unsafe_u32(div_ceil_unsafe_u32(a, b), b);
}

u64 next_multiple_of_unsafe_u64(u64 a, u64 b) {
    return mul_unsafe_u64(div_ceil_unsafe_u64(a, b), b);
}

u8 min_u8(u8 a, u8 b) { return (a > b) ? b : a; }

u16 min_u16(u16 a, u16 b) { return (a > b) ? b : a; }

u32 min_u32(u32 a, u32 b) { return (a > b) ? b : a; }

u64 min_u64(u64 a, u64 b) { return (a > b) ? b : a; }

i8 min_i8(i8 a, i8 b) { return (a > b) ? b : a; }

i16 min_i16(i16 a, i16 b) { return (a > b) ? b : a; }

i32 min_i32(i32 a, i32 b) { return (a > b) ? b : a; }

i64 min_i64(i64 a, i64 b) { return (a > b) ? b : a; }

u8 max_u8(u8 a, u8 b) { return (a < b) ? b : a; }

u16 max_u16(u16 a, u16 b) { return (a < b) ? b : a; }

u32 max_u32(u32 a, u32 b) { return (a < b) ? b : a; }

u64 max_u64(u64 a, u64 b) { return (a < b) ? b : a; }

i8 max_i8(i8 a, i8 b) { return (a < b) ? b : a; }

i16 max_i16(i16 a, i16 b) { return (a < b) ? b : a; }

i32 max_i32(i32 a, i32 b) { return (a < b) ? b : a; }

i64 max_i64(i64 a, i64 b) { return (a < b) ? b : a; }

u8 ctz_u8(u8 a) { return (u8)__builtin_ctzg(a, 8); }

u8 ctz_u16(u16 a) { return (u8)__builtin_ctzg(a, 16); }

u8 ctz_u32(u32 a) { return (u8)__builtin_ctzg(a, 32); }

u8 ctz_u64(u64 a) { return (u8)__builtin_ctzg(a, 64); }

u8 clz_u8(u8 a) { return (u8)__builtin_clzg(a, 8); }

u8 clz_u16(u16 a) { return (u8)__builtin_clzg(a, 16); }

u8 clz_u32(u32 a) { return (u8)__builtin_clzg(a, 32); }

u8 clz_u64(u64 a) { return (u8)__builtin_clzg(a, 64); }

u8 popcount_u8(u8 a) { return (u8)__builtin_popcountg(a); }

u8 popcount_u16(u16 a) { return (u8)__builtin_popcountg(a); }

u8 popcount_u32(u32 a) { return (u8)__builtin_popcountg(a); }

u8 popcount_u64(u64 a) { return (u8)__builtin_popcountg(a); }

u8 bit_width_u8(u8 a) { return u8{8} - (u8)__builtin_clzg(a, 8); }

u8 bit_width_u16(u16 a) { return u8{16} - (u8)__builtin_clzg(a, 16); }

u8 bit_width_u32(u32 a) { return u8{32} - (u8)__builtin_clzg(a, 32); }

u8 bit_width_u64(u64 a) { return u8{64} - (u8)__builtin_clzg(a, 64); }

u8 shl_u8(u8 a, u64 amount, bool *fail) {
    bool const oob = amount >= 8;
    *fail |= oob;
    u8 const res = oob ? 0 : (a << amount);
    return res;
}

u16 shl_u16(u16 a, u64 amount, bool *fail) {
    bool const oob = amount >= 16;
    *fail |= oob;
    u16 const res = oob ? 0 : (a << amount);
    return res;
}

u32 shl_u32(u32 a, u64 amount, bool *fail) {
    bool const oob = amount >= 32;
    *fail |= oob;
    u32 const res = oob ? 0 : (a << amount);
    return res;
}

u64 shl_u64(u64 a, u64 amount, bool *fail) {
    bool const oob = amount >= 64;
    *fail |= oob;
    u64 const res = oob ? 0 : (a << amount);
    return res;
}

u8 shl_unsafe_u8(u8 a, u64 amount) {
    assert_dev(amount < 8);
    return a << amount;
}

u16 shl_unsafe_u16(u16 a, u64 amount) {
    assert_dev(amount < 16);
    return a << amount;
}

u32 shl_unsafe_u32(u32 a, u64 amount) {
    assert_dev(amount < 32);
    return a << amount;
}

u64 shl_unsafe_u64(u64 a, u64 amount) {
    assert_dev(amount < 64);
    return a << amount;
}

u8 shr_u8(u8 a, u64 amount, bool *fail) {
    bool const oob = amount >= 8;
    *fail |= oob;
    u8 const res = oob ? 0 : (a >> amount);
    return res;
}

u16 shr_u16(u16 a, u64 amount, bool *fail) {
    bool const oob = amount >= 16;
    *fail |= oob;
    u16 const res = oob ? 0 : (a >> amount);
    return res;
}

u32 shr_u32(u32 a, u64 amount, bool *fail) {
    bool const oob = amount >= 32;
    *fail |= oob;
    u32 const res = oob ? 0 : (a >> amount);
    return res;
}

u64 shr_u64(u64 a, u64 amount, bool *fail) {
    bool const oob = amount >= 64;
    *fail |= oob;
    u64 const res = oob ? 0 : (a >> amount);
    return res;
}

u8 shr_unsafe_u8(u8 a, u64 amount) {
    assert_dev(amount < 8);
    return a >> amount;
}

u16 shr_unsafe_u16(u16 a, u64 amount) {
    assert_dev(amount < 16);
    return a >> amount;
}

u32 shr_unsafe_u32(u32 a, u64 amount) {
    assert_dev(amount < 32);
    return a >> amount;
}

u64 shr_unsafe_u64(u64 a, u64 amount) {
    assert_dev(amount < 64);
    return a >> amount;
}

u8 next_pow_two_u8(u8 a, bool *overflow) {
    if (a == 0) {
        return 1;
    }
    u8 const shift = bit_width_u8(a - u8{1});
    return shl_u8(1, shift, overflow);
}

u16 next_pow_two_u16(u16 a, bool *overflow) {
    if (a == 0) {
        return 1;
    }
    u8 const shift = bit_width_u16(a - u16{1});
    return shl_u16(1, shift, overflow);
}

u32 next_pow_two_u32(u32 a, bool *overflow) {
    if (a == 0) {
        return 1;
    }
    u8 const shift = bit_width_u32(a - u32{1});
    return shl_u32(1, shift, overflow);
}

u64 next_pow_two_u64(u64 a, bool *overflow) {
    if (a == 0) {
        return 1;
    }
    u8 const shift = bit_width_u64(a - u64{1});
    return shl_u64(1, shift, overflow);
}

u8 next_pow_two_unsafe_u8(u8 a) {
    if (a == 0) {
        return 1;
    }
    u8 const shift = bit_width_u8(a - u8{1});
    return shl_unsafe_u8(1, shift);
}

u16 next_pow_two_unsafe_u16(u16 a) {
    if (a == 0) {
        return 1;
    }
    u8 const shift = bit_width_u16(a - u16{1});
    return shl_unsafe_u16(1, shift);
}

u32 next_pow_two_unsafe_u32(u32 a) {
    if (a == 0) {
        return 1;
    }
    u8 const shift = bit_width_u32(a - u32{1});
    return shl_unsafe_u32(1, shift);
}

u64 next_pow_two_unsafe_u64(u64 a) {
    if (a == 0) {
        return 1;
    }
    u8 const shift = bit_width_u64(a - u64{1});
    return shl_unsafe_u64(1, shift);
}

constexpr bool is_pow_two_u8(u8 a) { return __builtin_popcountg(a) == 1; }

constexpr bool is_pow_two_u16(u16 a) { return __builtin_popcountg(a) == 1; }

constexpr bool is_pow_two_u32(u32 a) { return __builtin_popcountg(a) == 1; }

constexpr bool is_pow_two_u64(u64 a) { return __builtin_popcountg(a) == 1; }

u8 inverse_u8(u8 a) { return wrapping_add_u8(~a, 1); }

u16 inverse_u16(u16 a) { return wrapping_add_u16(~a, 1); }

u32 inverse_u32(u32 a) { return wrapping_add_u32(~a, 1); }

u64 inverse_u64(u64 a) { return wrapping_add_u64(~a, 1); }

u8 u16_to_u8(u16 a, bool *failure) {
    u8 res;
    *failure |= __builtin_add_overflow(a, u16{0}, &res);
    return res;
}

u8 u32_to_u8(u32 a, bool *failure) {
    u8 res;
    *failure |= __builtin_add_overflow(a, u32{0}, &res);
    return res;
}

u8 u64_to_u8(u64 a, bool *failure) {
    u8 res;
    *failure |= __builtin_add_overflow(a, u64{0}, &res);
    return res;
}

u8 i8_to_u8(i8 a, bool *failure) {
    u8 res;
    *failure |= __builtin_add_overflow(a, i8{0}, &res);
    return res;
}

u8 i16_to_u8(i16 a, bool *failure) {
    u8 res;
    *failure |= __builtin_add_overflow(a, i16{0}, &res);
    return res;
}

u8 i32_to_u8(i32 a, bool *failure) {
    u8 res;
    *failure |= __builtin_add_overflow(a, i32{0}, &res);
    return res;
}

u8 i64_to_u8(i64 a, bool *failure) {
    u8 res;
    *failure |= __builtin_add_overflow(a, i64{0}, &res);
    return res;
}

u16 u32_to_u16(u32 a, bool *failure) {
    u16 res;
    *failure |= __builtin_add_overflow(a, u32{0}, &res);
    return res;
}

u16 u64_to_u16(u64 a, bool *failure) {
    u16 res;
    *failure |= __builtin_add_overflow(a, u64{0}, &res);
    return res;
}

u16 i16_to_u16(i16 a, bool *failure) {
    u16 res;
    *failure |= __builtin_add_overflow(a, i16{0}, &res);
    return res;
}

u16 i32_to_u16(i32 a, bool *failure) {
    u16 res;
    *failure |= __builtin_add_overflow(a, i32{0}, &res);
    return res;
}

u16 i64_to_u16(i64 a, bool *failure) {
    u16 res;
    *failure |= __builtin_add_overflow(a, i64{0}, &res);
    return res;
}

u32 u64_to_u32(u64 a, bool *failure) {
    u32 res;
    *failure |= __builtin_add_overflow(a, u64{0}, &res);
    return res;
}

u32 i32_to_u32(i32 a, bool *failure) {
    u32 res;
    *failure |= __builtin_add_overflow(a, i32{0}, &res);
    return res;
}

u32 i64_to_u32(i64 a, bool *failure) {
    u32 res;
    *failure |= __builtin_add_overflow(a, i64{0}, &res);
    return res;
}

u64 i64_to_u64(i64 a, bool *failure) {
    u64 res;
    *failure |= __builtin_add_overflow(a, i64{0}, &res);
    return res;
}

i8 u8_to_i8(u8 a, bool *failure) {
    i8 res;
    *failure |= __builtin_add_overflow(a, u8{0}, &res);
    return res;
}

i8 u16_to_i8(u16 a, bool *failure) {
    i8 res;
    *failure |= __builtin_add_overflow(a, u16{0}, &res);
    return res;
}

i8 u32_to_i8(u32 a, bool *failure) {
    i8 res;
    *failure |= __builtin_add_overflow(a, u32{0}, &res);
    return res;
}

i8 u64_to_i8(u64 a, bool *failure) {
    i8 res;
    *failure |= __builtin_add_overflow(a, u64{0}, &res);
    return res;
}

i8 i16_to_i8(i16 a, bool *failure) {
    i8 res;
    *failure |= __builtin_add_overflow(a, i16{0}, &res);
    return res;
}

i8 i32_to_i8(i32 a, bool *failure) {
    i8 res;
    *failure |= __builtin_add_overflow(a, i32{0}, &res);
    return res;
}

i8 i64_to_i8(i64 a, bool *failure) {
    i8 res;
    *failure |= __builtin_add_overflow(a, i64{0}, &res);
    return res;
}

i16 u16_to_i16(u16 a, bool *failure) {
    i16 res;
    *failure |= __builtin_add_overflow(a, u16{0}, &res);
    return res;
}

i16 u32_to_i16(u32 a, bool *failure) {
    i16 res;
    *failure |= __builtin_add_overflow(a, u32{0}, &res);
    return res;
}

i16 u64_to_i16(u64 a, bool *failure) {
    i16 res;
    *failure |= __builtin_add_overflow(a, u64{0}, &res);
    return res;
}

i16 i32_to_i16(i32 a, bool *failure) {
    i16 res;
    *failure |= __builtin_add_overflow(a, i32{0}, &res);
    return res;
}

i16 i64_to_i16(i64 a, bool *failure) {
    i16 res;
    *failure |= __builtin_add_overflow(a, i64{0}, &res);
    return res;
}

i32 u32_to_i32(u32 a, bool *failure) {
    i32 res;
    *failure |= __builtin_add_overflow(a, u32{0}, &res);
    return res;
}

i32 u64_to_i32(u64 a, bool *failure) {
    i32 res;
    *failure |= __builtin_add_overflow(a, u64{0}, &res);
    return res;
}

i32 i64_to_i32(i64 a, bool *failure) {
    i32 res;
    *failure |= __builtin_add_overflow(a, i64{0}, &res);
    return res;
}

i64 u64_to_i64(u64 a, bool *failure) {
    i64 res;
    *failure |= __builtin_add_overflow(a, u64{0}, &res);
    return res;
}

u8 u16_to_u8_unsafe(u16 a) {
    u8 res;
    bool overflow = __builtin_add_overflow(a, u16{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u8 u32_to_u8_unsafe(u32 a) {
    u8 res;
    bool overflow = __builtin_add_overflow(a, u32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u8 u64_to_u8_unsafe(u64 a) {
    u8 res;
    bool overflow = __builtin_add_overflow(a, u64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u8 i8_to_u8_unsafe(i8 a) {
    u8 res;
    bool overflow = __builtin_add_overflow(a, i8{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u8 i16_to_u8_unsafe(i16 a) {
    u8 res;
    bool overflow = __builtin_add_overflow(a, i16{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u8 i32_to_u8_unsafe(i32 a) {
    u8 res;
    bool overflow = __builtin_add_overflow(a, i32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u8 i64_to_u8_unsafe(i64 a) {
    u8 res;
    bool overflow = __builtin_add_overflow(a, i64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u16 u32_to_u16_unsafe(u32 a) {
    u16 res;
    bool overflow = __builtin_add_overflow(a, u32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u16 u64_to_u16_unsafe(u64 a) {
    u16 res;
    bool overflow = __builtin_add_overflow(a, u64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u16 i16_to_u16_unsafe(i16 a) {
    u16 res;
    bool overflow = __builtin_add_overflow(a, i16{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u16 i32_to_u16_unsafe(i32 a) {
    u16 res;
    bool overflow = __builtin_add_overflow(a, i32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u16 i64_to_u16_unsafe(i64 a) {
    u16 res;
    bool overflow = __builtin_add_overflow(a, i64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u32 u64_to_u32_unsafe(u64 a) {
    u32 res;
    bool overflow = __builtin_add_overflow(a, u64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u32 i32_to_u32_unsafe(i32 a) {
    u32 res;
    bool overflow = __builtin_add_overflow(a, i32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u32 i64_to_u32_unsafe(i64 a) {
    u32 res;
    bool overflow = __builtin_add_overflow(a, i64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u64 i64_to_u64_unsafe(i64 a) {
    u64 res;
    bool overflow = __builtin_add_overflow(a, i64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i8 u8_to_i8_unsafe(u8 a) {
    i8 res;
    bool overflow = __builtin_add_overflow(a, u8{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i8 u16_to_i8_unsafe(u16 a) {
    i8 res;
    bool overflow = __builtin_add_overflow(a, u16{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i8 u32_to_i8_unsafe(u32 a) {
    i8 res;
    bool overflow = __builtin_add_overflow(a, u32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i8 u64_to_i8_unsafe(u64 a) {
    i8 res;
    bool overflow = __builtin_add_overflow(a, u64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i8 i16_to_i8_unsafe(i16 a) {
    i8 res;
    bool overflow = __builtin_add_overflow(a, i16{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i8 i32_to_i8_unsafe(i32 a) {
    i8 res;
    bool overflow = __builtin_add_overflow(a, i32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i8 i64_to_i8_unsafe(i64 a) {
    i8 res;
    bool overflow = __builtin_add_overflow(a, i64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i16 u16_to_i16_unsafe(u16 a) {
    i16 res;
    bool overflow = __builtin_add_overflow(a, u16{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i16 u32_to_i16_unsafe(u32 a) {
    i16 res;
    bool overflow = __builtin_add_overflow(a, u32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i16 u64_to_i16_unsafe(u64 a) {
    i16 res;
    bool overflow = __builtin_add_overflow(a, u64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i16 i32_to_i16_unsafe(i32 a) {
    i16 res;
    bool overflow = __builtin_add_overflow(a, i32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i16 i64_to_i16_unsafe(i64 a) {
    i16 res;
    bool overflow = __builtin_add_overflow(a, i64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i32 u32_to_i32_unsafe(u32 a) {
    i32 res;
    bool overflow = __builtin_add_overflow(a, u32{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i32 u64_to_i32_unsafe(u64 a) {
    i32 res;
    bool overflow = __builtin_add_overflow(a, u64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i32 i64_to_i32_unsafe(i64 a) {
    i32 res;
    bool overflow = __builtin_add_overflow(a, i64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

i64 u64_to_i64_unsafe(u64 a) {
    i64 res;
    bool overflow = __builtin_add_overflow(a, u64{0}, &res);
    zx::assert_dev(!overflow);
    return res;
}

u64 align_up_u64(u64 val, u64 align, bool *failure) {
    bool const pow_two = is_pow_two_u64(align);
    if (!pow_two) [[unlikely]] {
        *failure = true;
        return 0;
    }
    u64 const rem = val & (align - 1);
    return (rem == 0) ? val : add_u64(val, align - rem, failure);
}

u64 align_up_unsafe_u64(u64 val, u64 align) {
    assert_dev(is_pow_two_u64(align));
    u64 const rem = val & (align - 1);
    return (rem == 0) ? val : add_unsafe_u64(val, sub_unsafe_u64(align, rem));
}

u64 align_down_u64(u64 val, u64 align, bool *failure) {
    *failure |= !is_pow_two_u64(align);
    return val & ~(align - 1);
}

u64 align_down_unsafe_u64(u64 val, u64 align) {
    assert_dev(is_pow_two_u64(align));
    return val & ~(align - 1);
}

} // namespace zx
