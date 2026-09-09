export module zx:sys;
import :syscall;
import :def;
import :perf_event;
import :statx;

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

export namespace zx {

namespace sys {

// echo '#include <sys/mman.h>' | gcc -E - -dM | grep "#define MAP_"
constexpr i32 MAP_SHARED = 0x01;
constexpr i32 MAP_PRIVATE = 0x02;
constexpr i32 MAP_ANONYMOUS = 0x20;
constexpr i32 MAP_LOCKED = 0x02000;
constexpr i32 MAP_POPULATE = 0x08000;
constexpr i32 MAP_HUGETLB = 0x40000;

constexpr i32 MAP_HUGE_SHIFT = 26;
constexpr i32 MAP_HUGE_2MB = (21 << MAP_HUGE_SHIFT);
constexpr i32 MAP_HUGE_1GB = (30 << MAP_HUGE_SHIFT);

constexpr i32 PROT_NONE = 0x0;
constexpr i32 PROT_READ = 0x1;
constexpr i32 PROT_WRITE = 0x2;

// echo '#include <sys/fcntl.h>' | gcc -E -dM -xc - | grep -i "O_RD"
constexpr i32 SYS_O_RDONLY = 00;

// echo '#define _GNU_SOURCE 1
// #include <sys/fcntl.h>' | gcc -E -dM -xc - | grep -i "AT_"
constexpr i32 SYS_AT_EMPTY_PATH = 0x1000;

constexpr i32 STDIN = 0;
constexpr i32 STDOUT = 1;
constexpr i32 STDERR = 2;

// echo '#include <errno.h>' | gcc -E -dM -xc - | grep -E "^#define E"
constexpr u64 EAGAIN = 11;
constexpr u64 EMULTIHOP = 72;
constexpr u64 EUNATCH = 49;
constexpr u64 EAFNOSUPPORT = 97;
constexpr u64 ELIBMAX = 82;
constexpr u64 EREMCHG = 78;
constexpr u64 EACCES = 13;
constexpr u64 EDESTADDRREQ = 89;
constexpr u64 EILSEQ = 84;
constexpr u64 ESPIPE = 29;
constexpr u64 EMLINK = 31;
constexpr u64 EOWNERDEAD = 130;
constexpr u64 ENOTTY = 25;
constexpr u64 EBADE = 52;
constexpr u64 EBADF = 9;
constexpr u64 EBADR = 53;
constexpr u64 EADV = 68;
constexpr u64 ERANGE = 34;
constexpr u64 ECANCELED = 125;
constexpr u64 ETXTBSY = 26;
constexpr u64 ENOMEM = 12;
constexpr u64 EINPROGRESS = 115;
constexpr u64 ENOTBLK = 15;
constexpr u64 EPROTOTYPE = 91;
constexpr u64 ERESTART = 85;
constexpr u64 EISNAM = 120;
constexpr u64 ENOMSG = 42;
constexpr u64 EALREADY = 114;
constexpr u64 ETIMEDOUT = 110;
constexpr u64 ENODATA = 61;
constexpr u64 EINTR = 4;
constexpr u64 ENOLINK = 67;
constexpr u64 ECHILD = 10;
constexpr u64 EPERM = 1;
constexpr u64 ELOOP = 40;
constexpr u64 ESTALE = 116;
constexpr u64 ENOTSOCK = 88;
constexpr u64 EKEYREJECTED = 129;
constexpr u64 ENOSR = 63;
constexpr u64 EBADMSG = 74;
constexpr u64 ELNRNG = 48;
constexpr u64 ENOTUNIQ = 76;
constexpr u64 ENOSYS = 38;
constexpr u64 EDEADLK = 35;
constexpr u64 EPIPE = 32;
constexpr u64 EBFONT = 59;
constexpr u64 ERFKILL = 132;
constexpr u64 EREMOTE = 66;
constexpr u64 ETOOMANYREFS = 109;
constexpr u64 EPFNOSUPPORT = 96;
constexpr u64 EOPNOTSUPP = 95;
constexpr u64 ENOTSUP = EOPNOTSUPP;
constexpr u64 ESRMNT = 69;
constexpr u64 ENONET = 64;
constexpr u64 ENOTNAM = 118;
constexpr u64 ELIBEXEC = 83;
constexpr u64 ENOCSI = 50;
constexpr u64 ENOTEMPTY = 39;
constexpr u64 EADDRINUSE = 98;
constexpr u64 ENETRESET = 102;
constexpr u64 EISDIR = 21;
constexpr u64 EIDRM = 43;
constexpr u64 ECOMM = 70;
constexpr u64 ECONNABORTED = 103;
constexpr u64 EHOSTUNREACH = 113;
constexpr u64 EBADFD = 77;
constexpr u64 EL3HLT = 46;
constexpr u64 EL2HLT = 51;
constexpr u64 ENOKEY = 126;
constexpr u64 EINVAL = 22;
constexpr u64 EADDRNOTAVAIL = 99;
constexpr u64 ESHUTDOWN = 108;
constexpr u64 EFSBADCRC = EBADMSG;
constexpr u64 ELIBSCN = 81;
constexpr u64 ENAVAIL = 119;
constexpr u64 ENOSTR = 60;
constexpr u64 EMFILE = 24;
constexpr u64 EOVERFLOW = 75;
constexpr u64 EUCLEAN = 117;
constexpr u64 ENOMEDIUM = 123;
constexpr u64 EBUSY = 16;
constexpr u64 EPROTO = 71;
constexpr u64 ENODEV = 19;
constexpr u64 EKEYEXPIRED = 127;
constexpr u64 EROFS = 30;
constexpr u64 ELIBACC = 79;
constexpr u64 EHWPOISON = 133;
constexpr u64 E2BIG = 7;
constexpr u64 ECONNRESET = 104;
constexpr u64 ENXIO = 6;
constexpr u64 EBADRQC = 56;
constexpr u64 EL3RST = 47;
constexpr u64 ENAMETOOLONG = 36;
constexpr u64 ESOCKTNOSUPPORT = 94;
constexpr u64 EDOTDOT = 73;
constexpr u64 ETIME = 62;
constexpr u64 EPROTONOSUPPORT = 93;
constexpr u64 ENOTRECOVERABLE = 131;
constexpr u64 EIO = 5;
constexpr u64 ENETUNREACH = 101;
constexpr u64 EXDEV = 18;
constexpr u64 EDQUOT = 122;
constexpr u64 EREMOTEIO = 121;
constexpr u64 ENOSPC = 28;
constexpr u64 ENOEXEC = 8;
constexpr u64 EMSGSIZE = 90;
constexpr u64 EBADSLT = 57;
constexpr u64 EDOM = 33;
constexpr u64 EFBIG = 27;
constexpr u64 ESRCH = 3;
constexpr u64 ECHRNG = 44;
constexpr u64 EHOSTDOWN = 112;
constexpr u64 ENOLCK = 37;
constexpr u64 ENFILE = 23;
constexpr u64 ENOTCONN = 107;
constexpr u64 EDEADLOCK = EDEADLK;
constexpr u64 EFSCORRUPTED = EUCLEAN;
constexpr u64 ENOANO = 55;
constexpr u64 EISCONN = 106;
constexpr u64 EUSERS = 87;
constexpr u64 ENETDOWN = 100;
constexpr u64 ENOPROTOOPT = 92;
constexpr u64 ENOBUFS = 105;
constexpr u64 EFAULT = 14;
constexpr u64 EWOULDBLOCK = EAGAIN;
constexpr u64 ELIBBAD = 80;
constexpr u64 ESTRPIPE = 86;
constexpr u64 ECONNREFUSED = 111;
constexpr u64 EEXIST = 17;
constexpr u64 EL2NSYNC = 45;
constexpr u64 ENOENT = 2;
constexpr u64 ENOPKG = 65;
constexpr u64 EXFULL = 54;
constexpr u64 EKEYREVOKED = 128;
constexpr u64 ENOTDIR = 20;
constexpr u64 EMEDIUMTYPE = 124;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "read"
constexpr u64 NR_READ = 0;
constexpr u64 NR_WRITE = 1;
constexpr u64 NR_OPEN = 2;
constexpr u64 NR_CLOSE = 3;
constexpr u64 NR_MMAP = 9;
constexpr u64 NR_MPROTECT = 10;
constexpr u64 NR_MUNMAP = 11;
constexpr u64 NR_MADVISE = 28;
constexpr u64 NR_EXIT_GROUP = 231;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "statx"
constexpr u64 NR_STATX = 332;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "perf_event"
constexpr u64 NR_PERF_EVENT_OPEN = 298;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "io_uring"
constexpr u64 NR_IO_URING_SETUP = 425;
constexpr u64 NR_IO_URING_ENTER = 426;
constexpr u64 NR_IO_URING_REGISTER = 427;

// echo '#include <signal.h>' | gcc -E -dM -xc - | grep -E "SIGBUS"
constexpr u64 SIGSEGV = 11;
constexpr u64 SIGBUS = 7;

constexpr u64 SUCCESS = 0;

u64 errno_(u64 ret) {
    i64 const sgnd = (i64)ret;
    if (sgnd > -4096 && sgnd < 0) {
        return (u64)-sgnd;
    } else {
        return SUCCESS;
    }
}

enum class [[nodiscard]] ReadStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAgain,
    EBadF,
    EFault,
    EIntr,
    EInval,
    EIo,
    EIsDir,
};

ReadStatus read(i32 fd, u8 *buf, u64 count, u64 *n_read) {
    if (n_read == nullptr || buf == nullptr) {
        return ReadStatus::Nullptr;
    }

    u64 const res = syscall3(NR_READ, (u64)fd, (u64)buf, count);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) {
        // The returned n_read count isn't supposed to be used on error case.
        // Set to max value so the caller will do something unexpected if it
        // tries to use this value on error case. Then we can hope to catch this
        // in tests.
        *n_read = U64_MAX;

        // don't need to handle these separately since they are the same value.
        static_assert(EAGAIN == EWOULDBLOCK);

        switch (errno) {
        case EAGAIN: {
            return ReadStatus::EAgain;
        }
        case EBADF: {
            return ReadStatus::EBadF;
        }
        case EFAULT: {
            return ReadStatus::EFault;
        }
        case EINTR: {
            return ReadStatus::EIntr;
        }
        case EINVAL: {
            return ReadStatus::EInval;
        }
        case EIO: {
            return ReadStatus::EIo;
        }
        case EISDIR: {
            return ReadStatus::EIsDir;
        }
        default: {
            return ReadStatus::UnexpectedErr;
        }
        }
    }
    *n_read = res;
    return ReadStatus::Ok;
}

