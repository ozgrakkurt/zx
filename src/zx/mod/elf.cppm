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
import :ucontext;
import :fmt;
// TODO: remove this!!
import :sys;

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
    // DefCfaExpression,
    // Expression,
    OffsetExtendedSf,
    DefCfaSf,
    DefCfaOffsetSf,
    ValOffset,
    ValOffsetSf,
    // ValExpression,
    // LoUser,
    // HiUser,
    // GnuArgsSize,
    // GnuNegativeOffsetExtended,
};

constexpr u8 REG_MAP_RAX = 0;
constexpr u8 REG_MAP_RDX = 1;
constexpr u8 REG_MAP_RCX = 2;
constexpr u8 REG_MAP_RBX = 3;
constexpr u8 REG_MAP_RSI = 4;
constexpr u8 REG_MAP_RDI = 5;
constexpr u8 REG_MAP_RBP = 6;
constexpr u8 REG_MAP_RSP = 7;
constexpr u8 REG_MAP_R8 = 8;
constexpr u8 REG_MAP_R9 = 9;
constexpr u8 REG_MAP_R10 = 10;
constexpr u8 REG_MAP_R11 = 11;
constexpr u8 REG_MAP_R12 = 12;
constexpr u8 REG_MAP_R13 = 13;
constexpr u8 REG_MAP_R14 = 14;
constexpr u8 REG_MAP_R15 = 15;
constexpr u8 REG_MAP_RETURN_ADDRESS = 16;
constexpr u8 NUM_REGS = 17;

struct RegisterRule {
    enum class Type : u8_c {
        Undefined = 0,
        SameValue,
        Offset,
        ValOffset,
        Register,
    };
    i32 offset;
    u8 reg;
    Type type;
};

struct CFARule {
    enum class Type : u8_c {
        Undefined = 0,
        Register,
    };
    i32 offset;
    u8 reg;
    Type type;
};

struct RegisterState {
    CFARule cfa;
    RegisterRule registers[NUM_REGS];
};

struct Instructions {
    Slice<Instr> m_instr;
    Slice<u8> m_data;

    static u64 advance_loc_panic_(u32 code_alignment_factor, u64 loc,
                                  u32 delta) {
        bool overflow = false;
        u32 const scaled = mul_u32(code_alignment_factor, delta, &overflow);
        u64 out = add_u64(loc, scaled, &overflow);

        assert(!overflow);

        return out;
    }

    struct RunArgs {
        u64 pc_begin;
        u64 pc_target;
        u32 code_alignment_factor;
        i32 data_alignment_factor;
        RegisterState const *cie_state;
        List<RegisterState> *stack;
        RegisterState *current_state;
    };

