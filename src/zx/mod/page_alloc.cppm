export module zx:page_alloc;
import :sys;
import :def;
import :assert;
import :cpu_context;

#ifndef __linux__
#error "expected linux"
#endif

#ifndef __x86_64__
#error "expected x86_64"
#endif

export namespace zx {

namespace page_alloc {

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

AllocStatus alloc_(CPUContext *err_ctx, u64 size, u64 page_size,
                   i32 huge_page_flags, u8 **out) {
    if (out == nullptr) [[unlikely]] {
        zx_getcpucontext_(err_ctx);
        return AllocStatus::Nullptr;
    }

    assert(__builtin_popcountg(page_size) == 1);

    if (__builtin_popcountg(size) != 1 || size < page_size) [[unlikely]] {
        zx_getcpucontext_(err_ctx);
        return AllocStatus::InvalidSize;
    }

    i32 const prot = sys::PROT_READ | sys::PROT_WRITE;
    i32 const flags = sys::MAP_PRIVATE | sys::MAP_ANONYMOUS |
                      sys::MAP_POPULATE | huge_page_flags;

    u8 *mem = nullptr;
    if (sys::MMapStatus const status =
            sys::mmap(err_ctx, nullptr, size, prot, flags, -1, 0, &mem);
        sys::MMapStatus::Ok != status) [[unlikely]] {
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

    if (mem == nullptr) [[unlikely]] {
        zx_getcpucontext_(err_ctx);
        return AllocStatus::OsReturnedNullptr;
    }

    *out = mem;
    return AllocStatus::Ok;
}

AllocStatus alloc_4kb(CPUContext *err_ctx, u64 size, u8 **out) {
    return alloc_(err_ctx, size, u64{1} << 12, 0, out);
}

AllocStatus alloc_huge_2mb(CPUContext *err_ctx, u64 size, u8 **out) {
    return alloc_(err_ctx, size, u64{1} << 21,
                  sys::MAP_HUGETLB | sys::MAP_HUGE_2MB, out);
}

AllocStatus alloc_huge_1gb(CPUContext *err_ctx, u64 size, u8 **out) {
    return alloc_(err_ctx, size, u64{1} << 30,
                  sys::MAP_HUGETLB | sys::MAP_HUGE_1GB, out);
}

enum class [[nodiscard]] FreeStatus : u8_c {
    Ok = 0,
    Nullptr,
    InvalidSize,
    InvalidInput,
    UnexpectedErr,
};

FreeStatus free(CPUContext *err_ctx, u8 *mem, u64 size) {
    if (mem == nullptr) [[unlikely]] {
        zx_getcpucontext_(err_ctx);
        return FreeStatus::Nullptr;
    }

    if (__builtin_popcountg(size) != 1) [[unlikely]] {
        zx_getcpucontext_(err_ctx);
        return FreeStatus::InvalidSize;
    }

    sys::MMapStatus const status = sys::munmap(err_ctx, mem, size);
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

} // namespace page_alloc

} // namespace zx
