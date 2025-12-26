#![no_std]
#![doc = include_str!("../README.md")]
#![doc(
    html_logo_url = "https://raw.githubusercontent.com/RustCrypto/media/6ee8e381/logo.svg",
    html_favicon_url = "https://raw.githubusercontent.com/RustCrypto/media/6ee8e381/logo.svg"
)]
#![cfg_attr(docsrs, feature(doc_auto_cfg))]
#![warn(missing_docs, rust_2018_idioms)]

//! Collision checked Sha1.
//!
//! General techniques and implementation are based on the research and implementation done in [1], [2] by
//! Marc Stevens.
//!
//!
//! Original license can be found in [3].
//!
//! [1]: https://github.com/cr-marcstevens/sha1collisiondetection
//! [2]: https://marc-stevens.nl/research/papers/C13-S.pdf
//! [3]: https://github.com/cr-marcstevens/sha1collisiondetection/blob/master/LICENSE.txt

pub use digest::{self, Digest};

use core::slice::from_ref;

#[cfg(feature = "std")]
extern crate std;

use digest::{
    block_buffer::{BlockBuffer, Eager},
    core_api::BlockSizeUser,
    typenum::{Unsigned, U20, U64},
    FixedOutput, FixedOutputReset, HashMarker, Output, OutputSizeUser, Reset, Update,
};
#[cfg(feature = "zeroize")]
use zeroize::{Zeroize, ZeroizeOnDrop};

const BLOCK_SIZE: usize = <sha1::Sha1Core as BlockSizeUser>::BlockSize::USIZE;
const STATE_LEN: usize = 5;
const INITIAL_H: [u32; 5] = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0];

mod compress;
mod ubc_check;

/// SHA-1 collision detection hasher state.
#[derive(Clone)]
pub struct Sha1 {
    h: [u32; STATE_LEN],
    block_len: u64,
    detection: Option<DetectionState>,
    buffer: BlockBuffer<U64, Eager>,
}

impl HashMarker for Sha1 {}

impl Default for Sha1 {
    fn default() -> Self {
        Builder::default().build()
    }
}

impl Sha1 {
    /// Create a new Sha1 instance, with collision detection enabled.
    pub fn new() -> Self {
        Self::default()
    }

    /// Create a new Sha1 builder to configure detection.
    pub fn builder() -> Builder {
        Builder::default()
    }

    /// Oneshot hashing, reporting the collision state.
    ///
    /// # Examples
    ///
    /// ```
    /// use hex_literal::hex;
    /// use sha1_checked::Sha1;
    ///
    /// let result = Sha1::try_digest(b"hello world");
    /// assert_eq!(result.hash().as_ref(), hex!("2aae6c35c94fcfb415dbe95f408b9ce91ee846ed"));
    /// assert!(!result.has_collision());
    /// ```
    pub fn try_digest(data: impl AsRef<[u8]>) -> CollisionResult {
        let mut hasher = Self::default();
        Digest::update(&mut hasher, data);
        hasher.try_finalize()
    }

    /// Try finalization, reporting the collision state.
    pub fn try_finalize(mut self) -> CollisionResult {
        let mut out = Output::<Self>::default();
        self.finalize_inner(&mut out);

        if let Some(ref ctx) = self.detection {
            if ctx.found_collision {
                if ctx.safe_hash {
                    return CollisionResult::Mitigated(out);
                }
                return CollisionResult::Collision(out);
            }
        }
        CollisionResult::Ok(out)
    }

    fn finalize_inner(&mut self, out: &mut Output<Self>) {
        let bs = 64;
        let buffer = &mut self.buffer;
        let h = &mut self.h;

        if let Some(ref mut ctx) = self.detection {
            let last_block = buffer.get_data();
            compress::finalize(h, bs * self.block_len, last_block, ctx);
        } else {
            let bit_len = 8 * (buffer.get_pos() as u64 + bs * self.block_len);
            buffer.len64_padding_be(bit_len, |b| sha1::compress(h, from_ref(b)));
        }

        for (chunk, v) in out.chunks_exact_mut(4).zip(h.iter()) {
            chunk.copy_from_slice(&v.to_be_bytes());
        }
    }
}

/// Result when trying to finalize a hash.
#[derive(Debug)]
pub enum CollisionResult {
    /// No collision.
    Ok(Output<Sha1>),
    /// Collision occured, but was mititgated.
    Mitigated(Output<Sha1>),
    /// Collision occured, the hash is the one that collided.
    Collision(Output<Sha1>),
}

impl CollisionResult {
    /// Returns the output hash.
    pub fn hash(&self) -> &Output<Sha1> {
        match self {
            CollisionResult::Ok(ref s) => s,
            CollisionResult::Mitigated(ref s) => s,
            CollisionResult::Collision(ref s) => s,
        }
    }

    /// Returns if there was a collision
    pub fn has_collision(&self) -> bool {
        !matches!(self, CollisionResult::Ok(_))
    }
}

impl core::fmt::Debug for Sha1 {
    #[inline]
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> Result<(), core::fmt::Error> {
        f.write_str("Sha1CollisionDetection { .. }")
    }
}

impl Reset for Sha1 {
    #[inline]
    fn reset(&mut self) {
        self.h = INITIAL_H;
        self.block_len = 0;
        self.buffer.reset();
        if let Some(ref mut ctx) = self.detection {
            ctx.reset();
        }
    }
}