enum class [[nodiscard]] WriteStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAgain,
    EBadF,
    EDestAddrReq,
    EDQuot,
    EFault,
    EFBig,
    EIntr,
    EInval,
    EIo,
    ENoSpc,
    EPerm,
    EPipe,
};

WriteStatus write(i32 fd, u8 const *buf, u64 count, u64 *n_written) {
    if (n_written == nullptr || buf == nullptr) {
        return WriteStatus::Nullptr;
    }

    u64 const res = syscall3(NR_WRITE, (u64)fd, (u64)buf, count);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) {
        *n_written = U64_MAX;

        switch (errno) {
        case EAGAIN: {
            return WriteStatus::EAgain;
        }
        case EBADF: {
            return WriteStatus::EBadF;
        }
        case EDESTADDRREQ: {
            return WriteStatus::EDestAddrReq;
        }
        case EDQUOT: {
            return WriteStatus::EDQuot;
        }
        case EFAULT: {
            return WriteStatus::EFault;
        }
        case EFBIG: {
            return WriteStatus::EFBig;
        }
        case EINTR: {
            return WriteStatus::EIntr;
        }
        case EINVAL: {
            return WriteStatus::EInval;
        }
        case EIO: {
            return WriteStatus::EIo;
        }
        case ENOSPC: {
            return WriteStatus::ENoSpc;
        }
        case EPERM: {
            return WriteStatus::EPerm;
        }
        case EPIPE: {
            return WriteStatus::EPipe;
        }
        default: {
            return WriteStatus::UnexpectedErr;
        }
        }
    }

    *n_written = res;
    return WriteStatus::Ok;
}

