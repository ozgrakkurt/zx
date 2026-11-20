export module zx:hashtable;
import :def;
import :slice;
import :vec;
import :arith;
import :alloc;
import :assert;
import :backtrace;

export namespace zx {

enum class [[nodiscard]] HashtableInitStatus : u8_c {
    Ok = 0,
    Nullptr,
    OutOfMemory,
    AllocFail,
    IntegerOverflow,
    UnexpectedErr,
};

enum class [[nodiscard]] HashtableInsertStatus : u8_c {
    CreatedNewEntry = 0,
    ModifiedExistingEntry,
    OutOfCapacity,
    Nullptr,
};

enum class [[nodiscard]] HashtableGetOrInsertStatus : u8_c {
    CreatedNewEntry = 0,
    FoundExistingEntry,
    OutOfCapacity,
    Nullptr,
};

enum class [[nodiscard]] HashtableGetStatus : u8_c {
    Found = 0,
    NotFound,
    Nullptr,
};

template <typename T, typename EqCtx,
          bool EqFn(EqCtx ctx, T const *a, T const *b)>
struct Hashtable {
    SliceMut<T> m_entries;
    SliceMut<u8> m_metadata;
    u32 m_cap_mask;
    u32 m_capacity_left;
    u32 m_capacity;

    static constexpr u8 GROUP_SIZE_ = sizeof(v128_u8);
    static constexpr u8 EMPTY_TAG_ = 0b11111111;
    static constexpr u32 LOAD_FACTOR_ = 80;

    static HashtableInitStatus init(Backtrace *err_bt, Alloc *alloc,
                                    u32 capacity, Hashtable *table) {
        if (alloc == nullptr || table == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return HashtableInitStatus::Nullptr;
        }

        bool overflow = false;
        u32 cap =
            next_pow_two_u32(div_ceil_u32(mul_u32(capacity, 100, &overflow),
                                          LOAD_FACTOR_, &overflow),
                             &overflow);
        cap = max_u32(GROUP_SIZE_, cap);

        u32 const cap_mask = sub_u32(cap, 1, &overflow);

        // allocate extra sizeof(simd) bytes so we can do simd loads on any byte
        // index also have to mirror the first group with the extra group at the
        // end to have simple read path.
        u64 const metadata_size = add_u64(cap, GROUP_SIZE_, &overflow);

        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return HashtableInitStatus::IntegerOverflow;
        }

        if (cap <= capacity || cap == 0 || !is_pow_two_u32(cap)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return HashtableInitStatus::UnexpectedErr;
        }

        SliceMut<u8> metadata;
        switch (alloc->alloc<u8>(err_bt, metadata_size, &metadata)) {
        case AllocStatus::Ok: {
            break;
        }
        case AllocStatus::OutOfMemory: {
            return HashtableInitStatus::OutOfMemory;
            break;
        }
        default: {
            return HashtableInitStatus::AllocFail;
            break;
        }
        }
        metadata.fill(EMPTY_TAG_);

        SliceMut<T> entries;
        switch (alloc->alloc<T>(err_bt, cap, &entries)) {
        case AllocStatus::Ok: {
            break;
        }
        case AllocStatus::OutOfMemory: {
            return HashtableInitStatus::OutOfMemory;
            break;
        }
        default: {
            return HashtableInitStatus::AllocFail;
            break;
        }
        }

        *table = Hashtable{
            .m_entries = entries,
            .m_metadata = metadata,
            .m_cap_mask = cap_mask,
            .m_capacity_left = capacity,
            .m_capacity = capacity,
        };

        return HashtableInitStatus::Ok;
    }

    u32 n_occupied() const {
        return sub_unsafe_u32(this->m_capacity, this->m_capacity_left);
    }

    u32 capacity() const { return this->m_capacity; }

    u32 capacity_left() const { return this->m_capacity_left; }

    void reset() {
#ifdef ZX_DEV
        this->m_entries.memset(0xaa);
#endif
        this->m_metadata.fill(EMPTY_TAG_);
        this->m_capacity_left = this->m_capacity;
    }

    static u32 maybe_mirror_index_(u32 cap_mask, u32 index) {
        return wrapping_add_u32(
            (wrapping_sub_u32(index, GROUP_SIZE_) & cap_mask), GROUP_SIZE_);
    }

    static u16 tag_match_(v128_u8 group, v128_u8 tag_vec) {
        return (u16)__builtin_ia32_pmovmskb128(group == tag_vec);
    }

    static u16 empty_match_(v128_u8 group) {
        return (u16)__builtin_ia32_pmovmskb128(group);
    }

    static v128_u8 broadcast_(u8 tag) {
        return {tag, tag, tag, tag, tag, tag, tag, tag,
                tag, tag, tag, tag, tag, tag, tag, tag};
    }

    static u16 remove_last_set_bit_(u16 v) {
        return v & wrapping_sub_u16(v, 1);
    }

