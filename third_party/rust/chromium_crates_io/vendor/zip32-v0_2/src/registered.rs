//! Registered key derivation.
//!
//! In the context of a particular application protocol defined by a ZIP, there is
//! sometimes a need to define an HD subtree that will not collide with keys derived
//! for other protocols, as far as that is possible to assure by following the
//! [ZIP process].
//!
//! Within this subtree, the application protocol may use derivation paths related to
//! those used for existing key material — for example, to derive an account-level key.
//! The instantiation of the [hardened key derivation framework] in this module may be
//! used for this purpose.
//!
//! It is strongly RECOMMENDED that implementors ensure that documentation of the
//! usage and derivation paths of the application protocol's key tree in the
//! corresponding ZIP is substantially complete, before public deployment of software
//! or hardware using this mechanism. The ZIP process allows for subsequent updates
//! and corrections.
//!
//! The functionality of this module is similar to that of the [`zip32::arbitrary`]
//! module, with the following improvements:
//!
//! - The key tree is associated with the ZIP that should document it, and cannot
//!   collide with the tree for any other ZIP.
//! - Child indices may include byte sequence tags.
//! - A 64-bit cryptovalue can be derived at the same path as any node in the tree,
//!   without any cryptographic unsafety.
//!
//! The keys derived by the functions in this module will be unrelated to any keys
//! derived by functions in the [`zip32::arbitrary`] module, even if the same context
//! string and seed are used.
//!
//! Defined in [ZIP 32: Registered key derivation][regkd].
//!
//! [hardened key derivation framework]: crate::hardened_only
//! [regkd]: https://zips.z.cash/zip-0032#specification-registered-key-derivation
//! [ZIP process]: https://zips.z.cash/zip-0000
//! [`zip32::arbitrary`]: `crate::arbitrary`

use core::fmt::Display;

use zcash_spec::PrfExpand;

use crate::{
    hardened_only::{Context, HardenedOnlyCkdDomain, HardenedOnlyKey},
    ChainCode, ChildIndex,
};

use super::with_ikm;

struct Registered;

impl Context for Registered {
    const MKG_DOMAIN: [u8; 16] = *b"ZIPRegistered_KD";
    const CKD_DOMAIN: HardenedOnlyCkdDomain = PrfExpand::REGISTERED_ZIP32_CHILD;
}

/// An error that occurred in cryptovalue derivation.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DerivationError {
    /// The provided seed data was invalid. A seed must be between 32 and 252 bytes in length,
    /// inclusive.
    SeedInvalid,
    /// The provided context string is invalid; context strings must be non-empty and no greater
    /// than 252 bytes in length.
    ContextStringInvalid,
    /// The provided subpath was empty. Empty subpaths are not permitted by this API, as the
    /// full-width cryptovalue at the empty subpath would be outside the allowed subtree
    /// rooted at `m_{context} / zip_number'`.
    SubpathEmpty,
}

impl Display for DerivationError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            DerivationError::SeedInvalid => {
                write!(f, "Seed must be between 32 and 252 bytes, inclusive.")
            }
            DerivationError::ContextStringInvalid => write!(
                f,
                "Context string must be between 1 and 252 bytes, inclusive."
            ),
            DerivationError::SubpathEmpty => write!(
                f,
                "ZIP 32 registered 64-byte cryptovalue subpaths must have at least one element."
            ),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for DerivationError {}

/// A ZIP 32 registered key derivation path element, consisting of a child index and an
/// optionally-empty tag value.
pub struct PathElement<'a> {
    child_index: ChildIndex,
    tag: &'a [u8],
}

impl<'a> PathElement<'a> {
    /// Constructs a new [`PathElement`] from its constituent parts.
    pub fn new(child_index: ChildIndex, tag: &'a [u8]) -> Self {
        Self { child_index, tag }
    }

    /// Returns the index at which the child key will be derived.
    pub fn child_index(&self) -> ChildIndex {
        self.child_index
    }

    /// Returns the tag that will be used in derivation of the child key.
    pub fn tag(&self) -> &[u8] {
        self.tag
    }
}

/// A registered extended secret key.
///
/// Defined in [ZIP 32: Registered key derivation][regkd].
///
/// [regkd]: https://zips.z.cash/zip-0032#specification-registered-key-derivation
pub struct SecretKey {
    inner: HardenedOnlyKey<Registered>,
}

