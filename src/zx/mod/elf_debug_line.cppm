export module zx:elf_debug_line;
import :def;
import :assert;
import :arith;
import :slice;
import :alloc;
import :panic;
import :leb128;
import :cursor;
import :list;
import :fmt;
import :backtrace;

export namespace zx {

namespace elf_debug_line {

// These are defined in the DWARF5 spec. Can search by name in there and should
// find the tables that define them.
constexpr u8 DW_LNCT_path = 0x1;
constexpr u8 DW_LNCT_directory_index = 0x2;
constexpr u8 DW_FORM_string = 0x08;
constexpr u8 DW_FORM_udata = 0x0f;
constexpr u8 DW_FORM_strp = 0x0e;
constexpr u8 DW_FORM_line_strp = 0x1f;
constexpr u8 DW_FORM_data16 = 0x1e;

// Line number standard op codes
constexpr u8 DW_LNS_copy = 0x01;
constexpr u8 DW_LNS_advance_pc = 0x02;
constexpr u8 DW_LNS_advance_line = 0x03;
constexpr u8 DW_LNS_set_file = 0x04;
constexpr u8 DW_LNS_set_column = 0x05;
constexpr u8 DW_LNS_negate_stmt = 0x06;
constexpr u8 DW_LNS_set_basic_block = 0x07;
constexpr u8 DW_LNS_const_add_pc = 0x08;
constexpr u8 DW_LNS_fixed_advance_pc = 0x09;
constexpr u8 DW_LNS_set_prologue_end = 0x0a;
constexpr u8 DW_LNS_set_epilogue_begin = 0x0b;
constexpr u8 DW_LNS_set_isa = 0x0c;

// Line number extended op codes
constexpr u8 DW_LNE_end_sequence = 0x01;
constexpr u8 DW_LNE_set_address = 0x02;
constexpr u8 DW_LNE_reserved_ = 0x03;
constexpr u8 DW_LNE_set_discriminator = 0x04;
constexpr u8 DW_LNE_lo_user = 0x80;
constexpr u8 DW_LNE_hi_user = 0xff;

// Macro information entry type encodings (DWARF5, Section 8.23, Table 8.28)
constexpr u8 DW_MACRO_padding = 0x00;
constexpr u8 DW_MACRO_define = 0x01;
constexpr u8 DW_MACRO_undef = 0x02;
constexpr u8 DW_MACRO_start_file = 0x03;
constexpr u8 DW_MACRO_end_file = 0x04;
constexpr u8 DW_MACRO_define_strp = 0x05;
constexpr u8 DW_MACRO_undef_strp = 0x06;
constexpr u8 DW_MACRO_import = 0x07;

constexpr u8 DW_MACRO_define_strx = 0x0b;
constexpr u8 DW_MACRO_undef_strx = 0x0c;
constexpr u8 DW_MACRO_define_sup4 = 0x0d;
constexpr u8 DW_MACRO_define_sup8 = 0x0e;
constexpr u8 DW_MACRO_undef_sup4 = 0x0f;
constexpr u8 DW_MACRO_undef_sup8 = 0x10;
constexpr u8 DW_MACRO_import_sup4 = 0x11;
constexpr u8 DW_MACRO_import_sup8 = 0x12;

constexpr u8 DW_MACRO_lo_user = 0xe0;
constexpr u8 DW_MACRO_hi_user = 0xff;

inline u64 strnlen_panic_(u8 const *str, u64 max_len) {
    for (u64 i = 0; i < max_len; i++) {
        if (str[i] == 0) {
            return i;
        }
    }

    panic();
}

struct DebugLine {
    struct Sequence {
        u64 pc_begin;
        u64 pc_end;
    };

    struct Row {
        u64 pc;
        u16 file_no;
        u16 line_no;
        u16 column_no;
    };

