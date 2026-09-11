export module zx:tmp_file;
import :sys;
import :xoshiro256;
import :fmt;
import :uuid_v4;

export namespace zx {

enum class [[nodiscard]] MakeTmpFileStatus : u8_c {
    Ok = 0,
    Fail,
};

MakeTmpFileStatus make_tmp_file(Xoshiro256 *rand) {
    (void)rand;
    return MakeTmpFileStatus::Ok;
}

} // namespace zx
