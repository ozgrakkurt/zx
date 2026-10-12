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
import :fmt;
import :sys;
import :cpu_context;
import :elf_eh_frame;
import :elf_debug_line;
import :page_alloc;

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
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));
    
    assert(headers.len() == out.len());
    if (out.len() == 0) {
        return;
    }

    u8 starting_byte = 0xaa;
    assert(SliceStatus::Ok == data.read(&err_ctx, 0, &starting_byte));
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

        // Dummy context to pass to fallible functions. It is otherwise unused.
        CPUContext err_ctx;
        __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

        Slice<u8> header_bytes;
        assert(SliceStatus::Ok ==
               buf.slice(&err_ctx, 0, sizeof(ElfHeader), &header_bytes));

        Slice<ElfHeader> header_list;
        assert(SliceCastStatus::Ok ==
               header_bytes.cast<ElfHeader>(&err_ctx, &header_list));
        assert(header_list.len() == 1);

        ElfHeader const *header = nullptr;
        assert(SliceStatus::Ok == header_list.offset(&err_ctx, 0, &header));
        assert(header != nullptr);

        assert(header->ehsize == sizeof(ElfHeader));
        assert(header->shentsize == sizeof(SectionHeader));

        bool overflow = false;
        u64 const section_headers_nbytes =
            mul_u64(sizeof(SectionHeader), header->shnum, &overflow);
        assert(!overflow);

        Slice<u8> section_header_bytes;
        assert(SliceStatus::Ok == buf.slice(&err_ctx, header->shoff,
                                            section_headers_nbytes,
                                            &section_header_bytes));

        Slice<SectionHeader> section_headers;
        assert(SliceCastStatus::Ok ==
               section_header_bytes.cast<SectionHeader>(&err_ctx, &section_headers));
        assert(section_headers.len() == header->shnum);

        Slice<u8> dummy;
        for (u64 i = 0; i < section_headers.len(); i++) {
            SectionHeader const *hdr = section_headers.offset_unsafe(i);
            assert(SliceStatus::Ok ==
                   buf.slice(&err_ctx, hdr->offset, hdr->size, &dummy));
        }

        SectionHeader const *names_header = nullptr;
        assert(SliceStatus::Ok ==
               section_headers.offset(&err_ctx, header->shstrndx, &names_header));

        SliceMut<Slice<u8>> section_names;
        assert(AllocStatus::Ok ==
               alloc->alloc<Slice<u8>>(&err_ctx, header->shnum, &section_names));

        Slice<u8> section_names_data;
        assert(SliceStatus::Ok == buf.slice(&err_ctx, names_header->offset,
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
        CPUContext err_ctx;
        __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

        u64 const index = this->get_section_index_panic_(name);

        SectionHeader const *header = nullptr;
        assert(SliceStatus::Ok ==
               this->m_section_headers.offset(&err_ctx, index, &header));

        Slice<u8> body;
        assert(SliceStatus::Ok ==
               this->m_buf.slice(&err_ctx, header->offset, header->size, &body));

        return Section{
            .header = header,
            .body = body,
        };
    }
};

Slice<u8> read_self_exe_panic(Alloc *alloc) {
    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    i32 fd = -1;
    assert(sys::OpenStatus::Ok ==
           sys::open(&err_ctx, (u8 const *)"/proc/self/exe", sys::O_RDONLY, 0, &fd));
    assert(fd != -1);

    sys::StatX statx;
    __builtin_memset(&statx, 0, sizeof(statx));
    assert(sys::StatXStatus::Ok == sys::statx(&err_ctx, fd, (u8 const *)"",
                                              sys::AT_EMPTY_PATH,
                                              sys::STATX_SIZE, &statx));
    assert((statx.stx_mask & sys::STATX_SIZE) != 0);

    u64 const file_size = statx.stx_size;

    SliceMut<u8> buf;
    // Need the explicit alignment since we cast pointers into this buffer into struct pointers
    // while parsing the ELF format.
    assert(AllocStatus::Ok == alloc->aligned_alloc<8, u8>(&err_ctx, file_size, &buf));

    u64 n_read = 0;
    assert(sys::ReadStatus::Ok == sys::read(&err_ctx, fd, buf.ptr(), buf.len(), &n_read));
    assert(n_read == file_size);

    return buf.as_const();
}

