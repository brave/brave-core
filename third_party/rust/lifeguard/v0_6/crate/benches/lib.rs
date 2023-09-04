#![feature(test)]
extern crate test;
extern crate lifeguard;

#[cfg(test)]
mod tests {
  use test::Bencher;
  use test::black_box;
  use lifeguard::{Pool,Recycled,RcRecycled};

  const ITERATIONS : u32 = 10_000;

  // Calling String::new() is very close to a no-op; no actual allocation
  // is performed until bytes are pushed onto the end of the String. As such,
  // we need to explicitly ask for some space to be available to trigger allocation.
  const EMPTY_STRING_CAPACITY : usize = 4;

  #[bench]
  fn allocation_standard(b: &mut Bencher) {
    b.iter(|| {
      for _ in 0..ITERATIONS {
        let _string = black_box(String::with_capacity(EMPTY_STRING_CAPACITY));
        let _string = black_box(String::with_capacity(EMPTY_STRING_CAPACITY));
        let _string = black_box(String::with_capacity(EMPTY_STRING_CAPACITY));
        let _string = black_box(String::with_capacity(EMPTY_STRING_CAPACITY));
        let _string = black_box(String::with_capacity(EMPTY_STRING_CAPACITY));
      }
    });
  }

  #[bench]
  fn allocation_pooled(b: &mut Bencher) {
    let pool : Pool<String> = Pool::with_size(5);
    b.iter(|| {
      for _ in 0..ITERATIONS {
        let _string = pool.new();
        let _string = pool.new();
        let _string = pool.new();
        let _string = pool.new();
        let _string = pool.new();
      }
    });
  }

  #[bench]
  fn allocation_pooled_rc(b: &mut Bencher) {
    let pool : Pool<String> = Pool::with_size(5);
    b.iter(|| {
      for _ in 0..ITERATIONS {
        let _string = pool.new_rc();
        let _string = pool.new_rc();
        let _string = pool.new_rc();
        let _string = pool.new_rc();
        let _string = pool.new_rc();
      }
    });
  }

  #[bench]
  fn initialized_allocation_standard(b: &mut Bencher) {
    b.iter(|| {
      for _ in 0..ITERATIONS {
        let _string = "man".to_owned();
        let _string = "dog".to_owned();
        let _string = "cat".to_owned();
        let _string = "mouse".to_owned();
        let _string = "cheese".to_owned();
      }
    });
  }

  #[bench]
  fn initialized_allocation_pooled(b: &mut Bencher) {
    let pool : Pool<String> = Pool::with_size(5);
    b.iter(|| {
      for _ in 0..ITERATIONS {
        let _string = pool.new_from("man");
        let _string = pool.new_from("dog");
        let _string = pool.new_from("cat");
        let _string = pool.new_from("mouse");
        let _string = pool.new_from("cheese");
      }
    });
  }

  #[bench]
  fn initialized_allocation_pooled_rc(b: &mut Bencher) {
    let pool : Pool<String> = Pool::with_size(5);
    b.iter(|| {
      for _ in 0..ITERATIONS {
        let _string = pool.new_rc_from("man");
        let _string = pool.new_rc_from("dog");
        let _string = pool.new_rc_from("cat");
        let _string = pool.new_rc_from("mouse");
        let _string = pool.new_rc_from("cheese");
      }
    });
  }

  #[bench]
  fn initialized_allocation_pooled_with_cap_empty(b: &mut Bencher) {
    let pool : Pool<String> = Pool::with_size_and_max(0, 5);
    b.iter(|| {
      for _ in 0..ITERATIONS {
        let _string = pool.new_from("man");
        let _string = pool.new_from("dog");
        let _string = pool.new_from("cat");
        let _string = pool.new_from("mouse");
        let _string = pool.new_from("cheese");
      }
    });
  }

  #[bench]
  fn initialized_allocation_pooled_with_cap_full(b: &mut Bencher) {
    let pool : Pool<String> = Pool::with_size_and_max(5, 5);
    b.iter(|| {
      for _ in 0..ITERATIONS {
        let _string = pool.new_from("man");
        let _string = pool.new_from("dog");
        let _string = pool.new_from("cat");
        let _string = pool.new_from("mouse");
        let _string = pool.new_from("cheese");
      }
    });
  }

  #[bench]
  fn initialized_allocation_pooled_with_low_cap(b: &mut Bencher) {
    let pool : Pool<String> = Pool::with_size_and_max(0, 2);
    b.iter(|| {
      for _ in 0..ITERATIONS {
        let _string = pool.new_from("man");
        let _string = pool.new_from("dog");
        let _string = pool.new_from("cat");
        let _string = pool.new_from("mouse");
        let _string = pool.new_from("cheese");
      }
    });
  }

  #[bench]
  fn vec_vec_str_standard(bencher: &mut Bencher) {
      bencher.iter(|| {
          let mut v1 = Vec::new();
          for _ in 0..100 {
              let mut v2 = Vec::new();
              for _ in 0..100 {
                  v2.push(("test!").to_owned());
              }
              v1.push(v2);
          }
          v1
      });
  }

  #[bench]
  fn vec_vec_str_pooled(bencher: &mut Bencher) {
      // Note that because we're using scoped values (not Rc'ed values)
      // and we're storing items from one pool in the other,
      // the order that our pools are declared matters. 
      // Reversing them results in a compile error regarding lifetimes.
      let str_pool : Pool<String> = Pool::with_size(10000);
      let vec_str_pool : Pool<Vec<Recycled<String>>> = Pool::with_size(100);
      bencher.iter(|| {
          let mut v1 = Vec::new();
          for _ in 0..100 {
              let mut v2 = vec_str_pool.new();
              for _ in 0..100 {
                  v2.push(str_pool.new_from("test!"));
              }
              v1.push(v2);
          }
          v1
      });
  }

  #[bench]
  fn vec_vec_str_pooled_rc(bencher: &mut Bencher) {
      let vec_str_pool : Pool<Vec<RcRecycled<String>>> = Pool::with_size(100);
      let str_pool : Pool<String> = Pool::with_size(10000);
      bencher.iter(|| {
          let mut v1 = Vec::new();
          for _ in 0..100 {
              let mut v2 = vec_str_pool.new_rc();
              for _ in 0..100 {
                  v2.push(str_pool.new_rc_from("test!"));
              }
              v1.push(v2);
          }
          v1
      });
  }
}
