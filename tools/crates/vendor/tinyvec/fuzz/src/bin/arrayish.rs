use honggfuzz::fuzz;
use rutenspitz::arbitrary_stateful_operations;
use std::{fmt::Debug, iter::FromIterator, ops::RangeBounds};

use tinyvec::ArrayVec;
use tinyvec_fuzz::ArbRange;

const CAPACITY: usize = 32;

arbitrary_stateful_operations! {
    model = Vec<T>,
    tested = ArrayVec<[T; CAPACITY]>,

    type_parameters = <
        T: Default + Clone + Debug + Eq + Ord,
        R: RangeBounds<usize> + Clone + Debug,
    >,

    methods {
        equal {
            fn as_mut_slice(&mut self) -> &mut [T];
            fn as_slice(&self) -> &[T];
            fn clear(&mut self);
            fn dedup(&mut self);
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
            fn split_off(&mut self, at: usize) -> impl IntoIterator<Item = T>;
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
            Self::insert { .. } | Self::push { .. } if model.len() == CAPACITY => {
                return;
            }
            Self::resize { new_len, .. } if new_len > CAPACITY => {
                return;
            }
            Self::extend_from_slice { sli } if model.len().saturating_add(sli.len()) > CAPACITY => {
                return;
            }
            _ => {}
        }
    }
}

fn fuzz_cycle(data: &[u8]) -> Result<(), ()> {
  use arbitrary::{Arbitrary, Unstructured};

  let mut ring = Unstructured::new(&data);

  let mut model = Vec::<u16>::default();
  let mut tested: ArrayVec<[u16; CAPACITY]> = ArrayVec::new();

  let mut _op_trace = String::new();
  while let Ok(op) =
    <op::Op<u16, ArbRange<usize>> as Arbitrary>::arbitrary(&mut ring)
  {
    #[cfg(fuzzing_debug)]
    _op_trace.push_str(&format!("{}\n", op.to_string()));
    // println!("Operations trace:\n{}", _op_trace);
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
