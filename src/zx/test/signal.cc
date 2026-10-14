import zx;

using namespace zx;

static void handler(i32_c signum, sys::SigInfo *info, void *ucontext) {
    (void)info;
    (void)ucontext;

    assert((u32)signum == (u32)sys::SIG::ILL);
    sys::exit_group(0);
}

int main() {
    sys::SigAction act = sys::SigAction{
        .sigaction = handler,
        .flags = 0,
        .mask = sys::SigSet::init(),
    };
    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(nullptr, sys::SIG::ILL, &act, nullptr));

    __builtin_trap();
    return 69;
}
