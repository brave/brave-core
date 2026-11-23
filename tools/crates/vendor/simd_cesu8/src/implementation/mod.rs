//! This module contains certain implementation details that are used for
//! optimization purposes.
//!
//! THIS MODULE IS NOT PART OF THE PUBLIC API AND IS SEMVER EXEMPT.

pub mod fallback;
#[cfg(feature = "nightly")]
pub mod simd;
#[cfg(any(feature = "bench", not(feature = "nightly")))]
pub mod word;

#[cfg(feature = "nightly")]
pub use self::simd as active;
#[cfg(not(feature = "nightly"))]
pub use self::word as active;
