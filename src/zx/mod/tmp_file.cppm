export module zx:tmp_file;
import :sys;

export namespace zx {

enum class [[nodiscard]] MakeTmpFileStatus : u8_c {
    Ok = 0,
    Nullptr,
    CreateFileFail,
};

MakeTmpFileStatus make_tmp_file(i32 *fd) {
    if (fd == nullptr) {
        return MakeTmpFileStatus::Nullptr;
    }

    if (sys::OpenStatus::Ok !=
        sys::open((u8 const *)"/tmp",
                  sys::O_TMPFILE | sys::O_EXCL | sys::O_RDWR,
                  sys::S_IWUSR | sys::S_IRUSR, fd)) {
        return MakeTmpFileStatus::CreateFileFail;
    }

    return MakeTmpFileStatus::Ok;
}

} // namespace zx
