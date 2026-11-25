// This code is copied over from 'liburing' project and slightly modified.
// Types are changed from zx_u8 style to zx_u8 style.
// https://github.com/axboe/liburing/blob/master/src/include/liburing/io_uring.h
// Commit 3891a21
//
// Changed all primitive types to zx_ variants.
// Removed code that requires types from linux headers.
//
// Original license for the code:
// Copyright 2020 Jens Axboe
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

/* SPDX-License-Identifier: (GPL-2.0 WITH Linux-syscall-note) OR MIT */
/*
 * Header file for the io_uring interface.
 *
 * Copyright (C) 2019 Jens Axboe
 * Copyright (C) 2019 Christoph Hellwig
 */

export module zx:io_uring;
import :def;

export namespace zx {

namespace io_uring {

extern "C" {

/*
 * IO submission data structure (Submission Queue Entry)
 */
struct IoUringSQE {
    u8 opcode;  /* type of operation for this sqe */
    u8 flags;   /* IOSQE_ flags */
    u16 ioprio; /* ioprio for the request */
    i32 fd;     /* file descriptor to do IO on */
    union {
        u64 off; /* offset into file */
        u64 addr2;
        struct {
            u32 cmd_op;
            u32 __pad1;
        };
    };
    union {
        u64 addr; /* pointer to buffer or iovecs */
        u64 splice_off_in;
        struct {
            u32 level;
            u32 optname;
        };
    };
    u32 len; /* buffer size or number of iovecs */
    union {
        i32 rw_flags;
        u32 fsync_flags;
        u16 poll_events;   /* compatibility */
        u32 poll32_events; /* word-reversed for BE */
        u32 sync_range_flags;
        u32 msg_flags;
        u32 timeout_flags;
        u32 accept_flags;
        u32 cancel_flags;
        u32 open_flags;
        u32 statx_flags;
        u32 fadvise_advice;
        u32 splice_flags;
        u32 rename_flags;
        u32 unlink_flags;
        u32 hardlink_flags;
        u32 xattr_flags;
        u32 msg_ring_flags;
        u32 uring_cmd_flags;
        u32 waitid_flags;
        u32 futex_flags;
        u32 install_fd_flags;
        u32 nop_flags;
        u32 pipe_flags;
    };
    u64 user_data; /* data to be passed back at completion time */
    /* pack this to avoid bogus arm OABI complaints */
    union {
        /* index into fixed buffers, if used */
        u16 buf_index;
        /* for grouped buffer selection */
        u16 buf_group;
    } __attribute__((packed));
    /* personality to use, if used */
    u16 personality;
    union {
        i32 splice_fd_in;
        u32 file_index;
        u32 zcrx_ifq_idx;
        u32 optlen;
        struct {
            u16 addr_len;
            u16 __pad3[1];
        };
    };
    union {
        struct {
            u64 addr3;
            u64 __pad2[1];
        };
        struct {
            u64 attr_ptr;       /* pointer to attribute information */
            u64 attr_type_mask; /* bit mask of attributes */
        };
        u64 optval;
        /*
         * If the ring is initialized with IORING_SETUP_SQE128, then
         * this field is used for 80 bytes of arbitrary command data
         */
        u8 cmd[0];
    };
};

/* sqe->attr_type_mask flags */
constexpr u64 IORING_RW_ATTR_FLAG_PI = (1U << 0);
/* PI attribute information */
struct IoUringAttrPI {
    u16 flags;
    u16 app_tag;
    u32 len;
    u64 addr;
    u64 seed;
    u64 rsvd;
};

/*
 * If sqe->file_index is set to this for opcodes that instantiate a new
 * direct descriptor (like openat/openat2/accept), then io_uring will allocate
 * an available direct descriptor instead of having the application pass one
 * in. The picked direct descriptor will be returned in cqe->res, or -ENFILE
 * if the space is full.
 */
constexpr u32 IORING_FILE_INDEX_ALLOC = (~0U);

enum class IoUringSQEFlagsBit : u32_c {
    FixedFileBit,
    IoDrainBit,
    IoLinkBit,
    IoHardlinkBit,
    AsyncBit,
    BufferSelectBit,
    CQESkipSuccessBit,
};

/*
 * sqe->flags
 */
/* use fixed fileset */
constexpr u8 IOSQE_FIXED_FILE = (1U << (u32)IoUringSQEFlagsBit::FixedFileBit);
/* issue after inflight IO */
constexpr u8 IOSQE_IO_DRAIN = (1U << (u32)IoUringSQEFlagsBit::IoDrainBit);
/* links next sqe */
constexpr u8 IOSQE_IO_LINK = (1U << (u32)IoUringSQEFlagsBit::IoLinkBit);
/* like LINK, but stronger */
constexpr u8 IOSQE_IO_HARDLINK = (1U << (u32)IoUringSQEFlagsBit::IoHardlinkBit);
/* always go async */
constexpr u8 IOSQE_ASYNC = (1U << (u32)IoUringSQEFlagsBit::AsyncBit);
/* select buffer from sqe->buf_group */
constexpr u8 IOSQE_BUFFER_SELECT =
    (1U << (u32)IoUringSQEFlagsBit::BufferSelectBit);
/* don't post CQE if request succeeded */
constexpr u8 IOSQE_CQE_SKIP_SUCCESS =
    (1U << (u32)IoUringSQEFlagsBit::CQESkipSuccessBit);

/*
 * io_uring_setup() flags
 */
constexpr u32 IORING_SETUP_IOPOLL = (1U << 0);    /* io_context is polled */
constexpr u32 IORING_SETUP_SQPOLL = (1U << 1);    /* SQ poll thread */
constexpr u32 IORING_SETUP_SQ_AFF = (1U << 2);    /* sq_thread_cpu is valid */
constexpr u32 IORING_SETUP_CQSIZE = (1U << 3);    /* app defines CQ size */
constexpr u32 IORING_SETUP_CLAMP = (1U << 4);     /* clamp SQ/CQ ring sizes */
constexpr u32 IORING_SETUP_ATTACH_WQ = (1U << 5); /* attach to existing wq */
constexpr u32 IORING_SETUP_R_DISABLED =
    (1U << 6); /* start with ring disabled */
constexpr u32 IORING_SETUP_SUBMIT_ALL =
    (1U << 7); /* continue submit on error */
/*
 * Cooperative task running. When requests complete, they often require
 * forcing the submitter to transition to the kernel to complete. If this
 * flag is set, work will be done when the task transitions anyway, rather
 * than force an inter-processor interrupt reschedule. This avoids interrupting
 * a task running in userspace, and saves an IPI.
 */
constexpr u32 IORING_SETUP_COOP_TASKRUN = (1U << 8);
/*
 * If COOP_TASKRUN is set, get notified if task work is available for
 * running and a kernel transition would be needed to run it. This sets
 * IORING_SQ_TASKRUN in the sq ring flags. Not valid without COOP_TASKRUN
 * or DEFER_TASKRUN.
 */
constexpr u32 IORING_SETUP_TASKRUN_FLAG = (1U << 9);
constexpr u32 IORING_SETUP_SQE128 = (1U << 10); /* SQEs are 128 byte */
constexpr u32 IORING_SETUP_CQE32 = (1U << 11);  /* CQEs are 32 byte */
/*
 * Only one task is allowed to submit requests
 */
constexpr u32 IORING_SETUP_SINGLE_ISSUER = (1U << 12);

/*
 * Defer running task work to get events.
 * Rather than running bits of task work whenever the task transitions
 * try to do it just before it is needed.
 */
constexpr u32 IORING_SETUP_DEFER_TASKRUN = (1U << 13);

/*
 * Application provides the memory for the rings
 */
constexpr u32 IORING_SETUP_NO_MMAP = (1U << 14);

/*
 * Register the ring fd in itself for use with
 * IORING_REGISTER_USE_REGISTERED_RING; return a registered fd index rather
 * than an fd.
 */
constexpr u32 IORING_SETUP_REGISTERED_FD_ONLY = (1U << 15);

/*
 * Removes indirection through the SQ index array.
 */
constexpr u32 IORING_SETUP_NO_SQARRAY = (1U << 16);

/* Use hybrid poll in iopoll process */
constexpr u32 IORING_SETUP_HYBRID_IOPOLL = (1U << 17);

/*
 * Allow both 16b and 32b CQEs. If a 32b CQE is posted, it will have
 * IORING_CQE_F_32 set in cqe->flags.
 */
constexpr u32 IORING_SETUP_CQE_MIXED = (1U << 18);

/*
 *  Allow both 64b and 128b SQEs. If a 128b SQE is posted, it will use a 128b
 *  opcode.
 */
constexpr u32 IORING_SETUP_SQE_MIXED = (1U << 19);

/*
 * When set, io_uring ignores SQ head and tail and fetches SQEs to submit
 * starting from index 0 instead from the index stored in the head pointer.
 * IOW, the user should place all SQE at the beginning of the SQ memory
 * before issuing a submission syscall.
 *
 * It requires IORING_SETUP_NO_SQARRAY and is incompatible with
 * IORING_SETUP_SQPOLL. The user must also never change the SQ head and tail
 * values and keep it set to 0. Any other value is undefined behaviour.
 */
constexpr u32 IORING_SETUP_SQ_REWIND = (1U << 20);

enum class IoUringOp : u8_c {
    Nop,
    ReadV,
    WriteV,
    FSync,
    ReadFixed,
    WriteFixed,
    PollAdd,
    PollRemove,
    SyncFileRange,
    SendMsg,
    RecvMsg,
    TimeOut,
    TimeOutRemove,
    Accept,
    AsyncCancel,
    LinkTimeOut,
    Connect,
    Fallocate,
    OpenAt,
    Close,
    FilesUpdate,
    StatX,
    Read,
    Write,
    FAdvise,
    MAdvise,
    Send,
    Recv,
    OpenAt2,
    EpollCtl,
    Splice,
    ProvideBuffers,
    RemoveBuffers,
    Tee,
    Shutdown,
    RenameAt,
    UnlinkAt,
    MkdirAt,
    SymLinkAt,
    LinkAt,
    MsgRing,
    FSetXAttr,
    SetXAttr,
    FGetXAttr,
    GetXAttr,
    Socket,
    UringCmd,
    SendZC,
    SendMsgZC,
    ReadMultishot,
    WaitID,
    FutexWait,
    FutexWake,
    FutexWaitV,
    FixedFdInstall,
    FTruncate,
    Bind,
    Listen,
    RecvZC,
    EpollWait,
    ReadVFixed,
    WriteVFixed,
    Pipe,
    Nop128,
    UringCMD128,

    /* this goes last, obviously */
    Last,
};

/*
 * sqe->uring_cmd_flags		top 8bits aren't available for userspace
 * IORING_URING_CMD_FIXED	use registered buffer; pass this flag
 *				along with setting sqe->buf_index.
 */
constexpr u32 IORING_URING_CMD_FIXED = (1U << 0);
constexpr u32 IORING_URING_CMD_MASK = IORING_URING_CMD_FIXED;

/*
 * sqe->fsync_flags
 */
constexpr u32 IORING_FSYNC_DATASYNC = (1U << 0);

/*
 * sqe->timeout_flags
 *
 * IORING_TIMEOUT_IMMEDIATE_ARG:	If set, sqe->addr stores the timeout
 *					value in nanoseconds instead of
 *					pointing to a timespec.
 */
constexpr u32 IORING_TIMEOUT_ABS = (1U << 0);
constexpr u32 IORING_TIMEOUT_UPDATE = (1U << 1);
constexpr u32 IORING_TIMEOUT_BOOTTIME = (1U << 2);
constexpr u32 IORING_TIMEOUT_REALTIME = (1U << 3);
constexpr u32 IORING_LINK_TIMEOUT_UPDATE = (1U << 4);
constexpr u32 IORING_TIMEOUT_ETIME_SUCCESS = (1U << 5);
constexpr u32 IORING_TIMEOUT_MULTISHOT = (1U << 6);
constexpr u32 IORING_TIMEOUT_IMMEDIATE_ARG = (1U << 7);
constexpr u32 IORING_TIMEOUT_CLOCK_MASK =
    (IORING_TIMEOUT_BOOTTIME | IORING_TIMEOUT_REALTIME);
constexpr u32 IORING_TIMEOUT_UPDATE_MASK =
    (IORING_TIMEOUT_UPDATE | IORING_LINK_TIMEOUT_UPDATE);
/*
 * sqe->splice_flags
 * extends splice(2) flags
 */
constexpr u32 SPLICE_F_FD_IN_FIXED = (1U << 31); /* the last bit of u32 */

/*
 * POLL_ADD flags. Note that since sqe->poll_events is the flag space, the
 * command flags for POLL_ADD are stored in sqe->len.
 *
 * IORING_POLL_ADD_MULTI	Multishot poll. Sets IORING_CQE_F_MORE if
 *				the poll handler will continue to report
 *				CQEs on behalf of the same SQE.
 *
 * IORING_POLL_UPDATE		Update existing poll request, matching
 *				sqe->addr as the old user_data field.
 *
 * IORING_POLL_LEVEL		Level triggered poll.
 */
constexpr u32 IORING_POLL_ADD_MULTI = (1U << 0);
constexpr u32 IORING_POLL_UPDATE_EVENTS = (1U << 1);
constexpr u32 IORING_POLL_UPDATE_USER_DATA = (1U << 2);
constexpr u32 IORING_POLL_ADD_LEVEL = (1U << 3);

/*
 * ASYNC_CANCEL flags.
 *
 * IORING_ASYNC_CANCEL_ALL	Cancel all requests that match the given key
 * IORING_ASYNC_CANCEL_FD	Key off 'fd' for cancelation rather than the
 *				request 'user_data'
 * IORING_ASYNC_CANCEL_ANY	Match any request
 * IORING_ASYNC_CANCEL_FD_FIXED	'fd' passed in is a fixed descriptor
 * IORING_ASYNC_CANCEL_USERDATA	Match on user_data, default for no other key
 * IORING_ASYNC_CANCEL_OP	Match request based on opcode
 */
constexpr u32 IORING_ASYNC_CANCEL_ALL = (1U << 0);
constexpr u32 IORING_ASYNC_CANCEL_FD = (1U << 1);
constexpr u32 IORING_ASYNC_CANCEL_ANY = (1U << 2);
constexpr u32 IORING_ASYNC_CANCEL_FD_FIXED = (1U << 3);
constexpr u32 IORING_ASYNC_CANCEL_USERDATA = (1U << 4);
constexpr u32 IORING_ASYNC_CANCEL_OP = (1U << 5);

/*
 * send/sendmsg and recv/recvmsg flags (sqe->ioprio)
 *
 * IORING_RECVSEND_POLL_FIRST	If set, instead of first attempting to send
 *				or receive and arm poll if that yields an
 *				-EAGAIN result, arm poll upfront and skip
 *				the initial transfer attempt.
 *
 * IORING_RECV_MULTISHOT	Multishot recv. Sets IORING_CQE_F_MORE if
 *				the handler will continue to report
 *				CQEs on behalf of the same SQE.
 *
 * IORING_RECVSEND_FIXED_BUF	Use registered buffers, the index is stored in
 *				the buf_index field.
 *
 * IORING_SEND_ZC_REPORT_USAGE
 *				If set, SEND[MSG]_ZC should report
 *				the zerocopy usage in cqe.res
 *				for the IORING_CQE_F_NOTIF cqe.
 *				0 is reported if zerocopy was actually possible.
 *				IORING_NOTIF_USAGE_ZC_COPIED if data was copied
 *				(at least partially).
 *
 * IORING_RECVSEND_BUNDLE	Used with IOSQE_BUFFER_SELECT. If set, send or
 *				recv will grab as many buffers from the buffer
 *				group ID given and send them all. The completion
 *				result 	will be the number of buffers send, with
 *				the starting buffer ID in cqe->flags as per
 *				usual for provided buffer usage. The buffers
 *				will be	contiguous from the starting buffer ID.
 *
 * IORING_SEND_VECTORIZED	If set, SEND[_ZC] will take a pointer to a
 * io_vec to allow vectorized send operations.
 */
constexpr u16 IORING_RECVSEND_POLL_FIRST = (1U << 0);
constexpr u16 IORING_RECV_MULTISHOT = (1U << 1);
constexpr u16 IORING_RECVSEND_FIXED_BUF = (1U << 2);
constexpr u16 IORING_SEND_ZC_REPORT_USAGE = (1U << 3);
constexpr u16 IORING_RECVSEND_BUNDLE = (1U << 4);
constexpr u16 IORING_SEND_VECTORIZED = (1U << 5);

/*
 * cqe.res for IORING_CQE_F_NOTIF if
 * IORING_SEND_ZC_REPORT_USAGE was requested
 *
 * It should be treated as a flag, all other
 * bits of cqe.res should be treated as reserved!
 */
constexpr u32 IORING_NOTIF_USAGE_ZC_COPIED = (1U << 31);

/*
 * accept flags stored in sqe->ioprio
 */
constexpr u16 IORING_ACCEPT_MULTISHOT = (1U << 0);
constexpr u16 IORING_ACCEPT_DONTWAIT = (1U << 1);
constexpr u16 IORING_ACCEPT_POLL_FIRST = (1U << 2);

/*
 * IORING_OP_MSG_RING command types, stored in sqe->addr
 */
enum class IoUringMsgRingFlags : u32_c {
    MsgData,   /* pass sqe->len as 'res' and off as user_data */
    MsgSendFd, /* send a registered fd to another ring */
};

/*
 * IORING_OP_MSG_RING flags (sqe->msg_ring_flags)
 *
 * IORING_MSG_RING_CQE_SKIP	Don't post a CQE to the target ring. Not
 *				applicable for IORING_MSG_DATA, obviously.
 */
constexpr u32 IORING_MSG_RING_CQE_SKIP = (1U << 0);
/* Pass through the flags from sqe->file_index to cqe->flags */
constexpr u32 IORING_MSG_RING_FLAGS_PASS = (1U << 1);

/*
 * IORING_OP_FIXED_FD_INSTALL flags (sqe->install_fd_flags)
 *
 * IORING_FIXED_FD_NO_CLOEXEC	Don't mark the fd as O_CLOEXEC
 */
constexpr u32 IORING_FIXED_FD_NO_CLOEXEC = (1U << 0);

/*
 * IORING_OP_NOP flags (sqe->nop_flags)
 *
 * IORING_NOP_INJECT_RESULT	Inject result from sqe->result
 */
constexpr u32 IORING_NOP_INJECT_RESULT = (1U << 0);
constexpr u32 IORING_NOP_CQE32 = (1U << 5);

/*
 * IO completion data structure (Completion Queue Entry)
 */
struct IoUringCQE {
    u64 user_data; /* sqe->user_data value passed back */
    i32 res;       /* result code for this event */
    u32 flags;

    /*
     * If the ring is initialized with IORING_SETUP_CQE32, then this field
     * contains 16-bytes of padding, doubling the size of the CQE.
     */
    u64 big_cqe[];
};

/*
 * cqe->flags
 *
 * IORING_CQE_F_BUFFER	If set, the upper 16 bits are the buffer ID
 * IORING_CQE_F_MORE	If set, parent SQE will generate more CQE entries
 * IORING_CQE_F_SOCK_NONEMPTY	If set, more data to read after socket recv
 * IORING_CQE_F_NOTIF	Set for notification CQEs. Can be used to distinct
 * 			them from sends.
 * IORING_CQE_F_BUF_MORE If set, the buffer ID set in the completion will get
 *			more completions. In other words, the buffer is being
 *			partially consumed, and will be used by the kernel for
 *			more completions. This is only set for buffers used via
 *			the incremental buffer consumption, as provided by
 *			a ring buffer setup with IOU_PBUF_RING_INC. For any
 *			other provided buffer type, all completions with a
 *			buffer passed back is automatically returned to the
 *			application.
 * IORING_CQE_F_SKIP	If set, then the application/liburing must ignore this
 *			CQE. It's only purpose is to fill a gap in the ring,
 *			if a large CQE is attempted posted when the ring has
 *			just a single small CQE worth of space left before
 *			wrapping.
 * IORING_CQE_F_32	If set, this is a 32b/big-cqe posting. Use with rings
 *			setup in a mixed CQE mode, where both 16b and 32b
 *			CQEs may be posted to the CQ ring.
 */
constexpr u32 IORING_CQE_F_BUFFER = (1U << 0);
constexpr u32 IORING_CQE_F_MORE = (1U << 1);
constexpr u32 IORING_CQE_F_SOCK_NONEMPTY = (1U << 2);
constexpr u32 IORING_CQE_F_NOTIF = (1U << 3);
constexpr u32 IORING_CQE_F_BUF_MORE = (1U << 4);
constexpr u32 IORING_CQE_F_SKIP = (1U << 5);
constexpr u32 IORING_CQE_F_32 = (1U << 15);

constexpr u32 IORING_CQE_BUFFER_SHIFT = 16;

/*
 * Magic offsets for the application to mmap the data it needs
 */
constexpr u64 IORING_OFF_SQ_RING = 0ULL;
constexpr u64 IORING_OFF_CQ_RING = 0x8000000ULL;
constexpr u64 IORING_OFF_SQES = 0x10000000ULL;
constexpr u64 IORING_OFF_PBUF_RING = 0x80000000ULL;
constexpr u64 IORING_OFF_PBUF_SHIFT = 16;
constexpr u64 IORING_OFF_MMAP_MASK = 0xf8000000ULL;

/*
 * Filled with the offset for mmap(2)
 */
struct IoSQRingOffsets {
    u32 head;
    u32 tail;
    u32 ring_mask;
    u32 ring_entries;
    u32 flags;
    u32 dropped;
    u32 array;
    u32 resv1;
    u64 user_addr;
};

/*
 * sq_ring->flags
 */
constexpr u32 IORING_SQ_NEED_WAKEUP =
    (1U << 0); /* needs io_uring_enter wakeup */
constexpr u32 IORING_SQ_CQ_OVERFLOW = (1U << 1); /* CQ ring is overflown */
constexpr u32 IORING_SQ_TASKRUN = (1U << 2); /* task should enter the kernel */

struct IoCQRingOffsets {
    u32 head;
    u32 tail;
    u32 ring_mask;
    u32 ring_entries;
    u32 overflow;
    u32 cqes;
    u32 flags;
    u32 resv1;
    u64 user_addr;
};

/*
 * cq_ring->flags
 */

/* disable eventfd notifications */
constexpr u32 IORING_CQ_EVENTFD_DISABLED = (1U << 0);

/*
 * io_uring_enter(2) flags
 */
constexpr u32 IORING_ENTER_GETEVENTS = (1U << 0);
constexpr u32 IORING_ENTER_SQ_WAKEUP = (1U << 1);
constexpr u32 IORING_ENTER_SQ_WAIT = (1U << 2);
constexpr u32 IORING_ENTER_EXT_ARG = (1U << 3);
constexpr u32 IORING_ENTER_REGISTERED_RING = (1U << 4);
constexpr u32 IORING_ENTER_ABS_TIMER = (1U << 5);
constexpr u32 IORING_ENTER_EXT_ARG_REG = (1U << 6);
constexpr u32 IORING_ENTER_NO_IOWAIT = (1U << 7);

/*
 * Passed in for io_uring_setup(2). Copied back with updated info on success
 */
struct IoUringParams {
    u32 sq_entries;
    u32 cq_entries;
    u32 flags;
    u32 sq_thread_cpu;
    u32 sq_thread_idle;
    u32 features;
    u32 wq_fd;
    u32 resv[3];
    IoSQRingOffsets sq_off;
    IoCQRingOffsets cq_off;
};

/*
 * io_uring_params->features flags
 */
constexpr u32 IORING_FEAT_SINGLE_MMAP = (1U << 0);
constexpr u32 IORING_FEAT_NODROP = (1U << 1);
constexpr u32 IORING_FEAT_SUBMIT_STABLE = (1U << 2);
constexpr u32 IORING_FEAT_RW_CUR_POS = (1U << 3);
constexpr u32 IORING_FEAT_CUR_PERSONALITY = (1U << 4);
constexpr u32 IORING_FEAT_FAST_POLL = (1U << 5);
constexpr u32 IORING_FEAT_POLL_32BITS = (1U << 6);
constexpr u32 IORING_FEAT_SQPOLL_NONFIXED = (1U << 7);
constexpr u32 IORING_FEAT_EXT_ARG = (1U << 8);
constexpr u32 IORING_FEAT_NATIVE_WORKERS = (1U << 9);
constexpr u32 IORING_FEAT_RSRC_TAGS = (1U << 10);
constexpr u32 IORING_FEAT_CQE_SKIP = (1U << 11);
constexpr u32 IORING_FEAT_LINKED_FILE = (1U << 12);
constexpr u32 IORING_FEAT_REG_REG_RING = (1U << 13);
constexpr u32 IORING_FEAT_RECVSEND_BUNDLE = (1U << 14);
constexpr u32 IORING_FEAT_MIN_TIMEOUT = (1U << 15);
constexpr u32 IORING_FEAT_RW_ATTR = (1U << 16);
constexpr u32 IORING_FEAT_NO_IOWAIT = (1U << 17);

/*
 * io_uring_register(2) opcodes and arguments
 */
enum class IoUringRegisterOp : u32_c {
    RegisterBuffers = 0,
    UnregisterBuffers = 1,
    RegisterFiles = 2,
    UnregisterFiles = 3,
    RegisterEventFd = 4,
    UnregisterEventFd = 5,
    RegisterFilesUpdate = 6,
    RegisterEventFdAsync = 7,
    RegisterProbe = 8,
    RegisterPersonality = 9,
    UnregisterPersonality = 10,
    RegisterRestrictions = 11,
    RegisterEnableRings = 12,

    /* extended with tagging */
    RegisterFiles2 = 13,
    RegisterFilesUpdate2 = 14,
    RegisterBuffers2 = 15,
    RegisterBuffersUpdate = 16,

    /* set/clear io-wq thread affinities */
    RegisterIoWqAff = 17,
    UnregisterIoWqAff = 18,

    /* set/get max number of io-wq workers */
    RegisterIoWqMaxWorkers = 19,

    /* register/unregister io_uring fd with the ring */
    RegisterRingFds = 20,
    UnregisterRingFds = 21,

    /* register ring based provide buffer group */
    RegisterPBufRing = 22,
    UnregisterPBufRing = 23,

    /* sync cancelation API */
    RegisterSyncCancel = 24,

    /* register a range of fixed file slots for automatic slot allocation */
    RegisterFileAllocRange = 25,

    /* return status information for a buffer group */
    RegisterPBufStatus = 26,

    /* set/clear busy poll settings */
    RegisterNapi = 27,
    UnregisterNapi = 28,

    RegisterClock = 29,

    /* clone registered buffers from source ring to current ring */
    RegisterCloneBuffers = 30,

    /* send MSG_RING without having a ring */
    RegisterSendMsgRing = 31,

    /* register a netdev hw rx queue for zerocopy */
    RegisterZcRxIfq = 32,

    /* resize CQ ring */
    RegisterResizeRings = 33,

    RegisterMemRegion = 34,

    /* query various aspects of io_uring, see linux/io_uring/query.h */
    RegisterQuery = 35,

    /* auxiliary zcrx configuration, see enum zcrx_ctrl_op */
    RegisterZcRxCtrl = 36,

    /* register bpf filtering programs */
    RegisterBpfFilter = 37,

    /* this goes last */
    RegisterLast,

    /* flag added to the opcode to use a registered ring fd */
    RegisterUseRegisteredRing = 1U << 31,
};

/* io-wq worker categories */
enum class IoUringIoWqType : u32_c {
    Bound,
    Unbound,
};

/* deprecated, see struct io_uring_rsrc_update */
struct IoUringFilesUpdate {
    u32 offset;
    u32 resv;
    u64 /* i32* */ fds;
};

/* initialise with user provided memory pointed by user_addr */
constexpr u32 IORING_MEM_REGION_TYPE_USER = 1;

struct IoUringRegionDesc {
    u64 user_addr;
    u64 size;
    u32 flags;
    u32 id;
    u64 mmap_offset;
    u64 __resv[4];
};

/* expose the region as registered wait arguments */
constexpr u32 IORING_MEM_REGION_REG_WAIT_ARG = 1;

struct IoUringMemRegionReg {
    u64 region_uptr; /* struct io_uring_region_desc * */
    u64 flags;
    u64 __resv[2];
};

/*
 * Register a fully sparse file space, rather than pass in an array of all
 * -1 file descriptors.
 */
constexpr u32 IORING_RSRC_REGISTER_SPARSE = (1U << 0);

struct IoUringRsRcRegister {
    u32 nr;
    u32 flags;
    u64 resv2;
    u64 data;
    u64 tags;
};

struct IoUringRsRcUpdate {
    u32 offset;
    u32 resv;
    u64 data;
};

struct IoUringRsRcUpdate2 {
    u32 offset;
    u32 resv;
    u64 data;
    u64 tags;
    u32 nr;
    u32 resv2;
};

/* Skip updating fd indexes set to this value in the fd table */
constexpr u32 IORING_REGISTER_FILES_SKIP = (u32)i32{-2};

constexpr u32 IO_URING_OP_SUPPORTED = (1U << 0);

struct IoUringProbeOp {
    u8 op;
    u8 resv;
    u16 flags; /* IO_URING_OP_* flags */
    u32 resv2;
};

struct IoUringProbe {
    u8 last_op; /* last opcode supported */
    u8 ops_len; /* length of ops[] array below */
    u16 resv;
    u32 resv2[3];
    IoUringProbeOp ops[];
};

struct IoUringRestriction {
    u16 opcode;
    union {
        u8 register_op; /* IORING_RESTRICTION_REGISTER_OP */
        u8 sqe_op;      /* IORING_RESTRICTION_SQE_OP */
        u8 sqe_flags;   /* IORING_RESTRICTION_SQE_FLAGS_* */
    };
    u8 resv;
    u32 resv2[3];
};

struct IoUringTaskRestriction {
    u16 flags;
    u16 nr_res;
    u32 resv[3];
    IoUringRestriction restrictions[0];
};

struct IoUringClockRegister {
    u32 clockid;
    u32 __resv[3];
};

constexpr u32 IORING_REGISTER_SRC_REGISTERED = (1U << 0);
constexpr u32 IORING_REGISTER_DST_REPLACE = (1U << 1);

struct IoUringCloneBuffers {
    u32 src_fd;
    u32 flags;
    u32 src_off;
    u32 dst_off;
    u32 nr;
    u32 pad[3];
};

struct IoUringBuf {
    u64 addr;
    u32 len;
    u16 bid;
    u16 resv;
};

struct IoUringBufRing {
    union {
        /*
         * To avoid spilling into more pages than we need to, the
         * ring tail is overlaid with the io_uring_buf->resv field.
         */
        struct {
            u64 resv1;
            u32 resv2;
            u16 resv3;
            u16 tail;
        };
        IoUringBuf bufs[0];
    };
};

/*
 * Flags for IORING_REGISTER_PBUF_RING.
 *
 * IOU_PBUF_RING_MMAP:	If set, kernel will allocate the memory for the ring.
 *			The application must not set a ring_addr in struct
 *			io_uring_buf_reg, instead it must subsequently call
 *			mmap(2) with the offset set as:
 *			IORING_OFF_PBUF_RING | (bgid << IORING_OFF_PBUF_SHIFT)
 *			to get a virtual mapping for the ring.
 * IOU_PBUF_RING_INC:	If set, buffers consumed from this buffer ring can be
 *			consumed incrementally. Normally one (or more) buffers
 *			are fully consumed. With incremental consumptions, it's
 *			feasible to register big ranges of buffers, and each
 *			use of it will consume only as much as it needs. This
 *			requires that both the kernel and application keep
 *			track of where the current read/recv index is at.
 */
enum class IoUringRegisterPBufRingFlags : u32_c {
    MMap = 1,
    Inc = 2,
};

/* argument for IORING_(UN)REGISTER_PBUF_RING */
struct IoUringBufReg {
    u64 ring_addr;
    u32 ring_entries;
    u16 bgid;
    u16 flags;
    u32 min_left;
    u32 resv[5];
};

/* argument for IORING_REGISTER_PBUF_STATUS */
struct IoUringBufStatus {
    u32 buf_group; /* input */
    u32 head;      /* output */
    u32 resv[8];
};

/* argument for IORING_(UN)REGISTER_NAPI */
struct IoUringNapi {
    u32 busy_poll_to;
    u8 prefer_busy_poll;
    u8 pad[3];
    u64 resv;
};

/*
 * io_uring_restriction->opcode values
 */
enum class IoUringRegisterRestrictionOp : u32_c {
    RegisterOp = 0,
    SqeOp = 1,
    SqeFlagsAllowed = 2,
    SqeFlagsRequired = 3,
    Last,
};

constexpr u32 IORING_REG_WAIT_TS = (1U << 0);

/*
 * Argument for io_uring_enter(2) with IORING_GETEVENTS | IORING_ENTER_EXT_ARG
 */
struct IoUringGetEventsArg {
    u64 sigmask;
    u32 sigmask_sz;
    u32 min_wait_usec;
    u64 ts;
};

/*
 * Argument for IORING_REGISTER_FILE_ALLOC_RANGE
 * The range is specified as [off, off + len)
 */
struct IoUringFileIndexRange {
    u32 off;
    u32 len;
    u64 resv;
};

struct IoUringRecvmsgOut {
    u32 namelen;
    u32 controllen;
    u32 payloadlen;
    u32 flags;
};

/*
 * Argument for IORING_OP_URING_CMD when file is a socket
 */
enum class IoUringSocketOp : u32_c {
    SioCInQ = 0,
    SioCOutQ,
    GetSockOpt,
    SetSockOpt,
    TxTimestamp,
    GetSockname,
};

/*
 * SOCKET_URING_OP_TX_TIMESTAMP definitions
 */

constexpr u32 IORING_TIMESTAMP_HW_SHIFT = 16;
/* The cqe->flags bit from which the timestamp type is stored */
constexpr u32 IORING_TIMESTAMP_TYPE_SHIFT = (IORING_TIMESTAMP_HW_SHIFT + 1);
/* The cqe->flags flag signifying whether it's a hardware timestamp */
constexpr u32 IORING_CQE_F_TSTAMP_HW = ((u32)1 << IORING_TIMESTAMP_HW_SHIFT);

struct IoTimespec {
    u64 tv_sec;
    u64 tv_nsec;
};

/* Zero copy receive refill queue entry */
struct IoUringZcRxRqe {
    u64 off;
    u32 len;
    u32 __pad;
};

struct IoUringZcRxCqe {
    u64 off;
    u64 __pad;
};

/* The bit from which area id is encoded into offsets */
constexpr u64 IORING_ZCRX_AREA_SHIFT = 48;
constexpr u64 IORING_ZCRX_AREA_MASK =
    (~(((u64)1 << IORING_ZCRX_AREA_SHIFT) - 1));

struct IoUringZcRxOffsets {
    u32 head;
    u32 tail;
    u32 rqes;
    u32 __resv2;
    u64 __resv[2];
};

enum class IoUringZcRxAreaFlags : u32_c {
    DmaBuf = 1,
};

struct IoUringZcRxAreaReg {
    u64 addr;
    u64 len;
    u64 rq_area_token;
    u32 flags;
    u32 dmabuf_fd;
    u64 __resv2[2];
};

enum class ZcRxRegFlags : u32_c {
    Import = 1,
    /*
     * Register a zcrx instance without a net device. All data will be
     * copied. The refill queue entries might not be automatically
     * consumed and need to be flushed, see ZCRX_CTRL_FLUSH_RQ.
     */
    NodeV = 2,
};

enum class ZcRxFeatures : u32_c {
    /*
     * The user can ask for the desired rx page size by passing the
     * value in struct io_uring_zcrx_ifq_reg::rx_buf_len.
     */
    PageSize = 1 << 0,
};

/*
 * Argument for IORING_REGISTER_ZCRX_IFQ
 */
struct IoUringZcRxIfqReg {
    u32 if_idx;
    u32 if_rxq;
    u32 rq_entries;
    u32 flags;

    u64 area_ptr;   /* pointer to struct io_uring_zcrx_area_reg */
    u64 region_ptr; /* struct io_uring_region_desc * */

    IoUringZcRxOffsets offsets;
    u32 zcrx_id;
    u32 rx_buf_len;
    u64 __resv[3];
};

enum class ZcRxCtrlOp : u32_c {
    FlushRq,
    Export,
    Last,
};

struct ZcRxCtrlFlushRq {
    u64 __resv[6];
};

struct ZcRxCtrlExport {
    u32 zcrx_fd;
    u32 __resv1[11];
};

struct ZcRxCtrl {
    u32 zcrx_id;
    u32 op; /* see enum zcrx_ctrl_op */
    u64 __resv[2];

    union {
        ZcRxCtrlExport zc_export;
        ZcRxCtrlFlushRq zc_flush;
    };
};

} // extern "C"

} // namespace io_uring

} // namespace zx
