//! # strobe-rs
//!
//! This is a `no_std` implementation of the [Strobe protocol framework][strobe] in pure Rust. It
//! is intended to be used as a library to build other protocols and frameworks. This
//! implementation currently only supports Keccak-f\[1600\].
//!
//! Here is a simple program that encrypts and decrypts a message:
//! ```
//! # use strobe_rs::{SecParam, Strobe};
//! # fn main() {
//! // Transmitter initializes their STROBE instance with a public context string
//! let mut tx = Strobe::new(b"correctnesstest", SecParam::B128);
//! // Receiver initializes their STROBE instance with a public context string
//! let mut rx = Strobe::new(b"correctnesstest", SecParam::B128);
//!
//! // Transmitter keys their instance
//! tx.key(b"the-combination-on-my-luggage", false);
//! // Receiver keys their instance
//! rx.key(b"the-combination-on-my-luggage", false);
//!
//! // Transmitter encrypts a message in place
//! let mut msg = b"Attack at dawn".to_vec();
//! tx.send_enc(msg.as_mut_slice(), false);
//!
//! // Rename for clarity. `msg` has been encrypted in-place.
//! let mut ciphertext = msg;
//!
//! // Receiver takes the message and decrypts it in place
//! rx.recv_enc(ciphertext.as_mut_slice(), false);
//!
//! // Rename for clarity again
//! let round_trip_msg = ciphertext;
//!
//! // Ensure that the sent message was the one received
//! assert_eq!(&round_trip_msg, b"Attack at dawn");
//! # }
//! ```
//!
//! [strobe]: https://strobe.sourceforge.io/

// The doc_cfg feature is only available in nightly. It lets us mark items in documentation as
// dependent on specific features.
#![cfg_attr(docsrs, feature(doc_cfg))]
//-------- no_std stuff --------//
#![no_std]

#[cfg(feature = "std")]
#[macro_use]
extern crate std;

// An Error type is just something that's Debug and Display
#[cfg_attr(docsrs, doc(cfg(feature = "std")))]
#[cfg(feature = "std")]
impl std::error::Error for AuthError {}

//-------- Testing stuff --------//
#[cfg(test)]
mod basic_tests;

// kat_tests requires std
#[cfg(all(test, feature = "std"))]
mod kat_tests;

//-------- Modules and exports--------//

mod keccak;
mod strobe;

pub use crate::strobe::*;
