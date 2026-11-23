export module zx:leb128;
import :def;
import :assert;
import :slice;
import :backtrace;

export namespace zx {

namespace leb128 {

constexpr u8 CONT_BIT = 0x80__wbu;
constexpr u8 SIGN_BIT = 0x40__wbu;

inline u8 getval_(u8 byte) { return byte & 0x7f__wbu; }

enum class [[nodiscard]] EncodeStatus : u8_c {
    Ok = 0,
    Nullptr,
    InvalidOffset,
    OutOfCapacity,
};

inline EncodeStatus encode_u64(Backtrace *err_bt, u64 num, SliceMut<u8> out,
                               u64 *offset) {
    if (offset == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return EncodeStatus::Nullptr;
    }

    u64 offs = *offset;

    if (offs > out.len()) [[unlikely]] {
        Backtrace::collect(err_bt);
        return EncodeStatus::InvalidOffset;
    }

    while (true) {
        assert_dev(offs <= out.len());
        if (offs == out.len()) [[unlikely]] {
            Backtrace::collect(err_bt);
            *offset = offs;
            return EncodeStatus::OutOfCapacity;
        }

        u8 const byte = getval_((u8)num);
        num = num >> 7;
        if (num != 0) {
            out.write_unsafe(offs, byte | CONT_BIT);
            offs++;
        } else {
            out.write_unsafe(offs, byte);
            offs++;
            *offset = offs;
            return EncodeStatus::Ok;
        }
    }
}

enum class [[nodiscard]] DecodeStatus : u8_c {
    Ok = 0,
    Nullptr,
    InvalidOffset,
    UnexpectedEndOfInput,
    InvalidInput,
};

inline DecodeStatus decode_u64(Backtrace *err_bt, Slice<u8> input, u64 *offset,
                               u64 *num) {
    if (offset == nullptr || num == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return DecodeStatus::Nullptr;
    }

    u64 offs = *offset;

    if (offs > input.len()) [[unlikely]] {
        Backtrace::collect(err_bt);
        return DecodeStatus::InvalidOffset;
    }

    u64 out = 0;
    u8 shift = 0;
    for (u8 iter = 0; iter < 10; iter++) {
        assert_dev(offs <= input.len());
        if (offs == input.len()) [[unlikely]] {
            Backtrace::collect(err_bt);
            *num = 0;
            *offset = offs;
            return DecodeStatus::UnexpectedEndOfInput;
        }

        u8 const byte = input.read_unsafe(offs);
        offs++;

        u64 const val = getval_(byte);

        // this will lose data if we don't check.
        if (shift == 63 && val > 1) [[unlikely]] {
            Backtrace::collect(err_bt);
            *num = 0;
            *offset = offs;
            return DecodeStatus::InvalidInput;
        }

        assert_dev(shift < 64);
        out |= val << shift;
        shift += 7__wbu;
        if ((byte & CONT_BIT) == 0) {
            *num = out;
            *offset = offs;
            return DecodeStatus::Ok;
        }
    }

    Backtrace::collect(err_bt);
    *num = 0;
    *offset = offs;
    return DecodeStatus::InvalidInput;
}

inline EncodeStatus encode_i64(Backtrace *err_bt, i64 num, SliceMut<u8> out,
                               u64 *offset) {
    if (offset == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return EncodeStatus::Nullptr;
    }

    u64 offs = *offset;

    if (offs > out.len()) [[unlikely]] {
        Backtrace::collect(err_bt);
        return EncodeStatus::InvalidOffset;
    }

    while (true) {
        assert_dev(offs <= out.len());
        if (offs == out.len()) [[unlikely]] {
            Backtrace::collect(err_bt);
            *offset = offs;
            return EncodeStatus::OutOfCapacity;
        }

        u8 const byte = getval_((u8)num);
        num >>= 7;
        bool const negative_end = num == -1 && (byte & SIGN_BIT) != 0;
        bool const positive_end = num == 0 && (byte & SIGN_BIT) == 0;
        if (negative_end || positive_end) {
            out.write_unsafe(offs, byte);
            offs++;
            *offset = offs;
            return EncodeStatus::Ok;
        } else {
            out.write_unsafe(offs, byte | CONT_BIT);
            offs++;
        }
    }
}

inline DecodeStatus decode_i64(Backtrace *err_bt, Slice<u8> input, u64 *offset,
                               i64 *num) {
    if (offset == nullptr || num == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return DecodeStatus::Nullptr;
    }

    u64 offs = *offset;

    if (offs > input.len()) [[unlikely]] {
        Backtrace::collect(err_bt);
        return DecodeStatus::InvalidOffset;
    }

    u64 out = 0;
    u8 shift = 0;
    for (u8 iter = 0; iter < 10; iter++) {
        assert_dev(offs <= input.len());
        if (offs == input.len()) [[unlikely]] {
            Backtrace::collect(err_bt);
            *num = 0;
            *offset = offs;
            return DecodeStatus::UnexpectedEndOfInput;
        }

        u8 const byte = input.read_unsafe(offs);
        offs++;

        u64 const val = getval_(byte);

        // this will lose data if we don't check.
        if (shift == 63 && val != 0x00 && val != 0x7f) [[unlikely]] {
            Backtrace::collect(err_bt);
            *num = 0;
            *offset = offs;
            return DecodeStatus::InvalidInput;
        }

        assert_dev(shift < 64);
        out |= val << shift;
        shift += 7__wbu;
        if ((byte & CONT_BIT) == 0) {
            // sign extend if needed
            bool const is_negative = (byte & SIGN_BIT) == SIGN_BIT;
            if (shift < 64 && is_negative) {
                out |= ~u64{0} << shift;
            }
            *num = (i64)out;
            *offset = offs;
            return DecodeStatus::Ok;
        }
    }

    Backtrace::collect(err_bt);
    *num = 0;
    *offset = offs;
    return DecodeStatus::InvalidInput;
}

} // namespace leb128

} // namespace zx
