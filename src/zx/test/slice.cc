import zx;

using namespace zx;

// this is how to pass a template to a template
template <template <typename T> class S>
static void test_slicing_impl(Backtrace *err_bt) {
    u16 data[] = {1, 2, 3, 4, 5};
    u64 const data_len = sizeof(data) / sizeof(data[0]);
    S<u16> const slice = S<u16>::init_unsafe(data, data_len);
    S<u16> other;

    // SLICE
    testing::assert_with_trace(
        SliceStatus::Ok == slice.slice(err_bt, 0, 5, &other), err_bt);
    assert(slice.equals(other));

    testing::assert_with_trace(
        SliceStatus::Ok == slice.slice(err_bt, 4, 0, &other), err_bt);
    assert(other.len() == 0);

    // edge
    testing::assert_with_trace(
        SliceStatus::Ok == slice.slice(err_bt, 5, 0, &other), err_bt);
    assert(other.len() == 0);

    // oob
    assert(SliceStatus::OutOfBounds == slice.slice(err_bt, 6, 0, &other));
    assert(SliceStatus::OutOfBounds == slice.slice(err_bt, 1, 5, &other));
    assert(SliceStatus::OutOfBounds == slice.slice(err_bt, 0, 6, &other));

    // SLICE_UNSAFE
    other = slice.slice_unsafe(0, 5);
    assert(slice.equals(other));

    other = slice.slice_unsafe(4, 0);
    assert(other.len() == 0);

    other = slice.slice_unsafe(5, 0);
    assert(other.len() == 0);

    // SLICE_AT
    testing::assert_with_trace(
        SliceStatus::Ok == slice.slice_at(err_bt, 0, &other), err_bt);
    assert(other.equals(slice));

    testing::assert_with_trace(
        SliceStatus::Ok == slice.slice_at(err_bt, 4, &other), err_bt);
    assert(other.len() == 1);
    assert(other.equals(slice.slice_unsafe(4, 1)));

    testing::assert_with_trace(
        SliceStatus::Ok == slice.slice_at(err_bt, 5, &other), err_bt);
    assert(other.len() == 0);

    // oob
    assert(SliceStatus::OutOfBounds == slice.slice_at(err_bt, 6, &other));

    // SLICE_AT_UNSAFE
    other = slice.slice_at_unsafe(0);
    assert(other.equals(slice));

    other = slice.slice_at_unsafe(4);
    assert(other.len() == 1);
    assert(other.equals(slice.slice_unsafe(4, 1)));

    other = slice.slice_at_unsafe(5);
    assert(other.len() == 0);
}

static void test_slicing(Backtrace *err_bt) {
    test_slicing_impl<Slice>(err_bt);
    test_slicing_impl<SliceMut>(err_bt);
}

static void test_cstring() {
    char const *str = "hello";

    CString cstr = CString::init_unsafe(str);
    assert(cstr.as_slice().len() == 5);

    Slice<u8> slice =
        Slice<u8>::init_unsafe((u8 const *)str, __builtin_strlen(str));

    assert(slice.equals(cstr.as_slice()));
    assert(str == cstr.as_cstr());
}

static void test_memfunctions(Backtrace *err_bt) {
    u16 data[] = {1, 2, 3, 4, 5};
    u64 const data_len = sizeof(data) / sizeof(data[0]);
    SliceMut<u16> const slice = SliceMut<u16>::init_unsafe(data, data_len);

    slice.fill(69);

    for (u64 i = 0; i < 5; i++) {
        assert(data[i] == 69);
    }

    slice.memset(0xaa);

    for (u64 i = 0; i < 5; i++) {
        assert(data[i] == 0xaaaa__wbu);
    }

    assert(slice.slice_unsafe(0, 2).is_overlapping(slice.slice_unsafe(1, 2)));
    assert(SliceMemcpyStatus::Overlapping ==
           slice.slice_unsafe(0, 2).memcpy_from(
               err_bt, slice.slice_unsafe(1, 2).as_const()));

    assert(SliceMemcpyStatus::SizeMismatch ==
           slice.slice_unsafe(0, 1).memcpy_from(err_bt, slice.as_const()));

    data[0] = 1131;
    data[4] = 69;
    testing::assert_with_trace(
        SliceMemcpyStatus::Ok ==
            slice.slice_unsafe(0, 1).memcpy_from(
                err_bt, slice.slice_unsafe(4, 1).as_const()),
        err_bt);
    assert(data[0] == 69);

    data[0] = 69;
    data[4] = 1131;
    slice.slice_unsafe(0, 1).memcpy_from_unsafe(
        slice.slice_unsafe(4, 1).as_const());
    assert(data[0] == 1131);

    for (u16 i = 0; i < 5; i++) {
        data[i] = i;
    }
}

