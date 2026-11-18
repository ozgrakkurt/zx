export module zx:slice;
import :def;
import :assert;
import :arith;
import :backtrace;

export namespace zx {

enum class [[nodiscard]] SliceInitStatus : u8_c {
    Ok = 0,
    Nullptr,
    UnalignedPtr,
    IntegerOverflow,
};

enum class [[nodiscard]] SliceCastStatus : u8_c {
    Ok = 0,
    Nullptr,
    IntegerOverflow,
    AlignMismatch,
    Truncation,
};

enum class [[nodiscard]] SliceStatus : u8_c {
    Ok = 0,
    Nullptr,
    OutOfBounds,
    IntegerOverflow,
};

enum class Comparison : u8_c {
    Equal = 0,
    Greater,
    Less,
};

enum class [[nodiscard]] SliceMemcpyStatus : u8_c {
    Ok = 0,
    SizeMismatch,
    Overlapping,
};

enum class [[nodiscard]] SliceAlignedInitStatus : u8_c {
    Ok = 0,
    Nullptr,
    InvalidAlign,
};

template <typename T> struct Slice {
    u64 m_len;
    T const *m_ptr;

    static SliceInitStatus init(Backtrace *err_bt, T const *ptr, u64 len,
                                Slice *out) {
        if (out == nullptr || ptr == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::Nullptr;
        }

        if (!__builtin_is_aligned(ptr, alignof(T))) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::UnalignedPtr;
        }

        bool overflow = false;
        mul_u64(len, sizeof(T), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::IntegerOverflow;
        }

        *out = Slice{
            .m_ptr = ptr,
            .m_len = len,
        };
        return SliceInitStatus::Ok;
    }

    static Slice init_unsafe(T const *ptr, u64 len) {
        assert_dev(ptr != nullptr);
        assert_dev(__builtin_is_aligned(ptr, alignof(T)));

        bool overflow = false;
        mul_u64(len, sizeof(T), &overflow);
        assert_dev(!overflow);

        return Slice{
            .m_ptr = ptr,
            .m_len = len,
        };
    }

    template <typename U>
    static SliceInitStatus init_from_ptr(Backtrace *err_bt, U const *ptr,
                                         Slice *out) {
        static_assert(sizeof(U) % sizeof(T) == 0);

        if (ptr == nullptr || out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::Nullptr;
        }

        bool const is_t_aligned = __builtin_is_aligned(ptr, alignof(T));
        bool const is_u_aligned = __builtin_is_aligned(ptr, alignof(U));

        if (!is_t_aligned || !is_u_aligned) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::UnalignedPtr;
        }

        constexpr u64 len = sizeof(U) / sizeof(T);

        *out = Slice{
            .m_ptr = (T const *)ptr,
            .m_len = len,
        };