    void run_panic(RunArgs args) const {
        assert(args.current_state != nullptr);
        assert(args.stack != nullptr);

        Cursor cursor = Cursor::init(this->m_data);

        u64 pc = args.pc_begin;

        for (u64 i = 0; i < this->m_instr.len(); i++) {
            switch (this->m_instr.read_unsafe(i)) {
            case Instr::AdvanceLoc: {
                u8 const delta = cursor.read_panic<u8>();
                pc = advance_loc_panic_(args.code_alignment_factor, pc, delta);

                break;
            }
            case Instr::Offset: {
                u8 const regno = cursor.read_panic<u8>();
                u32 const off = cursor.read_panic<u32>();

                bool overflow = false;
                i32 const soff = u32_to_i32(off, &overflow);
                assert(!overflow);

                assert(regno < NUM_REGS);

                args.current_state->registers[regno] = RegisterRule{
                    .offset =
                        mul_i32(soff, args.data_alignment_factor, &overflow),
                    .reg = U8_MAX,
                    .type = RegisterRule::Type::Offset,
                };
                assert(!overflow);

                break;
            }
            case Instr::Restore: {
                assert(args.cie_state != nullptr);

                u8 const regno = cursor.read_panic<u8>();

                assert(regno < NUM_REGS);

                args.current_state->registers[regno] =
                    args.cie_state->registers[regno];

                break;
            }
            case Instr::SetLoc: {
                u64 const addr = cursor.read_panic<u64>();
                assert(addr > pc);
                pc = addr;

                break;
            }
            case Instr::AdvanceLoc1: {
                u8 const delta = cursor.read_panic<u8>();
                pc = advance_loc_panic_(args.code_alignment_factor, pc, delta);

                break;
            }
            case Instr::AdvanceLoc2: {
                u16 const delta = cursor.read_panic<u16>();
                pc = advance_loc_panic_(args.code_alignment_factor, pc, delta);

                break;
            }
            case Instr::AdvanceLoc4: {
                u32 const delta = cursor.read_panic<u32>();
                pc = advance_loc_panic_(args.code_alignment_factor, pc, delta);

                break;
            }
            case Instr::OffsetExtended: {
                u8 const reg = cursor.read_panic<u8>();
                u32 const off = cursor.read_panic<u32>();

                bool overflow = false;

                i32 const soff = u32_to_i32(off, &overflow);
                assert(!overflow);

                assert(reg < NUM_REGS);

                args.current_state->registers[reg] = RegisterRule{
                    .type = RegisterRule::Type::Offset,
                    .offset =
                        mul_i32(soff, args.data_alignment_factor, &overflow),
                    .reg = U8_MAX,
                };
                assert(!overflow);

                break;
            }
            case Instr::RestoreExtended: {
                u8 const reg = cursor.read_panic<u8>();

                assert(reg < NUM_REGS);

                assert(args.cie_state != nullptr);
                args.current_state->registers[reg] =
                    args.cie_state->registers[reg];

                break;
            }
            case Instr::Undefined: {
                u8 const reg = cursor.read_panic<u8>();

                assert(reg < NUM_REGS);

                __builtin_memset(&args.current_state->registers[reg], 0,
                                 sizeof(RegisterRule));

                break;
            }
            case Instr::SameValue: {
                u8 const reg = cursor.read_panic<u8>();

                assert(reg < NUM_REGS);

                args.current_state->registers[reg] = RegisterRule{
                    .type = RegisterRule::Type::SameValue,
                    .reg = U8_MAX,
                    .offset = 0,
                };

                break;
            }
            case Instr::Register: {
                u8 const reg_tgt = cursor.read_panic<u8>();
                u8 const reg_src = cursor.read_panic<u8>();

                assert(reg_tgt < NUM_REGS);
                assert(reg_src < NUM_REGS);

                args.current_state->registers[reg_tgt] = RegisterRule{
                    .type = RegisterRule::Type::Register,
                    .reg = reg_src,
                    .offset = 0,
                };

                break;
            }
            case Instr::RememberState: {
                args.stack->push_panic(*args.current_state);

                break;
            }
            case Instr::RestoreState: {
                *args.current_state = args.stack->pop_panic();

                break;
            }
            case Instr::DefCfa: {
                u8 const reg = cursor.read_panic<u8>();
                u32 const off = cursor.read_panic<u32>();

                assert(reg < NUM_REGS);

                bool overflow = false;
                args.current_state->cfa = CFARule{
                    .type = CFARule::Type::Register,
                    .reg = reg,
                    .offset = u32_to_i32(off, &overflow),
                };
                assert(!overflow);

                break;
            }
            case Instr::DefCfaRegister: {
                u8 const reg = cursor.read_panic<u8>();

                assert(reg < NUM_REGS);

                args.current_state->cfa = CFARule{
                    .type = CFARule::Type::Register,
                    .reg = reg,
                    .offset = args.current_state->cfa.offset,
                };

                break;
            }
            case Instr::DefCfaOffset: {
                u32 const off = cursor.read_panic<u32>();

                bool overflow = false;
                args.current_state->cfa = CFARule{
                    .type = CFARule::Type::Register,
                    .reg = args.current_state->cfa.reg,
                    .offset = u32_to_i32(off, &overflow),
                };
                assert(!overflow);

                break;
            }
            case Instr::OffsetExtendedSf: {
                u8 const reg = cursor.read_panic<u8>();
                i32 const off = cursor.read_panic<i32>();

                assert(reg < NUM_REGS);

                bool overflow = false;
                i32 const scaled =
                    mul_i32(off, args.data_alignment_factor, &overflow);
                assert(!overflow);

                args.current_state->registers[reg] = RegisterRule{
                    .type = RegisterRule::Type::Offset,
                    .reg = U8_MAX,
                    .offset = scaled,
                };

                break;
            }
            case Instr::DefCfaSf: {
                u8 const reg = cursor.read_panic<u8>();
                i32 const off = cursor.read_panic<i32>();

                assert(reg < NUM_REGS);

                bool overflow = false;
                i32 const scaled =
                    mul_i32(off, args.data_alignment_factor, &overflow);
                assert(!overflow);

                args.current_state->cfa = CFARule{
                    .type = CFARule::Type::Register,
                    .reg = reg,
                    .offset = scaled,
                };

                break;
            }
            case Instr::DefCfaOffsetSf: {
                i32 const off = cursor.read_panic<i32>();

                bool overflow = false;
                i32 const scaled =
                    mul_i32(off, args.data_alignment_factor, &overflow);
                assert(!overflow);

                args.current_state->cfa = CFARule{
                    .type = CFARule::Type::Register,
                    .offset = scaled,
                    .reg = args.current_state->cfa.reg,
                };

                break;
            }
            case Instr::ValOffset: {
                u8 const reg = cursor.read_panic<u8>();
                u32 const off = cursor.read_panic<u32>();

                assert(reg < NUM_REGS);

                bool overflow = false;
                i32 const scaled =
                    mul_i32(u32_to_i32(off, &overflow),
                            args.data_alignment_factor, &overflow);
                assert(!overflow);

                args.current_state->registers[reg] = RegisterRule{
                    .type = RegisterRule::Type::ValOffset,
                    .offset = scaled,
                    .reg = U8_MAX,
                };

                break;
            }
            case Instr::ValOffsetSf: {
                u8 const reg = cursor.read_panic<u8>();
                i32 const off = cursor.read_panic<i32>();

                assert(reg < NUM_REGS);

                bool overflow = false;
                i32 const scaled =
                    mul_i32(off, args.data_alignment_factor, &overflow);
                assert(!overflow);

                args.current_state->registers[reg] = RegisterRule{
                    .type = RegisterRule::Type::ValOffset,
                    .offset = scaled,
                    .reg = U8_MAX,
                };

                break;
            }
            default: {
                panic();
                break;
            }
            }

            if (pc > args.pc_target) {
                return;
            }
        }
    }

