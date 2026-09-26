import zx;

using namespace zx;

// this is how to pass a template to a template
template <template <typename T> class S> static void test_slicing_impl() {
    u16 data[] = {1, 2, 3, 4, 5};
    u64 const data_len = sizeof(data) / sizeof(data[0]);
    S<u16> const slice = S<u16>::init_unsafe(data, data_len);
    S<u16> other;

    // SLICE
    assert(SliceStatus::Ok == slice.slice(0, 5, &other));
    assert(slice.equals(other));

    assert(SliceStatus::Ok == slice.slice(4, 0, &other));
    assert(other.len() == 0);

    // edge
    assert(SliceStatus::Ok == slice.slice(5, 0, &other));
    assert(other.len() == 0);

    // oob
    assert(SliceStatus::OutOfBounds == slice.slice(6, 0, &other));
    assert(SliceStatus::OutOfBounds == slice.slice(1, 5, &other));
    assert(SliceStatus::OutOfBounds == slice.slice(0, 6, &other));

    // SLICE_UNSAFE
    other = slice.slice_unsafe(0, 5);
    assert(slice.equals(other));

    other = slice.slice_unsafe(4, 0);
    assert(other.len() == 0);

    other = slice.slice_unsafe(5, 0);
    assert(other.len() == 0);

    // SLICE_AT
    assert(SliceStatus::Ok == slice.slice_at(0, &other));
    assert(other.equals(slice));

    assert(SliceStatus::Ok == slice.slice_at(4, &other));
    assert(other.len() == 1);
    assert(other.equals(slice.slice_unsafe(4, 1)));

    assert(SliceStatus::Ok == slice.slice_at(5, &other));
    assert(other.len() == 0);

    // oob
    assert(SliceStatus::OutOfBounds == slice.slice_at(6, &other));

    // SLICE_AT_UNSAFE
    other = slice.slice_at_unsafe(0);
    assert(other.equals(slice));

    other = slice.slice_at_unsafe(4);
    assert(other.len() == 1);
    assert(other.equals(slice.slice_unsafe(4, 1)));

    other = slice.slice_at_unsafe(5);
    assert(other.len() == 0);
}

static void test_slicing() {
    test_slicing_impl<Slice>();
    test_slicing_impl<SliceMut>();
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

static void test_memfunctions() {
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
               slice.slice_unsafe(1, 2).as_const()));

    assert(SliceMemcpyStatus::SizeMismatch ==
           slice.slice_unsafe(0, 1).memcpy_from(slice.as_const()));

    data[0] = 1131;
    data[4] = 69;
    assert(SliceMemcpyStatus::Ok == slice.slice_unsafe(0, 1).memcpy_from(
                                        slice.slice_unsafe(4, 1).as_const()));
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

static void test_write_access() {
    u16 data[] = {1, 2, 3, 4, 5};
    u64 const data_len = sizeof(data) / sizeof(data[0]);
    SliceMut<u16> const slice = SliceMut<u16>::init_unsafe(data, data_len);

    // WRITE
    assert(SliceStatus::Ok == slice.write(0, 69));
    assert(data[0] == 69);

    assert(SliceStatus::Ok == slice.write(4, 1131));
    assert(data[4] == 1131);

    // oob
    assert(SliceStatus::OutOfBounds == slice.write(5, 100));
    assert(SliceStatus::OutOfBounds == slice.write(500, 100));

    // WRITE_UNSAFE
    slice.write_unsafe(0, 10);
    assert(data[0] == 10);

    slice.write_unsafe(2, 11);
    assert(data[2] == 11);

    // WRITE_UNALIGNED
    assert(SliceStatus::Ok == slice.template write_unaligned<u8>(1, 255));
    assert(data[0] == 10);  // unchanged
    assert(data[1] == 255); // low byte should be 255
    assert(data[2] == 11);  // unchanged

    assert(SliceStatus::Ok ==
           slice.template write_unaligned<u64>(0, 0x0123456789ABCDEF__wbu));
    u64 result;
    __builtin_memcpy(&result, &data[0], sizeof(u64));
    assert(result == 0x0123456789ABCDEF__wbu);

    // oob
    assert(SliceStatus::OutOfBounds ==
           slice.template write_unaligned<u64>(5, 100));
    assert(SliceStatus::OutOfBounds ==
           slice.template write_unaligned<u64>(50, 100));
    assert(SliceStatus::OutOfBounds ==
           slice.template write_unaligned<u8>(5, 100));
    assert(SliceStatus::OutOfBounds ==
           slice.template write_unaligned<u8>(50, 100));

    // Should still work for valid offset
    assert(SliceStatus::Ok == slice.template write_unaligned<u16>(4, 42));
    assert(data[4] == 42);
}

