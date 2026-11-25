// The errors sets on this impl should correspond 1-to-1 with the linux manpages
// section (2). Section (3) is for library functions, section (2) is for the
// direct systemcalls.

export module zx:sys;
import :syscall;
import :def;
import :perf_event;
import :statx;
import :io_uring;
import :signal;
import :backtrace;

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
constexpr i32 PROT_EXEC = 0x4;

// echo '#include <sys/fcntl.h>' | gcc -E -dM -xc - | grep -i "O_"
constexpr i32 O_RDONLY = 00;
constexpr i32 O_WRONLY = 01;
constexpr i32 O_RDWR = 02;
constexpr i32 O_NOCTTY = 0400;
constexpr i32 O_ACCMODE = 0003;
constexpr i32 O_NONBLOCK = 04000;
constexpr i32 O_NDELAY = O_NONBLOCK;
constexpr i32 O_ASYNC = 020000;
constexpr i32 O_SYNC = 04010000;
constexpr i32 O_RSYNC = O_SYNC;
constexpr i32 O_FSYNC = O_SYNC;
constexpr i32 O_DSYNC = 010000;
constexpr i32 O_APPEND = 02000;
constexpr i32 O_DIRECTORY = 0200000;
constexpr i32 O_EXCL = 0200;
constexpr i32 O_CLOEXEC = 02000000;
constexpr i32 O_CREAT = 0100;
constexpr i32 O_TRUNC = 01000;
constexpr i32 O_DIRECT = 040000;
constexpr i32 O_NOFOLLOW = 0400000;
constexpr i32 O_PATH = 010000000;
constexpr i32 O_TMPFILE = (020000000 | O_DIRECTORY);

// echo '#include <sys/fcntl.h>' | gcc -E -dM -xc - | grep -i "S_"
constexpr u32 S_IFBLK = 0060000;
constexpr u32 S_IFIFO = 0010000;
constexpr u32 S_IREAD = 0400;
constexpr u32 S_ISVTX = 01000;
constexpr u32 S_IFCHR = 0020000;
constexpr u32 S_IFREG = 0100000;
constexpr u32 S_IFDIR = 0040000;
constexpr u32 S_IWRITE = 0200;
constexpr u32 S_IEXEC = 0100;
constexpr u32 S_IFLNK = 0120000;
constexpr u32 S_IFSOCK = 0140000;
constexpr u32 S_ISUID = 04000;
constexpr u32 S_IFMT = 0170000;
constexpr u32 S_ISGID = 02000;
constexpr u32 S_IWUSR = S_IWRITE;
constexpr u32 S_IXUSR = S_IEXEC;
constexpr u32 S_IXGRP = (S_IXUSR >> 3);
constexpr u32 S_IXOTH = (S_IXGRP >> 3);
constexpr u32 S_IRWXU = (S_IREAD | S_IWRITE | S_IEXEC);
constexpr u32 S_IWGRP = (S_IWUSR >> 3);
constexpr u32 S_IRWXG = (S_IRWXU >> 3);
constexpr u32 S_IRWXO = (S_IRWXG >> 3);
constexpr u32 S_IRUSR = S_IREAD;
constexpr u32 S_IWOTH = (S_IWGRP >> 3);
constexpr u32 S_IRGRP = (S_IRUSR >> 3);
constexpr u32 S_IROTH = (S_IRGRP >> 3);

// echo '#define _GNU_SOURCE 1
// #include <sys/fcntl.h>' | gcc -E -dM -xc - | grep -i "AT_"
constexpr i32 AT_EMPTY_PATH = 0x1000;
constexpr i32 AT_REMOVEDIR = 0x200;
constexpr i32 AT_NO_AUTOMOUNT = 0x800;
constexpr i32 AT_SYMLINK_NOFOLLOW = 0x100;
constexpr i32 AT_STATX_SYNC_AS_STAT = 0x0000;
constexpr i32 AT_STATX_FORCE_SYNC = 0x2000;
constexpr i32 AT_STATX_DONT_SYNC = 0x4000;
constexpr i32 AT_FDCWD = -100;

constexpr i32 STDIN = 0;
constexpr i32 STDOUT = 1;
constexpr i32 STDERR = 2;

// grep -n "EXIT_" /usr/include/stdlib.h
constexpr i32 EXIT_FAILURE = 1;
constexpr i32 EXIT_SUCCESS = 0;

// echo '#include <sys/types.h>' | gcc -E -dM | grep -E "SEEK_"
constexpr i32 SEEK_SET = 0;
constexpr i32 SEEK_CUR = 1;
constexpr i32 SEEK_END = 2;

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
constexpr u64 NR_UNLINKAT = 263;
constexpr u64 NR_PREAD64 = 17;
constexpr u64 NR_PWRITE64 = 18;
constexpr u64 NR_LSEEK = 8;
constexpr u64 NR_GETRANDOM = 318;
constexpr u64 NR_FTRUNCATE = 77;
constexpr u64 NR_RT_SIGRETURN = 15;
constexpr u64 NR_RT_SIGACTION = 13;
constexpr u64 NR_RT_SIGALTSTACK = 131;
constexpr u64 NR_PRCTL = 157;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "statx"
constexpr u64 NR_STATX = 332;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "perf_event"
constexpr u64 NR_PERF_EVENT_OPEN = 298;

// echo '#include <sys/syscall.h>' | gcc -E -dM -xc - | grep -i "io_uring"
constexpr u64 NR_IO_URING_SETUP = 425;
constexpr u64 NR_IO_URING_ENTER = 426;
constexpr u64 NR_IO_URING_REGISTER = 427;

// echo '#include <sys/random.h>' | gcc -E -dM -xc - | grep -E "GRND"
constexpr u32 GRND_INSECURE = 0x04;
constexpr u32 GRND_NONBLOCK = 0x01;
constexpr u32 GRND_RANDOM = 0x02;

constexpr u64 SUCCESS = 0;

