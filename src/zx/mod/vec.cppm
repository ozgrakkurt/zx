export module zx:vec;
import :def;

export namespace zx {

typedef u8_c v128_u8 __attribute__((vector_size(16)));
typedef u16_c v128_u16 __attribute__((vector_size(16)));
typedef u32_c v128_u32 __attribute__((vector_size(16)));
typedef u64_c v128_u64 __attribute__((vector_size(16)));
typedef i8_c v128_i8 __attribute__((vector_size(16)));
typedef i16_c v128_i16 __attribute__((vector_size(16)));
typedef i32_c v128_i32 __attribute__((vector_size(16)));
typedef i64_c v128_i64 __attribute__((vector_size(16)));
typedef f32 v128_f32 __attribute__((vector_size(16)));
typedef f64 v128_f64 __attribute__((vector_size(16)));

typedef u8_c v256_u8 __attribute__((vector_size(32)));
typedef u16_c v256_u16 __attribute__((vector_size(32)));
typedef u32_c v256_u32 __attribute__((vector_size(32)));
typedef u64_c v256_u64 __attribute__((vector_size(32)));
typedef i8_c v256_i8 __attribute__((vector_size(32)));
typedef i16_c v256_i16 __attribute__((vector_size(32)));
typedef i32_c v256_i32 __attribute__((vector_size(32)));
typedef i64_c v256_i64 __attribute__((vector_size(32)));
typedef f32 v256_f32 __attribute__((vector_size(32)));
typedef f64 v256_f64 __attribute__((vector_size(32)));

typedef u8_c v512_u8 __attribute__((vector_size(64)));
typedef u16_c v512_u16 __attribute__((vector_size(64)));
typedef u32_c v512_u32 __attribute__((vector_size(64)));
typedef u64_c v512_u64 __attribute__((vector_size(64)));
typedef i8_c v512_i8 __attribute__((vector_size(64)));
typedef i16_c v512_i16 __attribute__((vector_size(64)));
typedef i32_c v512_i32 __attribute__((vector_size(64)));
typedef i64_c v512_i64 __attribute__((vector_size(64)));
typedef f32 v512_f32 __attribute__((vector_size(64)));
typedef f64 v512_f64 __attribute__((vector_size(64)));

} // namespace zx
