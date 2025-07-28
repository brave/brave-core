#![cfg_attr(not(feature = "std"), no_std)]
#![cfg_attr(feature = "nightly", feature(external_doc))]
#![cfg_attr(feature = "nightly", doc(include = "../README.md"))]
#![doc(html_root_url = "https://docs.rs/merlin/3.0.0")]
// put this after the #![doc(..)] so it appears as a footer:
//! Note that docs will only build on nightly Rust until
//! [RFC 1990 stabilizes](https://github.com/rust-lang/rust/issues/44732).

#[cfg(target_endian = "big")]
compile_error!(
    r#"
This crate doesn't support big-endian targets, since I didn't
have one to test correctness on.  If you're seeing this message,
please file an issue!
"#
);

mod constants;
mod strobe;
mod transcript;

pub use crate::transcript::Transcript;
pub use crate::transcript::TranscriptRng;
pub use crate::transcript::TranscriptRngBuilder;