impl SecretKey {
    /// Derives a key for a registered application protocol at the given path from the
    /// given seed. Each path element may consist of an index and (possibly empty) tag.
    ///
    /// - `context_string`: an identifier for the context in which this key will be used. It must
    ///   be globally unique, non-empty, and no more than 252 bytes in length.
    /// - `seed`: the root seed. Must be between 32 bytes and 252 bytes in length, inclusive.
    /// - `zip_number`: the number of the ZIP defining the application protocol. The corresponding
    ///   hardened index (with empty tag) will be prepended to the `subpath` to obtain the ZIP 32
    ///   path.
    /// - `subpath`: the path to the desired child element.
    pub fn from_subpath(
        context_string: &[u8],
        seed: &[u8],
        zip_number: u16,
        subpath: &[PathElement<'_>],
    ) -> Result<Self, DerivationError> {
        if context_string.is_empty() || context_string.len() > 252 {
            return Err(DerivationError::ContextStringInvalid);
        }
        if seed.len() < 32 || seed.len() > 252 {
            return Err(DerivationError::SeedInvalid);
        }

        let mut xsk = Self::master(context_string, seed)
            .derive_child(ChildIndex::hardened(u32::from(zip_number)));

        for elem in subpath {
            xsk = xsk.derive_child_with_tag(elem.child_index, elem.tag);
        }
        Ok(xsk)
    }

    /// Constructs a key for a registered application protocol from its constituent parts.
    ///
    /// This is a low-level API. The constructor must only be called with parts that were
    /// obtained from previous calls to [`key.data()`][`Self::data`] and
    /// [`key.chain_code()`][`Self::chain_code`] for some `key: registered::SecretKey`.
    pub fn from_parts(sk: [u8; 32], chain_code: ChainCode) -> Self {
        Self {
            inner: HardenedOnlyKey::from_parts(sk, chain_code),
        }
    }

    /// Generates the master key of a registered extended secret key.
    /// This should not be exposed directly. It is defined as an intermediate
    /// value in [ZIP 32: Registered subtree root key generation][regroot].
    ///
    /// [regroot]: https://zips.z.cash/zip-0032#registered-subtree-root-key-generation
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

    /// Derives a child key from a parent key at a given index and empty tag.
    ///
    /// This is a convenience function equivalent to
    /// `self.derive_child_with_tag(index, &[])`.
    pub fn derive_child(&self, index: ChildIndex) -> Self {
        self.derive_child_with_tag(index, &[])
    }

    /// Derives a child key from a parent key at a given index and (possibly empty) tag.
    ///
    /// Defined in [ZIP 32: Registered child key derivation][regckd].
    ///
    /// [regckd]: https://zips.z.cash/zip-0032#registered-child-key-derivation
    pub fn derive_child_with_tag(&self, index: ChildIndex, tag: &[u8]) -> Self {
        Self {
            inner: self.inner.derive_child_with_tag(index, tag),
        }
    }

    /// Derives a 64-byte child cryptovalue from a parent key at a given index
    /// and (possibly empty) tag.
    ///
    /// Defined in [ZIP 32: Full-width child cryptovalue derivation][fwccd].
    ///
    /// [fwccd]: https://zips.z.cash/zip-0032#full-width-child-cryptovalue-derivation
    pub fn derive_child_cryptovalue(&self, index: ChildIndex, tag: &[u8]) -> [u8; 64] {
        self.inner.ckdh_internal(index, 1, tag)
    }

    /// Returns the key material for this key.
    pub fn data(&self) -> &[u8; 32] {
        self.inner.parts().0
    }

    /// Returns the chain code for this key.
    pub fn chain_code(&self) -> &ChainCode {
        self.inner.parts().1
    }
}

/// Derives a 64-byte cryptovalue (for use as key material for example), for a registered
/// application protocol at the given non-empty subpath from the given seed. Each subpath element
/// may consist of an index and a (possibly empty) tag.
///
/// - `context_string`: an identifier for the context in which this key will be used. It must be
///   globally unique, non-empty, and no more than 252 bytes in length.
/// - `seed`: the root seed. Must be between 32 bytes and 252 bytes in length, inclusive.
/// - `zip_number`: the number of the ZIP defining the application protocol. The corresponding
///   hardened index (with empty tag) will be prepended to the `subpath` to obtain the ZIP 32 path.
/// - `subpath`: the path to the desired child element. A non-empty path is required, in order
///   to ensure that the resulting full-width cryptovalue is within the allowed subtree rooted
///   at `m_{context} / zip_number'`.
pub fn cryptovalue_from_subpath(
    context_string: &[u8],
    seed: &[u8],
    zip_number: u16,
    subpath: &[PathElement<'_>],
) -> Result<[u8; 64], DerivationError> {
    if context_string.is_empty() || context_string.len() > 252 {
        return Err(DerivationError::ContextStringInvalid);
    }
    if seed.len() < 32 || seed.len() > 252 {
        return Err(DerivationError::SeedInvalid);
    }
    // We can't use NonEmpty because it requires allocation.
    if subpath.is_empty() {
        return Err(DerivationError::SubpathEmpty);
    }

    let mut xsk = SecretKey::master(context_string, seed)
        .derive_child(ChildIndex::hardened(u32::from(zip_number)));

    for elem in subpath.iter().take(subpath.len() - 1) {
        xsk = xsk.derive_child_with_tag(elem.child_index, elem.tag);
    }
    let elem = subpath.last().expect("nonempty");
    Ok(xsk.derive_child_cryptovalue(elem.child_index, elem.tag))
}

#[cfg(test)]
mod tests {
    use alloc::string::ToString;

    use assert_matches::assert_matches;

    use crate::{fingerprint::SeedFingerprint, registered::PathElement};

    use super::{cryptovalue_from_subpath, ChildIndex, DerivationError, SecretKey};

    #[test]
    fn test_cryptovalue_from_empty_subpath_errors() {
        assert_eq!(
            cryptovalue_from_subpath(&[0], &[0; 32], 32, &[]),
            Err(DerivationError::SubpathEmpty),
        );
    }

    struct TestVector {
        context_string: &'static [u8],
        seed: [u8; 32],
        seedfp: &'static str,
        zip_number: u16,
        subpath: &'static [(u32, &'static [u8])],
        sk: [u8; 32],
        c: [u8; 32],
        full_width: Option<[u8; 64]>,
    }