static void test_write_access(Backtrace *err_bt) {
    u16 data[] = {1, 2, 3, 4, 5};
    u64 const data_len = sizeof(data) / sizeof(data[0]);
    SliceMut<u16> const slice = SliceMut<u16>::init_unsafe(data, data_len);

    // WRITE
    testing::assert_with_trace(SliceStatus::Ok == slice.write(err_bt, 0, 69),
                               err_bt);
    assert(data[0] == 69);

    testing::assert_with_trace(SliceStatus::Ok == slice.write(err_bt, 4, 1131),
                               err_bt);
    assert(data[4] == 1131);

    // oob
    assert(SliceStatus::OutOfBounds == slice.write(err_bt, 5, 100));
    assert(SliceStatus::OutOfBounds == slice.write(err_bt, 500, 100));

    // WRITE_UNSAFE
    slice.write_unsafe(0, 10);
    assert(data[0] == 10);

    slice.write_unsafe(2, 11);
    assert(data[2] == 11);

    // WRITE_UNALIGNED
    testing::assert_with_trace(
        SliceStatus::Ok == slice.template write_unaligned<u8>(err_bt, 1, 255),
        err_bt);
    assert(data[0] == 10);  // unchanged
    assert(data[1] == 255); // low byte should be 255
    assert(data[2] == 11);  // unchanged

    testing::assert_with_trace(SliceStatus::Ok ==
                                   slice.template write_unaligned<u64>(
                                       err_bt, 0, 0x0123456789ABCDEF__wbu),
                               err_bt);
    u64 result;
    __builtin_memcpy(&result, &data[0], sizeof(u64));
    assert(result == 0x0123456789ABCDEF__wbu);

    // oob
    assert(SliceStatus::OutOfBounds ==
           slice.template write_unaligned<u64>(err_bt, 5, 100));
    assert(SliceStatus::OutOfBounds ==
           slice.template write_unaligned<u64>(err_bt, 50, 100));
    assert(SliceStatus::OutOfBounds ==
           slice.template write_unaligned<u8>(err_bt, 5, 100));
    assert(SliceStatus::OutOfBounds ==
           slice.template write_unaligned<u8>(err_bt, 50, 100));

    // Should still work for valid offset
    testing::assert_with_trace(
        SliceStatus::Ok == slice.template write_unaligned<u16>(err_bt, 4, 42),
        err_bt);
    assert(data[4] == 42);
}

template <template <typename T> class S>
static void test_read_access_impl(Backtrace *err_bt) {
    u16 data[] = {1, 2, 3, 4, 5};
    u64 const data_len = sizeof(data) / sizeof(data[0]);
    S<u16> const slice = S<u16>::init_unsafe(data, data_len);
    // for offset_ methods
    u16 const *ptr = nullptr;
    // for read_ methods
    u16 val = 69;
    // for read_unaligned_ methods
    u64 big_val = 1131;
    u8 small_val = 211;

    // OFFSET
    testing::assert_with_trace(SliceStatus::Ok == slice.offset(err_bt, 0, &ptr),
                               err_bt);
    assert(*ptr == 1);

    testing::assert_with_trace(SliceStatus::Ok == slice.offset(err_bt, 4, &ptr),
                               err_bt);
    assert(*ptr == 5);

    // oob
    assert(SliceStatus::OutOfBounds == slice.offset(err_bt, 5, &ptr));
    assert(SliceStatus::OutOfBounds == slice.offset(err_bt, 55, &ptr));

    // OFFSET_UNSAFE
    ptr = slice.offset_unsafe(0);
    assert(*ptr == 1);

    ptr = slice.offset_unsafe(4);
    assert(*ptr == 5);

    // READ
    testing::assert_with_trace(SliceStatus::Ok == slice.read(err_bt, 0, &val),
                               err_bt);
    assert(val == 1);

    testing::assert_with_trace(SliceStatus::Ok == slice.read(err_bt, 3, &val),
                               err_bt);
    assert(val == 4);

    // READ_UNSAFE
    val = slice.read_unsafe(0);
    assert(val == 1);

    val = slice.read_unsafe(3);
    assert(val == 4);

    // READ_UNALIGNED
    u64 expected;
    __builtin_memcpy(&expected, &data[1], sizeof(u64));
    testing::assert_with_trace(
        SliceStatus::Ok ==
            slice.template read_unaligned<u64>(err_bt, 1, &big_val),
        err_bt);
    assert(big_val == expected);

    testing::assert_with_trace(
        SliceStatus::Ok ==
            slice.template read_unaligned<u8>(err_bt, 2, &small_val),
        err_bt);
    assert(small_val == 3);

    // oob
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u64>(err_bt, 2, &big_val));
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u64>(err_bt, 5, &big_val));
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u64>(err_bt, 77, &big_val));
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u8>(err_bt, 77, &small_val));
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u8>(err_bt, 5, &small_val));

    // READ_UNALIGNED_UNSAFE
    __builtin_memcpy(&expected, &data[0], sizeof(u64));
    big_val = slice.template read_unaligned_unsafe<u64>(0);
    assert(big_val == expected);

    __builtin_memcpy(&expected, &data[1], sizeof(u64));
    big_val = slice.template read_unaligned_unsafe<u64>(1);
    assert(big_val == expected);

    small_val = slice.template read_unaligned_unsafe<u8>(4);
    assert(small_val == 5);
}

