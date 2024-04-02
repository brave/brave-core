//! This crate provides low-level types for implementing Zcash specifications. When a
//! common function defined in [the Zcash Protocol Specification] is used in multiple
//! protocols (for example the Sapling and Orchard shielded protocols), a corresponding
//! common type in this crate can be shared between implementations (for example by the
//! [`sapling-crypto`] and [`orchard`] crates).
//!
//! [the Zcash Protocol Specification]: https://zips.z.cash/protocol/protocol.pdf
//! [`sapling-crypto`]: https://crates.io/crates/sapling-crypto
//! [`orchard`]: https://crates.io/crates/orchard

#![no_std]
#![deny(unsafe_code)]
#![deny(rustdoc::broken_intra_doc_links)]

mod prf_expand;
pub use prf_expand::PrfExpand;