    static u32 offset_to_u32_panic_(u64 offset) {
        bool overflow = false;
        u32 const out = u64_to_u32(offset, &overflow);
        assert(!overflow);
        return out;
    }

    static i32 offset_to_i32_panic_(i64 offset) {
        bool overflow = false;
        i32 const out = i64_to_i32(offset, &overflow);
        assert(!overflow);
        return out;
    }

    static u8 register_to_u8_panic_(u64 reg) {
        static_assert(NUM_REGS < U8_MAX);
        assert(reg < NUM_REGS);
        return (u8)reg;
    }

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
            data_writer->write_panic<u32>(offset_to_u32_panic_(off));

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
                u64 const reg = input_cursor->read_uleb128_panic();
                u64 const off = input_cursor->read_uleb128_panic();

                data_writer->write_panic<u8>(register_to_u8_panic_(reg));
                data_writer->write_panic<u32>(offset_to_u32_panic_(off));

                instr_list->push_panic(Instr::OffsetExtended);

                break;
            }
            // restore_extended
            case u8{0x06}: {
                u64 const reg = input_cursor->read_uleb128_panic();

                data_writer->write_panic<u8>(register_to_u8_panic_(reg));

                instr_list->push_panic(Instr::RestoreExtended);

                break;
            }
            // undefined
            case u8{0x07}: {
                u64 const reg = input_cursor->read_uleb128_panic();

                data_writer->write_panic<u8>(register_to_u8_panic_(reg));

                instr_list->push_panic(Instr::Undefined);

                break;
            }
            // same_value
            case u8{0x08}: {
                u64 const reg = input_cursor->read_uleb128_panic();

                data_writer->write_panic<u8>(register_to_u8_panic_(reg));

                instr_list->push_panic(Instr::SameValue);

                break;
            }
            // register
            case u8{0x09}: {
                u64 const reg0 = input_cursor->read_uleb128_panic();
                u64 const reg1 = input_cursor->read_uleb128_panic();

                data_writer->write_panic<u8>(register_to_u8_panic_(reg0));
                data_writer->write_panic<u8>(register_to_u8_panic_(reg1));

                instr_list->push_panic(Instr::Register);

                break;
            }
            // remember_state
            case u8{0x0a}: {
                instr_list->push_panic(Instr::RememberState);

                break;
            }
            // restore_state
            case u8{0x0b}: {
                instr_list->push_panic(Instr::RestoreState);

                break;
            }
            // def_cfa
            case u8{0x0c}: {
                u64 const reg = input_cursor->read_uleb128_panic();
                u64 const off = input_cursor->read_uleb128_panic();

                data_writer->write_panic<u8>(register_to_u8_panic_(reg));
                data_writer->write_panic<u32>(offset_to_u32_panic_(off));

                instr_list->push_panic(Instr::DefCfa);

                break;
            }
            // def_cfa_register
            case u8{0x0d}: {
                u64 const reg = input_cursor->read_uleb128_panic();

                data_writer->write_panic<u8>(register_to_u8_panic_(reg));

                instr_list->push_panic(Instr::DefCfaRegister);

                break;
            }
            // def_cfa_offset
            case u8{0x0e}: {
                u64 const off = input_cursor->read_uleb128_panic();
                data_writer->write_panic<u32>(offset_to_u32_panic_(off));

                instr_list->push_panic(Instr::DefCfaOffset);

                break;
            }
            // def_cfa_expression
            case u8{0x0f}: {
                // unimplemented
                panic();
                break;
            }
            // expression
            case u8{0x10}: {
                // unimplemented
                panic();
                break;
            }
            // offset_extended_sf
            case u8{0x11}: {
                u64 const reg = input_cursor->read_uleb128_panic();
                i64 const off = input_cursor->read_sleb128_panic();
                data_writer->write_panic<u8>(register_to_u8_panic_(reg));
                data_writer->write_panic<i32>(offset_to_i32_panic_(off));

                instr_list->push_panic(Instr::OffsetExtendedSf);

                break;
            }
            // def_cfa_sf
            case u8{0x12}: {
                u64 const reg = input_cursor->read_uleb128_panic();
                i64 const off = input_cursor->read_sleb128_panic();
                data_writer->write_panic<u8>(register_to_u8_panic_(reg));
                data_writer->write_panic<i32>(offset_to_i32_panic_(off));

                instr_list->push_panic(Instr::DefCfaSf);

                break;
            }
            // def_cfa_offset_sf
            case u8{0x13}: {
                i64 const off = input_cursor->read_sleb128_panic();
                data_writer->write_panic<i32>(offset_to_i32_panic_(off));

                instr_list->push_panic(Instr::DefCfaOffsetSf);

                break;
            }
            // val_offset
            case u8{0x14}: {
                u64 const reg = input_cursor->read_uleb128_panic();
                u64 const off = input_cursor->read_uleb128_panic();

                data_writer->write_panic<u8>(register_to_u8_panic_(reg));
                data_writer->write_panic<u32>(offset_to_u32_panic_(off));

                instr_list->push_panic(Instr::ValOffset);

                break;
            }
            // val_offset_sf
            case u8{0x15}: {
                u64 const reg = input_cursor->read_uleb128_panic();
                i64 const off = input_cursor->read_sleb128_panic();

                data_writer->write_panic<u8>(register_to_u8_panic_(reg));
                data_writer->write_panic<i32>(offset_to_i32_panic_(off));

                instr_list->push_panic(Instr::ValOffsetSf);

                break;
            }
            // val_expression
            case u8{0x16}: {
                // unimplemented
                panic();
                break;
            }
            // lo_user
            case u8{0x1c}: {
                // skip this as it has no effect?
                break;
            }
            // hi_user
            case u8{0x3f}: {
                // skip this as it has no effect?
                break;
            }
            // GNU_args_size
            case u8{0x2e}: {
                // skip this by consuming the argument, we don't need
                // to record it as it has no effect on the state.
                u64 const arg_size = input_cursor->read_uleb128_panic();
                (void)arg_size;
                break;
            }
            // GNU_negative_offset_extended
            case u8{0x2f}: {
                // unimplemented
                panic();
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
        assert(alloc != tmp_alloc);
        if (input_cursor->offset() == instructions_end_offset) {
            return Instructions{
                .m_instr = Slice<Instr>::init_dangling(),
                .m_data = Slice<u8>::init_dangling(),
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
            .m_instr = instr_out.as_const(),
            .m_data = data_out.as_const(),
        };
    }
};

