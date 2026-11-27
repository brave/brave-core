#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![no_std]
//! Low-level bindings to the [zstd] library.
//!
//! [zstd]: https://facebook.github.io/zstd/

#[cfg(target_family = "wasm")]
extern crate alloc;

// Linked for the `ZSTD_*` symbols it exports, which our bindings call.
#[cfg(feature = "rust-backend")]
extern crate libzstd_rs_sys;

// Otherwise the seekable functions would only fail at link time.
#[cfg(all(feature = "rust-backend", feature = "seekable"))]
compile_error!(
    "the `seekable` feature is not supported with `rust-backend`: \
     libzstd-rs-sys does not include zstd's seekable format"
);

#[cfg(target_family = "wasm")]
mod wasm_shim;

// If running bindgen, we'll end up with the correct bindings anyway.
#[cfg(feature = "bindgen")]
include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

// The bindings used depend on a few feature flags.
#[cfg(all(not(feature = "experimental"), not(feature = "bindgen")))]
include!("bindings_zstd.rs");

#[cfg(all(
    not(feature = "experimental"),
    feature = "zdict_builder",
    not(feature = "bindgen")
))]
include!("bindings_zdict.rs");

#[cfg(all(feature = "experimental", not(feature = "bindgen")))]
include!("bindings_zstd_experimental.rs");

#[cfg(all(
    feature = "experimental",
    feature = "zdict_builder",
    not(feature = "bindgen")
))]
include!("bindings_zdict_experimental.rs");

#[cfg(all(feature = "seekable", not(feature = "bindgen")))]
include!("bindings_zstd_seekable.rs");