inline u64 errno_(u64 ret) {
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

inline ReadStatus read(Backtrace *err_bt, i32 fd, u8 *buf, u64 count,
                       u64 *n_read) {
    if (n_read == nullptr || buf == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return ReadStatus::Nullptr;
    }

    u64 const res = syscall3(NR_READ, (u64)fd, (u64)buf, count);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        // The returned n_read count isn't supposed to be used on error case.
        // Set to max value so the caller will do something unexpected if it
        // tries to use this value on error case. Then we can hope to catch this
        // in tests.
        *n_read = U64_MAX;

        // don't need to handle these separately since they are the same value.
        static_assert(EAGAIN == EWOULDBLOCK);

        switch (errno) {
        case EAGAIN: {
            Backtrace::collect(err_bt);
            return ReadStatus::EAgain;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return ReadStatus::EBadF;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return ReadStatus::EFault;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return ReadStatus::EIntr;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return ReadStatus::EInval;
        }
        case EIO: {
            Backtrace::collect(err_bt);
            return ReadStatus::EIo;
        }
        case EISDIR: {
            Backtrace::collect(err_bt);
            return ReadStatus::EIsDir;
        }
        default: {
            Backtrace::collect(err_bt);
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

inline WriteStatus write(Backtrace *err_bt, i32 fd, u8 const *buf, u64 count,
                         u64 *n_written) {
    if (n_written == nullptr || buf == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return WriteStatus::Nullptr;
    }

    u64 const res = syscall3(NR_WRITE, (u64)fd, (u64)buf, count);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        *n_written = U64_MAX;

        switch (errno) {
        case EAGAIN: {
            Backtrace::collect(err_bt);
            return WriteStatus::EAgain;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return WriteStatus::EBadF;
        }
        case EDESTADDRREQ: {
            Backtrace::collect(err_bt);
            return WriteStatus::EDestAddrReq;
        }
        case EDQUOT: {
            Backtrace::collect(err_bt);
            return WriteStatus::EDQuot;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return WriteStatus::EFault;
        }
        case EFBIG: {
            Backtrace::collect(err_bt);
            return WriteStatus::EFBig;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return WriteStatus::EIntr;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return WriteStatus::EInval;
        }
        case EIO: {
            Backtrace::collect(err_bt);
            return WriteStatus::EIo;
        }
        case ENOSPC: {
            Backtrace::collect(err_bt);
            return WriteStatus::ENoSpc;
        }
        case EPERM: {
            Backtrace::collect(err_bt);
            return WriteStatus::EPerm;
        }
        case EPIPE: {
            Backtrace::collect(err_bt);
            return WriteStatus::EPipe;
        }
        default: {
            Backtrace::collect(err_bt);
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
    ENXIo,
    EOpNotSupp,
    EOverflow,
    EPerm,
    ERoFs,
    ETxtBsy,
    EWouldBlock,
};

inline OpenStatus open(Backtrace *err_bt, u8 const *path, i32 flags, u32 mode,
                       i32 *fd) {
    if (path == nullptr || fd == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return OpenStatus::Nullptr;
    }

    u64 const res = syscall3(NR_OPEN, (u64)path, (u64)flags, mode);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        *fd = -1;

        switch (errno) {
        case EACCES: {
            Backtrace::collect(err_bt);
            return OpenStatus::EAcces;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return OpenStatus::EBadF;
        }
        case EBUSY: {
            Backtrace::collect(err_bt);
            return OpenStatus::EBusy;
        }
        case EDQUOT: {
            Backtrace::collect(err_bt);
            return OpenStatus::EDQuot;
        }
        case EEXIST: {
            Backtrace::collect(err_bt);
            return OpenStatus::EExist;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return OpenStatus::EFault;
        }
        case EFBIG: {
            Backtrace::collect(err_bt);
            return OpenStatus::EFBig;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return OpenStatus::EIntr;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return OpenStatus::EInval;
        }
        case EISDIR: {
            Backtrace::collect(err_bt);
            return OpenStatus::EIsDir;
        }
        case ELOOP: {
            Backtrace::collect(err_bt);
            return OpenStatus::ELoop;
        }
        case EMFILE: {
            Backtrace::collect(err_bt);
            return OpenStatus::EMFile;
        }
        case ENAMETOOLONG: {
            Backtrace::collect(err_bt);
            return OpenStatus::ENameTooLong;
        }
        case ENFILE: {
            Backtrace::collect(err_bt);
            return OpenStatus::ENFile;
        }
        case ENODEV: {
            Backtrace::collect(err_bt);
            return OpenStatus::ENoDev;
        }
        case ENOENT: {
            Backtrace::collect(err_bt);
            return OpenStatus::ENoEnt;
        }
        case ENOMEM: {
            Backtrace::collect(err_bt);
            return OpenStatus::ENoMem;
        }
        case ENOSPC: {
            Backtrace::collect(err_bt);
            return OpenStatus::ENoSpc;
        }
        case ENOTDIR: {
            Backtrace::collect(err_bt);
            return OpenStatus::ENotDir;
        }
        case ENXIO: {
            Backtrace::collect(err_bt);
            return OpenStatus::ENXIo;
        }
        case EOPNOTSUPP: {
            Backtrace::collect(err_bt);
            return OpenStatus::EOpNotSupp;
        }
        case EOVERFLOW: {
            Backtrace::collect(err_bt);
            return OpenStatus::EOverflow;
        }
        case EPERM: {
            Backtrace::collect(err_bt);
            return OpenStatus::EPerm;
        }
        case EROFS: {
            Backtrace::collect(err_bt);
            return OpenStatus::ERoFs;
        }
        case ETXTBSY: {
            Backtrace::collect(err_bt);
            return OpenStatus::ETxtBsy;
        }
        case EWOULDBLOCK: {
            Backtrace::collect(err_bt);
            return OpenStatus::EWouldBlock;
        }
        default: {
            Backtrace::collect(err_bt);
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

inline CloseStatus close(Backtrace *err_bt, i32 fd) {
    u64 const res = syscall1(NR_CLOSE, (u64)fd);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        switch (errno) {
        case EBADF: {
            Backtrace::collect(err_bt);
            return CloseStatus::EBadF;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return CloseStatus::EIntr;
        }
        case EIO: {
            Backtrace::collect(err_bt);
            return CloseStatus::EIo;
        }
        case ENOSPC: {
            Backtrace::collect(err_bt);
            return CloseStatus::ENoSpc;
        }
        case EDQUOT: {
            Backtrace::collect(err_bt);
            return CloseStatus::EDQuot;
        }
        default: {
            Backtrace::collect(err_bt);
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

inline MMapStatus mmap_err_(Backtrace *err_bt, u64 errno) {
    switch (errno) {
    case EACCES: {
        Backtrace::collect(err_bt);
        return MMapStatus::EAcces;
    }
    case EAGAIN: {
        Backtrace::collect(err_bt);
        return MMapStatus::EAgain;
    }
    case EBADF: {
        Backtrace::collect(err_bt);
        return MMapStatus::EBadF;
    }
    case EEXIST: {
        Backtrace::collect(err_bt);
        return MMapStatus::EExist;
    }
    case EINVAL: {
        Backtrace::collect(err_bt);
        return MMapStatus::EInval;
    }
    case ENFILE: {
        Backtrace::collect(err_bt);
        return MMapStatus::ENFile;
    }
    case ENODEV: {
        Backtrace::collect(err_bt);
        return MMapStatus::ENoDev;
    }
    case ENOMEM: {
        Backtrace::collect(err_bt);
        return MMapStatus::ENoMem;
    }
    case EOVERFLOW: {
        Backtrace::collect(err_bt);
        return MMapStatus::EOverflow;
    }
    case EPERM: {
        Backtrace::collect(err_bt);
        return MMapStatus::EPerm;
    }
    case ETXTBSY: {
        Backtrace::collect(err_bt);
        return MMapStatus::ETxtBsy;
    }
    default: {
        Backtrace::collect(err_bt);
        return MMapStatus::UnexpectedErr;
    }
    }
}

inline MMapStatus mmap(Backtrace *err_bt, u8 *addr, u64 length, i32 prot,
                       i32 flags, i32 fd, i64 offset, u8 **mem) {
    if (mem == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return MMapStatus::Nullptr;
    }

    u64 const res = syscall6(NR_MMAP, (u64)addr, length, (u64)prot, (u64)flags,
                             (u64)fd, (u64)offset);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) [[unlikely]] {
        *mem = nullptr;
        return mmap_err_(err_bt, errno);
    }

    *mem = (u8 *)res;
    return MMapStatus::Ok;
}

inline MMapStatus munmap(Backtrace *err_bt, u8 *mem, u64 length) {
    if (mem == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return MMapStatus::Nullptr;
    }

    u64 const res = syscall2(NR_MUNMAP, (u64)mem, length);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) [[unlikely]] {
        return mmap_err_(err_bt, errno);
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

inline MProtectStatus mprotect(Backtrace *err_bt, u8 *addr, u64 length,
                               i32 prot) {
    if (addr == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return MProtectStatus::Nullptr;
    }

    u64 const res = syscall3(NR_MPROTECT, (u64)addr, length, (u64)prot);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) [[unlikely]] {
        switch (errno) {
        case EACCES: {
            Backtrace::collect(err_bt);
            return MProtectStatus::EAcces;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return MProtectStatus::EInval;
        }
        case ENOMEM: {
            Backtrace::collect(err_bt);
            return MProtectStatus::ENoMem;
        }
        default: {
            Backtrace::collect(err_bt);
            return MProtectStatus::UnexpectedErr;
        }
        }
    }

    return MProtectStatus::Ok;
}

[[noreturn]] inline void exit_group(i32 status) {
    syscall1(NR_EXIT_GROUP, (u64)status);
    // should never reach this but we put it to make sure
    // that this function doesn't return and the compiler is happy
    // about the [[noreturn]] annotation.
    __builtin_trap();
}

enum class [[nodiscard]] PerfEventOpenStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
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

inline PerfEventOpenStatus perf_event_open(Backtrace *err_bt,
                                           perf_event::PerfEventAttr *attr,
                                           i32 pid, i32 cpu, i32 group_fd,
                                           u64 flags, i32 *fd) {
    if (attr == nullptr || fd == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return PerfEventOpenStatus::Nullptr;
    }

    u64 const res = syscall5(NR_PERF_EVENT_OPEN, (u64)attr, (u64)pid, (u64)cpu,
                             (u64)group_fd, flags);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) [[unlikely]] {
        *fd = -1;
        switch (errno) {
        case E2BIG: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::E2Big;
        }
        case EACCES: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EAcces;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EBadF;
        }
        case EBUSY: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EBusy;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EFault;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EIntr;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EInval;
        }
        case EMFILE: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EMFile;
        }
        case ENODEV: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::ENoDev;
        }
        case ENOENT: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::ENoEnt;
        }
        case ENOSPC: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::ENoSpc;
        }
        case ENOSYS: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::ENoSys;
        }
        case EOPNOTSUPP: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EOpNotSupp;
        }
        case EOVERFLOW: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EOverflow;
        }
        case EPERM: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::EPerm;
        }
        case ESRCH: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::ESrch;
        }
        default: {
            Backtrace::collect(err_bt);
            return PerfEventOpenStatus::UnexpectedErr;
        }
        }
    }
    i32 const ret_fd = (i32)res;
    if (ret_fd < 0) [[unlikely]] {
        Backtrace::collect(err_bt);
        *fd = -1;
        return PerfEventOpenStatus::InvalidFd;
    }

    *fd = ret_fd;
    return PerfEventOpenStatus::Ok;
}

