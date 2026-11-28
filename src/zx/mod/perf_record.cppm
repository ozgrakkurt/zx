// Basic benchmark counters using perf_event_open.
//
// This is intentionally simple:
//   - five generic hardware events: cycles, instructions, branches,
//     branch misses and cache misses, opened as a single perf event group,
//   - page faults and context switches from getrusage(RUSAGE_THREAD).
//
// There is no sampling, no mmap, no rdpmc and no ring buffer. init() opens the
// events, start() resets/enables the group, stop() disables and reads it, and
// deinit() closes them. This is intended for CPU benchmarks where a handful of
// syscalls at the measurement boundaries do not matter.
//
// All events use pid=0 and cpu=-1, so they only measure the calling thread /
// task, on whatever CPU it happens to run on. They are never CPU-wide and
// never include other threads or processes. Context switches and page faults
// use getrusage(RUSAGE_THREAD), which is also per-thread.
//
// exclude_kernel=1 and exclude_hv=1 keep the hardware events to user space and
// make them work unprivileged with perf_event_paranoid <= 2.
//
// The group is scheduled as a unit, so all hardware events cover the same set
// of executed instructions and their values can be meaningfully compared. The
// counters are read without scaling. On Zen 4+ five hardware events fit in the
// six general purpose counters, so the group should not be multiplexed. On
// CPUs with fewer counters the group may be multiplexed as a whole, in which
// case the raw counts under-report by a single factor shared by all events.

export module zx:perf_record;
import :def;
import :sys;
import :syscall;
import :perf_event;
import :backtrace;
import :assert;

export namespace zx {

namespace perf_record {

constexpr u64 NR_IOCTL_ = 16;
constexpr u64 NR_GETRUSAGE_ = 98;
constexpr u64 RUSAGE_THREAD_ = 1;

// <asm-generic/ioctls.h>: _IO('$', n)
constexpr u64 PERF_EVENT_IOC_ENABLE_ = 0x2400;
constexpr u64 PERF_EVENT_IOC_DISABLE_ = 0x2401;
constexpr u64 PERF_EVENT_IOC_RESET_ = 0x2403;

// <linux/perf_event.h>
constexpr u64 PERF_FORMAT_TOTAL_TIME_ENABLED_ = 1 << 0;
constexpr u64 PERF_FORMAT_TOTAL_TIME_RUNNING_ = 1 << 1;
constexpr u64 PERF_FORMAT_GROUP_ = 1 << 3;

// pid=0 with cpu=-1 measures the calling thread on any CPU.
constexpr i32 CURRENT_THREAD_PID_ = 0;
constexpr i32 ANY_CPU_ = -1;

struct Timeval_ {
    i64 m_sec;
    i64 m_usec;
};

// Matches struct rusage in <sys/resource.h> on Linux x86_64.
struct RUsage_ {
    Timeval_ m_utime;
    Timeval_ m_stime;
    i64 m_maxrss;
    i64 m_ixrss;
    i64 m_idrss;
    i64 m_isrss;
    i64 m_minflt;
    i64 m_majflt;
    i64 m_nswap;
    i64 m_inblock;
    i64 m_oublock;
    i64 m_msgsnd;
    i64 m_msgrcv;
    i64 m_nsignals;
    i64 m_nvcsw;
    i64 m_nivcsw;
};

static_assert(sizeof(RUsage_) == 144);

enum class HwIndex_ : u8_c {
    Cycles = 0,
    Instructions,
    BranchInstructions,
    BranchMisses,
    CacheMisses,

