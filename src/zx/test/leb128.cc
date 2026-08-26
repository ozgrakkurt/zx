import zx;

static void testcase_unsigned(zx::u8 const *buf, zx::u64 buf_len, zx::u64 num) {
  zx::u64 out_num = zx::U64_MAX;
  zx::u64 offset = 0;
  zx::raw_assert(zx::leb128::DecodeStatus::Ok ==
                 zx::leb128::decode_u64(buf, buf_len, &offset, &out_num));
  zx::raw_assert(offset == buf_len);
  zx::raw_assert(out_num == num);

  zx::u8 out_buf[16];
  constexpr zx::u64 OUT_BUF_SIZE = sizeof(out_buf) / sizeof(out_buf[0]);
  __builtin_memset(out_buf, 0xaa, OUT_BUF_SIZE);

  offset = 0;
  zx::raw_assert(zx::leb128::EncodeStatus::Ok ==
                 zx::leb128::encode_u64(num, out_buf, OUT_BUF_SIZE, &offset));
  zx::raw_assert(offset == buf_len);
  zx::raw_assert(0 == __builtin_memcmp(out_buf, buf, offset));
}

static void testcase_signed(zx::u8 const *buf, zx::u64 buf_len, zx::i64 num) {
  zx::i64 out_num = zx::I64_MAX;
  zx::u64 offset = 0;
  zx::raw_assert(zx::leb128::DecodeStatus::Ok ==
                 zx::leb128::decode_i64(buf, buf_len, &offset, &out_num));
  zx::raw_assert(offset == buf_len);
  zx::raw_assert(out_num == num);

  zx::u8 out_buf[16];
  constexpr zx::u64 OUT_BUF_SIZE = sizeof(out_buf) / sizeof(out_buf[0]);
  __builtin_memset(out_buf, 0xaa, OUT_BUF_SIZE);

  offset = 0;
  zx::raw_assert(zx::leb128::EncodeStatus::Ok ==
                 zx::leb128::encode_i64(num, out_buf, OUT_BUF_SIZE, &offset));
  zx::raw_assert(offset == buf_len);
  zx::raw_assert(0 == __builtin_memcmp(out_buf, buf, offset));
}

static void test_unsigned() {
  testcase_unsigned((zx::u8[]){2__wbu}, 1, 2);
  testcase_unsigned((zx::u8[]){127__wbu}, 1, 127);
  testcase_unsigned((zx::u8[]){0x80__wbu, 1__wbu}, 2, 128);
  testcase_unsigned((zx::u8[]){1__wbu | 0x80__wbu, 1__wbu}, 2, 129);
  testcase_unsigned((zx::u8[]){57__wbu | 0x80__wbu, 100__wbu}, 2, 12857__wbu);
}

static void test_signed() {
  testcase_signed((zx::u8[]){2__wbu}, 1, 2);
  testcase_signed((zx::u8[]){0x7e__wbu}, 1, -2);
  testcase_signed((zx::u8[]){127__wbu | 0x80__wbu, 0}, 2, 127);
  testcase_signed((zx::u8[]){1__wbu | 0x80__wbu, 0x7f__wbu}, 2, -127);
  testcase_signed((zx::u8[]){0x80__wbu, 1__wbu}, 2, 128);
  testcase_signed((zx::u8[]){0x80__wbu, 0x7f__wbu}, 2, -128);
  testcase_signed((zx::u8[]){1__wbu | 0x80__wbu, 1__wbu}, 2, 129);
  testcase_signed((zx::u8[]){0x7f__wbu | 0x80__wbu, 0x7e__wbu}, 2, -129);
}

static void signed_roundtrip(zx::u8 *slice, zx::u64 len, zx::i64 i) {
  zx::u64 write_offset = 0;
  zx::raw_assert(zx::leb128::EncodeStatus::Ok ==
                 zx::leb128::encode_i64(i, slice, len, &write_offset));

  zx::u64 read_offset = 0;
  zx::i64 num = zx::I64_MAX;
  zx::raw_assert(zx::leb128::DecodeStatus::Ok ==
                 zx::leb128::decode_i64(slice, len, &read_offset, &num));

  zx::raw_assert(num == i);
  zx::raw_assert(read_offset == write_offset);
}

static void unsigned_roundtrip(zx::u8 *slice, zx::u64 len, zx::u64 i) {
  zx::u64 write_offset = 0;
  zx::raw_assert(zx::leb128::EncodeStatus::Ok ==
                 zx::leb128::encode_u64(i, slice, len, &write_offset));

  zx::u64 read_offset = 0;
  zx::u64 num = zx::U64_MAX;
  zx::raw_assert(zx::leb128::DecodeStatus::Ok ==
                 zx::leb128::decode_u64(slice, len, &read_offset, &num));

  zx::raw_assert(num == i);
  zx::raw_assert(read_offset == write_offset);
}

static void test_limits() {
  zx::u8 buf[15];
  constexpr zx::u64 SIZE = sizeof(buf) / sizeof(buf[0]);

  unsigned_roundtrip(buf, SIZE, zx::U64_MAX);
  unsigned_roundtrip(buf, SIZE, 0);
  unsigned_roundtrip(buf, SIZE, 1);

  signed_roundtrip(buf, SIZE, zx::I64_MAX);
  signed_roundtrip(buf, SIZE, zx::I64_MIN);
  signed_roundtrip(buf, SIZE, -1);
  signed_roundtrip(buf, SIZE, 1);
}

static void test_roundtrip() {
  zx::u8 buf[15];
  constexpr zx::u64 SIZE = sizeof(buf) / sizeof(buf[0]);

  for (zx::u64 i = 0; i < 1025; i++) {
    unsigned_roundtrip(buf, SIZE, i);
  }

  for (zx::i64 i = -1025; i < 1024; i++) {
    signed_roundtrip(buf, SIZE, i);
  }
}

static void test_read_with_overflow() {
  zx::u8 buf[] = {
      0xff__wbu, 0xff__wbu, 0xff__wbu,       0xff__wbu,
      0xff__wbu, 0xff__wbu, 0xff__wbu,       0xff__wbu,
      0xff__wbu, 0xff__wbu, 0b01111111__wbu, 0b11100100__wbu,
  };
  constexpr zx::u64 SIZE = sizeof(buf) / sizeof(buf[0]);

  zx::u64 offset = 0;
  zx::u64 num = 0;
  zx::raw_assert(zx::leb128::DecodeStatus::InvalidInput ==
                 zx::leb128::decode_u64(buf, SIZE, &offset, &num));

  offset = 0;
  zx::i64 inum = 0;
  zx::raw_assert(zx::leb128::DecodeStatus::InvalidInput ==
                 zx::leb128::decode_i64(buf, SIZE, &offset, &inum));
}

int main(void) {
  test_unsigned();
  test_signed();
  test_limits();
  test_read_with_overflow();
  test_roundtrip();

  return 0;
}
