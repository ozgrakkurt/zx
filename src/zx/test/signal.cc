import zx;

using namespace zx;

static void handler(i32_c signum, signal::SigInfo *info, void *ucontext) {
    (void)info;
    (void)ucontext;

    assert((u32)signum == (u32)signal::SIG::ILL);
    sys::exit_group(0);
}

static void sigset_test() {
    // init is the empty set.
    signal::SigSet set = signal::SigSet::init();
    assert(set.m_data[0] == 0);
    assert(set.m_data[1] == 0);
    assert(!set.get(signal::SIG::HUP));
    assert(!set.get(signal::SIG::KILL));
    assert(!set.get(signal::SIG::RTMIN));

    for (u32 sig = (u32)signal::SIG::MIN_; sig <= (u32)signal::SIG::MAX_;
         sig++) {
        set.set((signal::SIG)sig);
    }

    // signals 1..32 fill word 0 completely, signals 33..63 fill bits 0..30 of
    // word 1.
    assert(set.m_data[0] == U32_MAX);
    assert(set.m_data[1] == U32_MAX >> 1);

    for (u32 sig = 1; sig <= 63; sig++) {
        assert(set.get((signal::SIG)sig));
    }

    // unset at the edges of both words and re-check.
    set.unset(signal::SIG::HUP);   // bit 0 of word 0
    set.unset(signal::SIG::RTMIN); // bit 31 of word 0
    set.unset((signal::SIG)33);    // bit 0 of word 1
    set.unset((signal::SIG)63);    // bit 30 of word 1

    assert(set.m_data[0] == (U32_MAX & ~(u32{1} << 0) & ~(u32{1} << 31)));
    assert(set.m_data[1] ==
           ((U32_MAX >> 1) & ~(u32{1} << 0) & ~(u32{1} << 30)));

    assert(!set.get(signal::SIG::HUP));
    assert(set.get(signal::SIG::INT));
    assert(!set.get(signal::SIG::RTMIN));
    assert(set.get(signal::SIG::SYS));
    assert(!set.get((signal::SIG)33));
    assert(set.get((signal::SIG)34));
    assert(!set.get((signal::SIG)63));
    assert(set.get((signal::SIG)62));

    // fill sets every bit in both words.
    signal::SigSet filled = signal::SigSet::init();
    filled.fill();
    assert(filled.get(signal::SIG::HUP));
    assert(filled.get(signal::SIG::SYS));
    assert(filled.get((signal::SIG)63));
}

int main() {
    sigset_test();

    signal::SigAction act = signal::SigAction{
        .sigaction = handler,
        .flags = 0,
        .mask = signal::SigSet::init(),
    };
    assert(sys::SigActionStatus::Ok ==
           sys::sigaction(nullptr, signal::SIG::ILL, &act, nullptr));

    __builtin_trap();
    return 69;
}