    Slice<CString> file_paths;
    Slice<Row> rows;
    Slice<Sequence> sequences;
    // array[sequence_idx] = (index of the last row in sequence) + 1
    Slice<u32> sequence_ends;

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
    inline static Slice<u16> parse_filepaths_panic_(Alloc *alloc,
                                                    Alloc *tmp_alloc,
                                                    Slice<u8> debug_line_str,
                                                    Cursor *input,
                                                    List<CString> *file_paths) {
        assert(alloc != tmp_alloc);

        bool overflow = false;

        SliceMut<Slice<u8>> directory_paths;
        {
            u8 directory_entry_format_count = 0;
            assert(Cursor::Status::Ok ==
                   input->read<u8>(nullptr, &directory_entry_format_count));
            assert(directory_entry_format_count == 1);

            u64 def_content_type = 0;
            assert(leb128::DecodeStatus::Ok ==
                   input->read_uleb128(nullptr, &def_content_type));
            u64 def_form_code = 0;
            assert(leb128::DecodeStatus::Ok ==
                   input->read_uleb128(nullptr, &def_form_code));

            assert(def_content_type == DW_LNCT_path);
            // means that the value is an u32 offset into the .debug_line_str
            // section
            assert(def_form_code == DW_FORM_line_strp);

            u64 directories_count = 0;
            assert(leb128::DecodeStatus::Ok ==
                   input->read_uleb128(nullptr, &directories_count));
            assert(AllocStatus::Ok ==
                   tmp_alloc->alloc<Slice<u8>>(nullptr, directories_count,
                                               &directory_paths));

            for (u64 i = 0; i < directories_count; i++) {
                u32 offset = 0;
                assert(Cursor::Status::Ok ==
                       input->read<u32>(nullptr, &offset));
                assert(offset < debug_line_str.len());
                u8 const *start = debug_line_str.offset_unsafe(offset);
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
        SliceMut<u16> file_path_map;
        {
            u8 file_name_entry_format_count = 0;
            assert(Cursor::Status::Ok ==
                   input->read<u8>(nullptr, &file_name_entry_format_count));
            assert(file_name_entry_format_count >= 2);

            // file path
            {
                u64 def_content_type = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input->read_uleb128(nullptr, &def_content_type));
                u64 def_form_code = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input->read_uleb128(nullptr, &def_form_code));
                assert(def_content_type == DW_LNCT_path);
                assert(def_form_code == DW_FORM_line_strp);
            }

            // directory index
            {
                u64 def_content_type = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input->read_uleb128(nullptr, &def_content_type));
                u64 def_form_code = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input->read_uleb128(nullptr, &def_form_code));
                assert(def_content_type == DW_LNCT_directory_index);
                assert(def_form_code == DW_FORM_udata);
            }

            u8 const num_unknown_formats =
                sub_unsafe_u8(file_name_entry_format_count, 2);

            u8 unk_formats[16];

            assert(num_unknown_formats <= sizeof(unk_formats));

            for (u8 i = 0; i < num_unknown_formats; i++) {
                u64 def_content_type = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input->read_uleb128(nullptr, &def_content_type));
                u64 def_form_code = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input->read_uleb128(nullptr, &def_form_code));
                // don't care about this as we will just skip this one.
                (void)def_content_type;
                overflow = false;
                unk_formats[i] = u64_to_u8(def_form_code, &overflow);
                assert(!overflow);
            }

            u64 file_names_count = 0;
            assert(leb128::DecodeStatus::Ok ==
                   input->read_uleb128(nullptr, &file_names_count));

            assert(AllocStatus::Ok == tmp_alloc->alloc<u16>(nullptr,
                                                            file_names_count,
                                                            &file_path_map));

