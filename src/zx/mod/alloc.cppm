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

  
} // export namespace zx
