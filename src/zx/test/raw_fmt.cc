import zx;

static void basic_test() {  
  constexpr zx::u64 cap = zx::u64{1} << 12;
  zx::u8 arr[cap];
  __builtin_memset(arr, 0xaa, cap);

  zx::RawFmt fmt = zx::RawFmt::init(arr, cap);  

  for (zx::u64 i = 0; i < 2; i++) {
    bool overflow = false;
    fmt.push_i64(-1212, &overflow);
    zx::raw_assert(fmt.n_written() == 5);
    zx::raw_assert(!overflow);
    zx::raw_assert(0 == __builtin_memcmp(fmt.mem(), "-1212", 5));

    fmt.push_char('x', &overflow);
    zx::raw_assert(fmt.n_written() == 6);
    zx::raw_assert(!overflow);
    zx::raw_assert(0 == __builtin_memcmp(fmt.mem(), "-1212x", 6));

    fmt.push_i64(10, &overflow);
    zx::raw_assert(fmt.n_written() == 8);
    zx::raw_assert(!overflow);
    zx::raw_assert(0 == __builtin_memcmp(fmt.mem(), "-1212x10", 8));

    fmt.push_u64(1010, &overflow);
    zx::raw_assert(fmt.n_written() == 12);
    zx::raw_assert(!overflow);
    zx::raw_assert(0 == __builtin_memcmp(fmt.mem(), "-1212x101010", 12));

    fmt.push_i64(9'223'372'036'854'775'807__wb, &overflow);
    zx::raw_assert(fmt.n_written() == 31);
    zx::raw_assert(!overflow);
    zx::raw_assert(0 == __builtin_memcmp(fmt.mem(), "-1212x1010109223372036854775807", 31));

    fmt.push_i64(-9'223'372'036'854'775'808__wb, &overflow);
    zx::raw_assert(fmt.n_written() == 51);
    zx::raw_assert(!overflow);
    zx::raw_assert(0 == __builtin_memcmp(fmt.mem(),
                                         "-1212x1010109223372036854775807-9223372036854775808",
                                         51));

    fmt.push_i64(0__wb, &overflow);
    zx::raw_assert(fmt.n_written() == 52);
    zx::raw_assert(!overflow);
    zx::raw_assert(0 == __builtin_memcmp(fmt.mem(),
                                         "-1212x1010109223372036854775807-92233720368547758080",
                                         52));

    fmt.push_u64(0__wb, &overflow);
    zx::raw_assert(fmt.n_written() == 53);
    zx::raw_assert(!overflow);
    zx::raw_assert(0 == __builtin_memcmp(fmt.mem(),
                                         "-1212x1010109223372036854775807-922337203685477580800",
                                         53));

    fmt.push_u64(18'446'744'073'709'551'615__wb, &overflow);
    zx::raw_assert(fmt.n_written() == 73);
    zx::raw_assert(!overflow);
    zx::raw_assert(0 == __builtin_memcmp(
      fmt.mem(),
      "-1212x1010109223372036854775807-92233720368547758080018446744073709551615",
      73
    ));

    fmt.reset();
  }
}

static void empty_test() {
  zx::RawFmt fmt = zx::RawFmt::init((zx::u8*)alignof(zx::u8), 0);
  bool overflow = false;

  fmt.push_u64(1, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  fmt.push_i64(-1, &overflow);
  zx::raw_assert(overflow);

  overflow = false;
  fmt.push_char('x', &overflow);
  zx::raw_assert(overflow);
}

int main(void) {
  basic_test();
  empty_test();

  return 0;
}