enum class [[nodiscard]] StatXStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
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

inline StatXStatus statx(Backtrace *err_bt, i32 dirfd, u8 const *path,
                         i32 flags, u32 mask, statx::StatX *statxbuf) {
    if (statxbuf == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return StatXStatus::Nullptr;
    }
    __builtin_memset(statxbuf, 0, sizeof(statx::StatX));
    u64 const res = syscall5(NR_STATX, (u64)dirfd, (u64)path, (u64)flags,
                             (u64)mask, (u64)statxbuf);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        switch (errno) {
        case EACCES: {
            Backtrace::collect(err_bt);
            return StatXStatus::EAcces;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return StatXStatus::EBadF;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return StatXStatus::EFault;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return StatXStatus::EInval;
        }
        case ELOOP: {
            Backtrace::collect(err_bt);
            return StatXStatus::ELoop;
        }
        case ENAMETOOLONG: {
            Backtrace::collect(err_bt);
            return StatXStatus::ENameTooLong;
        }
        case ENOENT: {
            Backtrace::collect(err_bt);
            return StatXStatus::ENoEnt;
        }
        case ENOMEM: {
            Backtrace::collect(err_bt);
            return StatXStatus::ENoMem;
        }
        case ENOTDIR: {
            Backtrace::collect(err_bt);
            return StatXStatus::ENotDir;
        }
        default: {
            Backtrace::collect(err_bt);
            return StatXStatus::UnexpectedErr;
        }
        }
    }

    return StatXStatus::Ok;
}

enum class [[nodiscard]] IoUringSetupStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EFault,
    EInval,
    EMFile,
    ENFile,
    ENoMem,
    EPerm,
    ENXIo,
};

