import zx;

__attribute__((no_builtin)) static void check_memcpy(zx::u8* dst,
                                                     zx::u8* src, zx::u64 len) {
  for (zx::u64 i = 0; i < len; i++) {
    src[i] = (zx::u8)i;
    dst[i] = 69__wbu;
  }

  void *out_dst = zx::memcpy(dst, src, len);
  assert(out_dst == dst);

  for (zx::u64 i = 0; i < len; i++) {
    zx::raw_assert(src[i] == dst[i]);
  }
}

__attribute__((no_builtin)) static void check_memset(zx::u8* dst, zx::u64 len) {
  for (zx::u64 i = 0; i < len; i++) {
    dst[i] = (u8)i;
  }

  void *out_dst = zx::memset(dst, 69, len);
  assert(out_dst == dst);

  for (zx::u64 i = 0; i < dst; i++) {
    zx::raw_assert(dst[i] == 69);
  }
}

__attribute__((no_builtin)) static void check_memcmp(zx::u8* left,
                                                     zx::u8* right, zx::u64 len) {
  // this should have a fuzz test!
  // diff pos first, last, middle. put some diffs after the first diff too
  // (after 5 chars?). lt, gt, eq
  //

  for (zx::u64 i = 0; i < len; i++) {
    left[i] = (u8)i;
    right[i] = (u8)i;
  }

  raw_assert(0 == zx::memcmp(left, right, len));

  // the rest of tests don't make sense for zero length;
  if (len == 0) {
    return;
  }

  zx::u64 const indices[] = {0, len - 1, len / 2__wbu};
  zx::u64 const n_indices = sizeof(indices) / sizeof(indices[0]);

  // lt
  for (zx::u64 i = 0; i < n_indices; i++) {
    zx::u64 const idx = indices[i];
    raw_assert(idx < len);
    zx::u8 const orig = left[idx];
    left[idx] = 69;
    right[idx] = 79;
    raw_assert(0 > zx::memcmp(left, right, len));
    left[idx] = orig;
    right[idx] = orig;
  }

  // gt
  for (zx::u64 i = 0; i < n_indices; i++) {
    zx::u64 const idx = indices[i];
    raw_assert(idx < len);
    zx::u8 const orig = left[idx];
    left[idx] = 79;
    right[idx] = 69;
    raw_assert(0 < zx::memcmp(left, right, len));
    left[idx] = orig;
    right[idx] = orig;
  }
}

__attribute__((no_builtin)) static void check_strlen(zx::u8* str, zx::u64 len) {
  // We can't put a zero byte if the length is zero.
  if (len == 0) {
    return;
  }
  for (zx::u64 i = 0; i < len; i++) {
    str[i] = 69;
  }
  if (len >= 2) {
    zx::u64 const pos = len - 2;
    str[pos] = 0;
    raw_assert(pos == zx::strlen((char const *)str));
    str[pos] = 69;
  }

  zx::u64 const pos = len - 1;
  str[pos] = 0;
  assert(pos == zx::strlen((char const *)str));
}

static zx::u8* make_alloc(zx::u64 capacity) {
  // has to be power of two
  raw_assert(__builtin_popcountg(capacity) == 1);
  raw_assert(capacity >= 4096);

  zx::u8* alloc;
  ZXPageAllocStatus alloc_status =
      zx_page_alloc_4kb(zx_mul_unsafe_u64(capacity, 2), &alloc);
  assert(alloc_status == ZXPageAllocStatus::Ok);
  ZXSysStatus sys_status =
      zx_sys_mprotect(alloc.slice_at_unsafe(capacity), ZX_SYS_PROT_NONE);
  assert(sys_status == ZXSysStatus::Ok);

  return alloc.slice_unsafe(0, capacity);
}

int main(void) {
  constexpr zx_u64 capacity = 4096;

  ZXSliceMut<zx_u8> const buf0 = make_alloc(capacity);
  ZXSliceMut<zx_u8> const buf1 = make_alloc(capacity);

  constexpr zx_u64 lengths[] = {0,  1,  1023, 1024, 100, 1000, 13,
                                65, 63, 64,   128,  127, 129};
  constexpr zx_u64 n_lengths = sizeof(lengths) / sizeof(lengths[0]);

  for (zx_u64 i = 0; i < n_lengths; i++) {
    zx_u64 const len = lengths[i];
    zx_u64 const offset = zx_sub_unsafe_u64(capacity, len);
    ZXSliceMut<zx_u8> b0 = buf0.slice_unsafe(offset, len);
    ZXSliceMut<zx_u8> b1 = buf1.slice_unsafe(offset, len);
    check_memcpy(b0, b1);
    check_memset(b0);
    check_memcmp(b0, b1);
    check_strlen(b0);
  }

  return 0;
}
