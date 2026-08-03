export module zx:page_alloc;
import :sys;
import :def;
import :raw_assert;

#ifndef __linux__
#error "expected linux"
#endif

#ifndef __x86_64__
#error "expected x86_64"
#endif

export namespace zx {

enum class [[nodiscard]] AllocStatus : u8_c {
  Ok = 0,
  Nullptr,
  UnexpectedErr,
  InvalidSize,
  PermissionErr,
  NoMem,
  OsReturnedNullptr,
  InvalidInput,
};

AllocStatus alloc_(u64 size, u64 page_size, i32 huge_page_flags, u8** out) {
  if (out == nullptr) {
    return AllocStatus::Nullptr;
  }

  raw_assert(__builtin_popcountg(page_size) == 1);

  if (__builtin_popcountg(size) != 1 || size < page_size) {
    return AllocStatus::InvalidSize;
  }

  i32 const prot = sys::PROT_READ | sys::PROT_WRITE;
  i32 const flags = sys::MAP_PRIVATE | sys::MAP_ANONYMOUS |
                       sys::MAP_POPULATE | huge_page_flags;

  u8 *mem = nullptr;
  if (sys::MMapStatus const status = sys::mmap(nullptr, size, prot, flags, -1, 0, &mem);
      sys::MMapStatus::Ok != status) {
    switch (status) {
    case sys::MMapStatus::EPerm: {
      return AllocStatus::PermissionErr;
    }
    case sys::MMapStatus::ENoMem: {
      return AllocStatus::NoMem;    
    }
    case sys::MMapStatus::EInval: {
      return AllocStatus::InvalidInput;
    }
    default: {
      return AllocStatus::UnexpectedErr;
    }
    }
  }

  if (mem == nullptr) {
    return AllocStatus::OsReturnedNullptr;
  }

  *out = mem;
  return AllocStatus::Ok;
}

AllocStatus alloc_4kb(u64 size, u8** out) {
  return alloc_(size, u64{1} << 12, 0, out);
}

AllocStatus alloc_huge_2mb(u64 size, u8** out) {
  return alloc_(size, u64{1} << 21, sys::MAP_HUGETLB | sys::MAP_HUGE_2MB, out);
}

AllocStatus alloc_huge_1gb(u64 size, u8** out) {
  return alloc_(size, u64{1} << 30, sys::MAP_HUGETLB | sys::MAP_HUGE_1GB, out);
}

enum class [[nodiscard]] FreeStatus : u8_c {
  Ok = 0,
  Nullptr,
  InvalidSize,
  InvalidInput,
  UnexpectedErr,
};

FreeStatus free(u8* mem, u64 size) {
  if (mem == nullptr) {
    return FreeStatus::Nullptr;
  }

  if (__builtin_popcountg(size) != 1) {
    return FreeStatus::InvalidSize;
  }

  sys::MMapStatus const status = sys::munmap(mem, size);
  if (sys::MMapStatus::Ok != status) {
    switch (status) {
    case sys::MMapStatus::EInval: {
      return FreeStatus::InvalidInput;
    }
    default: {
      return FreeStatus::UnexpectedErr;
    }
    }
  }

  return FreeStatus::Ok;
}

} // export namespace zx
