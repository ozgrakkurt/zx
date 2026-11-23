import compression;
import zx;

namespace noz = compression::noz;

struct Ctx {
    zx::Backtrace *err_bt;
    zx::SliceMut<zx::u8> input;
    zx::SliceMut<zx::u8> output;
    zx::SliceMut<zx::u8> compressed;
    zx::SliceMut<zx::u8> compressed_copy;
};

static void fuzz_one(void *usr_ctx, zx::FuzzInput *fuzz_input) {
    zx::assert(usr_ctx != nullptr);
    zx::assert(fuzz_input != nullptr);

    Ctx const *ctx = (Ctx const *)usr_ctx;

    zx::Backtrace *err_bt = ctx->err_bt;

    zx::u64 const capacity = ctx->input.len();
    zx::assert(capacity == ctx->output.len());
    zx::u64 const len = zx::mod_unsafe_u64(fuzz_input->next(), capacity);

    zx::u64 const offset = zx::sub_unsafe_u64(capacity, len);
    zx::SliceMut<zx::u8> const input = ctx->input.slice_unsafe(offset, len);
    zx::SliceMut<zx::u8> const output = ctx->output.slice_unsafe(offset, len);
    zx::SliceMut<zx::u8> const compressed = ctx->compressed;
    zx::SliceMut<zx::u8> const compressed_copy = ctx->compressed_copy;

    zx::Xoshiro256 rand = zx::Xoshiro256::init(fuzz_input->next());

    zx::u64 const data_kind = fuzz_input->next() % 3;
    switch (data_kind) {
    case 0: {
        rand.fill(input);
        break;
    }
    case 1: {
        input.memset(0);
        break;
    }
    case 2: {
        input.memset(0xaa);
        break;
    }
    default: {
        zx::panic();
        break;
    }
    }

    zx::u64 bound = zx::U64_MAX;
    zx::testing::assert_with_trace(
        noz::CompressBoundStatus::Ok ==
            noz::compress_bound(err_bt, input.len(), &bound),
        err_bt);
    zx::assert(bound <= compressed.len());

    compressed.memset(0xaa);
    zx::u64 n_written = zx::U64_MAX;
    zx::testing::assert_with_trace(
        noz::CompressStatus::Ok ==
            noz::compress(err_bt, input.as_const(), compressed, &n_written),
        err_bt);
    zx::assert(n_written <= bound);

    zx::SliceMut<zx::u8> copy = compressed_copy.slice_unsafe(
        zx::sub_unsafe_u64(compressed_copy.len(), n_written), n_written);
    copy.memcpy_from_unsafe(compressed.as_const().slice_unsafe(0, n_written));

    zx::u64 n_read = zx::U64_MAX;
    zx::testing::assert_with_trace(
        noz::DecompressStatus::Ok ==
            noz::decompress(err_bt, copy.as_const(), output, &n_read),
        err_bt);
    zx::assert(n_read == n_written);
    zx::assert(n_read == copy.len());
    zx::assert(input.as_const().memcmp_unsafe(output.as_const()) == 0);
}

zx::Alloc make_alloc(zx::Backtrace *err_bt, zx::u64 size) {
    zx::u8 *mem = nullptr;
    zx::testing::assert_with_trace(
        zx::page_alloc::AllocStatus::Ok ==
            zx::page_alloc::alloc_4kb(err_bt, size, &mem),
        err_bt);

    zx::Alloc alloc =
        zx::Alloc::init(zx::SliceMut<zx::u8>::init_unsafe(mem, size));

    return alloc;
}

static zx::SliceMut<zx::u8> make_guarded_alloc(zx::Backtrace *err_bt,
                                               zx::u64 capacity) {
    zx::assert(zx::is_pow_two_u64(capacity));
    zx::assert(capacity >= 4096);

    zx::u64 const mem_size = zx::mul_unsafe_u64(capacity, 2);
    zx::u8 *mem = nullptr;
    zx::page_alloc::AllocStatus const alloc_status =
        zx::page_alloc::alloc_4kb(err_bt, mem_size, &mem);
    zx::testing::assert_with_trace(
        alloc_status == zx::page_alloc::AllocStatus::Ok, err_bt);

    zx::sys::MProtectStatus sys_status =
        zx::sys::mprotect(err_bt, &mem[capacity], capacity, zx::sys::PROT_NONE);
    zx::testing::assert_with_trace(sys_status == zx::sys::MProtectStatus::Ok,
                                   err_bt);

    zx::SliceMut<zx::u8> ret = zx::SliceMut<zx::u8>::init_unsafe(mem, capacity);
    ret.memset(0);

    return ret;
}

int main(void) {
    zx::Backtrace err_bt = zx::Backtrace::init();

    zx::Alloc alloc = make_alloc(&err_bt, zx::FUZZ_MEM_CAP);

    Ctx const ctx = Ctx{
        .input = make_guarded_alloc(&err_bt, 4096),
        .output = make_guarded_alloc(&err_bt, 4096),
        .compressed = make_guarded_alloc(&err_bt, 8192),
        .compressed_copy = make_guarded_alloc(&err_bt, 8192),
        .err_bt = &err_bt,
    };

    zx::fuzz(zx::FuzzParams{
        .alloc = &alloc,
        .usr_ctx = (void *)&ctx,
        .fuzz_one = fuzz_one,
    });

    return 0;
}
