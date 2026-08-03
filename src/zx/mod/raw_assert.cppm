export module zx:raw_assert;

export namespace zx {

#ifdef ZX_DEV
void raw_assert_dev(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}
#else
void raw_assert_dev(bool cond) {
  (void)cond;
}
#endif

void raw_assert(bool cond) {
  if (!cond) [[unlikely]] {
    __builtin_trap();
  }
}

} // export namespace zx
