import zx;

using namespace zx;

// Known-answer vectors for xoshiro256++ 1.0. The expected outputs were
// generated with the canonical reference implementation from:
// https://xoshiro.di.unimi.it/xoshiro256plusplus.c
// Seeded vectors use the SplitMix64 expansion implemented by
// `Xoshiro256::init`.
static void check_state(u64 s0, u64 s1, u64 s2, u64 s3, u64 const *expected,
                        u64 count) {
    Xoshiro256 rand;
    rand.s[0] = s0;
    rand.s[1] = s1;
    rand.s[2] = s2;
    rand.s[3] = s3;

    for (u64 i = 0; i < count; i++) {
        assert(rand.next() == expected[i]);
    }
}

static void check_seed(u64 seed, u64 const *expected, u64 count) {
    Xoshiro256 rand = Xoshiro256::init(seed);

    for (u64 i = 0; i < count; i++) {
        assert(rand.next() == expected[i]);
    }
}

static void known_answer_states() {
    {
        constexpr u64 expected[] = {
            0x0000000002800001__wbu, 0x0000000003800067__wbu,
            0x000cc00003800067__wbu, 0x000cc201994400b2__wbu,
            0x8012a2019ac433cd__wbu, 0x8a69978acdee33ba__wbu,
            0xc271134733154abd__wbu, 0xac2ba09179169e97__wbu,
            0xdbf3190a8f073fd8__wbu, 0x9105f14ab2229220__wbu,
        };
        check_state(1, 2, 3, 4, expected, sizeof(expected) / sizeof(*expected));
    }

    {
        constexpr u64 expected[] = {
            0xc5f92c5f923d70a2__wbu, 0x9cb6587213adcff9__wbu,
            0x1630d1fd9e375a74__wbu, 0x2d7e5fc2b0e9fd68__wbu,
            0xe3306b3967d84568__wbu, 0xb5edbd82215d4bb3__wbu,
            0x81e41b4fcf04e8ce__wbu, 0x3024279da796c214__wbu,
            0x811a948c7a1a8a74__wbu, 0x63a90a6e003b5ef1__wbu,
        };
        check_state(0x0123456789abcdef__wbu, 0xfedcba9876543210__wbu,
                    0x1111111111111111__wbu, 0x2222222222222222__wbu, expected,
                    sizeof(expected) / sizeof(*expected));
    }
}

static void known_answer_seeds() {
    {
        constexpr u64 expected[] = {
            0x53175d61490b23df__wbu, 0x61da6f3dc380d507__wbu,
            0x5c0fdf91ec9a7bfc__wbu, 0x02eebf8c3bbe5e1a__wbu,
            0x7eca04ebaf4a5eea__wbu, 0x0543c37757f08d9a__wbu,
            0xdb7490c75ab5026e__wbu, 0xd87343e6464bc959__wbu,
            0x4b7da0a02389f0ff__wbu, 0x1300fc58c0424c16__wbu,
        };
        check_seed(0, expected, sizeof(expected) / sizeof(*expected));
    }

    {
        constexpr u64 expected[] = {
            0xcfc5d07f6f03c29b__wbu, 0xbf424132963fe08d__wbu,
            0x19a37d5757aaf520__wbu, 0xbf08119f05cd56d6__wbu,
            0x2f47184b86186fa4__wbu, 0x97299fcae7202345__wbu,
            0xfca3c79508f41507__wbu, 0x85fea5c90363f221__wbu,
            0x18bae5b30d334bd0__wbu, 0x226113c9f026ec16__wbu,
        };
        check_seed(1, expected, sizeof(expected) / sizeof(*expected));
    }

    {
        constexpr u64 expected[] = {
            0xf2dc6ae2dc3f74e2__wbu, 0x1a08b2229cfb2906__wbu,
            0x839ffa06c62817e1__wbu, 0x030a027b60738686__wbu,
            0xb7cc15ac7b41329c__wbu, 0xfc37ab98e31d067a__wbu,
            0x112fed7cdad7cd64__wbu, 0x078996f54b20d875__wbu,
            0x03e013e684a25ec0__wbu, 0x45108087bd03297b__wbu,
        };
        check_seed(69, expected, sizeof(expected) / sizeof(*expected));
    }

    {
        constexpr u64 expected[] = {
            0xb2f2a310e96bd1c5__wbu, 0xb54062465b950493__wbu,
            0x87aca4a9668814b0__wbu, 0xf13d2e2448a9cffb__wbu,
            0xb7afdb427f6b86a2__wbu, 0xc3a68c4e4f50d0c7__wbu,
            0x5bde00c2b40585ae__wbu, 0xb27e2dd974f18e8a__wbu,
            0xa14dd5c28ddfa4ea__wbu, 0xb642d3593ecf568c__wbu,
        };
        check_seed(0x0123456789abcdef__wbu, expected,
                   sizeof(expected) / sizeof(*expected));
    }
}

