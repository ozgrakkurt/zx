export module zx:elf;
import :def;
import :assert;
import :arith;
import :slice;
import :alloc;
import :panic;
import :leb128;

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

void parse_string_table_panic_(Slice<u8> data, SliceMut<Slice<u8>> out) {
    if (out.len() == 0) {
        return;
    }

    u8 starting_byte = 0xaa;
    assert(SliceStatus::Ok == data.read(0, &starting_byte));
    assert(starting_byte == 0);

    bool overflow = false;
    u64 const n_expected_nulls = out.len();
    assert(!overflow);
    assert(data.count(0) == n_expected_nulls);

    u64 offset = 0;
    for (u64 i = 0; i < out.len(); i++) {
        u64 const len =
            __builtin_strlen((char const *)data.slice_at_unsafe(offset).ptr());
        out.write_unsafe(i, data.slice_unsafe(offset, len));
        offset = add_unsafe_u64(offset, add_unsafe_u64(len, 1));
    }

    return;
}

struct File {
    Slice<u8> m_buf;
    ElfHeader const *m_header;
    Slice<Slice<u8>> m_section_names;
    Slice<SectionHeader> m_section_headers;

    // Parses the buffer as an elf file, panics if there is a problem.
    // The resulting structure's lifetime is tied to `alloc` and `buf`.
    static File init_panic(Alloc *alloc, Slice<u8> buf) {
        assert(alloc != nullptr);

        Slice<u8> header_bytes;
        assert(SliceStatus::Ok ==
               buf.slice(0, sizeof(ElfHeader), &header_bytes));

        Slice<ElfHeader> header_list;
        assert(__builtin_is_aligned(header_bytes.ptr(), alignof(ElfHeader)));
        assert(SliceCastStatus::Ok ==
               header_bytes.cast<ElfHeader>(&header_list));
        assert(header_list.len() == 1);

        ElfHeader const *header = nullptr;
        assert(SliceStatus::Ok == header_list.offset(0, &header));
        assert(header != nullptr);

        assert(header->shentsize == sizeof(SectionHeader));

        bool overflow = false;
        u64 const section_headers_nbytes =
            mul_u64(sizeof(SectionHeader), header->shnum, &overflow);
        assert(!overflow);

        Slice<u8> section_header_bytes;
        assert(SliceStatus::Ok == buf.slice(header->shoff,
                                            section_headers_nbytes,
                                            &section_header_bytes));

        Slice<SectionHeader> section_headers;
        assert(SliceCastStatus::Ok ==
               section_header_bytes.cast<SectionHeader>(&section_headers));
        assert(section_headers.len() == header->shnum);

        Slice<u8> dummy;
        for (u64 i = 0; i < section_headers.len(); i++) {
            SectionHeader const *hdr = section_headers.offset_unsafe(i);
            assert(SliceStatus::Ok ==
                   buf.slice(hdr->offset, hdr->size, &dummy));
        }

        SectionHeader const *names_header = nullptr;
        assert(SliceStatus::Ok ==
               section_headers.offset(header->shstrndx, &names_header));

        SliceMut<Slice<u8>> section_names;
        assert(AllocStatus::Ok ==
               alloc->alloc<Slice<u8>>(header->shnum, &section_names));

        Slice<u8> section_names_data;
        assert(SliceStatus::Ok == buf.slice(names_header->offset,
                                            names_header->size,
                                            &section_names_data));

        parse_string_table_panic_(section_names_data, section_names);
        assert(section_names.len() == section_headers.len());

        return File{
            .m_section_names = section_names.as_const(),
            .m_section_headers = section_headers,
            .m_header = header,
            .m_buf = buf,
        };
    }

    // returns the named section, panics if unable
    Slice<u8> get_section_panic(Slice<u8> name) const {
        Slice<Slice<u8>> section_names = this->m_section_names;

        for (u64 i = 0; i < section_names.len(); i++) {
            Slice<u8> nm = section_names.read_unsafe(i);

            if (nm.equals(name)) {
                SectionHeader const *sh =
                    this->m_section_headers.offset_unsafe(i);
                Slice<u8> out;
                assert(SliceStatus::Ok ==
                       this->m_buf.slice(sh->offset, sh->size, &out));
                return out;
            }
        }

        panic();
    }
};

constexpr u8 EH_VALUE_MASK = 0x0f;
constexpr u8 EH_APP_MASK = 0xf0;

