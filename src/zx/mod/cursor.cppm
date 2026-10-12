export module zx:cursor;
import :def;
import :slice;
import :arith;
import :assert;
import :leb128;
import :cpu_context;

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

    bool is_finished() const { return this->m_offset == this->m_buf.len(); }

    template <typename T> Status read(CPUContext *err_ctx, T *out) {
        if (out == nullptr) [[unlikely]] {
            zx_getcpucontext_(err_ctx);
            return Status::Nullptr;
        }

        bool overflow = false;
        u64 const end = add_u64(this->m_offset, sizeof(T), &overflow);

        if (overflow) [[unlikely]] {
            zx_getcpucontext_(err_ctx);
            return Status::IntegerOverflow;
        }

        if (end > this->m_buf.len()) [[unlikely]] {
            zx_getcpucontext_(err_ctx);
            return Status::OutOfBounds;
        }

        __builtin_memcpy(out, this->m_buf.offset_unsafe(this->m_offset),
                         sizeof(T));
        this->m_offset = end;
        return Status::Ok;
    }

    leb128::DecodeStatus read_uleb128(CPUContext *err_ctx, u64 *out) {
        return leb128::decode_u64(err_ctx, this->m_buf, &this->m_offset, out);
    }

    leb128::DecodeStatus read_sleb128(CPUContext *err_ctx, i64 *out) {
        return leb128::decode_i64(err_ctx, this->m_buf, &this->m_offset, out);
    }

    Status skip(CPUContext *err_ctx, u64 len) {
        bool overflow = false;
        u64 const end = add_u64(this->m_offset, len, &overflow);
        if (overflow) [[unlikely]] {
            zx_getcpucontext_(err_ctx);
            return Status::IntegerOverflow;
        }
        if (end > this->m_buf.len()) [[unlikely]] {
            zx_getcpucontext_(err_ctx);
            return Status::OutOfBounds;
        }

        this->m_offset = end;
        return Status::Ok;
    }

    Status seek(CPUContext *err_ctx, u64 offset) {
        if (offset > this->m_buf.len()) [[unlikely]] {
            zx_getcpucontext_(err_ctx);
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

    bool is_finished() const { return this->m_offset == this->m_buf.len(); }

    Slice<u8> written() const {
        return this->m_buf.slice_unsafe(0, this->m_offset).as_const();
    }

    template <typename T> Status write(CPUContext *err_ctx, T val) {
        bool overflow = false;
        u64 const end = add_u64(this->m_offset, sizeof(T), &overflow);
        if (overflow) [[unlikely]] {
            zx_getcpucontext_(err_ctx);
            return Status::IntegerOverflow;
        }

        if (end > this->m_buf.len()) [[unlikely]] {
            zx_getcpucontext_(err_ctx);
            return Status::OutOfBounds;
        }

        __builtin_memcpy(this->m_buf.offset_mut_unsafe(this->m_offset), &val,
                         sizeof(T));
        this->m_offset = end;
        return Status::Ok;
    }
};

}; // namespace zx
