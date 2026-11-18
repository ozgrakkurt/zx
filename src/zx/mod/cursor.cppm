export module zx:cursor;
import :def;
import :slice;
import :arith;
import :assert;
import :leb128;
import :backtrace;

export namespace zx {

struct Cursor {
    enum class [[nodiscard]] Status : u8_c {
        Ok = 0,
        Nullptr,
        IntegerOverflow,
        OutOfBounds,
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

    bool is_finished() const { return this->m_offset == this->m_buf.len(); }

    template <typename T> Status read(Backtrace *err_bt, T *out) {
        if (out == nullptr) [[unlikely]] {
            Backtrace::collect(err_bt);
            return Status::Nullptr;
        }

        bool overflow = false;
        u64 const end = add_u64(this->m_offset, sizeof(T), &overflow);

        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return Status::IntegerOverflow;
        }

        if (end > this->m_buf.len()) [[unlikely]] {
            Backtrace::collect(err_bt);
            return Status::OutOfBounds;
        }

        __builtin_memcpy(out, this->m_buf.offset_unsafe(this->m_offset),
                         sizeof(T));
        this->m_offset = end;
        return Status::Ok;
    }

    leb128::DecodeStatus read_uleb128(Backtrace *err_bt, u64 *out) {
        return leb128::decode_u64(err_bt, this->m_buf, &this->m_offset, out);
    }

    leb128::DecodeStatus read_sleb128(Backtrace *err_bt, i64 *out) {
        return leb128::decode_i64(err_bt, this->m_buf, &this->m_offset, out);
    }

    Status skip(Backtrace *err_bt, u64 len) {
        bool overflow = false;
        u64 const end = add_u64(this->m_offset, len, &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return Status::IntegerOverflow;
        }
        if (end > this->m_buf.len()) [[unlikely]] {
            Backtrace::collect(err_bt);
            return Status::OutOfBounds;
        }

        this->m_offset = end;
        return Status::Ok;
    }

    Status seek(Backtrace *err_bt, u64 offset) {
        if (offset > this->m_buf.len()) [[unlikely]] {
            Backtrace::collect(err_bt);
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

    bool is_finished() const { return this->m_offset == this->m_buf.len(); }

    Slice<u8> written() const {
        return this->m_buf.slice_unsafe(0, this->m_offset).as_const();
    }

    template <typename T> Status write(Backtrace *err_bt, T val) {
        bool overflow = false;
        u64 const end = add_u64(this->m_offset, sizeof(T), &overflow);
        if (overflow) [[unlikely]] {
            Backtrace::collect(err_bt);
            return Status::IntegerOverflow;
        }

        if (end > this->m_buf.len()) [[unlikely]] {
            Backtrace::collect(err_bt);
            return Status::OutOfBounds;
        }

        __builtin_memcpy(this->m_buf.offset_mut_unsafe(this->m_offset), &val,
                         sizeof(T));
        this->m_offset = end;
        return Status::Ok;
    }
};

} // namespace zx
