export module zx:signal;
import :def;

export namespace zx {

namespace sys {

constexpr u64 NR_RT_SIGRETURN = 15;

// internal trampoline to be used for sigaction impl.
// https://www.man7.org/linux/man-pages/man2/sigreturn.2.html
//
// It essentially just returns control back to the kernel.
//
// Implementation is taken from zig implementation for the same thing on x86_64
// linux.
void sigreturn_() { syscall0(NR_RT_SIGRETURN); }

// echo "#include <signal.h>" | gcc -xc - -E | grep -i "__sighandler_t"
typedef void (*SigHandler)(int);

// echo "#include <signal.h>" | gcc -xc - -E | grep -i "SIG"
enum class Sig: u32_c {
  = 1,
};

// echo "#include <signal.h>" | gcc -xc - -E | grep -B 5 -i "__sigset_t"
// This is essentially a bitmap and the SIG.. constants are indices into the bitmap.
// We remove or add signals by setting/unsetting the bit that is indexed by the signal constant (e.g. SIGSEGV).
struct SigSet {
  u32 m_data[32];

  static SigSet init() {
    u32 data[32];
    __builtin_memset(data, 0, sizeof(data));
    return SigSet {
      .m_data = data,
    };
  }

  void fill() {
    __builtin_memset(this->m_data, u8{0xff__wbu}, sizeof(this->m_data));
  }

  void unset(sig: u32, bool* fail) {
    *fail |= sig == 0;
    u32 const s = sub_unsafe_u32(sig, 1);
    u8 const pos = s / 32;
    u8 const mask = u32{1} << (s % 32);
    this->m_data[pos] &= ~mask;
  }

  void set(sig: u32) {
    u32 const s = sub_unsafe_u32(sig, 1);
    u8 const pos = s / 32;
    u8 const mask = u32{1} << (s % 32);
    this->m_data[pos] |= mask;
  }

  bool get(sig: u32) const {
    u32 const s = sub_unsafe_u32(sig, 1);
    u8 const pos = s / 32;
    u8 const mask = u32{1} << (s % 32);
    return (this->m_data[pos] & mask) != 0;
  }
};

// taken from /usr/include/bits/sigaction.h
struct SigAction_ {
    SigHandler sa_handler;
    SigSet_ sa_mask;
    i32 sa_flags;
    void (*sa_restorer)(void);
};

struct SigAction {
  SigHandler handler;
  SigSet mask;
  i32 mask;
};

} // namespace sys

} // export namespace zx 