extern "C" struct CPUContext {
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 rbp;
    u64 rbx;
    u64 rsp;
    u64 rip;
};

extern "C" void zx_elf_getcpucontext_(CPUContext *out);

struct UnwindContext {
    u64 registers[NUM_REGS];

    __attribute__((always_inline)) static UnwindContext init() {
        CPUContext ctx;
        zx_elf_getcpucontext_(&ctx);

        UnwindContext out;
        __builtin_memset(out.registers, 0, sizeof(out.registers));

        out.registers[REG_MAP_R12] = ctx.r12;
        out.registers[REG_MAP_R13] = ctx.r13;
        out.registers[REG_MAP_R14] = ctx.r14;
        out.registers[REG_MAP_R15] = ctx.r15;
        out.registers[REG_MAP_RBP] = ctx.rbp;
        out.registers[REG_MAP_RBX] = ctx.rbx;
        out.registers[REG_MAP_RSP] = ctx.rsp;
        out.registers[REG_MAP_RETURN_ADDRESS] = ctx.rip;

        return out;
    }

    static UnwindContext init_from_ucontext(sys::UContext const *ctx) {
        UnwindContext out;
        __builtin_memset(out.registers, 0, sizeof(out.registers));

        out.registers[REG_MAP_R12] = ctx->uc_mcontext.r12;
        out.registers[REG_MAP_R13] = ctx->uc_mcontext.r13;
        out.registers[REG_MAP_R14] = ctx->uc_mcontext.r14;
        out.registers[REG_MAP_R15] = ctx->uc_mcontext.r15;
        out.registers[REG_MAP_RBP] = ctx->uc_mcontext.rbp;
        out.registers[REG_MAP_RBX] = ctx->uc_mcontext.rbx;
        out.registers[REG_MAP_RSP] = ctx->uc_mcontext.rsp;
        out.registers[REG_MAP_RETURN_ADDRESS] = ctx->uc_mcontext.rip;

        return out;
    }

    u64 rip() const { return this->registers[REG_MAP_RETURN_ADDRESS]; }
};

struct EhFrame {
    struct CIE {
        u32 code_alignment_factor;
        i32 data_alignment_factor;
        Instructions instr;
    };

    struct FDE {
        CIE const *cie;
        u64 pc_begin;
        u64 pc_range;
        Instructions instr;
    };

    Slice<FDE> m_fd_entries;

