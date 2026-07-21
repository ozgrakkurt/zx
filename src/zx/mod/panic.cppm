export module zx:panic;

export namespace zx {

[[noreturn]] void panic() {
  __builtin_trap();
}
  
} // export namespace zx