impl Update for Sha1 {
    #[inline]
    fn update(&mut self, input: &[u8]) {
        let Self {
            h,
            detection,
            buffer,
            ..
        } = self;
        buffer.digest_blocks(input, |blocks| {
            self.block_len += blocks.len() as u64;
            if let Some(ref mut ctx) = detection {
                // SAFETY: GenericArray<u8, U64> and [u8; 64] have
                // exactly the same memory layout
                let blocks: &[[u8; BLOCK_SIZE]] =
                    unsafe { &*(blocks as *const _ as *const [[u8; BLOCK_SIZE]]) };
                compress::compress(h, ctx, blocks);
            } else {
                sha1::compress(h, blocks);
            }
        });
    }
}

impl OutputSizeUser for Sha1 {
    type OutputSize = U20;
}

impl FixedOutput for Sha1 {
    #[inline]
    fn finalize_into(mut self, out: &mut Output<Self>) {
        self.finalize_inner(out);
    }
}

impl FixedOutputReset for Sha1 {
    #[inline]
    fn finalize_into_reset(&mut self, out: &mut Output<Self>) {
        self.finalize_inner(out);
        Reset::reset(self);
    }
}

#[cfg(feature = "zeroize")]
impl ZeroizeOnDrop for Sha1 {}

impl Drop for DetectionState {
    #[inline]
    fn drop(&mut self) {
        #[cfg(feature = "zeroize")]
        {
            self.ihv1.zeroize();
            self.ihv2.zeroize();
            self.m1.zeroize();
            self.m2.zeroize();
            self.state_58.zeroize();
            self.state_65.zeroize();
        }
    }
}

#[cfg(feature = "zeroize")]
impl ZeroizeOnDrop for DetectionState {}

#[cfg(feature = "oid")]
#[cfg_attr(docsrs, doc(cfg(feature = "oid")))]
impl digest::const_oid::AssociatedOid for Sha1 {
    const OID: digest::const_oid::ObjectIdentifier = sha1::Sha1Core::OID;
}

#[cfg(feature = "std")]
impl std::io::Write for Sha1 {
    #[inline]
    fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
        Update::update(self, buf);
        Ok(buf.len())
    }

    #[inline]
    fn flush(&mut self) -> std::io::Result<()> {
        Ok(())
    }
}

/// Builder for collision detection configuration.
#[derive(Clone)]
pub struct Builder {
    detect_collision: bool,
    safe_hash: bool,
    ubc_check: bool,
    reduced_round_collision: bool,
}

impl Default for Builder {
    fn default() -> Self {
        Self {
            detect_collision: true,
            safe_hash: true,
            ubc_check: true,
            reduced_round_collision: false,
        }
    }
}

impl Builder {
    /// Should we detect collisions at all? Default: true
    pub fn detect_collision(mut self, detect: bool) -> Self {
        self.detect_collision = detect;
        self
    }

    /// Should a fix be automatically be applied, or the original hash be returned? Default: true
    pub fn safe_hash(mut self, safe_hash: bool) -> Self {
        self.safe_hash = safe_hash;
        self
    }

    /// Should unavoidable bitconditions be used to speed up the check? Default: true
    pub fn use_ubc(mut self, ubc: bool) -> Self {
        self.ubc_check = ubc;
        self
    }

    /// Should reduced round collisions be used? Default: false
    pub fn reduced_round_collision(mut self, reduced: bool) -> Self {
        self.reduced_round_collision = reduced;
        self
    }

    fn into_detection_state(self) -> Option<DetectionState> {
        if self.detect_collision {
            Some(DetectionState {
                safe_hash: self.safe_hash,
                reduced_round_collision: self.reduced_round_collision,
                ubc_check: self.ubc_check,
                found_collision: false,
                ihv1: Default::default(),
                ihv2: Default::default(),
                m1: [0; 80],
                m2: [0; 80],
                state_58: Default::default(),
                state_65: Default::default(),
            })
        } else {
            None
        }
    }

    /// Create a Sha1 with a specific collision detection configuration.
    pub fn build(self) -> Sha1 {
        let detection = self.into_detection_state();
        Sha1 {
            h: INITIAL_H,
            block_len: 0,
            detection,
            buffer: Default::default(),
        }
    }
}

/// The internal state used to do collision detection.
#[derive(Clone, Debug)]
struct DetectionState {
    safe_hash: bool,
    ubc_check: bool,
    reduced_round_collision: bool,
    /// Has a collision been detected?
    found_collision: bool,
    ihv1: [u32; 5],
    ihv2: [u32; 5],
    m1: [u32; 80],
    m2: [u32; 80],
    /// Stores past states, for faster recompression.
    state_58: [u32; 5],
    state_65: [u32; 5],
}

impl Default for DetectionState {
    fn default() -> Self {
        Builder::default()
            .into_detection_state()
            .expect("enabled by default")
    }
}

impl DetectionState {
    fn reset(&mut self) {
        // Do not reset the config, it needs to be preserved

        self.found_collision = false;
        self.ihv1 = Default::default();
        self.ihv2 = Default::default();
        self.m1 = [0; 80];
        self.m2 = [0; 80];
        self.state_58 = Default::default();
        self.state_65 = Default::default();
    }
}
