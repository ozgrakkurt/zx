import zx;

using namespace zx;

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
    data.fill(69);

    Xoshiro256 rand = Xoshiro256::init(69);

    rand.fill(data.cast_unsafe<u8>());

    for (u64 i = 0; i < data.len(); i++) {
        u64 const v = data.as_const().read_unsafe(i);
        assert(v != 69);
    }
}

static void fill_partial() {
    constexpr u64 n_bytes = 9;
    u8 bytes[n_bytes];
    SliceMut<u8> const data = SliceMut<u8>::init_unsafe(bytes, n_bytes);
    data.fill(69);

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
    data.fill(69);

    Xoshiro256 rand = Xoshiro256::init(0);

    rand.fill(data);

    for (u64 i = 0; i < data.len(); i++) {
        u8 const v = data.as_const().read_unsafe(i);
        assert(v != 69);
    }
}

int main(void) {
    basic();
    fill_aligned();
    fill_partial();
    fill_long_and_partial();

    return 0;
}
