#![doc = include_str!("../README.md")]
#![cfg_attr(not(feature = "use_std"), no_std)]

#[cfg(all(not(feature = "use_std"), feature = "use_alloc"))]
extern crate alloc;

#[cfg(all(feature = "use_std", feature = "use_alloc"))]
use std as alloc;

mod util;
mod error;
pub mod core;

#[cfg(feature = "serde1")]
pub mod serde;

pub use error::{ EncodeError, DecodeError };
