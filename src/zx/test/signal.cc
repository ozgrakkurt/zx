import zx;

using namespace zx;

static void handler(i32_c signum, sys::SigInfo *info, void *ucontext) {
    (void)info;
    (void)ucontext;

    assert((u32)signum == (u32)sys::SIG::ILL);
    sys::exit_group(0);
}

int main() {
    CPUContext err_ctx;

    sys::SigAction act = sys::SigAction{
        .sigaction = handler,
        .flags = 0,
        .mask = sys::SigSet::init(),
    };
    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(&err_ctx, sys::SIG::ILL, &act, nullptr));

    __builtin_trap();
    return 69;
}
