export module zx:testing;
import :elf;
import :cpu_context;
import :panic;

export namespace zx {

namespace testing {

void assert_with_trace(bool cond, CPUContext const *ctx) {
    if (!cond) [[unlikely]] {
        elf::print_stack_trace_panic(ctx);
        panic();
    }
}

} // namespace testing

} // namespace zx