    HashtableInsertStatus insert(u64 hash, T const *entry, EqCtx eqctx) {
        if (entry == nullptr) [[unlikely]] {
            return HashtableInsertStatus::Nullptr;
        }

        u8 const tag = (u8)(hash >> 57);
        v128_u8 const tag_vec = broadcast_(tag);

        u32 stride = 0;

        u32 const cap_mask = this->m_cap_mask;

        u32 meta_idx = cap_mask & ((u32)(hash));
        while (true) {
            v128_u8 const group =
                this->m_metadata.read_unaligned_unsafe<v128_u8>(meta_idx);

            u16 tag_match = tag_match_(group, tag_vec);
            while (tag_match != 0) {
                u8 const tag_pos = ctz_u16(tag_match);

                u32 const index = add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

                T *entry_p = this->m_entries.offset_mut_unsafe(index);
                if (EqFn(eqctx, entry_p, entry)) [[likely]] {
                    *entry_p = *entry;
                    return HashtableInsertStatus::ModifiedExistingEntry;
                } else {
                    tag_match = remove_last_set_bit_(tag_match);
                }
            }

            u16 const empty_match = empty_match_(group);
            if (empty_match != 0) [[likely]] {
                u8 const tag_pos = ctz_u16(empty_match);

                u32 const index = add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

                u32 const maybe_mirror_idx =
                    maybe_mirror_index_(cap_mask, index);
                u32 const cap_left = this->m_capacity_left;
                if (cap_left == 0) {
                    return HashtableInsertStatus::OutOfCapacity;
                }
                SliceMut<u8> metadata = this->m_metadata;

                metadata.write_unsafe(index, tag);
                metadata.write_unsafe(maybe_mirror_idx, tag);
                this->m_capacity_left = sub_unsafe_u32(cap_left, 1);
                this->m_entries.write_unsafe(index, *entry);

                return HashtableInsertStatus::CreatedNewEntry;
            }

            // Should have found an empty slot by now
            assert_dev(stride <= cap_mask);

            stride = add_unsafe_u32(stride, GROUP_SIZE_);
            meta_idx = add_unsafe_u32(meta_idx, stride) & cap_mask;
        }
    }

    HashtableGetOrInsertStatus get_or_insert(u64 hash, T const *entry,
                                             EqCtx eqctx, T const **out) {
        if (out == nullptr || entry == nullptr) [[unlikely]] {
            return HashtableGetOrInsertStatus::Nullptr;
        }

        u8 const tag = (u8)(hash >> 57);
        u32 const cap_mask = this->m_cap_mask;
        v128_u8 const tag_vec = broadcast_(tag);
        u32 stride = 0;

        u32 meta_idx = cap_mask & ((u32)(hash));

        while (true) {
            v128_u8 const group =
                this->m_metadata.read_unaligned_unsafe<v128_u8>(meta_idx);

            u16 tag_match = tag_match_(group, tag_vec);
            while (tag_match != 0) {
                u8 const tag_pos = ctz_u16(tag_match);

                u32 const index = add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

                T *entry_p = this->m_entries.offset_mut_unsafe(index);
                if (EqFn(eqctx, entry_p, entry)) [[likely]] {
                    *out = entry_p;
                    return HashtableGetOrInsertStatus::FoundExistingEntry;
                } else {
                    tag_match = remove_last_set_bit_(tag_match);
                }
            }

            u16 const empty_match = empty_match_(group);
            if (empty_match != 0) [[likely]] {
                u8 const tag_pos = ctz_u16(empty_match);
                u32 const index = add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

                u32 const maybe_mirror_idx =
                    maybe_mirror_index_(cap_mask, index);
                u32 const cap_left = this->m_capacity_left;
                if (cap_left == 0) {
                    *out = nullptr;
                    return HashtableGetOrInsertStatus::OutOfCapacity;
                }
                SliceMut<u8> metadata = this->m_metadata;
                T *entry_p = this->m_entries.offset_mut_unsafe(index);

                *entry_p = *entry;
                metadata.write_unsafe(index, tag);
                metadata.write_unsafe(maybe_mirror_idx, tag);
                this->m_capacity_left = sub_unsafe_u32(cap_left, 1);

                *out = entry_p;
                return HashtableGetOrInsertStatus::CreatedNewEntry;
            }

            // Should have found an empty slot by now
            assert_dev(stride <= cap_mask);

            stride = add_unsafe_u32(stride, GROUP_SIZE_);
            meta_idx = add_unsafe_u32(meta_idx, stride) & cap_mask;
        }
    }

    HashtableGetStatus get(u64 const hash, T const *entry, EqCtx eqctx,
                           T const **out) const {
        if (out == nullptr || entry == nullptr) [[unlikely]] {
            return HashtableGetStatus::Nullptr;
        }

        u8 const tag = (u8)(hash >> 57);
        u32 const cap_mask = this->m_cap_mask;
        v128_u8 const tag_vec = broadcast_(tag);
        u32 stride = 0;

        u32 meta_idx = cap_mask & ((u32)(hash));

        while (true) {
            v128_u8 const group =
                this->m_metadata.read_unaligned_unsafe<v128_u8>(meta_idx);

            u16 tag_match = tag_match_(group, tag_vec);
            while (tag_match != 0) {
                u8 const tag_pos = ctz_u16(tag_match);

                u32 const index = add_unsafe_u32(meta_idx, tag_pos) & cap_mask;

                T const *entry_p = this->m_entries.offset_unsafe(index);
                if (EqFn(eqctx, entry_p, entry)) [[likely]] {
                    *out = entry_p;
                    return HashtableGetStatus::Found;
                } else {
                    tag_match = remove_last_set_bit_(tag_match);
                }
            }

            u16 const empty_match = empty_match_(group);
            if (empty_match != 0) [[likely]] {
                *out = nullptr;
                return HashtableGetStatus::NotFound;
            }

            // Should have found an empty slot by now
            assert_dev(stride <= cap_mask);

            stride = add_unsafe_u32(stride, GROUP_SIZE_);
            meta_idx = add_unsafe_u32(meta_idx, stride) & cap_mask;
        }
    }
}; // struct Hashtable

} // namespace zx
