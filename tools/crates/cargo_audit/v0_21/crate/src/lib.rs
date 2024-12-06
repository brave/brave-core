//! Audit Cargo.lock files for crates containing security vulnerabilities.
//!
//! `cargo audit` is a Cargo subcommand. Install it using the following:
//!
//! ```text
//! $ cargo install cargo-audit --locked
//! ```
//!
//! Then run `cargo audit` in the toplevel directory of any crate or workspace.
//!
//! If you wish to consume its core functionality as a library, see the
//! documentation for the `rustsec` crate:
//!
//! <https://docs.rs/rustsec/>

#![doc(
    html_logo_url = "https://raw.githubusercontent.com/RustSec/logos/main/rustsec-logo-lg.png",
    html_root_url = "https://docs.rs/cargo-audit/0.16.0"
)]
#![forbid(unsafe_code)]
#![warn(missing_docs, rust_2018_idioms, trivial_casts, unused_qualifications)]

pub mod application;
pub mod auditor;
#[cfg(feature = "binary-scanning")]
mod binary_deps;
mod binary_format;
#[cfg(feature = "binary-scanning")]
mod binary_type_filter;
pub mod commands;
pub mod config;
pub mod error;
pub mod lockfile;
mod prelude;
pub mod presenter;

/// Current version of the `cargo-audit` crate
pub const VERSION: &str = env!("CARGO_PKG_VERSION");
