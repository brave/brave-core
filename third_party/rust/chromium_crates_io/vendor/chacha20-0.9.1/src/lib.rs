//! Implementation of the [ChaCha] family of stream ciphers.
//!
//! Cipher functionality is accessed using traits from re-exported [`cipher`] crate.
//!
//! ChaCha stream ciphers are lightweight and amenable to fast, constant-time
//! implementations in software. It improves upon the previous [Salsa] design,
//! providing increased per-round diffusion with no cost to performance.
//!
//! This crate contains the following variants of the ChaCha20 core algorithm:
//!
//! - [`ChaCha20`]: standard IETF variant with 96-bit nonce
//! - [`ChaCha8`] / [`ChaCha12`]: reduced round variants of ChaCha20
//! - [`XChaCha20`]: 192-bit extended nonce variant
//! - [`XChaCha8`] / [`XChaCha12`]: reduced round variants of XChaCha20
//! - [`ChaCha20Legacy`]: "djb" variant with 64-bit nonce.
//! **WARNING:** This implementation internally uses 32-bit counter,
//! while the original implementation uses 64-bit counter. In other words,
//! it does not allow encryption of more than 256 GiB of data.
//!
//! # ⚠️ Security Warning: Hazmat!
//!
//! This crate does not ensure ciphertexts are authentic, which can lead to
//! serious vulnerabilities if used incorrectly!
//!
//! If in doubt, use the [`chacha20poly1305`] crate instead, which provides
//! an authenticated mode on top of ChaCha20.
//!
//! **USE AT YOUR OWN RISK!**
//!
//! # Diagram
//!
//! This diagram illustrates the ChaCha quarter round function.
//! Each round consists of four quarter-rounds:
//!
//! <img src="https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/img/stream-ciphers/chacha20.png" width="300px">
//!
//! Legend:
//!
//! - ⊞ add
//! - ‹‹‹ rotate
//! - ⊕ xor
//!
//! # Example
//! ```
//! use chacha20::ChaCha20;
//! // Import relevant traits
//! use chacha20::cipher::{KeyIvInit, StreamCipher, StreamCipherSeek};
//! use hex_literal::hex;
//!
//! let key = [0x42; 32];
//! let nonce = [0x24; 12];
//! let plaintext = hex!("00010203 04050607 08090A0B 0C0D0E0F");
//! let ciphertext = hex!("e405626e 4f1236b3 670ee428 332ea20e");
//!
//! // Key and IV must be references to the `GenericArray` type.
//! // Here we use the `Into` trait to convert arrays into it.
//! let mut cipher = ChaCha20::new(&key.into(), &nonce.into());
//!
//! let mut buffer = plaintext.clone();
//!
//! // apply keystream (encrypt)
//! cipher.apply_keystream(&mut buffer);
//! assert_eq!(buffer, ciphertext);
//!
//! let ciphertext = buffer.clone();
//!
//! // ChaCha ciphers support seeking
//! cipher.seek(0u32);
//!
//! // decrypt ciphertext by applying keystream again
//! cipher.apply_keystream(&mut buffer);
//! assert_eq!(buffer, plaintext);
//!
//! // stream ciphers can be used with streaming messages
//! cipher.seek(0u32);
//! for chunk in buffer.chunks_mut(3) {
//!     cipher.apply_keystream(chunk);
//! }
//! assert_eq!(buffer, ciphertext);
//! ```
//!
//! # Configuration Flags
//!
//! You can modify crate using the following configuration flags:
//!
//! - `chacha20_force_avx2`: force AVX2 backend on x86/x86_64 targets.
//!   Requires enabled AVX2 target feature. Ignored on non-x86(-64) targets.
//! - `chacha20_force_neon`: force NEON backend on ARM targets.
//!   Requires enabled NEON target feature. Ignored on non-ARM targets. Nightly-only.
//! - `chacha20_force_soft`: force software backend.
//! - `chacha20_force_sse2`: force SSE2 backend on x86/x86_64 targets.
//!   Requires enabled SSE2 target feature. Ignored on non-x86(-64) targets.
//!
//! The flags can be enabled using `RUSTFLAGS` environmental variable
//! (e.g. `RUSTFLAGS="--cfg chacha20_force_avx2"`) or by modifying `.cargo/config`.
//!
//! You SHOULD NOT enable several `force` flags simultaneously.
//!
//! [ChaCha]: https://tools.ietf.org/html/rfc8439
//! [Salsa]: https://en.wikipedia.org/wiki/Salsa20
//! [`chacha20poly1305`]: https://docs.rs/chacha20poly1305

