export module zx:elf;
import :def;
import :assert;
import :arith;
import :slice;
import :alloc;
import :panic;
import :leb128;
import :cursor;
import :list;

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

void parse_section_name_table_panic_(Slice<SectionHeader> headers,
                                     Slice<u8> data, SliceMut<Slice<u8>> out) {
    assert(headers.len() == out.len());
    if (out.len() == 0) {
        return;
    }

    u8 starting_byte = 0xaa;
    assert(SliceStatus::Ok == data.read(0, &starting_byte));
    assert(starting_byte == 0);

    u64 last_null_pos = 0;
    for (u64 i = 0; i < data.len(); i++) {
        if (data.read_unsafe(i) == 0) {
            last_null_pos = i;
        }
    }

    for (u64 i = 0; i < headers.len(); i++) {
        u64 const pos = headers.offset_unsafe(i)->name;
        assert(pos <= last_null_pos);
        out.write_unsafe(
            i,
            data.slice_unsafe(
                pos, __builtin_strlen((char const *)data.offset_unsafe(pos))));
    }
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
        assert(SliceCastStatus::Ok ==
               header_bytes.cast<ElfHeader>(&header_list));
        assert(header_list.len() == 1);

        ElfHeader const *header = nullptr;
        assert(SliceStatus::Ok == header_list.offset(0, &header));
        assert(header != nullptr);

        assert(header->ehsize == sizeof(ElfHeader));
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

        parse_section_name_table_panic_(section_headers, section_names_data,
                                        section_names);
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

// section_addr is the address of the elf section at runtime. can be found in
// elf_section_header.addr field. field_offset is the offset of the field inside
// the section. encoded_val is the pcrel encoded EH_FRAME value to be decoded.
u64 decode_pcrel_panic_(u64 section_addr, u64 field_offset, i32 encoded_val) {
    i64 const extended_encoded = encoded_val;
    return wrapping_add_u64(wrapping_add_u64(section_addr, field_offset),
                            (u64)extended_encoded);
}

// Dwarf Call Frame Instruction Kinds
enum class Instr : u8_c {
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
    GnuArgsSize,
    GnuNegativeOffsetExtended,
};

struct Instructions {
    Slice<Instr> instr;
    Slice<u8> data;

    static void parse_one_panic_(u64 section_addr, List<Instr> *instr_list,
                                 WriteCursor *data_writer,
                                 Cursor *input_cursor) {
        u8 const byte = input_cursor->read_panic<u8>();

        u8 const high2 = (byte & 0b11000000) >> 6;
        u8 const low6 = byte & 0b00111111;

        switch (high2) {
        // advance_loc
        case u8{0x1}: {
            instr_list->push_panic(Instr::AdvanceLoc);
            data_writer->write_panic<u8>(low6);
            break;
        }
        // offset
        case u8{0x2}: {
            u64 off = 0;
            assert(leb128::DecodeStatus::Ok ==
                   input_cursor->read_uleb128(&off));

            instr_list->push_panic(Instr::Offset);
            data_writer->write_panic<u8>(low6);
            data_writer->write_panic<u64>(off);

            break;
        }
        // restore
        case u8{0x3}: {
            data_writer->write_panic<u8>(low6);
            instr_list->push_panic(Instr::Restore);

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
                u64 const field_offset = input_cursor->offset();
                i32 const addr_encoded = input_cursor->read_panic<i32>();
                u64 const addr = decode_pcrel_panic_(section_addr, field_offset,
                                                     addr_encoded);

                data_writer->write_panic<u64>(addr);

                instr_list->push_panic(Instr::SetLoc);

                break;
            }
            // advance_loc1
            case u8{0x02}: {
                u8 const delta = input_cursor->read_panic<u8>();

                data_writer->write_panic<u8>(delta);
                instr_list->push_panic(Instr::AdvanceLoc1);

                break;
            }
            // advance_loc2
            case u8{0x03}: {
                u16 const delta = input_cursor->read_panic<u16>();

                data_writer->write_panic<u16>(delta);
                instr_list->push_panic(Instr::AdvanceLoc2);

                break;
            }
            // advance_loc4
            case u8{0x04}: {
                u32 const delta = input_cursor->read_panic<u32>();
                data_writer->write_panic<u32>(delta);

                instr_list->push_panic(Instr::AdvanceLoc4);

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
            // GNU_args_size
            case u8{0x2e}: {
                break;
            }
            // GNU_negative_offset_extended
            case u8{0x2f}: {
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

    static Instructions parse_panic(Alloc *alloc, Alloc *tmp_alloc,
                                    u64 section_addr,
                                    u64 instructions_end_offset,
                                    Cursor *input_cursor) {
        if (input_cursor->offset() == instructions_end_offset) {
            return Instructions{
                .instr = Slice<Instr>::init_dangling(),
                .data = Slice<u8>::init_dangling(),
            };
        }

        u64 const tmp_alloc_initial_offset = tmp_alloc->offset();

        constexpr u64 max_num_instructions = u64{1} << 12;
        SliceMut<Instr> instr;
        assert(AllocStatus::Ok ==
               tmp_alloc->alloc<Instr>(max_num_instructions, &instr));

        constexpr u64 max_data_size = u64{1} << 16;
        SliceMut<u8> data;
        assert(AllocStatus::Ok == tmp_alloc->alloc<u8>(max_data_size, &data));

        List<Instr> instr_list = List<Instr>::init(instr);
        WriteCursor data_writer = WriteCursor::init(data);

        while (input_cursor->offset() < instructions_end_offset) {
            parse_one_panic_(section_addr, &instr_list, &data_writer,
                             input_cursor);
        }
        assert(input_cursor->offset() == instructions_end_offset);

        // move instr to real alloc
        SliceMut<Instr> instr_out;
        assert(AllocStatus::Ok ==
               alloc->alloc<Instr>(instr_list.n_written(), &instr_out));
        assert(SliceMemcpyStatus::Ok ==
               instr_out.memcpy_from(instr_list.written()));

        // move data to real alloc
        SliceMut<u8> data_out;
        assert(AllocStatus::Ok ==
               alloc->alloc<u8>(data_writer.offset(), &data_out));
        assert(SliceMemcpyStatus::Ok ==
               data_out.memcpy_from(data_writer.written()));

        // dealloc temporary memory we used
        assert(Alloc::ResizeStatus::Ok ==
               tmp_alloc->resize(tmp_alloc_initial_offset));

        return Instructions{
            .instr = instr_out.as_const(),
            .data = data_out.as_const(),
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

    static CIE parse_cie_panic(Alloc *alloc, Alloc *tmp_alloc, u64 end_offset,
                               u64 section_addr, Cursor *input_cursor) {
        u8 const version = input_cursor->read_panic<u8>();
        assert(version == 1);

        // This block of code just asserts that the augmentation string is equal
        // to "zR". this means the CIE will have a single byte in it's
        // augmentation data that will signify how pointers are encoded in
        // associated FDEs.
        //
        // We assume that this will be the case for all CIE so we just assert it
        // instead of conditionally parsing the augmentation string.
        {
            assert(input_cursor->read_panic<u8>() == 'z');
            assert(input_cursor->read_panic<u8>() == 'R');
            assert(input_cursor->read_panic<u8>() == '\0');
        }

        u64 const code_alignment_factor = input_cursor->read_uleb128_panic();
        assert(code_alignment_factor != 0);

        i64 const data_alignment_factor = input_cursor->read_sleb128_panic();
        assert(data_alignment_factor != 0);

        u8 const return_address_register = input_cursor->read_panic<u8>();
        assert(RETURN_ADDRESS_REGISTER == return_address_register);

        u64 const aug_data_len = input_cursor->read_uleb128_panic();
        assert(aug_data_len == 1);

        u8 const fde_ptr_enc = input_cursor->read_panic<u8>();
        // Assert that we have the expected encoding.
        {
            assert((fde_ptr_enc & EH_VALUE_MASK) == EH_VALUE_SDATA4);
            assert((fde_ptr_enc & EH_APP_MASK) == EH_APP_PCREL);
        }

        Instructions instr = Instructions::parse_panic(
            alloc, tmp_alloc, section_addr, end_offset, input_cursor);

        assert(input_cursor->offset() == end_offset);
        return CIE{
            .code_alignment_factor = code_alignment_factor,
            .data_alignment_factor = data_alignment_factor,
            .instr = instr,
        };
    }

    static FDE parse_fde_panic(Alloc *alloc, Alloc *tmp_alloc, u64 end_offset,
                               u64 section_addr, CIE const *cie,
                               Cursor *input_cursor) {
        // We know this is a SDATA4 encoded and PCREL applied value because we
        // assert this when parsing CIE.
        u64 const pc_begin_field_offset = input_cursor->offset();
        i32 const pc_begin_encoded = input_cursor->read_panic<i32>();
        u64 const pc_begin = decode_pcrel_panic_(
            section_addr, pc_begin_field_offset, pc_begin_encoded);

        u32 const pc_range = input_cursor->read_panic<u32>();

        u64 const aug_data_len = input_cursor->read_uleb128_panic();
        assert(aug_data_len == 0);

        Instructions instr = Instructions::parse_panic(
            alloc, tmp_alloc, section_addr, end_offset, input_cursor);

        assert(input_cursor->offset() == end_offset);
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
    static EhFrame init_panic(Alloc *alloc, Alloc *tmp_alloc,
                              SectionHeader const *header,
                              Slice<u8> input_bytes) {
        if (input_bytes.len() == 0) {
            return EhFrame{
                .fd_entries = Slice<FDE>::init_dangling(),
            };
        }

        Cursor input_cursor = Cursor::init(input_bytes);

        u64 const tmp_alloc_initial_offset = tmp_alloc->offset();

        constexpr u64 max_num_fdes = u64{1} << 14;

        SliceMut<FDE> fdes;
        assert(AllocStatus::Ok == tmp_alloc->alloc<FDE>(max_num_fdes, &fdes));

        List<FDE> fde_list = List<FDE>::init(fdes);

        CIE *cie = nullptr;
        u64 cie_offset = 0;
        for (;;) {
            u64 const offset = input_cursor.offset();
            // length is common between CIE and FDE so we parse it regardless
            u32 const length = input_cursor.read_panic<u32>();

            // This can't happen as that (0xff..) is a marker defined to
            // indicate this section is in 64bit dwarf format. And we expect
            // eh_frame to always be in the 32bit format.
            assert(length != U32_MAX);
            // length == 0 means we have found the TERMINATOR
            if (length == 0) {
                break;
            }

            // calculate the offset that this item is supposed to end at.
            bool overflow = false;
            u64 const end_offset =
                add_u64(input_cursor.offset(), length, &overflow);
            assert(!overflow);

            // this is called cie_id for a cie and cie_ptr for a fde.
            // it is defined to be 0 for cie and non-zero for fde so we use it
            // to differentiate between them.
            u64 const cie_id_offset = input_cursor.offset();
            u32 const cie_id = input_cursor.read_panic<u32>();

            if (cie_id == 0) {
                // parse cie
                assert(AllocStatus::Ok == alloc->create<CIE>(&cie));
                *cie = parse_cie_panic(alloc, tmp_alloc, end_offset,
                                       header->addr, &input_cursor);
                cie_offset = offset;
            } else {
                // parse fde
                assert(cie != nullptr);
                assert(cie_offset == (cie_id_offset - cie_id));

                // TODO: assert cie_id points to cie_offset
                FDE const fde =
                    parse_fde_panic(alloc, tmp_alloc, end_offset, header->addr,
                                    cie, &input_cursor);
                fde_list.push_panic(fde);
            }
        }
        assert(input_cursor.is_finished());

        // move fdes to long term alloc (non-tmp)
        SliceMut<FDE> out_fdes;
        assert(AllocStatus::Ok ==
               alloc->alloc<FDE>(fde_list.n_written(), &out_fdes));
        assert(SliceMemcpyStatus::Ok ==
               out_fdes.memcpy_from(fde_list.written()));

        // dealloc temporary memory that we used for this function
        assert(Alloc::ResizeStatus::Ok ==
               tmp_alloc->resize(tmp_alloc_initial_offset));

        return EhFrame{
            .fd_entries = out_fdes.as_const(),
        };
    }
};

} // namespace elf

} // namespace zx
