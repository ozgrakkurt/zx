export module zx:entry;
import :slice;
import :elf_eh_frame;
import :elf_debug_line;

export namespace zx {

struct EntryArgs {
    Slice<CString> cli_args;
    Slice<CString> env;
    // nullptr if debug info generation isn't enabled. (ifndef ZX_DEBUG_INFO)
    elf::EhFrame const *elf_eh_frame;
    // nullptr if debug info generation isn't enabled. (ifndef ZX_DEBUG_INFO)
    elf::DebugLine const *elf_debug_line;
};

// each binary should define this function to implement "main"
void entry(EntryArgs args);

} // namespace zx
