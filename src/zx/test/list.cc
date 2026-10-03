import zx;

using namespace zx;

void test_empty() {
  List<u8> list = List<u8>::init(SliceMut<u8>::init_dangling());

  assert(ListPushStatus::OutOfCapacity == list.push(69));

  u8 val = 69;
  assert(ListPopStatus::Empty == list.pop(&val));
}

void test_basic() {
  i32 buf[100];
  SliceMut<i32> slice = SliceMut<i32>::init_unsafe(buf, sizeof(buf) / sizeof(buf[0]));
  assert(slice.len() == 100);
  List<i32> list = List<i32>::init(slice);

  assert(list.n_written() == 0);

  assert(ListPushStatus::Ok == list.push(105));

  i32 val = -5;

  assert(ListPopStatus::Ok == list.pop(&val));

  assert(val == 105);

  assert(ListPopStatus::Empty == list.pop(&val));

  for (i32 i=0; i<100; i++) {
    list.push_panic(-i);
    assert(list.written().len() == (u64)(i + 1));
  }

  assert(list.written().len() == 100);

  for (i32 i=99; i>=0; i--) {
    assert(-i == list.pop_panic());
    assert(list.written().len() == (u64)i);
  }
}

int main() {
  test_empty();
  test_basic();
  
  return 0;
}
