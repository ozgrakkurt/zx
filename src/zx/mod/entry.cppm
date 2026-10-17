export module zx:entry;
import :slice;
import :backtrace;

export namespace zx {

// each binary should define this function to implement "main"
void entry(Slice<CString> cli_args, Slice<CString> env);

} // namespace zx
