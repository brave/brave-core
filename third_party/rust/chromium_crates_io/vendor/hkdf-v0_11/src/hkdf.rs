//! An implementation of HKDF, the [HMAC-based Extract-and-Expand Key Derivation Function][1].
//!
//! # Usage
//!
//! ```rust
//! # use sha2::Sha256;
//! # use hkdf::Hkdf;
//! let ikm = hex::decode("0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b").unwrap();
//! let salt = hex::decode("000102030405060708090a0b0c").unwrap();
//! let info = hex::decode("f0f1f2f3f4f5f6f7f8f9").unwrap();
//!
//! let h = Hkdf::<Sha256>::new(Some(&salt[..]), &ikm);
//! let mut okm = [0u8; 42];
//! h.expand(&info, &mut okm).unwrap();
//! println!("OKM is {}", hex::encode(&okm[..]));
//! ```
//!
//! [1]: https://tools.ietf.org/html/rfc5869

#![no_std]
#![warn(rust_2018_idioms)]

#[cfg(feature = "std")]
extern crate std;

use core::fmt;
use digest::generic_array::{self, ArrayLength, GenericArray};
use digest::{BlockInput, FixedOutput, Reset, Update};
use hmac::{Hmac, Mac, NewMac};

/// Error that is returned when supplied pseudorandom key (PRK) is not long enough.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub struct InvalidPrkLength;

/// Structure for InvalidLength, used for output error handling.
#[derive(Copy, Clone, Eq, PartialEq, Debug)]
pub struct InvalidLength;

/// Structure representing the streaming context of an HKDF-Extract operation
/// ```rust
/// # use hkdf::{Hkdf, HkdfExtract};
/// # use sha2::Sha256;
/// let mut extract_ctx = HkdfExtract::<Sha256>::new(Some(b"mysalt"));
/// extract_ctx.input_ikm(b"hello");
/// extract_ctx.input_ikm(b" world");
/// let (streamed_res, _) = extract_ctx.finalize();
///
/// let (oneshot_res, _) = Hkdf::<Sha256>::extract(Some(b"mysalt"), b"hello world");
/// assert_eq!(streamed_res, oneshot_res);
/// ```
#[derive(Clone)]
pub struct HkdfExtract<D>
where
    D: Update + BlockInput + FixedOutput + Reset + Default + Clone,
    D::BlockSize: ArrayLength<u8>,
    D::OutputSize: ArrayLength<u8>,
{
    hmac: Hmac<D>,
}

impl<D> HkdfExtract<D>
where
    D: Update + BlockInput + FixedOutput + Reset + Default + Clone,
    D::BlockSize: ArrayLength<u8>,
    D::OutputSize: ArrayLength<u8>,
{
    /// Initiates the HKDF-Extract context with the given optional salt
    pub fn new(salt: Option<&[u8]>) -> HkdfExtract<D> {
        let default_salt = GenericArray::<u8, D::OutputSize>::default();
        let salt = salt.unwrap_or(&default_salt);
        let hmac = Hmac::<D>::new_from_slice(salt).expect("HMAC can take a key of any size");
        HkdfExtract { hmac }
    }

    /// Feeds in additional input key material to the HKDF-Extract context
    pub fn input_ikm(&mut self, ikm: &[u8]) {
        self.hmac.update(ikm);
    }

    /// Completes the HKDF-Extract operation, returning both the generated pseudorandom key and
    /// `Hkdf` struct for expanding.
    pub fn finalize(self) -> (GenericArray<u8, D::OutputSize>, Hkdf<D>) {
        let prk = self.hmac.finalize().into_bytes();
        let hkdf = Hkdf::from_prk(&prk).expect("PRK size is correct");
        (prk, hkdf)
    }
}

