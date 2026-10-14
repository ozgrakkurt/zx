export module zx:backtrace;
import :def;

export namespace zx {

struct Backtrace {
    static constexpr u8 LEN = 32;

    u64 m_data[LEN];

    static Backtrace init() {
        Backtrace out;
        __builtin_memset(&out, 0, sizeof(out));
        return out;
    }

    static void collect(Backtrace *bt);
};

} // namespace zx
