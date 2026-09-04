export module zx:slice;

import :def;
import :assert;
import :arith;

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
  SizeMismatch,
  AlignMismatch,
};

enum class [[nodiscard]] SliceSliceStatus : u8_c {
  Ok = 0,
  Nullptr,
  OutOfBounds,
  IntegerOverflow,
};

template <typename T>
struct Slice {
  u64 m_len;
  T const* m_ptr;

  static SliceInitStatus init(T const* ptr, u64 len, Slice* out)  {
    if (out == nullptr || ptr == nullptr) {
      return SliceInitStatus::Nullptr;
    }

    if (__builtin_is_aligned(ptr, alignof(T))) {
      return SliceInitStatus::UnalignedPtr;
    }

    bool overflow = false;
    mul_u64(len, sizeof(T), &overflow);
    if (overflow) {
      return SliceInitStatus::IntegerOverflow;
    }

    *out = Slice {
      .m_ptr = ptr,
      .m_len = len,
    };
    return SliceInitStatus::Ok;
  }

  static Slice init_unsafe(T const* ptr, u64 len) {
    assert_dev(ptr != nullptr);
    assert_dev(__builtin_is_aligned(ptr, alignof(T)));

    bool overflow = false;
    mul_u64(len, sizeof(T), &overflow);
    assert_dev(!overflow);

    return Slice {
      .m_ptr = ptr,
      .m_len = len,
    };
  }

  template <typename U>
  static SliceInitStatus init_from_ptr(U const* ptr, Slice* out) {
    static_assert(sizeof(U) % sizeof(T) == 0);

    if (ptr == nullptr || out == nullptr) {
      return SliceInitStatus::Nullptr;
    }

    bool const is_t_aligned = __builtin_is_aligned(ptr, alignof(T));
    bool const is_u_aligned = __builtin_is_aligned(ptr, alignof(U));

    if (!is_t_aligned || !is_u_aligned) {
      return SliceInitStatus::UnalignedPtr;
    }

    constexpr u64 len = sizeof(U) / sizeof(T);

    *out = Slice {
      .m_ptr = (T const*)ptr,
      .m_len = len,
    };

    return SliceInitStatus::Ok;
  }

  template <typename U>
  static Slice init_from_ptr_unsafe(U const* ptr) {
    static_assert(sizeof(U) % sizeof(T) == 0);

    assert_dev(ptr != nullptr);
    assert_dev(out != nullptr);

    assert_dev(__builtin_is_aligned(ptr, alignof(T)));
    assert_dev(__builtin_is_aligned(ptr, alignof(U)));

    constexpr u64 len = sizeof(U) / sizeof(T);

    *out = Slice {
      .m_ptr = (T const*)ptr,
      .m_len = len,
    };

    return SliceInitStatus::Ok;
  }

  static Slice init_dangling() {
    return Slice {
      .m_ptr = (T const*)u64{alignof(T)},
      .m_len = 0,
    };
  }

  template <u16 ALIGN>
  static Slice aligned_init_dangling() {
    static_assert(is_pow_two(ALIGN));
    static_assert(ALIGN >= alignof(T));

    return Slice {
      .m_ptr = (T const*)u64{ALIGN},
      .m_len = 0,
    };
  }

  u64 len() const {
    return this->m_len;
  }

  T const* ptr() const {
    return this->m_ptr;
  }

  template <typename U>
  SliceCastStatus cast(Slice<U>* out) const {
    if (out == nullptr) {
      return SliceCastStatus::Nullptr;
    }

    bool overflow = false;
    u64 const byte_cap = mul_u64(this->m_len, sizeof(T), &overflow);
    if (overflow) {
      return SliceCastStatus::IntegerOverflow;
    }

    if (byte_cap % sizeof(U) != 0) {
      return SliceCastStatus::SizeMismatch;
    }

    overflow = 0;
    u64 const new_len = div_u64(byte_cap, sizeof(U), &overflow);
    if (overflow) {
      return SliceCastStatus::IntegerOverflow;
    }

    if (alignof(U) > alignof(T)) {
      if (__builtin_is_aligned(this->m_ptr, alignof(U))) {
        return SliceCastStatus::AlignMismatch;
      }
    }

    *out = Slice<U> {
      .m_ptr = (U const*)this->m_ptr,
      .m_len = new_len,
    };
    return SLiceCastStatus::Ok;
  }

  template <typename U>
  Slice<U> cast_unsafe() const {
    u64 const byte_cap = mul_unsafe_u64(this->m_len, sizeof(T));
    assert_dev(byte_cap % sizeof(U) == 0);
    u64 const new_len = div_unsafe_u64(byte_cap, sizeof(U));

    if (alignof(U) > alignof(T)) {
      assert_dev(__builtin_is_aligned(this->m_ptr, alignof(U)));
    }

    *out = Slice<U> {
      .m_ptr = (U const*)this->m_ptr,
      .m_len = new_len,
    };
    return SliceCastStatus::Ok;
  }

  template <typename U>
  Slice<U> cast_static() const {
    static_assert(alignof(U) <= alignof(T));
    static_assert(sizeof(U) <= sizeof(T));
    static_assert(sizeof(T) % sizeof(U) == 0);

    u64 const new_len = this->m_len * sizeof(T) / sizeof(U);

    return Slice<U> {
      .m_ptr = (U const*)this->m_ptr,
      .m_len = new_len,
    };
  }

  SliceSliceStatus slice(u64 offset, u64 len, Slice* out) const {
    if (out == nullptr) {
      return SliceSliceStatus::Nullptr;
    }

    bool overflow = false;    
    u64 const end = add_u64(offset, len, &overflow);
    if (overflow) {
      return SliceSliceStatus::IntegerOverflow;
    }

    if (end > this->m_len) {
      return SliceSliceStatus::OutOfBounds;
    }

    *out = Slice {
      .ptr = &this->m_ptr[offset],
      .m_len = len,
    };
    
    return SliceSliceStatus::Ok;
  }

  Slice slice_unsafe(u64 offset, u64 len) const {
    u64 const end = add_unsafe_u64(offset, len);
    assert_dev(end <= this->m_len);

    return Slice {
      .ptr = &this->m_ptr[offset],
      .m_len = len,
    };
  }

  SliceSliceStatus slice_at(u64 offset, Slice* out) const {
    if (out == nullptr) {
      return SliceSliceStatus::Nullptr;
    }

    if (offset > this->m_len) {
      return SliceSliceStatus::OutOfBounds;
    }

    u64 const len = sub_unsafe_u64(this->m_len, offset);

    *out = Slice {
      .m_ptr = &this->m_ptr[offset],
      .m_len = len,
    };
    return SliceSliceStatus::Ok;
  }

  Slice slice_at_unsafe(u64 offset) const {
    assert_dev(offset <= this->m_len);

    return Slice {
    
    };
  }
};

} // export namespace zx
