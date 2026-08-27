import zx;

int main() {
  zx::backtrace::Context ctx;
  __builtin_memset(&ctx, 0, sizeof(zx::backtrace::Context));
  zx::backtrace::GetContextStatus const status = zx::backtrace::getcontext(&ctx);
  zx::raw_assert(status == zx::backtrace::GetContextStatus::Ok);

  zx::raw_assert(zx::backtrace::GetContextStatus::Nullptr == zx::backtrace::getcontext(nullptr));

  return 0;
}