template <template <typename T> class S> static void test_read_access_impl() {
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
    assert(SliceStatus::Ok == slice.offset(0, &ptr));
    assert(*ptr == 1);

    assert(SliceStatus::Ok == slice.offset(4, &ptr));
    assert(*ptr == 5);

    // oob
    assert(SliceStatus::OutOfBounds == slice.offset(5, &ptr));
    assert(SliceStatus::OutOfBounds == slice.offset(55, &ptr));

    // OFFSET_UNSAFE
    ptr = slice.offset_unsafe(0);
    assert(*ptr == 1);

    ptr = slice.offset_unsafe(4);
    assert(*ptr == 5);

    // READ
    assert(SliceStatus::Ok == slice.read(0, &val));
    assert(val == 1);

    assert(SliceStatus::Ok == slice.read(3, &val));
    assert(val == 4);

    // READ_UNSAFE
    val = slice.read_unsafe(0);
    assert(val == 1);

    val = slice.read_unsafe(3);
    assert(val == 4);

    // READ_UNALIGNED
    u64 expected;
    __builtin_memcpy(&expected, &data[1], sizeof(u64));
    assert(SliceStatus::Ok == slice.template read_unaligned<u64>(1, &big_val));
    assert(big_val == expected);

    assert(SliceStatus::Ok == slice.template read_unaligned<u8>(2, &small_val));
    assert(small_val == 3);

    // oob
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u64>(2, &big_val));
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u64>(5, &big_val));
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u64>(77, &big_val));
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u8>(77, &small_val));
    assert(SliceStatus::OutOfBounds ==
           slice.template read_unaligned<u8>(5, &small_val));

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

static void test_read_access() {
    test_read_access_impl<Slice>();
    test_read_access_impl<SliceMut>();
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

static void test_aligned_init_dangling() {
    {
        Slice<u8> slice = Slice<u8>::template aligned_init_dangling<128>();
        assert((u64)slice.ptr() % 128 == 0);
        assert(slice.len() == 0);
    }

    SliceMut<u8> slice = SliceMut<u8>::template aligned_init_dangling<128>();
    assert((u64)slice.ptr() % 128 == 0);
    assert(slice.len() == 0);
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

static void test_from_ptr() {
    u256 data = 69;
    u256 *ptr_mut = &data;
    SliceMut<u16> slice_mut;
    assert(SliceInitStatus::Ok ==
           SliceMut<u16>::init_from_ptr(ptr_mut, &slice_mut));
    assert(slice_mut.len() == sizeof(u256) / sizeof(u16));

    u256 const *ptr = &data;
    Slice<u32> slice;
    assert(SliceInitStatus::Ok == Slice<u32>::init_from_ptr(ptr, &slice));
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

static void test_immut() {
    u32 data[20];
    Slice<u32> sli;
    assert(Slice<u32>::init(data, 20, &sli) == SliceInitStatus::Ok);

    for (u64 i = 0; i < 20; i++) {
        data[i] = 100;
    }

    data[10] = 69;
    assert(sli.read_unsafe(10) == 69);

    assert((u64)&data[10] == (u64)sli.offset_unsafe(10));

    u32 const *pp = nullptr;
    SliceStatus status = sli.offset(20, &pp);
    assert(status == SliceStatus::OutOfBounds);

    status = sli.offset(3, &pp);
    assert(status == SliceStatus::Ok);
    assert((u64)&data[3] == (u64)pp);

    Slice<u16> sli16;
    SliceCastStatus cast_status = sli.cast<u16>(&sli16);
    assert(cast_status == SliceCastStatus::Ok);
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

static void test_mut() {
    u32 data[20];
    SliceMut<u32> sli;
    assert(SliceMut<u32>::init(data, 20, &sli) == SliceInitStatus::Ok);

    sli.fill(100);

    SliceMut<u32> slik = SliceMut<u32>::init_unsafe(data, 20);
    assert(sli.as_const().equals(slik.as_const()));

    data[10] = 69;
    assert(sli.as_const().read_unsafe(10) == 69);

    sli.write_unsafe(u64{5}, u32{199});
    assert(data[5] == 199);

    assert((u64)&data[10] == (u64)sli.offset_unsafe(10));

    u32 *pp = nullptr;
    SliceStatus status = sli.offset_mut(20, &pp);
    assert(status == SliceStatus::OutOfBounds);

    status = sli.offset_mut(3, &pp);
    assert(status == SliceStatus::Ok);
    assert((u64)&data[3] == (u64)pp);

    SliceMut<u16> sli16;
    SliceCastStatus cast_status = sli.cast<u16>(&sli16);
    assert(cast_status == SliceCastStatus::Ok);
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
    test_mut();
    test_immut();
    test_cstr();
    test_from_ptr();
    test_from_ptr_unsafe();
    test_is_overlapping();
    test_aligned_init_dangling();
    test_cast_static();
    test_slicing();
    test_read_access();
    test_write_access();
    test_memfunctions();
    test_cstring();
    test_equals_same_but_diff_len();
    test_count();

    return 0;
}
