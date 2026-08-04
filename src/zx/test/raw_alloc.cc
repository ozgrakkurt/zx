import zx;

constexpr zx::u64 MEMORY_SIZE = zx::u64{1} << 16;

static void do_test(zx::RawAlloc *alloc) {
  zx::i32** slice = nullptr;
  zx::raw_assert(zx::RawAllocStatus::Ok == alloc->alloc<zx::i32*>(100, &slice));
  zx::raw_assert(slice != nullptr);
  zx::raw_assert(__builtin_is_aligned(slice, alignof(zx::i32*)));

  for (zx::u64 i=0; i<100; i++) {
    zx::raw_assert(zx::RawAllocStatus::Ok == alloc->alloc<zx::i32>(1, &slice[i]));
    zx::raw_assert(slice[i] != nullptr);
    zx::raw_assert(__builtin_is_aligned(slice[i], alignof(zx::i32)));
    *(slice[i]) = (zx::i32)i;
  }

  for (zx::u64 i = 0; i < 100; i++) {
    zx::raw_assert(*(slice[i]) == (zx::i32)i);
  }

  zx::u256* zero_len = nullptr;
  zx::raw_assert(zx::RawAllocStatus::Ok == alloc->alloc<zx::u256>(0, &zero_len));
  zx::raw_assert(zero_len != nullptr);
  zx::raw_assert(__builtin_is_aligned(zero_len, alignof(zx::u256)));

  // not nullptr and also not a valid address
  zx::u8* const dangling = (zx::u8*)alignof(zx::u8);

  {
    zx::u8* big = dangling;
    zx::raw_assert(zx::RawAllocStatus::OutOfMemory == alloc->alloc<zx::u8>(MEMORY_SIZE, &big));
    zx::raw_assert(big == dangling); 
  }

  alloc->reset();

  {
    zx::u8* big = dangling;
    zx::raw_assert(zx::RawAllocStatus::Ok == alloc->alloc<zx::u8>(MEMORY_SIZE, &big));
    zx::raw_assert(big != dangling);
  }
  
  zx::u8* small = dangling;
  zx::raw_assert(zx::RawAllocStatus::OutOfMemory == alloc->alloc<zx::u8>(1, &small));
  zx::raw_assert(small == dangling);

  zx::u256* zero2 = nullptr;
  zx::raw_assert(zx::RawAllocStatus::Ok == alloc->alloc<zx::u256>(0, &zero2));
  zx::raw_assert(zero2 != nullptr);
  zx::raw_assert(__builtin_is_aligned(zero2, alignof(zx::u256)));
}

static void do_test_nocap() {
  zx::RawAlloc alloc = zx::RawAlloc::init((zx::u8*)alignof(zx::u8), 0);

  zx::u64* slice = nullptr;
  zx::RawAllocStatus const status = alloc.alloc<zx::u64>(0, &slice);
  zx::raw_assert(zx::RawAllocStatus::Ok == status);
  zx::raw_assert(slice != nullptr);
  zx::raw_assert(__builtin_is_aligned(slice, alignof(zx::u64)));
}

int main() {
  zx::u8* mem = nullptr;
  zx::raw_assert(zx::page_alloc::AllocStatus::Ok == zx::page_alloc::alloc_4kb(MEMORY_SIZE, &mem));

  do_test_nocap();

  zx::RawAlloc alloc = zx::RawAlloc::init(mem, MEMORY_SIZE);
  do_test(&alloc);
  
  zx::raw_assert(zx::page_alloc::FreeStatus::Ok == zx::page_alloc::free(mem, MEMORY_SIZE));
  
  return 0;
}
