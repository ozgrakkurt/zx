export module zx:elf;
// import :def;
// import :assert;
// import :arith;
// import :slice;

// export namespace zx {

// namespace elf {

// // this is supposed to be equal to what might be commonly named Elf64_Shdr
// extern "C" struct SectionHeader {
//   u32 name;       /* Section name, index in string tbl */
//   u32 type;       /* Type of section */
//   u64 flags;     /* Miscellaneous section attributes */
//   u64 addr;       /* Section virtual addr at execution */
//   u64 offset;      /* Section file offset */
//   u64 size;      /* Size of section in bytes */
//   u32 link;       /* Index of another section */
//   u32 info;       /* Additional section information */
//   u64 addralign; /* Section alignment */
//   u64 entsize;   /* Entry size if section holds table */
// };

// // size of the ELF "magic number" in bytes
// constexpr u8 IDENT_SIZE = 16;

// // this is supposed to be equal to what might be commonly named Elf64_Ehdr
// extern "C" struct ElfHeader {
//   u8 ident[IDENT_SIZE]; /* ELF "magic number" */
//   u16 type;
//   u16 machine;
//   u32 version;
//   u64 entry; /* Entry point virtual address */
//   u64 phoff;  /* Program header table file offset */
//   u64 shoff;  /* Section header table file offset */
//   u32 flags;
//   u16 ehsize;
//   u16 phentsize;
//   u16 phnum;
//   u16 shentsize;
//   u16 shnum;
//   u16 shstrndx;
// };

// struct File {
//   ElfHeader const* header;
//   Slice<u8> buf;
//   Slice<Slice<u8>> string_table;
//   Slice<Slice<u8>> section_names;
//   Slice<SectionHeader> section_headers;

//   static File init_unsafe(Slice<u8> buf) {
//     ElfHeader header = buf.slice_unsafe(0, sizeof(ElfHeader))
//       .cast_unsafe<ElfHeader>()
//       .read_unsafe(0);

//     assert(header.shentsize == sizeof(SectionHeader));

//     u64 const section_headers_nbytes =
//       mul_unsafe_u64(sizeof(SectionHeader), header.shnum);
//     Slice<SectionHeader> const section_headers =
//       buf
//         .slice_unsafe(header.shoff, section_headers_nbytes)
//         .cast_unsafe<SectionHeader>();

//     return File {};
//   }
// };

// using addr = u64;
// using half = u16;
// using shalf = i16;
// using off = u64;
// using sword = i32;
// using word = u32;
// using xword = u64;
// using sxword = i64;
// using versym = u16;

// /* These constants are for the segment types stored in the image headers */
// constexpr u64 PT_NULL = 0;
// constexpr u64 PT_LOAD = 1;
// constexpr u64 PT_DYNAMIC = 2;
// constexpr u64 PT_INTERP = 3;
// constexpr u64 PT_NOTE = 4;
// constexpr u64 PT_SHLIB = 5;
// constexpr u64 PT_PHDR = 6;
// constexpr u64 PT_TLS = 7;         /* Thread local storage segment */
// constexpr u64 PT_LOOS = 0x60000000; /* OS-specific */
// constexpr u64 PT_HIOS = 0x6fffffff; /* OS-specific */
// constexpr u64 PT_LOPROC = 0x70000000;
// constexpr u64 PT_HIPROC = 0x7fffffff;
// constexpr u64 PT_GNU_EH_FRAME = (PT_LOOS + 0x474e550);
// constexpr u64 PT_GNU_STACK = (PT_LOOS + 0x474e551);
// constexpr u64 PT_GNU_RELRO = (PT_LOOS + 0x474e552);
// constexpr u64 PT_GNU_PROPERTY = (PT_LOOS + 0x474e553);

// /* ARM MTE memory tag segment type */
// constexpr u64 PT_AARCH64_MEMTAG_MTE = (PT_LOPROC + 0x2);

// /*
//  * Extended Numbering
//  *
//  * If the real number of program header table entries is larger than
//  * or equal to PN_XNUM(0xffff), it is set to sh_info field of the
//  * section header at index 0, and PN_XNUM is set to e_phnum
//  * field. Otherwise, the section header at index 0 is zero
//  * initialized, if it exists.
//  *
//  * Specifications are available in:
//  *
//  * - Oracle: Linker and Libraries.
//  *   Part No: 817–1984–19, August 2011.
//  *   https://docs.oracle.com/cd/E18752_01/pdf/817-1984.pdf
//  *
//  * - System V ABI AMD64 Architecture Processor Supplement
//  *   Draft Version 0.99.4,
//  *   January 13, 2010.
//  *
//  http://www.cs.washington.edu/education/courses/cse351/12wi/supp-docs/abi.pdf
//  */
// constexpr u64 PN_XNUM = 0xffff;

