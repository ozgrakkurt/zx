export module zx:backtrace;
import :def;

export namespace zx {
namespace backtrace {

#ifdef ZX_DEBUG_INFO

extern "C" struct Context {
  u64 r12;
  u64 r13;
  u64 r14;
  u64 r15;
  u64 rbp;
  u64 rbx;
  u64 rsp;
  u64 rip;
};

extern "C" void zx_backtrace_getcontext_(Context* out);

enum class [[nodiscard]] GetContextStatus : u8_c {
  Ok = 0,
  Nullptr,
};

GetContextStatus getcontext(Context* out) {
  if (out == nullptr) {
    return GetContextStatus::Nullptr;
  }

  zx_backtrace_getcontext_(out);
  return GetContextStatus::Ok;
}

#endif // ifdef ZX_DEBUG_INFO

} // namespace backtrace
} // export namespace zx
