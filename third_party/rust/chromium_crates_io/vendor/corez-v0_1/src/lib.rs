//! *Safe, `no_std`-compatible I/O traits for the Zcash ecosystem.*
//!
//! When the `std` feature is enabled (the default), this crate re-exports
//! types from [`std::io`] directly. In `no_std` mode, it provides minimal
//! safe implementations of [`io::Read`], [`io::Write`], [`io::Cursor`],
//! [`io::Error`], and [`io::ErrorKind`].
//!
//! # Usage
//!
//! ```
//! use corez::io::{Read, Write};
//! ```

#![no_std]
#![forbid(unsafe_code)]
#![deny(missing_docs)]
#![deny(rustdoc::broken_intra_doc_links)]

#[cfg(feature = "alloc")]
extern crate alloc;

#[cfg(feature = "std")]
extern crate std;

/// I/O traits, types, and error handling.
///
/// When the `std` feature is enabled, this module re-exports from
/// [`std::io`]. Otherwise it provides safe `no_std` implementations.
#[cfg(feature = "std")]
pub use std::io;

#[cfg(not(feature = "std"))]
pub mod io;
