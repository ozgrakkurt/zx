export module zx:testing;
import :assert;
import :elf;
import :cpu_context;
import :panic;

export namespace zx {

namespace testing {

#ifdef ZX_DEBUG_INFO

void assert_with_trace(bool cond, CPUContext const *ctx) {
    if (!cond) [[unlikely]] {
        elf::print_stack_trace_panic(ctx);
        panic();
    }
}

#else // ifdef ZX_DEBUG_INFO

void assert_with_trace(bool cond, CPUContext const *ctx) {
    (void)ctx;

    assert(cond);
}

#endif // ifdef ZX_DEBUG_INFO

} // namespace testing

} // namespace zx
