export module zx:arith;
import :def;
import :assert;

export namespace zx {

u8 wrapping_add_u8(u8 a, u8 b) {
  return a + b;
}

u16 wrapping_add_u16(u16 a, u16 b) {
  return a + b;
}

u32 wrapping_add_u32(u32 a, u32 b) {
  return a + b;
}

u64 wrapping_add_u64(u64 a, u64 b) {
  return a + b;
}

i8 wrapping_add_i8(i8 a, i8 b) {
  return a + b;
}

i16 wrapping_add_i16(i16 a, i16 b) {
  return a + b;
}

i32 wrapping_add_i32(i32 a, i32 b) {
  return a + b;
}

i64 wrapping_add_i64(i64 a, i64 b) {
  return a + b;
}

u8 saturating_add_u8(u8 a, u8 b) {
  return __builtin_elementwise_add_sat(a, b);
}

u16 saturating_add_u16(u16 a, u16 b) {
  return __builtin_elementwise_add_sat(a, b);
}

u32 saturating_add_u32(u32 a, u32 b) {
  return __builtin_elementwise_add_sat(a, b);
}

u64 saturating_add_u64(u64 a, u64 b) {
  return __builtin_elementwise_add_sat(a, b);
}

i8 saturating_add_i8(i8 a, i8 b) {
  return __builtin_elementwise_add_sat(a, b);
}

i16 saturating_add_i16(i16 a, i16 b) {
  return __builtin_elementwise_add_sat(a, b);
}

i32 saturating_add_i32(i32 a, i32 b) {
  return __builtin_elementwise_add_sat(a, b);
}

i64 saturating_add_i64(i64 a, i64 b) {
  return __builtin_elementwise_add_sat(a, b);
}

u8 add_u8(u8 a, u8 b, bool* overflow) {
  u8 res;
  *failure |= __builtin_add_overflow(a, b, &res);
  return res;
}

u16 add_u16(u16 a, u16 b, bool* overflow) {
  u16 res;
  *failure |= __builtin_add_overflow(a, b, &res);
  return res;
}

u32 add_u32(u32 a, u32 b, bool* overflow) {
  u32 res;
  *failure |= __builtin_add_overflow(a, b, &res);
  return res;
}

u64 add_u64(u64 a, u64 b, bool* overflow) {
  u64 res;
  *failure |= __builtin_add_overflow(a, b, &res);
  return res;
}

i8 add_i8(i8 a, i8 b, bool* overflow) {
  i8 res;
  *failure |= __builtin_add_overflow(a, b, &res);
  return res;
}

i16 add_i16(i16 a, i16 b, bool* overflow) {
  i16 res;
  *failure |= __builtin_add_overflow(a, b, &res);
  return res;
}

i32 add_i32(i32 a, i32 b, bool* overflow) {
  i32 res;
  *failure |= __builtin_add_overflow(a, b, &res);
  return res;
}

i64 add_i64(i64 a, i64 b, bool* overflow) {
  i64 res;
  *failure |= __builtin_add_overflow(a, b, &res);
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

u8 wrapping_sub_u8(u8 a, u8 b) {
  return a - b;
}

u16 wrapping_sub_u16(u16 a, u16 b) {
  return a - b;
}

u32 wrapping_sub_u32(u32 a, u32 b) {
  return a - b;
}

u64 wrapping_sub_u64(u64 a, u64 b) {
  return a - b;
}

i8 wrapping_sub_i8(i8 a, i8 b) {
  return a - b;
}

i16 wrapping_sub_i16(i16 a, i16 b) {
  return a - b;
}

i32 wrapping_sub_i32(i32 a, i32 b) {
  return a - b;
}

i64 wrapping_sub_i64(i64 a, i64 b) {
  return a - b;
}

u8 saturating_sub_u8(u8 a, u8 b) {
  return __builtin_elementwise_sub_sat(a, b);
}

u16 saturating_sub_u16(u16 a, u16 b) {
  return __builtin_elementwise_sub_sat(a, b);
}

u32 saturating_sub_u32(u32 a, u32 b) {
  return __builtin_elementwise_sub_sat(a, b);
}

u64 saturating_sub_u64(u64 a, u64 b) {
  return __builtin_elementwise_sub_sat(a, b);
}

i8 saturating_sub_i8(i8 a, i8 b) {
  return __builtin_elementwise_sub_sat(a, b);
}

i16 saturating_sub_i16(i16 a, i16 b) {
  return __builtin_elementwise_sub_sat(a, b);
}

i32 saturating_sub_i32(i32 a, i32 b) {
  return __builtin_elementwise_sub_sat(a, b);
}

i64 saturating_sub_i64(i64 a, i64 b) {
  return __builtin_elementwise_sub_sat(a, b);
}

u8 sub_u8(u8 a, u8 b, bool* overflow) {
  u8 res;
  *overflow |= __builtin_add_overflow(a, b, &res);
  return res;
}

u16 sub_u16(u16 a, u16 b, bool* overflow) {
  u16 res;
  *overflow |= __builtin_add_overflow(a, b, &res);
  return res;
}

u32 sub_u32(u32 a, u32 b, bool* overflow) {
  u32 res;
  *overflow |= __builtin_add_overflow(a, b, &res);
  return res;
}

u64 sub_u64(u64 a, u64 b, bool* overflow) {
  u64 res;
  *overflow |= __builtin_add_overflow(a, b, &res);
  return res;
}

i8 sub_i8(i8 a, i8 b, bool* overflow) {
  i8 res;
  *overflow |= __builtin_add_overflow(a, b, &res);
  return res;
}

i16 sub_i16(i16 a, i16 b, bool* overflow) {
  i16 res;
  *overflow |= __builtin_add_overflow(a, b, &res);
  return res;
}

i32 sub_i32(i32 a, i32 b, bool* overflow) {
  i32 res;
  *overflow |= __builtin_add_overflow(a, b, &res);
  return res;
}

i64 sub_i64(i64 a, i64 b, bool* overflow) {
  i64 res;
  *overflow |= __builtin_add_overflow(a, b, &res);
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

u8 wrapping_mul_u8(u8 a, u8 b) {
  return a * b;
}

u16 wrapping_mul_u16(u16 a, u16 b) {
  return a * b;
}

u32 wrapping_mul_u32(u32 a, u32 b) {
  return a * b;
}

u64 wrapping_mul_u64(u64 a, u64 b) {
  return a * b;
}

u8 saturating_mul_u8(u8 a, u8 b) {
  u8 res;
  if (__builtin_mul_overflow(a, b, &res)) {
    return U8_MAX;
  } else {
    return res;
  }
}

u16 saturating_mul_u16(u16 a, u16 b) {
  return __builtin_elementwise_mul_sat(a, b);
}

u32 saturating_mul_u32(u32 a, u32 b) {
  return __builtin_elementwise_mul_sat(a, b);
}

u64 saturating_mul_u64(u64 a, u64 b) {
  return __builtin_elementwise_mul_sat(a, b);
}

u8 mul_u8(u8 a, u8 b, bool* overflow) {
  u8 res;
  *overflow |= __builtin_mul_overflow(a, b, &res);
  return res;
}

u16 mul_u16(u16 a, u16 b, bool* overflow) {
  u16 res;
  *overflow |= __builtin_mul_overflow(a, b, &res);
  return res;
}

u32 mul_u32(u32 a, u32 b, bool* overflow) {
  u32 res;
  *overflow |= __builtin_mul_overflow(a, b, &res);
  return res;
}

u64 mul_u64(u64 a, u64 b, bool* overflow) {
  u64 res;
  *overflow |= __builtin_mul_overflow(a, b, &res);
  return res;
}

i8 mul_i8(i8 a, i8 b, bool* overflow) {
  i8 res;
  *overflow |= __builtin_mul_overflow(a, b, &res);
  return res;
}

i16 mul_i16(i16 a, i16 b, bool* overflow) {
  i16 res;
  *overflow |= __builtin_mul_overflow(a, b, &res);
  return res;
}

i32 mul_i32(i32 a, i32 b, bool* overflow) {
  i32 res;
  *overflow |= __builtin_mul_overflow(a, b, &res);
  return res;
}

i64 mul_i64(i64 a, i64 b, bool* overflow) {
  i64 res;
  *overflow |= __builtin_mul_overflow(a, b, &res);
  return res;
}

u8 mul_unsafe_u8(u8 a, u8 b) {
  u8 res;
  assert_dev(!__builtin_mul_overflow(a, b, &res));
  res;
}

u16 mul_unsafe_u16(u16 a, u16 b) {
  u16 res;
  assert_dev(!__builtin_mul_overflow(a, b, &res));
  res;
}

u32 mul_unsafe_u32(u32 a, u32 b) {
  u32 res;
  assert_dev(!__builtin_mul_overflow(a, b, &res));
  res;
}

u64 mul_unsafe_u64(u64 a, u64 b) {
  u64 res;
  assert_dev(!__builtin_mul_overflow(a, b, &res));
  res;
}

i8 mul_unsafe_i8(i8 a, i8 b) {
  i8 res;
  assert_dev(!__builtin_mul_overflow(a, b, &res));
  res;
}

i16 mul_unsafe_i16(i16 a, i16 b) {
  i16 res;
  assert_dev(!__builtin_mul_overflow(a, b, &res));
  res;
}

i32 mul_unsafe_i32(i32 a, i32 b) {
  i32 res;
  assert_dev(!__builtin_mul_overflow(a, b, &res));
  res;
}

i64 mul_unsafe_i64(i64 a, i64 b) {
  i64 res;
  assert_dev(!__builtin_mul_overflow(a, b, &res));
  res;
}

u8 div_u8(u8 a, u8 b, bool* div_by_zero) {
  if (b == 0) [[unlikely]] {
    *div_by_zero = true;
    return 0;
  } else {
    return a / b;
  }
}

u16 div_u16(u16 a, u16 b, bool* div_by_zero) {
  if (b == 0) [[unlikely]] {
    *div_by_zero = true;
    return 0;
  } else {
    return a / b;
  }
}

u32 div_u32(u32 a, u32 b, bool* div_by_zero) {
  if (b == 0) [[unlikely]] {
    *div_by_zero = true;
    return 0;
  } else {
    return a / b;
  }
}

u64 div_u64(u64 a, u64 b, bool* div_by_zero) {
  if (b == 0) [[unlikely]] {
    *div_by_zero = true;
    return 0;
  } else {
    return a / b;
  }
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

u8 mod_u8(u8 a, u8 b, bool* div_by_zero) {
  if (b == 0) [[unlikely]] {
    *div_by_zero = true;
    return 0;
  } else {
    return a % b;
  }
}

u16 mod_u16(u16 a, u16 b, bool* div_by_zero) {
  if (b == 0) [[unlikely]] {
    *div_by_zero = true;
    return 0;
  } else {
    return a % b;
  }
}

u32 mod_u32(u32 a, u32 b, bool* div_by_zero) {
  if (b == 0) [[unlikely]] {
    *div_by_zero = true;
    return 0;
  } else {
    return a % b;
  }
}

u64 mod_u64(u64 a, u64 b, bool* div_by_zero) {
  if (b == 0) [[unlikely]] {
    *div_by_zero = true;
    return 0;
  } else {
    return a % b;
  }
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

u32 mulshift_u32(u32 a, u32 b) {
  u64 const mul = u64{a} * u64{b};
  return (u32)(mul >> 32);
}

u64 mulshift_u64(u64 a, u64 b) {
  u128 const mul = u128{a} * u128{b};
  return (u64)(mul >> 64);
}

} // export namespace zx
