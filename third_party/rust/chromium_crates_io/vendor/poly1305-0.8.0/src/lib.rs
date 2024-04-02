//! The Poly1305 universal hash function and message authentication code.
//!
//! # About
//!
//! Poly1305 is a universal hash function suitable for use as a one-time
//! authenticator and, when combined with a cipher, a message authentication
//! code (MAC).
//!
//! It takes a 32-byte one-time key and a message and produces a 16-byte tag,
//! which can be used to authenticate the message.
//!
//! Poly1305 is primarily notable for its use in the [`ChaCha20Poly1305`] and
//! [`XSalsa20Poly1305`] authenticated encryption algorithms.
//!
//! # Minimum Supported Rust Version
//!
//! Rust **1.56** or higher.
//!
//! Minimum supported Rust version may be changed in the future, but such
//! changes will be accompanied with a minor version bump.
//!
//! # Security Notes
//!
//! This crate has received one [security audit by NCC Group][audit], with no
//! significant findings. We would like to thank [MobileCoin] for funding the
//! audit.
//!
//! NOTE: the audit predates the AVX2 backend, which has not yet been audited.
//!
//! All implementations contained in the crate are designed to execute in constant
//! time, either by relying on hardware intrinsics (e.g. AVX2 on x86/x86_64), or
//! using a portable implementation which is only constant time on processors which
//! implement constant-time multiplication.
//!
//! It is not suitable for use on processors with a variable-time multiplication
//! operation (e.g. short circuit on multiply-by-zero / multiply-by-one, such as
//! certain 32-bit PowerPC CPUs and some non-ARM microcontrollers).
//!
//! [`ChaCha20Poly1305`]: https://docs.rs/chacha20poly1305
//! [`XSalsa20Poly1305`]: https://docs.rs/xsalsa20poly1305
//! [audit]: https://research.nccgroup.com/2020/02/26/public-report-rustcrypto-aes-gcm-and-chacha20poly1305-implementation-review/
//! [MobileCoin]: https://mobilecoin.com

#![no_std]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/logo.svg",
    html_favicon_url = "https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/logo.svg"
)]
#![warn(missing_docs, rust_2018_idioms)]

#[cfg(feature = "std")]
extern crate std;

pub use universal_hash;

use universal_hash::{
    consts::{U16, U32},
    crypto_common::{BlockSizeUser, KeySizeUser},
    generic_array::GenericArray,
    KeyInit, UniversalHash,
};

mod backend;

#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    not(poly1305_force_soft),
    target_feature = "avx2", // Fuzz tests bypass AVX2 autodetection code
    any(fuzzing, test)
))]
mod fuzz;

#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    not(poly1305_force_soft)
))]
use crate::backend::autodetect::State;

#[cfg(not(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    not(poly1305_force_soft)
)))]
use crate::backend::soft::State;

/// Size of a Poly1305 key
pub const KEY_SIZE: usize = 32;

/// Size of the blocks Poly1305 acts upon
pub const BLOCK_SIZE: usize = 16;

/// Poly1305 keys (32-bytes)
pub type Key = universal_hash::Key<Poly1305>;

/// Poly1305 blocks (16-bytes)
pub type Block = universal_hash::Block<Poly1305>;

/// Poly1305 tags (16-bytes)
pub type Tag = universal_hash::Block<Poly1305>;

/// The Poly1305 universal hash function.
///
/// Note that Poly1305 is not a traditional MAC and is single-use only
/// (a.k.a. "one-time authenticator").
///
/// For this reason it doesn't impl the `crypto_mac::Mac` trait.
#[derive(Clone)]
pub struct Poly1305 {
    state: State,
}

impl KeySizeUser for Poly1305 {
    type KeySize = U32;
}

impl KeyInit for Poly1305 {
    /// Initialize Poly1305 with the given key
    fn new(key: &Key) -> Poly1305 {
        Poly1305 {
            state: State::new(key),
        }
    }
}

impl BlockSizeUser for Poly1305 {
    type BlockSize = U16;
}

impl UniversalHash for Poly1305 {
    fn update_with_backend(
        &mut self,
        f: impl universal_hash::UhfClosure<BlockSize = Self::BlockSize>,
    ) {
        self.state.update_with_backend(f);
    }

    /// Get the hashed output
    fn finalize(self) -> Tag {
        self.state.finalize()
    }
}

impl Poly1305 {
    /// Compute unpadded Poly1305 for the given input data.
    ///
    /// The main use case for this is XSalsa20Poly1305.
    pub fn compute_unpadded(mut self, data: &[u8]) -> Tag {
        for chunk in data.chunks(BLOCK_SIZE) {
            if chunk.len() == BLOCK_SIZE {
                let block = GenericArray::from_slice(chunk);
                self.state.compute_block(block, false);
            } else {
                let mut block = Block::default();
                block[..chunk.len()].copy_from_slice(chunk);
                block[chunk.len()] = 1;
                self.state.compute_block(&block, true)
            }
        }

        self.state.finalize()
    }
}

opaque_debug::implement!(Poly1305);

#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    not(poly1305_force_soft),
    target_feature = "avx2", // Fuzz tests bypass AVX2 autodetection code
    any(fuzzing, test)
))]
pub use crate::fuzz::fuzz_avx2;