    Count,
};

constexpr u64 N_HW_EVENTS_ = (u64)HwIndex_::Count;

constexpr perf_event::PerfEventHwId HW_CONFIGS_[N_HW_EVENTS_] = {
    perf_event::PerfEventHwId::CountHwCpuCycles,
    perf_event::PerfEventHwId::CountHwInstructions,
    perf_event::PerfEventHwId::CountHwBranchInstructions,
    perf_event::PerfEventHwId::CountHwBranchMisses,
    perf_event::PerfEventHwId::CountHwCacheMisses,
};

// Layout the kernel writes for a PERF_FORMAT_GROUP read with
// PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING:
//   u64 nr, time_enabled, time_running;
//   struct { u64 value; } values[nr];
// The values are in the order the events were added to the group, which is
// HW_CONFIGS_ order here (group leader first).
struct GroupCount_ {
    u64 m_nr;
    u64 m_time_enabled;
    u64 m_time_running;
    u64 m_values[N_HW_EVENTS_];
};

static_assert(sizeof(GroupCount_) == 3 * 8 + N_HW_EVENTS_ * 8);

inline i32 ioctl_(i32 fd, u64 request, u64 arg) {
    return (i32)syscall3(NR_IOCTL_, (u64)fd, request, arg);
}

// RUSAGE_THREAD only includes the calling thread, not the whole process.
inline bool getrusage_(RUsage_ *out) {
    return (i64)syscall2(NR_GETRUSAGE_, RUSAGE_THREAD_, (u64)out) == 0;
}

inline u64 nonneg_diff_(i64 later, i64 earlier) {
    return (later > earlier) ? (u64)(later - earlier) : 0;
}

} // namespace perf_record

struct Recorder {
    static constexpr u64 N_HW_EVENTS = perf_record::N_HW_EVENTS_;
    static constexpr u64 N_EVENTS = N_HW_EVENTS;

    enum class [[nodiscard]] InitStatus : u8_c {
        Ok = 0,
        Nullptr,
        PerfEventOpenErr,
    };

    u64 m_page_faults;
    u64 m_page_faults_minor;
    u64 m_page_faults_major;
    u64 m_context_switches;

    u64 m_cycles;
    u64 m_instructions;
    u64 m_branch_instructions;
    u64 m_branch_misses;
    u64 m_cache_misses; // On AMD this is the L2 miss count.

    // True if the hardware group was multiplexed during the interval. In that
    // case the raw counts under-report. Because the group is scheduled as a
    // unit, all events share the same scale factor.
    bool m_hw_multiplexed;

    i32 m_fds[N_EVENTS];
    perf_record::RUsage_ m_start_usage;
    bool m_running;

    inline static void cleanup_(Backtrace *err_bt, Recorder *out) {
        for (u64 i = 0; i < N_EVENTS; ++i) {
            if (out->m_fds[i] >= 0) {
                sys::CloseStatus const status =
                    sys::close(err_bt, out->m_fds[i]);
                assert_dev(status == sys::CloseStatus::Ok);
                out->m_fds[i] = -1;
            }
        }
    }

    inline static sys::PerfEventOpenStatus
    open_hw_(Backtrace *err_bt, perf_event::PerfEventHwId config, i32 group_fd,
             u64 read_format, i32 *fd) {
        perf_event::PerfEventAttr attr;
        __builtin_memset(&attr, 0, sizeof(attr));
        attr.type = (u32)perf_event::PerfEventTypeId::Hardware;
        attr.size = sizeof(attr);
        attr.config = (u64)config;
        // The leader controls the whole group; members start disabled
        // regardless of their own disabled bit.
        attr.disabled = (group_fd == -1) ? 1 : 0;
        attr.exclude_kernel = 1;
        attr.exclude_hv = 1;
        attr.read_format = read_format;

        return sys::perf_event_open(err_bt, &attr,
                                    perf_record::CURRENT_THREAD_PID_,
                                    perf_record::ANY_CPU_, group_fd, 0, fd);
    }

    inline static InitStatus init(Backtrace *err_bt, Recorder *out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return InitStatus::Nullptr;
        }

        __builtin_memset(out, 0, sizeof(*out));
        for (u64 i = 0; i < N_EVENTS; ++i) {
            out->m_fds[i] = -1;
        }

        // The group leader carries the group read format and is the fd used to
        // enable, disable, reset and read the whole group.
        i32 leader_fd = -1;
        sys::PerfEventOpenStatus const leader_status = open_hw_(
            err_bt, perf_record::HW_CONFIGS_[0], -1,
            perf_record::PERF_FORMAT_GROUP_ |
                perf_record::PERF_FORMAT_TOTAL_TIME_ENABLED_ |
                perf_record::PERF_FORMAT_TOTAL_TIME_RUNNING_,
            &leader_fd);
        if (leader_status != sys::PerfEventOpenStatus::Ok) [[unlikely]] {
            cleanup_(nullptr, out);
            return InitStatus::PerfEventOpenErr;
        }
        out->m_fds[0] = leader_fd;

