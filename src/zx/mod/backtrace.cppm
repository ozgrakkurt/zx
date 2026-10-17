export module zx:backtrace;
import :def;

export namespace zx {

struct Backtrace {
    static constexpr u8 LEN = 32;

    u64 program_counters[LEN];

    static void collect(Backtrace *bt);
};

} // namespace zx
