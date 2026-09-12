export module zx:tmp_file;
import :sys;

export namespace zx {

namespace tmp_file {

enum class [[nodiscard]] OpenStatus : u8_c {
    Ok = 0,
    Nullptr,
    SysOpenFail,
};

OpenStatus open(i32 *fd) {
    if (fd == nullptr) {
        return OpenStatus::Nullptr;
    }

    if (sys::OpenStatus::Ok !=
        sys::open((u8 const *)"/tmp",
                  sys::O_TMPFILE | sys::O_EXCL | sys::O_RDWR,
                  sys::S_IWUSR | sys::S_IRUSR, fd)) {
        return OpenStatus::SysOpenFail;
    }

    return OpenStatus::Ok;
}

} // namespace tmp_file

} // namespace zx