void print_stack_trace_panic(CPUContext const *cpu_ctx) {
    assert(cpu_ctx != nullptr);

    CPUContext err_ctx;
    __builtin_memset(&err_ctx, 0, sizeof(err_ctx));

    constexpr u64 MEM_SIZE = u64{1} << 25;
    u8 *mem = nullptr;
    assert(page_alloc::AllocStatus::Ok ==
           page_alloc::alloc_4kb(&err_ctx, MEM_SIZE, &mem));
    assert(mem != nullptr);

    constexpr u64 ALLOC_SIZE = u64{1} << 22;
    static_assert(MEM_SIZE > ALLOC_SIZE);
    constexpr u64 TMP_ALLOC_SIZE = MEM_SIZE - ALLOC_SIZE;

    Alloc alloc = Alloc::init(SliceMut<u8>::init_unsafe(mem, ALLOC_SIZE));
    Alloc tmp_alloc = Alloc::init(
        SliceMut<u8>::init_unsafe(&mem[ALLOC_SIZE], TMP_ALLOC_SIZE));

    Slice<u8> buf = read_self_exe_panic(&alloc);
    File const file = elf::File::init_panic(&alloc, buf);

    Section const eh_frame_section =
        file.get_section_panic(CString::init_unsafe(".eh_frame").as_slice());
    Section const debug_line_section =
        file.get_section_panic(CString::init_unsafe(".debug_line").as_slice());
    Section const debug_line_str_section = file.get_section_panic(
        CString::init_unsafe(".debug_line_str").as_slice());

    EhFrame const eh_frame =
        EhFrame::init_panic(&alloc, &tmp_alloc, eh_frame_section.header->addr,
                            eh_frame_section.body);
    DebugLine const debug_line =
        DebugLine::init_panic(&alloc, &tmp_alloc, debug_line_section.body,
                              debug_line_str_section.body);

    constexpr u64 FMT_SIZE = u64{1} << 14;

    SliceMut<u8> fmt_mem;
    assert(AllocStatus::Ok == alloc.alloc<u8>(&err_ctx, FMT_SIZE, &fmt_mem));
    Fmt fmt = Fmt::init(fmt_mem);

    UnwindContext unwind_ctx = UnwindContext::init(cpu_ctx);

    bool fmt_overflow = false;

    fmt.push_str(CString::init_unsafe("STACK TRACE:\n").as_slice(),
                 &fmt_overflow);

    while (unwind_ctx.rip() != 0) {
        u64 const pc = sub_unsafe_u64(unwind_ctx.rip(), 1);
        elf::DebugLine::LineInfo line_info;
        if (elf::DebugLine::GetLineInfoStatus::Ok ==
            debug_line.get_line_info(&err_ctx, pc, &line_info)) {
            fmt.push_char('-', &fmt_overflow);
            fmt.push_str(line_info.file_path.as_slice(), &fmt_overflow);
            fmt.push_char(':', &fmt_overflow);
            fmt.push_u64(line_info.line_no, &fmt_overflow);
            fmt.push_char(':', &fmt_overflow);
            fmt.push_u64(line_info.column_no, &fmt_overflow);
            fmt.push_char('\n', &fmt_overflow);
        } else {
            fmt.push_str(CString::init_unsafe("-??\n").as_slice(),
                         &fmt_overflow);
        }

        eh_frame.step_panic(&tmp_alloc, &unwind_ctx);
    }

    assert(!fmt_overflow);

    u64 n_written = 0;
    assert(sys::WriteStatus::Ok == sys::write(&err_ctx, sys::STDERR, fmt.written().ptr(),
                                              fmt.n_written(), &n_written));
    assert(n_written == fmt.n_written());

    assert(page_alloc::FreeStatus::Ok == page_alloc::free(&err_ctx, mem, MEM_SIZE));
}

} // namespace elf

} // namespace zx
