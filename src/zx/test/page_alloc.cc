import zx;

typedef zx::page_alloc::AllocStatus allocfn(zx::CPUContext *err_ctx,
                                            zx::u64 size, zx::u8 **out);

static void do_test(allocfn alloc, zx::u64 size) {
    zx::CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    zx::u8 *mem = nullptr;
    zx::testing::assert_with_trace(zx::page_alloc::AllocStatus::Ok ==
                                       alloc(&err_ctx, size, &mem),
                                   &err_ctx);
    zx::assert(mem != nullptr);

    zx::assert(size > 0);

    zx::u8 const first = mem[0];
    zx::u8 const last = mem[size - 1];

    mem[size / 2] = first + last;

    zx::testing::assert_with_trace(
        zx::page_alloc::FreeStatus::Ok ==
            zx::page_alloc::free(&err_ctx, mem, size),
        &err_ctx);

    // munmaping an already unmapped region doesn't fail
    zx::testing::assert_with_trace(
        zx::page_alloc::FreeStatus::Ok ==
            zx::page_alloc::free(&err_ctx, mem, size),
        &err_ctx);
}

int main(void) {
    do_test(zx::page_alloc::alloc_4kb, zx::u64{1} << 16);
    // do_test(zx::page_alloc::alloc_huge_2mb, zx::u64{1} << 21);
    // do_test(zx::page_alloc::alloc_huge_1gb, zx::u64{1} << 30);

    return 0;
}