    bool step_panic(Alloc *tmp_alloc, UnwindContext *ctx) const {
        u64 const rip = ctx->registers[REG_MAP_RETURN_ADDRESS];

        if (rip == 0) {
            return false;
        }

        u64 const tmp_alloc_offset = tmp_alloc->offset();

        constexpr u64 state_stack_size = 16;
        SliceMut<RegisterState> stack_slice;
        assert(AllocStatus::Ok ==
               tmp_alloc->alloc<RegisterState>(state_stack_size, &stack_slice));
        List<RegisterState> stack = List<RegisterState>::init(stack_slice);

        FDE const *fde = nullptr;
        for (u64 i = 0; i < this->m_fd_entries.len(); i++) {
            FDE const *candi = this->m_fd_entries.offset_unsafe(i);

            u64 const pc_end = candi->pc_begin + candi->pc_range;

            if (candi->pc_begin <= rip && rip <= pc_end) {
                fde = candi;
                break;
            }
        }
        assert(fde != nullptr);

        RegisterState cie_state;
        __builtin_memset(&cie_state, 0, sizeof(cie_state));

        fde->cie->instr.run_panic(Instructions::RunArgs{
            .pc_begin = 0,
            .pc_target = U64_MAX,
            .code_alignment_factor = fde->cie->code_alignment_factor,
            .data_alignment_factor = fde->cie->data_alignment_factor,
            .cie_state = nullptr,
            .current_state = &cie_state,
            .stack = &stack,
        });

        RegisterState state = cie_state;

        fde->instr.run_panic(Instructions::RunArgs{
            .pc_begin = fde->pc_begin,
            .pc_target = rip,
            .code_alignment_factor = fde->cie->code_alignment_factor,
            .data_alignment_factor = fde->cie->data_alignment_factor,
            .cie_state = &cie_state,
            .current_state = &state,
            .stack = &stack,
        });

        assert(Alloc::ResizeStatus::Ok == tmp_alloc->resize(tmp_alloc_offset));

        // APPLY THE RULES

        UnwindContext src_ctx = *ctx;

        // get cfa
        assert(state.cfa.type == CFARule::Type::Register);
        u64 const cfa =
            src_ctx.registers[state.cfa.reg] + (u64)(i64)state.cfa.offset;

        // apply the rest
        for (u8 reg = 0; reg < NUM_REGS; reg++) {
            RegisterRule const rule = state.registers[reg];

            switch (rule.type) {
            case RegisterRule::Type::Undefined: {
                ctx->registers[reg] = 0;
                break;
            }
            case RegisterRule::Type::SameValue: {
                ctx->registers[reg] = src_ctx.registers[reg];
                break;
            }
            case RegisterRule::Type::Offset: {
                u64 const addr = cfa + (u64)(i64)rule.offset;

                u64 val;
                __builtin_memcpy(&val, (u8 const *)addr, sizeof(u64));

                ctx->registers[reg] = val;

                break;
            }
            case RegisterRule::Type::ValOffset: {
                u64 const val = cfa + (u64)(i64)rule.offset;

                ctx->registers[reg] = val;

                break;
            }
            case RegisterRule::Type::Register: {
                assert(rule.reg < NUM_REGS);
                ctx->registers[reg] = src_ctx.registers[rule.reg];

                break;
            }
            default: {
                panic();
            }
            }
        }

        ctx->registers[REG_MAP_RSP] = cfa;

        return true;
    }

    static CIE parse_cie_panic(Alloc *alloc, Alloc *tmp_alloc, u64 end_offset,
                               u64 section_addr, Cursor *input_cursor) {
        assert(alloc != tmp_alloc);
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

        bool overflow = false;

        u64 const code_alignment_factor_64 = input_cursor->read_uleb128_panic();
        assert(code_alignment_factor_64 != 0);
        u32 const code_alignment_factor =
            u64_to_u32(code_alignment_factor_64, &overflow);
        assert(!overflow);

        i64 const data_alignment_factor_64 = input_cursor->read_sleb128_panic();
        assert(data_alignment_factor_64 != 0);
        i32 const data_alignment_factor =
            i64_to_i32(data_alignment_factor_64, &overflow);
        assert(!overflow);

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
        assert(alloc != tmp_alloc);
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
        assert(alloc != nullptr);
        assert(tmp_alloc != nullptr);
        assert(header != nullptr);
        assert(alloc != tmp_alloc);

        if (input_bytes.len() == 0) {
            return EhFrame{
                .m_fd_entries = Slice<FDE>::init_dangling(),
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
            .m_fd_entries = out_fdes.as_const(),
        };
    }
};

// These are defined in the DWARF5 spec. Can search by name in there and should
// find the tables that define them.
constexpr u8 DW_LNCT_path = 0x1;
constexpr u8 DW_LNCT_directory_index = 0x2;
constexpr u8 DW_FORM_string = 0x08;
constexpr u8 DW_FORM_udata = 0x0f;
constexpr u8 DW_FORM_strp = 0x0e;
constexpr u8 DW_FORM_line_strp = 0x1f;
constexpr u8 DW_FORM_data16 = 0x1e;

u64 strnlen_panic_(u8 const *str, u64 max_len) {
    for (u64 i = 0; i < max_len; i++) {
        if (str[i] == 0) {
            return i;
        }
    }

    panic();
}

struct DebugLine {
    struct Info {
        u16 file_no;
        u16 line_no;
        u16 column_no;
    };
    static_assert(sizeof(Info) == 6);
    static_assert(alignof(Info) == alignof(u16));

    Slice<CString> file_paths;
    u64 minimum_pc;
    Slice<Info> infos;

    struct LineInfo {
        CString file_path;
        u16 line_no;
        u16 column_no;
    };

