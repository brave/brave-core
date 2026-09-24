//! Recover dependency list from compiled Rust binaries
//!
//! Most functions and types in this module only appear
//! when the `binary_scanning` feature is enabled.

// enabled unconditionally due to being able to name the type being useful
// and this type alone not pulling in any additional dependencies
mod binary_format;
pub use binary_format::*;

#[cfg(feature = "binary-scanning")]
mod binary_deps;
#[cfg(feature = "binary-scanning")]
mod binary_type_filter;

#[cfg(feature = "binary-scanning")]
pub use binary_deps::*;
#[cfg(feature = "binary-scanning")]
pub use binary_type_filter::*;
