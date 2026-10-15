export module zx:elf_eh_frame;
import :def;
import :assert;
import :arith;
import :slice;
import :alloc;
import :panic;
import :leb128;
import :cursor;
import :list;
import :cpu_context;
import :backtrace;

export namespace zx {

namespace elf {

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
                u8 delta = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &delta));

                pc = advance_loc_panic_(args.code_alignment_factor, pc, delta);

                break;
            }
            case Instr::Offset: {
                u8 regno = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &regno));

                u32 off = 0;
                assert(Cursor::Status::Ok == cursor.read<u32>(nullptr, &off));

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

                u8 regno = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &regno));

                assert(regno < NUM_REGS);

                args.current_state->registers[regno] =
                    args.cie_state->registers[regno];

                break;
            }
            case Instr::SetLoc: {
                u64 addr = 0;
                assert(Cursor::Status::Ok == cursor.read<u64>(nullptr, &addr));

                assert(addr > pc);
                pc = addr;

                break;
            }
            case Instr::AdvanceLoc1: {
                u8 delta = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &delta));

                pc = advance_loc_panic_(args.code_alignment_factor, pc, delta);

                break;
            }
            case Instr::AdvanceLoc2: {
                u16 delta = 0;
                assert(Cursor::Status::Ok == cursor.read<u16>(nullptr, &delta));

                pc = advance_loc_panic_(args.code_alignment_factor, pc, delta);

                break;
            }
            case Instr::AdvanceLoc4: {
                u32 delta = 0;
                assert(Cursor::Status::Ok == cursor.read<u32>(nullptr, &delta));

                pc = advance_loc_panic_(args.code_alignment_factor, pc, delta);

                break;
            }
            case Instr::OffsetExtended: {
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                u32 off = 0;
                assert(Cursor::Status::Ok == cursor.read<u32>(nullptr, &off));

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
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                assert(reg < NUM_REGS);

                assert(args.cie_state != nullptr);
                args.current_state->registers[reg] =
                    args.cie_state->registers[reg];

                break;
            }
            case Instr::Undefined: {
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                assert(reg < NUM_REGS);

                __builtin_memset(&args.current_state->registers[reg], 0,
                                 sizeof(RegisterRule));

                break;
            }
            case Instr::SameValue: {
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                assert(reg < NUM_REGS);

                args.current_state->registers[reg] = RegisterRule{
                    .type = RegisterRule::Type::SameValue,
                    .reg = U8_MAX,
                    .offset = 0,
                };

                break;
            }
            case Instr::Register: {
                u8 reg_tgt = 0;
                assert(Cursor::Status::Ok ==
                       cursor.read<u8>(nullptr, &reg_tgt));
                u8 reg_src = 0;
                assert(Cursor::Status::Ok ==
                       cursor.read<u8>(nullptr, &reg_src));

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
                assert(ListPushStatus::Ok ==
                       args.stack->push(nullptr, *args.current_state));

                break;
            }
            case Instr::RestoreState: {
                assert(ListPopStatus::Ok ==
                       args.stack->pop(nullptr, args.current_state));

                break;
            }
            case Instr::DefCfa: {
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                u32 off = 0;
                assert(Cursor::Status::Ok == cursor.read<u32>(nullptr, &off));

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
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                assert(reg < NUM_REGS);

                args.current_state->cfa = CFARule{
                    .type = CFARule::Type::Register,
                    .reg = reg,
                    .offset = args.current_state->cfa.offset,
                };

                break;
            }
            case Instr::DefCfaOffset: {
                u32 off = 0;
                assert(Cursor::Status::Ok == cursor.read<u32>(nullptr, &off));

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
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                i32 off = 0;
                assert(Cursor::Status::Ok == cursor.read<i32>(nullptr, &off));

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
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                i32 off = 0;
                assert(Cursor::Status::Ok == cursor.read<i32>(nullptr, &off));

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
                i32 off = 0;
                assert(Cursor::Status::Ok == cursor.read<i32>(nullptr, &off));

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
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                u32 off = 0;
                assert(Cursor::Status::Ok == cursor.read<u32>(nullptr, &off));

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
                u8 reg = 0;
                assert(Cursor::Status::Ok == cursor.read<u8>(nullptr, &reg));

                i32 off = 0;
                assert(Cursor::Status::Ok == cursor.read<i32>(nullptr, &off));

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
        u8 byte = 0;
        assert(Cursor::Status::Ok == input_cursor->read<u8>(nullptr, &byte));

        u8 const high2 = (byte & 0b11000000) >> 6;
        u8 const low6 = byte & 0b00111111;

        switch (high2) {
        // advance_loc
        case u8{0x1}: {
            assert(ListPushStatus::Ok ==
                   instr_list->push(nullptr, Instr::AdvanceLoc));
            assert(WriteCursor::Status::Ok ==
                   data_writer->write<u8>(nullptr, low6));
            break;
        }
        // offset
        case u8{0x2}: {
            u64 off = 0;
            assert(leb128::DecodeStatus::Ok ==
                   input_cursor->read_uleb128(nullptr, &off));

            assert(ListPushStatus::Ok ==
                   instr_list->push(nullptr, Instr::Offset));
            assert(WriteCursor::Status::Ok ==
                   data_writer->write<u8>(nullptr, low6));
            assert(WriteCursor::Status::Ok ==
                   data_writer->write<u32>(nullptr, offset_to_u32_panic_(off)));

            break;
        }
        // restore
        case u8{0x3}: {
            assert(WriteCursor::Status::Ok ==
                   data_writer->write<u8>(nullptr, low6));
            assert(ListPushStatus::Ok ==
                   instr_list->push(nullptr, Instr::Restore));

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

                i32 addr_encoded = 0;
                assert(Cursor::Status::Ok ==
                       input_cursor->read<i32>(nullptr, &addr_encoded));

                u64 const addr = decode_pcrel_panic_(section_addr, field_offset,
                                                     addr_encoded);

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u64>(nullptr, addr));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::SetLoc));

                break;
            }
            // advance_loc1
            case u8{0x02}: {
                u8 delta = 0;
                assert(Cursor::Status::Ok ==
                       input_cursor->read<u8>(nullptr, &delta));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr, delta));
                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::AdvanceLoc1));

                break;
            }
            // advance_loc2
            case u8{0x03}: {
                u16 delta = 0;
                assert(Cursor::Status::Ok ==
                       input_cursor->read<u16>(nullptr, &delta));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u16>(nullptr, delta));
                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::AdvanceLoc2));

                break;
            }
            // advance_loc4
            case u8{0x04}: {
                u32 delta = 0;
                assert(Cursor::Status::Ok ==
                       input_cursor->read<u32>(nullptr, &delta));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u32>(nullptr, delta));
                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::AdvanceLoc4));

                break;
            }
            // offset_extended
            case u8{0x05}: {
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                u64 off = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &off));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));
                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u32>(nullptr,
                                               offset_to_u32_panic_(off)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::OffsetExtended));

                break;
            }
            // restore_extended
            case u8{0x06}: {
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::RestoreExtended));

                break;
            }
            // undefined
            case u8{0x07}: {
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::Undefined));

                break;
            }
            // same_value
            case u8{0x08}: {
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::SameValue));

                break;
            }
            // register
            case u8{0x09}: {
                u64 reg0 = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg0));

                u64 reg1 = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg1));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg0)));
                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg1)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::Register));

                break;
            }
            // remember_state
            case u8{0x0a}: {
                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::RememberState));

                break;
            }
            // restore_state
            case u8{0x0b}: {
                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::RestoreState));

                break;
            }
            // def_cfa
            case u8{0x0c}: {
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                u64 off = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &off));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));
                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u32>(nullptr,
                                               offset_to_u32_panic_(off)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::DefCfa));

                break;
            }
            // def_cfa_register
            case u8{0x0d}: {
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::DefCfaRegister));

                break;
            }
            // def_cfa_offset
            case u8{0x0e}: {
                u64 off = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &off));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u32>(nullptr,
                                               offset_to_u32_panic_(off)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::DefCfaOffset));

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
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                i64 off = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_sleb128(nullptr, &off));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));
                assert(WriteCursor::Status::Ok ==
                       data_writer->write<i32>(nullptr,
                                               offset_to_i32_panic_(off)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::OffsetExtendedSf));

                break;
            }
            // def_cfa_sf
            case u8{0x12}: {
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                i64 off = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_sleb128(nullptr, &off));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));
                assert(WriteCursor::Status::Ok ==
                       data_writer->write<i32>(nullptr,
                                               offset_to_i32_panic_(off)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::DefCfaSf));

                break;
            }
            // def_cfa_offset_sf
            case u8{0x13}: {
                i64 off = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_sleb128(nullptr, &off));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<i32>(nullptr,
                                               offset_to_i32_panic_(off)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::DefCfaOffsetSf));

                break;
            }
            // val_offset
            case u8{0x14}: {
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                u64 off = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &off));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));
                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u32>(nullptr,
                                               offset_to_u32_panic_(off)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::ValOffset));

                break;
            }
            // val_offset_sf
            case u8{0x15}: {
                u64 reg = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_uleb128(nullptr, &reg));

                i64 off = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input_cursor->read_sleb128(nullptr, &off));

                assert(WriteCursor::Status::Ok ==
                       data_writer->write<u8>(nullptr,
                                              register_to_u8_panic_(reg)));
                assert(WriteCursor::Status::Ok ==
                       data_writer->write<i32>(nullptr,
                                               offset_to_i32_panic_(off)));

                assert(ListPushStatus::Ok ==
                       instr_list->push(nullptr, Instr::ValOffsetSf));

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
                // shouldn't happen
                panic();
                break;
            }
            // hi_user
            case u8{0x3f}: {
                // shouldn't happen
                panic();
                break;
            }
            // GNU_args_size
            case u8{0x2e}: {
                // unimplemented, seems unapplicable
                panic();
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

        constexpr u64 max_num_instructions = u64{1} << 14;
        SliceMut<Instr> instr;
        assert(AllocStatus::Ok ==
               tmp_alloc->alloc<Instr>(nullptr, max_num_instructions, &instr));

        constexpr u64 max_data_size = u64{1} << 19;
        SliceMut<u8> data;
        assert(AllocStatus::Ok ==
               tmp_alloc->alloc<u8>(nullptr, max_data_size, &data));

        List<Instr> instr_list = List<Instr>::init(instr);
        WriteCursor data_writer = WriteCursor::init(data);

        while (input_cursor->offset() < instructions_end_offset) {
            parse_one_panic_(section_addr, &instr_list, &data_writer,
                             input_cursor);
        }
        assert(input_cursor->offset() == instructions_end_offset);

        // move instr to real alloc
        SliceMut<Instr> instr_out;
        assert(AllocStatus::Ok == alloc->alloc<Instr>(nullptr,
                                                      instr_list.n_written(),
                                                      &instr_out));
        assert(SliceMemcpyStatus::Ok ==
               instr_out.memcpy_from(nullptr, instr_list.written()));

        // move data to real alloc
        SliceMut<u8> data_out;
        assert(AllocStatus::Ok ==
               alloc->alloc<u8>(nullptr, data_writer.offset(), &data_out));
        assert(SliceMemcpyStatus::Ok ==
               data_out.memcpy_from(nullptr, data_writer.written()));

        // dealloc temporary memory we used
        assert(Alloc::ResizeStatus::Ok ==
               tmp_alloc->resize(nullptr, tmp_alloc_initial_offset));

        return Instructions{
            .m_instr = instr_out.as_const(),
            .m_data = data_out.as_const(),
        };
    }
};

