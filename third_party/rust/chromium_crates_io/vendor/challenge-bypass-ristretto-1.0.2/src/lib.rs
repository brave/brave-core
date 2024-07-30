#![no_std]
#![cfg_attr(all(feature = "alloc", not(feature = "std")), feature(alloc))]
#![deny(missing_docs)]
#![cfg_attr(feature = "cargo-clippy", allow(clippy::many_single_char_names))]
//! [`src/dleq_merlin.rs`]: javascript:void(0)
//! [`tests/e2e.rs`]: javascript:void(0)
//! [a more detailed writeup is also available]: #cryptographic-protocol
//! [`T`]: struct.TokenPreimage.html#method.T
//! [`TokenPreimage`]: voprf/struct.TokenPreimage.html
//! [`Token`]: voprf/struct.Token.html
//! [`BlindedToken`]: voprf/struct.BlindedToken.html
//! [`PublicKey`]: voprf/struct.PublicKey.html
//! [`SigningKey`]: voprf/struct.SigningKey.html
//! [`SignedToken`]: voprf/struct.SignedToken.html
//! [`UnblindedToken`]: voprf/struct.UnblindedToken.html
//! [`VerificationKey`]: voprf/struct.VerificationKey.html
//! [`VerificationSignature`]: voprf/struct.VerificationSignature.html
//! [`DLEQProof`]: voprf/struct.DLEQProof.html
//! [`BatchDLEQProof`]: voprf/struct.BatchDLEQProof.html
#![doc = include_str!("../README.md")]
#![doc = include_str!("../docs/PROTOCOL.md")]

#[cfg(all(feature = "alloc", not(feature = "std")))]
extern crate alloc;

#[cfg(any(test, feature = "std"))]
#[macro_use]
extern crate std;

#[macro_use]
mod macros;

mod oprf;

#[cfg(not(feature = "merlin"))]
mod dleq;
#[cfg(feature = "merlin")]
mod dleq_merlin;

pub mod errors;
pub mod voprf;
