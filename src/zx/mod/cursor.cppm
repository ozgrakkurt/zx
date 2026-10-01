export module zx:cursor;
import :def;
import :slice;
import :arith;
import :assert;
import :leb128;

export namespace zx {

struct Cursor {
    enum class [[nodiscard]] Status : u8_c {
        Ok = 0,
        Nullptr,
        IntegerOverflow,
        OutOfBounds,
        UnexpectedErr,
    };

    Slice<u8> m_buf;
    u64 m_offset;

    static Cursor init(Slice<u8> buf) {
        return Cursor{
            .m_buf = buf,
            .m_offset = 0,
        };
    }

    u64 offset() const { return this->m_offset; }

    Slice<u8> buf() const { return this->m_buf; }

    template <typename T> Status read(T *out) {
        if (out == nullptr) {
            return Status::Nullptr;
        }

        bool overflow = false;
        u64 const end = add_u64(this->m_offset, sizeof(T), &overflow);

        if (overflow) {
            return Status::IntegerOverflow;
        }

        if (end > this->m_buf.len()) {
            return Status::OutOfBounds;
        }

        __builtin_memcpy(out, this->m_buf.offset_unsafe(this->m_offset),
                         sizeof(T));
        this->m_offset = end;
        return Status::Ok;
    }

    template <typename T> T read_unsafe() {
        u64 const end = add_unsafe_u64(this->m_offset, sizeof(T));
        assert_dev(end <= this->m_buf.len());

        T out;
        __builtin_memcpy(&out, this->m_buf.offset_unsafe(this->m_offset),
                         sizeof(T));
        this->m_offset = end;
        return out;
    }

    template <typename T> T read_panic() {
        T out;
        __builtin_memset(&out, 0xaa, sizeof(out));
        assert(Status::Ok == this->read(&out));
        return out;
    }

    leb128::DecodeStatus read_uleb128(u64 *out) {
        return leb128::decode_u64(this->m_buf, &this->m_offset, out);
    }

    leb128::DecodeStatus read_sleb128(i64 *out) {
        return leb128::decode_i64(this->m_buf, &this->m_offset, out);
    }

    Status skip(u64 len) {
        bool overflow = false;
        u64 const end = add_u64(this->m_offset, len, &overflow);
        if (overflow) {
            return Status::IntegerOverflow;
        }
        if (end > this->m_buf.len()) {
            return Status::OutOfBounds;
        }

        this->m_offset = end;
        return Status::Ok;
    }

    Status seek(u64 offset) {
        if (offset > this->m_buf.len()) {
            return Status::OutOfBounds;
        }
        this->m_offset = offset;
        return Status::Ok;
    }
};

struct WriteCursor {
    enum class [[nodiscard]] Status : u8_c {
        Ok = 0,
        IntegerOverflow,
        OutOfBounds,
        UnexpectedErr,
    };

    SliceMut<u8> m_buf;
    u64 m_offset;

    static WriteCursor init(SliceMut<u8> buf) {
        return WriteCursor{
            .m_buf = buf,
            .m_offset = 0,
        };
    }

    u64 offset() const { return this->m_offset; }

    SliceMut<u8> buf() const { return this->m_buf; }

    template <typename T> Status write(T val) {
        bool overflow = false;
        u64 const end = add_u64(this->m_offset, sizeof(T), &overflow);
        if (overflow) {
            return Status::IntegerOverflow;
        }

        if (end > this->m_buf.len()) {
            return Status::OutOfBounds;
        }

        __builtin_memcpy(this->m_buf.offset_unsafe(this->m_offset), &val,
                         sizeof(T));
        this->m_offset = end;
        return Status::Ok;
    }

    template <typename T> void write_unsafe(T val) {
        u64 const end = add_unsafe_u64(this->m_offset, sizeof(T));
        assert_dev(end <= this->m_buf.len());

        __builtin_memcpy(this->m_buf.offset_unsafe(this->m_offset), &val,
                         sizeof(T));
        this->m_offset = end;
    }

    template <typename T> void write_panic(T val) {
        assert(Status::Ok == this->write(val));
    }
};

}; // namespace zx
