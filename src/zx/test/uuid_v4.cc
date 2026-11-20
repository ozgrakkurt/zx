import zx;

using namespace zx;

// The known-answer vectors for `generate` were produced with an independent
// python reference implementation of the full pipeline:
//
//   - SplitMix64 seeding and xoshiro256++ as implemented by `Xoshiro256`
//     (the reference was validated against the vectors in xoshiro256.cc)
//   - a u128 filled with two little-endian u64 outputs of `Xoshiro256::fill`
//   - the version and variant bits forced by masking and or-ing:
//
//         (out & 0xFFFFFFFFFFFF4FFFBFFFFFFFFFFFFFFF)
//             | 0x40008000000000000000
//
// The `print` vectors are the canonical textual form of their corresponding
// 128-bit values.

constexpr u128 VERSION_NIBBLE_MASK = u128{0xF} << 76__wbu;
constexpr u128 VERSION_NIBBLE_4 = u128{4} << 76__wbu;
constexpr u128 VARIANT_BITS_MASK = u128{0x3} << 62__wbu;
constexpr u128 VARIANT_BITS_RFC4122 = u128{0x2} << 62__wbu;
constexpr u128 UUID_RANDOM_BITS_MASK =
    ~(VERSION_NIBBLE_MASK | VARIANT_BITS_MASK);

static void check_generate(u64 seed, u128 const *expected, u64 count) {
    Xoshiro256 rand = Xoshiro256::init(seed);

    for (u64 i = 0; i < count; i++) {
        u128 const id = uuid_v4::generate(&rand);

        assert(id == expected[i]);
        assert((id & VERSION_NIBBLE_MASK) == VERSION_NIBBLE_4);
        assert((id & VARIANT_BITS_MASK) == VARIANT_BITS_RFC4122);
    }
}

static void known_answer_generate() {
    {
        constexpr u128 expected[] = {
            0x61da6f3dc380450793175d61490b23df__wbu,
            0x02eebf8c3bbe4e1a9c0fdf91ec9a7bfc__wbu,
            0x0543c37757f04d9abeca04ebaf4a5eea__wbu,
        };
        check_generate(0, expected, sizeof(expected) / sizeof(*expected));
    }

    {
        constexpr u128 expected[] = {
            0xbf424132963f408d8fc5d07f6f03c29b__wbu,
            0xbf08119f05cd46d699a37d5757aaf520__wbu,
            0x97299fcae7204345af47184b86186fa4__wbu,
        };
        check_generate(1, expected, sizeof(expected) / sizeof(*expected));
    }

    {
        constexpr u128 expected[] = {
            0x1a08b2229cfb4906b2dc6ae2dc3f74e2__wbu,
            0x030a027b60734686839ffa06c62817e1__wbu,
            0xfc37ab98e31d467ab7cc15ac7b41329c__wbu,
        };
        check_generate(69, expected, sizeof(expected) / sizeof(*expected));
    }

    {
        constexpr u128 expected[] = {
            0xb54062465b954493b2f2a310e96bd1c5__wbu,
            0xf13d2e2448a94ffb87aca4a9668814b0__wbu,
            0xc3a68c4e4f5040c7b7afdb427f6b86a2__wbu,
        };
        check_generate(0x0123456789abcdef__wbu, expected,
                       sizeof(expected) / sizeof(*expected));
    }
}

// `generate` must consume exactly two u64 words from the rng and must only
// touch the version and variant bits, everything else has to come from the
// rng output untouched.
static void generate_only_sets_version_and_variant() {
    constexpr u64 SEEDS[] = {0, 1, 69, 0x0123456789abcdef__wbu};

    for (u64 i = 0; i < sizeof(SEEDS) / sizeof(*SEEDS); i++) {
        Xoshiro256 gen_rand = Xoshiro256::init(SEEDS[i]);
        Xoshiro256 raw_rand = Xoshiro256::init(SEEDS[i]);

        u128 const id = uuid_v4::generate(&gen_rand);

        u128 raw = 0;
        raw_rand.fill(SliceMut<u8>::init_from_ptr_unsafe<u128>(&raw));

        // both rns consumed the same amount of state
        for (u8 j = 0; j < 4; j++) {
            assert(gen_rand.s[j] == raw_rand.s[j]);
        }

        // only the version and variant bits differ
        assert((id & UUID_RANDOM_BITS_MASK) == (raw & UUID_RANDOM_BITS_MASK));
        assert((id & VERSION_NIBBLE_MASK) == VERSION_NIBBLE_4);
        assert((id & VARIANT_BITS_MASK) == VARIANT_BITS_RFC4122);
    }
}

// All generated ids must be valid RFC 4122 v4 uuids and unique.
static void generate_properties() {
    constexpr u64 N_IDS = 1000;
    u128 ids[N_IDS];

    Xoshiro256 rand = Xoshiro256::init(69);

    for (u64 i = 0; i < N_IDS; i++) {
        ids[i] = uuid_v4::generate(&rand);

        assert((ids[i] & VERSION_NIBBLE_MASK) == VERSION_NIBBLE_4);
        assert((ids[i] & VARIANT_BITS_MASK) == VARIANT_BITS_RFC4122);
        assert(ids[i] != 0);
    }

    for (u64 i = 0; i < N_IDS; i++) {
        for (u64 j = 0; j < i; j++) {
            assert(ids[i] != ids[j]);
        }
    }
}

