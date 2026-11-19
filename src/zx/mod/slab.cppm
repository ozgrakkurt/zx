export module zx:slab;
import :def;
import :arith;
import :slice;
import :alloc;
import :assert;
import :backtrace;

export namespace zx {

enum class [[nodiscard]] SlabInitStatus : u8_c {
    Ok = 0,
    Nullptr,
    OutOfMemory,
    AllocFail,
};

enum class [[nodiscard]] SlabInsertStatus : u8_c {
    Ok = 0,
    OutOfCapacity,
    Nullptr,
};

enum class [[nodiscard]] SlabAccessStatus : u8_c {
    Found = 0,
    NotFound,
    Nullptr,
    OutOfBounds,
};

template <typename T> struct Slab {
    struct Entry_ {
        T val_;
        u32 next_free_;
        bool is_occupied_;
    };

    SliceMut<Entry_> m_entries;
    u32 m_first_free_entry;
    u32 m_n_occupied;
    u32 m_capacity;

    static SlabInitStatus init(Backtrace *err_bt, Alloc *alloc, u32 capacity,
                               Slab *slab) {
        if (alloc == nullptr || slab == nullptr) {
            Backtrace::collect(err_bt);
            return SlabInitStatus::Nullptr;
        }

        SliceMut<Entry_> entries;
        switch (alloc->alloc<Entry_>(err_bt, capacity, &entries)) {
        case AllocStatus::Ok: {
            break;
        }
        case AllocStatus::OutOfMemory: {
            return SlabInitStatus::OutOfMemory;
            break;
        }
        default: {
            return SlabInitStatus::AllocFail;
            break;
        }
        }

        for (u32 i = 0; i < capacity; i++) {
            Entry_ *ptr = entries.offset_mut_unsafe(i);
#ifdef ZX_DEV
            __builtin_memset(&ptr->val_, 0xaa, sizeof(T));
#endif
            ptr->next_free_ = add_unsafe_u32(i, 1);
            ptr->is_occupied_ = false;
        }

        *slab = Slab{
            .m_entries = entries,
            .m_first_free_entry = 0,
            .m_n_occupied = 0,
            .m_capacity = capacity,
        };
        return SlabInitStatus::Ok;
    }

    void reset() {
        SliceMut<Entry_> const entries = this->m_entries;
        u32 const capacity = this->m_capacity;
        for (u32 i = 0; i < capacity; i++) {
            Entry_ *ptr = entries.offset_mut_unsafe(i);
#ifdef ZX_DEV
            __builtin_memset(&ptr->val_, 0xaa, sizeof(T));
#endif
            ptr->next_free_ = add_unsafe_u32(i, 1);
            ptr->is_occupied_ = false;
        }
        this->m_n_occupied = 0;
        this->m_first_free_entry = 0;
    }

    u32 capacity() const { return this->m_capacity; }

    u32 n_occupied() const { return this->m_n_occupied; }

    u32 capacity_left() const {
        return sub_unsafe_u32(this->m_capacity, this->m_n_occupied);
    }

    SlabInsertStatus insert(Backtrace *err_bt, T const *val, u32 *idx) {
        if (val == nullptr || idx == nullptr) {
            Backtrace::collect(err_bt);
            return SlabInsertStatus::Nullptr;
        }

        u32 const key_idx = this->m_first_free_entry;

        if (key_idx == this->m_capacity) {
            Backtrace::collect(err_bt);
            *idx = U32_MAX;
            return SlabInsertStatus::OutOfCapacity;
        }
        assert_dev(this->m_capacity > this->m_n_occupied);
        assert_dev(key_idx < this->m_entries.len());

        Entry_ *entry = this->m_entries.offset_mut_unsafe(key_idx);
        assert_dev(!entry->is_occupied_);
        entry->is_occupied_ = true;
        entry->val_ = *val;
        this->m_first_free_entry = entry->next_free_;
        entry->next_free_ = this->m_capacity;

        this->m_n_occupied = add_unsafe_u32(this->m_n_occupied, 1);

        *idx = key_idx;
        return SlabInsertStatus::Ok;
    }

    SlabAccessStatus remove(Backtrace *err_bt, u32 key, T *out) {
        if (out == nullptr) {
            Backtrace::collect(err_bt);
            return SlabAccessStatus::Nullptr;
        }

        u32 const key_idx = key;

        if (key_idx >= this->m_capacity) {
            Backtrace::collect(err_bt);
#ifdef ZX_DEV
            __builtin_memset(out, 0xaa, sizeof(T));
#endif
            return SlabAccessStatus::OutOfBounds;
        }

        Entry_ *entry = this->m_entries.offset_mut_unsafe(key_idx);
        if (!entry->is_occupied_) {
            Backtrace::collect(err_bt);
#ifdef ZX_DEV
            __builtin_memset(out, 0xaa, sizeof(T));
#endif
            return SlabAccessStatus::NotFound;
        }
        entry->next_free_ = this->m_first_free_entry;
        entry->is_occupied_ = false;
        this->m_first_free_entry = key_idx;
        *out = entry->val_;
#ifdef ZX_DEV
        __builtin_memset(&entry->val_, 0xaa, sizeof(T));
#endif
        this->m_n_occupied = sub_unsafe_u32(this->m_n_occupied, 1);

        return SlabAccessStatus::Found;
    }

    SlabAccessStatus get_mut(Backtrace *err_bt, u32 key, T **out) {
        if (out == nullptr) {
            Backtrace::collect(err_bt);
            return SlabAccessStatus::Nullptr;
        }

        u32 const key_idx = key;

        if (key_idx >= this->m_capacity) {
            Backtrace::collect(err_bt);
            *out = nullptr;
            return SlabAccessStatus::OutOfBounds;
        }

        Entry_ *entry = this->m_entries.offset_mut_unsafe(key_idx);
        if (!entry->is_occupied_) {
            Backtrace::collect(err_bt);
            *out = nullptr;
            return SlabAccessStatus::NotFound;
        }

        *out = &entry->val_;
        return SlabAccessStatus::Found;
    }

    SlabAccessStatus get(Backtrace *err_bt, u32 key, T const **out) const {
        if (out == nullptr) {
            Backtrace::collect(err_bt);
            return SlabAccessStatus::Nullptr;
        }

        u32 const key_idx = key;

        if (key_idx >= this->m_capacity) {
            Backtrace::collect(err_bt);
            *out = nullptr;
            return SlabAccessStatus::OutOfBounds;
        }

        Entry_ const *entry = this->m_entries.offset_unsafe(key_idx);
        if (!entry->is_occupied_) {
            Backtrace::collect(err_bt);
            *out = nullptr;
            return SlabAccessStatus::NotFound;
        }

        *out = &entry->val_;
        return SlabAccessStatus::Found;
    }
};

} // namespace zx
