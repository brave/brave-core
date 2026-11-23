//! Regression test for the `tiny_vec!` macro's heap arm expanding cleanly
//! in a downstream `#![no_std]` crate, where there is no `std` prelude
//! `vec!` to mask a macro-hygiene resolution bug.
#![no_std]

use tinyvec::{tiny_vec, TinyVec};

#[allow(dead_code)]
fn expands_to_heap() -> TinyVec<[u8; 1]> {
  tiny_vec!([u8; 1] => 1, 2)
}