static void known_answer_fill() {
    // 20 bytes = 2 full little-endian u64 outputs followed by the low 4 bytes
    // of the third output.
    u8 bytes[20];
    SliceMut<u8> const data = SliceMut<u8>::init_unsafe(bytes, sizeof(bytes));

    Xoshiro256 rand = Xoshiro256::init(69);
    rand.fill(data);

    constexpr u8 expected[] = {
        0xe2__wbu, 0x74__wbu, 0x3f__wbu, 0xdc__wbu, 0xe2__wbu,
        0x6a__wbu, 0xdc__wbu, 0xf2__wbu, 0x06__wbu, 0x29__wbu,
        0xfb__wbu, 0x9c__wbu, 0x22__wbu, 0xb2__wbu, 0x08__wbu,
        0x1a__wbu, 0xe1__wbu, 0x17__wbu, 0x28__wbu, 0xc6__wbu,
    };

    for (u64 i = 0; i < sizeof(expected) / sizeof(*expected); i++) {
        assert(data.as_const().read_unsafe(i) == expected[i]);
    }
}

static void basic() {
    Xoshiro256 rand = Xoshiro256::init(69);

    u64 const a = rand.next();

    u64 const b = rand.next();

    assert(a != b);
}

static void fill_aligned() {
    constexpr u64 len = 69;
    u64 vals[len];
    SliceMut<u64> const data = SliceMut<u64>::init_unsafe(vals, len);
    data.memset(0x45);

    Xoshiro256 rand = Xoshiro256::init(69);

    rand.fill(data.cast_unsafe<u8>());

    for (u64 i = 0; i < data.len(); i++) {
        u64 const v = data.as_const().read_unsafe(i);
        assert(v != 0x4545454545454545__wbu);
    }
}

static void fill_partial() {
    constexpr u64 n_bytes = 9;
    u8 bytes[n_bytes];
    SliceMut<u8> const data = SliceMut<u8>::init_unsafe(bytes, n_bytes);
    data.memset(69);

    Xoshiro256 rand = Xoshiro256::init(69);

    rand.fill(data);

    for (u64 i = 0; i < data.len(); i++) {
        u8 const v = data.as_const().read_unsafe(i);
        assert(v != 69);
    }
}

static void fill_long_and_partial() {
    constexpr u64 n_bytes = 69;
    u8 bytes[n_bytes];
    SliceMut<u8> const data = SliceMut<u8>::init_unsafe(bytes, n_bytes);
    data.memset(69);

    Xoshiro256 rand = Xoshiro256::init(0);

    rand.fill(data);

    for (u64 i = 0; i < data.len(); i++) {
        u8 const v = data.as_const().read_unsafe(i);
        assert(v != 69);
    }
}

int main(void) {
    known_answer_states();
    known_answer_seeds();
    known_answer_fill();

    basic();
    fill_aligned();
    fill_partial();
    fill_long_and_partial();

    return 0;
}
