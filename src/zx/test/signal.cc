import zx;

using namespace zx;

bool zx_signal_test_sigill_caught_marker_ = false;

static void handler(i32_c signum) {
  assert((u32)signum == (u32)sys::SIG::ILL);
  zx_signal_test_sigill_caught_marker_ = true;

  u8 fmt_buf[69];
  Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(fmt_buf, sizeof(fmt_buf)));

  bool fmt_overflow = false;
  fmt.push_str(CString::init_unsafe("SIG HANDLER RAN@@@@@@@@@@@@@@@@\n").as_slice(), &fmt_overflow);

  assert(!fmt_overflow);

  assert(Fmt::PrintStatus::Ok == fmt.eprint());
}

int main() {
  zx_signal_test_sigill_caught_marker_ = false;
  sys::SigAction act = sys::SigAction {
    .handler = handler,
    .flags = sys::SA_NODEFER | sys::SA_RESETHAND,
    .mask = sys::SigSet::init(),
  };
  assert(sys::SigActionStatus::Ok == sys::sigaction(sys::SIG::ILL, &act, nullptr));
  assert(!zx_signal_test_sigill_caught_marker_);
  __builtin_trap();
  assert(zx_signal_test_sigill_caught_marker_);

  return 0;
}
