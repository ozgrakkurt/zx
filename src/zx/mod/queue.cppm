export module zx:queue;
import :def;
import :slice;
import :arith;
import :alloc;
import :assert;
import :backtrace;

export namespace zx {

enum class [[nodiscard]] QueueInitStatus : u8_c {
    Ok = 0,
    OutOfMemory,
    AllocFail,
    IntegerOverflow,
    Nullptr,
};

enum class [[nodiscard]] QueuePushStatus : u8_c {
    Ok = 0,
    OutOfCapacity,
    Nullptr,
};

enum class [[nodiscard]] QueuePopStatus : u8_c {
    Ok = 0,
    Empty,
    Nullptr,
};

template <typename T> struct Queue {
    SliceMut<T> m_entries;
    u32 m_start;
    u32 m_len;
    u32 m_cap_mask;
    u32 m_capacity;

    inline static QueueInitStatus init(Backtrace *err_bt, Alloc *alloc,
                                       u32 capacity, Queue *queue) {
        if (alloc == nullptr || queue == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return QueueInitStatus::Nullptr;
        }

        bool overflow = false;

        u32 const cap =
            (capacity == 0) ? 0 : next_pow_two_u32(capacity, &overflow);

        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return QueueInitStatus::IntegerOverflow;
        }

        SliceMut<T> entries;
        switch (alloc->alloc<T>(err_bt, cap, &entries)) {
        case AllocStatus::Ok: {
            break;
        }
        case AllocStatus::OutOfMemory: {
            return QueueInitStatus::OutOfMemory;
        }
        default: {
            return QueueInitStatus::AllocFail;
        }
        }

        *queue = Queue{
            .m_entries = entries,
            .m_start = 0,
            .m_len = 0,
            .m_cap_mask = saturating_sub_u32(cap, 1),
            .m_capacity = capacity,
        };

        return QueueInitStatus::Ok;
    }

    inline void reset() {
#ifdef ZX_DEV
        this->m_entries.memset(0xaa);
#endif
        this->m_start = 0;
        this->m_len = 0;
    }

    inline u32 capacity() const { return this->m_capacity; }

    inline u32 capacity_left() const {
        return sub_unsafe_u32(this->m_capacity, this->m_len);
    }

    inline u32 len() const { return this->m_len; }

    inline QueuePushStatus push(Backtrace *err_bt, T const *entry) {
        if (entry == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return QueuePushStatus::Nullptr;
        }

        if (this->m_capacity == this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return QueuePushStatus::OutOfCapacity;
        }

        u32 const end =
            add_unsafe_u32(this->m_start, this->m_len) & this->m_cap_mask;

        this->m_entries.write_unsafe(end, *entry);
        this->m_len = add_unsafe_u32(this->m_len, 1);
        return QueuePushStatus::Ok;
    }

    inline QueuePopStatus pop(Backtrace *err_bt, T *entry) {
        if (entry == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return QueuePopStatus::Nullptr;
        }

        if (this->m_len == 0) [[unlikely]] {
            Backtrace::collect(err_bt);
            return QueuePopStatus::Empty;
        }

        u32 const start = this->m_start;
        *entry = this->m_entries.read_unsafe(start);

        this->m_start = add_unsafe_u32(start, 1) & this->m_cap_mask;
        this->m_len = sub_unsafe_u32(this->m_len, 1);

        return QueuePopStatus::Ok;
    }
};

} // namespace zx