enum class [[nodiscard]] OpenStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAcces,
    EBadF,
    EBusy,
    EDQuot,
    EExist,
    EFault,
    EFBig,
    EIntr,
    EInval,
    EIsDir,
    ELoop,
    EMFile,
    ENameTooLong,
    ENFile,
    ENoDev,
    ENoEnt,
    ENoMem,
    ENoSpc,
    ENotDir,
    ENxIo,
    EOpNotSupp,
    EOverflow,
    EPerm,
    ERoFs,
    ETxtBsy,
    EWouldBlock,
};

OpenStatus open(u8 const *path, i32 flags, u32 mode, i32 *fd) {
    if (path == nullptr || fd == nullptr) {
        return OpenStatus::Nullptr;
    }

    u64 const res = syscall3(NR_OPEN, (u64)path, (u64)flags, mode);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) {
        *fd = -1;

        switch (errno) {
        case EACCES: {
            return OpenStatus::EAcces;
        }
        case EBADF: {
            return OpenStatus::EBadF;
        }
        case EBUSY: {
            return OpenStatus::EBusy;
        }
        case EDQUOT: {
            return OpenStatus::EDQuot;
        }
        case EEXIST: {
            return OpenStatus::EExist;
        }
        case EFAULT: {
            return OpenStatus::EFault;
        }
        case EFBIG: {
            return OpenStatus::EFBig;
        }
        case EINTR: {
            return OpenStatus::EIntr;
        }
        case EINVAL: {
            return OpenStatus::EInval;
        }
        case EISDIR: {
            return OpenStatus::EIsDir;
        }
        case ELOOP: {
            return OpenStatus::ELoop;
        }
        case EMFILE: {
            return OpenStatus::EMFile;
        }
        case ENAMETOOLONG: {
            return OpenStatus::ENameTooLong;
        }
        case ENFILE: {
            return OpenStatus::ENFile;
        }
        case ENODEV: {
            return OpenStatus::ENoDev;
        }
        case ENOENT: {
            return OpenStatus::ENoEnt;
        }
        case ENOMEM: {
            return OpenStatus::ENoMem;
        }
        case ENOSPC: {
            return OpenStatus::ENoSpc;
        }
        case ENOTDIR: {
            return OpenStatus::ENotDir;
        }
        case ENXIO: {
            return OpenStatus::ENxIo;
        }
        case EOPNOTSUPP: {
            return OpenStatus::EOpNotSupp;
        }
        case EOVERFLOW: {
            return OpenStatus::EOverflow;
        }
        case EPERM: {
            return OpenStatus::EPerm;
        }
        case EROFS: {
            return OpenStatus::ERoFs;
        }
        case ETXTBSY: {
            return OpenStatus::ETxtBsy;
        }
        case EWOULDBLOCK: {
            return OpenStatus::EWouldBlock;
        }
        default: {
            return OpenStatus::UnexpectedErr;
        }
        }
    }

    *fd = (i32)res;
    return OpenStatus::Ok;
}

