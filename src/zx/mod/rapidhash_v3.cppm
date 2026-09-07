// THIS CODE IS ADAPTED FROM
// https://github.com/Nicoshev/rapidhash/blob/rapidhash_v3/rapidhash.h
// Specifically the "Nano" version, which is optimized for code size and small
// inputs, is used here.
// Original code is licensed under MIT:
// https://github.com/Nicoshev/rapidhash/blob/rapidhash_v3/LICENSE
// Original License:
//
// Copyright 2025 Nicolas De Carli
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

export module zx:rapidhash_v3;
import :def;
import :slice;
import :arith;
import :assert;

export namespace zx {
namespace rapidhash_v3 {

constexpr u64 RAPIDHASH_V3_SECRET_[8] = {
    0x2d358dccaa6c78a5, 0x8bb84b93962eacc9, 0x4b33a62ed433d4a3,
    0x4d5a2da51de1aa47, 0xa0761d6478bd642f, 0xe7037ed1a0b428db,
    0x90ed1765281c388c, 0xaaaaaaaaaaaaaaaa,
};

void rapidhash_v3_mum_(u64 *a, u64 *b) {
    u128 const r = ((u128){*a}) * ((u128){*b});
    *a = (u64)r;
    *b = (u64)(r >> 64);
}

u64 rapidhash_v3_mix_(u64 a, u64 b) {
    rapidhash_v3_mum_(&a, &b);
    return a ^ b;
}

u64 rapidhash_v3_finish_(u64 a, u64 b, u64 seed, u64 i) {
    a ^= RAPIDHASH_V3_SECRET_[1];
    b ^= seed;
    rapidhash_v3_mum_(&a, &b);
    return rapidhash_v3_mix_(a ^ RAPIDHASH_V3_SECRET_[7],
                             b ^ RAPIDHASH_V3_SECRET_[1] ^ i);
}

u64 rapidhash_v3_16_(Slice<u8> key, u64 seed) {
    assert(key.len() <= 16);

    u64 a = 0;
    u64 b = 0;
    u64 s = seed;

    s ^=
        rapidhash_v3_mix_(s ^ RAPIDHASH_V3_SECRET_[2], RAPIDHASH_V3_SECRET_[1]);

    if (key.len() >= 4) {
        s ^= key.len();
        if (key.len() >= 8) {
            a = key.read_unaligned_unsafe<u64>(0);
            b = key.read_unaligned_unsafe<u64>(sub_unsafe_u64(key.len(), 8));
        } else {
            a = key.read_unaligned_unsafe<u32>(0);
            b = key.read_unaligned_unsafe<u32>(sub_unsafe_u64(key.len(), 4));
        }
    } else if (key.len() > 0) {
        a = shl_unsafe_u64((u64)u8{key.read_unsafe(0)}, 45) |
            ((u64)u8{key.read_unsafe(sub_unsafe_u64(key.len(), 1))});
        b = (u64)key.read_unsafe(shr_unsafe_u64(key.len(), 1));
    } else {
        a = 0;
        b = 0;
    }

    return rapidhash_v3_finish_(a, b, s, key.len());
}

u64 rapidhash_v3_long_(Slice<u8> key, u64 seed) {
    assert(key.len() > 16);
    u64 a = 0;
    u64 b = 0;
    u64 s = seed;

    u64 offset = 0;

    s ^=
        rapidhash_v3_mix_(s ^ RAPIDHASH_V3_SECRET_[2], RAPIDHASH_V3_SECRET_[1]);

    if (key.len() > 48) {
        u64 s0 = s;
        u64 s1 = s;

        do {
            s = rapidhash_v3_mix_(
                key.read_unaligned_unsafe<u64>(offset) ^
                    RAPIDHASH_V3_SECRET_[0],
                key.read_unaligned_unsafe<u64>(add_unsafe_u64(offset, 8)) ^ s);

            s0 = rapidhash_v3_mix_(
                key.read_unaligned_unsafe<u64>(add_unsafe_u64(offset, 16)) ^
                    RAPIDHASH_V3_SECRET_[1],
                key.read_unaligned_unsafe<u64>(add_unsafe_u64(offset, 24)) ^
                    s0);

            s1 = rapidhash_v3_mix_(
                key.read_unaligned_unsafe<u64>(add_unsafe_u64(offset, 32)) ^
                    RAPIDHASH_V3_SECRET_[2],
                key.read_unaligned_unsafe<u64>(add_unsafe_u64(offset, 40)) ^
                    s1);

            offset = add_unsafe_u64(offset, 48);
        } while (add_unsafe_u64(offset, 48) < key.len());

        s ^= s0;
        s ^= s1;
    }

    if (add_unsafe_u64(offset, 16) < key.len()) {
        s = rapidhash_v3_mix_(
            key.read_unaligned_unsafe<u64>(offset) ^ RAPIDHASH_V3_SECRET_[2],
            key.read_unaligned_unsafe<u64>(add_unsafe_u64(offset, 8)) ^ s);
        if (add_unsafe_u64(offset, 32) < key.len()) {
            s = rapidhash_v3_mix_(
                key.read_unaligned_unsafe<u64>(add_unsafe_u64(offset, 16)) ^
                    RAPIDHASH_V3_SECRET_[2],
                key.read_unaligned_unsafe<u64>(add_unsafe_u64(offset, 24)) ^ s);
        }
    }

    u64 const i = sub_unsafe_u64(key.len(), offset);
    a = key.read_unaligned_unsafe<u64>(sub_unsafe_u64(key.len(), 16)) ^ i;
    b = key.read_unaligned_unsafe<u64>(sub_unsafe_u64(key.len(), 8));

    return rapidhash_v3_finish_(a, b, s, i);
}

u64 hash(Slice<u8> key, u64 seed) {
    if (key.len() <= 16) {
        return rapidhash_v3_16_(key, seed);
    } else {
        return rapidhash_v3_long_(key, seed);
    }
}

} // namespace rapidhash_v3
} // namespace zx
