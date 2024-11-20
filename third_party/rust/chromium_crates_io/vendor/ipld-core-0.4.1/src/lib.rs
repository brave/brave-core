#![doc = include_str!("../README.md")]
#![deny(missing_docs)]
#![deny(warnings)]
#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

#[cfg(all(feature = "std", feature = "codec"))]
pub mod codec;
pub mod convert;
pub mod ipld;
#[cfg(feature = "serde")]
pub mod serde;

#[cfg(feature = "arb")]
mod arb;
mod macros;

pub use cid;

// This is a hack to get those types working in the `ipld!` macro with and without `no_std`. The
// idea is from
// https://stackoverflow.com/questions/71675411/refer-to-an-extern-crate-in-macro-expansion/71675639#71675639
#[doc(hidden)]
pub mod __private_do_not_use {
    pub use alloc::{collections::BTreeMap, vec};
}
