// Basic benchmark counters using perf_event_open.
//
// This is intentionally simple:
//   - five generic hardware events: cycles, instructions, branches,
//     branch misses and cache misses,
//   - three generic software events: page faults, minor faults and major
//     faults.
//
// There is no sampling, no mmap, no rdpmc and no ring buffer. init() opens the
// events, start() resets/enables them, stop() disables and reads them, and
// deinit() closes them. This is intended for CPU benchmarks where a handful of
// syscalls at the measurement boundaries do not matter.
//
// All events use pid=0 and cpu=-1, so they only measure the calling thread /
// task, on whatever CPU it happens to run on. They are never CPU-wide and
// never include other threads or processes. Context switches use
// getrusage(RUSAGE_THREAD), which is also per-thread.
//
// exclude_kernel=1 and exclude_hv=1 keep the hardware events to user space and
// make them work unprivileged with perf_event_paranoid <= 2.
//
// The counters are read without scaling. On Zen 4+ five hardware events fit in
// the six general purpose counters, so they should not be multiplexed. On CPUs
// with fewer counters an event may be multiplexed and its raw count will
// under-report.

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

// PERF_COUNT_SW_* values.
constexpr u64 SW_PAGE_FAULTS_ = 2;
constexpr u64 SW_PAGE_FAULTS_MIN_ = 5;
constexpr u64 SW_PAGE_FAULTS_MAJ_ = 6;

// <linux/perf_event.h>
constexpr u64 PERF_FORMAT_TOTAL_TIME_ENABLED_ = 1 << 0;
constexpr u64 PERF_FORMAT_TOTAL_TIME_RUNNING_ = 1 << 1;

struct HardwareCount_ {
    u64 m_value;
    u64 m_time_enabled;
    u64 m_time_running;
};

constexpr i32 CURRENT_PROCESS_PID_ = 0;
constexpr i32 NO_CPU_ = -1;

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

enum class SwIndex_ : u8_c {
    PageFaults = 0,
    PageFaultsMinor,
    PageFaultsMajor,

    Count,
};

constexpr u64 N_SW_EVENTS_ = (u64)SwIndex_::Count;

