export module zx:raw_alloc;
import :def;
import :raw_assert;

export namespace zx {

enum class [[nodiscard]] RawAllocStatus : u8_c {
  Ok = 0,
  Fail,
  OutOfMemory,
};

struct RawAlloc {
  u8* m_mem;
  u64 m_capacity;
  u64 m_offset;

  static RawAlloc init(u8* mem, u64 capacity) {
#ifdef ZX_DEV
    __builtin_memset(mem, 0xaa, capacity);
#endif
    raw_assert(mem != nullptr);
    return RawAlloc {
      .m_mem = mem,
      .m_capacity = capacity,
      .m_offset = 0,
    };
  }

  u64 capacity_left() const {
    raw_assert_dev(this->m_offset <= this->m_capacity);
    return this->m_capacity - this->m_offset;
  }

  void reset() {
#ifdef ZX_DEV
    __builtin_memset(this->m_mem, 0xaa, this->m_capacity);
#endif
    this->m_offset = 0;
  }

  template <typename T>
  RawAllocStatus aligned_alloc(u16 align, u64 len, T** out) {
    bool const invalid_input = (align < alignof(T)) |
                               (align > 128) |
                               (__builtin_popcountg(align) != 1) |
                               (out == nullptr);

    if (invalid_input) {
      return RawAllocStatus::Fail;
    }

    if (len == 0) {
      *out = (T*)alignof(T);
      return RawAllocStatus::Ok;
    }

    u8* const mem = this->m_mem;
    u64 const offset = this->m_offset;

    static_assert(sizeof(void*) == sizeof(u64));

    bool overflow = false;
    
    u64 mem_pos;
    overflow |= __builtin_add_overflow((u64)mem, offset, &mem_pos);
    u64 const mem_aligned = __builtin_align_up(mem_pos, align);
    u64 padding;
    overflow |= __builtin_sub_overflow(mem_aligned, mem_pos, &padding);

    u64 size;
    overflow |= __builtin_mul_overflow(len, sizeof(T), &size);
    u64 alloc_offset;
    overflow |= __builtin_add_overflow(offset, padding, &alloc_offset);
    u64 new_offset;
    overflow |= __builtin_add_overflow(alloc_offset, size, &new_offset);

    if (overflow) {
      return RawAllocStatus::Fail;
    }

    if (new_offset > this->m_capacity) {
      return RawAllocStatus::OutOfMemory;
    }

    u8* out_mem = &mem[alloc_offset];

#ifdef ZX_DEV
    __builtin_memset(out_mem, 0xaa, size);
#endif

    this->m_offset = new_offset;
    *out = (T*)out_mem;
    return RawAllocStatus::Ok;
  }

  template <typename T>
  RawAllocStatus alloc(u64 len, T** out) {
    return this->aligned_alloc(alignof(T), len, out);
  }
};

} // export namespace zx
