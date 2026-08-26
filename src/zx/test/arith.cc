import zx;

int main(void) {
  // wrapping add
  zx::raw_assert(1 == zx::wrapping_add_u8(zx::U8_MAX, 2));
  zx::raw_assert(1 == zx::wrapping_add_u16(zx::U16_MAX, 2));
  zx::raw_assert(1 == zx::wrapping_add_u32(zx::U32_MAX, 2));
  zx::raw_assert(1 == zx::wrapping_add_u64(zx::U64_MAX, 2));

  zx::raw_assert(69 == zx::wrapping_add_u8(65, 4));
  zx::raw_assert(69 == zx::wrapping_add_u16(65, 4));
  zx::raw_assert(69 == zx::wrapping_add_u32(65, 4));
  zx::raw_assert(69 == zx::wrapping_add_u64(65, 4));

  // wrapping sub
  zx::raw_assert(1 == zx::wrapping_sub_u8(0, zx::U8_MAX));
  zx::raw_assert(1 == zx::wrapping_sub_u16(0, zx::U16_MAX));
  zx::raw_assert(1 == zx::wrapping_sub_u32(0, zx::U32_MAX));
  zx::raw_assert(1 == zx::wrapping_sub_u64(0, zx::U64_MAX));

  zx::raw_assert(61 == zx::wrapping_sub_u8(65, 4));
  zx::raw_assert(61 == zx::wrapping_sub_u16(65, 4));
  zx::raw_assert(61 == zx::wrapping_sub_u32(65, 4));
  zx::raw_assert(61 == zx::wrapping_sub_u64(65, 4));

  // wrapping mul
  zx::raw_assert(zx::U8_MAX - 2__wbu == zx::wrapping_mul_u8(zx::U8_MAX, 3));
  zx::raw_assert(zx::U16_MAX - 2__wbu == zx::wrapping_mul_u16(zx::U16_MAX, 3));
  zx::raw_assert(zx::U32_MAX - 2__wbu == zx::wrapping_mul_u32(zx::U32_MAX, 3));
  zx::raw_assert(zx::U64_MAX - 2__wbu == zx::wrapping_mul_u64(zx::U64_MAX, 3));

  zx::raw_assert(3 == zx::wrapping_mul_u8(1, 3));
  zx::raw_assert(3 == zx::wrapping_mul_u16(1, 3));
  zx::raw_assert(3 == zx::wrapping_mul_u32(1, 3));
  zx::raw_assert(3 == zx::wrapping_mul_u64(1, 3));

  // saturating add
  zx::raw_assert(zx::U8_MAX == zx::saturating_add_u8(zx::U8_MAX - 1, 250));
  zx::raw_assert(zx::U16_MAX == zx::saturating_add_u16(zx::U16_MAX - 1, 250));
  zx::raw_assert(zx::U32_MAX == zx::saturating_add_u32(zx::U32_MAX - 1, 250));
  zx::raw_assert(zx::U64_MAX == zx::saturating_add_u64(zx::U64_MAX - 1, 250));
  zx::raw_assert(2 == zx::saturating_add_u8(1, 1));
  zx::raw_assert(2 == zx::saturating_add_u16(1, 1));
  zx::raw_assert(2 == zx::saturating_add_u32(1, 1));
  zx::raw_assert(2 == zx::saturating_add_u64(1, 1));

  // saturating sub
  zx::raw_assert(0 == zx::saturating_sub_u8(1, 2));
  zx::raw_assert(0 == zx::saturating_sub_u16(1, 2));
  zx::raw_assert(0 == zx::saturating_sub_u32(1, 2));
  zx::raw_assert(0 == zx::saturating_sub_u64(1, 2));

  // add one over
  bool overflow = false;
  zx::add_u8(zx::U8_MAX, 1, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::add_u16(zx::U16_MAX, 1, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::add_u32(zx::U32_MAX, 1, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::add_u64(zx::U64_MAX, 1, &overflow);
  zx::raw_assert(overflow);

  // add no overflow
  overflow = false;
  zx::add_u8(69, 31, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(100 == zx::add_unsafe_u8(69, 31));

  overflow = false;
  zx::add_u16(69, 31, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(100 == zx::add_unsafe_u16(69, 31));

  overflow = false;
  zx::add_u32(69, 31, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(100 == zx::add_unsafe_u32(69, 31));

  overflow = false;
  zx::add_u64(69, 31, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(100 == zx::add_unsafe_u64(69, 31));

  // sub one over
  overflow = false;
  zx::sub_u8(0, 1, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::sub_u16(0, 1, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::sub_u32(0, 1, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::sub_u64(0, 1, &overflow);
  zx::raw_assert(overflow);

  // sub no overflow
  overflow = false;
  zx::sub_u8(zx::U8_MAX, 69, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(zx::u8{0} - zx::u8{70} == zx::sub_unsafe_u8(zx::U8_MAX, 69));

  overflow = false;
  zx::sub_u16(zx::U16_MAX, 69, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(zx::u16{0} - zx::u16{70} == zx::sub_unsafe_u16(zx::U16_MAX, 69));

  overflow = false;
  zx::sub_u32(zx::U32_MAX, 69, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(zx::u32{0} - zx::u32{70} == zx::sub_unsafe_u32(zx::U32_MAX, 69));

  overflow = false;
  zx::sub_u64(zx::U64_MAX, 69, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(zx::u64{0} - zx::u64{70} == zx::sub_unsafe_u64(zx::U64_MAX, 69));

  // mul overflow
  overflow = false;
  zx::mul_u8(zx::U8_MAX / 2, 3, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::mul_u16(zx::U16_MAX / 2, 3, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::mul_u32(zx::U32_MAX / 2, 3, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::mul_u64(zx::U64_MAX / 2, 3, &overflow);
  zx::raw_assert(overflow);

  // mul no overflow
  overflow = false;
  zx::mul_u8(zx::U8_MAX / 2, 2, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(zx::U8_MAX - 1__wbu == zx::mul_unsafe_u8(zx::U8_MAX / 2, 2));

  overflow = false;
  zx::mul_u16(zx::U16_MAX / 2, 2, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(zx::U16_MAX - 1__wbu == zx::mul_unsafe_u16(zx::U16_MAX / 2, 2));

  overflow = false;
  zx::mul_u32(zx::U32_MAX / 2, 2, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(zx::U32_MAX - 1__wbu == zx::mul_unsafe_u32(zx::U32_MAX / 2, 2));

  overflow = false;
  zx::mul_u64(zx::U64_MAX / 2, 2, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(zx::U64_MAX - 1__wbu == zx::mul_unsafe_u64(zx::U64_MAX / 2, 2));

  // div zero
  overflow = false;
  zx::div_u8(69, 0, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::div_u16(69, 0, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::div_u32(69, 0, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::div_u64(69, 0, &overflow);
  zx::raw_assert(overflow);

  // div nonzero
  overflow = false;
  zx::raw_assert(0 == zx::div_u8(69, 113, &overflow));
  zx::raw_assert(!overflow);

  zx::raw_assert(0 == zx::div_unsafe_u8(69, 113));

  overflow = false;
  zx::raw_assert(0 == zx::div_u16(69, 1131, &overflow));
  zx::raw_assert(!overflow);

  zx::raw_assert(0 == zx::div_unsafe_u16(69, 1131));

  overflow = false;
  zx::raw_assert(0 == zx::div_u32(69, 1131, &overflow));
  zx::raw_assert(!overflow);

  zx::raw_assert(0 == zx::div_unsafe_u32(69, 1131));

  overflow = false;
  zx::raw_assert(0 == zx::div_u64(69, 1131, &overflow));
  zx::raw_assert(!overflow);

  zx::raw_assert(0 == zx::div_unsafe_u64(69, 1131));

  // mod zero
  overflow = false;
  zx::mod_u8(69, 0, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::mod_u16(69, 0, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::mod_u32(69, 0, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::mod_u64(69, 0, &overflow);
  zx::raw_assert(overflow);

  // mod nonzero
  overflow = false;
  zx::mod_u8(69, 113, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(69__wbu == zx::mod_unsafe_u8(69, 113));

  overflow = false;
  zx::mod_u16(69, 1131, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(69__wbu == zx::mod_unsafe_u16(69, 113));

  overflow = false;
  zx::mod_u32(69, 1131, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(69__wbu == zx::mod_unsafe_u32(69, 113));

  overflow = false;
  zx::mod_u64(69, 1131, &overflow);
  zx::raw_assert(!overflow);

  zx::raw_assert(69__wbu == zx::mod_unsafe_u64(69, 113));

  // div_ceil overflow
  overflow = false;
  zx::div_ceil_u8(zx::U8_MAX, 3, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::div_ceil_u16(zx::U16_MAX, 3, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::div_ceil_u32(zx::U32_MAX, 3, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::div_ceil_u64(zx::U64_MAX, 3, &overflow);
  zx::raw_assert(overflow);

  // div_ceil no overflow
  overflow = false;
  zx::raw_assert(1 == zx::div_ceil_u8(1, 3, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(2 == zx::div_ceil_u8(4, 3, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(1 == zx::div_ceil_u16(1, 3, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(2 == zx::div_ceil_u16(4, 3, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(1 == zx::div_ceil_u32(1, 3, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(2 == zx::div_ceil_u32(4, 3, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(1 == zx::div_ceil_u64(1, 3, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(2 == zx::div_ceil_u64(4, 3, &overflow));
  zx::raw_assert(!overflow);

  // next_mult overflow
  overflow = false;
  zx::next_multiple_of_u8(zx::U8_MAX, 3, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::next_multiple_of_u16(zx::U16_MAX, 3, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::next_multiple_of_u32(zx::U32_MAX, 3, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::next_multiple_of_u64(zx::U64_MAX, 3, &overflow);
  zx::raw_assert(overflow);

  // next_mult no overflow
  overflow = false;
  zx::raw_assert(3 == zx::next_multiple_of_u8(1, 3, &overflow));
  zx::raw_assert(!overflow);

  zx::raw_assert(3 == zx::next_multiple_of_unsafe_u8(1, 3));

  overflow = false;
  zx::raw_assert(3 == zx::next_multiple_of_u16(1, 3, &overflow));
  zx::raw_assert(!overflow);

  zx::raw_assert(3 == zx::next_multiple_of_unsafe_u16(1, 3));

  overflow = false;
  zx::raw_assert(3 == zx::next_multiple_of_u32(1, 3, &overflow));
  zx::raw_assert(!overflow);

  zx::raw_assert(3 == zx::next_multiple_of_unsafe_u32(1, 3));

  overflow = false;
  zx::raw_assert(3 == zx::next_multiple_of_u64(1, 3, &overflow));
  zx::raw_assert(!overflow);

  zx::raw_assert(3 == zx::next_multiple_of_unsafe_u64(1, 3));

  // min
  zx::raw_assert(31 == zx::min_u8(31, 69));
  zx::raw_assert(31 == zx::min_u8(69, 31));

  zx::raw_assert(31 == zx::min_u16(31, 69));
  zx::raw_assert(31 == zx::min_u16(69, 31));

  zx::raw_assert(31 == zx::min_u32(31, 69));
  zx::raw_assert(31 == zx::min_u32(69, 31));

  zx::raw_assert(31 == zx::min_u64(31, 69));
  zx::raw_assert(31 == zx::min_u64(69, 31));

  // max
  zx::raw_assert(69 == zx::max_u8(31, 69));
  zx::raw_assert(69 == zx::max_u8(69, 31));

  zx::raw_assert(69 == zx::max_u16(31, 69));
  zx::raw_assert(69 == zx::max_u16(69, 31));

  zx::raw_assert(69 == zx::max_u32(31, 69));
  zx::raw_assert(69 == zx::max_u32(69, 31));

  zx::raw_assert(69 == zx::max_u64(31, 69));
  zx::raw_assert(69 == zx::max_u64(69, 31));

  // shr UB
  bool fail = false;
  zx::shr_u8(1, 8, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shr_u16(0, 16, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shr_u32(1, 32, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shr_u64(0, 64, &fail);
  zx::raw_assert(fail);

  // shr UB more
  fail = false;
  zx::shr_u8(1, 20, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shr_u16(1, 30, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shr_u32(1, 60, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shr_u64(1, zx::U8_MAX, &fail);
  zx::raw_assert(fail);

  // shr no UB
  fail = false;
  zx::shr_u8(0, 1, &fail);
  zx::raw_assert(!fail);

  zx::raw_assert(0 == zx::shr_unsafe_u8(0, 1));
  zx::raw_assert(1 == zx::shr_unsafe_u8(4, 2));

  fail = false;
  zx::shr_u16(0, 1, &fail);
  zx::raw_assert(!fail);

  zx::raw_assert(0 == zx::shr_unsafe_u16(0, 1));
  zx::raw_assert(1 == zx::shr_unsafe_u16(4, 2));

  fail = false;
  zx::shr_u32(0, 1, &fail);
  zx::raw_assert(!fail);

  zx::raw_assert(0 == zx::shr_unsafe_u32(0, 1));
  zx::raw_assert(1 == zx::shr_unsafe_u32(4, 2));

  fail = false;
  zx::shr_u64(0, 1, &fail);
  zx::raw_assert(!fail);

  zx::raw_assert(0 == zx::shr_unsafe_u64(0, 1));
  zx::raw_assert(1 == zx::shr_unsafe_u64(4, 2));

  // shl UB
  fail = false;
  zx::shl_u8(1, 8, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shl_u16(0, 16, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shl_u32(1, 32, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shl_u64(0, 64, &fail);
  zx::raw_assert(fail);

  // shl UB more
  fail = false;
  zx::shl_u8(1, 20, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shl_u16(1, 30, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shl_u32(1, 60, &fail);
  zx::raw_assert(fail);

  fail = false;
  zx::shl_u64(1, zx::U8_MAX, &fail);
  zx::raw_assert(fail);

  // shl no UB
  fail = false;
  zx::shl_u8(0, 1, &fail);
  zx::raw_assert(!fail);

  zx::raw_assert(0 == zx::shl_unsafe_u8(0, 1));
  zx::raw_assert(4 == zx::shl_unsafe_u8(1, 2));

  fail = false;
  zx::shl_u16(0, 1, &fail);
  zx::raw_assert(!fail);

  zx::raw_assert(0 == zx::shl_unsafe_u16(0, 1));
  zx::raw_assert(4 == zx::shl_unsafe_u16(1, 2));

  fail = false;
  zx::shl_u32(0, 1, &fail);
  zx::raw_assert(!fail);

  zx::raw_assert(0 == zx::shl_unsafe_u32(0, 1));
  zx::raw_assert(4 == zx::shl_unsafe_u32(1, 2));

  fail = false;
  zx::shl_u64(0, 1, &fail);
  zx::raw_assert(!fail);

  zx::raw_assert(0 == zx::shl_unsafe_u64(0, 1));
  zx::raw_assert(4 == zx::shl_unsafe_u64(1, 2));

  // ctz
  zx::raw_assert(3 == zx::ctz_u8(0b01111000));
  zx::raw_assert(3 == zx::ctz_u16(0b01111000));
  zx::raw_assert(3 == zx::ctz_u32(0b01111000));
  zx::raw_assert(3 == zx::ctz_u64(0b01111000));
  zx::raw_assert(8 == zx::ctz_u8(0));
  zx::raw_assert(16 == zx::ctz_u16(0));
  zx::raw_assert(32 == zx::ctz_u32(0));
  zx::raw_assert(64 == zx::ctz_u64(0));

  // clz
  zx::raw_assert(1 == zx::clz_u8(0b01111000));
  zx::raw_assert(9 == zx::clz_u16(0b01111000));
  zx::raw_assert(25 == zx::clz_u32(0b01111000));
  zx::raw_assert(57 == zx::clz_u64(0b01111000));
  zx::raw_assert(8 == zx::clz_u8(0));
  zx::raw_assert(16 == zx::clz_u16(0));
  zx::raw_assert(32 == zx::clz_u32(0));
  zx::raw_assert(64 == zx::clz_u64(0));

  // popcount
  zx::raw_assert(4 == zx::popcount_u8(0b01111000));
  zx::raw_assert(4 == zx::popcount_u16(0b01111000));
  zx::raw_assert(4 == zx::popcount_u32(0b01111000));
  zx::raw_assert(4 == zx::popcount_u64(0b01111000));
  zx::raw_assert(0 == zx::popcount_u8(0));
  zx::raw_assert(0 == zx::popcount_u16(0));
  zx::raw_assert(0 == zx::popcount_u32(0));
  zx::raw_assert(0 == zx::popcount_u64(0));
  zx::raw_assert(8 == zx::popcount_u8(zx::U8_MAX));
  zx::raw_assert(16 == zx::popcount_u16(zx::U16_MAX));
  zx::raw_assert(32 == zx::popcount_u32(zx::U32_MAX));
  zx::raw_assert(64 == zx::popcount_u64(zx::U64_MAX));

  // bit_width
  zx::raw_assert(7 == zx::bit_width_u8(0b01111000));
  zx::raw_assert(7 == zx::bit_width_u16(0b01111000));
  zx::raw_assert(7 == zx::bit_width_u32(0b01111000));
  zx::raw_assert(7 == zx::bit_width_u64(0b01111000));
  zx::raw_assert(0 == zx::bit_width_u8(0));
  zx::raw_assert(0 == zx::bit_width_u16(0));
  zx::raw_assert(0 == zx::bit_width_u32(0));
  zx::raw_assert(0 == zx::bit_width_u64(0));
  zx::raw_assert(8 == zx::bit_width_u8(zx::U8_MAX));
  zx::raw_assert(16 == zx::bit_width_u16(zx::U16_MAX));
  zx::raw_assert(32 == zx::bit_width_u32(zx::U32_MAX));
  zx::raw_assert(64 == zx::bit_width_u64(zx::U64_MAX));

  // next_pow_two
  overflow = false;
  zx::raw_assert(128 == zx::next_pow_two_u8(100, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(128 == zx::next_pow_two_u16(100, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(128 == zx::next_pow_two_u32(100, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(128 == zx::next_pow_two_u64(100, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(1 == zx::next_pow_two_u8(0, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(1 == zx::next_pow_two_u16(0, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(1 == zx::next_pow_two_u32(0, &overflow));
  zx::raw_assert(!overflow);

  overflow = false;
  zx::raw_assert(1 == zx::next_pow_two_u64(0, &overflow));
  zx::raw_assert(!overflow);

  // next_pow_two overflow
  overflow = false;
  zx::next_pow_two_u8(zx::U8_MAX, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::next_pow_two_u16(zx::U16_MAX, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::next_pow_two_u32(zx::U32_MAX, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::next_pow_two_u64(zx::U64_MAX, &overflow);
  zx::raw_assert(overflow);

  // is_pow_two
  zx::raw_assert(zx::is_pow_two_u8(1));
  zx::raw_assert(zx::is_pow_two_u16(1));
  zx::raw_assert(zx::is_pow_two_u32(1));
  zx::raw_assert(zx::is_pow_two_u64(1));

  zx::raw_assert(!zx::is_pow_two_u8(0));
  zx::raw_assert(!zx::is_pow_two_u16(0));
  zx::raw_assert(!zx::is_pow_two_u32(0));
  zx::raw_assert(!zx::is_pow_two_u64(0));

  // inverse
  zx::raw_assert(zx::u8{123} + zx::inverse_u8(123) == 0);
  zx::raw_assert(zx::u16{123} + zx::inverse_u16(123) == 0);
  zx::raw_assert(zx::u32{123} + zx::inverse_u32(123) == 0);
  zx::raw_assert(zx::u64{123} + zx::inverse_u64(123) == 0);

  // cast to u8
  overflow = false;

  zx::raw_assert(69 == zx::u16_to_u8(69, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u8<zx::u16>(zx::u16{69}));

  zx::raw_assert(69 == zx::cast_to_u8<zx::u32>(zx::u32{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u8<zx::u32>(zx::u32{69}));

  zx::raw_assert(69 == zx::cast_to_u8<zx::u64>(zx::u64{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u8<zx::u64>(zx::u64{69}));

  zx::raw_assert(69 == zx::cast_to_u8<zx::i8>(zx::i8{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u8<zx::i8>(zx::i8{69}));

  zx::raw_assert(69 == zx::cast_to_u8<zx::i16>(zx::i16{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u8<zx::i16>(zx::i16{69}));

  zx::raw_assert(69 == zx::cast_to_u8<zx::i32>(zx::i32{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u8<zx::i32>(zx::i32{69}));

  zx::raw_assert(69 == zx::cast_to_u8<zx::i64>(zx::i64{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u8<zx::i64>(zx::i64{69}));

  // cast to u16
  overflow = false;
  zx::raw_assert(69 == zx::cast_to_u16<zx::u8>(zx::u8{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u16<zx::u8>(zx::u8{69}));

  zx::raw_assert(1100 == zx::cast_to_u16<zx::u16>(zx::u16{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u16<zx::u16>(zx::u16{1100}));

  zx::raw_assert(1100 == zx::cast_to_u16<zx::u32>(zx::u32{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u16<zx::u32>(zx::u32{1100}));

  zx::raw_assert(1100 == zx::cast_to_u16<zx::u64>(zx::u64{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u16<zx::u64>(zx::u64{1100}));

  zx::raw_assert(120 == zx::cast_to_u16<zx::i8>(zx::i8{120}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(120 == zx::cast_unsafe_to_u16<zx::i8>(zx::i8{120}));

  zx::raw_assert(1100 == zx::cast_to_u16<zx::i16>(zx::i16{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u16<zx::i16>(zx::i16{1100}));

  zx::raw_assert(1100 == zx::cast_to_u16<zx::i32>(zx::i32{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u16<zx::i32>(zx::i32{1100}));

  zx::raw_assert(1100 == zx::cast_to_u16<zx::i64>(zx::i64{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u16<zx::i64>(zx::i64{1100}));

  // cast to u32
  overflow = false;
  zx::raw_assert(69 == zx::cast_to_u32<zx::u8>(zx::u8{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u32<zx::u8>(zx::u8{69}));

  zx::raw_assert(1100 == zx::cast_to_u32<zx::u16>(zx::u16{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u32<zx::u16>(zx::u16{1100}));

  zx::raw_assert(zx::u32{zx::U16_MAX} + 1__wbu ==
         zx::cast_to_u32<zx::u32>(zx::u32{zx::U16_MAX} + 1__wbu, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::u32{zx::U16_MAX} + 1__wbu ==
         zx::cast_unsafe_to_u32<zx::u32>(zx::u32{zx::U16_MAX} + 1__wbu));

  zx::raw_assert(zx::u64{zx::U16_MAX} + 1__wbu ==
         zx::cast_to_u32<zx::u64>(zx::u64{zx::U16_MAX} + 1__wbu, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::u32{zx::U16_MAX} + 1__wbu ==
         zx::cast_unsafe_to_u32<zx::u64>(zx::u64{zx::U16_MAX} + 1__wbu));

  zx::raw_assert(120 == zx::cast_to_u32<zx::i8>(zx::i8{120}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(120 == zx::cast_unsafe_to_u32<zx::i8>(zx::i8{120}));

  zx::raw_assert(1100 == zx::cast_to_u32<zx::i16>(zx::i16{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u32<zx::i16>(zx::i16{1100}));

  zx::raw_assert(zx::i32{zx::U16_MAX} + 1__wb ==
         zx::cast_to_u32<zx::i32>(zx::i32{zx::U16_MAX} + 1__wb, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::i32{zx::U16_MAX} + 1__wb ==
         zx::cast_unsafe_to_u32<zx::i32>(zx::i32{zx::U16_MAX} + 1__wb));

  zx::raw_assert(zx::U16_MAX + 1 ==
         zx::cast_to_u32<zx::i64>(zx::i64{zx::U16_MAX + 1}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::i64{zx::U16_MAX} + 1__wb ==
         zx::cast_unsafe_to_u32<zx::i64>(zx::i64{zx::U16_MAX} + 1__wb));

  // cast to u64
  overflow = false;
  zx::raw_assert(69 == zx::cast_to_u64<zx::u8>(zx::u8{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_u64<zx::u8>(zx::u8{69}));

  zx::raw_assert(1100 == zx::cast_to_u64<zx::u16>(zx::u16{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u64<zx::u16>(zx::u16{1100}));

  zx::raw_assert(zx::u32{zx::U16_MAX} + 1__wbu ==
         zx::cast_to_u64<zx::u32>(zx::u32{zx::U16_MAX} + 1__wbu, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::u32{zx::U16_MAX} + 1__wbu ==
         zx::cast_unsafe_to_u64<zx::u32>(zx::u32{zx::U16_MAX} + 1__wbu));

  zx::raw_assert(zx::U32_MAX + zx::u64{1} ==
         zx::cast_to_u64<zx::u64>(zx::u64{zx::U32_MAX + zx::u64{1}}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::u64{zx::U16_MAX} + 1__wbu ==
         zx::cast_unsafe_to_u64<zx::u64>(zx::u64{zx::U16_MAX} + 1__wbu));

  zx::raw_assert(120 == zx::cast_to_u64<zx::i8>(zx::i8{120}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(120 == zx::cast_unsafe_to_u64<zx::i8>(zx::i8{120}));

  zx::raw_assert(1100 == zx::cast_to_u64<zx::i16>(zx::i16{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_u64<zx::i16>(zx::i16{1100}));

  zx::raw_assert(zx::i32{zx::U16_MAX} + 1__wb ==
         zx::cast_to_u64<zx::i32>(zx::i32{zx::U16_MAX} + 1__wb, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::i32{zx::U16_MAX} + 1__wb ==
         zx::cast_unsafe_to_u64<zx::i32>(zx::i32{zx::U16_MAX} + 1__wb));

  zx::raw_assert(zx::U32_MAX + zx::u64{1} ==
         zx::cast_to_u64<zx::i64>(zx::i64{zx::U32_MAX + zx::u64{1}}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::i64{zx::U16_MAX} + 1__wb ==
         zx::cast_unsafe_to_u64<zx::i64>(zx::i64{zx::U16_MAX} + 1__wb));

  // cast to i8
  overflow = false;
  zx::raw_assert(69 == zx::cast_to_i8<zx::u8>(zx::u8{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_i8<zx::u8>(zx::u8{69}));

  zx::raw_assert(69 == zx::cast_to_i8<zx::u16>(zx::u16{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_i8<zx::u16>(zx::u16{69}));

  zx::raw_assert(69 == zx::cast_to_i8<zx::u32>(zx::u32{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_i8<zx::u32>(zx::u32{69}));

  zx::raw_assert(69 == zx::cast_to_i8<zx::u64>(zx::u64{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_i8<zx::u64>(zx::u64{69}));

  zx::raw_assert(-69 == zx::cast_to_i8<zx::i8>(zx::i8{-69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-69 == zx::cast_unsafe_to_i8<zx::i8>(zx::i8{-69}));

  zx::raw_assert(-69 == zx::cast_to_i8<zx::i16>(zx::i16{-69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-69 == zx::cast_unsafe_to_i8<zx::i16>(zx::i16{-69}));

  zx::raw_assert(-69 == zx::cast_to_i8<zx::i32>(zx::i32{-69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-69 == zx::cast_unsafe_to_i8<zx::i32>(zx::i32{-69}));

  zx::raw_assert(-69 == zx::cast_to_i8<zx::i64>(zx::i64{-69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-69 == zx::cast_unsafe_to_i8<zx::i64>(zx::i64{-69}));

  // cast to i16
  overflow = false;
  zx::raw_assert(69 == zx::cast_to_i16<zx::u8>(zx::u8{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_i16<zx::u8>(zx::u8{69}));

  zx::raw_assert(1100 == zx::cast_to_i16<zx::u16>(zx::u16{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_i16<zx::u16>(zx::u16{1100}));

  zx::raw_assert(1100 == zx::cast_to_i16<zx::u32>(zx::u32{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_i16<zx::u32>(zx::u32{1100}));

  zx::raw_assert(1100 == zx::cast_to_i16<zx::u64>(zx::u64{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_i16<zx::u64>(zx::u64{1100}));

  zx::raw_assert(-120 == zx::cast_to_i16<zx::i8>(zx::i8{-120}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-120 == zx::cast_unsafe_to_i16<zx::i8>(zx::i8{-120}));

  zx::raw_assert(-1100 == zx::cast_to_i16<zx::i16>(zx::i16{-1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-1100 == zx::cast_unsafe_to_i16<zx::i16>(zx::i16{-1100}));

  zx::raw_assert(-1100 == zx::cast_to_i16<zx::i32>(zx::i32{-1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-1100 == zx::cast_unsafe_to_i16<zx::i32>(zx::i32{-1100}));

  zx::raw_assert(-1100 == zx::cast_to_i16<zx::i64>(zx::i64{-1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-1100 == zx::cast_unsafe_to_i16<zx::i64>(zx::i64{-1100}));

  // cast to i32
  overflow = false;
  zx::raw_assert(69 == zx::cast_to_i32<zx::u8>(zx::u8{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_i32<zx::u8>(zx::u8{69}));

  zx::raw_assert(1100 == zx::cast_to_i32<zx::u16>(zx::u16{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_i32<zx::u16>(zx::u16{1100}));

  zx::raw_assert(zx::u32{zx::U16_MAX} + 1__wbu ==
         zx::cast_to_i32<zx::u32>(zx::u32{zx::U16_MAX} + 1__wbu, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::u32{zx::U16_MAX} + 1__wbu ==
         zx::cast_unsafe_to_i32<zx::u32>(zx::u32{zx::U16_MAX} + 1__wbu));

  zx::raw_assert(zx::u64{zx::U16_MAX} + 1__wbu ==
         zx::cast_to_i32<zx::u64>(zx::u64{zx::U16_MAX} + 1__wbu, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::u64{zx::U16_MAX} + 1__wbu ==
         zx::cast_unsafe_to_i32<zx::u64>(zx::u64{zx::U16_MAX} + 1__wbu));

  zx::raw_assert(-120 == zx::cast_to_i32<zx::i8>(zx::i8{-120}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-120 == zx::cast_unsafe_to_i32<zx::i8>(zx::i8{-120}));

  zx::raw_assert(-1100 == zx::cast_to_i32<zx::i16>(zx::i16{-1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-1100 == zx::cast_unsafe_to_i32<zx::i16>(zx::i16{-1100}));

  zx::raw_assert(-65537 == zx::cast_to_i32<zx::i32>(zx::i32{-65537}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-65537 == zx::cast_unsafe_to_i32<zx::i32>(zx::i32{-65537}));

  zx::raw_assert(-65537 == zx::cast_to_i32<zx::i64>(zx::i64{-65537}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-65537 == zx::cast_unsafe_to_i32<zx::i64>(zx::i64{-65537}));

  // cast to i64
  overflow = false;
  zx::raw_assert(69 == zx::cast_to_i64<zx::u8>(zx::u8{69}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(69 == zx::cast_unsafe_to_i64<zx::u8>(zx::u8{69}));

  zx::raw_assert(1100 == zx::cast_to_i64<zx::u16>(zx::u16{1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(1100 == zx::cast_unsafe_to_i64<zx::u16>(zx::u16{1100}));

  zx::raw_assert(zx::u32{zx::U16_MAX} + 1__wbu ==
         zx::cast_to_i64<zx::u32>(zx::u32{zx::U16_MAX} + 1__wbu, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::u32{zx::U16_MAX} + 1__wbu ==
         zx::cast_unsafe_to_i64<zx::u32>(zx::u32{zx::U16_MAX} + 1__wbu));

  zx::raw_assert(zx::u64{zx::U32_MAX} + 1__wbu ==
         zx::cast_to_i64<zx::u64>(zx::u64{zx::U32_MAX} + 1__wbu, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(zx::u64{zx::U32_MAX} + 1__wbu ==
         zx::cast_unsafe_to_i64<zx::u64>(zx::u64{zx::U32_MAX} + 1__wbu));

  zx::raw_assert(-120 == zx::cast_to_i64<zx::i8>(zx::i8{-120}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-120 == zx::cast_unsafe_to_i64<zx::i8>(zx::i8{-120}));

  zx::raw_assert(-1100 == zx::cast_to_i64<zx::i16>(zx::i16{-1100}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-1100 == zx::cast_unsafe_to_i64<zx::i16>(zx::i16{-1100}));

  zx::raw_assert(-65537 == zx::cast_to_i64<zx::i32>(zx::i32{-65537}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-65537 == zx::cast_unsafe_to_i64<zx::i32>(zx::i32{-65537}));

  zx::raw_assert(-4294967297__wb ==
         zx::cast_to_i64<zx::i64>(zx::i64{-4294967297__wb}, &overflow));
  zx::raw_assert(!overflow);
  zx::raw_assert(-4294967297__wb ==
         zx::cast_unsafe_to_i64<zx::i64>(zx::i64{-4294967297__wb}));

  // cast to u8 overflow
  overflow = false;
  zx::cast_to_u8<zx::u16>(zx::u16{300}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u8<zx::u32>(zx::u32{300}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u8<zx::u64>(zx::u64{300}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u8<zx::i8>(zx::i8{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u8<zx::i16>(zx::i16{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u8<zx::i16>(zx::i16{300}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u8<zx::i32>(zx::i32{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u8<zx::i32>(zx::i32{300}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u8<zx::i64>(zx::i64{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u8<zx::i64>(zx::i64{300}, &overflow);
  zx::raw_assert(overflow);

  // cast to u16 overflow
  overflow = false;
  zx::cast_to_u16<zx::u32>(zx::u32{70000}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u16<zx::u64>(zx::u64{70000}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u16<zx::i8>(zx::i8{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u16<zx::i16>(zx::i16{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u16<zx::i32>(zx::i32{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u16<zx::i32>(zx::i32{70000}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u16<zx::i64>(zx::i64{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u16<zx::i64>(zx::i64{70000}, &overflow);
  zx::raw_assert(overflow);

  // cast to u32 overflow
  overflow = false;
  zx::cast_to_u32<zx::u64>(zx::u64{zx::U32_MAX + zx::u64{1}}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u32<zx::i8>(zx::i8{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u32<zx::i16>(zx::i16{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u32<zx::i32>(zx::i32{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u32<zx::i64>(zx::i64{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u32<zx::i64>(zx::i64{zx::U32_MAX + zx::i64{1}}, &overflow);
  zx::raw_assert(overflow);

  // cast to u64 overflow
  overflow = false;
  zx::cast_to_u64<zx::u128>(zx::u128{zx::U64_MAX} + 1__wbu, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u64<zx::i8>(zx::i8{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u64<zx::i16>(zx::i16{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u64<zx::i32>(zx::i32{-1}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_u64<zx::i64>(zx::i64{-1}, &overflow);
  zx::raw_assert(overflow);

  // cast to i8 overflow
  overflow = false;
  zx::cast_to_i8<zx::u8>(zx::u8{128}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i8<zx::u16>(zx::u16{128}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i8<zx::u32>(zx::u32{128}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i8<zx::u64>(zx::u64{128}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i8<zx::i16>(zx::i16{-129}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i8<zx::i16>(zx::i16{128}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i8<zx::i32>(zx::i32{-129}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i8<zx::i32>(zx::i32{128}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i8<zx::i64>(zx::i64{-129}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i8<zx::i64>(zx::i64{128}, &overflow);
  zx::raw_assert(overflow);

  // cast to i16 overflow
  overflow = false;
  zx::cast_to_i16<zx::u16>(zx::u16{32768}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i16<zx::u32>(zx::u32{32768}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i16<zx::u64>(zx::u64{32768}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i16<zx::i32>(zx::i32{-32769}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i16<zx::i32>(zx::i32{32768}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i16<zx::i64>(zx::i64{-32769}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i16<zx::i64>(zx::i64{32768}, &overflow);
  zx::raw_assert(overflow);

  // cast to i32 overflow
  overflow = false;
  zx::cast_to_i32<zx::u32>(zx::u32{2147483648}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i32<zx::u64>(zx::u64{2147483648}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i32<zx::i64>(zx::i64{-2147483649__wb}, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  zx::cast_to_i32<zx::i64>(zx::i64{2147483648__wb}, &overflow);
  zx::raw_assert(overflow);

  // cast to i64 overflow
  overflow = false;
  zx::cast_to_i64<zx::u64>(zx::u64{9223372036854775808__wbu}, &overflow);
  zx::raw_assert(overflow);

  return 0;
}
