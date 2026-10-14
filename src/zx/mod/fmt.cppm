export module zx:fmt;
import :def;
import :assert;
import :slice;

export namespace zx {

struct Fmt {
    SliceMut<u8> m_mem;
    u64 m_offset;

    static Fmt init(SliceMut<u8> mem) {
        return Fmt{
            .m_mem = mem,
            .m_offset = 0,
        };
    }

    SliceMut<u8> mem() const { return this->m_mem; }

    u64 n_written() const {
        assert_dev(this->m_mem.len() >= this->m_offset);
        return this->m_offset;
    }

    Slice<u8> written() const {
        return this->m_mem.slice_unsafe(0, this->m_offset).as_const();
    }

    u64 capacity() const {
        assert_dev(this->m_mem.len() >= this->m_offset);
        return this->m_mem.len();
    }

    u64 capacity_left() const {
        return sub_unsafe_u64(this->m_mem.len(), this->m_offset);
    }

    void reset() {
#ifdef ZX_DEV
        this->m_mem.memset(0xaa);
#endif
        this->m_offset = 0;
    }

    void push_str(Slice<u8> str, bool *overflow) {
        SliceMut<u8> const mem = this->m_mem;
        u64 const offset = this->m_offset;
        u64 const capacity_left = this->capacity_left();
        u64 const n_write =
            (capacity_left > str.len()) ? str.len() : capacity_left;
        *overflow |= (n_write < str.len());
        if (n_write > 0) {
            mem.slice_unsafe(offset, n_write)
                .memcpy_from_unsafe(str.slice_unsafe(0, n_write));
        }
        this->m_offset = add_unsafe_u64(offset, n_write);
    }

    void push_char(u8 ch, bool *overflow) {
        SliceMut<u8> const mem = this->m_mem;
        u64 const offset = this->m_offset;
        bool const capacity_left = this->capacity_left() > 0;
        *overflow |= !capacity_left;
        if (capacity_left) {
            mem.write_unsafe(offset, ch);
            this->m_offset = add_unsafe_u64(offset, 1);
        }
    }

    void push_u64(u64 val, bool *overflow) {
        constexpr u64 MAX_NUM_DIGITS = 20;
        u8 digits[MAX_NUM_DIGITS];

        u64 pos = MAX_NUM_DIGITS;
        do {
            pos = sub_unsafe_u64(pos, 1);
            digits[pos] = (u8)'0' + (u8)(val % 10);
            val /= 10;
        } while (val > 0);
        u64 const len = sub_unsafe_u64(MAX_NUM_DIGITS, pos);
        this->push_str(Slice<u8>::init_unsafe(&digits[pos], len), overflow);
    }

    void push_i64(i64 val, bool *overflow) {
        if (val >= 0) {
            this->push_u64((u64)val, overflow);
        } else {
            this->push_char((u8)'-', overflow);
            this->push_u64((u64)-val, overflow);
        }
    }

    void push_u64_hex(u64 val, bool *overflow) {
        constexpr u64 MAX_NUM_DIGITS = 22;
        u8 digits[MAX_NUM_DIGITS];

        constexpr u8 TABLE[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

        u64 pos = MAX_NUM_DIGITS;
        do {
            pos = sub_unsafe_u64(pos, 1);
            digits[pos] = TABLE[val % 16];
            val /= 16;
        } while (val > 0);

        pos = sub_unsafe_u64(pos, 1);
        digits[pos] = 'x';
        pos = sub_unsafe_u64(pos, 1);
        digits[pos] = '0';

        u64 const len = sub_unsafe_u64(MAX_NUM_DIGITS, pos);
        this->push_str(Slice<u8>::init_unsafe(&digits[pos], len), overflow);
    }
};

} // namespace zx
