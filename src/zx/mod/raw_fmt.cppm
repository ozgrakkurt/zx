export module zx:raw_fmt;
import :def;
import :raw_assert;
import :sys;

export namespace zx {

struct RawFmt {
  u8* m_mem;
  u64 m_capacity;
  u64 m_offset;

  static RawFmt init(u8* mem, u64 capacity) {
    return RawFmt {
      .m_mem = mem,
      .m_capacity = capacity,
      .m_offset = 0,
    };
  }

  u8 const* mem() const {
    return this->m_mem; 
  }

  u64 n_written() const {
    raw_assert_dev(this->m_capacity >= this->m_offset);
    return this->m_offset;
  }

  u64 capacity() const {
    raw_assert_dev(this->m_capacity >= this->m_offset);
    return this->m_capacity;
  }

  u64 capacity_left() const {
    raw_assert_dev(this->m_capacity >= this->m_offset);
    return this->m_capacity - this->m_offset;
  }

  void reset() {
#ifdef ZX_DEV
    __builtin_memset(this->m_mem, 0xaa, this->m_capacity);
#endif
    this->m_offset = 0;
  }

  RawFmt* push_str(u8 const* str, u64 len, bool* overflow) {
    u8* const mem = this->m_mem;
    u64 const offset = this->m_offset;
    u64 const capacity_left = this->capacity_left();
    u64 const n_write = (capacity_left > len) ? len : capacity_left;
    *overflow |= (n_write < len);
    if (n_write > 0) {
      __builtin_memcpy(&mem[offset], str, n_write);
    }
    this->m_offset = offset + n_write;
    return this;
  }

  RawFmt* push_char(u8 ch, bool* overflow) {
    u8* const mem = this->m_mem;
    u64 const offset = this->m_offset;
    bool const capacity_left = this->capacity_left() > 0;
    *overflow |= !capacity_left;
    if (capacity_left) {
      mem[offset] = ch;
      this->m_offset = offset + 1;
    }
    return this;
  }

  RawFmt* push_u64(u64 val, bool* overflow) {
    constexpr u64 MAX_NUM_DIGITS = 20;
    u8 digits[MAX_NUM_DIGITS];

    u64 pos = MAX_NUM_DIGITS;
    do {
      raw_assert_dev(pos > 0);
      pos--;
      digits[pos] = (u8)'0' + (u8)(val % 10);
      val /= 10;
    } while (val > 0);
    u64 const len = MAX_NUM_DIGITS -  pos;
    this->push_str(&digits[pos], len, overflow);
    return this;
  }

  RawFmt* push_i64(i64 val, bool* overflow) {
    if (val >= 0) {
      this->push_u64((u64)val, overflow);
    } else {
      this->push_char((u8)'-', overflow);
      this->push_u64((u64)-val, overflow);
    }

    return this;
  }

  RawFmt* push_u64_hex(u64 val, bool* overflow) {
    constexpr u64 MAX_NUM_DIGITS = 22;
    u8 digits[MAX_NUM_DIGITS];

    constexpr u8 TABLE[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                 '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    u64 pos = MAX_NUM_DIGITS;
    do {
      raw_assert_dev(pos > 0);
      pos--;
      digits[pos] = TABLE[val % 16];
      val /= 16;
    } while (val > 0);

    raw_assert_dev(pos > 0);
    pos--;
    digits[pos] = 'x';
    raw_assert_dev(pos > 0);
    pos--;
    digits[pos] = '0';

    raw_assert_dev(pos <= MAX_NUM_DIGITS);
    u64 const len = MAX_NUM_DIGITS - pos;
    this->push_str(&digits[pos], len, overflow);
    return this;
  }

  enum class [[nodiscard]] PrintStatus : u8_c {
    Ok = 0,
    ShortWrite,
    UnexpectedErr,
  };

  PrintStatus print_(i32 fd) const {
    if (this->m_offset == 0) {
      return PrintStatus::Ok;
    }

    u64 n_written = 0;
    if (sys::WriteStatus::Ok != sys::write(fd, this->m_mem, this->m_offset, &n_written)) {
      return PrintStatus::UnexpectedErr;
    }
    if (n_written < this->m_offset) {
      return PrintStatus::ShortWrite;
    }
    return PrintStatus::Ok;
  }

  PrintStatus print() const {
    return this->print_(sys::STDOUT);
  }

  PrintStatus eprint() const {    
    return this->print_(sys::STDERR);
  }
};

} // export namespace zx