inline IoUringSetupStatus io_uring_setup(Backtrace *err_bt, u32 n_entries,
                                         io_uring::IoUringParams *params,
                                         i32 *fd) {
    if (fd == nullptr || params == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return IoUringSetupStatus::Nullptr;
    }

    u64 const res = syscall2(NR_IO_URING_SETUP, n_entries, (u64)params);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        *fd = -1;
        switch (errno) {
        case EFAULT: {
            Backtrace::collect(err_bt);
            return IoUringSetupStatus::EFault;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return IoUringSetupStatus::EInval;
        }
        case EMFILE: {
            Backtrace::collect(err_bt);
            return IoUringSetupStatus::EMFile;
        }
        case ENFILE: {
            Backtrace::collect(err_bt);
            return IoUringSetupStatus::ENFile;
        }
        case ENOMEM: {
            Backtrace::collect(err_bt);
            return IoUringSetupStatus::ENoMem;
        }
        case EPERM: {
            Backtrace::collect(err_bt);
            return IoUringSetupStatus::EPerm;
        }
        case ENXIO: {
            Backtrace::collect(err_bt);
            return IoUringSetupStatus::ENXIo;
        }
        default: {
            Backtrace::collect(err_bt);
            return IoUringSetupStatus::UnexpectedErr;
        }
        }
    }

    *fd = (i32)res;
    return IoUringSetupStatus::Ok;
}

enum class [[nodiscard]] IoUringEnterStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAgain,
    EBadF,
    EBadFd,
    EBadR,
    EBusy,
    EExist,
    EInval,
    EFault,
    ENXIo,
    EOpNotSupp,
    EIntr,
    EOwnerDead,
};

inline IoUringEnterStatus io_uring_enter(Backtrace *err_bt, i32 fd,
                                         u32 to_submit, u32 min_complete,
                                         u32 flags, void *arg, u64 sz,
                                         u32 *n_submitted) {
    if (n_submitted == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return IoUringEnterStatus::Nullptr;
    }

    u64 const res = syscall6(NR_IO_URING_ENTER, (u64)fd, to_submit,
                             min_complete, flags, (u64)arg, sz);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        *n_submitted = U32_MAX;

        switch (errno) {
        case EAGAIN: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EAgain;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EBadF;
        }
        case EBADFD: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EBadFd;
        }
        case EBADR: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EBadR;
        }
        case EBUSY: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EBusy;
        }
        case EEXIST: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EExist;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EInval;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EFault;
        }
        case ENXIO: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::ENXIo;
        }
        case EOPNOTSUPP: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EOpNotSupp;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EIntr;
        }
        case EOWNERDEAD: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::EOwnerDead;
        }
        default: {
            Backtrace::collect(err_bt);
            return IoUringEnterStatus::UnexpectedErr;
        }
        }
    }

    *n_submitted = (u32)res;
    return IoUringEnterStatus::Ok;
}

enum class [[nodiscard]] IoUringRegisterStatus : u8_c {
    Ok = 0,
    UnexpectedErr,
    EAcces,
    EBadF,
    EBadFd,
    EBusy,
    EExist,
    EFault,
    EInval,
    EMFile,
    ENoMem,
    ENXIo,
    EOpNotSupp,
    ENoEnt,
};

inline IoUringRegisterStatus io_uring_register(Backtrace *err_bt, i32 fd,
                                               u32 opcode, void const *arg,
                                               u32 nr_args) {
    u64 const res =
        syscall4(NR_IO_URING_REGISTER, (u64)fd, opcode, (u64)arg, nr_args);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        switch (errno) {
        case EACCES: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::EAcces;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::EBadF;
        }
        case EBADFD: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::EBadFd;
        }
        case EBUSY: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::EBusy;
        }
        case EEXIST: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::EExist;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::EFault;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::EInval;
        }
        case EMFILE: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::EMFile;
        }
        case ENOMEM: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::ENoMem;
        }
        case ENXIO: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::ENXIo;
        }
        case EOPNOTSUPP: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::EOpNotSupp;
        }
        case ENOENT: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::ENoEnt;
        }
        default: {
            Backtrace::collect(err_bt);
            return IoUringRegisterStatus::UnexpectedErr;
        }
        }
    }
    return IoUringRegisterStatus::Ok;
}

enum class [[nodiscard]] UnlinkAtStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAcces,
    EBusy,
    EFault,
    EIo,
    EIsDir,
    ELoop,
    ENameTooLong,
    ENoEnt,
    ENoMem,
    ENotDir,
    EPerm,
    ERoFs,
    EBadF,
    EInval,
};

inline UnlinkAtStatus unlink_at(Backtrace *err_bt, i32 dirfd,
                                u8 const *pathname, i32 flags) {
    if (pathname == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return UnlinkAtStatus::Nullptr;
    }

    u64 const res =
        syscall3(NR_UNLINKAT, (u64)dirfd, (u64)pathname, (u64)flags);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        switch (errno) {
        case EACCES: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::EAcces;
        }
        case EBUSY: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::EBusy;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::EFault;
        }
        case EIO: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::EIo;
        }
        case EISDIR: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::EIsDir;
        }
        case ELOOP: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::ELoop;
        }
        case ENAMETOOLONG: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::ENameTooLong;
        }
        case ENOENT: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::ENoEnt;
        }
        case ENOMEM: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::ENoMem;
        }
        case ENOTDIR: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::ENotDir;
        }
        case EPERM: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::EPerm;
        }
        case EROFS: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::ERoFs;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::EBadF;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::EInval;
        }
        default: {
            Backtrace::collect(err_bt);
            return UnlinkAtStatus::UnexpectedErr;
        }
        }
    }

    return UnlinkAtStatus::Ok;
}

enum class [[nodiscard]] PReadStatus : u8_c {
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
    ENXIo,
    EOverflow,
    ESPipe,
};

inline PReadStatus pread(Backtrace *err_bt, i32 fd, u8 *buf, u64 count,
                         i64 offset, u64 *n_read) {
    if (n_read == nullptr || buf == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return PReadStatus::Nullptr;
    }

    u64 const res = syscall4(NR_PREAD64, (u64)fd, (u64)buf, count, (u64)offset);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        // The returned n_read count isn't supposed to be used on error case.
        // Set to max value so the caller will do something unexpected if it
        // tries to use this value on error case. Then we can hope to catch this
        // in tests.
        *n_read = U64_MAX;

        // don't need to handle these separately since they are the same value.
        static_assert(EAGAIN == EWOULDBLOCK);

        switch (errno) {
        case EAGAIN: {
            Backtrace::collect(err_bt);
            return PReadStatus::EAgain;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return PReadStatus::EBadF;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return PReadStatus::EFault;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return PReadStatus::EIntr;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return PReadStatus::EInval;
        }
        case EIO: {
            Backtrace::collect(err_bt);
            return PReadStatus::EIo;
        }
        case EISDIR: {
            Backtrace::collect(err_bt);
            return PReadStatus::EIsDir;
        }
        case ENXIO: {
            Backtrace::collect(err_bt);
            return PReadStatus::ENXIo;
        }
        case EOVERFLOW: {
            Backtrace::collect(err_bt);
            return PReadStatus::EOverflow;
        }
        case ESPIPE: {
            Backtrace::collect(err_bt);
            return PReadStatus::ESPipe;
        }
        default: {
            Backtrace::collect(err_bt);
            return PReadStatus::UnexpectedErr;
        }
        }
    }
    *n_read = res;
    return PReadStatus::Ok;
}

