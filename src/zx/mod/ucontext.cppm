export module zx:ucontext;
import :def;

export namespace zx {

namespace sys {

// Use this command to get real C definitions:
// echo '#include <ucontext.h>' | gcc -E -P -x c - | less
// Can find mcontext as struct sigcontext in LSB-Core-AMD64 document

struct MContext {
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 rdi;
    u64 rsi;
    u64 rbp;
    u64 rbx;
    u64 rdx;
    u64 rax;
    u64 rcx;
    u64 rsp;
    u64 rip;
    u64 eflags;
    u16 cs;
    u16 gs;
    u16 fs;
    u16 __pad0;
    u64 err;
    u64 trapno;
    u64 oldmask;
    u64 cr2;
    void *fpstate;
    u64 __reserved1[8];
};

struct UContext {
    u64 uc_flags;
    UContext *uc_link;
    u64 uc_stack[3];
    MContext uc_mcontext;
    u64 uc_sigmask[16];
};

} // namespace sys

} // namespace zx
