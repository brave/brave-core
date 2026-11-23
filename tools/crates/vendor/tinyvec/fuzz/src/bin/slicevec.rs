use honggfuzz::fuzz;
use rutenspitz::arbitrary_stateful_operations;
use std::{fmt::Debug, iter::FromIterator, ops::RangeBounds};

use tinyvec::SliceVec;
use tinyvec_fuzz::ArbRange;

arbitrary_stateful_operations! {
    model = Vec<T>,
    tested = SliceVec<'_, T>,

    type_parameters = <
        T: Default + Clone + Debug + Eq + Ord,
        R: RangeBounds<usize> + Clone + Debug,
    >,

    methods {
        equal {
            fn as_mut_slice(&mut self) -> &mut [T];
            fn as_slice(&self) -> &[T];
            fn clear(&mut self);
            //fn dedup(&mut self);
            fn extend_from_slice(&mut self, sli: &Box<[T]>);
            fn insert(&mut self, index: usize, item: T);
            fn is_empty(&self) -> bool;
            fn is_full(&self) -> bool;
            fn len(&self) -> usize;
            fn pop(&mut self) -> Option<T>;
            fn push(&mut self, item: T);
            fn remove(&mut self, index: usize) -> T;
            fn resize(&mut self, new_len: usize, new_val: T);
            fn swap_remove(&mut self, index: usize) -> T;
            fn truncate(&mut self, new_len: usize);
        }

        equal_with(Vec::from_iter) {
            //fn split_off(&mut self, at: usize) -> impl IntoIterator<Item = T>;
            fn drain(&self, range: R) -> impl Iterator<Item = T>;
            fn iter(&self) -> impl Iterator<Item = &T>;
            fn iter_mut(&self) -> impl Iterator<Item = &mut T>;
        }
    }

    pre {
        match self {
            // We are comparing ArrayVec with a limited capacity against
            // Vec to which you can push indefinitely. This is a behavior mismatch.
            // To compensate we skip adding any elements if it would exceed capacity.
            Self::insert { .. } | Self::push { .. } if model.len() == model.capacity() => {
                return;
            }
            Self::resize { new_len, .. } if new_len > model.capacity() => {
                return;
            }
            Self::extend_from_slice { sli } if model.len().saturating_add(sli.len()) > model.capacity() => {
                return;
            }
            _ => {}
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

fn rand_array(mut x: u32) -> [u32; 32] {
  let mut tested = [0u32; 32];

  for item in tested.iter_mut() {
    x = xorshift(x);
    *item = x;
  }

  return tested;
}

fn fuzz_cycle(data: &[u8]) -> Result<(), ()> {
  use arbitrary::{Arbitrary, Unstructured};

  let mut ring = Unstructured::new(&data);

  let mut array = rand_array(seed(data));
  let mut model = array.to_vec();
  let mut tested = SliceVec::from(&mut array);

  while let Ok(op) =
    <op::Op<u32, ArbRange<usize>> as Arbitrary>::arbitrary(&mut ring)
  {
    #[cfg(fuzzing_debug)]
    eprintln!("{:?}", op);
    op.execute_and_compare(&mut model, &mut tested);
  }

  Ok(())
}

fn main() -> Result<(), ()> {
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
