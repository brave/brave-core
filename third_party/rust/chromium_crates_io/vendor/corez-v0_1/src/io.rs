//! I/O traits, types, and error handling for `no_std` environments.
//!
//! This module provides safe implementations of [`Read`], [`Write`],
//! [`Cursor`], [`Error`], and [`ErrorKind`] that work without the
//! standard library.

mod error;
pub use error::{Error, ErrorKind};

/// A specialized [`Result`](core::result::Result) type for I/O operations.
pub type Result<T> = core::result::Result<T, Error>;

mod traits;
pub use traits::{Read, Write};

mod impls;

mod cursor;
pub use cursor::Cursor;
