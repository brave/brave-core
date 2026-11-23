//! Zstandard codecs backed by the pure-Rust [`zrip`](https://docs.rs/zrip) implementation.
//!
//! These produce and consume the same Zstandard format as the `zstd` module, and are an
//! alternative backend for it. zrip encodes levels -7 to 4 (higher levels are clamped to 4) and
//! decodes frames from any level.
//!
//! This module is unstable: it requires both the `zstd-zrip` feature and building with
//! `RUSTFLAGS="--cfg async_compression_unstable"`, and it may change in any release. `zrip`
//! requires Rust 1.89 or newer.

mod decoder;
mod encoder;
pub mod params;

pub use self::{decoder::ZripDecoder, encoder::ZripEncoder};
