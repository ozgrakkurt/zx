export module zx:elf;
import :def;
import :assert;
import :arith;
import :slice;
import :alloc;
import :panic;
import :elf_eh_frame;
import :elf_debug_line;
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
    assert(SliceStatus::Ok == data.read(nullptr, 0, &starting_byte));
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

struct Section {
    SectionHeader const *header;
    Slice<u8> body;
};

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
               buf.slice(nullptr, 0, sizeof(ElfHeader), &header_bytes));

        Slice<ElfHeader> header_list;
        assert(SliceCastStatus::Ok ==
               header_bytes.cast<ElfHeader>(nullptr, &header_list));
        assert(header_list.len() == 1);

        ElfHeader const *header = nullptr;
        assert(SliceStatus::Ok == header_list.offset(nullptr, 0, &header));
        assert(header != nullptr);

        assert(header->ehsize == sizeof(ElfHeader));
        assert(header->shentsize == sizeof(SectionHeader));

        bool overflow = false;
        u64 const section_headers_nbytes =
            mul_u64(sizeof(SectionHeader), header->shnum, &overflow);
        assert(!overflow);

        Slice<u8> section_header_bytes;
        assert(SliceStatus::Ok == buf.slice(nullptr, header->shoff,
                                            section_headers_nbytes,
                                            &section_header_bytes));

        Slice<SectionHeader> section_headers;
        assert(SliceCastStatus::Ok == section_header_bytes.cast<SectionHeader>(
                                          nullptr, &section_headers));
        assert(section_headers.len() == header->shnum);

        Slice<u8> dummy;
        for (u64 i = 0; i < section_headers.len(); i++) {
            SectionHeader const *hdr = section_headers.offset_unsafe(i);
            assert(SliceStatus::Ok ==
                   buf.slice(nullptr, hdr->offset, hdr->size, &dummy));
        }

        SectionHeader const *names_header = nullptr;
        assert(SliceStatus::Ok == section_headers.offset(nullptr,
                                                         header->shstrndx,
                                                         &names_header));

        SliceMut<Slice<u8>> section_names;
        assert(AllocStatus::Ok ==
               alloc->alloc<Slice<u8>>(nullptr, header->shnum, &section_names));

        Slice<u8> section_names_data;
        assert(SliceStatus::Ok == buf.slice(nullptr, names_header->offset,
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

    u64 get_section_index_panic_(Slice<u8> name) const {
        Slice<Slice<u8>> section_names = this->m_section_names;

        for (u64 i = 0; i < section_names.len(); i++) {
            Slice<u8> nm = section_names.read_unsafe(i);

            if (nm.equals(name)) {
                return i;
            }
        }

        panic();
    }

    Section get_section_panic(Slice<u8> name) const {
        u64 const index = this->get_section_index_panic_(name);

        SectionHeader const *header = nullptr;
        assert(SliceStatus::Ok ==
               this->m_section_headers.offset(nullptr, index, &header));

        Slice<u8> body;
        assert(SliceStatus::Ok ==
               this->m_buf.slice(nullptr, header->offset, header->size, &body));

        return Section{
            .header = header,
            .body = body,
        };
    }
};

#ifdef ZX_DEBUG_INFO

void print_stack_trace_panic(DebugLine const *debug_line, Backtrace const *bt) {
    assert(bt != nullptr);

    constexpr u64 FMT_SIZE = u64{1} << 14;
    u8 fmt_mem[FMT_SIZE];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(fmt_mem, FMT_SIZE));

    bool fmt_overflow = false;

    fmt.push_str(CString::init_unsafe("STACK TRACE:\n").as_slice(),
                 &fmt_overflow);

    constexpr u64 N_LINE_INFO = 8;
    DebugLine::LineInfo line_info_mem[N_LINE_INFO];
    List<DebugLine::LineInfo> line_infos = List<DebugLine::LineInfo>::init(
        SliceMut<DebugLine::LineInfo>::init_unsafe(line_info_mem, N_LINE_INFO));

    for (u64 i = 0; i < Backtrace::LEN; i++) {
        u64 const pc = bt->program_counters[i];

        if (pc == 0) {
            break;
        }
        line_infos.reset();
        DebugLine::GetLineInfosStatus const gl_status =
            debug_line_->get_line_infos(pc, &line_infos);

        if (gl_status == DebugLine::GetLineInfosStatus::NotFound) {
            assert(line_infos.n_written() == 0);
            fmt.push_str(CString::init_unsafe("-??\n").as_slice(),
                         &fmt_overflow);
        }

        for (u64 j = 0; j < line_infos.n_written(); j++) {
            DebugLine::LineInfo const line_info =
                line_infos.written().read_unsafe(j);
            fmt.push_char('-', &fmt_overflow);
            if (line_infos.n_written() > 1) {
                fmt.push_char('-', &fmt_overflow);
            }
            fmt.push_str(line_info.file_path.as_slice(), &fmt_overflow);
            fmt.push_char(':', &fmt_overflow);
            fmt.push_u64(line_info.line_no, &fmt_overflow);
            fmt.push_char(':', &fmt_overflow);
            fmt.push_u64(line_info.column_no, &fmt_overflow);
            fmt.push_char('\n', &fmt_overflow);
        }

        if (gl_status == DebugLine::GetLineInfosStatus::OutOfCapacity) {
            fmt.push_str(CString::init_unsafe("<TRUNCATED>\n").as_slice(),
                         &fmt_overflow);
        }
    }

    assert(!fmt_overflow);

    u64 n_written = 0;
    assert(sys::WriteStatus::Ok == sys::write(nullptr, sys::STDERR,
                                              fmt.written().ptr(),
                                              fmt.n_written(), &n_written));
    assert(n_written == fmt.n_written());
}

#endif // ifdef ZX_DEBUG_INFO

} // namespace elf

} // namespace zx
