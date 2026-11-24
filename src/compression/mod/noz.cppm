export module compression:noz;
import zx;

export namespace compression {

namespace noz {

enum class [[nodiscard]] CompressBoundStatus : zx::u8_c {
    Ok = 0,
    Nullptr,
    IntegerOverflow,
};

inline CompressBoundStatus compress_bound(zx::Backtrace *err_bt,
                                          zx::u64 input_size, zx::u64 *bound) {
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

inline CompressStatus compress(zx::Backtrace *err_bt, zx::Slice<zx::u8> input,
                               zx::SliceMut<zx::u8> output,
                               zx::u64 *n_written) {
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

    zx::u64 in_offset = 0;
    zx::u64_c out_offset = 0;
    for (; zx::add_unsafe_u64(in_offset, 64) <= input.len();
         in_offset = zx::add_unsafe_u64(in_offset, 64)) {
        __asm__ volatile(
                // read input
                "vmovdqu8 (%[input]), %%ymm31\n\t"
                "vmovdqu8 32(%[input]), %%ymm30\n\t"
                // check zeroes to create the masks
                "vptestmb %%ymm31, %%ymm31, %%k7\n\t"
                "vptestmb %%ymm30, %%ymm30, %%k6\n\t"
                // move masks to gpr
                "kmovq %%k7, %%r11\n\t"
                "kmovq %%k6, %%r10\n\t"
                // compress to other register
                "vpcompressb %%ymm31, %%ymm29 %{%%k7%}\n\t"
                "vpcompressb %%ymm30, %%ymm28 %{%%k6%}\n\t"
                // calculate offset for second write"
                "popcntq %%r11, %%r9\n\t"
                "addq %[out_offset], %%r9\n\t"
                // write data
                "vmovdqu8 %%ymm29, 8(%[output]), %[out_offset])\n\t"
                "vmovdqu8 %%ymm29, 8(%[output], %%r9)\n\t"
                // combine two control masks into one
                "shlq $32, %%r10, %%r9\n\t"
                "orq %%r11, %%r9\n\t"
                // write the combined 8 byte control mask
                "movq %%r8, (%[output], %[out_offset])\n\t"
                // calculate num total bytes written
                "popcntq %%r9, %%r9\n\t"
                // update output offset
                "addq %%r9, %[out_offset]\n\t"
                : [control0] "=&r"(control0), [control1] "=&r"
                : [input] "r"(&input[in_offset]), [output] "r"(&output[data_offset])
                : "ymm31", "ymm30", "ymm29", "ymm28", "k7", "k6", "r9", "r10", "r11", "memory");
    }

    zx::u64 const remainder = zx::sub_unsafe_u64(input.len(), in_offset);
    if (remainder > 0) {
        register zx::u64_c const loadmask __asm__("k6") =
            __builtin_ia32_bzhi_di(~(zx::u64){0}, remainder);
        zx::u64 const data_offset =
            zx::add_unsafe_u64(out_offset, sizeof(zx::u64));
        zx::u64_c n_w;
        __asm__("vmovdqu8 %[input], %%zmm31 %{%[loadmask]%}%{z%}\n\t"
                "vpcmpub $4, %%zmm31, %[zeros], %%k7\n\t"
                "kmovq %%k7, %[out0]\n\t"
                "kmovq %%k7, %[n_w]\n\t"
                "vpcompressb %%zmm31, %%zmm30 %{%%k7%}\n\t"
                "vmovdqu8 %%zmm30, %[out1]\n\t"
                "popcntq %[n_w], %[n_w]\n\t"
                : [out0] "=&m"(*(zx::u8(*)[8]) & output.ptr()[out_offset]),
                  [out1] "=&m"(*(zx::u8(*)[64]) & output.ptr()[data_offset]),
                  [n_w] "=&r"(n_w)
                : [input] "m"(*(zx::u8 const(*)[64]) & input.ptr()[in_offset]),
                  [zeros] "v"(zeros), [loadmask] "k"(loadmask)
                : "zmm31", "zmm30", "k7");

        out_offset = zx::add_unsafe_u64(data_offset, n_w);
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

inline DecompressStatus decompress(zx::Backtrace *err_bt,
                                   zx::Slice<zx::u8> input,
                                   zx::SliceMut<zx::u8> output,
                                   zx::u64 *n_read) {
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

        register zx::u64_c control __asm__("k7") =
            input.read_unaligned_unsafe<zx::u64>(in_offset);

        zx::u64 const step_end =
            zx::add_unsafe_u64(control_end, zx::popcount_u64(control));
        if (step_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        __asm__(
            "vpexpandb %[input], %%zmm31 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm31, %[output]\n\t"
            : [output] "=&m"(*(zx::u8(*)[64]) & output.ptr()[out_offset])
            : [input] "m"(*(zx::u8 const(*)[64]) & input.ptr()[control_end]),
              [control] "k"(control)
            : "zmm31");

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

        register zx::u64_c control __asm__("k7") =
            input.read_unaligned_unsafe<zx::u64>(in_offset);

        zx::u64 const step_end =
            zx::add_unsafe_u64(control_end, zx::popcount_u64(control));
        if (step_end > input.len()) [[unlikely]] {
            zx::Backtrace::collect(err_bt);
            return DecompressStatus::InputTooShort;
        }

        register zx::u64_c const storemask __asm__("k6") =
            __builtin_ia32_bzhi_di(~(zx::u64){0}, remainder);

        __asm__(
            "vpexpandb %[input], %%zmm31 %{%[control]%}%{z%}\n\t"
            "vmovdqu8 %%zmm31, %[output] %{%[storemask]%}\n\t"
            : [output] "=&m"(*(zx::u8(*)[64]) & output.ptr()[out_offset])
            : [input] "m"(*(zx::u8 const(*)[64]) & input.ptr()[control_end]),
              [control] "k"(control), [storemask] "k"(storemask)
            : "zmm31");

        in_offset = step_end;
    }

    *n_read = in_offset;
    return DecompressStatus::Ok;
}

} // namespace noz

} // namespace compression
