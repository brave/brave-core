//! Implementations for IO traits exported by [`tokio` v1.x](::tokio).

// `pin_project!` generates `field: field` struct expressions for unpinned fields
// in the projection impls. Recent nightly clippy started flagging those when the
// struct definition comes from one of our own `macro_rules!` (the `impl_*!`
// macros in `crate::generic`), so allow the lint for the modules that invoke them.
#![allow(clippy::redundant_field_names)]

pub mod bufread;
pub mod write;
