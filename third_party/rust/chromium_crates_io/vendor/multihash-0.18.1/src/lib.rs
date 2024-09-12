//! Multihash implementation.
//!
//! Feature Flags
//! -------------
//!
//! Multihash has lots of [feature flags], by default a table with cryptographically secure hashers
//! is created.
//!
//! Some of the features are about specific hash functions, these are ("default" marks the hashers
//! that are enabled by default):
//!
//!  - `blake2b`: (default) Enable Blake2b hashers
//!  - `blake2s`: (default) Enable Blake2s hashers
//!  - `identity`: Enable the Identity hashers (using it is discouraged as it's not a hash function
//!     in the sense that it produces a fixed sized output independent of the input size)
//!  - `sha1`: Enable SHA-1 hasher
//!  - `sha2`: (default) Enable SHA-2 hashers
//!  - `sha3`: (default) Enable SHA-3 hashers
//!  - `strobe`: Enable Strobe hashers
//!
//! In order to enable all cryptographically secure hashers, you can set the `secure-hashes`
//! feature flag (enabled by default).
//!
//! The library has support for `no_std`, if you disable the `std` feature flag.
//!
//! The `multihash-impl` feature flag (enabled by default) enables a default Multihash
//! implementation that contains some of the bundled hashers. If you want a different set of hash
//! algorithms you can change this with enabled the corresponding features.
//!
//! For example if you only need SHA2 hasher, you could set the features in the `multihash`
//! dependency like this:
//!
//! ```toml
//! multihash = { version = …, default-features = false, features = ["std", "multihash-impl", "sha2"] }
//! ```
//!
//! If you want to customize your code table even more, for example you want only one specific hash
//! digest size and not whole family, you would only enable the `derive` feature (enabled by
//! default), which enables the [`Multihash` derive], together with the hashers you want.
//!
//! The `arb` feature flag enables the quickcheck arbitrary implementation for property based
//! testing.
//!
//! For serializing the multihash there is support for [Serde] via the `serde-codec` feature and
//! the [SCALE Codec] via the `scale-codec` feature.
//!
//! [feature flags]: https://doc.rust-lang.org/cargo/reference/manifest.html#the-features-section
//! [`Multihash` derive]: crate::derive
//! [Serde]: https://serde.rs
//! [SCALE Codec]: https://github.com/paritytech/parity-scale-codec

#![deny(missing_docs, unsafe_code)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(any(test, feature = "arb"))]
mod arb;
mod error;
mod hasher;
mod hasher_impl;
mod multihash;
#[cfg(feature = "multihash-impl")]
mod multihash_impl;

pub use crate::error::{Error, Result};
pub use crate::hasher::Hasher;
pub use crate::multihash::{Multihash as MultihashGeneric, MultihashDigest};
#[cfg(feature = "derive")]
pub use multihash_derive as derive;

#[cfg(feature = "multihash-impl")]
pub use crate::multihash_impl::{Code, Multihash};

#[cfg(feature = "blake2b")]
pub use crate::hasher_impl::blake2b::{Blake2b256, Blake2b512, Blake2bHasher};
#[cfg(feature = "blake2s")]
pub use crate::hasher_impl::blake2s::{Blake2s128, Blake2s256, Blake2sHasher};
#[cfg(feature = "blake3")]
pub use crate::hasher_impl::blake3::{Blake3Hasher, Blake3_256};
pub use crate::hasher_impl::identity::{Identity256, IdentityHasher};
#[cfg(feature = "ripemd")]
pub use crate::hasher_impl::ripemd::{Ripemd160, Ripemd256, Ripemd320};
#[cfg(feature = "sha1")]
pub use crate::hasher_impl::sha1::Sha1;
#[cfg(feature = "sha2")]
pub use crate::hasher_impl::sha2::{Sha2_256, Sha2_512};
#[cfg(feature = "sha3")]
pub use crate::hasher_impl::sha3::{Keccak224, Keccak256, Keccak384, Keccak512};
#[cfg(feature = "sha3")]
pub use crate::hasher_impl::sha3::{Sha3_224, Sha3_256, Sha3_384, Sha3_512};
#[cfg(feature = "strobe")]
pub use crate::hasher_impl::strobe::{Strobe256, Strobe512, StrobeHasher};
