export module compression:noz;
import zx;

export namespace compression {

namespace noz {

enum class [[nodiscard]] CompressBoundStatus : zx::u8_c {
    Ok = 0,
    Nullptr,
    IntegerOverflow,
};

CompressBoundStatus compress_bound(zx::Backtrace *err_bt, zx::u64 input_size,
                                   zx::u64 *bound) {
    if (bound == nullptr) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressBoundStatus::Nullptr;
    }

    bool overflow = false;

    zx::u64 const n_chunks = zx::div_ceil_u64(input_size, 64, &overflow);

    zx::u64 const header_size =
        zx::mul_u64(n_chunks, sizeof(zx::u64), &overflow);

    zx::u64 const data_size = zx::mul_u64(n_chunks, 64, &overflow);

    *bound = zx::add_u64(header_size, data_size, &overflow);

    if (overflow) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressBoundStatus::IntegerOverflow;
    }

    return CompressBoundStatus::Ok;
}

enum class [[nodiscard]] CompressStatus : zx::u8_c {
    Ok = 0,
    Nullptr,
    InputOutputOverlap,
    BoundFail,
    OutputTooShort,
};

CompressStatus compress(zx::Backtrace *err_bt, zx::Slice<zx::u8> input,
                        zx::SliceMut<zx::u8> output, zx::u64 *n_written) {
    if (n_written == nullptr) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressStatus::Nullptr;
    }

    if (input.is_overlapping(output.as_const())) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressStatus::InputOutputOverlap;
    }

    zx::u64 bound = zx::U64_MAX;
    if (CompressBoundStatus::Ok != compress_bound(err_bt, input.len(), &bound))
        [[unlikely]] {
        return CompressStatus::BoundFail;
    }

    if (output.len() < bound) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return CompressStatus::OutputTooShort;
    }

    if (input.len() == 0) {
        *n_written = 0;
        return CompressStatus::Ok;
    }

    zx::v512_u8 const zeros = {};

    zx::u64 in_offset = 0;
    zx::u64 out_offset = 0;
    for (; zx::add_unsafe_u64(in_offset, 64) <= input.len();
         in_offset = zx::add_unsafe_u64(in_offset, 64)) {
        zx::u64 const data_offset =
            zx::add_unsafe_u64(out_offset, sizeof(zx::u64));

        zx::v512_u8 const in_vec =
            input.read_unaligned_unsafe<zx::v512_u8>(in_offset);
        zx::u64 const mask =
            __builtin_ia32_ucmpb512_mask(in_vec, zeros, 4, zx::U64_MAX);
        zx::v512_u8 const compressed =
            __builtin_ia32_compressqi512_mask(in_vec, zeros, mask);

        output.write_unaligned_unsafe<zx::u64>(out_offset, mask);
        output.write_unaligned_unsafe<zx::v512_u8>(data_offset, compressed);

        out_offset = zx::add_unsafe_u64(data_offset, zx::popcount_u64(mask));
    }

    zx::u64 const remainder = zx::sub_unsafe_u64(input.len(), in_offset);
    if (remainder > 0) {
        zx::u64 const loadmask = __builtin_ia32_bzhi_di(zx::U64_MAX, remainder);
        zx::u64 const data_offset =
            zx::add_unsafe_u64(out_offset, sizeof(zx::u64));

        zx::v512_u8 const in_vec = __builtin_ia32_loaddquqi512_mask(
            (zx::v512_i8 const *)&input.ptr()[in_offset], zeros, loadmask);
        zx::u64 const mask =
            __builtin_ia32_ucmpb512_mask(in_vec, zeros, 4, zx::U64_MAX);
        zx::v512_u8 const compressed =
            __builtin_ia32_compressqi512_mask(in_vec, zeros, mask);

        output.write_unaligned_unsafe<zx::u64>(out_offset, mask);
        output.write_unaligned_unsafe<zx::v512_u8>(data_offset, compressed);

        out_offset = zx::add_unsafe_u64(data_offset, zx::popcount_u64(mask));
    }

    *n_written = out_offset;
    return CompressStatus::Ok;
}

enum class [[nodiscard]] DecompressStatus : zx::u8_c {
    Ok = 0,
    Nullptr,
    InputOutputOverlap,
    InputTooShort,
};

DecompressStatus decompress(zx::Backtrace *err_bt, zx::Slice<zx::u8> input,
                            zx::SliceMut<zx::u8> output, zx::u64 *n_read) {
    if (n_read == nullptr) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return DecompressStatus::Nullptr;
    }

    if (input.is_overlapping(output.as_const())) [[unlikely]] {
        zx::Backtrace::collect(err_bt);
        return DecompressStatus::InputOutputOverlap;
    }

    if (output.len() == 0) {
        *n_read = 0;
        return DecompressStatus::Ok;
    }

    zx::v512_u8 const zeros = {};

    zx::u64 in_offset = 0;
    zx::u64 out_offset = 0;
    for (; zx::add_unsafe_u64(out_offset, 64) <= output.len();
         out_offset = zx::add_unsafe_u64(out_offset, 64)) {
        zx::u64 const control_end =
            zx::add_unsafe_u64(in_offset, sizeof(zx::u64));
        if (control_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        zx::u64 const control = input.read_unaligned_unsafe<zx::u64>(in_offset);

        zx::u64 const step_end =
            zx::add_unsafe_u64(control_end, zx::popcount_u64(control));
        if (step_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        zx::v512_u8 const expanded = __builtin_ia32_expandloadqi512_mask(
            (zx::v512_i8 const *)&input.ptr()[control_end], zeros, control);
        output.write_unaligned_unsafe<zx::v512_u8>(out_offset, expanded);

        in_offset = step_end;
    }

    zx::u64 const remainder = zx::sub_unsafe_u64(output.len(), out_offset);
    if (remainder > 0) {
        zx::u64 const control_end =
            zx::add_unsafe_u64(in_offset, sizeof(zx::u64));
        if (control_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        zx::u64 const control = input.read_unaligned_unsafe<zx::u64>(in_offset);

        zx::u64 const step_end =
            zx::add_unsafe_u64(control_end, zx::popcount_u64(control));
        if (step_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        zx::u64 const storemask =
            __builtin_ia32_bzhi_di(zx::U64_MAX, remainder);

        zx::v512_u8 const expanded = __builtin_ia32_expandloadqi512_mask(
            (zx::v512_i8 const *)&input.ptr()[control_end], zeros, control);
        __builtin_ia32_storedquqi512_mask(
            (zx::v512_i8 *)&output.ptr()[out_offset], expanded, storemask);

        in_offset = step_end;
    }

    *n_read = in_offset;
    return DecompressStatus::Ok;
}

} // namespace noz

} // namespace compression