#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/logo.svg",
    html_favicon_url = "https://raw.githubusercontent.com/RustCrypto/media/8f1a9894/logo.svg"
)]
#![allow(clippy::needless_range_loop)]
#![warn(missing_docs, rust_2018_idioms, trivial_casts, unused_qualifications)]

pub use cipher;

use cfg_if::cfg_if;
use cipher::{
    consts::{U10, U12, U32, U4, U6, U64},
    generic_array::{typenum::Unsigned, GenericArray},
    BlockSizeUser, IvSizeUser, KeyIvInit, KeySizeUser, StreamCipherCore, StreamCipherCoreWrapper,
    StreamCipherSeekCore, StreamClosure,
};
use core::marker::PhantomData;

#[cfg(feature = "zeroize")]
use cipher::zeroize::{Zeroize, ZeroizeOnDrop};

mod backends;
mod legacy;
mod xchacha;

pub use legacy::{ChaCha20Legacy, ChaCha20LegacyCore, LegacyNonce};
pub use xchacha::{hchacha, XChaCha12, XChaCha20, XChaCha8, XChaChaCore, XNonce};

/// State initialization constant ("expand 32-byte k")
const CONSTANTS: [u32; 4] = [0x6170_7865, 0x3320_646e, 0x7962_2d32, 0x6b20_6574];

/// Number of 32-bit words in the ChaCha state
const STATE_WORDS: usize = 16;

/// Block type used by all ChaCha variants.
type Block = GenericArray<u8, U64>;

/// Key type used by all ChaCha variants.
pub type Key = GenericArray<u8, U32>;

/// Nonce type used by ChaCha variants.
pub type Nonce = GenericArray<u8, U12>;

/// ChaCha8 stream cipher (reduced-round variant of [`ChaCha20`] with 8 rounds)
pub type ChaCha8 = StreamCipherCoreWrapper<ChaChaCore<U4>>;

/// ChaCha12 stream cipher (reduced-round variant of [`ChaCha20`] with 12 rounds)
pub type ChaCha12 = StreamCipherCoreWrapper<ChaChaCore<U6>>;

/// ChaCha20 stream cipher (RFC 8439 version with 96-bit nonce)
pub type ChaCha20 = StreamCipherCoreWrapper<ChaChaCore<U10>>;

cfg_if! {
    if #[cfg(chacha20_force_soft)] {
        type Tokens = ();
    } else if #[cfg(any(target_arch = "x86", target_arch = "x86_64"))] {
        cfg_if! {
            if #[cfg(chacha20_force_avx2)] {
                #[cfg(not(target_feature = "avx2"))]
                compile_error!("You must enable `avx2` target feature with \
                    `chacha20_force_avx2` configuration option");
                type Tokens = ();
            } else if #[cfg(chacha20_force_sse2)] {
                #[cfg(not(target_feature = "sse2"))]
                compile_error!("You must enable `sse2` target feature with \
                    `chacha20_force_sse2` configuration option");
                type Tokens = ();
            } else {
                cpufeatures::new!(avx2_cpuid, "avx2");
                cpufeatures::new!(sse2_cpuid, "sse2");
                type Tokens = (avx2_cpuid::InitToken, sse2_cpuid::InitToken);
            }
        }
    } else {
        type Tokens = ();
    }
}

/// The ChaCha core function.
pub struct ChaChaCore<R: Unsigned> {
    /// Internal state of the core function
    state: [u32; STATE_WORDS],
    /// CPU target feature tokens
    #[allow(dead_code)]
    tokens: Tokens,
    /// Number of rounds to perform
    rounds: PhantomData<R>,
}

impl<R: Unsigned> KeySizeUser for ChaChaCore<R> {
    type KeySize = U32;
}

