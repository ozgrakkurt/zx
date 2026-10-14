export module zx:testing;
import :assert;
import :elf;
import :panic;
import :backtrace;

export namespace zx {

namespace testing {

#ifdef ZX_DEBUG_INFO

void assert_with_trace(bool cond, Backtrace const *bt) {
    if (!cond) [[unlikely]] {
        elf::print_stack_trace_panic(bt);
        panic();
    }
}

#else // ifdef ZX_DEBUG_INFO

void assert_with_trace(bool cond, Backtrace const *bt) {
    (void)bt;

    assert(cond);
}

#endif // ifdef ZX_DEBUG_INFO

} // namespace testing

} // namespace zx
