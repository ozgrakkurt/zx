import zx;

int main(void) {
    // wrapping add
    zx::assert(1 == zx::wrapping_add_u8(zx::U8_MAX, 2));
    zx::assert(1 == zx::wrapping_add_u16(zx::U16_MAX, 2));
    zx::assert(1 == zx::wrapping_add_u32(zx::U32_MAX, 2));
    zx::assert(1 == zx::wrapping_add_u64(zx::U64_MAX, 2));

    zx::assert(69 == zx::wrapping_add_u8(65, 4));
    zx::assert(69 == zx::wrapping_add_u16(65, 4));
    zx::assert(69 == zx::wrapping_add_u32(65, 4));
    zx::assert(69 == zx::wrapping_add_u64(65, 4));

    zx::assert(zx::I8_MIN == zx::wrapping_add_i8(zx::I8_MAX, 1));
    zx::assert(zx::I16_MIN == zx::wrapping_add_i16(zx::I16_MAX, 1));
    zx::assert(zx::I32_MIN == zx::wrapping_add_i32(zx::I32_MAX, 1));
    zx::assert(zx::I64_MIN == zx::wrapping_add_i64(zx::I64_MAX, 1));

    zx::assert(-69 == zx::wrapping_add_i8(-65, -4));

    // wrapping sub
    zx::assert(1 == zx::wrapping_sub_u8(0, zx::U8_MAX));
    zx::assert(1 == zx::wrapping_sub_u16(0, zx::U16_MAX));
    zx::assert(1 == zx::wrapping_sub_u32(0, zx::U32_MAX));
    zx::assert(1 == zx::wrapping_sub_u64(0, zx::U64_MAX));

    zx::assert(61 == zx::wrapping_sub_u8(65, 4));
    zx::assert(61 == zx::wrapping_sub_u16(65, 4));
    zx::assert(61 == zx::wrapping_sub_u32(65, 4));
    zx::assert(61 == zx::wrapping_sub_u64(65, 4));

    zx::assert(zx::I8_MAX == zx::wrapping_sub_i8(zx::I8_MIN, 1));
    zx::assert(zx::I16_MAX == zx::wrapping_sub_i16(zx::I16_MIN, 1));
    zx::assert(zx::I32_MAX == zx::wrapping_sub_i32(zx::I32_MIN, 1));
    zx::assert(zx::I64_MAX == zx::wrapping_sub_i64(zx::I64_MIN, 1));

    zx::assert(-61 == zx::wrapping_sub_i8(-65, -4));

    // wrapping mul
    zx::assert(zx::U8_MAX - 2__wbu == zx::wrapping_mul_u8(zx::U8_MAX, 3));
    zx::assert(zx::U16_MAX - 2__wbu == zx::wrapping_mul_u16(zx::U16_MAX, 3));
    zx::assert(zx::U32_MAX - 2__wbu == zx::wrapping_mul_u32(zx::U32_MAX, 3));
    zx::assert(zx::U64_MAX - 2__wbu == zx::wrapping_mul_u64(zx::U64_MAX, 3));

    zx::assert(3 == zx::wrapping_mul_u8(1, 3));
    zx::assert(3 == zx::wrapping_mul_u16(1, 3));
    zx::assert(3 == zx::wrapping_mul_u32(1, 3));
    zx::assert(3 == zx::wrapping_mul_u64(1, 3));

    // saturating add
    zx::assert(zx::U8_MAX == zx::saturating_add_u8(zx::U8_MAX - 1, 250));
    zx::assert(zx::U16_MAX == zx::saturating_add_u16(zx::U16_MAX - 1, 250));
    zx::assert(zx::U32_MAX == zx::saturating_add_u32(zx::U32_MAX - 1, 250));
    zx::assert(zx::U64_MAX == zx::saturating_add_u64(zx::U64_MAX - 1, 250));
    zx::assert(2 == zx::saturating_add_u8(1, 1));
    zx::assert(2 == zx::saturating_add_u16(1, 1));
    zx::assert(2 == zx::saturating_add_u32(1, 1));
    zx::assert(2 == zx::saturating_add_u64(1, 1));

    zx::assert(zx::I8_MAX == zx::saturating_add_i8(zx::I8_MAX, 1));
    zx::assert(zx::I16_MAX == zx::saturating_add_i16(zx::I16_MAX, 1));
    zx::assert(zx::I32_MAX == zx::saturating_add_i32(zx::I32_MAX, 1));
    zx::assert(zx::I64_MAX == zx::saturating_add_i64(zx::I64_MAX, 1));

    zx::assert(zx::I8_MIN == zx::saturating_add_i8(zx::I8_MIN, -1));
    zx::assert(zx::I16_MIN == zx::saturating_add_i16(zx::I16_MIN, -1));
    zx::assert(zx::I32_MIN == zx::saturating_add_i32(zx::I32_MIN, -1));
    zx::assert(zx::I64_MIN == zx::saturating_add_i64(zx::I64_MIN, -1));

    // saturating sub
    zx::assert(0 == zx::saturating_sub_u8(1, 2));
    zx::assert(0 == zx::saturating_sub_u16(1, 2));
    zx::assert(0 == zx::saturating_sub_u32(1, 2));
    zx::assert(0 == zx::saturating_sub_u64(1, 2));

    zx::assert(zx::I8_MIN == zx::saturating_sub_i8(zx::I8_MIN, 1));
    zx::assert(zx::I16_MIN == zx::saturating_sub_i16(zx::I16_MIN, 1));
    zx::assert(zx::I32_MIN == zx::saturating_sub_i32(zx::I32_MIN, 1));
    zx::assert(zx::I64_MIN == zx::saturating_sub_i64(zx::I64_MIN, 1));

    zx::assert(zx::I8_MAX == zx::saturating_sub_i8(zx::I8_MAX, -1));
    zx::assert(zx::I16_MAX == zx::saturating_sub_i16(zx::I16_MAX, -1));
    zx::assert(zx::I32_MAX == zx::saturating_sub_i32(zx::I32_MAX, -1));
    zx::assert(zx::I64_MAX == zx::saturating_sub_i64(zx::I64_MAX, -1));

    // saturating mul
    zx::assert(zx::U8_MAX == zx::saturating_mul_u8(16, 16));
    zx::assert(zx::U16_MAX == zx::saturating_mul_u16(256, 256));
    zx::assert(zx::U32_MAX == zx::saturating_mul_u32(65536, 65536));
    zx::assert(zx::U64_MAX ==
               zx::saturating_mul_u64(4294967296__wbu, 4294967296__wbu));

    zx::assert(240 == zx::saturating_mul_u8(16, 15));
    zx::assert(69 == zx::saturating_mul_u8(69, 1));
    zx::assert(0 == zx::saturating_mul_u8(69, 0));
    zx::assert(69 == zx::saturating_mul_u64(69, 1));

    // add one over
    bool overflow = false;
    zx::add_u8(zx::U8_MAX, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_u16(zx::U16_MAX, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_u32(zx::U32_MAX, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_u64(zx::U64_MAX, 1, &overflow);
    zx::assert(overflow);

    // add no overflow
    overflow = false;
    zx::add_u8(69, 31, &overflow);
    zx::assert(!overflow);

    zx::assert(100 == zx::add_unsafe_u8(69, 31));

    overflow = false;
    zx::add_u16(69, 31, &overflow);
    zx::assert(!overflow);

    zx::assert(100 == zx::add_unsafe_u16(69, 31));

    overflow = false;
    zx::add_u32(69, 31, &overflow);
    zx::assert(!overflow);

    zx::assert(100 == zx::add_unsafe_u32(69, 31));

    overflow = false;
    zx::add_u64(69, 31, &overflow);
    zx::assert(!overflow);

    zx::assert(100 == zx::add_unsafe_u64(69, 31));

    // sub one over
    overflow = false;
    zx::sub_u8(0, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_u16(0, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_u32(0, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_u64(0, 1, &overflow);
    zx::assert(overflow);

    // sub no overflow
    overflow = false;
    zx::sub_u8(zx::U8_MAX, 69, &overflow);
    zx::assert(!overflow);

    zx::assert(zx::u8{0} - zx::u8{70} == zx::sub_unsafe_u8(zx::U8_MAX, 69));

    overflow = false;
    zx::sub_u16(zx::U16_MAX, 69, &overflow);
    zx::assert(!overflow);

    zx::assert(zx::u16{0} - zx::u16{70} == zx::sub_unsafe_u16(zx::U16_MAX, 69));

    overflow = false;
    zx::sub_u32(zx::U32_MAX, 69, &overflow);
    zx::assert(!overflow);

    zx::assert(zx::u32{0} - zx::u32{70} == zx::sub_unsafe_u32(zx::U32_MAX, 69));

    overflow = false;
    zx::sub_u64(zx::U64_MAX, 69, &overflow);
    zx::assert(!overflow);

    zx::assert(zx::u64{0} - zx::u64{70} == zx::sub_unsafe_u64(zx::U64_MAX, 69));

    // mul overflow
    overflow = false;
    zx::mul_u8(zx::U8_MAX / 2, 3, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_u16(zx::U16_MAX / 2, 3, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_u32(zx::U32_MAX / 2, 3, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_u64(zx::U64_MAX / 2, 3, &overflow);
    zx::assert(overflow);

    // mul no overflow
    overflow = false;
    zx::mul_u8(zx::U8_MAX / 2, 2, &overflow);
    zx::assert(!overflow);

    zx::assert(zx::U8_MAX - 1__wbu == zx::mul_unsafe_u8(zx::U8_MAX / 2, 2));

    overflow = false;
    zx::mul_u16(zx::U16_MAX / 2, 2, &overflow);
    zx::assert(!overflow);

    zx::assert(zx::U16_MAX - 1__wbu == zx::mul_unsafe_u16(zx::U16_MAX / 2, 2));

    overflow = false;
    zx::mul_u32(zx::U32_MAX / 2, 2, &overflow);
    zx::assert(!overflow);

    zx::assert(zx::U32_MAX - 1__wbu == zx::mul_unsafe_u32(zx::U32_MAX / 2, 2));

    overflow = false;
    zx::mul_u64(zx::U64_MAX / 2, 2, &overflow);
    zx::assert(!overflow);

    zx::assert(zx::U64_MAX - 1__wbu == zx::mul_unsafe_u64(zx::U64_MAX / 2, 2));

    // signed checked add overflow
    overflow = false;
    zx::add_i8(zx::I8_MAX, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_i16(zx::I16_MAX, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_i32(zx::I32_MAX, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_i64(zx::I64_MAX, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_i8(zx::I8_MIN, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_i16(zx::I16_MIN, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_i32(zx::I32_MIN, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::add_i64(zx::I64_MIN, -1, &overflow);
    zx::assert(overflow);

    // signed checked add no overflow
    overflow = false;
    zx::assert(zx::I8_MAX == zx::add_i8(zx::I8_MAX, 0, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(zx::I8_MIN == zx::add_i8(zx::I8_MIN, 0, &overflow));
    zx::assert(!overflow);

    // signed checked sub overflow
    overflow = false;
    zx::sub_i8(zx::I8_MAX, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_i16(zx::I16_MAX, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_i32(zx::I32_MAX, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_i64(zx::I64_MAX, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_i8(zx::I8_MIN, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_i16(zx::I16_MIN, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_i32(zx::I32_MIN, 1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::sub_i64(zx::I64_MIN, 1, &overflow);
    zx::assert(overflow);

    // signed checked sub no overflow
    overflow = false;
    zx::assert(zx::I8_MIN == zx::sub_i8(zx::I8_MIN, 0, &overflow));
    zx::assert(!overflow);

    // signed checked mul overflow
    overflow = false;
    zx::mul_i8(zx::I8_MIN, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_i16(zx::I16_MIN, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_i32(zx::I32_MIN, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_i64(zx::I64_MIN, -1, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_i8(zx::I8_MAX, 2, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_i16(zx::I16_MAX, 2, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_i32(zx::I32_MAX, 2, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mul_i64(zx::I64_MAX, 2, &overflow);
    zx::assert(overflow);

    // signed checked mul no overflow
    overflow = false;
    zx::assert(zx::I8_MIN == zx::mul_i8(zx::I8_MIN, 1, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(69 == zx::mul_i8(69, 1, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(-69 == zx::mul_i64(-69, 1, &overflow));
    zx::assert(!overflow);

    // div zero
    overflow = false;
    zx::div_u8(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::div_u16(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::div_u32(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::div_u64(69, 0, &overflow);
    zx::assert(overflow);

    // div nonzero
    overflow = false;
    zx::assert(0 == zx::div_u8(69, 113, &overflow));
    zx::assert(!overflow);

    zx::assert(0 == zx::div_unsafe_u8(69, 113));

    overflow = false;
    zx::assert(0 == zx::div_u16(69, 1131, &overflow));
    zx::assert(!overflow);

    zx::assert(0 == zx::div_unsafe_u16(69, 1131));

    overflow = false;
    zx::assert(0 == zx::div_u32(69, 1131, &overflow));
    zx::assert(!overflow);

    zx::assert(0 == zx::div_unsafe_u32(69, 1131));

    overflow = false;
    zx::assert(0 == zx::div_u64(69, 1131, &overflow));
    zx::assert(!overflow);

    zx::assert(0 == zx::div_unsafe_u64(69, 1131));

    // mod zero
    overflow = false;
    zx::mod_u8(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mod_u16(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mod_u32(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::mod_u64(69, 0, &overflow);
    zx::assert(overflow);

    // mod nonzero
    overflow = false;
    zx::mod_u8(69, 113, &overflow);
    zx::assert(!overflow);

    zx::assert(69__wbu == zx::mod_unsafe_u8(69, 113));

    overflow = false;
    zx::mod_u16(69, 1131, &overflow);
    zx::assert(!overflow);

    zx::assert(69__wbu == zx::mod_unsafe_u16(69, 113));

    overflow = false;
    zx::mod_u32(69, 1131, &overflow);
    zx::assert(!overflow);

    zx::assert(69__wbu == zx::mod_unsafe_u32(69, 113));

    overflow = false;
    zx::mod_u64(69, 1131, &overflow);
    zx::assert(!overflow);

    zx::assert(69__wbu == zx::mod_unsafe_u64(69, 113));

    // rand_mulshift_mod, result is always in [0, b) for b != 0. it is a
    // multiply-shift so it stays total for b == 0 and returns 0.
    zx::assert(0 == zx::rand_mulshift_mod_u32(69, 0));
    zx::assert(0 == zx::rand_mulshift_mod_u64(69, 0));

    zx::assert(0 == zx::rand_mulshift_mod_u32(69, 1));
    zx::assert(0 == zx::rand_mulshift_mod_u64(69, 1));

    // exact multiply-shift values
    zx::assert(0 == zx::rand_mulshift_mod_u32(0, 69));
    zx::assert(1 == zx::rand_mulshift_mod_u32(zx::U32_MAX, 2));
    zx::assert(0xFFFFFFFE__wbu ==
               zx::rand_mulshift_mod_u32(zx::U32_MAX, zx::U32_MAX));
    zx::assert(1 == zx::rand_mulshift_mod_u64(zx::U64_MAX, 2));

    // range sweep
    for (zx::u32 a = 0; a < 300; a++) {
        for (zx::u32 b = 1; b < 300; b++) {
            zx::assert(zx::rand_mulshift_mod_u32(a, b) < b);
        }
    }

    for (zx::u64 a = 0; a < 64; a++) {
        for (zx::u64 b = 1; b < 64; b++) {
            zx::assert(zx::rand_mulshift_mod_u64(a, b) < b);
        }
    }

    // div_ceil div by zero
    overflow = false;
    zx::div_ceil_u8(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::div_ceil_u16(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::div_ceil_u32(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::div_ceil_u64(69, 0, &overflow);
    zx::assert(overflow);

    // div_ceil b == 1
    overflow = false;
    zx::assert(zx::U8_MAX == zx::div_ceil_u8(zx::U8_MAX, 1, &overflow));
    zx::assert(!overflow);

    // div_ceil, a is an exact multiple of b at the top of the range. the true
    // result fits so the overflow flag must not be set.
    overflow = false;
    zx::assert(85 == zx::div_ceil_u8(zx::U8_MAX, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(21845 == zx::div_ceil_u16(zx::U16_MAX, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(1431655765 == zx::div_ceil_u32(zx::U32_MAX, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(6148914691236517205__wbu ==
               zx::div_ceil_u64(zx::U64_MAX, 3, &overflow));
    zx::assert(!overflow);

    // div_ceil, the intermediate a + b would overflow but the true result
    // fits. the flag must not be set and the value must be exact.
    overflow = false;
    zx::assert(128 == zx::div_ceil_u8(zx::U8_MAX, 2, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(32768 == zx::div_ceil_u16(zx::U16_MAX, 2, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(0x80000000__wbu == zx::div_ceil_u32(zx::U32_MAX, 2, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(0x8000000000000000__wbu ==
               zx::div_ceil_u64(zx::U64_MAX, 2, &overflow));
    zx::assert(!overflow);

    // unsafe variants, no intermediate wrapping
    zx::assert(128 == zx::div_ceil_unsafe_u8(zx::U8_MAX, 2));
    zx::assert(0x8000000000000000__wbu ==
               zx::div_ceil_unsafe_u64(zx::U64_MAX, 2));

    // div_ceil no overflow
    overflow = false;
    zx::assert(1 == zx::div_ceil_u8(1, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(2 == zx::div_ceil_u8(4, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(1 == zx::div_ceil_u16(1, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(2 == zx::div_ceil_u16(4, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(1 == zx::div_ceil_u32(1, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(2 == zx::div_ceil_u32(4, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(1 == zx::div_ceil_u64(1, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(2 == zx::div_ceil_u64(4, 3, &overflow));
    zx::assert(!overflow);

    // next_mult div by zero
    overflow = false;
    zx::next_multiple_of_u8(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::next_multiple_of_u16(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::next_multiple_of_u32(69, 0, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::next_multiple_of_u64(69, 0, &overflow);
    zx::assert(overflow);

    // next_multiple_of, a is already a multiple of b at the top of the
    // range. the result is a itself so the overflow flag must not be set.
    overflow = false;
    zx::assert(zx::U8_MAX == zx::next_multiple_of_u8(zx::U8_MAX, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(zx::U16_MAX ==
               zx::next_multiple_of_u16(zx::U16_MAX, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(zx::U32_MAX ==
               zx::next_multiple_of_u32(zx::U32_MAX, 3, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(zx::U64_MAX ==
               zx::next_multiple_of_u64(zx::U64_MAX, 3, &overflow));
    zx::assert(!overflow);

    // next_multiple_of, a is already a multiple of b even though the
    // intermediate a + b would overflow. the result is a itself, flag must not
    // be set.
    overflow = false;
    zx::assert(252 == zx::next_multiple_of_u8(252, 4, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(65532 == zx::next_multiple_of_u16(65532, 4, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert((zx::U32_MAX - 3__wbu) ==
               zx::next_multiple_of_u32(zx::U32_MAX - 3__wbu, 4, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert((zx::U64_MAX - 3__wbu) ==
               zx::next_multiple_of_u64(zx::U64_MAX - 3__wbu, 4, &overflow));
    zx::assert(!overflow);

    // unsafe variants, no intermediate wrapping
    zx::assert(252 == zx::next_multiple_of_unsafe_u8(252, 4));
    zx::assert((zx::U64_MAX - 3__wbu) ==
               zx::next_multiple_of_unsafe_u64(zx::U64_MAX - 3__wbu, 4));

    // next_mult genuine overflow, the next multiple of b doesn't fit the type
    overflow = false;
    zx::next_multiple_of_u8(255, 4, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::next_multiple_of_u64(zx::U64_MAX - 2__wbu, 4, &overflow);
    zx::assert(overflow);

    // next_mult no overflow
    overflow = false;
    zx::assert(3 == zx::next_multiple_of_u8(1, 3, &overflow));
    zx::assert(!overflow);

    zx::assert(3 == zx::next_multiple_of_unsafe_u8(1, 3));

    overflow = false;
    zx::assert(3 == zx::next_multiple_of_u16(1, 3, &overflow));
    zx::assert(!overflow);

    zx::assert(3 == zx::next_multiple_of_unsafe_u16(1, 3));

    overflow = false;
    zx::assert(3 == zx::next_multiple_of_u32(1, 3, &overflow));
    zx::assert(!overflow);

    zx::assert(3 == zx::next_multiple_of_unsafe_u32(1, 3));

    overflow = false;
    zx::assert(3 == zx::next_multiple_of_u64(1, 3, &overflow));
    zx::assert(!overflow);

    zx::assert(3 == zx::next_multiple_of_unsafe_u64(1, 3));

    // align_up
    overflow = false;
    zx::assert(8 == zx::align_up_u64(5, 8, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(0 == zx::align_up_u64(0, 8, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(8 == zx::align_up_u64(8, 8, &overflow));
    zx::assert(!overflow);

    // align_up, val is already aligned at the top of the range. the result is
    // val itself so the overflow flag must not be set.
    overflow = false;
    zx::assert((zx::U64_MAX - 7__wbu) ==
               zx::align_up_u64(zx::U64_MAX - 7__wbu, 8, &overflow));
    zx::assert(!overflow);

    // align_up genuine overflow, the next multiple of 8 doesn't fit
    overflow = false;
    zx::align_up_u64(zx::U64_MAX - 6__wbu, 8, &overflow);
    zx::assert(overflow);

    // align_up invalid alignment, must set the flag
    overflow = false;
    zx::align_up_u64(69, 3, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::align_up_u64(69, 0, &overflow);
    zx::assert(overflow);

    // align_down
    overflow = false;
    zx::assert((zx::U64_MAX - 7__wbu) ==
               zx::align_down_u64(zx::U64_MAX, 8, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(0 == zx::align_down_u64(7, 8, &overflow));
    zx::assert(!overflow);

    // align_down invalid alignment, must set the flag. the return value is
    // garbage by contract on this path so only the flag is asserted.
    overflow = false;
    zx::align_down_u64(69, 3, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::align_down_u64(69, 0, &overflow);
    zx::assert(overflow);

    // align_up_unsafe
    zx::assert(8 == zx::align_up_unsafe_u64(5, 8));
    zx::assert(0 == zx::align_up_unsafe_u64(0, 8));
    zx::assert(8 == zx::align_up_unsafe_u64(8, 8));
    zx::assert(64 == zx::align_up_unsafe_u64(33, 64));

    // already aligned at the top of the range, no intermediate wrapping
    zx::assert((zx::U64_MAX - 7__wbu) ==
               zx::align_up_unsafe_u64(zx::U64_MAX - 7__wbu, 8));

    // align_down_unsafe
    zx::assert((zx::U64_MAX - 7__wbu) ==
               zx::align_down_unsafe_u64(zx::U64_MAX, 8));
    zx::assert(0 == zx::align_down_unsafe_u64(7, 8));
    zx::assert(69 == zx::align_down_unsafe_u64(69, 1));
    zx::assert(64 == zx::align_down_unsafe_u64(69, 64));

    // min
    zx::assert(31 == zx::min_u8(31, 69));
    zx::assert(31 == zx::min_u8(69, 31));

    zx::assert(31 == zx::min_u16(31, 69));
    zx::assert(31 == zx::min_u16(69, 31));

    zx::assert(31 == zx::min_u32(31, 69));
    zx::assert(31 == zx::min_u32(69, 31));

    zx::assert(31 == zx::min_u64(31, 69));
    zx::assert(31 == zx::min_u64(69, 31));

    // max
    zx::assert(69 == zx::max_u8(31, 69));
    zx::assert(69 == zx::max_u8(69, 31));

    zx::assert(69 == zx::max_u16(31, 69));
    zx::assert(69 == zx::max_u16(69, 31));

    zx::assert(69 == zx::max_u32(31, 69));
    zx::assert(69 == zx::max_u32(69, 31));

    zx::assert(69 == zx::max_u64(31, 69));
    zx::assert(69 == zx::max_u64(69, 31));

    zx::assert(zx::I8_MIN == zx::min_i8(zx::I8_MIN, zx::I8_MAX));
    zx::assert(zx::I8_MIN == zx::min_i8(zx::I8_MAX, zx::I8_MIN));
    zx::assert(zx::I16_MIN == zx::min_i16(zx::I16_MIN, zx::I16_MAX));
    zx::assert(zx::I16_MIN == zx::min_i16(zx::I16_MAX, zx::I16_MIN));
    zx::assert(zx::I32_MIN == zx::min_i32(zx::I32_MIN, zx::I32_MAX));
    zx::assert(zx::I32_MIN == zx::min_i32(zx::I32_MAX, zx::I32_MIN));
    zx::assert(zx::I64_MIN == zx::min_i64(zx::I64_MIN, zx::I64_MAX));
    zx::assert(zx::I64_MIN == zx::min_i64(zx::I64_MAX, zx::I64_MIN));

    zx::assert(-31 == zx::min_i8(-31, 69));
    zx::assert(-31 == zx::min_i64(69, -31));

    zx::assert(zx::I8_MAX == zx::max_i8(zx::I8_MIN, zx::I8_MAX));
    zx::assert(zx::I8_MAX == zx::max_i8(zx::I8_MAX, zx::I8_MIN));
    zx::assert(zx::I16_MAX == zx::max_i16(zx::I16_MIN, zx::I16_MAX));
    zx::assert(zx::I16_MAX == zx::max_i16(zx::I16_MAX, zx::I16_MIN));
    zx::assert(zx::I32_MAX == zx::max_i32(zx::I32_MIN, zx::I32_MAX));
    zx::assert(zx::I32_MAX == zx::max_i32(zx::I32_MAX, zx::I32_MIN));
    zx::assert(zx::I64_MAX == zx::max_i64(zx::I64_MIN, zx::I64_MAX));
    zx::assert(zx::I64_MAX == zx::max_i64(zx::I64_MAX, zx::I64_MIN));

    // shr UB
    bool fail = false;
    zx::shr_u8(1, 8, &fail);
    zx::assert(fail);

    fail = false;
    zx::shr_u16(0, 16, &fail);
    zx::assert(fail);

    fail = false;
    zx::shr_u32(1, 32, &fail);
    zx::assert(fail);

    fail = false;
    zx::shr_u64(0, 64, &fail);
    zx::assert(fail);

    // shr UB more
    fail = false;
    zx::shr_u8(1, 20, &fail);
    zx::assert(fail);

    fail = false;
    zx::shr_u16(1, 30, &fail);
    zx::assert(fail);

    fail = false;
    zx::shr_u32(1, 60, &fail);
    zx::assert(fail);

    fail = false;
    zx::shr_u64(1, zx::U8_MAX, &fail);
    zx::assert(fail);

    // shr no UB
    fail = false;
    zx::shr_u8(0, 1, &fail);
    zx::assert(!fail);

    zx::assert(0 == zx::shr_unsafe_u8(0, 1));
    zx::assert(1 == zx::shr_unsafe_u8(4, 2));

    fail = false;
    zx::shr_u16(0, 1, &fail);
    zx::assert(!fail);

    zx::assert(0 == zx::shr_unsafe_u16(0, 1));
    zx::assert(1 == zx::shr_unsafe_u16(4, 2));

    fail = false;
    zx::shr_u32(0, 1, &fail);
    zx::assert(!fail);

    zx::assert(0 == zx::shr_unsafe_u32(0, 1));
    zx::assert(1 == zx::shr_unsafe_u32(4, 2));

    fail = false;
    zx::shr_u64(0, 1, &fail);
    zx::assert(!fail);

    zx::assert(0 == zx::shr_unsafe_u64(0, 1));
    zx::assert(1 == zx::shr_unsafe_u64(4, 2));

    // shl UB
    fail = false;
    zx::shl_u8(1, 8, &fail);
    zx::assert(fail);

    fail = false;
    zx::shl_u16(0, 16, &fail);
    zx::assert(fail);

    fail = false;
    zx::shl_u32(1, 32, &fail);
    zx::assert(fail);

    fail = false;
    zx::shl_u64(0, 64, &fail);
    zx::assert(fail);

    // shl UB more
    fail = false;
    zx::shl_u8(1, 20, &fail);
    zx::assert(fail);

    fail = false;
    zx::shl_u16(1, 30, &fail);
    zx::assert(fail);

    fail = false;
    zx::shl_u32(1, 60, &fail);
    zx::assert(fail);

    fail = false;
    zx::shl_u64(1, zx::U8_MAX, &fail);
    zx::assert(fail);

    // shl no UB
    fail = false;
    zx::shl_u8(0, 1, &fail);
    zx::assert(!fail);

    zx::assert(0 == zx::shl_unsafe_u8(0, 1));
    zx::assert(4 == zx::shl_unsafe_u8(1, 2));

    fail = false;
    zx::shl_u16(0, 1, &fail);
    zx::assert(!fail);

    zx::assert(0 == zx::shl_unsafe_u16(0, 1));
    zx::assert(4 == zx::shl_unsafe_u16(1, 2));

    fail = false;
    zx::shl_u32(0, 1, &fail);
    zx::assert(!fail);

    zx::assert(0 == zx::shl_unsafe_u32(0, 1));
    zx::assert(4 == zx::shl_unsafe_u32(1, 2));

    fail = false;
    zx::shl_u64(0, 1, &fail);
    zx::assert(!fail);

    zx::assert(0 == zx::shl_unsafe_u64(0, 1));
    zx::assert(4 == zx::shl_unsafe_u64(1, 2));

    // ctz
    zx::assert(3 == zx::ctz_u8(0b01111000));
    zx::assert(3 == zx::ctz_u16(0b01111000));
    zx::assert(3 == zx::ctz_u32(0b01111000));
    zx::assert(3 == zx::ctz_u64(0b01111000));
    zx::assert(8 == zx::ctz_u8(0));
    zx::assert(16 == zx::ctz_u16(0));
    zx::assert(32 == zx::ctz_u32(0));
    zx::assert(64 == zx::ctz_u64(0));

    // clz
    zx::assert(1 == zx::clz_u8(0b01111000));
    zx::assert(9 == zx::clz_u16(0b01111000));
    zx::assert(25 == zx::clz_u32(0b01111000));
    zx::assert(57 == zx::clz_u64(0b01111000));
    zx::assert(8 == zx::clz_u8(0));
    zx::assert(16 == zx::clz_u16(0));
    zx::assert(32 == zx::clz_u32(0));
    zx::assert(64 == zx::clz_u64(0));

    // popcount
    zx::assert(4 == zx::popcount_u8(0b01111000));
    zx::assert(4 == zx::popcount_u16(0b01111000));
    zx::assert(4 == zx::popcount_u32(0b01111000));
    zx::assert(4 == zx::popcount_u64(0b01111000));
    zx::assert(0 == zx::popcount_u8(0));
    zx::assert(0 == zx::popcount_u16(0));
    zx::assert(0 == zx::popcount_u32(0));
    zx::assert(0 == zx::popcount_u64(0));
    zx::assert(8 == zx::popcount_u8(zx::U8_MAX));
    zx::assert(16 == zx::popcount_u16(zx::U16_MAX));
    zx::assert(32 == zx::popcount_u32(zx::U32_MAX));
    zx::assert(64 == zx::popcount_u64(zx::U64_MAX));

    // bit_width
    zx::assert(7 == zx::bit_width_u8(0b01111000));
    zx::assert(7 == zx::bit_width_u16(0b01111000));
    zx::assert(7 == zx::bit_width_u32(0b01111000));
    zx::assert(7 == zx::bit_width_u64(0b01111000));
    zx::assert(0 == zx::bit_width_u8(0));
    zx::assert(0 == zx::bit_width_u16(0));
    zx::assert(0 == zx::bit_width_u32(0));
    zx::assert(0 == zx::bit_width_u64(0));
    zx::assert(8 == zx::bit_width_u8(zx::U8_MAX));
    zx::assert(16 == zx::bit_width_u16(zx::U16_MAX));
    zx::assert(32 == zx::bit_width_u32(zx::U32_MAX));
    zx::assert(64 == zx::bit_width_u64(zx::U64_MAX));

    // next_pow_two
    overflow = false;
    zx::assert(128 == zx::next_pow_two_u8(100, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(128 == zx::next_pow_two_u16(100, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(128 == zx::next_pow_two_u32(100, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(128 == zx::next_pow_two_u64(100, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(1 == zx::next_pow_two_u8(0, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(1 == zx::next_pow_two_u16(0, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(1 == zx::next_pow_two_u32(0, &overflow));
    zx::assert(!overflow);

    overflow = false;
    zx::assert(1 == zx::next_pow_two_u64(0, &overflow));
    zx::assert(!overflow);

    // next_pow_two overflow
    overflow = false;
    zx::next_pow_two_u8(zx::U8_MAX, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::next_pow_two_u16(zx::U16_MAX, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::next_pow_two_u32(zx::U32_MAX, &overflow);
    zx::assert(overflow);

    overflow = false;
    zx::next_pow_two_u64(zx::U64_MAX, &overflow);
    zx::assert(overflow);

    // is_pow_two
    zx::assert(zx::is_pow_two_u8(1));
    zx::assert(zx::is_pow_two_u16(1));
    zx::assert(zx::is_pow_two_u32(1));
    zx::assert(zx::is_pow_two_u64(1));

    zx::assert(!zx::is_pow_two_u8(0));
    zx::assert(!zx::is_pow_two_u16(0));
    zx::assert(!zx::is_pow_two_u32(0));
    zx::assert(!zx::is_pow_two_u64(0));

    // inverse
    zx::assert(zx::u8{123} + zx::inverse_u8(123) == 0);
    zx::assert(zx::u16{123} + zx::inverse_u16(123) == 0);
    zx::assert(zx::u32{123} + zx::inverse_u32(123) == 0);
    zx::assert(zx::u64{123} + zx::inverse_u64(123) == 0);

    // cast to u8
    overflow = false;
    // u16 -> u8
    zx::assert(69 == zx::u16_to_u8(zx::u16{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::u16_to_u8_unsafe(zx::u16{69}));

    // u32 -> u8
    zx::assert(69 == zx::u32_to_u8(zx::u32{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::u32_to_u8_unsafe(zx::u32{69}));

    // u64 -> u8
    zx::assert(69 == zx::u64_to_u8(zx::u64{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::u64_to_u8_unsafe(zx::u64{69}));

    // i8 -> u8
    zx::assert(69 == zx::i8_to_u8(zx::i8{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::i8_to_u8_unsafe(zx::i8{69}));

    // i16 -> u8
    zx::assert(69 == zx::i16_to_u8(zx::i16{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::i16_to_u8_unsafe(zx::i16{69}));

    // i32 -> u8
    zx::assert(69 == zx::i32_to_u8(zx::i32{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::i32_to_u8_unsafe(zx::i32{69}));

    // i64 -> u8
    zx::assert(69 == zx::i64_to_u8(zx::i64{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::i64_to_u8_unsafe(zx::i64{69}));

    // cast to u16
    overflow = false;
    // u32 -> u16
    zx::assert(1100 == zx::u32_to_u16(zx::u32{1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(1100 == zx::u32_to_u16_unsafe(zx::u32{1100}));

    // u64 -> u16
    zx::assert(1100 == zx::u64_to_u16(zx::u64{1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(1100 == zx::u64_to_u16_unsafe(zx::u64{1100}));

    // i16 -> u16
    zx::assert(1100 == zx::i16_to_u16(zx::i16{1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(1100 == zx::i16_to_u16_unsafe(zx::i16{1100}));

    // i32 -> u16
    zx::assert(1100 == zx::i32_to_u16(zx::i32{1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(1100 == zx::i32_to_u16_unsafe(zx::i32{1100}));

    // i64 -> u16
    zx::assert(1100 == zx::i64_to_u16(zx::i64{1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(1100 == zx::i64_to_u16_unsafe(zx::i64{1100}));

    // cast to u32
    overflow = false;
    // u64 -> u32
    zx::assert(zx::u64{zx::U16_MAX} + 1__wbu ==
               zx::u64_to_u32(zx::u64{zx::U16_MAX} + 1__wbu, &overflow));
    zx::assert(!overflow);
    zx::assert(zx::u64{zx::U16_MAX} + 1__wbu ==
               zx::u64_to_u32_unsafe(zx::u64{zx::U16_MAX} + 1__wbu));

    // i32 -> u32
    zx::assert(zx::i32{zx::U16_MAX} + 1__wb ==
               zx::i32_to_u32(zx::i32{zx::U16_MAX} + 1__wb, &overflow));
    zx::assert(!overflow);
    zx::assert(zx::i32{zx::U16_MAX} + 1__wb ==
               zx::i32_to_u32_unsafe(zx::i32{zx::U16_MAX} + 1__wb));

    // i64 -> u32
    zx::assert(zx::U16_MAX + 1 ==
               zx::i64_to_u32(zx::i64{zx::U16_MAX + 1}, &overflow));
    zx::assert(!overflow);
    zx::assert(zx::i64{zx::U16_MAX} + 1__wb ==
               zx::i64_to_u32_unsafe(zx::i64{zx::U16_MAX} + 1__wb));

    // cast to u64
    overflow = false;
    // i64 -> u64
    zx::assert(zx::U32_MAX + zx::u64{1} ==
               zx::i64_to_u64(zx::i64{zx::U32_MAX + zx::u64{1}}, &overflow));
    zx::assert(!overflow);
    zx::assert(zx::i64{zx::U16_MAX} + 1__wb ==
               zx::i64_to_u64_unsafe(zx::i64{zx::U16_MAX} + 1__wb));

    // cast to i8
    overflow = false;
    // u8 -> i8
    zx::assert(69 == zx::u8_to_i8(zx::u8{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::u8_to_i8_unsafe(zx::u8{69}));

    // u16 -> i8
    zx::assert(69 == zx::u16_to_i8(zx::u16{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::u16_to_i8_unsafe(zx::u16{69}));

    // u32 -> i8
    zx::assert(69 == zx::u32_to_i8(zx::u32{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::u32_to_i8_unsafe(zx::u32{69}));

    // u64 -> i8
    zx::assert(69 == zx::u64_to_i8(zx::u64{69}, &overflow));
    zx::assert(!overflow);
    zx::assert(69 == zx::u64_to_i8_unsafe(zx::u64{69}));

    // i16 -> i8
    zx::assert(-69 == zx::i16_to_i8(zx::i16{-69}, &overflow));
    zx::assert(!overflow);
    zx::assert(-69 == zx::i16_to_i8_unsafe(zx::i16{-69}));

    // i32 -> i8
    zx::assert(-69 == zx::i32_to_i8(zx::i32{-69}, &overflow));
    zx::assert(!overflow);
    zx::assert(-69 == zx::i32_to_i8_unsafe(zx::i32{-69}));

    // i64 -> i8
    zx::assert(-69 == zx::i64_to_i8(zx::i64{-69}, &overflow));
    zx::assert(!overflow);
    zx::assert(-69 == zx::i64_to_i8_unsafe(zx::i64{-69}));

    // cast to i16
    overflow = false;
    // u16 -> i16
    zx::assert(1100 == zx::u16_to_i16(zx::u16{1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(1100 == zx::u16_to_i16_unsafe(zx::u16{1100}));

    // u32 -> i16
    zx::assert(1100 == zx::u32_to_i16(zx::u32{1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(1100 == zx::u32_to_i16_unsafe(zx::u32{1100}));

    // u64 -> i16
    zx::assert(1100 == zx::u64_to_i16(zx::u64{1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(1100 == zx::u64_to_i16_unsafe(zx::u64{1100}));

    // i32 -> i16
    zx::assert(-1100 == zx::i32_to_i16(zx::i32{-1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(-1100 == zx::i32_to_i16_unsafe(zx::i32{-1100}));

    // i64 -> i16
    zx::assert(-1100 == zx::i64_to_i16(zx::i64{-1100}, &overflow));
    zx::assert(!overflow);
    zx::assert(-1100 == zx::i64_to_i16_unsafe(zx::i64{-1100}));

    // cast to i32
    overflow = false;
    // u32 -> i32
    zx::assert(zx::u32{zx::U16_MAX} + 1__wbu ==
               zx::u32_to_i32(zx::u32{zx::U16_MAX} + 1__wbu, &overflow));
    zx::assert(!overflow);
    zx::assert(zx::u32{zx::U16_MAX} + 1__wbu ==
               zx::u32_to_i32_unsafe(zx::u32{zx::U16_MAX} + 1__wbu));

    // u64 -> i32
    zx::assert(zx::u64{zx::U16_MAX} + 1__wbu ==
               zx::u64_to_i32(zx::u64{zx::U16_MAX} + 1__wbu, &overflow));
    zx::assert(!overflow);
    zx::assert(zx::u64{zx::U16_MAX} + 1__wbu ==
               zx::u64_to_i32_unsafe(zx::u64{zx::U16_MAX} + 1__wbu));

    // i64 -> i32
    zx::assert(-65537 == zx::i64_to_i32(zx::i64{-65537}, &overflow));
    zx::assert(!overflow);
    zx::assert(-65537 == zx::i64_to_i32_unsafe(zx::i64{-65537}));

    // cast to i64
    overflow = false;
    // u64 -> i64
    zx::assert(zx::u64{zx::U32_MAX} + 1__wbu ==
               zx::u64_to_i64(zx::u64{zx::U32_MAX} + 1__wbu, &overflow));
    zx::assert(!overflow);
    zx::assert(zx::u64{zx::U32_MAX} + 1__wbu ==
               zx::u64_to_i64_unsafe(zx::u64{zx::U32_MAX} + 1__wbu));

    // overflow tests (safe variants only)
    // cast to u8 overflow
    overflow = false;
    zx::u16_to_u8(zx::u16{300}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::u32_to_u8(zx::u32{300}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::u64_to_u8(zx::u64{300}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i8_to_u8(zx::i8{-1}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i16_to_u8(zx::i16{-1}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i16_to_u8(zx::i16{300}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i32_to_u8(zx::i32{-1}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i32_to_u8(zx::i32{300}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_u8(zx::i64{-1}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_u8(zx::i64{300}, &overflow);
    zx::assert(overflow);

    // cast to u16 overflow
    overflow = false;
    zx::u32_to_u16(zx::u32{70000}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::u64_to_u16(zx::u64{70000}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i16_to_u16(zx::i16{-1}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i32_to_u16(zx::i32{-1}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i32_to_u16(zx::i32{70000}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_u16(zx::i64{-1}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_u16(zx::i64{70000}, &overflow);
    zx::assert(overflow);

    // cast to u32 overflow
    overflow = false;
    zx::u64_to_u32(zx::u64{zx::U32_MAX + zx::u64{1}}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i32_to_u32(zx::i32{-1}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_u32(zx::i64{-1}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_u32(zx::i64{zx::U32_MAX + zx::i64{1}}, &overflow);
    zx::assert(overflow);

    // cast to u64 overflow
    overflow = false;
    zx::i64_to_u64(zx::i64{-1}, &overflow);
    zx::assert(overflow);

    // cast to i8 overflow
    overflow = false;
    zx::u8_to_i8(zx::u8{128}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::u16_to_i8(zx::u16{128}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::u32_to_i8(zx::u32{128}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::u64_to_i8(zx::u64{128}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i16_to_i8(zx::i16{-129}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i16_to_i8(zx::i16{128}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i32_to_i8(zx::i32{-129}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i32_to_i8(zx::i32{128}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_i8(zx::i64{-129}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_i8(zx::i64{128}, &overflow);
    zx::assert(overflow);

    // cast to i16 overflow
    overflow = false;
    zx::u16_to_i16(zx::u16{32768}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::u32_to_i16(zx::u32{32768}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::u64_to_i16(zx::u64{32768}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i32_to_i16(zx::i32{-32769}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i32_to_i16(zx::i32{32768}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_i16(zx::i64{-32769}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_i16(zx::i64{32768}, &overflow);
    zx::assert(overflow);

    // cast to i32 overflow
    overflow = false;
    zx::u32_to_i32(zx::u32{2147483648}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::u64_to_i32(zx::u64{2147483648}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_i32(zx::i64{-2147483649__wb}, &overflow);
    zx::assert(overflow);
    overflow = false;
    zx::i64_to_i32(zx::i64{2147483648__wb}, &overflow);
    zx::assert(overflow);

    // cast to i64 overflow
    overflow = false;
    zx::u64_to_i64(zx::u64{9223372036854775808__wbu}, &overflow);
    zx::assert(overflow);

    return 0;
}
