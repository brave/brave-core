//! Bare-minimum multihash data structure.
//!
//! This crate defines a `no_std` compatible data structures for representing a `Multihash`.
//!
//! It does not offer any hashing, instead you are encouraged to either do the hashing yourself.
//! Alternatively, you can use an existing code table or make your own code table.
//!
//! The [`multihash-codetable`] crate defines a set of hashes to get started quickly.
//! To make your own codetable, use the [`multihash-derive`] crate.
//!
//! The `arb` feature flag enables the quickcheck arbitrary implementation for property based
//! testing.
//!
//! For serializing the multihash there is support for [Serde] via the `serde-codec` feature and
//! the [SCALE Codec] via the `scale-codec` feature.
//!
//! [Serde]: https://serde.rs
//! [SCALE Codec]: https://github.com/paritytech/parity-scale-codec
//! [`multihash-derive`]: https://docs.rs/multihash-derive
//! [`multihash-codetable`]: https://docs.rs/multihash-codetable

#![deny(missing_docs, unsafe_code)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(any(test, feature = "arb"))]
mod arb;
mod error;
mod multihash;
#[cfg(feature = "serde")]
mod serde;

/// Multihash result.
#[deprecated(note = "Use `Result<T, multihash::Error>` instead")]
pub type Result<T> = core::result::Result<T, Error>;

pub use crate::error::Error;
pub use crate::multihash::Multihash;

/// Deprecated type-alias for the [`Multihash`] type.
#[deprecated(since = "0.18.0", note = "Use `multihash::Multihash instead.")]
pub type MultihashGeneric<const N: usize> = Multihash<N>;
