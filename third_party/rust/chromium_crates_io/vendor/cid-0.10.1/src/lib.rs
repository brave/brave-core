//! # cid
//!
//! Implementation of [cid](https://github.com/ipld/cid) in Rust.

#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

mod cid;
mod error;
mod version;

#[cfg(any(test, feature = "arb"))]
mod arb;
#[cfg(feature = "serde-codec")]
pub mod serde;

pub use self::cid::Cid as CidGeneric;
pub use self::error::{Error, Result};
pub use self::version::Version;

#[cfg(feature = "alloc")]
pub use multibase;
pub use multihash;

/// A Cid that contains a multihash with an allocated size of 512 bits.
///
/// This is the same digest size the default multihash code table has.
///
/// If you need a CID that is generic over its digest size, use [`CidGeneric`] instead.
pub type Cid = CidGeneric<64>;
