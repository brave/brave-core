//! Another Rust implementation of [BIP-0039](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki) standard.
//!
//! ## Usage
//!
//! ```rust
//! use bip0039::{Count, Language, Mnemonic};
//!
//! /// Generates an English mnemonic with 12 words randomly
//! let mnemonic = Mnemonic::generate(Count::Words12);
//!
//! /// Gets the phrase
//! let phrase = mnemonic.phrase();
//! println!("phrase: {}", phrase);
//!
//! /// Generates the HD wallet seed from the mnemonic and the passphrase.
//! let seed = mnemonic.to_seed("");
//! println!("seed: {}", hex::encode(&seed[..]));
//!
//! /// Generates a Simplified Chinese mnemonic with 12 words randomly
//! let mnemonic = Mnemonic::generate_in(Language::SimplifiedChinese, Count::Words12);
//! println!("phrase: {}", mnemonic.phrase());
//! ```
//!

#![deny(unused_imports)]
#![deny(missing_docs)]
#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(not(feature = "std"))]
extern crate alloc;

mod error;
mod language;
mod mnemonic;

pub use self::{
    error::Error,
    language::Language,
    mnemonic::{Count, Mnemonic},
};