    // From https://github.com/zcash-hackworks/zcash-test-vectors/blob/master/zip_0032_registered.py
    const TEST_VECTORS: &[TestVector] = &[
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
            seedfp: "zip32seedfp1mmlkqnpyvug0w9mdatgz4f6x7t7c65uf7urj24kuk42lm0j78t3sne2h0z",
            zip_number: 1,
            subpath: &[],
            sk: [
                0x53, 0xa7, 0x15, 0x07, 0xe6, 0xdf, 0xda, 0x58, 0x8b, 0xc1, 0xe1, 0x38, 0xc2, 0x65,
                0x7c, 0x92, 0x69, 0xe5, 0x5f, 0x5d, 0x9b, 0x99, 0xe3, 0x88, 0x7c, 0x13, 0x40, 0x08,
                0x19, 0x3a, 0x2f, 0x47,
            ],
            c: [
                0x08, 0xbb, 0x26, 0xaa, 0xe2, 0x1d, 0x4e, 0xfd, 0xc3, 0x24, 0x9b, 0x95, 0x57, 0xfc,
                0xd9, 0x13, 0x1e, 0x8b, 0x98, 0x27, 0x24, 0x1d, 0x9f, 0x61, 0xd0, 0xd7, 0x74, 0xbb,
                0x4f, 0xed, 0x3d, 0xe6,
            ],
            full_width: None,
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
            seedfp: "zip32seedfp1mmlkqnpyvug0w9mdatgz4f6x7t7c65uf7urj24kuk42lm0j78t3sne2h0z",
            zip_number: 1,
            subpath: &[(
                2147483650,
                &[
                    0x74, 0x72, 0x61, 0x6e, 0x73, 0x20, 0x72, 0x69, 0x67, 0x68, 0x74, 0x73, 0x20,
                    0x61, 0x72, 0x65, 0x20, 0x68, 0x75, 0x6d, 0x61, 0x6e, 0x20, 0x72, 0x69, 0x67,
                    0x68, 0x74, 0x73,
                ],
            )],
            sk: [
                0x02, 0xdc, 0x25, 0xcc, 0x40, 0x31, 0x0e, 0xed, 0x08, 0xb0, 0x28, 0xe0, 0x7f, 0xae,
                0x9a, 0xdb, 0xee, 0x2f, 0xbe, 0x56, 0xa4, 0x69, 0x4d, 0xef, 0x04, 0x01, 0xe6, 0x56,
                0xdf, 0xae, 0x02, 0x11,
            ],
            c: [
                0xd8, 0xf9, 0xd8, 0xa1, 0xf8, 0x1d, 0x1b, 0x5d, 0x55, 0x06, 0xb5, 0xff, 0x94, 0x2d,
                0x2f, 0xf3, 0xda, 0xe7, 0xa6, 0x3f, 0x57, 0xd6, 0xb8, 0xc7, 0xfb, 0xe5, 0x81, 0x49,
                0x82, 0x3c, 0xc6, 0xec,
            ],
            full_width: Some([
                0x25, 0x5d, 0x75, 0xb5, 0xf9, 0x7d, 0xd8, 0x80, 0xa1, 0x44, 0x60, 0xab, 0x0a, 0x28,
                0x93, 0x8e, 0x7b, 0xa4, 0x97, 0xce, 0xb1, 0x45, 0x7f, 0xff, 0x29, 0x92, 0xe9, 0x01,
                0x5a, 0x84, 0x03, 0xf8, 0xc0, 0x81, 0x12, 0xb7, 0xa9, 0x4c, 0xf5, 0x39, 0xc2, 0x1c,
                0x9d, 0xa7, 0xee, 0x99, 0x89, 0x7b, 0xe9, 0x47, 0x6b, 0x68, 0x13, 0x53, 0x2e, 0xe2,
                0x2c, 0x89, 0x47, 0xd7, 0x53, 0xb7, 0x2b, 0xdf,
            ]),
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
            seedfp: "zip32seedfp1mmlkqnpyvug0w9mdatgz4f6x7t7c65uf7urj24kuk42lm0j78t3sne2h0z",
            zip_number: 1,
            subpath: &[
                (
                    2147483650,
                    &[
                        0x74, 0x72, 0x61, 0x6e, 0x73, 0x20, 0x72, 0x69, 0x67, 0x68, 0x74, 0x73,
                        0x20, 0x61, 0x72, 0x65, 0x20, 0x68, 0x75, 0x6d, 0x61, 0x6e, 0x20, 0x72,
                        0x69, 0x67, 0x68, 0x74, 0x73,
                    ],
                ),
                (2147483651, &[]),
            ],
            sk: [
                0xa1, 0x27, 0xdb, 0x66, 0x62, 0x8b, 0x25, 0x6e, 0x5b, 0x66, 0x4d, 0x54, 0x05, 0x0c,
                0x1e, 0x6b, 0x02, 0x89, 0x63, 0xae, 0xa2, 0x2b, 0x04, 0xd1, 0xbc, 0x6f, 0x48, 0x12,
                0x36, 0x74, 0xed, 0x82,
            ],
            c: [
                0x34, 0x00, 0x84, 0x03, 0x36, 0x05, 0xed, 0xca, 0x11, 0x46, 0x3f, 0xfe, 0xc5, 0x6b,
                0xf0, 0xca, 0xc4, 0x25, 0xc4, 0x10, 0xe9, 0x53, 0x62, 0x86, 0x71, 0xce, 0xc6, 0xa6,
                0x51, 0x4c, 0x32, 0xa8,
            ],
            full_width: Some([
                0x7f, 0x85, 0x3e, 0xef, 0x00, 0x1b, 0x1b, 0xc5, 0xa1, 0xa5, 0xe6, 0x7f, 0x5d, 0xfd,
                0x0e, 0x90, 0x42, 0x75, 0x96, 0xd4, 0x84, 0x2f, 0x5b, 0x10, 0xa1, 0x11, 0xe9, 0x7c,
                0x40, 0x73, 0x20, 0x3c, 0xed, 0xf6, 0xb8, 0x0a, 0x85, 0x14, 0x5e, 0x50, 0x61, 0xac,
                0xd2, 0x9b, 0xc5, 0xa4, 0xe3, 0x49, 0xb1, 0x4f, 0x85, 0x57, 0xa7, 0x03, 0x3e, 0x23,
                0xb0, 0x66, 0xb7, 0xce, 0x24, 0x09, 0xd9, 0x73,
            ]),
        },
    ];

    #[test]
    fn test_vectors() {
        for tv in TEST_VECTORS {
            let seedfp = SeedFingerprint::from_seed(&tv.seed).unwrap();
            assert_eq!(seedfp.to_string(), tv.seedfp);
            assert_matches!(tv.seedfp.parse::<SeedFingerprint>(), Ok(fp) if fp == seedfp);

            let subpath = tv
                .subpath
                .iter()
                .map(|(i, tag)| {
                    PathElement::new(ChildIndex::from_index(*i).expect("hardened"), tag)
                })
                .collect::<alloc::vec::Vec<_>>();

            let sk = SecretKey::from_subpath(tv.context_string, &tv.seed, tv.zip_number, &subpath)
                .unwrap();
            assert_eq!(sk.data(), &tv.sk);
            assert_eq!(sk.chain_code().as_bytes(), &tv.c);

            let fw = (!subpath.is_empty()).then(|| {
                cryptovalue_from_subpath(tv.context_string, &tv.seed, tv.zip_number, &subpath)
                    .unwrap()
            });
            assert_eq!(&fw, &tv.full_width);
            if let Some(fw) = fw {
                assert_ne!(&fw[..32], &tv.sk);
                assert_ne!(&fw[32..], &tv.c);
            }
        }
    }
}
