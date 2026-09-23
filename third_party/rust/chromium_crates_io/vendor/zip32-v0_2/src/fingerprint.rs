//! Seed Fingerprints according to ZIP 32
//!
//! Implements section [Seed Fingerprints] of Shielded Hierarchical Deterministic Wallets (ZIP 32).
//!
//! [Seed Fingerprints]: https://zips.z.cash/zip-0032#seed-fingerprints

use core::{fmt, str::FromStr};

use bech32::{
    primitives::decode::{CheckedHrpstring, CheckedHrpstringError},
    Bech32m,
};
use blake2b_simd::Params as Blake2bParams;

const ZIP32_SEED_FP_PERSONALIZATION: &[u8; 16] = b"Zcash_HD_Seed_FP";

const HRP: bech32::Hrp = bech32::Hrp::parse_unchecked("zip32seedfp");

/// The fingerprint for a wallet's seed bytes, as defined in [ZIP 32].
///
/// [ZIP 32]: https://zips.z.cash/zip-0032#seed-fingerprints
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct SeedFingerprint([u8; 32]);

impl ::core::fmt::Debug for SeedFingerprint {
    fn fmt(&self, f: &mut ::core::fmt::Formatter) -> ::core::fmt::Result {
        write!(f, "SeedFingerprint([")?;
        // The hex byte encoding in `zcashd` was byte-reversed. To avoid devs interpreting
        // this encoding as a single string, render it as a Rust array of bytes.
        for (i, b) in self.0.iter().enumerate() {
            if i != 0 {
                write!(f, ", ")?;
            }
            write!(f, "0x{:02x}", b)?;
        }
        write!(f, "])")?;

        Ok(())
    }
}

impl SeedFingerprint {
    /// Derives the fingerprint of the given seed bytes.
    ///
    /// Returns `None` if the length of `seed_bytes` is less than 32 or greater than 252.
    pub fn from_seed(seed_bytes: &[u8]) -> Option<SeedFingerprint> {
        let seed_len = seed_bytes.len();

        if (32..=252).contains(&seed_len) {
            let seed_len: u8 = seed_len.try_into().unwrap();
            Some(SeedFingerprint(
                Blake2bParams::new()
                    .hash_length(32)
                    .personal(ZIP32_SEED_FP_PERSONALIZATION)
                    .to_state()
                    .update(&[seed_len])
                    .update(seed_bytes)
                    .finalize()
                    .as_bytes()
                    .try_into()
                    .expect("hash length should be 32 bytes"),
            ))
        } else {
            None
        }
    }

    /// Reconstructs the fingerprint from a buffer containing a previously computed fingerprint.
    pub fn from_bytes(hash: [u8; 32]) -> Self {
        Self(hash)
    }

    /// Returns the fingerprint as a byte array.
    pub fn to_bytes(&self) -> [u8; 32] {
        self.0
    }
}

impl FromStr for SeedFingerprint {
    type Err = ParseError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let checked = CheckedHrpstring::new::<Bech32m>(s).map_err(ParseError::NotABech32mString)?;
        if checked.hrp() == HRP {
            let data = checked.byte_iter();
            if data.len() == 32 {
                let mut seedfp = [0; 32];
                for (source, dest) in data.zip(seedfp.iter_mut()) {
                    *dest = source;
                }
                Ok(Self(seedfp))
            } else {
                Err(ParseError::InvalidLength)
            }
        } else {
            Err(ParseError::NotASeedFingerprint)
        }
    }
}

impl fmt::Display for SeedFingerprint {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        bech32::encode_to_fmt::<Bech32m, _>(f, HRP, &self.0).map_err(|e| match e {
            bech32::EncodeError::Fmt(error) => error,
            // We know a seed fingerprint is not too long.
            bech32::EncodeError::TooLong(_) => unreachable!(),
            // Error type is non-exhaustive, but there aren't any other kinds of errors.
            _ => panic!(),
        })
    }
}

/// Errors that can occur when parsing a ZIP 32 seed fingerprint string.
#[derive(Debug)]
pub enum ParseError {
    /// The string claims to be a ZIP 32 seed fingerprint, but the length is not 32 bytes.
    InvalidLength,
    /// The string is not a valid Bech32m string, and thus not a ZIP 32 seed fingerprint.
    NotABech32mString(CheckedHrpstringError),
    /// The string is a valid Bech32m string, but with some other HRP.
    NotASeedFingerprint,
}

#[test]
fn test_seed_fingerprint() {
    use alloc::string::ToString;

    struct TestVector {
        root_seed: [u8; 32],
        fingerprint: [u8; 32],
        fingerprint_str: &'static str,
    }

    let test_vectors = [TestVector {
        root_seed: [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
            0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
            0x1c, 0x1d, 0x1e, 0x1f,
        ],
        fingerprint: [
            0xde, 0xff, 0x60, 0x4c, 0x24, 0x67, 0x10, 0xf7, 0x17, 0x6d, 0xea, 0xd0, 0x2a, 0xa7,
            0x46, 0xf2, 0xfd, 0x8d, 0x53, 0x89, 0xf7, 0x7, 0x25, 0x56, 0xdc, 0xb5, 0x55, 0xfd,
            0xbe, 0x5e, 0x3a, 0xe3,
        ],
        fingerprint_str: "zip32seedfp1mmlkqnpyvug0w9mdatgz4f6x7t7c65uf7urj24kuk42lm0j78t3sne2h0z",
    }];

    for tv in test_vectors {
        let fp = SeedFingerprint::from_seed(&tv.root_seed).expect("root_seed has valid length");
        assert_eq!(&fp.to_bytes(), &tv.fingerprint[..]);
        assert_eq!(fp.to_string(), tv.fingerprint_str);
    }
}
#[test]
fn test_seed_fingerprint_is_none() {
    let odd_seed = [
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f,
    ];

    assert!(
        SeedFingerprint::from_seed(&odd_seed).is_none(),
        "fingerprint from short seed should be `None`"
    );
}
