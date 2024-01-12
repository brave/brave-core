//! Core ipld types used by ipld codecs.
#![deny(missing_docs)]
#![deny(warnings)]
#![cfg_attr(not(feature = "std"), no_std)]

extern crate alloc;

pub mod codec;
pub mod convert;
pub mod error;
pub mod ipld;
pub mod link;
pub mod raw;
pub mod raw_value;
#[cfg(feature = "serde-codec")]
pub mod serde;

#[cfg(feature = "arb")]
mod arb;

pub use cid;
#[cfg(feature = "std")]
pub use multibase;
pub use multihash;

#[cfg(not(feature = "std"))]
use core2::io;
#[cfg(feature = "std")]
use std::io;