enum class [[nodiscard]] PWriteStatus : u8_c {
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
    ENXIo,
    EOverflow,
    ESPipe,
};

inline PWriteStatus pwrite(Backtrace *err_bt, i32 fd, u8 const *buf, u64 count,
                           i64 offset, u64 *n_written) {
    if (n_written == nullptr || buf == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return PWriteStatus::Nullptr;
    }

    u64 const res =
        syscall4(NR_PWRITE64, (u64)fd, (u64)buf, count, (u64)offset);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        *n_written = U64_MAX;

        switch (errno) {
        case EAGAIN: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EAgain;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EBadF;
        }
        case EDESTADDRREQ: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EDestAddrReq;
        }
        case EDQUOT: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EDQuot;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EFault;
        }
        case EFBIG: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EFBig;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EIntr;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EInval;
        }
        case EIO: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EIo;
        }
        case ENOSPC: {
            Backtrace::collect(err_bt);
            return PWriteStatus::ENoSpc;
        }
        case EPERM: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EPerm;
        }
        case EPIPE: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EPipe;
        }
        case ENXIO: {
            Backtrace::collect(err_bt);
            return PWriteStatus::ENXIo;
        }
        case EOVERFLOW: {
            Backtrace::collect(err_bt);
            return PWriteStatus::EOverflow;
        }
        case ESPIPE: {
            Backtrace::collect(err_bt);
            return PWriteStatus::ESPipe;
        }
        default: {
            Backtrace::collect(err_bt);
            return PWriteStatus::UnexpectedErr;
        }
        }
    }

    *n_written = res;
    return PWriteStatus::Ok;
}

enum class [[nodiscard]] LSeekStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EBadF,
    EInval,
    ENXIo,
    EOverflow,
    ESPipe,
};

inline LSeekStatus lseek(Backtrace *err_bt, i32 fd, i64 offset, i32 whence,
                         i64 *new_offset) {
    if (new_offset == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return LSeekStatus::Nullptr;
    }

    u64 const res = syscall3(NR_LSEEK, (u64)fd, (u64)offset, (u64)whence);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) [[unlikely]] {
        // Make the output invalid so callers can detect misuse if they
        // ignore the returned status.
        *new_offset = I64_MIN;

        switch (errno) {
        case EBADF: {
            Backtrace::collect(err_bt);
            return LSeekStatus::EBadF;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return LSeekStatus::EInval;
        }
        case ENXIO: {
            Backtrace::collect(err_bt);
            return LSeekStatus::ENXIo;
        }
        case EOVERFLOW: {
            Backtrace::collect(err_bt);
            return LSeekStatus::EOverflow;
        }
        case ESPIPE: {
            Backtrace::collect(err_bt);
            return LSeekStatus::ESPipe;
        }
        default: {
            Backtrace::collect(err_bt);
            return LSeekStatus::UnexpectedErr;
        }
        }
    }

    *new_offset = (i64)res;
    return LSeekStatus::Ok;
}

enum class [[nodiscard]] GetRandomStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EAgain,
    EFault,
    EIntr,
    EInval,
    ENoSys,
};

inline GetRandomStatus getrandom(Backtrace *err_bt, u8 *buf, u64 size,
                                 u32 flags, u64 *n_read) {
    if (buf == nullptr || n_read == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return GetRandomStatus::Nullptr;
    }

    u64 const res = syscall3(NR_GETRANDOM, (u64)buf, size, (u64)flags);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        *n_read = U64_MAX;

        switch (errno) {
        case EAGAIN: {
            Backtrace::collect(err_bt);
            return GetRandomStatus::EAgain;
        }
        case EFAULT: {
            Backtrace::collect(err_bt);
            return GetRandomStatus::EFault;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return GetRandomStatus::EIntr;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return GetRandomStatus::EInval;
        }
        case ENOSYS: {
            Backtrace::collect(err_bt);
            return GetRandomStatus::ENoSys;
        }
        default: {
            Backtrace::collect(err_bt);
            return GetRandomStatus::UnexpectedErr;
        }
        }
    }

    *n_read = res;
    return GetRandomStatus::Ok;
}

enum class [[nodiscard]] FTruncateStatus : u8_c {
    Ok = 0,
    UnexpectedErr,
    EFBig,
    ENoSpc,
    EIntr,
    EInval,
    EIo,
    EPerm,
    ERoFs,
    ETxtBsy,
    EBadF,
};

inline FTruncateStatus ftruncate(Backtrace *err_bt, i32 fildes, i64 length) {
    u64 const res = syscall2(NR_FTRUNCATE, (u64)fildes, (u64)length);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) [[unlikely]] {
        switch (errno) {
        case EFBIG: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::EFBig;
        }
        case ENOSPC: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::ENoSpc;
        }
        case EINTR: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::EIntr;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::EInval;
        }
        case EIO: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::EIo;
        }
        case EPERM: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::EPerm;
        }
        case EROFS: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::ERoFs;
        }
        case ETXTBSY: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::ETxtBsy;
        }
        case EBADF: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::EBadF;
        }
        default: {
            Backtrace::collect(err_bt);
            return FTruncateStatus::UnexpectedErr;
        }
        }
    }

    return FTruncateStatus::Ok;
}

// internal trampoline to be used for sigaction impl.
// https://www.man7.org/linux/man-pages/man2/sigreturn.2.html
//
// It essentially just returns control back to the kernel.
__attribute__((naked, noreturn)) void sigreturn_() {
    __asm__ volatile("movq $15, %rax\n\t" // __NR_rt_sigreturn
                     "syscall\n\t"
                     "ud2\n\t");
}

enum class [[nodiscard]] SigActionStatus : u8_c {
    Ok = 0,
    UnexpectedErr,
    EFault,
    EInval,
};

