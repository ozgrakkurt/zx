export module zx:testing;
import :assert;
import :elf;
import :panic;
import :backtrace;
import :sys;

export namespace zx {

namespace testing {

#ifdef ZX_DEBUG_INFO

void print_panic_(char const *message) {
    assert(message != nullptr);

    u64 const len = __builtin_strlen(message);
    u64 n_written = 0;
    assert(sys::WriteStatus::Ok == sys::write(nullptr, sys::STDERR,
                                              (u8 const *)message, len,
                                              &n_written));
    assert(n_written == len);
}

void assert_with_trace(bool cond, Backtrace const *bt) {
    if (!cond) [[unlikely]] {
        print_panic_(
            "Printing stack trace because testing error assertion failed.\n");

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