    enum class [[nodiscard]] GetLineInfoStatus : u8_c {
        Ok = 0,
        NotFound,
        Invalid,
        Nullptr,
    };

    struct InfoRow {
        u64 pc;
        u16 file_no;
        u16 line_no;
        u16 column_no;
    };

    // Parses the directory_path and file_path section of the compute unit
    // header.
    //
    // Pushes the found paths into the global `file_path` list.
    //
    // Returns file_path_map which maps local file_no register values to global
    // `file_paths` index. For example, if we are executing a debug_line program
    // and file_no register has value 69. We can map that index to a global
    // index by doing file_path_map[69], then we can write the global index to a
    // InfoRow.
    //
    // Return slice is allocated using tmp_alloc.
    //
    // Regular alloc is only used for allocating the individual file paths that
    // go into the global `file_paths` list.
    //
    // Lifetimes of paths might be tied to debug_line_str data too.
    static Slice<u32> parse_filepaths_panic_(Alloc *alloc, Alloc *tmp_alloc,
                                             Slice<u8> debug_line_str,
                                             Cursor *input,
                                             List<CString> *file_paths) {
        assert(alloc != tmp_alloc);

        bool overflow = false;

        SliceMut<Slice<u8>> directory_paths;
        {
            u8 const directory_entry_format_count = input->read_panic<u8>();
            assert(directory_entry_format_count == 1);

            u64 const def_content_type = input->read_uleb128_panic();
            u64 const def_form_code = input->read_uleb128_panic();

            assert(def_content_type == DW_LNCT_path);
            // means that the value is an u32 offset into the .debug_line_str
            // section
            assert(def_form_code == DW_FORM_line_strp);

            u64 const directories_count = input->read_uleb128_panic();
            assert(AllocStatus::Ok == tmp_alloc->alloc<Slice<u8>>(
                                          directories_count, &directory_paths));

            for (u64 i = 0; i < directories_count; i++) {
                u32 const offset = input->read_panic<u32>();
                u8 const *start = debug_line_str.offset_unsafe(offset);
                assert(offset < debug_line_str.len());
                u64 const len = strnlen_panic_(
                    start, sub_unsafe_u64(debug_line_str.len(), offset));

                Slice<u8> dir_path = Slice<u8>::init_unsafe(start, len);
                assert(dir_path.len() > 0);

                directory_paths.write_unsafe(i, dir_path);
            }
        }

        // should at least have the base absolute path at 0 index
        assert(directory_paths.len() > 0);
        // first one should be absolute
        assert(directory_paths.read_unsafe(0).read_unsafe(0) == '/');
        for (u64 i = 1; i < directory_paths.len(); i++) {
            // rest should be relative
            assert(directory_paths.read_unsafe(i).read_unsafe(0) != '/');
        }

        for (u64 i = 0; i < directory_paths.len(); i++) {
            Slice<u8> path = directory_paths.read_unsafe(i);
            u64 const last_idx = sub_u64(path.len(), 1, &overflow);
            assert(!overflow);
            // none of these should end with a '/'.
            // this is important as we assume this while cancatenating them.
            assert(path.read_unsafe(last_idx) != '/');
        }

        // maps local file_path index to global file path index
        // so if line number program state's `file` register is set to 69
        // then we can do file_paths[file_path_map[69]] to access the actual
        // path string.
        //
        // Practically we will use file_path_map[69] to record it as
        // InfoRow.file_no into the global InfoRow list.
        SliceMut<u32> file_path_map;
        {
            u8 const file_name_entry_format_count = input->read_panic<u8>();
            assert(file_name_entry_format_count >= 2);

            // file path
            {
                u64 const def_content_type = input->read_uleb128_panic();
                u64 const def_form_code = input->read_uleb128_panic();
                assert(def_content_type == DW_LNCT_path);
                assert(def_form_code == DW_FORM_line_strp);
            }

            // directory index
            {
                u64 const def_content_type = input->read_uleb128_panic();
                u64 const def_form_code = input->read_uleb128_panic();
                assert(def_content_type == DW_LNCT_directory_index);
                assert(def_form_code == DW_FORM_udata);
            }

            u8 const num_unknown_formats =
                sub_unsafe_u8(file_name_entry_format_count, 2);

            u8 unk_formats[16];

            assert(num_unknown_formats < sizeof(unk_formats));

            for (u8 i = 0; i < num_unknown_formats; i++) {
                u64 const def_content_type = input->read_uleb128_panic();
                u64 const def_form_code = input->read_uleb128_panic();
                // don't care about this as we will just skip this one.
                (void)def_content_type;
                overflow = false;
                unk_formats[i] = u64_to_u8(def_form_code, &overflow);
                assert(!overflow);
            }

            u64 const file_names_count = input->read_uleb128_panic();

            assert(AllocStatus::Ok ==
                   tmp_alloc->alloc<u32>(file_names_count, &file_path_map));

            for (u64 i = 0; i < file_names_count; i++) {
                u32 const offset = input->read_panic<u32>();
                // skip directory index
                u64 const directory_index = input->read_uleb128_panic();
                // skip the unknown stuff
                for (u64 j = 0; j < num_unknown_formats; j++) {
                    switch (unk_formats[j]) {
                    case DW_FORM_line_strp: {
                        input->read_panic<u32>();
                        break;
                    }
                    case DW_FORM_data16: {
                        assert(Cursor::Status::Ok == input->skip(16));
                        break;
                    }
                    default: {
                        panic();
                    }
                    }
                }

                u8 const *start = debug_line_str.offset_unsafe(offset);
                assert(offset < debug_line_str.len());
                u64 const len = strnlen_panic_(
                    start, sub_unsafe_u64(debug_line_str.len(), offset));

                CString path_cstr = CString::init_unsafe((char const *)start);
                assert(path_cstr.as_slice().len() == len);

                Slice<u8> path = path_cstr.as_slice();
                // can't be empty
                assert(path.len() > 0);

                bool const is_absolute = path.read_unsafe(0) == '/';

                CString abs_file_path;
                if (is_absolute) {
                    abs_file_path = path_cstr;
                } else {
                    assert(directory_paths.len() > 0);
                    Slice<u8> base = directory_paths.read_unsafe(0);
                    Slice<u8> dir;
                    assert(SliceStatus::Ok ==
                           directory_paths.read(directory_index, &dir));

                    // 3 is because we have a format like `base / dir / path
                    // NULL_BYTE` 2 for slashes one for the null byte.
                    u64 const needed_len = add_u64(
                        add_u64(base.len(), 3, &overflow),
                        add_u64(dir.len(), path.len(), &overflow), &overflow);
                    assert(!overflow);

                    SliceMut<u8> abs_p;
                    assert(AllocStatus::Ok ==
                           alloc->alloc<u8>(needed_len, &abs_p));
                    Fmt fmt = Fmt::init(abs_p);
                    bool fmt_overflow = false;

                    fmt.push_str(base, &fmt_overflow);
                    fmt.push_char('/', &fmt_overflow);
                    fmt.push_str(dir, &fmt_overflow);
                    fmt.push_char('/', &fmt_overflow);
                    fmt.push_str(path, &fmt_overflow);
                    fmt.push_char(0, &fmt_overflow);
                    assert(!fmt_overflow);

                    abs_file_path =
                        CString::init_unsafe((char const *)fmt.written().ptr());
                    assert(abs_file_path.as_slice().len() ==
                           sub_unsafe_u64(needed_len, 1));
                }

                bool found = false;
                u32 idx = U32_MAX;
                for (u64 j = 0; j < file_paths->n_written(); j++) {
                    Slice<u8> other =
                        file_paths->written().read_unsafe(j).as_slice();

                    if (other.equals(abs_file_path.as_slice())) {
                        idx = u64_to_u32(j, &overflow);
                        assert(!overflow);
                        found = true;
                        break;
                    }
                }

                if (found) {
                    assert(idx != U32_MAX);
                    file_path_map.write_unsafe(i, idx);
                } else {
                    file_paths->push_panic(abs_file_path);
                    file_path_map.write_unsafe(
                        i, u64_to_u32(file_paths->n_written(), &overflow));
                    assert(!overflow);
                }
            }
        }

        // UNCOMMENT THIS TO PRINT ALL FILE PATHS, intended for debugging
        // purposes. Don't forget to disable :sys import after using this.
        //
        // for (u64 i=0; i < file_paths->n_written(); i++) {
        //     Slice<u8> path = file_paths->written().read_unsafe(i).as_slice();
        //     u64 nw = 0;
        //     assert(sys::WriteStatus::Ok == sys::write(sys::STDERR,
        //     path.ptr(), path.len(), &nw)); assert(nw == path.len());

        //     assert(sys::WriteStatus::Ok == sys::write(sys::STDERR, (u8
        //     const*)"\n", 1, &nw)); assert(nw == 1);
        // }
        // panic();

        return file_path_map.as_const();
    }

