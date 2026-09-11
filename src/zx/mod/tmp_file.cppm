export module zx:tmp_file;
import :sys;
import :xoshiro256;
import :fmt;
import :uuid_v4;
import :slice;
import :assert;

export namespace zx {

enum class [[nodiscard]] MakeTmpFileStatus : u8_c {
    Ok = 0,
    Nullptr,
    CreateFileFail,
    UnexpectedError,
};

MakeTmpFileStatus make_tmp_file(Xoshiro256 *rand, i32* fd) {
    if (fd == nullptr) {
        return MakeTmpFileStatus::Nullptr;
    }
    
    u128 const uuid = uuid_v4::generate(rand);

    u8 fmt_buf[48];
    Fmt fmt = Fmt::init(SliceMut<u8>::init_unsafe(fmt_buf, sizeof(fmt_buf)));

    bool fmt_overflow = false;
    fmt.push_str(CString::init_unsafe("/tmp/"), &fmt_overflow);
    uuid_v4::print(uuid, false, &fmt, &fmt_overflow);
    fmt.push_char(0);
    assert_dev(!fmt_overflow);

    Slice<u8> path = fmt.written();
    assert_dev(sub_unsafe_u64(path.len(), 1) == __builtin_strlen(path.ptr()));

    i32 file = -1;

    if (sys::OpenStatus::Ok != sys::open(path.ptr(), sys::O_CREAT | sys::O_RDWR, mode, &file)) {
        return MakeTmpFileStatus::CreateFileFail;
    }

    if (sys::UnlinkAtStatus::Ok != sys::unlinkat()) {
        return MakeTmpFileStatus::UnexpectedError;
    }

    *fd = file;
    return MakeTmpFileStatus::Ok;
}

} // namespace zx
