export module zx:fmt;
import :def;

export namespace zx {

struct Fmt {
    u8 *m_mem;
    u64 m_capacity;
    u64 m_offset;

    static Fmt init(u8 *mem, u64 capacity) {
        return Fmt{
            .m_mem = mem,
            .m_capacity = capacity,
            .m_offset = 0,
        };
    }
};

} // namespace zx
