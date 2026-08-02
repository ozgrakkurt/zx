export module zx:sys;
import :syscall;
import :def;

#ifndef __x86_64__
#error "only x86_64 is supported"
#endif

#ifndef __linux__
#error "only linux is supported"
#endif

export namespace zx {

namespace sys {

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

u64 const SUCCESS = 0;

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

ReadStatus read(i32 fd, u8* buf, u64 count, u64* n_read) {
  if (n_read == nullptr) {
    return ReadStatus::Nullptr;
  }

  u64 const res = syscall3(0, (u64)fd, (u64)buf, count);
  u64 const errno = errno_(res);
  if (errno != SUCCESS) {
     *n_read = U64_MAX;
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

} // namespace sys

} // namespace zx
