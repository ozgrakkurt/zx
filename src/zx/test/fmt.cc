import zx;

static void basic_test() {
    constexpr zx::u64 cap = zx::u64{1} << 12;
    zx::u8 arr[cap];
    __builtin_memset(arr, 0xaa, cap);

    zx::SliceMut<zx::u8> slice = zx::SliceMut<zx::u8>::init_unsafe(arr, cap);

    zx::Fmt fmt = zx::Fmt::init(slice);

    for (zx::u64 i = 0; i < 2; i++) {
        bool overflow = false;
        fmt.push_i64(-1212, &overflow);
        zx::assert(fmt.n_written() == 5);
        zx::assert(!overflow);
        zx::assert(0 == __builtin_memcmp(fmt.mem().ptr(), "-1212", 5));

        fmt.push_char('x', &overflow);
        zx::assert(fmt.n_written() == 6);
        zx::assert(!overflow);
        zx::assert(0 == __builtin_memcmp(fmt.mem().ptr(), "-1212x", 6));

        fmt.push_i64(10, &overflow);
        zx::assert(fmt.n_written() == 8);
        zx::assert(!overflow);
        zx::assert(0 == __builtin_memcmp(fmt.mem().ptr(), "-1212x10", 8));

        fmt.push_u64(1010, &overflow);
        zx::assert(fmt.n_written() == 12);
        zx::assert(!overflow);
        zx::assert(0 == __builtin_memcmp(fmt.mem().ptr(), "-1212x101010", 12));

        fmt.push_i64(9'223'372'036'854'775'807__wb, &overflow);
        zx::assert(fmt.n_written() == 31);
        zx::assert(!overflow);
        zx::assert(0 == __builtin_memcmp(fmt.mem().ptr(),
                                         "-1212x1010109223372036854775807",
                                         31));

        fmt.push_i64(-9'223'372'036'854'775'808__wb, &overflow);
        zx::assert(fmt.n_written() == 51);
        zx::assert(!overflow);
        zx::assert(0 ==
                   __builtin_memcmp(
                       fmt.mem().ptr(),
                       "-1212x1010109223372036854775807-9223372036854775808",
                       51));

        fmt.push_i64(0__wb, &overflow);
        zx::assert(fmt.n_written() == 52);
        zx::assert(!overflow);
        zx::assert(0 ==
                   __builtin_memcmp(
                       fmt.mem().ptr(),
                       "-1212x1010109223372036854775807-92233720368547758080",
                       52));

        fmt.push_u64(0__wb, &overflow);
        zx::assert(fmt.n_written() == 53);
        zx::assert(!overflow);
        zx::assert(0 ==
                   __builtin_memcmp(
                       fmt.mem().ptr(),
                       "-1212x1010109223372036854775807-922337203685477580800",
                       53));

        fmt.push_u64(18'446'744'073'709'551'615__wb, &overflow);
        zx::assert(fmt.n_written() == 73);
        zx::assert(!overflow);
        zx::assert(0 ==
                   __builtin_memcmp(fmt.mem().ptr(),
                                    "-1212x1010109223372036854775807-"
                                    "92233720368547758080018446744073709551615",
                                    73));

        fmt.push_u64_hex(69, &overflow);
        zx::assert(fmt.n_written() == 77);
        zx::assert(0 == __builtin_memcmp(
                            fmt.mem().ptr(),
                            "-1212x1010109223372036854775807-"
                            "922337203685477580800184467440737095516150x45",
                            77));

        fmt.push_u64_hex(0, &overflow);
        zx::assert(fmt.n_written() == 80);
        zx::assert(0 == __builtin_memcmp(
                            fmt.mem().ptr(),
                            "-1212x1010109223372036854775807-"
                            "922337203685477580800184467440737095516150x450x0",
                            80));

        fmt.reset();
    }
}

static void empty_test() {
    zx::Fmt fmt = zx::Fmt::init(zx::SliceMut<zx::u8>::init_dangling());
    bool overflow = false;

    fmt.push_u64(1, &overflow);
    zx::assert(overflow);

    overflow = false;
    fmt.push_i64(-1, &overflow);
    zx::assert(overflow);

    overflow = false;
    fmt.push_char('x', &overflow);
    zx::assert(overflow);
}

static void overlap_test() {
    zx::u8 arr[16];
    __builtin_memset(arr, 0xaa, sizeof(arr));

    zx::Fmt fmt =
        zx::Fmt::init(zx::SliceMut<zx::u8>::init_unsafe(arr, sizeof(arr)));

    // The source and destination are the same range [0, 8), so the copy must
    // be rejected.
    bool fail = false;
    fmt.push_str(zx::Slice<zx::u8>::init_unsafe(arr, 8), &fail);
    zx::assert(fail);
    zx::assert(fmt.n_written() == 0);
    for (zx::u64 i = 0; i < sizeof(arr); i++) {
        zx::assert(arr[i] == 0xaa);
    }

    // Write a prefix from a non-overlapping source. This must not set fail.
    zx::u8 const prefix[] = {'a', 'b', 'c', 'd'};
    fail = false;
    fmt.push_str(zx::Slice<zx::u8>::init_unsafe(prefix, 4), &fail);
    zx::assert(!fail);
    zx::assert(fmt.n_written() == 4);

    // Now the destination is [4, 9) and the source is [2, 7), which overlap
    // on [4, 7). This partial overlap must also be rejected without writing
    // anything.
    fail = false;
    fmt.push_str(zx::Slice<zx::u8>::init_unsafe(arr + 2, 5), &fail);
    zx::assert(fail);
    zx::assert(fmt.n_written() == 4);
    zx::assert(0 == __builtin_memcmp(arr, "abcd", 4));
}

int main(void) {
    basic_test();
    empty_test();
    overlap_test();

    return 0;
}