enum class [[nodiscard]] CloseStatus : u8_c {
    Ok = 0,
    UnexpectedErr,
    EBadF,
    EIntr,
    EIo,
    ENoSpc,
    EDQuot,
};

CloseStatus close(i32 fd) {
    u64 const res = syscall1(NR_CLOSE, (u64)fd);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) {
        switch (errno) {
        case EBADF: {
            return CloseStatus::EBadF;
        }
        case EINTR: {
            return CloseStatus::EIntr;
        }
        case EIO: {
            return CloseStatus::EIo;
        }
        case ENOSPC: {
            return CloseStatus::ENoSpc;
        }
        case EDQUOT: {
            return CloseStatus::EDQuot;
        }
        default: {
            return CloseStatus::UnexpectedErr;
        }
        }
    }

    return CloseStatus::Ok;
}

enum class [[nodiscard]] MMapStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAcces,
    EAgain,
    EBadF,
    EExist,
    EInval,
    ENFile,
    ENoDev,
    ENoMem,
    EOverflow,
    EPerm,
    ETxtBsy,
};

MMapStatus mmap_err_(u64 errno) {
    switch (errno) {
    case EACCES: {
        return MMapStatus::EAcces;
    }
    case EAGAIN: {
        return MMapStatus::EAgain;
    }
    case EBADF: {
        return MMapStatus::EBadF;
    }
    case EEXIST: {
        return MMapStatus::EExist;
    }
    case EINVAL: {
        return MMapStatus::EInval;
    }
    case ENFILE: {
        return MMapStatus::ENFile;
    }
    case ENODEV: {
        return MMapStatus::ENoDev;
    }
    case ENOMEM: {
        return MMapStatus::ENoMem;
    }
    case EOVERFLOW: {
        return MMapStatus::EOverflow;
    }
    case EPERM: {
        return MMapStatus::EPerm;
    }
    case ETXTBSY: {
        return MMapStatus::ETxtBsy;
    }
    default: {
        return MMapStatus::UnexpectedErr;
    }
    }
}

