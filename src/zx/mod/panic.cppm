export module zx:panic;

export namespace zx {

[[noreturn]] panic() {
  __builtin_trap();
}
  
} // export namespace zx