        for (u64 i = 1; i < N_HW_EVENTS; ++i) {
            i32 fd = -1;
            sys::PerfEventOpenStatus const status = open_hw_(
                err_bt, perf_record::HW_CONFIGS_[i], leader_fd, 0, &fd);
            if (status != sys::PerfEventOpenStatus::Ok) [[unlikely]] {
                cleanup_(nullptr, out);
                return InitStatus::PerfEventOpenErr;
            }
            out->m_fds[i] = fd;
        }

        return InitStatus::Ok;
    }

    inline void deinit(Backtrace *err_bt) {
        assert(!this->m_running);
        cleanup_(err_bt, this);
    }

    inline void start() {
        assert(!this->m_running);
        i32 const leader_fd = this->m_fds[0];
        for (u64 i = 0; i < N_EVENTS; ++i) {
            assert_dev(this->m_fds[i] != -1);
        }

        // Resetting/enabling the group leader applies to the whole group.
        assert_dev(perf_record::ioctl_(
                       leader_fd, perf_record::PERF_EVENT_IOC_RESET_, 0) == 0);
        // Snapshot rusage just before the group starts so the rusage window
        // encloses the hardware window.
        assert_dev(perf_record::getrusage_(&this->m_start_usage));
        assert_dev(perf_record::ioctl_(
                       leader_fd, perf_record::PERF_EVENT_IOC_ENABLE_, 0) == 0);

        this->m_running = true;
    }

    inline static perf_record::GroupCount_ read_group_(i32 fd) {
        perf_record::GroupCount_ out;
        __builtin_memset(&out, 0, sizeof(out));

        u64 n_read = 0;
        sys::ReadStatus const status =
            sys::read(nullptr, fd, (u8 *)&out, sizeof(out), &n_read);
        assert_dev(status == sys::ReadStatus::Ok);
        assert_dev(n_read == sizeof(out));
        return out;
    }

    inline void stop() {
        assert(this->m_running);
        i32 const leader_fd = this->m_fds[0];
        for (u64 i = 0; i < N_EVENTS; ++i) {
            assert_dev(this->m_fds[i] != -1);
        }

        assert_dev(perf_record::ioctl_(
                       leader_fd, perf_record::PERF_EVENT_IOC_DISABLE_,
                       0) == 0);

        perf_record::RUsage_ end_usage;
        __builtin_memset(&end_usage, 0, sizeof(end_usage));
        // Snapshot rusage just after the group stops, matching start().
        assert_dev(perf_record::getrusage_(&end_usage));

        perf_record::GroupCount_ const count = read_group_(leader_fd);
        assert_dev(count.m_nr == N_HW_EVENTS);
        this->m_hw_multiplexed =
            count.m_time_running != count.m_time_enabled;

        u64 hw[N_HW_EVENTS];
        for (u64 i = 0; i < N_HW_EVENTS; ++i) {
            hw[i] = count.m_values[i];
        }

        this->m_page_faults_minor = perf_record::nonneg_diff_(
            end_usage.m_minflt, this->m_start_usage.m_minflt);
        this->m_page_faults_major = perf_record::nonneg_diff_(
            end_usage.m_majflt, this->m_start_usage.m_majflt);
        this->m_page_faults =
            this->m_page_faults_minor + this->m_page_faults_major;

        u64 const voluntary = perf_record::nonneg_diff_(
            end_usage.m_nvcsw, this->m_start_usage.m_nvcsw);
        u64 const involuntary = perf_record::nonneg_diff_(
            end_usage.m_nivcsw, this->m_start_usage.m_nivcsw);
        this->m_context_switches = voluntary + involuntary;

        this->m_cycles = hw[(u64)perf_record::HwIndex_::Cycles];
        this->m_instructions = hw[(u64)perf_record::HwIndex_::Instructions];
        this->m_branch_instructions =
            hw[(u64)perf_record::HwIndex_::BranchInstructions];
        this->m_branch_misses = hw[(u64)perf_record::HwIndex_::BranchMisses];
        this->m_cache_misses = hw[(u64)perf_record::HwIndex_::CacheMisses];

        this->m_running = false;
    }
};

} // namespace zx