static void test_read_access(Backtrace *err_bt) {
    test_read_access_impl<Slice>(err_bt);
    test_read_access_impl<SliceMut>(err_bt);
}

static void test_cast_static() {
    // zero len
    {
        Slice<u16> slice = Slice<u16>::init_dangling();
        Slice<u8> slice8 = slice.cast_static<u8>();
        assert(slice8.len() == 0);
        assert((u64)slice8.ptr() == (u64)slice.ptr());
    }
    {
        SliceMut<u16> slice = SliceMut<u16>::init_dangling();
        SliceMut<u8> slice8 = slice.cast_static<u8>();
        assert(slice8.len() == 0);
        assert((u64)slice8.ptr() == (u64)slice.ptr());
    }

    // with len > 0
    u16 data = 69;
    {
        Slice<u16> slice = Slice<u16>::init_unsafe(&data, 1);
        Slice<u8> slice8 = slice.cast_static<u8>();
        assert(slice8.len() == 2);
        assert((u64)slice8.ptr() == (u64)slice.ptr());
    }
    {
        SliceMut<u16> slice = SliceMut<u16>::init_unsafe(&data, 1);
        SliceMut<u8> slice8 = slice.cast_static<u8>();
        assert(slice8.len() == 2);
        assert((u64)slice8.ptr() == (u64)slice.ptr());
    }
}

static void test_aligned_init_dangling(Backtrace *err_bt) {
    // UNSAFE

    // immut
    {
        Slice<u8> slice = Slice<u8>::aligned_init_dangling_unsafe(128);
        assert(slice.ptr() != nullptr);
        assert((u64)slice.ptr() % 128 == 0);
        assert(slice.len() == 0);
    }

    // mut
    {
        SliceMut<u8> slice = SliceMut<u8>::aligned_init_dangling_unsafe(128);
        assert(slice.ptr() != nullptr);
        assert((u64)slice.ptr() % 128 == 0);
        assert(slice.len() == 0);
    }

    // REGULAR

    // immut
    {
        // valid
        Slice<u8> slice;
        testing::assert_with_trace(
            SliceAlignedInitStatus::Ok ==
                Slice<u8>::aligned_init_dangling(err_bt, 128, &slice),
            err_bt);
        assert(slice.ptr() != nullptr);
        assert((u64)slice.ptr() % 128 == 0);
        assert(slice.len() == 0);

        // too big alignment
        assert(SliceAlignedInitStatus::InvalidAlign ==
               Slice<u8>::aligned_init_dangling(nullptr, 256, &slice));

        // too small alignment
        Slice<u32> slice32;
        assert(SliceAlignedInitStatus::InvalidAlign ==
               Slice<u32>::aligned_init_dangling(nullptr, 2, &slice32));

        // non power of two alignment
        assert(SliceAlignedInitStatus::InvalidAlign ==
               Slice<u8>::aligned_init_dangling(nullptr, 69, &slice));
    }

    // mut
    {
        // valid
        SliceMut<u8> slice;
        testing::assert_with_trace(
            SliceAlignedInitStatus::Ok ==
                SliceMut<u8>::aligned_init_dangling(err_bt, 128, &slice),
            err_bt);
        assert(slice.ptr() != nullptr);
        assert((u64)slice.ptr() % 128 == 0);
        assert(slice.len() == 0);

        // too big alignment
        assert(SliceAlignedInitStatus::InvalidAlign ==
               SliceMut<u8>::aligned_init_dangling(nullptr, 256, &slice));

        // too small alignment
        SliceMut<u32> slice32;
        assert(SliceAlignedInitStatus::InvalidAlign ==
               SliceMut<u32>::aligned_init_dangling(nullptr, 2, &slice32));

        // non power of two alignment
        assert(SliceAlignedInitStatus::InvalidAlign ==
               SliceMut<u8>::aligned_init_dangling(nullptr, 69, &slice));
    }
}

