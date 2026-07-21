export module zx:alloc;
import :assert;
import :def;
import :arith;

export namespace zx {

enum class [[nodiscard]] AllocStatus : u8_c {
  Ok = 0,
  Fail,
  OutOfMemory,
};

struct Alloc {
  u8* m_mem;
  u64 m_cap;
  u64 m_offset;

  static Alloc init(u8* mem, u64 capacity) {
#ifdef ZX_DEV
    __builtin_memset(mem, 0, capacity);
#endif

    assert_dev(mem != nullptr);

    return Alloc {
      .m_mem = mem,
      .m_cap = capacity,
      .m_offset = 0,
    };
  }

  u64 capacity_left() const {
    return sub_unsafe_u64(this->m_cap, this->m_offset);
  }

  void reset() {
#ifdef ZX_DEV
    __builtin_memset(this->m_mem, 0, this->m_cap);
#endif
    this->m_offset = 0;
  }  
}
  
} // export namespace zx
