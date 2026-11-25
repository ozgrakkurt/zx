import zx;

using namespace zx;

// Option values are passed as the first argument to prctl(2). Argument values
// are passed through arg2..arg5.
static_assert(__is_same(i32 const, decltype(sys::PR_SET_DUMPABLE)));
static_assert(__is_same(i32 const, decltype(sys::PR_GET_PDEATHSIG)));
static_assert(__is_same(i32 const, decltype(sys::PR_SET_PTRACER)));
static_assert(__is_same(u64 const, decltype(sys::PR_MCE_KILL_DEFAULT)));
static_assert(__is_same(u64 const, decltype(sys::PR_SET_MM_START_CODE)));
static_assert(__is_same(u64 const, decltype(sys::PR_SET_PTRACER_ANY)));

static u64 prctl_ok(Backtrace *err_bt, i32 option, u64 arg0) {
    u64 ret = 0;
    testing::assert_with_trace(
        sys::PrctlStatus::Ok == sys::prctl(err_bt, option, arg0, 0, 0, 0, &ret),
        err_bt);
    return ret;
}

static void test_error_paths(Backtrace *err_bt) {
    testing::assert_with_trace(
        sys::PrctlStatus::Nullptr ==
            sys::prctl(err_bt, sys::PR_GET_DUMPABLE, 0, 0, 0, 0, nullptr),
        err_bt);

    u64 ret = 0;
    testing::assert_with_trace(
        sys::PrctlStatus::EInval ==
            sys::prctl(err_bt, I32_MAX, 0, 0, 0, 0, &ret),
        err_bt);
}

static void test_dumpable(Backtrace *err_bt) {
    u64 const original = prctl_ok(err_bt, sys::PR_GET_DUMPABLE, 0);
    assert(original <= 2);

    // SUID_DUMP_ROOT can only be set by a privileged process, but it can be
    // returned by PR_GET_DUMPABLE.
    if (original > 1) {
        return;
    }

    prctl_ok(err_bt, sys::PR_SET_DUMPABLE, 1);
    assert(prctl_ok(err_bt, sys::PR_GET_DUMPABLE, 0) == 1);

    prctl_ok(err_bt, sys::PR_SET_DUMPABLE, original);
    assert(prctl_ok(err_bt, sys::PR_GET_DUMPABLE, 0) == original);
}

static void test_pdeathsig(Backtrace *err_bt) {
    u64 ret = 0;
    i32 sig = -1;
    testing::assert_with_trace(
        sys::PrctlStatus::Ok ==
            sys::prctl(err_bt, sys::PR_GET_PDEATHSIG, (u64)&sig, 0, 0, 0, &ret),
        err_bt);
    i32 const original = sig;
    assert(original >= 0);

    prctl_ok(err_bt, sys::PR_SET_PDEATHSIG, (u64)signal::SIG::USR1);

    sig = -1;
    testing::assert_with_trace(
        sys::PrctlStatus::Ok ==
            sys::prctl(err_bt, sys::PR_GET_PDEATHSIG, (u64)&sig, 0, 0, 0, &ret),
        err_bt);
    assert(sig == (i32)signal::SIG::USR1);

    prctl_ok(err_bt, sys::PR_SET_PDEATHSIG, (u64)original);
}

static void test_name(Backtrace *err_bt) {
    char const *const name = "zx_prctl_smoke";
    prctl_ok(err_bt, sys::PR_SET_NAME, (u64)name);

    u8 buf[16];
    __builtin_memset(buf, 0, sizeof(buf));
    prctl_ok(err_bt, sys::PR_GET_NAME, (u64)buf);

    u64 const name_len = __builtin_strlen(name);
    assert(name_len < sizeof(buf));
    assert(__builtin_memcmp(buf, name, name_len + 1) == 0);
}

