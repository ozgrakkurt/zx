import zx;
using namespace zx;

// Known-answer vectors generated with the upstream rapidhash v3 reference
// implementation (the "Nano" variant) from:
// https://github.com/Nicoshev/rapidhash/blob/rapidhash_v3/rapidhash.h
//
// The `pattern` vectors hash `len` bytes where byte i is `(i * 31 + 7) & 0xff`.
struct PatternVec {
    u64 len;
    u64 seed;
    u64 expected;
};

static void known_answer_pattern() {
    constexpr u64 PATTERN_LEN = 200;
    u8 data[PATTERN_LEN];
    SliceMut<u8> const buf = SliceMut<u8>::init_unsafe(data, PATTERN_LEN);

    for (u64 i = 0; i < PATTERN_LEN; i++) {
        buf.write_unsafe(i, (u8)(i * 31 + 7));
    }

    constexpr PatternVec vectors[] = {
        {0, 0x0000000000000000__wbu, 0x0338dc4be2cecdae__wbu},
        {1, 0x0000000000000000__wbu, 0x5c2caf7d68f06d3e__wbu},
        {2, 0x0000000000000000__wbu, 0xae51663c7995f8f8__wbu},
        {3, 0x0000000000000000__wbu, 0x3bd769fd1eeff68b__wbu},
        {4, 0x0000000000000000__wbu, 0x94c4e44cba1be502__wbu},
        {5, 0x0000000000000000__wbu, 0x20b60f776813eba8__wbu},
        {7, 0x0000000000000000__wbu, 0x3402f3bff7d5ef0f__wbu},
        {8, 0x0000000000000000__wbu, 0xefd148285045d1f3__wbu},
        {9, 0x0000000000000000__wbu, 0x2f8407ca55d0192d__wbu},
        {15, 0x0000000000000000__wbu, 0x94192c8d95e7a5a5__wbu},
        {16, 0x0000000000000000__wbu, 0x0811971e7cf397ba__wbu},
        {17, 0x0000000000000000__wbu, 0x6d84939d31572677__wbu},
        {24, 0x0000000000000000__wbu, 0x34acc0ed95129db0__wbu},
        {31, 0x0000000000000000__wbu, 0x8371afa501b0cd07__wbu},
        {32, 0x0000000000000000__wbu, 0xba48299a836e97da__wbu},
        {33, 0x0000000000000000__wbu, 0xaf3a3a115f66dba3__wbu},
        {47, 0x0000000000000000__wbu, 0x92f8b21ca81775e0__wbu},
        {48, 0x0000000000000000__wbu, 0xb647c688e65ab5d9__wbu},
        {49, 0x0000000000000000__wbu, 0x1ba1ca115378c59a__wbu},
        {63, 0x0000000000000000__wbu, 0x1f17afcaaed0b92e__wbu},
        {64, 0x0000000000000000__wbu, 0x0a771ffc3ae04a6c__wbu},
        {65, 0x0000000000000000__wbu, 0xbe1cc7968e7ddd14__wbu},
        {95, 0x0000000000000000__wbu, 0xbc0f8819eb63f5f9__wbu},
        {96, 0x0000000000000000__wbu, 0xdebf957759d92837__wbu},
        {97, 0x0000000000000000__wbu, 0x02445fbdb880506b__wbu},
        {128, 0x0000000000000000__wbu, 0xfd2676b980cdc82a__wbu},
        {200, 0x0000000000000000__wbu, 0xf0bc1925d39c0306__wbu},
        {0, 0x0000000000000001__wbu, 0xad700ecdf353d5ca__wbu},
        {16, 0x0000000000000001__wbu, 0x09e3e05c292967d7__wbu},
        {17, 0x0000000000000001__wbu, 0x7d027e2dd2c6a102__wbu},
        {48, 0x0000000000000001__wbu, 0xde7fb4b4aa0bf8c6__wbu},
        {97, 0x0000000000000001__wbu, 0x42b67c44b8d3e5ad__wbu},
        {0, 0x0123456789abcdef__wbu, 0x565ef32cd8efb3dd__wbu},
        {16, 0x0123456789abcdef__wbu, 0x34e80d5ea92ea3b2__wbu},
        {17, 0x0123456789abcdef__wbu, 0xd513048899422a28__wbu},
        {97, 0x0123456789abcdef__wbu, 0xe33381b6dc3c0750__wbu},
    };

    for (u64 i = 0; i < sizeof(vectors) / sizeof(*vectors); i++) {
        Slice<u8> const key = buf.as_const().slice_unsafe(0, vectors[i].len);
        assert(rapidhash_v3::hash(key, vectors[i].seed) == vectors[i].expected);
    }
}

struct StringVec {
    char const *key;
    u64 seed;
    u64 expected;
};

static void known_answer_strings() {
    constexpr StringVec vectors[] = {
        {"", 0x0000000000000000__wbu, 0x0338dc4be2cecdae__wbu},
        {"a", 0x0000000000000000__wbu, 0x599f47df33a2e1eb__wbu},
        {"ab", 0x0000000000000000__wbu, 0x7b20ba72bb425975__wbu},
        {"abc", 0x0000000000000000__wbu, 0xcb475beafa9c0da2__wbu},
        {"abcd", 0x0000000000000000__wbu, 0xf8f44f4a65e26132__wbu},
        {"hello", 0x0000000000000000__wbu, 0x2e2d7651b45f7946__wbu},
        {"hello world!", 0x0000000000000000__wbu, 0xf81e76290e0249a3__wbu},
        {"0123456789abcdef", 0x0000000000000000__wbu, 0x8b249eb98f29bff6__wbu},
        {"0123456789abcdefg", 0x0000000000000000__wbu, 0x62b040278b0412a6__wbu},
        {"The quick brown fox jumps over the lazy dog", 0x0000000000000000__wbu,
         0x91722dc8d52a3f7b__wbu},
        {"The quick brown fox jumps over the lazy dog", 0xdeadbeefcafebabe__wbu,
         0xa498f55d35a7c1be__wbu},
    };

    for (u64 i = 0; i < sizeof(vectors) / sizeof(*vectors); i++) {
        char const *key = vectors[i].key;
        Slice<u8> const slice =
            Slice<u8>::init_unsafe((u8 const *)key, __builtin_strlen(key));
        assert(rapidhash_v3::hash(slice, vectors[i].seed) ==
               vectors[i].expected);
    }
}

int main(void) {
    known_answer_pattern();
    known_answer_strings();

    constexpr u64 len = 200;
    u8 data[len];

    SliceMut<u8> const slice = SliceMut<u8>::init_unsafe(data, len);

    slice.fill(76);

    for (u64 sl = 0; sl < len; sl++) {
        SliceMut<u8> const s = slice.slice_unsafe(0, sl);
        u64 const base_hash = rapidhash_v3::hash(s.as_const(), 0);
        // check that changing any byte causes the hash to change.
        // This is important because we want to be including every byte in the
        // hash process.
        for (u64 i = 0; i < sl; i++) {
            s.write_unsafe(i, 100);
            u64 const chg_hash = rapidhash_v3::hash(s.as_const(), 0);

            if (base_hash == chg_hash) {
                panic();
            }
            s.write_unsafe(i, 76);
        }
    }

    return 0;
}
