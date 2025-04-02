//! Seed Fingerprints according to ZIP 32
//!
//! Implements section [Seed Fingerprints] of Shielded Hierarchical Deterministic Wallets (ZIP 32).
//!
//! [Seed Fingerprints]: https://zips.z.cash/zip-0032#seed-fingerprints
use blake2b_simd::Params as Blake2bParams;

const ZIP32_SEED_FP_PERSONALIZATION: &[u8; 16] = b"Zcash_HD_Seed_FP";

/// The fingerprint for a wallet's seed bytes, as defined in [ZIP 32].
///
/// [ZIP 32]: https://zips.z.cash/zip-0032#seed-fingerprints
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct SeedFingerprint([u8; 32]);

impl ::core::fmt::Debug for SeedFingerprint {
    fn fmt(&self, f: &mut ::core::fmt::Formatter) -> ::core::fmt::Result {
        write!(f, "SeedFingerprint(")?;
        for i in self.0 {
            write!(f, "{:02x}", i)?;
        }
        write!(f, ")")?;

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

#[test]
fn test_seed_fingerprint() {
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
        fingerprint_str: "deff604c246710f7176dead02aa746f2fd8d5389f7072556dcb555fdbe5e3ae3",
    }];

    for tv in test_vectors {
        let fp = SeedFingerprint::from_seed(&tv.root_seed).expect("root_seed has valid length");
        assert_eq!(&fp.to_bytes(), &tv.fingerprint[..]);
        assert_eq!(
            std::format!("{:?}", fp),
            std::format!("SeedFingerprint({})", tv.fingerprint_str)
        );
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