struct UnwindContext {
    u64 registers[NUM_REGS];

    static UnwindContext init(CPUContext const *ctx) {
        UnwindContext out;
        __builtin_memset(out.registers, 0, sizeof(out.registers));

        out.registers[REG_MAP_R12] = ctx->r12;
        out.registers[REG_MAP_R13] = ctx->r13;
        out.registers[REG_MAP_R14] = ctx->r14;
        out.registers[REG_MAP_R15] = ctx->r15;
        out.registers[REG_MAP_RBP] = ctx->rbp;
        out.registers[REG_MAP_RBX] = ctx->rbx;
        out.registers[REG_MAP_RSP] = ctx->rsp;
        out.registers[REG_MAP_RETURN_ADDRESS] = ctx->return_address;

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

    void step_panic(List<RegisterState> *state_stack,
                    UnwindContext *ctx) const {
        assert(state_stack != nullptr);
        assert(ctx != nullptr);
        assert(state_stack->n_written() == 0);

        u64 const return_addr = ctx->registers[REG_MAP_RETURN_ADDRESS];

        assert(return_addr != 0);

        u64 const rip = return_addr - 1;

        ctx->registers[REG_MAP_RETURN_ADDRESS] = rip;

        FDE const *fde = nullptr;
        for (u64 i = 0; i < this->m_fd_entries.len(); i++) {
            FDE const *candi = this->m_fd_entries.offset_unsafe(i);

            bool overflow = false;
            u64 const pc_end =
                add_u64(candi->pc_begin, candi->pc_range, &overflow);
            assert(!overflow);

            if (candi->pc_begin <= rip && rip < pc_end) {
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
            .stack = state_stack,
        });

        RegisterState state = cie_state;

        fde->instr.run_panic(Instructions::RunArgs{
            .pc_begin = fde->pc_begin,
            .pc_target = rip,
            .code_alignment_factor = fde->cie->code_alignment_factor,
            .data_alignment_factor = fde->cie->data_alignment_factor,
            .cie_state = &cie_state,
            .current_state = &state,
            .stack = state_stack,
        });

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
    }

    enum class [[nodiscard]] BacktraceStatus : u8_c {
        Ok = 0,
        OutOfCapacity,
    };

    BacktraceStatus backtrace_panic(CPUContext const *cpu_ctx, Backtrace *bt) {
        assert(cpu_ctx != nullptr);
        assert(bt != nullptr);

        constexpr u64 STATE_STACK_SIZE = 16;
        RegisterState state_stack_mem[STATE_STACK_SIZE];
        __builtin_memset(state_stack_mem, 0, sizeof(state_stack_mem));

        List<RegisterState> state_stack =
            List<RegisterState>::init(SliceMut<RegisterState>::init_unsafe(
                state_stack_mem, STATE_STACK_SIZE));

        UnwindContext unwind_ctx = UnwindContext::init(cpu_ctx);

        u64 rip = unwind_ctx.registers[REG_MAP_RETURN_ADDRESS];
        u64 bt_offset = 0;
        while (rip != 0) {
            if (bt_offset == Backtrace::LEN) {
                return BacktraceStatus::OutOfCapacity;
            }
            bt->m_data[bt_offset] = rip;
            bt_offset++;

            state_stack.reset();
            step_panic(&state_stack, &unwind_ctx);

            rip = unwind_ctx.registers[REG_MAP_RETURN_ADDRESS];
        }

        // Fill the rest with zeroes so the end of the actual trace is apparent.
        for (u64 i = bt_offset; i < Backtrace::LEN; i++) {
            bt->m_data[i] = 0;
        }

        return BacktraceStatus::Ok;
    }

    static CIE parse_cie_panic(Alloc *alloc, Alloc *tmp_alloc, u64 end_offset,
                               u64 section_addr, Cursor *input_cursor) {
        assert(alloc != tmp_alloc);

        u8 version = 0;
        assert(Cursor::Status::Ok == input_cursor->read<u8>(nullptr, &version));
        assert(version == 1);

        // This block of code just asserts that the augmentation string is equal
        // to "zR". this means the CIE will have a single byte in it's
        // augmentation data that will signify how pointers are encoded in
        // associated FDEs.
        //
        // We assume that this will be the case for all CIE so we just assert it
        // instead of conditionally parsing the augmentation string.
        {
            u8 byte = 0;

            assert(Cursor::Status::Ok ==
                   input_cursor->read<u8>(nullptr, &byte));
            assert(byte == 'z');

            assert(Cursor::Status::Ok ==
                   input_cursor->read<u8>(nullptr, &byte));
            assert(byte == 'R');

            assert(Cursor::Status::Ok ==
                   input_cursor->read<u8>(nullptr, &byte));
            assert(byte == '\0');
        }

        bool overflow = false;

        u64 code_alignment_factor_64 = 0;
        assert(leb128::DecodeStatus::Ok ==
               input_cursor->read_uleb128(nullptr, &code_alignment_factor_64));
        assert(code_alignment_factor_64 != 0);
        u32 const code_alignment_factor =
            u64_to_u32(code_alignment_factor_64, &overflow);
        assert(!overflow);

        i64 data_alignment_factor_64 = 0;
        assert(leb128::DecodeStatus::Ok ==
               input_cursor->read_sleb128(nullptr, &data_alignment_factor_64));
        assert(data_alignment_factor_64 != 0);
        i32 const data_alignment_factor =
            i64_to_i32(data_alignment_factor_64, &overflow);
        assert(!overflow);

        u64 return_address_register = 0;
        assert(leb128::DecodeStatus::Ok ==
               input_cursor->read_uleb128(nullptr, &return_address_register));
        assert(RETURN_ADDRESS_REGISTER == return_address_register);

        u64 aug_data_len = 0;
        assert(leb128::DecodeStatus::Ok ==
               input_cursor->read_uleb128(nullptr, &aug_data_len));
        assert(aug_data_len == 1);

        u8 fde_ptr_enc = 0;
        assert(Cursor::Status::Ok ==
               input_cursor->read<u8>(nullptr, &fde_ptr_enc));
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

        i32 pc_begin_encoded = 0;
        assert(Cursor::Status::Ok ==
               input_cursor->read<i32>(nullptr, &pc_begin_encoded));

        u64 const pc_begin = decode_pcrel_panic_(
            section_addr, pc_begin_field_offset, pc_begin_encoded);

        u32 pc_range = 0;
        assert(Cursor::Status::Ok ==
               input_cursor->read<u32>(nullptr, &pc_range));

        u64 aug_data_len = 0;
        assert(leb128::DecodeStatus::Ok ==
               input_cursor->read_uleb128(nullptr, &aug_data_len));
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
                              u64 section_address, Slice<u8> input_bytes) {
        assert(alloc != nullptr);
        assert(tmp_alloc != nullptr);
        assert(alloc != tmp_alloc);

        if (input_bytes.len() == 0) {
            return EhFrame{
                .m_fd_entries = Slice<FDE>::init_dangling(),
            };
        }

        Cursor input_cursor = Cursor::init(input_bytes);

        u64 const tmp_alloc_initial_offset = tmp_alloc->offset();

        constexpr u64 max_num_fdes = u64{1} << 18;

        SliceMut<FDE> fdes;
        assert(AllocStatus::Ok ==
               tmp_alloc->alloc<FDE>(nullptr, max_num_fdes, &fdes));

        List<FDE> fde_list = List<FDE>::init(fdes);

        CIE *cie = nullptr;
        u64 cie_offset = 0;
        for (;;) {
            u64 const offset = input_cursor.offset();
            // length is common between CIE and FDE so we parse it regardless
            u32 length = 0;
            assert(Cursor::Status::Ok ==
                   input_cursor.read<u32>(nullptr, &length));

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
            u32 cie_id = 0;
            assert(Cursor::Status::Ok ==
                   input_cursor.read<u32>(nullptr, &cie_id));

            if (cie_id == 0) {
                // parse cie
                assert(AllocStatus::Ok == alloc->create<CIE>(nullptr, &cie));
                *cie = parse_cie_panic(alloc, tmp_alloc, end_offset,
                                       section_address, &input_cursor);
                cie_offset = offset;
            } else {
                // parse fde
                assert(cie != nullptr);
                assert(cie_offset == (cie_id_offset - cie_id));

                FDE const fde =
                    parse_fde_panic(alloc, tmp_alloc, end_offset,
                                    section_address, cie, &input_cursor);

                assert(ListPushStatus::Ok == fde_list.push(nullptr, fde));
            }
        }
        assert(input_cursor.is_finished());

        // move fdes to long term alloc (non-tmp)
        SliceMut<FDE> out_fdes;
        assert(AllocStatus::Ok ==
               alloc->alloc<FDE>(nullptr, fde_list.n_written(), &out_fdes));
        assert(SliceMemcpyStatus::Ok ==
               out_fdes.memcpy_from(nullptr, fde_list.written()));

        // dealloc temporary memory that we used for this function
        assert(Alloc::ResizeStatus::Ok ==
               tmp_alloc->resize(nullptr, tmp_alloc_initial_offset));

        return EhFrame{
            .m_fd_entries = out_fdes.as_const(),
        };
    }
};

} // namespace elf

} // namespace zx