// /* These constants define the different elf file types */
// constexpr u64 ET_NONE = 0;
// constexpr u64 ET_REL = 1;
// constexpr u64 ET_EXEC = 2;
// constexpr u64 ET_DYN = 3;
// constexpr u64 ET_CORE = 4;
// constexpr u64 ET_LOPROC = 0xff00;
// constexpr u64 ET_HIPROC = 0xffff;

// /* This is the info that is needed to parse the dynamic section of the file
// */ constexpr u64 DT_NULL = 0; constexpr u64 DT_NEEDED = 1; constexpr u64
// DT_PLTRELSZ = 2; constexpr u64 DT_PLTGOT = 3; constexpr u64 DT_HASH = 4;
// constexpr u64 DT_STRTAB = 5;
// constexpr u64 DT_SYMTAB = 6;
// constexpr u64 DT_RELA = 7;
// constexpr u64 DT_RELASZ = 8;
// constexpr u64 DT_RELAENT = 9;
// constexpr u64 DT_STRSZ = 10;
// constexpr u64 DT_SYMENT = 11;
// constexpr u64 DT_INIT = 12;
// constexpr u64 DT_FINI = 13;
// constexpr u64 DT_SONAME = 14;
// constexpr u64 DT_RPATH = 15;
// constexpr u64 DT_SYMBOLIC = 16;
// constexpr u64 DT_REL = 17;
// constexpr u64 DT_RELSZ = 18;
// constexpr u64 DT_RELENT = 19;
// constexpr u64 DT_PLTREL = 20;
// constexpr u64 DT_DEBUG = 21;
// constexpr u64 DT_TEXTREL = 22;
// constexpr u64 DT_JMPREL = 23;
// constexpr u64 DT_ENCODING = 32;
// constexpr u64 OLD_DT_LOOS = 0x60000000;
// constexpr u64 DT_LOOS = 0x6000000d;
// constexpr u64 DT_HIOS = 0x6ffff000;
// constexpr u64 DT_VALRNGLO = 0x6ffffd00;
// constexpr u64 DT_VALRNGHI = 0x6ffffdff;
// constexpr u64 DT_ADDRRNGLO = 0x6ffffe00;
// constexpr u64 DT_GNU_HASH = 0x6ffffef5;
// constexpr u64 DT_ADDRRNGHI = 0x6ffffeff;
// constexpr u64 DT_VERSYM = 0x6ffffff0;
// constexpr u64 DT_RELACOUNT = 0x6ffffff9;
// constexpr u64 DT_RELCOUNT = 0x6ffffffa;
// constexpr u64 DT_FLAGS_1 = 0x6ffffffb;
// constexpr u64 DT_VERDEF = 0x6ffffffc;
// constexpr u64 DT_VERDEFNUM = 0x6ffffffd;
// constexpr u64 DT_VERNEED = 0x6ffffffe;
// constexpr u64 DT_VERNEEDNUM = 0x6fffffff;
// constexpr u64 OLD_DT_HIOS = 0x6fffffff;
// constexpr u64 DT_LOPROC = 0x70000000;
// constexpr u64 DT_HIPROC = 0x7fffffff;

// /* This info is needed when parsing the symbol table */
// constexpr u64 STB_LOCAL = 0;
// constexpr u64 STB_GLOBAL = 1;
// constexpr u64 STB_WEAK = 2;

// constexpr u64 STN_UNDEF = 0;

// constexpr u64 STT_NOTYPE = 0;
// constexpr u64 STT_OBJECT = 1;
// constexpr u64 STT_FUNC = 2;
// constexpr u64 STT_SECTION = 3;
// constexpr u64 STT_FILE = 4;
// constexpr u64 STT_COMMON = 5;
// constexpr u64 STT_TLS = 6;

// constexpr u64 VER_FLG_BASE = 0x1;
// constexpr u64 VER_FLG_WEAK = 0x2;

// constexpr u64 st_bind(u64 x) {
//   return x >> 4;
// }

// constexpr u64 st_type(u64 x) {
//   return x & 0xf__wbu;
// }

// extern "C" {

// } // extern "C"

// } // namespace elf

// } // export namespace zx