inline SigActionStatus sigaction(Backtrace *err_bt, signal::SIG signum,
                                 signal::SigAction const *__restrict act,
                                 signal::SigAction *__restrict oldact) {
    signal::SigAction_ k_act;
    if (act != nullptr) {
        k_act = signal::SigAction_{
            .sa_sigaction = act->sigaction,
            .sa_mask = act->mask,
            .sa_flags = act->flags | signal::SA_RESTORER | signal::SA_SIGINFO,
            .sa_restorer = sigreturn_,
        };
    }
    signal::SigAction_ k_oldact;
    signal::SigAction_ const *new_ptr = (act == nullptr) ? nullptr : &k_act;
    signal::SigAction_ *old_ptr = (oldact == nullptr) ? nullptr : &k_oldact;
    u64 const res = syscall4(NR_RT_SIGACTION, u64{(u32)signum}, (u64)new_ptr,
                             (u64)old_ptr, sizeof(signal::SigSet));
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        switch (errno) {
        case EFAULT: {
            Backtrace::collect(err_bt);
            return SigActionStatus::EFault;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return SigActionStatus::EInval;
        }
        default: {
            Backtrace::collect(err_bt);
            return SigActionStatus::UnexpectedErr;
        }
        }
    }

    if (oldact != nullptr) {
        *oldact = signal::SigAction{
            .sigaction = k_oldact.sa_sigaction,
            .mask = k_oldact.sa_mask,
            .flags = k_oldact.sa_flags,
        };
    }

    return SigActionStatus::Ok;
}

enum class [[nodiscard]] SigAltStackStatus : u8_c {
    Ok = 0,
    UnexpectedErr,
    EFault,
    EInval,
    ENoMem,
    EPerm,
};

inline SigAltStackStatus sigaltstack(Backtrace *err_bt,
                                     signal::Stack const *__restrict ss,
                                     signal::Stack *__restrict old_ss) {
    u64 const res = syscall2(NR_RT_SIGALTSTACK, (u64)ss, (u64)old_ss);
    u64 const errno = errno_(res);

    if (errno != SUCCESS) [[unlikely]] {
        switch (errno) {
        case EFAULT: {
            Backtrace::collect(err_bt);
            return SigAltStackStatus::EFault;
        }
        case EINVAL: {
            Backtrace::collect(err_bt);
            return SigAltStackStatus::EInval;
        }
        case ENOMEM: {
            Backtrace::collect(err_bt);
            return SigAltStackStatus::ENoMem;
        }
        case EPERM: {
            Backtrace::collect(err_bt);
            return SigAltStackStatus::EPerm;
        }
        default: {
            Backtrace::collect(err_bt);
            return SigAltStackStatus::UnexpectedErr;
        }
        }
    }

    return SigAltStackStatus::Ok;
}

