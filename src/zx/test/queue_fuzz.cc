import zx;

using namespace zx;

struct Ctx {
    Alloc *alloc;
    Backtrace *err_bt;
};

static void fuzz_one(void *usr_ctx, FuzzInput *fuzz_input) {
    assert(usr_ctx != nullptr);
    Ctx ctx = *(Ctx *)usr_ctx;

    Alloc *alloc = ctx.alloc;
    alloc->reset();

    Backtrace *err_bt = ctx.err_bt;

    constexpr u32 max_capacity = 128;
    u32 const capacity =
        u64_to_u32_unsafe(mod_unsafe_u64(fuzz_input->next(), max_capacity));

    u32 const max_n_ops = mul_unsafe_u32(max_capacity, 4);
    u32 const n_ops =
        u64_to_u32_unsafe(mod_unsafe_u64(fuzz_input->next(), max_n_ops));

    Queue<u32> queue;
    testing::assert_with_trace(
        QueueInitStatus::Ok ==
            Queue<u32>::init(err_bt, alloc, capacity, &queue),
        err_bt);

    u32 start = 0;
    u32 end = 0;

    for (u32 i = 0; i < n_ops; i++) {
        u8 const op = u64_to_u8_unsafe(mod_unsafe_u64(fuzz_input->next(), 2));
        switch (op) {
        // push
        case 0: {
            QueuePushStatus const status = queue.push(nullptr, &end);
            if (status == QueuePushStatus::Ok) {
                end = add_unsafe_u32(end, 1);
            } else if (status != QueuePushStatus::OutOfCapacity) {
                panic();
            }
            break;
        }
        // pop
        case 1: {
            u32 val = U32_MAX;
            QueuePopStatus const status = queue.pop(nullptr, &val);
            if (status == QueuePopStatus::Ok) {
                assert(val == start);
                start = add_unsafe_u32(start, 1);
            } else if (status != QueuePopStatus::Empty) {
                panic();
            }

            break;
        }
        default: {
            panic();
        }
        }
    }

    u32 val = U32_MAX;
    while (queue.pop(nullptr, &val) == QueuePopStatus::Ok) {
        assert(val == start);
        start = add_unsafe_u32(start, 1);
    }

    assert(start == end);
}

Alloc make_alloc(Backtrace *err_bt, u64 size) {
    u8 *mem = nullptr;
    testing::assert_with_trace(page_alloc::AllocStatus::Ok ==
                                   page_alloc::alloc_4kb(err_bt, size, &mem),
                               err_bt);

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, size));

    return alloc;
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    constexpr u64 MEM_CAP = u64{1} << 20;
    Alloc alloc = make_alloc(&err_bt, MEM_CAP);

    Alloc fuzz_alloc = make_alloc(&err_bt, FUZZ_MEM_CAP);

    Ctx ctx = Ctx{
        .alloc = &alloc,
        .err_bt = &err_bt,
    };
    constexpr u64 n_iter = u64{1} << 12;
    fuzz(FuzzParams{.alloc = &fuzz_alloc,
                    .usr_ctx = (void *)&ctx,
                    .fuzz_one = fuzz_one,
                    .iter_limit = n_iter});
    return 0;
}
