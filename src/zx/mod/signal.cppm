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

// echo '#include <asm/signal.h>' | gcc -E -dM -xc - 2>/dev/null | grep -i
// 'SA_RESTORER'
constexpr i32 SA_RESTORER = (i32)0x04000000__wbu;

extern "C" {

// echo '#include <asm/signal.h>' | gcc -E -dM -xc - | grep -E '^#define
// SIG[A-Z0-9]+[ \t]' | sort -t' ' -k3 -n
enum class SIG : u32_c {
    HUP = 1,
    INT = 2,
    QUIT = 3,
    ILL = 4,
    TRAP = 5,
    ABRT = 6,
    BUS = 7,
    FPE = 8,
    KILL = 9,
    USR1 = 10,
    SEGV = 11,
    USR2 = 12,
    PIPE = 13,
    ALRM = 14,
    TERM = 15,
    STKFLT = 16,
    CHLD = 17,
    CONT = 18,
    STOP = 19,
    TSTP = 20,
    TTIN = 21,
    TTOU = 22,
    URG = 23,
    XCPU = 24,
    XFSZ = 25,
    VTALRM = 26,
    PROF = 27,
    WINCH = 28,
    IO = 29,
    PWR = 30,
    SYS = 31,
    RTMIN = 32,
};

extern "C" struct SigInfo {
    u32 signo;
    i32 errno;
    i32 code;
    union {
        // Padding to bring the total to 128 bytes
        u8 pad[128 - 4 * sizeof(u32)];

        struct {
            union {
                struct {
                    i32 pid;
                    u32 uid;
                } piduid;

                struct {
                    i32 timerid;
                    i32 overrun;
                } timer;
            } first;

            union {
                union {
                    i32 int_;
                    void *ptr;
                } value;

                struct {
                    i32 status;
                    i64 utime;
                    i64 stime;
                } sigchld;
            } second;
        } common;

        struct {
            void *addr; // faulting address
            i16 addr_lsb;
            union {
                struct {
                    void *lower;
                    void *upper;
                } addr_bnd;
                u32 pkey;
            } first;
        } sigfault;

        struct {
            i64 band;
            i32 fd;
        } sigpoll;

        struct {
            void *call_addr;
            i32 syscall;
            u32 native_arch;
        } sigsys;
    } fields;
};

static_assert(sizeof(SigInfo) == 128);

// echo "#include <signal.h>" | gcc -xc - -E | grep -i "sa_sigaction"
typedef void (*SigHandler)(i32_c sig, SigInfo *info, void *ucontext);

// echo "#include <signal.h>" | gcc -xc - -E | grep -B 5 -i "__sigset_t"
// This is essentially a bitmap and the SIG.. constants are indices into the
// bitmap. We remove or add signals by setting/unsetting the bit that is indexed
// by the signal constant (e.g. SIGSEGV).
struct SigSet {
    static constexpr u8 N_ELEM = 2;

    u32 m_data[N_ELEM];

    static constexpr u8 BITS_PER_WORD = sizeof(u32) * 8;

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
        u32 const pos = s / BITS_PER_WORD;
        u32 const mask = u32{1} << (s % BITS_PER_WORD);
        assert_dev(pos < N_ELEM);
        this->m_data[pos] &= ~mask;
    }

    void set(SIG sig) {
        u32 const s = sub_unsafe_u32((u32)sig, 1);
        u32 const pos = s / BITS_PER_WORD;
        u32 const mask = u32{1} << (s % BITS_PER_WORD);
        assert_dev(pos < N_ELEM);
        this->m_data[pos] |= mask;
    }

    bool get(SIG sig) const {
        u32 const s = sub_unsafe_u32((u32)sig, 1);
        u32 const pos = s / BITS_PER_WORD;
        u32 const mask = u32{1} << (s % BITS_PER_WORD);
        assert_dev(pos < N_ELEM);
        return (this->m_data[pos] & mask) != 0;
    }
};

struct SigAction_ {
    SigHandler sa_sigaction;
    i32 sa_flags;
    void (*sa_restorer)(void);
    SigSet sa_mask;
};

struct SigAction {
    SigHandler sigaction;
    i32 flags;
    SigSet mask;
};

constexpr i32 SS_ONSTACK = 1;
constexpr i32 SS_DISABLE = 2;
constexpr i32 SS_AUTODISARM = (i32)(1U << 31);

struct Stack {
    void *ss_sp;
    i32 ss_flags;
    u64 ss_size;
};

} // extern C

} // namespace sys

} // namespace zx
