//! Arbitrary key derivation.
//!
//! In some contexts there is a need for deriving arbitrary keys with the same derivation
//! path as existing key material (for example, deriving an arbitrary account-level key),
//! without the need for ecosystem-wide coordination. The following instantiation of the
//! [hardened key generation framework] may be used for this purpose.
//!
//! Defined in [ZIP32: Arbitrary key derivation][arbkd].
//!
//! [hardened key generation framework]: crate::hardened_only
//! [arbkd]: https://zips.z.cash/zip-0032#specification-arbitrary-key-derivation

use zcash_spec::PrfExpand;

use crate::{
    hardened_only::{Context, HardenedOnlyKey},
    ChainCode, ChildIndex,
};

struct Arbitrary;

impl Context for Arbitrary {
    const MKG_DOMAIN: [u8; 16] = *b"ZcashArbitraryKD";
    const CKD_DOMAIN: PrfExpand<([u8; 32], [u8; 4])> = PrfExpand::ARBITRARY_ZIP32_CHILD;
}

/// An arbitrary extended secret key.
///
/// Defined in [ZIP32: Arbitrary key derivation][arbkd].
///
/// [arbkd]: https://zips.z.cash/zip-0032#specification-arbitrary-key-derivation
pub struct SecretKey {
    inner: HardenedOnlyKey<Arbitrary>,
}

fn with_ikm<F, T>(context_string: &[u8], seed: &[u8], f: F) -> T
where
    F: FnOnce(&[&[u8]]) -> T,
{
    let context_len =
        u8::try_from(context_string.len()).expect("context string should be at most 252 bytes");
    assert!((1..=252).contains(&context_len));

    let seed_len = u8::try_from(seed.len()).expect("seed should be at most 252 bytes");
    assert!((32..=252).contains(&seed_len));

    let ikm = &[&[context_len], context_string, &[seed_len], seed];

    f(ikm)
}

impl SecretKey {
    /// Derives an arbitrary key at the given path from the given seed.
    ///
    /// `context_string` is an identifier for the context in which this key will be used.
    /// It must be globally unique.
    ///
    /// # Panics
    ///
    /// Panics if:
    /// - the context string is empty or longer than 252 bytes.
    /// - the seed is shorter than 32 bytes or longer than 252 bytes.
    pub fn from_path(context_string: &[u8], seed: &[u8], path: &[ChildIndex]) -> Self {
        let mut xsk = Self::master(context_string, seed);
        for i in path {
            xsk = xsk.derive_child(*i);
        }
        xsk
    }

    /// Generates the master key of an Arbitrary extended secret key.
    ///
    /// Defined in [ZIP32: Arbitrary master key generation][mkgarb].
    ///
    /// [mkgarb]: https://zips.z.cash/zip-0032#arbitrary-master-key-generation
    ///
    /// # Panics
    ///
    /// Panics if:
    /// - the context string is empty or longer than 252 bytes.
    /// - the seed is shorter than 32 bytes or longer than 252 bytes.
    fn master(context_string: &[u8], seed: &[u8]) -> Self {
        with_ikm(context_string, seed, |ikm| Self {
            inner: HardenedOnlyKey::master(ikm),
        })
    }

    /// Derives a child key from a parent key at a given index.
    ///
    /// Defined in [ZIP32: Arbitrary-only child key derivation][ckdarb].
    ///
    /// [ckdarb]: https://zips.z.cash/zip-0032#arbitrary-child-key-derivation
    fn derive_child(&self, index: ChildIndex) -> Self {
        Self {
            inner: self.inner.derive_child(index),
        }
    }

    /// Returns the key material for this arbitrary key.
    pub fn data(&self) -> &[u8; 32] {
        self.inner.parts().0
    }

    /// Returns the chain code for this arbitrary key.
    pub fn chain_code(&self) -> &ChainCode {
        self.inner.parts().1
    }