// echo '#include <linux/prctl.h>' | gcc -E -dM -xc - | grep -E "PR_"
constexpr u64 PR_SET_MM_END_DATA = 4;
constexpr u64 PR_SPEC_INDIRECT_BRANCH = 1;
constexpr u64 PR_SYS_DISPATCH_EXCLUSIVE_ON = 1;
constexpr i32 PR_SVE_SET_VL = 50;
constexpr u64 PR_SCHED_CORE_SCOPE_PROCESS_GROUP = 2;
constexpr u64 PR_MTE_TCF_SYNC = (1UL << 1);
constexpr u64 PR_SCHED_CORE_SCOPE_THREAD = 0;
constexpr i32 PR_GET_FP_MODE = 46;
constexpr u64 PR_FP_EXC_OVF = 0x020000;
constexpr i32 PR_PAC_RESET_KEYS = 54;
constexpr u64 PR_FP_MODE_FR = (1 << 0);
constexpr i32 PR_LOCK_SHADOW_STACK_STATUS = 76;
constexpr u64 PR_MTE_TAG_SHIFT = 3;
constexpr u64 PR_TIMING_STATISTICAL = 0;
constexpr u64 PR_PMLEN_SHIFT = 24;
constexpr i32 PR_SET_UNALIGN = 6;
constexpr u64 PR_PPC_DEXCR_CTRL_MASK = 0x1f;
constexpr u64 PR_CFI_DISABLE = (1UL << 1);
constexpr i32 PR_SET_CHILD_SUBREAPER = 36;
constexpr u64 PR_FP_EXC_ASYNC = 2;
constexpr i32 PR_MPX_DISABLE_MANAGEMENT = 44;
constexpr u64 PR_TIMER_CREATE_RESTORE_IDS_GET = 2;
constexpr u64 PR_MCE_KILL_DEFAULT = 2;
constexpr i32 PR_SET_SECCOMP = 22;
constexpr i32 PR_CAPBSET_DROP = 24;
constexpr u64 PR_RISCV_CTX_SW_FENCEI_ON = 0;
constexpr u64 PR_FP_EXC_RES = 0x080000;
constexpr u64 PR_SYS_DISPATCH_INCLUSIVE_ON = 2;
constexpr u64 PR_CFI_BRANCH_LANDING_PADS = 0;
constexpr i32 PR_GET_SHADOW_STACK_STATUS = 74;
constexpr i32 PR_SET_MM = 35;
constexpr i32 PR_RSEQ_SLICE_EXTENSION = 79;
constexpr i32 PR_SET_MEMORY_MERGE = 67;
constexpr i32 PR_SET_DUMPABLE = 4;
constexpr u64 PR_SET_MM_ENV_END = 11;
constexpr u64 PR_RISCV_CTX_SW_FENCEI_OFF = 1;
constexpr i32 PR_GET_THP_DISABLE = 42;
constexpr u64 PR_SME_VL_INHERIT = (1 << 17);
constexpr i32 PR_GET_KEEPCAPS = 7;
constexpr i32 PR_SET_TIMERSLACK = 29;
constexpr u64 PR_MDWE_REFUSE_EXEC_GAIN = (1UL << 0);
constexpr u64 PR_SET_MM_START_STACK = 5;
constexpr u64 PR_CFI_LOCK = (1UL << 2);
constexpr u64 PR_MCE_KILL_SET = 1;
constexpr u64 PR_MTE_TCF_SHIFT = 1;
constexpr u64 PR_SCHED_CORE_SHARE_FROM = 3;
constexpr u64 PR_FP_EXC_PRECISE = 3;
constexpr u64 PR_UNALIGN_SIGBUS = 2;
constexpr u64 PR_PAC_APIAKEY = (1UL << 0);
constexpr u64 PR_SHADOW_STACK_WRITE = (1UL << 1);
constexpr u64 PR_RISCV_V_VSTATE_CTRL_MASK = 0x1f;
constexpr u64 PR_TSC_ENABLE = 1;
constexpr i32 PR_GET_ENDIAN = 19;
constexpr u64 PR_SET_MM_START_CODE = 1;
constexpr u64 PR_PPC_DEXCR_SRAPD = 2;
constexpr u64 PR_CAP_AMBIENT_RAISE = 2;
constexpr u64 PR_RISCV_SCOPE_PER_THREAD = 1;
constexpr u64 PR_SYS_DISPATCH_ON = PR_SYS_DISPATCH_EXCLUSIVE_ON;
constexpr u64 PR_SVE_VL_LEN_MASK = 0xffff;
constexpr u64 PR_UNALIGN_NOPRINT = 1;
constexpr u64 PR_ENDIAN_LITTLE = 1;
constexpr u64 PR_SYS_DISPATCH_OFF = 0;
constexpr u64 PR_TIMER_CREATE_RESTORE_IDS_OFF = 0;
constexpr u64 PR_SET_MM_AUXV = 12;
constexpr u64 PR_ENDIAN_PPC_LITTLE = 2;
constexpr u64 PR_RISCV_V_VSTATE_CTRL_ON = 2;
constexpr u64 PR_PPC_DEXCR_IBRTPD = 1;
constexpr i32 PR_SET_THP_DISABLE = 41;
constexpr u64 PR_FPEMU_NOPRINT = 1;
constexpr u64 PR_RISCV_V_VSTATE_CTRL_INHERIT = (1 << 4);
constexpr i32 PR_GET_NO_NEW_PRIVS = 39;
constexpr i32 PR_PPC_GET_DEXCR = 72;
constexpr i32 PR_SME_SET_VL = 63;
constexpr u64 PR_RISCV_V_VSTATE_CTRL_DEFAULT = 0;
constexpr u64 PR_RSEQ_SLICE_EXTENSION_GET = 1;
constexpr i32 PR_TASK_PERF_EVENTS_DISABLE = 31;
constexpr u64 PR_PAC_APDAKEY = (1UL << 2);
constexpr i32 PR_SET_NAME = 15;
constexpr i32 PR_SET_TAGGED_ADDR_CTRL = 55;
constexpr i32 PR_SET_PTRACER = 0x59616d61;
constexpr u64 PR_TIMING_TIMESTAMP = 1;
constexpr u64 PR_FP_EXC_INV = 0x100000;
constexpr i32 PR_SET_SHADOW_STACK_STATUS = 75;
constexpr i32 PR_PAC_SET_ENABLED_KEYS = 60;
constexpr i32 PR_TIMER_CREATE_RESTORE_IDS = 77;
constexpr u64 PR_SET_MM_MAP = 14;
constexpr u64 PR_SPEC_L1D_FLUSH = 2;
constexpr u64 PR_SCHED_CORE_CREATE = 1;
constexpr u64 PR_PAC_APGAKEY = (1UL << 4);
constexpr i32 PR_SET_FPEMU = 10;
constexpr i32 PR_RISCV_V_GET_CONTROL = 70;
constexpr u64 PR_MTE_STORE_ONLY = (1UL << 19);
constexpr i32 PR_GET_UNALIGN = 5;
constexpr u64 PR_MCE_KILL_LATE = 0;
constexpr u64 PR_SPEC_DISABLE = (1UL << 2);
constexpr u64 PR_SET_MM_BRK = 7;
constexpr u64 PR_SET_MM_START_BRK = 6;
constexpr u64 PR_SET_MM_END_CODE = 2;
constexpr i32 PR_SET_SPECULATION_CTRL = 53;
constexpr i32 PR_SET_FPEXC = 12;
constexpr i32 PR_PPC_SET_DEXCR = 73;
constexpr u64 PR_SPEC_DISABLE_NOEXEC = (1UL << 4);
constexpr u64 PR_SPEC_STORE_BYPASS = 0;
constexpr u64 PR_FP_MODE_FRE = (1 << 1);
constexpr u64 PR_SHADOW_STACK_PUSH = (1UL << 2);
constexpr u64 PR_SVE_VL_INHERIT = (1 << 17);
constexpr i32 PR_GET_FPEMU = 9;
constexpr i32 PR_GET_IO_FLUSHER = 58;
constexpr u64 PR_CAP_AMBIENT_LOWER = 3;
constexpr u64 PR_PPC_DEXCR_CTRL_SET = 0x2;
constexpr u64 PR_SET_MM_ARG_START = 8;
constexpr i32 PR_SET_TSC = 26;
constexpr u64 PR_RSEQ_SLICE_EXTENSION_SET = 2;
constexpr i32 PR_SET_IO_FLUSHER = 57;
constexpr i32 PR_SET_PDEATHSIG = 1;
constexpr u64 PR_RISCV_V_VSTATE_CTRL_OFF = 1;
constexpr i32 PR_GET_FPEXC = 11;
constexpr u64 PR_TAGGED_ADDR_ENABLE = (1UL << 0);
constexpr u64 PR_MCE_KILL_EARLY = 1;
constexpr u64 PR_SCHED_CORE_MAX = 4;
constexpr u64 PR_SET_MM_ENV_START = 10;
constexpr i32 PR_GET_TIMING = 13;
constexpr i32 PR_SET_SYSCALL_USER_DISPATCH = 59;
constexpr i32 PR_MCE_KILL = 33;
constexpr i32 PR_RISCV_SET_ICACHE_FLUSH_CTX = 71;
constexpr u64 PR_SPEC_NOT_AFFECTED = 0;
constexpr u64 PR_SME_VL_LEN_MASK = 0xffff;
constexpr u64 PR_PPC_DEXCR_CTRL_EDITABLE = 0x1;
constexpr u64 PR_ENDIAN_BIG = 0;
constexpr u64 PR_PAC_APIBKEY = (1UL << 1);
constexpr i32 PR_GET_TIMERSLACK = 30;
constexpr u64 PR_MTE_TAG_MASK = (0xffffUL << PR_MTE_TAG_SHIFT);
constexpr i32 PR_GET_MEMORY_MERGE = 68;
constexpr u64 PR_MTE_TCF_ASYNC = (1UL << 2);
constexpr u64 PR_MTE_TCF_MASK = (PR_MTE_TCF_SYNC | PR_MTE_TCF_ASYNC);
constexpr i32 PR_GET_TSC = 25;
constexpr u64 PR_PMLEN_MASK = (0x7fUL << PR_PMLEN_SHIFT);
constexpr i32 PR_GET_TAGGED_ADDR_CTRL = 56;
constexpr u64 PR_SET_PTRACER_ANY = (u64)-1;
constexpr i32 PR_SET_MDWE = 65;
constexpr u64 PR_TIMER_CREATE_RESTORE_IDS_ON = 1;
constexpr u64 PR_SET_MM_ARG_END = 9;
constexpr u64 PR_FP_EXC_UND = 0x040000;
constexpr u64 PR_FPEMU_SIGFPE = 2;
constexpr i32 PR_FUTEX_HASH = 78;
constexpr u64 PR_RISCV_V_VSTATE_CTRL_CUR_MASK = 0x3;
constexpr i32 PR_SET_TIMING = 14;
constexpr i32 PR_MPX_ENABLE_MANAGEMENT = 43;
constexpr i32 PR_RISCV_V_SET_CONTROL = 69;
constexpr i32 PR_SET_KEEPCAPS = 8;
constexpr i32 PR_SCHED_CORE = 62;
constexpr u64 PR_TSC_SIGSEGV = 2;
constexpr i32 PR_CAPBSET_READ = 23;
constexpr i32 PR_GET_SPECULATION_CTRL = 52;
constexpr i32 PR_SET_FP_MODE = 45;
constexpr u64 PR_MDWE_NO_INHERIT = (1UL << 1);
constexpr i32 PR_SET_SECUREBITS = 28;
constexpr u64 PR_PPC_DEXCR_NPHIE = 3;
constexpr i32 PR_TASK_PERF_EVENTS_ENABLE = 32;
constexpr i32 PR_SME_GET_VL = 64;
constexpr i32 PR_GET_CHILD_SUBREAPER = 37;
constexpr u64 PR_SCHED_CORE_SCOPE_THREAD_GROUP = 1;
constexpr i32 PR_GET_DUMPABLE = 3;
constexpr u64 PR_RISCV_V_VSTATE_CTRL_NEXT_MASK = 0xc;
constexpr u64 PR_RISCV_SCOPE_PER_PROCESS = 0;
constexpr u64 PR_SPEC_ENABLE = (1UL << 1);
constexpr i32 PR_SET_CFI = 81;
constexpr u64 PR_MTE_TCF_NONE = 0UL;
constexpr u64 PR_PAC_APDBKEY = (1UL << 3);
constexpr u64 PR_SCHED_CORE_SHARE_TO = 2;
constexpr u64 PR_SPEC_FORCE_DISABLE = (1UL << 3);
constexpr u64 PR_RSEQ_SLICE_EXT_ENABLE = 0x01;
constexpr i32 PR_SET_ENDIAN = 20;
constexpr u64 PR_PPC_DEXCR_CTRL_CLEAR = 0x4;
constexpr u64 PR_SME_SET_VL_ONEXEC = (1 << 18);
constexpr u64 PR_SPEC_PRCTL = (1UL << 0);
constexpr u64 PR_CFI_ENABLE = (1UL << 0);
constexpr u64 PR_PPC_DEXCR_CTRL_SET_ONEXEC = 0x8;
constexpr u64 PR_SET_VMA_ANON_NAME = 0;
constexpr u64 PR_PPC_DEXCR_CTRL_CLEAR_ONEXEC = 0x10;
constexpr u64 PR_SET_MM_START_DATA = 3;
constexpr u64 PR_PPC_DEXCR_SBHE = 0;
constexpr i32 PR_GET_AUXV = 0x41555856;
constexpr i32 PR_SET_NO_NEW_PRIVS = 38;
constexpr i32 PR_GET_TID_ADDRESS = 40;
constexpr i32 PR_GET_SECCOMP = 21;
constexpr i32 PR_SVE_GET_VL = 51;
constexpr u64 PR_FUTEX_HASH_SET_SLOTS = 1;
constexpr u64 PR_FP_EXC_SW_ENABLE = 0x80;
constexpr i32 PR_GET_CFI = 80;
constexpr u64 PR_SET_MM_EXE_FILE = 13;
constexpr u64 PR_SCHED_CORE_GET = 0;
constexpr i32 PR_PAC_GET_ENABLED_KEYS = 61;
constexpr i32 PR_MCE_KILL_GET = 34;
constexpr u64 PR_SET_MM_MAP_SIZE = 15;
constexpr u64 PR_SVE_SET_VL_ONEXEC = (1 << 18);
constexpr u64 PR_FUTEX_HASH_GET_SLOTS = 2;
constexpr u64 PR_FP_EXC_DISABLED = 0;
constexpr i32 PR_GET_NAME = 16;
constexpr u64 PR_SHADOW_STACK_ENABLE = (1UL << 0);
constexpr u64 PR_CAP_AMBIENT_IS_SET = 1;
constexpr i32 PR_GET_MDWE = 66;
constexpr u64 PR_THP_DISABLE_EXCEPT_ADVISED = (1 << 1);
constexpr i32 PR_GET_SECUREBITS = 27;
constexpr u64 PR_CAP_AMBIENT_CLEAR_ALL = 4;
constexpr u64 PR_FP_EXC_DIV = 0x010000;
constexpr u64 PR_FP_EXC_NONRECOV = 1;
constexpr u64 PR_MCE_KILL_CLEAR = 0;
constexpr i32 PR_CAP_AMBIENT = 47;
constexpr i32 PR_SET_VMA = 0x53564d41;
constexpr i32 PR_GET_PDEATHSIG = 2;

enum class [[nodiscard]] PrctlStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnexpectedErr,
    EInval,
};

inline PrctlStatus prctl(Backtrace *err_bt, i32 option, u64 arg0, u64 arg1,
                         u64 arg2, u64 arg3, u64 *ret) {
    if (ret == nullptr) {
        Backtrace::collect(err_bt);
        return PrctlStatus::Nullptr;
    }

    u64 const res = syscall5(NR_PRCTL, (u64)option, arg0, arg1, arg2, arg3);
    u64 const errno = errno_(res);
    if (errno != SUCCESS) [[unlikely]] {
        switch (errno) {
        case EINVAL: {
            Backtrace::collect(err_bt);
            return PrctlStatus::EInval;
        }
        default: {
            Backtrace::collect(err_bt);
            return PrctlStatus::UnexpectedErr;
        }
        }
    }

    *ret = res;
    return PrctlStatus::Ok;
}

} // namespace sys

} // namespace zx
