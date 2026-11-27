use honggfuzz::fuzz;
use rutenspitz::{arbitrary_stateful_operations, OutcomePanic};
use std::{convert::TryInto, fmt::Debug};

use tinyvec::{ArrayVec, ArrayVecIterator};
use tinyvec_fuzz::ArbRange;

const CAPACITY: usize = 28;

arbitrary_stateful_operations! {
    model = std::vec::IntoIter<u32>,
    tested = ArrayVecIterator<[u32; CAPACITY]>,

    methods {
        equal {
            fn next(&mut self) -> Option<u32>;
            fn nth(&mut self, n: usize) -> Option<u32>;
            fn next_back(&mut self) -> Option<u32>;
            fn nth_back(&mut self, n: usize) -> Option<u32>;
            fn size_hint(&self) -> (usize, Option<usize>);
        }
    }
}

fn xorshift(x: u32) -> u32 {
  let x = x ^ (x << 13);
  let x = x ^ (x >> 17);
  let x = x ^ (x << 5);
  return x;
}

fn seed(data: &[u8]) -> u32 {
  let mut rng = [1u8; 4];
  let len = if data.len() > 4 { 4 } else { data.len() };
  rng[..len].copy_from_slice(&data[..len]);
  u32::from_ne_bytes(rng)
}

fn rand_arrvec(mut x: u32) -> ArrayVec<[u32; CAPACITY]> {
  let mut tested: ArrayVec<[u32; CAPACITY]> = ArrayVec::new();
  let len = xorshift(x) as usize % CAPACITY;
  tested.set_len(len);

  for item in tested.iter_mut() {
    x = xorshift(x);
    *item = x;
  }

  return tested;
}

fn fuzz_cycle(data: &[u8]) -> Result<(), ()> {
  use arbitrary::{Arbitrary, Unstructured};

  let mut ring = Unstructured::new(&data);

  let rng = seed(data);
  let tested = rand_arrvec(rng);
  let model: Vec<u32> = Vec::from(tested.as_slice());

  let mut tested = tested.into_iter();
  let mut model = model.into_iter();

  while let Ok(op) = op::Op::arbitrary(&mut ring) {
    #[cfg(fuzzing_debug)]
    eprintln!("{:?}", op);
    op.execute_and_compare(&mut model, &mut tested);
  }

  Ok(())
}

fn main() -> Result<(), ()> {
  //better_panic::install();
  std::panic::set_hook(Box::new(|panic_info| {
    if let Some(outpanic) =
      panic_info.payload().downcast_ref::<rutenspitz::OutcomePanic>()
    {
      eprintln!("{}", outpanic.0);
      std::process::abort();
    }
  }));

  loop {
    fuzz!(|data: &[u8]| {
      let _ = fuzz_cycle(data);
    });
  }
}
