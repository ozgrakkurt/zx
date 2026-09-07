import zx;
using namespace zx;

int main(void) {
    constexpr u64 len = 569;
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