    static void parse_compilation_unit_panic_(Alloc *alloc, Alloc *tmp_alloc,
                                              Slice<u8> debug_line_str,
                                              Cursor *input,
                                              List<InfoRow> *infos,
                                              List<CString> *file_paths,
                                              u64 *min_pc, u64 *max_pc) {
        assert(alloc != tmp_alloc);

        u64 const tmp_alloc_initial_offset = tmp_alloc->offset();

        u32 const unit_length = input->read_panic<u32>();
        // this means DWARF 64bit version, we only support parsing the 32bit
        // version.
        assert(unit_length != U32_MAX);

        u16 const version = input->read_panic<u16>();
        assert(version == 5);

        u8 const address_size = input->read_panic<u8>();
        assert(address_size == 8);

        u8 const segment_selector_size = input->read_panic<u8>();
        assert(segment_selector_size == 0);

        u32 const header_length = input->read_panic<u32>();
        // We don't support 64bit dwarf format. Only 32bit is supported.
        assert(header_length != U32_MAX);

        bool overflow = false;

        u64 const expected_instructions_start_offset =
            add_u64(input->offset(), header_length, &overflow);
        assert(!overflow);

        u8 const minimum_instruction_length = input->read_panic<u8>();
        u8 const maximum_operations_per_instruction = input->read_panic<u8>();
        (void)minimum_instruction_length;
        (void)maximum_operations_per_instruction;

        // unused, we don't care about breakpoints since we are not implementing
        // a debugger.
        {
            u8 const default_is_stmt = input->read_panic<u8>();
            (void)default_is_stmt;
        }

        i8 const line_base = input->read_panic<i8>();
        u8 const line_range = input->read_panic<u8>();
        (void)line_base;
        (void)line_range;

        u8 const opcode_base = input->read_panic<u8>();
        (void)opcode_base;
        // TODO: remove these (void) stuff
        (void)infos;
        (void)file_paths;
        (void)min_pc;
        (void)max_pc;

        // unused, we will error if we encounter an unknown opcode
        overflow = false;
        u8 const num_standard_opcodes = sub_u8(opcode_base, 1, &overflow);
        assert(!overflow);

        assert(Cursor::Status::Ok ==
               input->skip(num_standard_opcodes * sizeof(u8)));

        Slice<u32> file_path_map = parse_filepaths_panic_(
            alloc, tmp_alloc, debug_line_str, input, file_paths);
        (void)file_path_map;

        // parsing file paths should have consumed the right number of bytes at
        // this point.
        assert(input->offset() == expected_instructions_start_offset);

        assert(Alloc::ResizeStatus::Ok ==
               tmp_alloc->resize(tmp_alloc_initial_offset));
    }

