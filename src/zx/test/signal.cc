import zx;

using namespace zx;

static void handler(i32_c signum, sys::SigInfo *info, void *ucontext) {
    (void)info;
    (void)ucontext;

    assert((u32)signum == (u32)sys::SIG::ILL);
    sys::exit_group(0);
}

static void sigset_test() {
    // init is the empty set.
    sys::SigSet set = sys::SigSet::init();
    assert(set.m_data[0] == 0);
    assert(set.m_data[1] == 0);
    assert(!set.get(sys::SIG::HUP));
    assert(!set.get(sys::SIG::KILL));
    assert(!set.get(sys::SIG::RTMIN));

    for (u32 sig = (u32)sys::SIG::MIN_; sig <= (u32)sys::SIG::MAX_; sig++) {
        set.set((sys::SIG)sig);
    }

    // signals 1..32 fill word 0 completely, signals 33..63 fill bits 0..30 of
    // word 1.
    assert(set.m_data[0] == U32_MAX);
    assert(set.m_data[1] == U32_MAX >> 1);

    for (u32 sig = 1; sig <= 63; sig++) {
        assert(set.get((sys::SIG)sig));
    }

    // unset at the edges of both words and re-check.
    set.unset(sys::SIG::HUP);   // bit 0 of word 0
    set.unset(sys::SIG::RTMIN); // bit 31 of word 0
    set.unset((sys::SIG)33);    // bit 0 of word 1
    set.unset((sys::SIG)63);    // bit 30 of word 1

    assert(set.m_data[0] == (U32_MAX & ~(u32{1} << 0) & ~(u32{1} << 31)));
    assert(set.m_data[1] ==
           ((U32_MAX >> 1) & ~(u32{1} << 0) & ~(u32{1} << 30)));

    assert(!set.get(sys::SIG::HUP));
    assert(set.get(sys::SIG::INT));
    assert(!set.get(sys::SIG::RTMIN));
    assert(set.get(sys::SIG::SYS));
    assert(!set.get((sys::SIG)33));
    assert(set.get((sys::SIG)34));
    assert(!set.get((sys::SIG)63));
    assert(set.get((sys::SIG)62));

    // fill sets every bit in both words.
    sys::SigSet filled = sys::SigSet::init();
    filled.fill();
    assert(filled.get(sys::SIG::HUP));
    assert(filled.get(sys::SIG::SYS));
    assert(filled.get((sys::SIG)63));
}

int main() {
    sigset_test();

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
