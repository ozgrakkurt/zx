import zx;

extern "C" {

__attribute__((no_builtin)) void *
memcpy(void *__restrict dest, void const *__restrict src, zx::u64_c count) {
  return zx::memcpy(dest, src, count);
}

__attribute__((no_builtin)) void *memset(void *dest, zx::i32_c ch,
                                         zx::u64_c count) {
  return zx::memset(dest, ch, count);
}

__attribute__((no_builtin)) zx::i32_c memcmp(void const *lhs, void const *rhs,
                                             zx::u64_c count) {
  return zx::memcmp(lhs, rhs, count);
}

__attribute__((no_builtin)) zx::u64_c strlen(char const *str) {
  return zx::strlen(str);
}

} // extern "C"