    static DebugLine init_panic(Alloc *alloc, Alloc *tmp_alloc,
                                Slice<u8> input_bytes,
                                Slice<u8> debug_line_str) {
        assert(alloc != nullptr);
        assert(tmp_alloc != nullptr);
        assert(alloc != tmp_alloc);

        if (input_bytes.len() == 0) {
            return DebugLine{
                .file_paths = Slice<CString>::init_dangling(),
                .minimum_pc = 0,
                .infos = Slice<Info>::init_dangling(),
            };
        }

        u64 const tmp_alloc_initial_offset = tmp_alloc->offset();

        SliceMut<InfoRow> info_mem;
        assert(AllocStatus::Ok ==
               tmp_alloc->alloc<InfoRow>(input_bytes.len(), &info_mem));
        List<InfoRow> infos = List<InfoRow>::init(info_mem);

        constexpr u64 max_num_files = 1024;
        SliceMut<CString> file_paths_mem;
        assert(AllocStatus::Ok ==
               tmp_alloc->alloc<CString>(max_num_files, &file_paths_mem));
        List<CString> file_paths = List<CString>::init(file_paths_mem);

        Cursor input = Cursor::init(input_bytes);

        u64 min_pc = 0;
        u64 max_pc = 0;

        // iterate over compilation units
        while (!input.is_finished()) {
            parse_compilation_unit_panic_(alloc, tmp_alloc, debug_line_str,
                                          &input, &infos, &file_paths, &min_pc,
                                          &max_pc);
            // TODO: remove this!!
            break;
        }

        bool overflow = false;
        u64 const n_pcs = sub_u64(max_pc, min_pc, &overflow);
        assert(!overflow);

        // ## move the found infos to the orderly output array
        SliceMut<Info> infos_out;
        assert(AllocStatus::Ok == alloc->alloc<Info>(n_pcs, &infos_out));
        infos_out.memset(0);

        Slice<InfoRow> collected_infos = infos.written();

        for (u64 i = 0; i < collected_infos.len(); i++) {
            InfoRow const inf = collected_infos.read_unsafe(i);

            overflow = false;
            u64 const pc_idx = sub_u64(inf.pc, min_pc, &overflow);
            assert(!overflow);

            // write to output
            infos_out.write_unsafe(pc_idx, Info{
                                               .file_no = inf.file_no,
                                               .line_no = inf.line_no,
                                               .column_no = inf.column_no,
                                           });
        }

        // ## move file path info to regular alloc since we will discard the tmp
        // alloc.
        SliceMut<CString> file_paths_out;
        assert(AllocStatus::Ok ==
               alloc->alloc<CString>(file_paths.n_written(), &file_paths_out));
        assert(SliceMemcpyStatus::Ok ==
               file_paths_out.memcpy_from(file_paths.written()));

        // discard tmp memory
        assert(Alloc::ResizeStatus::Ok ==
               tmp_alloc->resize(tmp_alloc_initial_offset));

        return DebugLine{
            .infos = infos_out.as_const(),
            .file_paths = file_paths_out.as_const(),
            .minimum_pc = min_pc,
        };
    }

    // GetLineInfoStatus get_line_info(u64 pc, LineInfo* out) const {
    //    index into this->infos to get the info, don't forget to subtract
    //    this->minimum_pc from the query pc.
    //    file_path.len() == 0 means NOT FOUND
    // }
};

} // namespace elf

} // namespace zx