/// Structure representing the HKDF, capable of HKDF-Expand and HKDF-Extract operations.
#[derive(Clone)]
pub struct Hkdf<D>
where
    D: Update + BlockInput + FixedOutput + Reset + Default + Clone,
    D::BlockSize: ArrayLength<u8>,
    D::OutputSize: ArrayLength<u8>,
{
    hmac: Hmac<D>,
}

impl<D> Hkdf<D>
where
    D: Update + BlockInput + FixedOutput + Reset + Default + Clone,
    D::BlockSize: ArrayLength<u8>,
    D::OutputSize: ArrayLength<u8>,
{
    /// Convenience method for [`extract`][Hkdf::extract] when the generated
    /// pseudorandom key can be ignored and only HKDF-Expand operation is needed. This is the most
    /// common constructor.
    pub fn new(salt: Option<&[u8]>, ikm: &[u8]) -> Hkdf<D> {
        let (_, hkdf) = Hkdf::extract(salt, ikm);
        hkdf
    }

    /// Create `Hkdf` from an already cryptographically strong pseudorandom key
    /// as per section 3.3 from RFC5869.
    pub fn from_prk(prk: &[u8]) -> Result<Hkdf<D>, InvalidPrkLength> {
        use crate::generic_array::typenum::Unsigned;

        // section 2.3 specifies that prk must be "at least HashLen octets"
        if prk.len() < D::OutputSize::to_usize() {
            return Err(InvalidPrkLength);
        }

        Ok(Hkdf {
            hmac: Hmac::new_from_slice(prk).expect("HMAC can take a key of any size"),
        })
    }

    /// The RFC5869 HKDF-Extract operation returning both the generated
    /// pseudorandom key and `Hkdf` struct for expanding.
    pub fn extract(salt: Option<&[u8]>, ikm: &[u8]) -> (GenericArray<u8, D::OutputSize>, Hkdf<D>) {
        let mut extract_ctx = HkdfExtract::new(salt);
        extract_ctx.input_ikm(ikm);
        extract_ctx.finalize()
    }

    /// The RFC5869 HKDF-Expand operation. This is equivalent to calling
    /// [`expand`][Hkdf::extract] with the `info` argument set equal to the
    /// concatenation of all the elements of `info_components`.
    pub fn expand_multi_info(
        &self,
        info_components: &[&[u8]],
        okm: &mut [u8],
    ) -> Result<(), InvalidLength> {
        use crate::generic_array::typenum::Unsigned;

        let mut prev: Option<GenericArray<u8, <D as digest::FixedOutput>::OutputSize>> = None;

        let hmac_output_bytes = D::OutputSize::to_usize();
        if okm.len() > hmac_output_bytes * 255 {
            return Err(InvalidLength);
        }

        let mut hmac = self.hmac.clone();
        for (blocknum, okm_block) in okm.chunks_mut(hmac_output_bytes).enumerate() {
            let block_len = okm_block.len();

            if let Some(ref prev) = prev {
                hmac.update(prev)
            };

            // Feed in the info components in sequence. This is equivalent to feeding in the
            // concatenation of all the info components
            for info in info_components {
                hmac.update(info);
            }

            hmac.update(&[blocknum as u8 + 1]);

            let output = hmac.finalize_reset().into_bytes();
            okm_block.copy_from_slice(&output[..block_len]);

            prev = Some(output);
        }

        Ok(())
    }

    /// The RFC5869 HKDF-Expand operation
    ///
    /// If you don't have any `info` to pass, use an empty slice.
    pub fn expand(&self, info: &[u8], okm: &mut [u8]) -> Result<(), InvalidLength> {
        self.expand_multi_info(&[info], okm)
    }
}

impl fmt::Display for InvalidPrkLength {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        f.write_str("invalid pseudorandom key length, too short")
    }
}

#[cfg(feature = "std")]
impl ::std::error::Error for InvalidPrkLength {}

impl fmt::Display for InvalidLength {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        f.write_str("invalid number of blocks, too large output")
    }
}

#[cfg(feature = "std")]
impl ::std::error::Error for InvalidLength {}
