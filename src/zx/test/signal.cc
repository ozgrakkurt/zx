import zx;

using namespace zx;

bool zx_signal_test_sigill_caught_marker_ = false;

static void handler(int signum) {
  assert((u32)signum == (u32)sys::SIG::ILL);
  zx_signal_test_sigill_caught_marker_ = true;
}

int main() {
  zx_signal_test_sigill_caught_marker_ = false;
  sys::SigAction act = sys::SigAction {
    .handler = handler,
    .flags = 0,
    .mask = sys::SigSet::init(),
  };
  assert(sys::SigActionStatus::Ok == sys::sigaction(sys::SIG::ILL, &act, nullptr));
  assert(!zx_signal_test_sigill_caught_marker_);
  __builtin_trap();
  assert(zx_signal_test_sigill_caught_marker_);
  
  return 0;
}
