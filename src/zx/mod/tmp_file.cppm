export module zx:tmp_file;
import :def;
import :sys;
import :backtrace;

export namespace zx {

namespace tmp_file {

enum class [[nodiscard]] OpenStatus : u8_c {
    Ok = 0,
    Nullptr,
    SysOpenFail,
};

inline OpenStatus open(Backtrace *err_bt, i32 *fd) {
    if (fd == nullptr) [[unlikely]] {
        Backtrace::collect(err_bt);
        return OpenStatus::Nullptr;
    }

    if (sys::OpenStatus::Ok !=
        sys::open(err_bt, (u8 const *)"/tmp",
                  sys::O_TMPFILE | sys::O_EXCL | sys::O_RDWR,
                  sys::S_IWUSR | sys::S_IRUSR, fd)) [[unlikely]] {
        return OpenStatus::SysOpenFail;
    }

    return OpenStatus::Ok;
}

} // namespace tmp_file

} // namespace zx
