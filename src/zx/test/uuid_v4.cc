import zx;

using namespace zx;

int main() {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    constexpr u64 N_IDS = 50;
    u128 ids[N_IDS];
    for (u64 i = 0; i < N_IDS; i++) {
        ids[i] = 60;
    }

    Xoshiro256 rng = Xoshiro256::init(69);

    for (u64 i = 0; i < N_IDS; i++) {
        ids[i] = uuid_v4::generate(&rng);
    }

    for (u64 i = 0; i < N_IDS; i++) {
        for (u64 j = 0; j < i; j++) {
            assert(ids[i] != ids[j]);
        }
    }

    u8 fmt_buf[48];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(fmt_buf, sizeof(fmt_buf)));

    bool fmt_overflow = false;

    uuid_v4::print(306208844067214555354773908615903551303__wbu, false, &fmt,
                   &fmt_overflow);
    assert(!fmt_overflow);

    testing::assert_with_trace(Fmt::PrintStatus::Ok == fmt.eprint(&err_ctx),
                               &err_ctx);

    assert(fmt.written().equals(
        CString::init_unsafe("e65dad9e-ae41-11f1-a125-325096b39f47")
            .as_slice()));

    return 0;
}
