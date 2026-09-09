// These definitions are taken from
// /usr/include/linux/stat.h
// Types are changed to zx_* types.
//
// These defitions are only definitions used for interfacing with the kernel
// through syscalls. So this is not derived work.
//
// Original license notice:
/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */

export module zx:statx;
import :def;

export namespace zx {

namespace sys {

constexpr i32 S_IFMT = 00170000;
constexpr i32 S_IFSOCK = 0140000;
constexpr i32 S_IFLNK = 0120000;
constexpr i32 S_IFREG = 0100000;
constexpr i32 S_IFBLK = 0060000;
constexpr i32 S_IFDIR = 0040000;
constexpr i32 S_IFCHR = 0020000;
constexpr i32 S_IFIFO = 0010000;
constexpr i32 S_ISUID = 0004000;
constexpr i32 S_ISGID = 0002000;
constexpr i32 S_ISVTX = 0001000;

constexpr i32 S_IRWXU = 00700;
constexpr i32 S_IRUSR = 00400;
constexpr i32 S_IWUSR = 00200;
constexpr i32 S_IXUSR = 00100;

constexpr i32 S_IRWXG = 00070;
constexpr i32 S_IRGRP = 00040;
constexpr i32 S_IWGRP = 00020;
constexpr i32 S_IXGRP = 00010;

constexpr i32 S_IRWXO = 00007;
constexpr i32 S_IROTH = 00004;
constexpr i32 S_IWOTH = 00002;
constexpr i32 S_IXOTH = 00001;

extern "C" {

/*
 * Timestamp structure for the timestamps in struct statx.
 *
 * tv_sec holds the number of seconds before (negative) or after (positive)
 * 00:00:00 1st January 1970 UTC.
 *
 * tv_nsec holds a number of nanoseconds (0..999,999,999) after the tv_sec time.
 *
 * __reserved is held in case we need a yet finer resolution.
 */
struct StatXTimeStamp {
    i64 tv_sec;
    u32 tv_nsec;
    i32 __reserved;
};

/*
 * Structures for the extended file attribute retrieval system call
 * (statx()).
 *
 * The caller passes a mask of what they're specifically interested in as a
 * parameter to statx().  What statx() actually got will be indicated in
 * st_mask upon return.
 *
 * For each bit in the mask argument:
 *
 * - if the datum is not supported:
 *
 *   - the bit will be cleared, and
 *
 *   - the datum will be set to an appropriate fabricated value if one is
 *     available (eg. CIFS can take a default uid and gid), otherwise
 *
 *   - the field will be cleared;
 *
 * - otherwise, if explicitly requested:
 *
 *   - the datum will be synchronised to the server if AT_STATX_FORCE_SYNC is
 *     set or if the datum is considered out of date, and
 *
 *   - the field will be filled in and the bit will be set;
 *
 * - otherwise, if not requested, but available in approximate form without any
 *   effort, it will be filled in anyway, and the bit will be set upon return
 *   (it might not be up to date, however, and no attempt will be made to
 *   synchronise the internal state first);
 *
 * - otherwise the field and the bit will be cleared before returning.
 *
 * Items in STATX_BASIC_STATS may be marked unavailable on return, but they
 * will have values installed for compatibility purposes so that stat() and
 * co. can be emulated in userspace.
 */
struct StatX {
    /* 0x00 */
    /* What results were written [uncond] */
    u32 stx_mask;

    /* Preferred general I/O size [uncond] */
    u32 stx_blksize;

    /* Flags conveying information about the file [uncond] */
    u64 stx_attributes;

    /* 0x10 */
    /* Number of hard links */
    u32 stx_nlink;

    /* User ID of owner */
    u32 stx_uid;

    /* Group ID of owner */
    u32 stx_gid;

    /* File mode */
    u16 stx_mode;
    u16 __spare0[1];

    /* 0x20 */
    /* Inode number */
    u64 stx_ino;

    /* File size */
    u64 stx_size;

    /* Number of 512-byte blocks allocated */
    u64 stx_blocks;

    /* Mask to show what's supported in stx_attributes */
    u64 stx_attributes_mask;

    /* 0x40 */
    /* Last access time */
    StatXTimeStamp stx_atime;

    /* File creation time */
    StatXTimeStamp stx_btime;

    /* Last attribute change time */
    StatXTimeStamp stx_ctime;

    /* Last data modification time */
    StatXTimeStamp stx_mtime;

    /* 0x80 */
    /* Device ID of special file [if bdev/cdev] */
    u32 stx_rdev_major;
    u32 stx_rdev_minor;

    /* ID of device containing file [uncond] */
    u32 stx_dev_major;
    u32 stx_dev_minor;

    /* 0x90 */
    u64 stx_mnt_id;

    /* Memory buffer alignment for direct I/O */
    u32 stx_dio_mem_align;

    /* File offset alignment for direct I/O */
    u32 stx_dio_offset_align;

    /* 0xa0 */
    /* Subvolume identifier */
    u64 stx_subvol;

    /* Min atomic write unit in bytes */
    u32 stx_atomic_write_unit_min;

    /* Max atomic write unit in bytes */
    u32 stx_atomic_write_unit_max;

    /* 0xb0 */
    /* Max atomic write segment count */
    u32 stx_atomic_write_segments_max;

    /* File offset alignment for direct I/O reads */
    u32 stx_dio_read_offset_align;

    /* Optimised max atomic write unit in bytes */
    u32 stx_atomic_write_unit_max_opt;
    u32 __spare2[1];

    /* 0xc0 */
    u64 __spare3[8]; /* Spare space for future expansion */

    /* 0x100 */
};

/*
 * Flags to be stx_mask
 *
 * Query request/result mask for statx() and struct statx::stx_mask.
 *
 * These bits should be set in the mask argument of statx() to request
 * particular items when calling statx().
 */
constexpr u32 STATX_TYPE = 0x00000001U;  /* Want/got stx_mode & S_IFMT */
constexpr u32 STATX_MODE = 0x00000002U;  /* Want/got stx_mode & ~S_IFMT */
constexpr u32 STATX_NLINK = 0x00000004U; /* Want/got stx_nlink */
constexpr u32 STATX_UID = 0x00000008U;   /* Want/got stx_uid */
constexpr u32 STATX_GID = 0x00000010U;   /* Want/got stx_gid */
constexpr u32 STATX_ATIME = 0x00000020U; /* Want/got stx_atime */
constexpr u32 STATX_MTIME = 0x00000040U; /* Want/got stx_mtime */
constexpr u32 STATX_CTIME = 0x00000080U; /* Want/got stx_ctime */
constexpr u32 STATX_INO = 0x00000100U;   /* Want/got stx_ino */
constexpr u32 STATX_SIZE = 0x00000200U;  /* Want/got stx_size */
constexpr u32 STATX_BLOCKS = 0x00000400; /* Want/got stx_blocks */
constexpr u32 STATX_BASIC_STATS =
    0x000007ffU; /* The stuff in the normal stat struct */
constexpr u32 STATX_BTIME = 0x00000800U;  /* Want/got stx_btime */
constexpr u32 STATX_MNT_ID = 0x00001000U; /* Got stx_mnt_id */
constexpr u32 STATX_DIOALIGN =
    0x00002000U; /* Want/got direct I/O alignment info */
constexpr u32 STATX_MNT_ID_UNIQUE =
    0x00004000U;                          /* Want/got extended stx_mount_id */
constexpr u32 STATX_SUBVOL = 0x00008000U; /* Want/got stx_subvol */
constexpr u32 STATX_WRITE_ATOMIC =
    0x00010000U; /* Want/got atomic_write_* fields */
constexpr u32 STATX_DIO_READ_ALIGN =
    0x00020000U; /* Want/got dio read alignment info */

constexpr u32 STATX__RESERVED =
    0x80000000U; /* Reserved for future struct statx expansion */

/*
 * This is deprecated, and shall remain the same value in the future.  To avoid
 * confusion please use the equivalent (STATX_BASIC_STATS | STATX_BTIME)
 * instead.
 */
constexpr u32 STATX_ALL = 0x00000fffU;

/*
 * Attributes to be found in stx_attributes and masked in stx_attributes_mask.
 *
 * These give information about the features or the state of a file that might
 * be of use to ordinary userspace programs such as GUIs or ls rather than
 * specialised tools.
 *
 * Note that the flags marked [I] correspond to the FS_IOC_SETFLAGS flags
 * semantically.  Where possible, the numerical value is picked to correspond
 * also.  Note that the DAX attribute indicates that the file is in the CPU
 * direct access state.  It does not correspond to the per-inode flag that
 * some filesystems support.
 *
 */
constexpr u32 STATX_ATTR_COMPRESSED =
    0x00000004; /* [I] File is compressed by the fs */
constexpr u32 STATX_ATTR_IMMUTABLE =
    0x00000010;                               /* [I] File is marked immutable */
constexpr u32 STATX_ATTR_APPEND = 0x00000020; /* [I] File is append-only */
constexpr u32 STATX_ATTR_NODUMP = 0x00000040; /* [I] File is not to be dumped */
constexpr u32 STATX_ATTR_ENCRYPTED =
    0x00000800; /* [I] File requires key to decrypt in fs */
constexpr u32 STATX_ATTR_AUTOMOUNT = 0x00001000;  /* Dir: Automount trigger */
constexpr u32 STATX_ATTR_MOUNT_ROOT = 0x00002000; /* Root of a mount */
constexpr u32 STATX_ATTR_VERITY = 0x00100000; /* [I] Verity protected file */
constexpr u32 STATX_ATTR_DAX = 0x00200000; /* File is currently in DAX state */
constexpr u32 STATX_ATTR_WRITE_ATOMIC =
    0x00400000; /* File supports atomic write operations */

} // extern "C"

} // namespace sys

} // namespace zx