MMapStatus mmap(u8 *addr, u64 length, i32 prot, i32 flags, i32 fd, i64 offset,
                u8 **mem) {
    if (mem == nullptr) {
        return MMapStatus::Nullptr;
    }

    u64 const res = syscall6(NR_MMAP, (u64)addr, length, (u64)prot, (u64)flags,
                             (u64)fd, (u64)offset);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) {
        *mem = nullptr;
        return mmap_err_(errno);
    }

    *mem = (u8 *)res;
    return MMapStatus::Ok;
}

MMapStatus munmap(u8 *mem, u64 length) {
    if (mem == nullptr) {
        return MMapStatus::Nullptr;
    }

    u64 const res = syscall2(NR_MUNMAP, (u64)mem, length);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) {
        return mmap_err_(errno);
    }

    return MMapStatus::Ok;
}

enum class [[nodiscard]] MProtectStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAcces,
    EInval,
    ENoMem,
};

MProtectStatus mprotect(u8 *addr, u64 length, int prot) {
    if (addr == nullptr) {
        return MProtectStatus::Nullptr;
    }

    u64 const res = syscall3(NR_MPROTECT, (u64)addr, length, (u64)prot);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) {
        switch (errno) {
        case EACCES: {
            return MProtectStatus::EAcces;
        }
        case EINVAL: {
            return MProtectStatus::EInval;
        }
        case ENOMEM: {
            return MProtectStatus::ENoMem;
        }
        default: {
            return MProtectStatus::UnexpectedErr;
        }
        }
    }

    return MProtectStatus::Ok;
}

[[noreturn]] void exit_group(int status) {
    syscall1(NR_EXIT_GROUP, (u64)status);
    // should never reach this but we put it to make sure
    // that this function doesn't return and the compiler is happy
    // about the [[noreturn]] annotation.
    __builtin_trap();
}

enum class [[nodiscard]] PerfEventOpenStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedError,
    InvalidFd,
    E2Big,
    EAcces,
    EBadF,
    EBusy,
    EFault,
    EIntr,
    EInval,
    EMFile,
    ENoDev,
    ENoEnt,
    ENoSpc,
    ENoSys,
    EOpNotSupp,
    EOverflow,
    EPerm,
    ESrch,
};