        return SliceInitStatus::Ok;
    }

    template <typename U> static Slice init_from_ptr_unsafe(U const *ptr) {
        static_assert(sizeof(U) % sizeof(T) == 0);

        assert_dev(ptr != nullptr);

        assert_dev(__builtin_is_aligned(ptr, alignof(T)));
        assert_dev(__builtin_is_aligned(ptr, alignof(U)));

        constexpr u64 len = sizeof(U) / sizeof(T);

        return Slice{
            .m_ptr = (T const *)ptr,
            .m_len = len,
        };
    }

    static Slice init_dangling() {
        return Slice{
            .m_ptr = (T const *)u64{alignof(T)},
            .m_len = 0,
        };
    }

    static SliceAlignedInitStatus aligned_init_dangling(Backtrace *err_bt,
                                                        u16 align, Slice *out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceAlignedInitStatus::Nullptr;
        }

        if (!is_pow_two_u16(align)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceAlignedInitStatus::InvalidAlign;
        }

        if (align > 128) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceAlignedInitStatus::InvalidAlign;
        }

        if (align < alignof(T)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceAlignedInitStatus::InvalidAlign;
        }

        *out = Slice{
            .m_ptr = (T const *)u64{align},
            .m_len = 0,
        };
        return SliceAlignedInitStatus::Ok;
    }

    static Slice aligned_init_dangling_unsafe(u16 align) {
        assert_dev(is_pow_two_u16(align));
        assert_dev(align <= 128);
        assert_dev(align >= alignof(T));

        return Slice{
            .m_ptr = (T const *)u64{align},
            .m_len = 0,
        };
    }

    u64 len() const { return this->m_len; }

    T const *ptr() const { return this->m_ptr; }

    template <typename U>
    SliceCastStatus cast_truncate(Backtrace *err_bt, Slice<U> *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::Nullptr;
        }

        bool overflow = false;
        u64 const byte_cap = mul_u64(this->m_len, sizeof(T), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }

        overflow = false;
        u64 const new_len = div_u64(byte_cap, sizeof(U), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }

        if (alignof(U) > alignof(T)) {
            if (!__builtin_is_aligned(this->m_ptr, alignof(U))) [[unlikely]] {
                Backtrace::collect(err_bt);
                return SliceCastStatus::AlignMismatch;
            }
        }

        *out = Slice<U>{
            .m_ptr = (U const *)this->m_ptr,
            .m_len = new_len,
        };
        return SliceCastStatus::Ok;
    }

    template <typename U>
    SliceCastStatus cast(Backtrace *err_bt, Slice<U> *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::Nullptr;
        }

        bool overflow = false;
        u64 const byte_cap = mul_u64(this->m_len, sizeof(T), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }

        overflow = false;
        u64 const new_len = div_u64(byte_cap, sizeof(U), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }

        overflow = false;
        u64 const rem = mod_u64(byte_cap, sizeof(U), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }
        if (rem != 0) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::Truncation;
        }

        if (alignof(U) > alignof(T)) {
            if (!__builtin_is_aligned(this->m_ptr, alignof(U))) [[unlikely]] {
                Backtrace::collect(err_bt);
                return SliceCastStatus::AlignMismatch;
            }
        }

        *out = Slice<U>{
            .m_ptr = (U const *)this->m_ptr,
            .m_len = new_len,
        };
        return SliceCastStatus::Ok;
    }

    template <typename U> Slice<U> cast_truncate_unsafe() const {
        u64 const byte_cap = mul_unsafe_u64(this->m_len, sizeof(T));
        u64 const new_len = div_unsafe_u64(byte_cap, sizeof(U));

        if (alignof(U) > alignof(T)) {
            assert_dev(__builtin_is_aligned(this->m_ptr, alignof(U)));
        }

        return Slice<U>{
            .m_ptr = (U const *)this->m_ptr,
            .m_len = new_len,
        };
    }

    template <typename U> Slice<U> cast_unsafe() const {
        u64 const byte_cap = mul_unsafe_u64(this->m_len, sizeof(T));
        u64 const new_len = div_unsafe_u64(byte_cap, sizeof(U));
        assert_dev(mod_unsafe_u64(byte_cap, sizeof(U)) == 0);

        if (alignof(U) > alignof(T)) {
            assert_dev(__builtin_is_aligned(this->m_ptr, alignof(U)));
        }

        return Slice<U>{
            .m_ptr = (U const *)this->m_ptr,
            .m_len = new_len,
        };
    }

    template <typename U> Slice<U> cast_static() const {
        static_assert(alignof(U) <= alignof(T));
        static_assert(sizeof(U) <= sizeof(T));
        static_assert(sizeof(T) % sizeof(U) == 0);

        u64 const new_len = this->m_len * sizeof(T) / sizeof(U);

        return Slice<U>{
            .m_ptr = (U const *)this->m_ptr,
            .m_len = new_len,
        };
    }

    SliceStatus slice(Backtrace *err_bt, u64 offset, u64 len,
                      Slice *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        bool overflow = false;
        u64 const end = add_u64(offset, len, &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::IntegerOverflow;
        }

        if (end > this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        *out = Slice{
            .m_ptr = &this->m_ptr[offset],
            .m_len = len,
        };

        return SliceStatus::Ok;
    }

    Slice slice_unsafe(u64 offset, u64 len) const {
        u64 const end = add_unsafe_u64(offset, len);
        assert_dev(end <= this->m_len);

        return Slice{
            .m_ptr = &this->m_ptr[offset],
            .m_len = len,
        };
    }

    SliceStatus slice_at(Backtrace *err_bt, u64 offset, Slice *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (offset > this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        u64 const len = sub_unsafe_u64(this->m_len, offset);

        *out = Slice{
            .m_ptr = &this->m_ptr[offset],
            .m_len = len,
        };
        return SliceStatus::Ok;
    }

    Slice slice_at_unsafe(u64 offset) const {
        assert_dev(offset <= this->m_len);

        u64 const len = sub_unsafe_u64(this->m_len, offset);

        return Slice{
            .m_ptr = &this->m_ptr[offset],
            .m_len = len,
        };
    }

    SliceStatus offset(Backtrace *err_bt, u64 offset, T const **out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (offset >= this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        *out = &this->m_ptr[offset];

        return SliceStatus::Ok;
    }

    T const *offset_unsafe(u64 offset) const {
        assert_dev(offset < this->m_len);
        return &this->m_ptr[offset];
    }

    SliceStatus read(Backtrace *err_bt, u64 offset, T *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (offset >= this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        *out = this->m_ptr[offset];

        return SliceStatus::Ok;
    }

    T read_unsafe(u64 offset) const {
        assert_dev(offset < this->m_len);
        return this->m_ptr[offset];
    }

    SliceStatus read_last(Backtrace *err_bt, T *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (this->m_len == 0) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        u64 const idx = this->m_len - 1;
        *out = this->m_ptr[idx];

        return SliceStatus::Ok;
    }

    T read_last_unsafe() const {
        assert_dev(this->m_len > 0);
        u64 const idx = this->m_len - 1;
        return this->m_ptr[idx];
    }

    template <typename U>
    SliceStatus read_unaligned(Backtrace *err_bt, u64 offset, U *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (offset >= this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        bool overflow = false;
        u64 const item_cap = sub_u64(this->m_len, offset, &overflow);
        u64 const byte_cap = mul_u64(item_cap, sizeof(T), &overflow);

        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::IntegerOverflow;
        }

        if (byte_cap < sizeof(U)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        __builtin_memcpy(out, &this->m_ptr[offset], sizeof(U));
        return SliceStatus::Ok;
    }

    template <typename U> U read_unaligned_unsafe(u64 offset) const {
#ifdef ZX_DEV
        assert(offset < this->m_len);
        u64 const item_cap = sub_unsafe_u64(this->m_len, offset);
        u64 const byte_cap = mul_unsafe_u64(item_cap, sizeof(T));
        assert(byte_cap >= sizeof(U));
#endif

        U val;
        __builtin_memcpy(&val, &this->m_ptr[offset], sizeof(U));
        return val;
    }

    bool is_overlapping(Slice other) const {
        u64 const self_start = (u64)this->m_ptr;
        u64 const other_start = (u64)other.m_ptr;

        // These additions are ok because the overflow condition is checked in
        // init method.
        u64 const self_end =
            add_unsafe_u64(self_start, mul_unsafe_u64(this->m_len, sizeof(T)));
        u64 const other_end =
            add_unsafe_u64(other_start, mul_unsafe_u64(other.m_len, sizeof(T)));

        return this->m_len > 0 && other.m_len > 0 && self_start < other_end &&
               other_start < self_end;
    }

    [[nodiscard]] Comparison compare(Slice other) const {
        u64 const len = min_u64(this->m_len, other.m_len);
        // This should be ok because overflow condition is checked in the init
        // method.
        u64 const byte_count = mul_unsafe_u64(len, sizeof(T));
        // We are assuming little endian here.
        i32 res = __builtin_memcmp((u8 const *)this->m_ptr,
                                   (u8 const *)other.m_ptr, byte_count);
        if (res == 0) {
            if (this->m_len > other.m_len) {
                return Comparison::Greater;
            } else if (this->m_len < other.m_len) {
                return Comparison::Less;
            } else {
                return Comparison::Equal;
            }
        } else if (res < 0) {
            return Comparison::Less;
        } else {
            return Comparison::Greater;
        }
    }

    [[nodiscard]] bool equals(Slice other) const {
        if (this->m_len != other.m_len) {
            return false;
        }
        return this->compare(other) == Comparison::Equal;
    }

    [[nodiscard]] u64 count(T elem) const {
        u64 cnt = 0;

        for (u64 i = 0; i < this->m_len; i++) {
            cnt += (this->m_ptr[i] == elem);
        }

        return cnt;
    }
};

template <typename T> struct SliceMut {
    u64 m_len;
    T *m_ptr;

    static SliceInitStatus init(Backtrace *err_bt, T *ptr, u64 len,
                                SliceMut *out) {
        if (out == nullptr || ptr == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::Nullptr;
        }

        if (!__builtin_is_aligned(ptr, alignof(T))) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::UnalignedPtr;
        }

        bool overflow = false;
        mul_u64(len, sizeof(T), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::IntegerOverflow;
        }

        *out = SliceMut{
            .m_ptr = ptr,
            .m_len = len,
        };
        return SliceInitStatus::Ok;
    }

    static SliceMut init_unsafe(T *ptr, u64 len) {
        assert_dev(ptr != nullptr);
        assert_dev(__builtin_is_aligned(ptr, alignof(T)));

        bool overflow = false;
        mul_u64(len, sizeof(T), &overflow);
        assert_dev(!overflow);

        return SliceMut{
            .m_ptr = ptr,
            .m_len = len,
        };
    }

    template <typename U>
    static SliceInitStatus init_from_ptr(Backtrace *err_bt, U *ptr,
                                         SliceMut *out) {
        static_assert(sizeof(U) % sizeof(T) == 0);

        if (ptr == nullptr || out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::Nullptr;
        }

        bool const is_t_aligned = __builtin_is_aligned(ptr, alignof(T));
        bool const is_u_aligned = __builtin_is_aligned(ptr, alignof(U));

        if (!is_t_aligned || !is_u_aligned) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::UnalignedPtr;
        }

        constexpr u64 len = sizeof(U) / sizeof(T);

        *out = SliceMut{
            .m_ptr = (T *)ptr,
            .m_len = len,
        };

        return SliceInitStatus::Ok;
    }

    template <typename U> static SliceMut init_from_ptr_unsafe(U *ptr) {
        static_assert(sizeof(U) % sizeof(T) == 0);

        assert_dev(ptr != nullptr);

        assert_dev(__builtin_is_aligned(ptr, alignof(T)));
        assert_dev(__builtin_is_aligned(ptr, alignof(U)));

        constexpr u64 len = sizeof(U) / sizeof(T);

        return SliceMut{
            .m_ptr = (T *)ptr,
            .m_len = len,
        };
    }

    static SliceMut init_dangling() {
        return SliceMut{
            .m_ptr = (T *)u64{alignof(T)},
            .m_len = 0,
        };
    }

    static SliceAlignedInitStatus
    aligned_init_dangling(Backtrace *err_bt, u16 align, SliceMut *out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceAlignedInitStatus::Nullptr;
        }

        if (!is_pow_two_u16(align)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceAlignedInitStatus::InvalidAlign;
        }

        if (align > 128) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceAlignedInitStatus::InvalidAlign;
        }

        if (align < alignof(T)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceAlignedInitStatus::InvalidAlign;
        }

        *out = SliceMut{
            .m_ptr = (T *)u64{align},
            .m_len = 0,
        };
        return SliceAlignedInitStatus::Ok;
    }

    static SliceMut aligned_init_dangling_unsafe(u16 align) {
        assert_dev(is_pow_two_u16(align));
        assert_dev(align <= 128);
        assert_dev(align >= alignof(T));

        return SliceMut{
            .m_ptr = (T *)u64{align},
            .m_len = 0,
        };
    }

    u64 len() const { return this->m_len; }

    T *ptr() const { return this->m_ptr; }

    template <typename U>
    SliceCastStatus cast_truncate(Backtrace *err_bt, SliceMut<U> *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::Nullptr;
        }

        bool overflow = false;
        u64 const byte_cap = mul_u64(this->m_len, sizeof(T), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }

        overflow = false;
        u64 const new_len = div_u64(byte_cap, sizeof(U), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }

        if (alignof(U) > alignof(T)) {
            if (!__builtin_is_aligned(this->m_ptr, alignof(U))) [[unlikely]] {
                Backtrace::collect(err_bt);
                return SliceCastStatus::AlignMismatch;
            }
        }

        *out = SliceMut<U>{
            .m_ptr = (U *)this->m_ptr,
            .m_len = new_len,
        };
        return SliceCastStatus::Ok;
    }

    template <typename U>
    SliceCastStatus cast(Backtrace *err_bt, SliceMut<U> *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::Nullptr;
        }

        bool overflow = false;
        u64 const byte_cap = mul_u64(this->m_len, sizeof(T), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }

        overflow = false;
        u64 const new_len = div_u64(byte_cap, sizeof(U), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }

        overflow = false;
        u64 const rem = mod_u64(byte_cap, sizeof(U), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::IntegerOverflow;
        }
        if (rem != 0) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceCastStatus::Truncation;
        }

        if (alignof(U) > alignof(T)) {
            if (!__builtin_is_aligned(this->m_ptr, alignof(U))) [[unlikely]] {
                Backtrace::collect(err_bt);
                return SliceCastStatus::AlignMismatch;
            }
        }

        *out = SliceMut<U>{
            .m_ptr = (U *)this->m_ptr,
            .m_len = new_len,
        };
        return SliceCastStatus::Ok;
    }

    template <typename U> SliceMut<U> cast_truncate_unsafe() const {
        u64 const byte_cap = mul_unsafe_u64(this->m_len, sizeof(T));
        u64 const new_len = div_unsafe_u64(byte_cap, sizeof(U));

        if (alignof(U) > alignof(T)) {
            assert_dev(__builtin_is_aligned(this->m_ptr, alignof(U)));
        }

        return SliceMut<U>{
            .m_ptr = (U *)this->m_ptr,
            .m_len = new_len,
        };
    }

    template <typename U> SliceMut<U> cast_unsafe() const {
        u64 const byte_cap = mul_unsafe_u64(this->m_len, sizeof(T));
        u64 const new_len = div_unsafe_u64(byte_cap, sizeof(U));
        assert_dev(mod_unsafe_u64(byte_cap, sizeof(U)) == 0);

        if (alignof(U) > alignof(T)) {
            assert_dev(__builtin_is_aligned(this->m_ptr, alignof(U)));
        }

        return SliceMut<U>{
            .m_ptr = (U *)this->m_ptr,
            .m_len = new_len,
        };
    }

    template <typename U> SliceMut<U> cast_static() const {
        static_assert(alignof(U) <= alignof(T));
        static_assert(sizeof(U) <= sizeof(T));
        static_assert(sizeof(T) % sizeof(U) == 0);

        u64 const new_len = this->m_len * sizeof(T) / sizeof(U);

        return SliceMut<U>{
            .m_ptr = (U *)this->m_ptr,
            .m_len = new_len,
        };
    }

    SliceStatus slice(Backtrace *err_bt, u64 offset, u64 len,
                      SliceMut *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        bool overflow = false;
        u64 const end = add_u64(offset, len, &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::IntegerOverflow;
        }

        if (end > this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        *out = SliceMut{
            .m_ptr = &this->m_ptr[offset],
            .m_len = len,
        };

        return SliceStatus::Ok;
    }

    SliceMut slice_unsafe(u64 offset, u64 len) const {
        u64 const end = add_unsafe_u64(offset, len);
        assert_dev(end <= this->m_len);

        return SliceMut{
            .m_ptr = &this->m_ptr[offset],
            .m_len = len,
        };
    }

    SliceStatus slice_at(Backtrace *err_bt, u64 offset, SliceMut *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (offset > this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        u64 const len = sub_unsafe_u64(this->m_len, offset);

        *out = SliceMut{
            .m_ptr = &this->m_ptr[offset],
            .m_len = len,
        };
        return SliceStatus::Ok;
    }

    SliceMut slice_at_unsafe(u64 offset) const {
        assert_dev(offset <= this->m_len);

        u64 const len = sub_unsafe_u64(this->m_len, offset);

        return SliceMut{
            .m_ptr = &this->m_ptr[offset],
            .m_len = len,
        };
    }

    SliceStatus offset(Backtrace *err_bt, u64 offset, T const **out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (offset >= this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        *out = &this->m_ptr[offset];

        return SliceStatus::Ok;
    }

    T const *offset_unsafe(u64 offset) const {
        assert_dev(offset < this->m_len);
        return &this->m_ptr[offset];
    }

    SliceStatus offset_mut(Backtrace *err_bt, u64 offset, T **out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (offset >= this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        *out = &this->m_ptr[offset];

        return SliceStatus::Ok;
    }

    T *offset_mut_unsafe(u64 offset) const {
        assert_dev(offset < this->m_len);
        return &this->m_ptr[offset];
    }

    SliceStatus read(Backtrace *err_bt, u64 offset, T *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (offset >= this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        *out = this->m_ptr[offset];

        return SliceStatus::Ok;
    }

    T read_unsafe(u64 offset) const {
        assert_dev(offset < this->m_len);
        return this->m_ptr[offset];
    }

    SliceStatus read_last(Backtrace *err_bt, T *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (this->m_len == 0) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        u64 const idx = this->m_len - 1;
        *out = this->m_ptr[idx];

        return SliceStatus::Ok;
    }

    T read_last_unsafe() const {
        assert_dev(this->m_len > 0);
        u64 const idx = this->m_len - 1;
        return this->m_ptr[idx];
    }

    template <typename U>
    SliceStatus read_unaligned(Backtrace *err_bt, u64 offset, U *out) const {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::Nullptr;
        }

        if (offset >= this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        bool overflow = false;
        u64 const item_cap = sub_u64(this->m_len, offset, &overflow);
        u64 const byte_cap = mul_u64(item_cap, sizeof(T), &overflow);

        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::IntegerOverflow;
        }

        if (byte_cap < sizeof(U)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        __builtin_memcpy(out, &this->m_ptr[offset], sizeof(U));
        return SliceStatus::Ok;
    }

    template <typename U> U read_unaligned_unsafe(u64 offset) const {
#ifdef ZX_DEV
        assert(offset < this->m_len);
        u64 const item_cap = sub_unsafe_u64(this->m_len, offset);
        u64 const byte_cap = mul_unsafe_u64(item_cap, sizeof(T));
        assert(byte_cap >= sizeof(U));
#endif

        U val;
        __builtin_memcpy(&val, &this->m_ptr[offset], sizeof(U));
        return val;
    }

    bool is_overlapping(SliceMut other) const {
        u64 const self_start = (u64)this->m_ptr;
        u64 const other_start = (u64)other.m_ptr;

        // These additions are ok because the overflow condition is checked in
        // init method.
        u64 const self_end =
            add_unsafe_u64(self_start, mul_unsafe_u64(this->m_len, sizeof(T)));
        u64 const other_end =
            add_unsafe_u64(other_start, mul_unsafe_u64(other.m_len, sizeof(T)));

        return this->m_len > 0 && other.m_len > 0 && self_start < other_end &&
               other_start < self_end;
    }

    [[nodiscard]] Comparison compare(SliceMut other) const {
        u64 const len = min_u64(this->m_len, other.m_len);
        // This should be ok because overflow condition is checked in the init
        // method.
        u64 const byte_count = mul_unsafe_u64(len, sizeof(T));
        // We are assuming little endian here.
        i32 res = __builtin_memcmp((u8 const *)this->m_ptr,
                                   (u8 const *)other.m_ptr, byte_count);
        if (res == 0) {
            if (this->m_len > other.m_len) {
                return Comparison::Greater;
            } else if (this->m_len < other.m_len) {
                return Comparison::Less;
            } else {
                return Comparison::Equal;
            }
        } else if (res < 0) {
            return Comparison::Less;
        } else {
            return Comparison::Greater;
        }
    }

    [[nodiscard]] bool equals(SliceMut other) const {
        if (this->m_len != other.m_len) {
            return false;
        }
        return this->compare(other) == Comparison::Equal;
    }

    [[nodiscard]] u64 count(T elem) const {
        u64 cnt = 0;

        for (u64 i = 0; i < this->m_len; i++) {
            cnt += (this->m_ptr[i] == elem);
        }

        return cnt;
    }

    Slice<T> as_const() const {
        return Slice<T>{
            .m_ptr = this->m_ptr,
            .m_len = this->m_len,
        };
    }

    SliceStatus write(Backtrace *err_bt, u64 offset, T val) const {
        if (offset >= this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        this->m_ptr[offset] = val;

        return SliceStatus::Ok;
    }

    void write_unsafe(u64 offset, T val) const {
        assert_dev(offset < this->m_len);
        this->m_ptr[offset] = val;
    }

    template <typename U>
    SliceStatus write_unaligned(Backtrace *err_bt, u64 offset, U val) const {
        if (offset >= this->m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        bool overflow = false;
        u64 const item_cap = sub_u64(this->m_len, offset, &overflow);
        u64 const byte_cap = mul_u64(item_cap, sizeof(T), &overflow);

        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::IntegerOverflow;
        }

        if (byte_cap < sizeof(U)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceStatus::OutOfBounds;
        }

        __builtin_memcpy(&this->m_ptr[offset], &val, sizeof(U));

        return SliceStatus::Ok;
    }

    template <typename U> void write_unaligned_unsafe(u64 offset, U val) const {
#ifdef ZX_DEV
        assert(offset < this->m_len);
        u64 const item_cap = sub_unsafe_u64(this->m_len, offset);
        u64 const byte_cap = mul_unsafe_u64(item_cap, sizeof(T));
        assert(byte_cap >= sizeof(U));
#endif

        __builtin_memcpy(&this->m_ptr[offset], &val, sizeof(U));
    }

    void fill(T val) const {
        u64 const len = this->m_len;
        T *const ptr = this->m_ptr;
        for (u64 i = 0; i < len; i++) {
            ptr[i] = val;
        }
    }

    void memset(u8 byte) const {
        // safe since we check overflow in init func
        u64 const n_bytes = mul_unsafe_u64(this->m_len, sizeof(T));
        __builtin_memset(this->m_ptr, byte, n_bytes);
    }

    SliceMemcpyStatus memcpy_from(Backtrace *err_bt, Slice<T> other) const {
        if (this->m_len != other.m_len) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceMemcpyStatus::SizeMismatch;
        }

        if (this->as_const().is_overlapping(other)) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceMemcpyStatus::Overlapping;
        }

        // this is safe to do because overflow condition is checked in the init
        // method.
        u64 const n_bytes = mul_unsafe_u64(this->m_len, sizeof(T));

        __builtin_memcpy(this->m_ptr, other.m_ptr, n_bytes);
        return SliceMemcpyStatus::Ok;
    }

    void memcpy_from_unsafe(Slice<T> other) const {
        assert_dev(this->m_len == other.m_len);
        assert_dev(!this->as_const().is_overlapping(other));

        // this is safe to do because overflow condition is checked in the init
        // method.
        u64 const n_bytes = mul_unsafe_u64(this->m_len, sizeof(T));

        __builtin_memcpy(this->m_ptr, other.m_ptr, n_bytes);
    }
};

struct CString {
    Slice<u8> m_slice;

    static SliceInitStatus init(Backtrace *err_bt, char const *cstr,
                                CString *out) {
        if (out == nullptr || cstr == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return SliceInitStatus::Nullptr;
        }

        u64 const len = __builtin_strlen(cstr);
        Slice<u8> slice;
        SliceInitStatus const init_status =
            Slice<u8>::init(err_bt, (u8 const *)cstr, len, &slice);
        if (init_status != SliceInitStatus::Ok) {
            return init_status;
        }

        *out = CString{
            .m_slice = slice,
        };

        return SliceInitStatus::Ok;
    }

    static CString init_unsafe(char const *cstr) {
        assert_dev(cstr != nullptr);

        u64 const len = __builtin_strlen(cstr);

        return CString{
            .m_slice = Slice<u8>::init_unsafe((u8 const *)cstr, len),
        };
    }

    Slice<u8> as_slice() const { return this->m_slice; }

    char const *as_cstr() const { return (char const *)this->m_slice.ptr(); }
};

} // namespace zx
