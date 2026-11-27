use honggfuzz::fuzz;
use rutenspitz::arbitrary_stateful_operations;
use std::{convert::TryInto, fmt::Debug};

use tinyvec::{TinyVec, TinyVecDrain};
use tinyvec_fuzz::ArbRange;

const CAPACITY: usize = 28;

arbitrary_stateful_operations! {
    model = std::vec::Drain<'_, u32>,
    tested = TinyVecDrain<'_, [u32; CAPACITY]>,

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

fn rand_tinyvec(mut x: u32) -> TinyVec<[u32; CAPACITY]> {
  let mut tested: TinyVec<[u32; CAPACITY]> = Default::default();
  let len = xorshift(x) as u8; /* Effectively modulo 256 */

  for _ in 0..len {
    x = xorshift(x);
    tested.push(x);
  }

  return tested;
}

fn fuzz_cycle(data: &[u8]) -> Result<(), ()> {
  use arbitrary::{Arbitrary, Unstructured};

  let mut ring = Unstructured::new(&data);

  let rng = seed(data);
  let mut tested = rand_tinyvec(rng);
  let mut model: Vec<u32> = Vec::from(tested.as_slice());

  let mut tested = tested.drain(..);
  let mut model = model.drain(..);

  while let Ok(op) = op::Op::arbitrary(&mut ring) {
    op.execute_and_compare(&mut model, &mut tested);
  }

  Ok(())
}

fn main() -> Result<(), ()> {
  better_panic::install();

  loop {
    fuzz!(|data: &[u8]| {
      let _ = fuzz_cycle(data);
    });
  }
}