            for (u64 i = 0; i < file_names_count; i++) {
                u32 offset = 0;
                assert(Cursor::Status::Ok ==
                       input->read<u32>(nullptr, &offset));
                // skip directory index
                u64 directory_index = 0;
                assert(leb128::DecodeStatus::Ok ==
                       input->read_uleb128(nullptr, &directory_index));
                // skip the unknown stuff
                for (u64 j = 0; j < num_unknown_formats; j++) {
                    switch (unk_formats[j]) {
                    case DW_FORM_line_strp: {
                        u32 dummy = 0;
                        assert(Cursor::Status::Ok ==
                               input->read<u32>(nullptr, &dummy));
                        break;
                    }
                    case DW_FORM_data16: {
                        assert(Cursor::Status::Ok == input->skip(nullptr, 16));
                        break;
                    }
                    default: {
                        panic();
                    }
                    }
                }

                assert(offset < debug_line_str.len());
                u8 const *start = debug_line_str.offset_unsafe(offset);
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
                    // should be 0 for absolute paths
                    assert(directory_index == 0);

                    abs_file_path = path_cstr;
                } else {
                    // need to create an absolute path manually using
                    // the three parts BASE_COMPILATION_DIR + DIRECTORY + FILE

                    // this can't be 0 as 0 is the base compilation directory
                    // path.
                    assert(directory_index != 0);

                    assert(directory_paths.len() > 0);
                    Slice<u8> base = directory_paths.read_unsafe(0);
                    Slice<u8> dir;
                    assert(
                        SliceStatus::Ok ==
                        directory_paths.read(nullptr, directory_index, &dir));

                    // 3 is because we have a format like `base / dir / path
                    // NULL_BYTE` 2 for slashes one for the null byte.
                    u64 const needed_len = add_u64(
                        add_u64(base.len(), 3, &overflow),
                        add_u64(dir.len(), path.len(), &overflow), &overflow);
                    assert(!overflow);

                    SliceMut<u8> abs_p;
                    assert(AllocStatus::Ok ==
                           alloc->alloc<u8>(nullptr, needed_len, &abs_p));
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
                u16 idx = U16_MAX;
                for (u64 j = 0; j < file_paths->n_written(); j++) {
                    Slice<u8> other =
                        file_paths->written().read_unsafe(j).as_slice();

                    if (other.len() == abs_file_path.as_slice().len() &&
                        other.memcmp_unsafe(abs_file_path.as_slice()) == 0) {
                        idx = u64_to_u16(j, &overflow);
                        assert(!overflow);
                        found = true;
                        break;
                    }
                }

                if (found) {
                    assert(idx != U16_MAX);
                    file_path_map.write_unsafe(i, idx);
                } else {
                    u64 const w_idx = file_paths->n_written();
                    assert(ListPushStatus::Ok ==
                           file_paths->push(nullptr, abs_file_path));
                    file_path_map.write_unsafe(i, u64_to_u16(w_idx, &overflow));
                    assert(!overflow);
                }
            }
        }

        return file_path_map.as_const();
    }

    struct RunArgs {
        Cursor *input;
        List<Sequence> *sequences;
        List<u32> *sequence_ends;
        List<Row> *rows;
        Slice<u16> file_path_map;
        u64 expected_unit_end_offset;
        i8 line_base;
        u8 line_range;
        u8 opcode_base;
        u8 minimum_instruction_length;
    };

    struct RunState {
        u64 address;
        u16 column;
        u16 line;
        u16 file;

        inline static RunState init() {
            return RunState{
                .address = 0,
                .column = 0,
                .line = 1,
                .file = 1,
            };
        }
    };

    inline static void push_row_panic_(RunArgs args, RunState state) {
        u16 file_no = 0;
        assert(SliceStatus::Ok ==
               args.file_path_map.read(nullptr, state.file, &file_no));

        Row const row = Row{
            .pc = state.address,
            .column_no = state.column,
            .line_no = state.line,
            .file_no = file_no,
        };
        assert(ListPushStatus::Ok == args.rows->push(nullptr, row));
    }

    inline static void end_sequence_panic_(RunArgs args, u64 pc_end) {
        bool overflow = false;

        u64 const sequence_index = args.sequences->n_written();

        u32 const rows_end = u64_to_u32(args.rows->n_written(), &overflow);
        assert(!overflow);

        u32 rows_start = 0;
        if (sequence_index > 0) {
            u64 const before_index = sub_unsafe_u64(sequence_index, 1);
            assert(SliceStatus::Ok == args.sequence_ends->written().read(
                                          nullptr, before_index, &rows_start));
        }

        // ignore if empty
        if (rows_start == rows_end) {
            return;
        }

        assert(rows_end > rows_start);

        // assert row pc in this sequence are strictly increasing
        {
            u64 prev_pc = 0;
            for (u64 i = rows_start; i < rows_end; i++) {
                Row row;
                assert(SliceStatus::Ok ==
                       args.rows->written().read(nullptr, i, &row));

                assert(prev_pc <= row.pc);

                prev_pc = row.pc;
            }
        }

        Row begin_row;
        assert(SliceStatus::Ok ==
               args.rows->written().read(nullptr, rows_start, &begin_row));

        u64 const last_row_index = sub_u64(rows_end, 1, &overflow);
        assert(!overflow);

        Row last_row;
        assert(SliceStatus::Ok ==
               args.rows->written().read(nullptr, last_row_index, &last_row));

        u64 const pc_begin = begin_row.pc;
        u64 const min_pc_end = add_u64(last_row.pc, 1, &overflow);
        assert(!overflow);
        assert(min_pc_end <= pc_end);

        assert(ListPushStatus::Ok ==
               args.sequences->push(nullptr, Sequence{
                                                 .pc_begin = pc_begin,
                                                 .pc_end = pc_end,
                                             }));

        assert(ListPushStatus::Ok ==
               args.sequence_ends->push(nullptr, rows_end));
    }

    inline static void run_instructions_panic_(RunArgs args) {
        RunState state = RunState::init();

        bool overflow = false;

        // We set this to false on start of every loop iteration and then
        // we set it to true only if the instruction was the end_sequence
        // instruction.
        //
        // Then we expect it to be true at exit because the last instruction is
        // defined to always be the end_sequence instruction by the DWARF
        // standard.
        bool end_sequence = true;

        while (args.input->offset() < args.expected_unit_end_offset) {
            end_sequence = false;

            u8 opcode = 0;
            assert(Cursor::Status::Ok ==
                   args.input->read<u8>(nullptr, &opcode));

            switch (opcode) {
            case DW_LNS_copy: {
                push_row_panic_(args, state);
                break;
            }
            case DW_LNS_advance_pc: {
                u64 operand = 0;
                assert(leb128::DecodeStatus::Ok ==
                       args.input->read_uleb128(nullptr, &operand));

                state.address = add_u64(state.address,
                                        mul_u64(args.minimum_instruction_length,
                                                operand, &overflow),
                                        &overflow);
                assert(!overflow);
                break;
            }
            case DW_LNS_advance_line: {
                i64 operand = 0;
                assert(leb128::DecodeStatus::Ok ==
                       args.input->read_sleb128(nullptr, &operand));
                i64 const l = add_i64(operand, state.line, &overflow);
                assert(!overflow);
                state.line = i64_to_u16(l, &overflow);
                assert(!overflow);
                break;
            }
            case DW_LNS_set_file: {
                u64 operand = 0;
                assert(leb128::DecodeStatus::Ok ==
                       args.input->read_uleb128(nullptr, &operand));

                state.file = u64_to_u16(operand, &overflow);
                assert(!overflow);
                break;
            }
            case DW_LNS_set_column: {
                u64 operand = 0;
                assert(leb128::DecodeStatus::Ok ==
                       args.input->read_uleb128(nullptr, &operand));

                state.column = u64_to_u16(operand, &overflow);
                assert(!overflow);
                break;
            }
            case DW_LNS_negate_stmt: {
                // no-op for us since we don't trace the is_stmt register.
                break;
            }
            case DW_LNS_set_basic_block: {
                // no-op for us
                break;
            }
            case DW_LNS_const_add_pc: {
                u8 const adjusted_opcode =
                    sub_u8(u8{255}, args.opcode_base, &overflow);
                u64 const operation_advance =
                    div_u8(adjusted_opcode, args.line_range, &overflow);
                state.address = add_u64(state.address,
                                        mul_u64(args.minimum_instruction_length,
                                                operation_advance, &overflow),
                                        &overflow);
                assert(!overflow);
                break;
            }
            case DW_LNS_fixed_advance_pc: {
                u16 param = 0;
                assert(Cursor::Status::Ok ==
                       args.input->read<u16>(nullptr, &param));
                state.address = add_u64(state.address, param, &overflow);
                assert(!overflow);
                break;
            }
            case DW_LNS_set_prologue_end: {
                // no-op for us
                break;
            }
            case DW_LNS_set_epilogue_begin: {
                // no-op for us
                break;
            }
            case DW_LNS_set_isa: {
                // shouldn't happen
                //
                // If want to change this to ignoring this opcode:
                // - Add code to actually skip the leb128 operand so it doesn't
                //   misalign the input stream.
                panic();
                break;
            }
            case 0: {
                u64 len = 0;
                assert(leb128::DecodeStatus::Ok ==
                       args.input->read_uleb128(nullptr, &len));

                u64 const exit_offset =
                    add_u64(args.input->offset(), len, &overflow);
                assert(!overflow);

                u8 extended_opcode = 0;
                assert(Cursor::Status::Ok ==
                       args.input->read<u8>(nullptr, &extended_opcode));

                switch (extended_opcode) {
                case DW_LNE_end_sequence: {
                    end_sequence_panic_(args, state.address);
                    state = RunState::init();
                    end_sequence = true;
                    break;
                }
                case DW_LNE_set_address: {
                    u64 new_addr = 0;
                    assert(Cursor::Status::Ok ==
                           args.input->read<u64>(nullptr, &new_addr));

                    state.address = new_addr;

                    break;
                };
                case DW_LNE_set_discriminator: {
                    u64 operand = 0;
                    assert(leb128::DecodeStatus::Ok ==
                           args.input->read_uleb128(nullptr, &operand));

                    (void)operand;
                    break;
                };
                case DW_LNE_reserved_: {
                    panic();
                    break;
                }
                default: {
                    panic();
                    break;
                }
                }

                assert(args.input->offset() == exit_offset);
                // Not 100% sure about the assert above, if that fails then
                // uncomment the line below and comment the assert line.
                //
                // The assert should probably fine as long as we don't
                // allow/ignore instructions that we don't know of by removing
                // the panic in the default arm of the switch statement.
                //
                // assert(Cursor::Status::Ok == args.input->seek(exit_offset));
                break;
            }
            default: {
                assert(opcode >= args.opcode_base);

                u8 const adjusted_opcode =
                    sub_u8(opcode, args.opcode_base, &overflow);
                u64 const operation_advance =
                    div_u8(adjusted_opcode, args.line_range, &overflow);
                state.address = add_u64(state.address,
                                        mul_u64(args.minimum_instruction_length,
                                                operation_advance, &overflow),
                                        &overflow);

                i32 const line_increment =
                    add_i32(args.line_base,
                            mod_u8(adjusted_opcode, args.line_range, &overflow),
                            &overflow);
                state.line = i32_to_u16(
                    add_i32(state.line, line_increment, &overflow), &overflow);
                assert(!overflow);

                push_row_panic_(args, state);

                break;
            }
            }
        }

        assert(end_sequence);
        assert(args.sequence_ends->n_written() == args.sequences->n_written());
    }

    inline static void parse_compilation_unit_panic_(
        Alloc *alloc, Alloc *tmp_alloc, Slice<u8> debug_line_str, Cursor *input,
        List<Row> *rows, List<Sequence> *sequences, List<u32> *sequence_ends,
        List<CString> *file_paths) {
        assert(alloc != tmp_alloc);

        bool overflow = false;

        u64 const tmp_alloc_initial_offset = tmp_alloc->offset();

        u32 unit_length = 0;
        assert(Cursor::Status::Ok == input->read<u32>(nullptr, &unit_length));
        // this means DWARF 64bit version, we only support parsing the 32bit
        // version.
        assert(unit_length != U32_MAX);
        u64 const expected_unit_end_offset =
            add_u64(input->offset(), unit_length, &overflow);
        assert(!overflow);

        u16 version = 0;
        assert(Cursor::Status::Ok == input->read<u16>(nullptr, &version));
        assert(version == 5);

        u8 address_size = 0;
        assert(Cursor::Status::Ok == input->read<u8>(nullptr, &address_size));
        assert(address_size == 8);

        u8 segment_selector_size = 0;
        assert(Cursor::Status::Ok ==
               input->read<u8>(nullptr, &segment_selector_size));
        assert(segment_selector_size == 0);

        u32 header_length = 0;
        assert(Cursor::Status::Ok == input->read<u32>(nullptr, &header_length));
        // We don't support 64bit dwarf format. Only 32bit is supported.
        assert(header_length != U32_MAX);

        u64 const expected_instructions_start_offset =
            add_u64(input->offset(), header_length, &overflow);
        assert(!overflow);

        u8 minimum_instruction_length = 0;
        assert(Cursor::Status::Ok ==
               input->read<u8>(nullptr, &minimum_instruction_length));

        u8 maximum_operations_per_instruction = 0;
        assert(Cursor::Status::Ok ==
               input->read<u8>(nullptr, &maximum_operations_per_instruction));
        // We are not on a VLIW architecture.
        assert(maximum_operations_per_instruction == 1);

        // unused, we don't care about breakpoints since we are not implementing
        // a debugger.
        {
            u8 default_is_stmt = 0;
            assert(Cursor::Status::Ok ==
                   input->read<u8>(nullptr, &default_is_stmt));
            (void)default_is_stmt;
        }

        i8 line_base = 0;
        assert(Cursor::Status::Ok == input->read<i8>(nullptr, &line_base));

        u8 line_range = 0;
        assert(Cursor::Status::Ok == input->read<u8>(nullptr, &line_range));
        assert(line_range > 0);

        u8 opcode_base = 0;
        assert(Cursor::Status::Ok == input->read<u8>(nullptr, &opcode_base));

        // unused, we will error if we encounter an unknown opcode
        overflow = false;
        u8 const num_standard_opcodes = sub_u8(opcode_base, 1, &overflow);
        assert(!overflow);

        assert(Cursor::Status::Ok ==
               input->skip(nullptr, num_standard_opcodes * sizeof(u8)));

        Slice<u16> file_path_map = parse_filepaths_panic_(
            alloc, tmp_alloc, debug_line_str, input, file_paths);

        // parsing file paths should have consumed the right number of bytes at
        // this point.
        assert(input->offset() == expected_instructions_start_offset);

        run_instructions_panic_(RunArgs{
            .input = input,
            .rows = rows,
            .sequences = sequences,
            .sequence_ends = sequence_ends,
            .file_path_map = file_path_map,
            .line_base = line_base,
            .line_range = line_range,
            .opcode_base = opcode_base,
            .minimum_instruction_length = minimum_instruction_length,
            .expected_unit_end_offset = expected_unit_end_offset,
        });

        // we should have reached the unit end by now.
        assert(input->offset() == expected_unit_end_offset);

        assert(Alloc::ResizeStatus::Ok ==
               tmp_alloc->resize(nullptr, tmp_alloc_initial_offset));
    }

    template <typename T>
    inline static List<T> make_tmp_list_panic_(Alloc *tmp_alloc, u64 len) {
        SliceMut<T> mem;
        assert(AllocStatus::Ok == tmp_alloc->alloc<T>(nullptr, len, &mem));
        return List<T>::init(mem);
    }

    inline static DebugLine init_panic(Alloc *alloc, Alloc *tmp_alloc,
                                       Slice<u8> input_bytes,
                                       Slice<u8> debug_line_str) {
        assert(alloc != nullptr);
        assert(tmp_alloc != nullptr);
        assert(alloc != tmp_alloc);

        if (input_bytes.len() == 0) {
            return DebugLine{
                .file_paths = Slice<CString>::init_dangling(),
                .rows = Slice<Row>::init_dangling(),
                .sequences = Slice<Sequence>::init_dangling(),
                .sequence_ends = Slice<u32>::init_dangling(),
            };
        }

        u64 const tmp_alloc_initial_offset = tmp_alloc->offset();

        constexpr u64 max_num_files = 1024;
        List<CString> file_paths =
            make_tmp_list_panic_<CString>(tmp_alloc, max_num_files);

        constexpr u64 max_num_sequences = u64{1} << 16;
        List<Sequence> sequences =
            make_tmp_list_panic_<Sequence>(tmp_alloc, max_num_sequences);
        List<u32> sequence_ends =
            make_tmp_list_panic_<u32>(tmp_alloc, max_num_sequences);

        constexpr u64 max_num_rows = u64{1} << 18;
        List<Row> rows = make_tmp_list_panic_<Row>(tmp_alloc, max_num_rows);

        Cursor input = Cursor::init(input_bytes);

        // iterate over compilation units
        while (!input.is_finished()) {
            parse_compilation_unit_panic_(alloc, tmp_alloc, debug_line_str,
                                          &input, &rows, &sequences,
                                          &sequence_ends, &file_paths);
            assert(sequences.n_written() == sequence_ends.n_written());
        }

        // ## move parsed data to regular alloc since we will discard the tmp
        // alloc.

        SliceMut<CString> file_paths_out;
        assert(AllocStatus::Ok ==
               alloc->copy_from<CString>(nullptr, file_paths.written(),
                                         &file_paths_out));

        SliceMut<Sequence> sequences_out;
        assert(AllocStatus::Ok ==
               alloc->copy_from<Sequence>(nullptr, sequences.written(),
                                          &sequences_out));

        SliceMut<u32> sequence_ends_out;
        assert(AllocStatus::Ok == alloc->copy_from<u32>(nullptr,
                                                        sequence_ends.written(),
                                                        &sequence_ends_out));

        SliceMut<Row> rows_out;
        assert(AllocStatus::Ok ==
               alloc->copy_from<Row>(nullptr, rows.written(), &rows_out));

        // discard tmp memory
        assert(Alloc::ResizeStatus::Ok ==
               tmp_alloc->resize(nullptr, tmp_alloc_initial_offset));

        return DebugLine{
            .sequences = sequences_out.as_const(),
            .sequence_ends = sequence_ends_out.as_const(),
            .rows = rows_out.as_const(),
            .file_paths = file_paths_out.as_const(),
        };
    }

    struct LineInfo {
        CString file_path;
        u16 line_no;
        u16 column_no;
    };

    enum class [[nodiscard]] GetLineInfosStatus : u8_c {
        Ok = 0,
        OutOfCapacity,
        Nullptr,
        NotFound,
    };

    inline LineInfo row_to_line_info_unsafe_(Row const row) const {
        return LineInfo{
            .file_path = this->file_paths.read_unsafe(row.file_no),
            .line_no = row.line_no,
            .column_no = row.column_no,
        };
    }

    inline GetLineInfosStatus get_line_infos(u64 pc,
                                             List<LineInfo> *out) const {
        if (out == nullptr) {
            return GetLineInfosStatus::Nullptr;
        }

        assert_dev(this->sequences.len() == this->sequence_ends.len());

        u64 sequence_index = 0;
        Sequence sq;
        bool found = false;
        for (u64 i = 0; i < this->sequences.len(); i++) {
            sq = this->sequences.read_unsafe(i);
            if (sq.pc_begin <= pc && sq.pc_end > pc) {
                sequence_index = i;
                found = true;
                break;
            }
        }

        if (!found) {
            return GetLineInfosStatus::NotFound;
        }

        u64 rows_begin = 0;
        if (sequence_index != 0) {
            rows_begin = this->sequence_ends.read_unsafe(sequence_index - 1);
        }
        u64 const rows_end = this->sequence_ends.read_unsafe(sequence_index);

        assert_dev(rows_end > rows_begin);

        assert_dev(this->rows.read_unsafe(rows_begin).pc <= pc);
        assert_dev(this->rows.read_unsafe(rows_begin).pc == sq.pc_begin);

        u64 row_idx = rows_begin;
        found = false;
        for (; row_idx < sub_unsafe_u64(rows_end, 1); row_idx++) {
            Row const next_row =
                this->rows.read_unsafe(add_unsafe_u64(row_idx, 1));
            if (pc < next_row.pc) {
                found = true;
                break;
            }
        }

        if (!found) {
            row_idx = sub_unsafe_u64(rows_end, 1);
        }

        u64 const row_pc = this->rows.read_unsafe(row_idx).pc;

        // Keep going forwards while row PC is same.
        // We do this to collect all source locations that correspond
        // to this PC.
        while (row_idx >= rows_begin) {
            Row const row = this->rows.read_unsafe(row_idx);

            if (row.pc != row_pc) {
                break;
            }

            if (out->is_full()) {
                return GetLineInfosStatus::OutOfCapacity;
            }

            assert_dev(ListPushStatus::Ok ==
                       out->push(nullptr, row_to_line_info_unsafe_(row)));

            if (row_idx == 0) {
                break;
            } else {
                row_idx--;
            }
        }

        return GetLineInfosStatus::Ok;
    }
};

} // namespace elf_debug_line

} // namespace zx
