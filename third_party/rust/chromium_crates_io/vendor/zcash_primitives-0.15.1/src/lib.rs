//! *General Zcash primitives.*
//!
//! `zcash_primitives` is a library that provides the core structs and functions necessary
//! for working with Zcash.
//!
//! ## Feature flags
#![doc = document_features::document_features!()]
//!

#![cfg_attr(docsrs, feature(doc_cfg))]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]
// Catch documentation errors caused by code changes.
#![deny(rustdoc::broken_intra_doc_links)]
// Temporary until we have addressed all Result<T, ()> cases.
#![allow(clippy::result_unit_err)]
// Present to reduce refactoring noise from changing all the imports inside this crate for
// the `sapling` crate extraction.
#![allow(clippy::single_component_path_imports)]

pub mod block;
pub use zcash_protocol::consensus;
pub use zcash_protocol::constants;
pub mod legacy;
pub use zcash_protocol::memo;
pub mod merkle_tree;
use sapling;
pub mod transaction;
pub use zip32;
#[cfg(zcash_unstable = "zfuture")]
pub mod extensions;
pub mod zip339;
