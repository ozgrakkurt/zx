export module zx:time;
import :def;
import :sys;
import :arith;
import :perf_event;
import :assert;

export namespace zx {

struct TimeInfo {
    u64 m_time_shift;
    u64 m_time_mult;

    static constexpr u64 MMAP_LEN = 4096;

    // function for cleaning up temporary resources that
    // are allocated in the `init` function.
    static void cleanup_(sys::PerfEventMmapPage* mmap_page, i32 fd) {
        if (mmap_page != nullptr) {
            sys::MMapStatus const sys_status = sys::munmap((u8*)mmap_page, MMAP_LEN);
            // we silently ignore error on non-dev mode here.
            // this should be ok since this call shouldn't fail.
            assert_dev(sys_status == sys::MMapStatus::Ok);
        }
        if (fd != -1) {
            sys::CloseStatus const sys_status = sys::close(fd);
            // we silently ignore error on non-dev mode here.
            // this should be ok since this call shouldn't fail.
            assert_dev(sys_status == sys::CloseStatus::Ok);
        }
    }

    enum class [[nodiscard]] InitStatus : u8_c {
        Ok = 0,
        Nullptr,
        PerfEventOpenErr,
        MMapErr,
        NotSupported,
        InvalidNum,
    };

    static InitStatus init(TimeInfo *out) {
        if (out == nullptr) {
            return InitStatus::Nullptr;
        }
        
        sys::PerfEventAttr pattr;
        __builtin_memset(&pattr, 0, sizeof(pattr));
        pattr.type = (u32)sys::PerfEventTypeId::Hardware;
        pattr.size = sizeof(pattr);
        pattr.config = (u64)sys::PerfEventHwId::CountHwInstructions;
        pattr.disabled = 1;
        pattr.exclude_kernel = 1;
        pattr.exclude_hv = 1;

        i32 fd = -1;
        sys::PerfEventMmapPage *mmap_page = nullptr;

        sys::PerfEventOpenStatus const pe_open_status =
            sys::perf_event_open(&pattr, 0, -1, -1, 0, &fd);
        if (pe_open_status != sys::PerfEventOpenStatus::Ok) {
            cleanup_(mmap_page, fd);
            return InitStatus::PerfEventOpenErr;
        }
        assert_dev(fd != -1);

        sys::MMapStatus const mmap_status =
            sys::mmap(nullptr, MMAP_LEN, sys::PROT_READ, sys::MAP_SHARED, fd,
                        0, (u8 **)&mmap_page);
        if (mmap_status != sys::MMapStatus::Ok) {
            cleanup_(mmap_page, fd);
            return InitStatus::MMapErr;
        }
        assert_dev(mmap_page != nullptr);

        if (mmap_page->cap_user_time != 1) {
            cleanup_(mmap_page, fd);
            return InitStatus::NotSupported;
        }

        TimeInfo const info = TimeInfo{
            .m_time_shift = mmap_page->time_shift,
            .m_time_mult = mmap_page->time_mult,
        };

        // This would cause ub when we are converting cycles to nanos later.
        if (info.m_time_shift >= 128 || info.m_time_mult == 0) {
            cleanup_(mmap_page, fd);
            return InitStatus::InvalidNum;
        }

        *out = info;
        return InitStatus::Ok;
    }

    u64 cycles_to_nanos(u64 cycles) const {
        u128 const x = cycles;
        assert_dev(this->m_time_shift < 128__wbu);
        return (u64)((x * this->m_time_mult) >> this->m_time_shift);
    }
};

struct Timer {
    TimeInfo m_info;
    u64 m_start;

    static u64 now_() {
        unsigned int aux;
        u64 const ticker = __builtin_ia32_rdtscp(&aux);
        (void)aux;

        return ticker;
    }

    static Timer init(TimeInfo info) {
        u64 const start = now_();

        return Timer{
            .m_start = start,
            .m_info = info,
        };
    }

    // Returns the number of nanoseconds elapsed since last lap.
    // and resets the lap timer.
    u64 lap() {
        u64 const now = now_();
        u64 const diff = sub_unsafe_u64(now, this->m_start);
        this->m_start = now;
        return this->m_info.cycles_to_nanos(diff);
    }

    // Returns the number of nanoseconds elapsed since last lap.
    u64 elapsed() const {
        u64 const now = now_();
        u64 const diff = sub_unsafe_u64(now, this->m_start);
        return this->m_info.cycles_to_nanos(diff);
    }
};

} // namespace zx
