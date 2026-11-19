// The implementation is ported from https://github.com/uuid-rs/uuid
//
// Original license for the rust code:
// Copyright (c) 2014 The Rust Project Developers
// Copyright (c) 2018 Ashley Mannix, Christopher Armstrong, Dylan DPC, Hunar
// Roop Kahlon
//
// Permission is hereby granted, free of charge, to any
// person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the
// Software without restriction, including without
// limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
// IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

export module zx:uuid_v4;
import :def;
import :xoshiro256;
import :fmt;
import :slice;

export namespace zx {

namespace uuid_v4 {

u128 generate(Xoshiro256 *rng) {
    u128 out = 0;
    rng->fill(SliceMut<u8>::init_from_ptr_unsafe<u128>(&out));
    // This is an optimized method for generating random UUIDs that just masks
    // out the bits for the version and variant and sets them both together
    return (out & u128{0xFFFFFFFFFFFF4FFFBFFFFFFFFFFFFFFF__wbu}) |
           u128{0x40008000000000000000__wbu};
}

u8 nibble_to_hex_(u8 nibble, u8 alpha_offset) {
    u8 const a = (nibble > 9) ? alpha_offset : u8{0};
    return nibble + '0' + a;
}

void format_simple_unsafe_(u128 uuid, bool upper_case, SliceMut<u8> out) {
    u8 const alpha_offset = upper_case ? u8{0x07} : u8{0x27};
    u8 bytes[sizeof(u128)];
    static_assert(sizeof(bytes) == sizeof(u128));
    u128 const swapped = __builtin_bswapg(uuid);
    __builtin_memcpy(bytes, &swapped, sizeof(u128));
    for (u8 i = 0; i < sizeof(bytes); i++) {
        u8 const x = bytes[i];
        out.write_unsafe(i * 2__wbu, nibble_to_hex_(x >> 4, alpha_offset));
        out.write_unsafe(i * 2__wbu + 1__wbu,
                         nibble_to_hex_(x & u8{0x0f}, alpha_offset));
    }
}

void print(u128 uuid, bool upper_case, Fmt *fmt, bool *fmt_overflow) {
    u8 simple_buf[sizeof(u128) * 2__wbu];
    SliceMut<u8> simple =
        SliceMut<u8>::init_unsafe(simple_buf, sizeof(simple_buf));
    format_simple_unsafe_(uuid, upper_case, simple);

    u8 dst_buf[sizeof(u128) * 2__wbu + 4__wbu];
    SliceMut<u8> dst = SliceMut<u8>::init_unsafe(dst_buf, sizeof(dst_buf));

    Slice<u8> simple_c = simple.as_const();

    dst.slice_unsafe(0, 8).memcpy_from_unsafe(simple_c.slice_unsafe(0, 8));
    dst.slice_unsafe(9, 4).memcpy_from_unsafe(simple_c.slice_unsafe(8, 4));
    dst.slice_unsafe(14, 4).memcpy_from_unsafe(simple_c.slice_unsafe(12, 4));
    dst.slice_unsafe(19, 4).memcpy_from_unsafe(simple_c.slice_unsafe(16, 4));
    dst.slice_unsafe(24, 12).memcpy_from_unsafe(simple_c.slice_unsafe(20, 12));

    dst.write_unsafe(8, '-');
    dst.write_unsafe(13, '-');
    dst.write_unsafe(18, '-');
    dst.write_unsafe(23, '-');

    fmt->push_str(dst.as_const(), fmt_overflow);
}

} // namespace uuid_v4

} // namespace zx
