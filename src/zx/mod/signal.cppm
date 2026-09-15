export module zx:signal;
import :def;
import :syscall;
import :arith;

export namespace zx {

namespace sys {

// echo '#define _GNU_SOURCE 1
//   #include <signal.h>' | gcc -E -dM -xc - | grep -i "SA_"
constexpr i32 SA_SIGINFO = 4;
constexpr i32 SA_ONSTACK = (i32)0x08000000__wbu;
constexpr i32 SA_NODEFER = (i32)0x40000000__wbu;
constexpr i32 SA_RESETHAND = (i32)0x80000000__wbu;
constexpr i32 SA_NOCLDSTOP = 1;
constexpr i32 SA_STACK = SA_ONSTACK;
constexpr i32 SA_ONESHOT = SA_RESETHAND;
constexpr i32 SA_RESTART = (i32)0x10000000__wbu;
constexpr i32 SA_INTERRUPT = (i32)0x20000000__wbu;
constexpr i32 SA_NOMASK = SA_NODEFER;
constexpr i32 SA_NOCLDWAIT = 2;

// echo "#include <signal.h>" | gcc -xc - -E | grep -i "__sighandler_t"
typedef void (*SigHandler)(int);

// echo "#include <signal.h>" | gcc -xc - -E | grep -i "SIG"
enum class SIG : u32_c {
    HUP = 1,
    INT = 2,
    QUIT = 3,
    ILL = 4,
    TRAP = 5,
    ABRT = 6,
    STKFLT = 7,
    FPE = 8,
    KILL = 9,
    BUS = 10,
    SEGV = 11,
    XCPU = 12,
    PIPE = 13,
    ALRM = 14,
    TERM = 15,
    USR1 = 16,
    USR2 = 17,
    CHLD = 18,
    PWR = 19,
    VTALRM = 20,
    PROF = 21,
    IO = 22,
    WINCH = 23,
    STOP = 24,
    TSTP = 25,
    CONT = 26,
    TTIN = 27,
    TTOU = 28,
    URG = 29,
    XFSZ = 30,
    SYS = 31,
};

// echo "#include <signal.h>" | gcc -xc - -E | grep -B 5 -i "__sigset_t"
// This is essentially a bitmap and the SIG.. constants are indices into the
// bitmap. We remove or add signals by setting/unsetting the bit that is indexed
// by the signal constant (e.g. SIGSEGV).
struct SigSet {
    u32 m_data[32];

    static SigSet init() {
        SigSet out;
        __builtin_memset(out.m_data, 0, sizeof(out.m_data));
        return out;
    }

    void fill() {
        __builtin_memset(this->m_data, u8{0xff__wbu}, sizeof(this->m_data));
    }

    void unset(SIG sig) {
        u32 const s = sub_unsafe_u32((u32)sig, 1);
        u32 const pos = s / 32__wbu;
        u32 const mask = u32{1} << (s % 32);
        assert_dev(pos < 32);
        this->m_data[pos] &= ~mask;
    }

    void set(SIG sig) {
        u32 const s = sub_unsafe_u32((u32)sig, 1);
        u32 const pos = s / 32__wbu;
        u32 const mask = u32{1} << (s % 32);
        assert_dev(pos < 32);
        this->m_data[pos] |= mask;
    }

    bool get(SIG sig) const {
        u32 const s = sub_unsafe_u32((u32)sig, 1);
        u32 const pos = s / 32__wbu;
        u32 const mask = u32{1} << (s % 32);
        assert_dev(pos < 32);
        return (this->m_data[pos] & mask) != 0;
    }
};

// taken from /usr/include/bits/sigaction.h
struct SigAction_ {
    SigHandler sa_handler;
    SigSet sa_mask;
    i32 sa_flags;
    void (*sa_restorer)(void);
};

struct SigAction {
    SigHandler handler;
    SigSet mask;
    i32 flags;
};

} // namespace sys

} // namespace zx
