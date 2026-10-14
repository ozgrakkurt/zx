import zx;

using namespace zx;

typedef page_alloc::AllocStatus allocfn(Backtrace *err_bt, u64 size, u8 **out);

static void do_test(allocfn alloc, u64 size) {
    Backtrace err_bt = Backtrace::init();

    u8 *mem = nullptr;
    testing::assert_with_trace(
        page_alloc::AllocStatus::Ok == alloc(&err_bt, size, &mem), &err_bt);
    assert(mem != nullptr);

    assert(size > 0);

    u8 const first = mem[0];
    u8 const last = mem[size - 1];

    mem[size / 2] = first + last;

    testing::assert_with_trace(page_alloc::FreeStatus::Ok ==
                                   page_alloc::free(&err_bt, mem, size),
                               &err_bt);

    // munmaping an already unmapped region doesn't fail
    testing::assert_with_trace(page_alloc::FreeStatus::Ok ==
                                   page_alloc::free(&err_bt, mem, size),
                               &err_bt);
}

int main(void) {
    do_test(page_alloc::alloc_4kb, u64{1} << 16);
    // do_test(page_alloc::alloc_huge_2mb, u64{1} << 21);
    // do_test(page_alloc::alloc_huge_1gb, u64{1} << 30);

    return 0;
}