constexpr u64 SW_CONFIGS_[N_SW_EVENTS_] = {
    SW_PAGE_FAULTS_,
    SW_PAGE_FAULTS_MIN_,
    SW_PAGE_FAULTS_MAJ_,
};

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
    static constexpr u64 N_SW_EVENTS = perf_record::N_SW_EVENTS_;
    static constexpr u64 N_EVENTS = N_HW_EVENTS + N_SW_EVENTS;

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

    // True if any hardware event was multiplexed during the interval. In that
    // case the raw counts under-report and the measurement is not exact.
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
    open_hw_(Backtrace *err_bt, perf_event::PerfEventHwId config, i32 *fd) {
        perf_event::PerfEventAttr attr;
        __builtin_memset(&attr, 0, sizeof(attr));
        attr.type = (u32)perf_event::PerfEventTypeId::Hardware;
        attr.size = sizeof(attr);
        attr.config = (u64)config;
        attr.disabled = 1;
        attr.exclude_kernel = 1;
        attr.exclude_hv = 1;
        attr.read_format = perf_record::PERF_FORMAT_TOTAL_TIME_ENABLED_ |
                           perf_record::PERF_FORMAT_TOTAL_TIME_RUNNING_;

        return sys::perf_event_open(err_bt, &attr,
                                    perf_record::CURRENT_PROCESS_PID_,
                                    perf_record::NO_CPU_, -1, 0, fd);
    }

    inline static sys::PerfEventOpenStatus open_sw_(Backtrace *err_bt,
                                                    u64 config, i32 *fd) {
        perf_event::PerfEventAttr attr;
        __builtin_memset(&attr, 0, sizeof(attr));
        attr.type = (u32)perf_event::PerfEventTypeId::Software;
        attr.size = sizeof(attr);
        attr.config = config;
        attr.disabled = 1;
        attr.exclude_kernel = 1;
        attr.exclude_hv = 1;

        return sys::perf_event_open(err_bt, &attr,
                                    perf_record::CURRENT_PROCESS_PID_,
                                    perf_record::NO_CPU_, -1, 0, fd);
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

        for (u64 i = 0; i < N_HW_EVENTS; ++i) {
            i32 fd = -1;
            sys::PerfEventOpenStatus const status =
                open_hw_(err_bt, perf_record::HW_CONFIGS_[i], &fd);
            if (status != sys::PerfEventOpenStatus::Ok) [[unlikely]] {
                cleanup_(nullptr, out);
                return InitStatus::PerfEventOpenErr;
            }
            out->m_fds[i] = fd;
        }

        for (u64 i = 0; i < N_SW_EVENTS; ++i) {
            i32 fd = -1;
            sys::PerfEventOpenStatus const status =
                open_sw_(err_bt, perf_record::SW_CONFIGS_[i], &fd);
            if (status != sys::PerfEventOpenStatus::Ok) [[unlikely]] {
                cleanup_(nullptr, out);
                return InitStatus::PerfEventOpenErr;
            }
            out->m_fds[N_HW_EVENTS + i] = fd;
        }

        return InitStatus::Ok;
    }

    inline void deinit(Backtrace *err_bt) {
        assert(!this->m_running);
        cleanup_(err_bt, this);
    }

    inline void start() {
        assert(!this->m_running);
        for (u64 i = 0; i < N_EVENTS; ++i) {
            i32 const fd = this->m_fds[i];
            assert_dev(fd != -1);
            assert_dev(perf_record::ioctl_(
                           fd, perf_record::PERF_EVENT_IOC_RESET_, 0) == 0);
        }
        for (u64 i = 0; i < N_EVENTS; ++i) {
            i32 const fd = this->m_fds[i];
            assert_dev(fd != -1);
            assert_dev(perf_record::ioctl_(
                           fd, perf_record::PERF_EVENT_IOC_ENABLE_, 0) == 0);
        }

        assert_dev(perf_record::getrusage_(&this->m_start_usage));
        this->m_running = true;
    }

    inline static u64 read_count_(i32 fd) {
        u64 value = 0;
        u64 n_read = 0;
        sys::ReadStatus const status =
            sys::read(nullptr, fd, (u8 *)&value, sizeof(value), &n_read);
        assert_dev(status == sys::ReadStatus::Ok);
        assert_dev(n_read == sizeof(value));
        return value;
    }

    inline static perf_record::HardwareCount_ read_hw_count_(i32 fd) {
        perf_record::HardwareCount_ out;
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

        perf_record::RUsage_ end_usage;
        __builtin_memset(&end_usage, 0, sizeof(end_usage));
        bool const usage_ok = perf_record::getrusage_(&end_usage);
        assert_dev(usage_ok);

        for (u64 i = 0; i < N_EVENTS; ++i) {
            i32 const fd = this->m_fds[i];
            assert_dev(fd != -1);
            assert_dev(perf_record::ioctl_(
                           fd, perf_record::PERF_EVENT_IOC_DISABLE_, 0) == 0);
        }

        u64 hw[N_HW_EVENTS];
        this->m_hw_multiplexed = false;
        for (u64 i = 0; i < N_HW_EVENTS; ++i) {
            perf_record::HardwareCount_ const count =
                read_hw_count_(this->m_fds[i]);
            hw[i] = count.m_value;
            if (count.m_time_running != count.m_time_enabled) {
                this->m_hw_multiplexed = true;
            }
        }

        u64 sw[N_SW_EVENTS];
        for (u64 i = 0; i < N_SW_EVENTS; ++i) {
            sw[i] = read_count_(this->m_fds[N_HW_EVENTS + i]);
        }

        this->m_page_faults = sw[(u64)perf_record::SwIndex_::PageFaults];
        this->m_page_faults_minor =
            sw[(u64)perf_record::SwIndex_::PageFaultsMinor];
        this->m_page_faults_major =
            sw[(u64)perf_record::SwIndex_::PageFaultsMajor];

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

        assert_dev(!this->m_hw_multiplexed);
        this->m_running = false;
    }
};

} // namespace zx