static void test_timing(Backtrace *err_bt) {
    assert(prctl_ok(err_bt, sys::PR_GET_TIMING, 0) ==
           sys::PR_TIMING_STATISTICAL);

    prctl_ok(err_bt, sys::PR_SET_TIMING, sys::PR_TIMING_STATISTICAL);

    u64 ret = 0;
    testing::assert_with_trace(sys::PrctlStatus::EInval ==
                                   sys::prctl(err_bt, sys::PR_SET_TIMING,
                                              sys::PR_TIMING_TIMESTAMP, 0, 0, 0,
                                              &ret),
                               err_bt);
}

static void test_timerslack(Backtrace *err_bt) {
    u64 const original = prctl_ok(err_bt, sys::PR_GET_TIMERSLACK, 0);
    assert(original > 0);

    prctl_ok(err_bt, sys::PR_SET_TIMERSLACK, 123456);
    assert(prctl_ok(err_bt, sys::PR_GET_TIMERSLACK, 0) == 123456);

    prctl_ok(err_bt, sys::PR_SET_TIMERSLACK, original);
    assert(prctl_ok(err_bt, sys::PR_GET_TIMERSLACK, 0) == original);
}

static void test_child_subreaper(Backtrace *err_bt) {
    u64 ret = 0;
    i32 subreaper = -1;
    testing::assert_with_trace(
        sys::PrctlStatus::Ok == sys::prctl(err_bt, sys::PR_GET_CHILD_SUBREAPER,
                                           (u64)&subreaper, 0, 0, 0, &ret),
        err_bt);
    i32 const original = subreaper;
    assert(original == 0 || original == 1);

    prctl_ok(err_bt, sys::PR_SET_CHILD_SUBREAPER, 1);

    subreaper = -1;
    testing::assert_with_trace(
        sys::PrctlStatus::Ok == sys::prctl(err_bt, sys::PR_GET_CHILD_SUBREAPER,
                                           (u64)&subreaper, 0, 0, 0, &ret),
        err_bt);
    assert(subreaper == 1);

    prctl_ok(err_bt, sys::PR_SET_CHILD_SUBREAPER, (u64)original);
}

static void test_thp_disable(Backtrace *err_bt) {
    u64 const original = prctl_ok(err_bt, sys::PR_GET_THP_DISABLE, 0);
    assert(original == 0 || original == 1);

    prctl_ok(err_bt, sys::PR_SET_THP_DISABLE, 1);
    assert(prctl_ok(err_bt, sys::PR_GET_THP_DISABLE, 0) == 1);

    prctl_ok(err_bt, sys::PR_SET_THP_DISABLE, original);
    assert(prctl_ok(err_bt, sys::PR_GET_THP_DISABLE, 0) == original);
}

static void test_getters(Backtrace *err_bt) {
    u64 const no_new_privs = prctl_ok(err_bt, sys::PR_GET_NO_NEW_PRIVS, 0);
    assert(no_new_privs == 0 || no_new_privs == 1);

    // CAP_CHOWN is capability 0 and is always a valid argument. The result is
    // whether the capability is in the bounding set.
    u64 const cap_in_bounding_set = prctl_ok(err_bt, sys::PR_CAPBSET_READ, 0);
    assert(cap_in_bounding_set == 0 || cap_in_bounding_set == 1);

    u64 const seccomp = prctl_ok(err_bt, sys::PR_GET_SECCOMP, 0);
    assert(seccomp <= 2);

    u64 const keepcaps = prctl_ok(err_bt, sys::PR_GET_KEEPCAPS, 0);
    assert(keepcaps == 0 || keepcaps == 1);
}

int main() {
    Backtrace err_bt = Backtrace::init();

    test_error_paths(&err_bt);
    test_dumpable(&err_bt);
    test_pdeathsig(&err_bt);
    test_name(&err_bt);
    test_timing(&err_bt);
    test_timerslack(&err_bt);
    test_child_subreaper(&err_bt);
    test_thp_disable(&err_bt);
    test_getters(&err_bt);
}