impl<R: Unsigned> IvSizeUser for ChaChaCore<R> {
    type IvSize = U12;
}

impl<R: Unsigned> BlockSizeUser for ChaChaCore<R> {
    type BlockSize = U64;
}

impl<R: Unsigned> KeyIvInit for ChaChaCore<R> {
    #[inline]
    fn new(key: &Key, iv: &Nonce) -> Self {
        let mut state = [0u32; STATE_WORDS];
        state[0..4].copy_from_slice(&CONSTANTS);
        let key_chunks = key.chunks_exact(4);
        for (val, chunk) in state[4..12].iter_mut().zip(key_chunks) {
            *val = u32::from_le_bytes(chunk.try_into().unwrap());
        }
        let iv_chunks = iv.chunks_exact(4);
        for (val, chunk) in state[13..16].iter_mut().zip(iv_chunks) {
            *val = u32::from_le_bytes(chunk.try_into().unwrap());
        }

        cfg_if! {
            if #[cfg(chacha20_force_soft)] {
                let tokens = ();
            } else if #[cfg(any(target_arch = "x86", target_arch = "x86_64"))] {
                cfg_if! {
                    if #[cfg(chacha20_force_avx2)] {
                        let tokens = ();
                    } else if #[cfg(chacha20_force_sse2)] {
                        let tokens = ();
                    } else {
                        let tokens = (avx2_cpuid::init(), sse2_cpuid::init());
                    }
                }
            } else {
                let tokens = ();
            }
        }

        Self {
            state,
            tokens,
            rounds: PhantomData,
        }
    }
}

impl<R: Unsigned> StreamCipherCore for ChaChaCore<R> {
    #[inline(always)]
    fn remaining_blocks(&self) -> Option<usize> {
        let rem = u32::MAX - self.get_block_pos();
        rem.try_into().ok()
    }

    fn process_with_backend(&mut self, f: impl StreamClosure<BlockSize = Self::BlockSize>) {
        cfg_if! {
            if #[cfg(chacha20_force_soft)] {
                f.call(&mut backends::soft::Backend(self));
            } else if #[cfg(any(target_arch = "x86", target_arch = "x86_64"))] {
                cfg_if! {
                    if #[cfg(chacha20_force_avx2)] {
                        unsafe {
                            backends::avx2::inner::<R, _>(&mut self.state, f);
                        }
                    } else if #[cfg(chacha20_force_sse2)] {
                        unsafe {
                            backends::sse2::inner::<R, _>(&mut self.state, f);
                        }
                    } else {
                        let (avx2_token, sse2_token) = self.tokens;
                        if avx2_token.get() {
                            unsafe {
                                backends::avx2::inner::<R, _>(&mut self.state, f);
                            }
                        } else if sse2_token.get() {
                            unsafe {
                                backends::sse2::inner::<R, _>(&mut self.state, f);
                            }
                        } else {
                            f.call(&mut backends::soft::Backend(self));
                        }
                    }
                }
            } else if #[cfg(all(chacha20_force_neon, target_arch = "aarch64", target_feature = "neon"))] {
                unsafe {
                    backends::neon::inner::<R, _>(&mut self.state, f);
                }
            } else {
                f.call(&mut backends::soft::Backend(self));
            }
        }
    }
}

impl<R: Unsigned> StreamCipherSeekCore for ChaChaCore<R> {
    type Counter = u32;

    #[inline(always)]
    fn get_block_pos(&self) -> u32 {
        self.state[12]
    }

    #[inline(always)]
    fn set_block_pos(&mut self, pos: u32) {
        self.state[12] = pos;
    }
}

#[cfg(feature = "zeroize")]
#[cfg_attr(docsrs, doc(cfg(feature = "zeroize")))]
impl<R: Unsigned> Drop for ChaChaCore<R> {
    fn drop(&mut self) {
        self.state.zeroize();
    }
}

#[cfg(feature = "zeroize")]
#[cfg_attr(docsrs, doc(cfg(feature = "zeroize")))]
impl<R: Unsigned> ZeroizeOnDrop for ChaChaCore<R> {}
