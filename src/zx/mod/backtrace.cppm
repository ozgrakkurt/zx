export module zx:backtrace;
import :def;

export namespace zx {

struct Backtrace {
    static constexpr u8 LEN = 32;

    u64 program_counters[LEN];

    static Backtrace init() {
        Backtrace bt;
        __builtin_memset(&bt, 0, sizeof(Backtrace));
        return bt;
    }

    static void collect(Backtrace *bt);
};

} // namespace zx
