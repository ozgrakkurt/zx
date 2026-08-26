export module zx:panic;
import :raw_alloc;
import :sys;
import :page_alloc;
import :raw_assert;

export namespace zx {

[[noreturn]] void panic() { __builtin_trap(); }

} // namespace zx