static void test_is_overlapping() {
    assert(
        !Slice<u8>::init_dangling().is_overlapping(Slice<u8>::init_dangling()));
    assert(!Slice<u256>::init_dangling().is_overlapping(
        Slice<u256>::init_dangling()));

    u256 arr[16];

    assert(!Slice<u256>::init_unsafe(arr, 16).is_overlapping(
        Slice<u256>::init_unsafe(arr, 0)));
    assert(Slice<u256>::init_unsafe(arr, 16).is_overlapping(
        Slice<u256>::init_unsafe(arr, 1)));
}

static void test_from_ptr(Backtrace *err_bt) {
    u256 data = 69;
    u256 *ptr_mut = &data;
    SliceMut<u16> slice_mut;
    testing::assert_with_trace(
        SliceInitStatus::Ok ==
            SliceMut<u16>::init_from_ptr(err_bt, ptr_mut, &slice_mut),
        err_bt);
    assert(slice_mut.len() == sizeof(u256) / sizeof(u16));

    u256 const *ptr = &data;
    Slice<u32> slice;
    testing::assert_with_trace(
        SliceInitStatus::Ok == Slice<u32>::init_from_ptr(err_bt, ptr, &slice),
        err_bt);
    assert(slice.len() == sizeof(u256) / sizeof(u32));
}

static void test_from_ptr_unsafe() {
    u256 data = 69;
    u256 *ptr_mut = &data;
    SliceMut<u16> const slice_mut =
        SliceMut<u16>::init_from_ptr_unsafe(ptr_mut);
    assert(slice_mut.len() == sizeof(u256) / sizeof(u16));

    u256 const *ptr = &data;
    Slice<u32> const slice = Slice<u32>::init_from_ptr_unsafe(ptr);
    assert(slice.len() == sizeof(u256) / sizeof(u32));
}

static void test_cstr() {
    CString const v = CString::init_unsafe("anan");
    assert(v.as_slice().len() == 4);
}

static void test_immut(Backtrace *err_bt) {
    u32 data[20];
    Slice<u32> sli;
    testing::assert_with_trace(Slice<u32>::init(err_bt, data, 20, &sli) ==
                                   SliceInitStatus::Ok,
                               err_bt);

    for (u64 i = 0; i < 20; i++) {
        data[i] = 100;
    }

    data[10] = 69;
    assert(sli.read_unsafe(10) == 69);

    assert((u64)&data[10] == (u64)sli.offset_unsafe(10));

    u32 const *pp = nullptr;
    SliceStatus status = sli.offset(err_bt, 20, &pp);
    assert(status == SliceStatus::OutOfBounds);

    status = sli.offset(err_bt, 3, &pp);
    testing::assert_with_trace(status == SliceStatus::Ok, err_bt);
    assert((u64)&data[3] == (u64)pp);

    Slice<u16> sli16;
    SliceCastStatus cast_status = sli.cast<u16>(err_bt, &sli16);
    testing::assert_with_trace(cast_status == SliceCastStatus::Ok, err_bt);
    assert(sli16.len() == mul_unsafe_u64(sli.len(), 2));

    Slice<u8> sli8 = sli.cast_unsafe<u8>();
    assert(sli8.len() == mul_unsafe_u64(sli.len(), 4));

    Slice<u32> slisli = sli.slice_unsafe(0, sli.len() / 2);
    assert(slisli.len() == sli16.len() / 4);

    assert(sli8.read_unaligned_unsafe<u32>(0) == sli.read_unsafe(0));

    assert(sli8.equals(sli.cast_unsafe<u8>()));

    assert(sli8.is_overlapping(sli.cast_unsafe<u8>()));

    Slice<u8> dangler = Slice<u8>::init_dangling();
    assert(!sli8.is_overlapping(dangler));

    assert(dangler.compare(sli8) == Comparison::Less);
    assert(sli8.compare(dangler) == Comparison::Greater);
    assert(sli8.compare(sli.cast_unsafe<u8>()) == Comparison::Equal);
}

