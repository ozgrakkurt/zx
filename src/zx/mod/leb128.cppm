export module zx:leb128;
import :def;
import :raw_assert;

export namespace zx {

namespace leb128 {

constexpr u8 CONT_BIT = 0x80__wbu;
constexpr u8 SIGN_BIT = 0x40__wbu;

u8 getval_(u8 byte) { return byte & 0x7f__wbu; }

enum class [[nodiscard]] EncodeStatus : u8_c {
    Ok = 0,
    Nullptr,
    InvalidOffset,
    OutOfCapacity,
};

EncodeStatus encode_u64(u64 num, u8 *out, u64 capacity, u64 *offset) {
    if (out == nullptr || offset == nullptr) {
        return EncodeStatus::Nullptr;
    }

    u64 offs = *offset;

    if (offs > capacity) {
        return EncodeStatus::InvalidOffset;
    }

    while (true) {
        raw_assert_dev(offs <= capacity);
        if (offs == capacity) {
            *offset = offs;
            return EncodeStatus::OutOfCapacity;
        }

        u8 const byte = getval_((u8)num);
        num = num >> 7;
        if (num != 0) {
            out[offs] = byte | CONT_BIT;
            offs++;
        } else {
            out[offs] = byte;
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

DecodeStatus decode_u64(u8 const *input, u64 len, u64 *offset, u64 *num) {
    if (input == nullptr || offset == nullptr || num == nullptr) {
        return DecodeStatus::Nullptr;
    }

    u64 offs = *offset;

    if (offs > len) {
        return DecodeStatus::InvalidOffset;
    }

    u64 out = 0;
    u8 shift = 0;
    for (u8 iter = 0; iter < 10; iter++) {
        raw_assert_dev(offs <= len);
        if (offs == len) {
            *num = 0;
            *offset = offs;
            return DecodeStatus::UnexpectedEndOfInput;
        }

        u8 const byte = input[offs];
        offs++;

        raw_assert_dev(shift < 64);
        out |= u64{getval_(byte)} << shift;
        shift += 7__wbu;
        if ((byte & CONT_BIT) == 0) {
            *num = out;
            *offset = offs;
            return DecodeStatus::Ok;
        }
    }

    *num = 0;
    *offset = offs;
    return DecodeStatus::InvalidInput;
}

EncodeStatus encode_i64(i64 num, u8 *out, u64 capacity, u64 *offset) {
    if (out == nullptr || offset == nullptr) {
        return EncodeStatus::Nullptr;
    }

    u64 offs = *offset;

    if (offs > capacity) {
        return EncodeStatus::InvalidOffset;
    }

    while (true) {
        raw_assert_dev(offs <= capacity);
        if (offs == capacity) {
            *offset = offs;
            return EncodeStatus::OutOfCapacity;
        }

        u8 const byte = getval_((u8)num);
        num >>= 7;
        bool const negative_end = num == -1 && (byte & SIGN_BIT) != 0;
        bool const positive_end = num == 0 && (byte & SIGN_BIT) == 0;
        if (negative_end || positive_end) {
            out[offs] = byte;
            offs++;
            *offset = offs;
            return EncodeStatus::Ok;
        } else {
            out[offs] = byte | CONT_BIT;
            offs++;
        }
    }
}

DecodeStatus decode_i64(u8 const *input, u64 len, u64 *offset, i64 *num) {
    if (input == nullptr || offset == nullptr || num == nullptr) {
        return DecodeStatus::Nullptr;
    }

    u64 offs = *offset;

    if (offs > len) {
        return DecodeStatus::InvalidOffset;
    }

    u64 out = 0;
    u8 shift = 0;
    for (u8 iter = 0; iter < 10; iter++) {
        raw_assert_dev(offs <= len);
        if (offs == len) {
            *num = 0;
            *offset = offs;
            return DecodeStatus::UnexpectedEndOfInput;
        }

        u8 const byte = input[offs];
        offs++;
        raw_assert_dev(shift < 64);
        out |= u64{getval_(byte)} << shift;
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

    *num = 0;
    *offset = offs;
    return DecodeStatus::InvalidInput;
}

} // namespace leb128

} // namespace zx
