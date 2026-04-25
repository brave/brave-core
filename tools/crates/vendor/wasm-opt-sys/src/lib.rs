//! Native build of `wasm-opt`.
//!
//! This crate builds the C++ code for `wasm-opt` but
//! does not provide any Rust bindings.
//!
//! Rust bindings can be found in [`wasm-opt-cxx-sys`],
//! but most users will probably want the high level APIs in the [`wasm-opt`] crate.
//!
//! [`wasm-opt-cxx-sys`]: https://docs.rs/wasm-opt-cxx-sys
//! [`wasm-opt`]: https://docs.rs/wasm-opt

/// Just here so that cxx-build becomes willing to manage the set of include
/// directories from this crate for downstream crates to include from. Perhaps
/// cxx-build should stop making it necessary to put this.
#[cxx::bridge]
mod dummy {}