static void check_print(u128 uuid, bool upper_case, char const *expected) {
    u8 buf[64];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));

    bool fmt_overflow = false;
    uuid_v4::print(uuid, upper_case, &fmt, &fmt_overflow);

    assert(!fmt_overflow);
    assert(fmt.n_written() == 36);
    assert(fmt.written().equals(CString::init_unsafe(expected).as_slice()));
}

static void known_answer_print() {
    // a v1 uuid, print must not touch any bits and just format
    check_print(0xe65dad9eae4111f1a125325096b39f47__wbu, false,
                "e65dad9e-ae41-11f1-a125-325096b39f47");

    // nil uuid
    check_print(0__wbu, false, "00000000-0000-0000-0000-000000000000");

    // max uuid
    check_print(U128_MAX, false, "ffffffff-ffff-ffff-ffff-ffffffffffff");
    check_print(U128_MAX, true, "FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF");

    // covers every hex digit in both cases
    check_print(0x0123456789abcdeffedcba9876543210__wbu, false,
                "01234567-89ab-cdef-fedc-ba9876543210");
    check_print(0x0123456789abcdeffedcba9876543210__wbu, true,
                "01234567-89AB-CDEF-FEDC-BA9876543210");

    // first uuids generated from a few seeds, lower and upper case
    check_print(0x61da6f3dc380450793175d61490b23df__wbu, false,
                "61da6f3d-c380-4507-9317-5d61490b23df");
    check_print(0xbf424132963f408d8fc5d07f6f03c29b__wbu, true,
                "BF424132-963F-408D-8FC5-D07F6F03C29B");
    check_print(0x1a08b2229cfb4906b2dc6ae2dc3f74e2__wbu, false,
                "1a08b222-9cfb-4906-b2dc-6ae2dc3f74e2");
    check_print(0xb54062465b954493b2f2a310e96bd1c5__wbu, true,
                "B5406246-5B95-4493-B2F2-A310E96BD1C5");
}

static void print_roundtrip() {
    Xoshiro256 rand = Xoshiro256::init(69);

    u8 buf[64];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));

    bool fmt_overflow = false;

    // every generated uuid must roundtrip through print
    for (u64 i = 0; i < 100; i++) {
        u128 const id = uuid_v4::generate(&rand);

        fmt.reset();
        uuid_v4::print(id, false, &fmt, &fmt_overflow);

        assert(!fmt_overflow);
        assert(fmt.n_written() == 36);

        Slice<u8> const written = fmt.written();

        // dashes
        assert(written.read_unsafe(8) == '-');
        assert(written.read_unsafe(13) == '-');
        assert(written.read_unsafe(18) == '-');
        assert(written.read_unsafe(23) == '-');

        // the hex digits must match the uuid, big endian
        u8 hex_buf[32];
        SliceMut<u8> hex = SliceMut<u8>::init_unsafe(hex_buf, sizeof(hex_buf));
        uuid_v4::format_simple_unsafe_(id, false, hex);

        for (u8 j = 0; j < 32__wbu; j++) {
            u8 const offset =
                (j < 8__wbu)
                    ? j
                    : ((j < 12__wbu)
                           ? j + 1__wbu
                           : ((j < 16__wbu)
                                  ? j + 2__wbu
                                  : ((j < 20__wbu) ? j + 3__wbu : j + 4__wbu)));
            assert(written.read_unsafe(offset) ==
                   hex.as_const().read_unsafe(j));
        }
    }
}

static void print_exact_fit() {
    u8 buf[36];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));

    bool fmt_overflow = false;
    uuid_v4::print(0x0123456789abcdeffedcba9876543210__wbu, false, &fmt,
                   &fmt_overflow);

    assert(!fmt_overflow);
    assert(fmt.n_written() == 36);
    assert(fmt.written().equals(
        CString::init_unsafe("01234567-89ab-cdef-fedc-ba9876543210")
            .as_slice()));
}

static void print_truncated() {
    u8 buf[35];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));

    bool fmt_overflow = false;
    uuid_v4::print(0x0123456789abcdeffedcba9876543210__wbu, false, &fmt,
                   &fmt_overflow);

    assert(fmt_overflow);
    assert(fmt.n_written() == 35);
    assert(fmt.written().equals(
        CString::init_unsafe("01234567-89ab-cdef-fedc-ba987654321")
            .as_slice()));
}

static void print_no_capacity() {
    Fmt fmt = Fmt::init(SliceMut<u8>::init_dangling());

    bool fmt_overflow = false;
    uuid_v4::print(0x0123456789abcdeffedcba9876543210__wbu, false, &fmt,
                   &fmt_overflow);

    assert(fmt_overflow);
    assert(fmt.n_written() == 0);
}

static void print_accumulates() {
    u8 buf[72];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(buf, sizeof(buf)));

    bool fmt_overflow = false;

    uuid_v4::print(0x0123456789abcdeffedcba9876543210__wbu, false, &fmt,
                   &fmt_overflow);
    assert(!fmt_overflow);

    uuid_v4::print(0__wbu, false, &fmt, &fmt_overflow);
    assert(!fmt_overflow);

    assert(fmt.n_written() == 72);
    assert(fmt.written().equals(
        CString::init_unsafe("01234567-89ab-cdef-fedc-ba9876543210"
                             "00000000-0000-0000-0000-000000000000")
            .as_slice()));
}

int main() {
    known_answer_generate();
    generate_only_sets_version_and_variant();
    generate_properties();

    known_answer_print();
    print_roundtrip();

    print_exact_fit();
    print_truncated();
    print_no_capacity();
    print_accumulates();

    return 0;
}
