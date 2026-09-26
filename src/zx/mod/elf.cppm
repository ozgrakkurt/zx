export module zx:elf;
import :def;
import :assert;
import :arith;
import :slice;
import :alloc;

export namespace zx {

namespace elf {

// this is supposed to be equal to what might be commonly named Elf64_Shdr
extern "C" struct SectionHeader {
    u32 name;      /* Section name, index in string tbl */
    u32 type;      /* Type of section */
    u64 flags;     /* Miscellaneous section attributes */
    u64 addr;      /* Section virtual addr at execution */
    u64 offset;    /* Section file offset */
    u64 size;      /* Size of section in bytes */
    u32 link;      /* Index of another section */
    u32 info;      /* Additional section information */
    u64 addralign; /* Section alignment */
    u64 entsize;   /* Entry size if section holds table */
};

// size of the ELF "magic number" in bytes
constexpr u8 IDENT_SIZE = 16;

// this is supposed to be equal to what might be commonly named Elf64_Ehdr
extern "C" struct ElfHeader {
    u8 ident[IDENT_SIZE]; /* ELF "magic number" */
    u16 type;
    u16 machine;
    u32 version;
    u64 entry; /* Entry point virtual address */
    u64 phoff; /* Program header table file offset */
    u64 shoff; /* Section header table file offset */
    u32 flags;
    u16 ehsize;
    u16 phentsize;
    u16 phnum;
    u16 shentsize;
    u16 shnum;
    u16 shstrndx;
};

enum class [[nodiscard]] ParseStringTableStatus : u8_c {
    Ok = 0,
    InvalidInput,
};

ParseStringTableStatus parse_string_table(Slice<u8> data,
                                          SliceMut<Slice<u8>> out) {
    if (out.len() == 0) {
        return ParseStringTableStatus::Ok;
    }

    u8 starting_byte = 0xaa;
    if (SliceStatus::Ok != data.read(0, &starting_byte)) {
        return ParseStringTableStatus::InvalidInput;
    }

    if (starting_byte != 0) {
        // first byte is defined to be the null byte.
        return ParseStringTableStatus::InvalidInput;
    }

    bool overflow = false;
    u64 const n_expected_nulls = add_u64(out.len(), 1, &overflow);
    if (overflow) {
        return ParseStringTableStatus::InvalidInput;
    }

    if (data.count(0) != n_expected_nulls) {
        return ParseStringTableStatus::InvalidInput;
    }

    u64 offset = 1;
    for (u64 i = 0; i < out.len(); i++) {
        u64 const len =
            __builtin_strlen((char const *)data.slice_at_unsafe(offset).ptr());
        out.write_unsafe(i, data.slice_unsafe(offset, len));
        offset = add_unsafe_u64(offset, add_unsafe_u64(len, 1));
    }

    return ParseStringTableStatus::Ok;
}

struct File {
    ElfHeader const *m_header;
    Slice<Slice<u8>> m_section_names;
    Slice<SectionHeader> m_section_headers;

    enum class [[nodiscard]] InitStatus : u8_c {
        Ok = 0,
        Nullptr,
        InvalidInput,
        UnexpectedErr,
        OutOfMemory,
    };

    static InitStatus init(Alloc *alloc, Slice<u8> buf, File *out) {
        if (out == nullptr || alloc == nullptr) {
            return InitStatus::Nullptr;
        }

        Slice<u8> header_bytes;
        if (SliceStatus::Ok != buf.slice(0, sizeof(ElfHeader), &header_bytes)) {
            return InitStatus::InvalidInput;
        }

        Slice<ElfHeader> header_list;
        if (SliceCastStatus::Ok != header_bytes.cast<ElfHeader>(&header_list)) {
            return InitStatus::UnexpectedErr;
        }

        if (header_list.len() != 1) {
            return InitStatus::UnexpectedErr;
        }

        ElfHeader const *header = nullptr;
        if (SliceStatus::Ok != header_list.offset(0, &header)) {
            return InitStatus::UnexpectedErr;
        }

        if (header == nullptr) {
            return InitStatus::UnexpectedErr;
        }

        if (header->shentsize != sizeof(SectionHeader)) {
            return InitStatus::InvalidInput;
        }

        bool overflow = false;
        u64 const section_headers_nbytes =
            mul_u64(sizeof(SectionHeader), header->shnum, &overflow);
        if (overflow) {
            return InitStatus::InvalidInput;
        }

        Slice<u8> section_header_bytes;
        if (SliceStatus::Ok != buf.slice(header->shoff, section_headers_nbytes,
                                         &section_header_bytes)) {
            return InitStatus::InvalidInput;
        }

        Slice<SectionHeader> section_headers;
        if (SliceCastStatus::Ok !=
            section_header_bytes.cast<SectionHeader>(&section_headers)) {
            return InitStatus::UnexpectedErr;
        }

        if (section_headers.len() != header->shnum) {
            return InitStatus::UnexpectedErr;
        }

        SectionHeader const *names_header = nullptr;
        if (SliceStatus::Ok !=
            section_headers.offset(header->shstrndx, &names_header)) {
            return InitStatus::InvalidInput;
        }

        SliceMut<Slice<u8>> section_names;
        AllocStatus const alloc_status =
            alloc->alloc<Slice<u8>>(header->shnum, &section_names);
        switch (alloc_status) {
        case AllocStatus::Ok: {
            break;
        }
        case AllocStatus::OutOfMemory: {
            return InitStatus::OutOfMemory;
        }
        default: {
            return InitStatus::UnexpectedErr;
        }
        }

        Slice<u8> section_names_data;
        if (SliceStatus::Ok != buf.slice(names_header->offset,
                                         names_header->size,
                                         &section_names_data)) {
            return InitStatus::InvalidInput;
        }

        if (ParseStringTableStatus::Ok !=
            parse_string_table(section_names_data, section_names)) {
            return InitStatus::InvalidInput;
        }

        *out = File{
            .m_section_names = section_names.as_const(),
            .m_section_headers = section_headers,
            .m_header = header,
        };
        return InitStatus::Ok;
    }
};

} // namespace elf

} // namespace zx
