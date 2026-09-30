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

    u64 const n_expected_nulls = out.len();
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

    u64 get_section_index_panic(Slice<u8> name) const {
        Slice<Slice<u8>> section_names = this->m_section_names;

        for (u64 i = 0; i < section_names.len(); i++) {
            Slice<u8> nm = section_names.read_unsafe(i);

            if (nm.equals(name)) {
                return i;
            }
        }

        panic();
    }

    // section data of the section at given index, panics if unable
    Slice<u8> get_section_data_panic(u64 idx) const {
        u64 const i = idx;
        SectionHeader const *sh = nullptr;
        assert(SliceStatus::Ok == this->m_section_headers.offset(i, &sh));
        Slice<u8> out;
        assert(SliceStatus::Ok ==
               this->m_buf.slice(sh->offset, sh->size, &out));
        return out;
    }

    // section header of the section at given index, panics if unable
    SectionHeader const *get_section_header_panic(u64 idx) const {
        u64 const i = idx;
        SectionHeader const *sh = nullptr;
        assert(SliceStatus::Ok == this->m_section_headers.offset(i, &sh));
        return sh;
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

// CIE.return_address_register in .eh_frame is defined to be 16
// on linux x86_64.
constexpr u8 RETURN_ADDRESS_REGISTER = 16;

// Dwarf Call Frame Instruction Kinds
enum class InstrKind : u8_c {
    AdvanceLoc,
    Offset,
    Restore,
    // # Don't need to parse anything into Nop, just discard it when decoding.
    // Nop,
    SetLoc,
    AdvanceLoc1,
    AdvanceLoc2,
    AdvanceLoc4,
    OffsetExtended,
    RestoreExtended,
    Undefined,
    SameValue,
    Register,
    RememberState,
    RestoreState,
    DefCfa,
    DefCfaRegister,
    DefCfaOffset,
    DefCfaExpression,
    Expression,
    OffsetExtendedSf,
    DefCfaSf,
    DefCfaOffsetSf,
    ValOffset,
    ValOffsetSf,
    ValExpression,
    LoUser,
    HiUser,
};

struct Instr {
    // Index into the Instructions.values array.
    //
    // Some kinds of instructions might read multiple values past that index
    // instead of just reading the value that is at that index.
    //
    // Some kinds of instructions don't need any separate value and use only the
    // supplement field so the value of idx will be set to U32_MAX.
    u32 idx;
    InstrKind kind;
    // used for instructions that keep meaningful values in low 6 bits.
    // Namely advance_loc, offset, restore instructions.
    u8 supplement;
};

struct Instructions {
    Slice<Instr> instr;
    Slice<u64> values;

    static Instructions parse_panic(Alloc *alloc, Slice<u8> buf) {
        u64 const max_num_instr = buf.len();

        bool overflow = false;
        u64 const max_num_values = mul_u64(buf.len(), 2, &overflow);
        assert(!overflow);

        SliceMut<Instr> instr;
        assert(AllocStatus::Ok == alloc->alloc<Instr>(max_num_instr, &instr));

        SliceMut<u64> values;
        assert(AllocStatus::Ok == alloc->alloc<u64>(max_num_values, &values));

        u64 num_instr = 0;
        u64 num_vals = 0;
        u64 offset = 0;
        while (offset < buf.len()) {
            u8 const byte = buf.read_unsafe(offset);
            offset = add_unsafe_u64(offset, 1);
            u8 const high2 = (byte & 0b11000000) >> 6;
            u8 const low6 = byte & 0b00111111;

            switch (high2) {
            // advance_loc
            case u8{0x1}: {
                break;
            }
            // offset
            case u8{0x2}: {
                break;
            }
            // restore
            case u8{0x3}: {
                break;
            }
            // other
            case u8{0}: {
                switch (low6) {
                // Nop:
                case u8{0}: {
                    // don't push it as it is meaningless.
                    break;
                }
                // set_loc
                case u8{0x01}: {
                    break;
                }
                // advance_loc1
                case u8{0x02}: {
                    break;
                }
                // advance_loc2
                case u8{0x03}: {
                    break;
                }
                // advance_loc4
                case u8{0x04}: {
                    break;
                }
                // offset_extended
                case u8{0x05}: {
                    break;
                }
                // restore_extended
                case u8{0x06}: {
                    break;
                }
                // undefined
                case u8{0x07}: {
                    break;
                }
                // same_value
                case u8{0x08}: {
                    break;
                }
                // register
                case u8{0x09}: {
                    break;
                }
                // remember_state
                case u8{0x0a}: {
                    break;
                }
                // restore_state
                case u8{0x0b}: {
                    break;
                }
                // def_cfa
                case u8{0x0c}: {
                    break;
                }
                // def_cfa_register
                case u8{0x0d}: {
                    break;
                }
                // def_cfa_offset
                case u8{0x0e}: {
                    break;
                }
                // def_cfa_expression
                case u8{0x0f}: {
                    break;
                }
                // expression
                case u8{0x10}: {
                    break;
                }
                // offset_extended_sf
                case u8{0x11}: {
                    break;
                }
                // def_cfa_sf
                case u8{0x12}: {
                    break;
                }
                // def_cfa_offset_sf
                case u8{0x13}: {
                    break;
                }
                // val_offset
                case u8{0x14}: {
                    break;
                }
                // val_offset_sf
                case u8{0x15}: {
                    break;
                }
                // val_expression
                case u8{0x16}: {
                    break;
                }
                // lo_user
                case u8{0x1c}: {
                    break;
                }
                // hi_user
                case u8{0x3f}: {
                    break;
                }
                default: {
                    panic();
                    break;
                }
                }
                break;
            }
            default: {
                panic();
                break;
            }
            }
        }

        return Instructions{
            .instr = instr.as_const().slice_unsafe(0, num_instr),
            .values = values.as_const().slice_unsafe(0, num_vals),
        };
    }
};

struct EhFrame {
    struct CIE {
        u64 code_alignment_factor;
        i64 data_alignment_factor;
        Instructions instr;
    };

    struct FDE {
        CIE const *cie;
        u64 pc_begin;
        u64 pc_range;
        Instructions instr;
    };

    Slice<FDE> fd_entries;

    static CIE parse_cie_panic(Alloc *alloc, Slice<u8> buf, u64 *offset) {
        bool overflow = false;

        u64 offs = *offset;

        u32 length = 0;
        assert(SliceStatus::Ok == buf.read_unaligned<u32>(offs, &length));
        // This can't happen as that (0xff..) is a marker defined to indicate
        // this section is in 64bit dwarf format. And we expect eh_frame to
        // always be in the 32bit format.
        assert(length != U32_MAX);
        assert(length <= buf.len());

        offs = add_u64(offs, sizeof(u32), &overflow);
        assert(!overflow);

        // check that reading this CIE won't overflow the buffer.
        assert((buf.len() - length) >= offs);

        u64 const cie_size = add_u64(length, sizeof(u32), &overflow);
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

        // This block of code just asserts that the augmentation string is equal
        // to "zR". this means each FDE will have a single byte in their
        // augmentation data that will signify how pointers are encoded in that
        // FDE.
        //
        // We assume that this will be the case for all CIE so we just assert it
        // instead of conditionally parsing.
        {
            u64 const needed_len = add_u64(offs, 3, &overflow);
            assert(!overflow);
            assert(buf.len() >= needed_len);
            assert(buf.read_unsafe(offs) == 'z');
            assert(buf.read_unsafe(offs + 1) == 'R');
            assert(buf.read_unsafe(offs + 2) == '\0');
            offs += 3;
        }

        u64 code_alignment_factor = 0;
        assert(leb128::DecodeStatus::Ok ==
               leb128::decode_u64(buf, &offs, &code_alignment_factor));
        assert(code_alignment_factor != 0);

        i64 data_alignment_factor = 0;
        assert(leb128::DecodeStatus::Ok ==
               leb128::decode_i64(buf, &offs, &data_alignment_factor));
        assert(data_alignment_factor != 0);

        u8 return_address_register = 0;
        assert(SliceStatus::Ok == buf.read(offs, &return_address_register));
        assert(RETURN_ADDRESS_REGISTER == return_address_register);
        offs = add_u64(offs, sizeof(u8), &overflow);
        assert(!overflow);

        u64 aug_data_len = 0;
        assert(leb128::DecodeStatus::Ok ==
               leb128::decode_u64(buf, &offs, &aug_data_len));
        assert(aug_data_len == 1);

        u8 fde_ptr_enc = 0;
        assert(SliceStatus::Ok == buf.read(offs, &fde_ptr_enc));
        offs = add_u64(offs, sizeof(u8), &overflow);
        assert(!overflow);

        assert((fde_ptr_enc & EH_VALUE_MASK) == EH_VALUE_SDATA4);
        assert((fde_ptr_enc & EH_APP_MASK) == EH_APP_PCREL);

        u64 const diff = sub_unsafe_u64(offs, *offset);
        assert(cie_size >= diff);

        u64 num_bytes_left = sub_unsafe_u64(cie_size, diff);
        Slice<u8> instructions_raw;
        assert(SliceStatus::Ok ==
               buf.slice(offs, num_bytes_left, &instructions_raw));

        Instructions instr = Instructions::parse_panic(alloc, instructions_raw);

        *offset = add_u64(*offset, cie_size, &overflow);
        assert(!overflow);

        return CIE{
            .code_alignment_factor = code_alignment_factor,
            .data_alignment_factor = data_alignment_factor,
            .instr = instr,
        };
    }

    static FDE parse_fde_panic(Alloc *alloc, i64 decode_pcrel, CIE const *cie,
                               Slice<u8> buf, u64 *offset) {
        bool overflow = false;

        u64 offs = *offset;

        u32 length = 0;
        assert(SliceStatus::Ok == buf.read_unaligned<u32>(offs, &length));
        // This can't happen as that (0xff..) is a marker defined to indicate
        // this section is in 64bit dwarf format. And we expect eh_frame to
        // always be in the 32bit format.
        assert(length != U32_MAX);
        assert(length <= buf.len());

        offs = add_u64(offs, sizeof(u32), &overflow);
        assert(!overflow);

        // check that reading this FDE won't overflow the buffer.
        assert((buf.len() - length) >= offs);

        u64 const fde_size = add_u64(length, sizeof(u32), &overflow);
        assert(!overflow);

        // We know this is a SDATA4 encoded and PCREL applied value because we
        // assert this when parsing CIE.
        i32 pc_begin_encoded = 0;
        assert(SliceStatus::Ok ==
               buf.read_unaligned<i32>(offs, &pc_begin_encoded));
        u64 pc_begin = i64_to_u64(
            add_i64(decode_pcrel, pc_begin_encoded, &overflow), &overflow);
        assert(!overflow);

        offs = add_u64(offs, sizeof(i32), &overflow);
        assert(!overflow);

        u32 pc_range = 0;
        assert(SliceStatus::Ok == buf.read_unaligned<u32>(offs, &pc_range));
        offs = add_u64(offs, sizeof(u32), &overflow);
        assert(!overflow);

        u64 const diff = sub_unsafe_u64(offs, *offset);
        assert(fde_size >= diff);

        u64 num_bytes_left = sub_unsafe_u64(fde_size, diff);
        Slice<u8> instructions_raw;
        assert(SliceStatus::Ok ==
               buf.slice(offs, num_bytes_left, &instructions_raw));

        Instructions instr = Instructions::parse_panic(alloc, instructions_raw);

        *offset = add_u64(*offset, fde_size, &overflow);
        assert(!overflow);

        return FDE{
            .cie = cie,
            .pc_begin = pc_begin,
            .pc_range = pc_range,
            .instr = instr,
        };
    }

    // Parses .eh_frame section in `buf`. Panics if there is a problem.
    // This resulting structure's lifetime is tied to the allocator that was
    // passed in.
    static EhFrame init_panic(Alloc *alloc, SectionHeader const *header,
                              Slice<u8> buf) {
        bool cast_fail = false;

        // Convert these to i64 so we can do signed arithmetic
        //
        i64 const sh_addr = u64_to_i64(header->addr, &cast_fail);
        assert(!cast_fail);

        i64 const sh_offset = u64_to_i64(header->offset, &cast_fail);
        assert(!cast_fail);

        // address_of_field = sh_addr + (field_file_offset - sh_offset)
        // and address_of_field is used for decoding EH_APP_PCREL values.
        // we can just add what we calculated here to the encoded value to
        // decode it.
        i64 const decode_pcrel = wrapping_sub_i64(sh_addr, sh_offset);

        if (buf.len() == 0) {
            return EhFrame{
                .fd_entries = Slice<FDE>::init_dangling(),
            };
        }

        u64 offset = 0;

        while (offset < buf.len()) {
            // length == 0 means we have found the TERMINATOR
            u32 length = 0;
            assert(SliceStatus::Ok == buf.read_unaligned<u32>(offset, &length));
            if (length == 0) {
                break;
            }

            CIE *cie = nullptr;
            assert(AllocStatus::Ok == alloc->create<CIE>(&cie));
            *cie = parse_cie_panic(alloc, buf, &offset);

            // parse fdes
            while (offset < buf.len()) {
                // length == 0 means we have found the TERMINATOR
                length = 0;
                assert(SliceStatus::Ok ==
                       buf.read_unaligned<u32>(offset, &length));
                if (length == 0) {
                    break;
                }

                bool overflow = false;
                u64 cie_id_offset = add_u64(offset, sizeof(u32), &overflow);
                assert(!overflow);
                u32 cie_id = 0;
                assert(SliceStatus::Ok ==
                       buf.read_unaligned<u32>(cie_id_offset, &cie_id));
                // This means this is not a fde but a cie, just go out to the
                // outer loop so we can parse the cie. And then we will come
                // back here to parse the FDEs of that CIE. This is just a
                // scuffed state machine.
                if (cie_id == 0) {
                    break;
                }

                FDE const fde =
                    parse_fde_panic(alloc, decode_pcrel, cie, buf, &offset);
                (void)fde;
            }
        }

        return EhFrame{
            .fd_entries = Slice<FDE>::init_dangling(),
        };
    }
};

} // namespace elf

} // namespace zx