static void test_mut(Backtrace *err_bt) {
    u32 data[20];
    SliceMut<u32> sli;
    testing::assert_with_trace(SliceMut<u32>::init(err_bt, data, 20, &sli) ==
                                   SliceInitStatus::Ok,
                               err_bt);

    sli.fill(100);

    SliceMut<u32> slik = SliceMut<u32>::init_unsafe(data, 20);
    assert(sli.as_const().equals(slik.as_const()));

    data[10] = 69;
    assert(sli.as_const().read_unsafe(10) == 69);

    sli.write_unsafe(u64{5}, u32{199});
    assert(data[5] == 199);

    assert((u64)&data[10] == (u64)sli.offset_unsafe(10));

    u32 *pp = nullptr;
    SliceStatus status = sli.offset_mut(err_bt, 20, &pp);
    assert(status == SliceStatus::OutOfBounds);

    status = sli.offset_mut(err_bt, 3, &pp);
    testing::assert_with_trace(status == SliceStatus::Ok, err_bt);
    assert((u64)&data[3] == (u64)pp);

    SliceMut<u16> sli16;
    SliceCastStatus cast_status = sli.cast<u16>(err_bt, &sli16);
    testing::assert_with_trace(cast_status == SliceCastStatus::Ok, err_bt);
    assert(sli16.len() == mul_unsafe_u64(sli.len(), 2));

    SliceMut<u8> sli8 = sli.cast_unsafe<u8>();
    assert(sli8.len() == mul_unsafe_u64(sli.len(), 4));

    SliceMut<u32> slisli = sli.slice_unsafe(0, sli.len() / 2);
    assert(slisli.len() == sli16.len() / 4);

    assert(sli8.as_const().read_unaligned_unsafe<u32>(0) ==
           sli.as_const().read_unsafe(0));

    assert(sli8.as_const().equals(sli.cast_unsafe<u8>().as_const()));

    assert(sli8.as_const().is_overlapping(sli.cast_unsafe<u8>().as_const()));

    SliceMut<u8> dangler = SliceMut<u8>::init_dangling();
    assert(!sli8.as_const().is_overlapping(dangler.as_const()));

    assert(dangler.as_const().compare(sli8.as_const()) == Comparison::Less);
    assert(sli8.as_const().compare(dangler.as_const()) == Comparison::Greater);
    assert(sli8.as_const().compare(sli.cast_unsafe<u8>().as_const()) ==
           Comparison::Equal);
}

void test_equals_same_but_diff_len() {
    u8 a[] = {1, 2, 3, 4, 5};
    u8 b[] = {1, 2, 3, 4, 5, 6};

    assert(!Slice<u8>::init_unsafe(a, sizeof(a))
                .equals(Slice<u8>::init_unsafe(b, sizeof(b))));

    assert(!SliceMut<u8>::init_unsafe(a, sizeof(a))
                .equals(SliceMut<u8>::init_unsafe(b, sizeof(b))));
}

void test_count() {
    u8 a[] = {1, 2, 1, 2, 1, 1, 3, 5};

    assert(Slice<u8>::init_unsafe(a, sizeof(a)).count(1) == 4);
    assert(Slice<u8>::init_unsafe(a, sizeof(a)).count(2) == 2);
    assert(Slice<u8>::init_unsafe(a, sizeof(a)).count(69) == 0);

    assert(SliceMut<u8>::init_unsafe(a, sizeof(a)).count(1) == 4);
    assert(SliceMut<u8>::init_unsafe(a, sizeof(a)).count(2) == 2);
    assert(SliceMut<u8>::init_unsafe(a, sizeof(a)).count(69) == 0);
}

int main(void) {
    Backtrace err_bt = Backtrace::init();

    test_mut(&err_bt);
    test_immut(&err_bt);
    test_cstr();
    test_from_ptr(&err_bt);
    test_from_ptr_unsafe();
    test_is_overlapping();
    test_aligned_init_dangling(&err_bt);
    test_cast_static();
    test_slicing(&err_bt);
    test_read_access(&err_bt);
    test_write_access(&err_bt);
    test_memfunctions(&err_bt);
    test_cstring();
    test_equals_same_but_diff_len();
    test_count();

    return 0;
}