constexpr u8 EH_VALUE_OMIT = 0xff;
constexpr u8 EH_VALUE_ULEB128 = 0x01;
constexpr u8 EH_VALUE_UDATA2 = 0x02;
constexpr u8 EH_VALUE_UDATA4 = 0x03;
constexpr u8 EH_VALUE_UDATA8 = 0x04;
constexpr u8 EH_VALUE_SLEB128 = 0x09;
constexpr u8 EH_VALUE_SDATA2 = 0x0A;
constexpr u8 EH_VALUE_SDATA4 = 0x0B;
constexpr u8 EH_VALUE_SDATA8 = 0x0C;

constexpr u8 EH_APP_ABSPTR = 0x00;
constexpr u8 EH_APP_PCREL = 0x10;
constexpr u8 EH_APP_DATAREL = 0x30;
constexpr u8 EH_APP_OMIT = 0xff;

struct EhFrame {
    enum class InstructionKind: u8_c {
        Wut = 0,  
    };
    
    struct InstructionPtr {
        u32 index;
        InstructionKind kind;  
    };

    struct Instructions {
        Slice<InstructionPtr> kinds;
        // slice per different kind
    };
    
    struct CIE {
        u64 code_alignment_factor;
        u64 data_alignment_factor;
        u8 return_address_register;
        Slice<u8> initial_instructions;
    };

    struct FDE {
        CIE const* cie_pointer;
        u64 pc_begin;
        u64 pc_range;
        Slice<u8> call_frame_instructions;
    };

    Slice<FDE> fd_entries;

    static CIE parse_cie_panic(Slice<u8> buf, u64* offset) {
        bool overflow = false;

        u64 offs = *offset;

        u32 is_64bit = 0;
        assert(SliceStatus::Ok == buf.read_unaligned<u32>(offs, &is_64bit));
        assert(is_64bit == ~u32{0});

        offs = add_u64(offs, sizeof(u32), &overflow);
        assert(!overflow);

        u64 length = 0;
        assert(SliceStatus::Ok == buf.read_unaligned<u64>(offs, &length));

        offs = add_u64(offs, sizeof(u64), &overflow);
        assert(!overflow);

        u64 const cie_size = add_u64(length, 12, &overflow);
        assert(!overflow);

        u32 cie_id = 0xff;
        assert(SliceStatus::Ok == buf.read_unaligned<u32>(offs, &cie_id));
        assert(cie_id == 0);

        offs = add_u64(offs, sizeof(u32), &overflow);
        assert(!overflow);

        u8 version = 0;
        assert(SliceStatus::Ok == buf.read(offs, &version));
        assert(version == 1);

        offs = add_u64(offs, sizeof(u8), &overflow);
        assert(!overflow);

        // augmentation string should be empty so it should consist of a single NULL byte.
        u8 should_be_null = 0xff;
        assert(SliceStatus::Ok == buf.read(offs, &should_be_null));
        assert(should_be_null == 0);

        offs = add_u64(offs, sizeof(u8), &overflow);
        assert(!overflow);

        u64 code_alignment_factor = 0;
        assert(leb128::DecodeStatus::Ok == leb128::decode_u64(buf, &offs, &code_alignment_factor));
        assert(code_alignment_factor != 0);

        i64 data_alignment_factor = 0;
        assert(leb128::DecodeStatus::Ok == leb128::decode_i64(buf, &offs, &data_alignment_factor));
        assert(data_alignment_factor != 0);

        u8 return_address_register = 0;
        assert(SliceStatus::Ok == buf.read(offs, &return_address_register));

        u64 diff = sub_unsafe_u64(*offset, offs);
        assert(length >= diff);

        // u64 num_bytes_left = sub_unsafe_u64(cie_size, diff);
        // TODO: parse instructions

        *offset = add_u64(*offset, cie_size, &overflow);
        assert(!overflow);

        return CIE {};
    }

    // Parses .eh_frame section in `buf`. Panics if there is a problem.
    // This resulting structure's lifetime is tied to the allocator that was passed in.
    static EhFrame init_panic(Alloc *alloc, Slice<u8> buf) {
        if (buf.len() == 0) {
            return EhFrame {
                .fd_entries = Slice<FDE>::init_dangling(),
            };
        }

        (void)alloc;

        u64 offset = 0;

        while (offset <= buf.len()) {
            CIE const cie = parse_cie_panic(buf, &offset);
            (void)cie;
        }
        
        return EhFrame {
            .fd_entries = Slice<FDE>::init_dangling(),
        };
    }
};

} // namespace elf

} // namespace zx
