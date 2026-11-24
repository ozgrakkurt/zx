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
                                   zx::u64 *bound);

enum class [[nodiscard]] CompressStatus : zx::u8_c {
    Ok = 0,
    Nullptr,
    InputOutputOverlap,
    BoundFail,
    OutputTooShort,
};

CompressStatus compress(zx::Backtrace *err_bt, zx::Slice<zx::u8> input,
                        zx::SliceMut<zx::u8> output, zx::u64 *n_written);

enum class [[nodiscard]] DecompressStatus : zx::u8_c {
    Ok = 0,
    Nullptr,
    InputOutputOverlap,
    InputTooShort,
};

DecompressStatus decompress(zx::Backtrace *err_bt, zx::Slice<zx::u8> input,
                            zx::SliceMut<zx::u8> output, zx::u64 *n_read);

} // namespace noz

} // namespace compression
