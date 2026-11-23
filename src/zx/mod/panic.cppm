export module zx:panic;

export namespace zx {

[[noreturn]] inline void panic() { __builtin_trap(); }

} // namespace zx