    /// Concatenates the key data and chain code to obtain a full-width key.
    ///
    /// This may be used when a context requires a 64-byte key instead of a 32-byte key
    /// (for example, to avoid an entropy bottleneck in its particular subsequent
    /// operations).
    ///
    /// Child keys MUST NOT be derived from any key on which this method is called. For
    /// the current API, this means that [`SecretKey::from_path`] MUST NOT be called with
    /// a `path` for which this key's path is a prefix.
    pub fn into_full_width_key(self) -> [u8; 64] {
        let (sk, c) = self.inner.into_parts();
        // Re-concatenate the key parts.
        let mut key = [0; 64];
        key[..32].copy_from_slice(&sk);
        key[32..].copy_from_slice(&c.0);
        key
    }
}

#[cfg(test)]
mod tests {
    use crate::{arbitrary::with_ikm, ChildIndex};

    use super::SecretKey;

    struct TestVector {
        context_string: &'static [u8],
        seed: [u8; 32],
        ikm: Option<&'static [u8]>,
        path: &'static [u32],
        sk: [u8; 32],
        c: [u8; 32],
    }

    // From https://github.com/zcash-hackworks/zcash-test-vectors/blob/master/zip_0032_arbitrary.py
    const TEST_VECTORS: &'static [TestVector] = &[
        TestVector {
            context_string: &[
                0x5a, 0x63, 0x61, 0x73, 0x68, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x76, 0x65, 0x63,
                0x74, 0x6f, 0x72, 0x73,
            ],
            seed: [
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
                0x1c, 0x1d, 0x1e, 0x1f,
            ],
            ikm: Some(&[
                0x12, 0x5a, 0x63, 0x61, 0x73, 0x68, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x76, 0x65,
                0x63, 0x74, 0x6f, 0x72, 0x73, 0x20, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
            ]),
            path: &[],
            sk: [
                0xe9, 0xda, 0x88, 0x06, 0x40, 0x9d, 0xc3, 0xc3, 0xeb, 0xd1, 0xfc, 0x2a, 0x71, 0xc8,
                0x79, 0xc1, 0x3d, 0xd7, 0xaa, 0x93, 0xed, 0xe8, 0x03, 0xbf, 0x1a, 0x83, 0x41, 0x4b,
                0x9d, 0x3b, 0x15, 0x8a,
            ],
            c: [
                0x65, 0xa7, 0x48, 0xf2, 0x90, 0x5f, 0x7a, 0x8a, 0xab, 0x9f, 0x3d, 0x02, 0xf1, 0xb2,
                0x6c, 0x3d, 0x65, 0xc8, 0x29, 0x94, 0xce, 0x59, 0xa0, 0x86, 0xd4, 0xc6, 0x51, 0xd8,
                0xa8, 0x1c, 0xec, 0x51,
            ],
        },
        TestVector {
            context_string: &[
                0x5a, 0x63, 0x61, 0x73, 0x68, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x76, 0x65, 0x63,
                0x74, 0x6f, 0x72, 0x73,
            ],
            seed: [
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
                0x1c, 0x1d, 0x1e, 0x1f,
            ],
            ikm: None,
            path: &[2147483649],
            sk: [
                0xe8, 0x40, 0x9a, 0xaa, 0x83, 0x2c, 0xc2, 0x37, 0x8f, 0x2b, 0xad, 0xeb, 0x77, 0x15,
                0x05, 0x62, 0x15, 0x37, 0x42, 0xfe, 0xe8, 0x76, 0xdc, 0xf4, 0x78, 0x3a, 0x6c, 0xcd,
                0x11, 0x9d, 0xa6, 0x6a,
            ],
            c: [
                0xcc, 0x08, 0x49, 0x22, 0xa0, 0xea, 0xd2, 0xda, 0x53, 0x38, 0xbd, 0x82, 0x20, 0x0a,
                0x19, 0x46, 0xbc, 0x85, 0x85, 0xb8, 0xd9, 0xee, 0x41, 0x6d, 0xf6, 0xa0, 0x9a, 0x71,
                0xab, 0x0e, 0x5b, 0x58,
            ],
        },
        TestVector {
            context_string: &[
                0x5a, 0x63, 0x61, 0x73, 0x68, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x76, 0x65, 0x63,
                0x74, 0x6f, 0x72, 0x73,
            ],
            seed: [
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
                0x1c, 0x1d, 0x1e, 0x1f,
            ],
            ikm: None,
            path: &[2147483649, 2147483650],
            sk: [
                0x46, 0x4f, 0x90, 0xa3, 0x64, 0xcf, 0xf8, 0x05, 0xfe, 0xe9, 0x3a, 0x85, 0xb7, 0x2f,
                0x48, 0x94, 0xce, 0x4e, 0x13, 0x58, 0xdc, 0xdc, 0x1e, 0x61, 0xa3, 0xd4, 0x30, 0x30,
                0x1c, 0x60, 0x91, 0x0e,
            ],
            c: [
                0xf9, 0xd2, 0x54, 0x4a, 0x55, 0x28, 0xae, 0x6b, 0xd9, 0xf0, 0x36, 0xf4, 0x2f, 0x9f,
                0x05, 0xd8, 0x3d, 0xff, 0x50, 0x7a, 0xeb, 0x2a, 0x81, 0x41, 0xaf, 0x11, 0xd9, 0xf1,
                0x67, 0xe2, 0x21, 0xae,
            ],
        },
        TestVector {
            context_string: &[
                0x5a, 0x63, 0x61, 0x73, 0x68, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x76, 0x65, 0x63,
                0x74, 0x6f, 0x72, 0x73,
            ],
            seed: [
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
                0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b,
                0x1c, 0x1d, 0x1e, 0x1f,
            ],
            ikm: None,
            path: &[2147483649, 2147483650, 2147483651],
            sk: [
                0xfc, 0x4b, 0x6e, 0x93, 0xb0, 0xe4, 0x2f, 0x7a, 0x76, 0x2c, 0xa0, 0xc6, 0x52, 0x2c,
                0xcd, 0x10, 0x45, 0xca, 0xb5, 0x06, 0xb3, 0x72, 0x45, 0x2a, 0xf7, 0x30, 0x6c, 0x87,
                0x38, 0x9a, 0xb6, 0x2c,
            ],
            c: [
                0xe8, 0x9b, 0xf2, 0xed, 0x73, 0xf5, 0xe0, 0x88, 0x75, 0x42, 0xe3, 0x67, 0x93, 0xfa,
                0xc8, 0x2c, 0x50, 0x8a, 0xb5, 0xd9, 0x91, 0x98, 0x57, 0x82, 0x27, 0xb2, 0x41, 0xfb,
                0xac, 0x19, 0x84, 0x29,
            ],
        },
    ];

    #[test]
    fn test_vectors() {
        let context_string = b"Zcash test vectors";
        let full_path = [
            ChildIndex::hardened(1),
            ChildIndex::hardened(2),
            ChildIndex::hardened(3),
        ];

        for (i, tv) in TEST_VECTORS.iter().enumerate() {
            assert_eq!(tv.context_string, context_string);

            let path = tv
                .path
                .into_iter()
                .map(|i| ChildIndex::from_index(*i).expect("hardened"))
                .collect::<alloc::vec::Vec<_>>();
            assert_eq!(&full_path[..i], &path);

            // The derived master key should be identical to the key at the empty path.
            if let Some(mut tv_ikm) = tv.ikm {
                with_ikm(tv.context_string, &tv.seed, |ikm| {
                    for part in ikm {
                        assert_eq!(*part, &tv_ikm[..part.len()]);
                        tv_ikm = &tv_ikm[part.len()..];
                    }
                });

                let sk = SecretKey::master(context_string, &tv.seed);
                assert_eq!((sk.data(), sk.chain_code().as_bytes()), (&tv.sk, &tv.c));
            }

            let sk = SecretKey::from_path(tv.context_string, &tv.seed, &path);
            assert_eq!(sk.data(), &tv.sk);
            assert_eq!(sk.chain_code().as_bytes(), &tv.c);
        }
    }
}
