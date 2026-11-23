export module zx:syscall;
import :def;

export namespace zx {

inline u64_c syscall0(u64_c num) {
    u64_c ret;

    __asm__ volatile("syscall" : "=a"(ret) : "a"(num) : "rcx", "r11", "memory");

    return ret;
}

inline u64_c syscall1(u64_c num, u64_c arg1) {
    u64_c ret;

    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(arg1)
                     : "rcx", "r11", "memory");

    return ret;
}

inline u64_c syscall2(u64_c num, u64_c arg1, u64_c arg2) {
    u64_c ret;

    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(arg1), "S"(arg2)
                     : "rcx", "r11", "memory");

    return ret;
}

inline u64_c syscall3(u64_c num, u64_c arg1, u64_c arg2, u64_c arg3) {
    u64_c ret;

    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3)
                     : "rcx", "r11", "memory");

    return ret;
}

inline u64_c syscall4(u64_c num, u64_c arg1, u64_c arg2, u64_c arg3,
                      u64_c arg4) {
    u64_c ret;
    register u64_c r10 __asm__("r10") = arg4;

    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10)
                     : "rcx", "r11", "memory");

    return ret;
}

inline u64_c syscall5(u64_c num, u64_c arg1, u64_c arg2, u64_c arg3, u64_c arg4,
                      u64_c arg5) {
    u64_c ret;
    register u64_c r10 __asm__("r10") = arg4;
    register u64_c r8 __asm__("r8") = arg5;

    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                       "r"(r8)
                     : "rcx", "r11", "memory");

    return ret;
}

inline u64_c syscall6(u64_c num, u64_c arg1, u64_c arg2, u64_c arg3, u64_c arg4,
                      u64_c arg5, u64_c arg6) {
    u64_c ret;
    register u64_c r10 __asm__("r10") = arg4;
    register u64_c r8 __asm__("r8") = arg5;
    register u64_c r9 __asm__("r9") = arg6;

    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                       "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");

    return ret;
}

} // namespace zx