PerfEventOpenStatus perf_event_open(PerfEventAttr *attr, i32 pid, i32 cpu,
                                    i32 group_fd, u64 flags, i32 *fd) {
    if (attr == nullptr || fd == nullptr) {
        return PerfEventOpenStatus::Nullptr;
    }

    u64 const res = syscall5(NR_PERF_EVENT_OPEN, (u64)attr, (u64)pid, (u64)cpu,
                             (u64)group_fd, flags);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) {
        *fd = -1;
        switch (errno) {
        case E2BIG: {
            return PerfEventOpenStatus::E2Big;
        }
        case EACCES: {
            return PerfEventOpenStatus::EAcces;
        }
        case EBADF: {
            return PerfEventOpenStatus::EBadF;
        }
        case EBUSY: {
            return PerfEventOpenStatus::EBusy;
        }
        case EFAULT: {
            return PerfEventOpenStatus::EFault;
        }
        case EINTR: {
            return PerfEventOpenStatus::EIntr;
        }
        case EINVAL: {
            return PerfEventOpenStatus::EInval;
        }
        case EMFILE: {
            return PerfEventOpenStatus::EMFile;
        }
        case ENODEV: {
            return PerfEventOpenStatus::ENoDev;
        }
        case ENOENT: {
            return PerfEventOpenStatus::ENoEnt;
        }
        case ENOSPC: {
            return PerfEventOpenStatus::ENoSpc;
        }
        case ENOSYS: {
            return PerfEventOpenStatus::ENoSys;
        }
        case EOPNOTSUPP: {
            return PerfEventOpenStatus::EOpNotSupp;
        }
        case EOVERFLOW: {
            return PerfEventOpenStatus::EOverflow;
        }
        case EPERM: {
            return PerfEventOpenStatus::EPerm;
        }
        case ESRCH: {
            return PerfEventOpenStatus::ESrch;
        }
        default: {
            return PerfEventOpenStatus::UnexpectedError;
        }
        }
    }
    i32 const ret_fd = (i32)res;
    if (ret_fd <= 0) {
        *fd = -1;
        return PerfEventOpenStatus::InvalidFd;
    }

    *fd = ret_fd;
    return PerfEventOpenStatus::Ok;
}

enum class [[nodiscard]] StatXStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedError,
    EAcces,
    EBadF,
    EFault,
    EInval,
    ELoop,
    ENameTooLong,
    ENoEnt,
    ENoMem,
    ENotDir,
};

StatXStatus statx(i32 dirfd, char const *path, i32 flags, u32 mask,
                  StatX *statxbuf) {
    if (statxbuf == nullptr) {
        return StatXStatus::Nullptr;
    }
    __builtin_memset(statxbuf, 0, sizeof(StatX));
    u64 const res = syscall5(NR_STATX, (u64)dirfd, (u64)path, (u64)flags,
                             (u64)mask, (u64)statxbuf);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) {
        switch (errno) {
        case EACCES: {
            return StatXStatus::EAcces;
        }
        case EBADF: {
            return StatXStatus::EBadF;
        }
        case EFAULT: {
            return StatXStatus::EFault;
        }
        case EINVAL: {
            return StatXStatus::EInval;
        }
        case ELOOP: {
            return StatXStatus::ELoop;
        }
        case ENAMETOOLONG: {
            return StatXStatus::ENameTooLong;
        }
        case ENOENT: {
            return StatXStatus::ENoEnt;
        }
        case ENOMEM: {
            return StatXStatus::ENoMem;
        }
        case ENOTDIR: {
            return StatXStatus::ENotDir;
        }
        default: {
            return StatXStatus::UnexpectedError;
        }
        }
    }

    return StatXStatus::Ok;
}

} // namespace sys

} // namespace zx
