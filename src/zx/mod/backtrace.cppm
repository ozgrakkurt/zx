export module zx:backtrace;
import :def;

export namespace zx {

struct Backtrace {
    static constexpr u8 LEN = 32;

    u64 program_counters[LEN];
    void *eh_frame_;
    void *debug_line_;

    static void collect(Backtrace *bt);
};

} // namespace zx
