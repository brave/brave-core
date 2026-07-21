//! Key structures for Orchard.

use core::fmt;

use blake2b_simd::Params as Blake2bParams;
use subtle::{Choice, ConstantTimeEq, CtOption};
use zcash_spec::VariableLengthSlice;
use zip32::{
    hardened_only::{self, HardenedOnlyKey},
    ChainCode,
};

use crate::{
    keys::{FullViewingKey, SpendingKey},
    spec::PrfExpand,
};

pub use zip32::ChildIndex;

const ZIP32_ORCHARD_PERSONALIZATION: &[u8; 16] = b"ZcashIP32Orchard";
const ZIP32_ORCHARD_FVFP_PERSONALIZATION: &[u8; 16] = b"ZcashOrchardFVFP";

/// Errors produced in derivation of extended spending keys
#[derive(Debug, PartialEq, Eq)]
#[non_exhaustive]
pub enum Error {
    /// A seed resulted in an invalid spending key
    InvalidSpendingKey,
    /// A child index in a derivation path exceeded 2^31
    InvalidChildIndex(u32),
    /// Derivation depth would exceed 255
    MaxDerivationDepth,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Seed produced invalid spending key.")
    }
}

#[cfg(feature = "std")]
impl std::error::Error for Error {}

/// An Orchard full viewing key fingerprint
struct FvkFingerprint([u8; 32]);

impl From<&FullViewingKey> for FvkFingerprint {
    fn from(fvk: &FullViewingKey) -> Self {
        let mut h = Blake2bParams::new()
            .hash_length(32)
            .personal(ZIP32_ORCHARD_FVFP_PERSONALIZATION)
            .to_state();
        h.update(&fvk.to_bytes());
        let mut fvfp = [0u8; 32];
        fvfp.copy_from_slice(h.finalize().as_bytes());
        FvkFingerprint(fvfp)
    }
}

/// An Orchard full viewing key tag
#[derive(Clone, Copy, Debug, PartialEq)]
struct FvkTag([u8; 4]);

impl FvkFingerprint {
    fn tag(&self) -> FvkTag {
        let mut tag = [0u8; 4];
        tag.copy_from_slice(&self.0[..4]);
        FvkTag(tag)
    }
}

impl FvkTag {
    fn master() -> Self {
        FvkTag([0u8; 4])
    }
}

/// The derivation index associated with a key.
///
/// Master keys are never derived via the ZIP 32 child derivation process, but they have
/// an index in their encoding. This type allows the encoding to be represented, while
/// also enabling the derivation methods to only accept [`ChildIndex`].
#[derive(Clone, Copy, Debug)]
struct KeyIndex(CtOption<ChildIndex>);

impl ConstantTimeEq for KeyIndex {
    fn ct_eq(&self, other: &Self) -> Choice {
        // We use a `CtOption` above instead of an enum so that we can implement this.
        self.0.ct_eq(&other.0)
    }
}

impl PartialEq for KeyIndex {
    fn eq(&self, other: &Self) -> bool {
        self.ct_eq(other).into()
    }
}

impl Eq for KeyIndex {}

impl KeyIndex {
    fn master() -> Self {
        Self(CtOption::new(ChildIndex::hardened(0), 0.into()))
    }

    fn child(i: ChildIndex) -> Self {
        Self(CtOption::new(i, 1.into()))
    }

    fn new(depth: u8, i: u32) -> Option<Self> {
        match (depth == 0, i) {
            (true, 0) => Some(KeyIndex::master()),
            (false, _) => ChildIndex::from_index(i).map(KeyIndex::child),
            _ => None,
        }
    }

    fn index(&self) -> u32 {
        if self.0.is_some().into() {
            self.0.unwrap().index()
        } else {
            0
        }
    }
}

#[derive(Clone, Copy, Debug)]
struct Orchard;

impl hardened_only::Context for Orchard {
    const MKG_DOMAIN: [u8; 16] = *ZIP32_ORCHARD_PERSONALIZATION;
    const CKD_DOMAIN: PrfExpand<([u8; 32], [u8; 4], [u8; 1], VariableLengthSlice)> =
        PrfExpand::ORCHARD_ZIP32_CHILD;
}

/// An Orchard extended spending key.
///
/// Defined in [ZIP32: Orchard extended keys][orchardextendedkeys].
///
/// [orchardextendedkeys]: https://zips.z.cash/zip-0032#orchard-extended-keys
#[derive(Debug, Clone)]
pub struct ExtendedSpendingKey {
    depth: u8,
    parent_fvk_tag: FvkTag,
    child_index: KeyIndex,
    inner: HardenedOnlyKey<Orchard>,
}

impl ConstantTimeEq for ExtendedSpendingKey {
    fn ct_eq(&self, rhs: &Self) -> Choice {
        self.depth.ct_eq(&rhs.depth)
            & self.parent_fvk_tag.0.ct_eq(&rhs.parent_fvk_tag.0)
            & self.child_index.ct_eq(&rhs.child_index)
            & self.inner.ct_eq(&rhs.inner)
    }
}

#[allow(non_snake_case)]
impl ExtendedSpendingKey {
    /// Returns the spending key of the child key corresponding to
    /// the path derived from the master key
    ///
    /// # Panics
    ///
    /// Panics if seed results in invalid spending key.
    pub fn from_path(seed: &[u8], path: &[ChildIndex]) -> Result<Self, Error> {
        let mut xsk = Self::master(seed)?;
        for i in path {
            xsk = xsk.derive_child(*i)?;
        }
        Ok(xsk)
    }

    /// Generates the master key of an Orchard extended spending key.
    ///
    /// Defined in [ZIP32: Orchard master key generation][orchardmasterkey].
    ///
    /// [orchardmasterkey]: https://zips.z.cash/zip-0032#orchard-master-key-generation
    ///
    /// # Panics
    ///
    /// Panics if the seed is shorter than 32 bytes or longer than 252 bytes.
    pub fn master(seed: &[u8]) -> Result<Self, Error> {
        let m_orchard = HardenedOnlyKey::master(&[seed]);

        let sk = SpendingKey::from_bytes(*m_orchard.parts().0);
        if sk.is_none().into() {
            return Err(Error::InvalidSpendingKey);
        }

        // For the master extended spending key, depth is 0, parent_fvk_tag is 4 zero bytes, and i is 0.
        Ok(Self {
            depth: 0,
            parent_fvk_tag: FvkTag([0; 4]),
            child_index: KeyIndex::master(),
            inner: m_orchard,
        })
    }

    /// Derives a child key from a parent key at a given index.
    ///
    /// Defined in [ZIP32: Orchard child key derivation][orchardchildkey].
    ///
    /// [orchardchildkey]: https://zips.z.cash/zip-0032#orchard-child-key-derivation
    ///
    /// Discards index if it results in an invalid sk
    pub fn derive_child(&self, index: ChildIndex) -> Result<Self, Error> {
        let depth = self.depth.checked_add(1).ok_or(Error::MaxDerivationDepth)?;

        let child_i = self.inner.derive_child(index);

        let sk = SpendingKey::from_bytes(*child_i.parts().0);
        if sk.is_none().into() {
            return Err(Error::InvalidSpendingKey);
        }

        let fvk: FullViewingKey = self.into();

        Ok(Self {
            depth,
            parent_fvk_tag: FvkFingerprint::from(&fvk).tag(),
            child_index: KeyIndex::child(index),
            inner: child_i,
        })
    }

    /// Returns sk of this ExtendedSpendingKey.
    pub fn sk(&self) -> SpendingKey {
        SpendingKey::from_bytes(*self.inner.parts().0).expect("checked during derivation")
    }

    /// Returns the chain code for this ExtendedSpendingKey.
    fn chain_code(&self) -> &ChainCode {
        self.inner.parts().1
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn derive_child() {
        let seed = [0; 32];
        let xsk_m = ExtendedSpendingKey::master(&seed).unwrap();

        let i_5 = ChildIndex::hardened(5);
        let xsk_5 = xsk_m.derive_child(i_5);

        assert!(xsk_5.is_ok());
    }

    #[test]
    fn derive_child_depth_overflow() {
        let seed = [0; 32];
        let mut xsk = ExtendedSpendingKey::master(&seed).unwrap();

        let i_5 = ChildIndex::hardened(5);
        for _ in 0..255 {
            xsk = xsk.derive_child(i_5).unwrap();
        }
        assert_eq!(xsk.depth, 255);
        assert!(matches!(
            xsk.derive_child(i_5),
            Err(Error::MaxDerivationDepth)
        ));
    }

    #[test]
    fn path() {
        let seed = [0; 32];
        let xsk_m = ExtendedSpendingKey::master(&seed).unwrap();

        let xsk_5h = xsk_m.derive_child(ChildIndex::hardened(5)).unwrap();
        assert!(bool::from(
            ExtendedSpendingKey::from_path(&seed, &[ChildIndex::hardened(5)])
                .unwrap()
                .ct_eq(&xsk_5h)
        ));

        let xsk_5h_7 = xsk_5h.derive_child(ChildIndex::hardened(7)).unwrap();
        assert!(bool::from(
            ExtendedSpendingKey::from_path(
                &seed,
                &[ChildIndex::hardened(5), ChildIndex::hardened(7)]
            )
            .unwrap()
            .ct_eq(&xsk_5h_7)
        ));
    }

    #[test]
    fn test_vectors() {
        let test_vectors = crate::test_vectors::zip32::TEST_VECTORS;

        let seed = [
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
            24, 25, 26, 27, 28, 29, 30, 31,
        ];

        let i1h = ChildIndex::hardened(1);
        let i2h = ChildIndex::hardened(2);
        let i3h = ChildIndex::hardened(3);

        let m = ExtendedSpendingKey::master(&seed).unwrap();
        let m_1h = m.derive_child(i1h).unwrap();
        let m_1h_2h = ExtendedSpendingKey::from_path(&seed, &[i1h, i2h]).unwrap();
        let m_1h_2h_3h = m_1h_2h.derive_child(i3h).unwrap();

        let xsks = [m, m_1h, m_1h_2h, m_1h_2h_3h];
        assert_eq!(test_vectors.len(), xsks.len());

        for (xsk, tv) in xsks.iter().zip(test_vectors.iter()) {
            assert_eq!(xsk.sk().to_bytes(), &tv.sk);
            assert_eq!(xsk.chain_code().as_bytes(), &tv.c);

            assert_eq!(xsk.depth, tv.xsk[0]);
            assert_eq!(&xsk.parent_fvk_tag.0, &tv.xsk[1..5]);
            assert_eq!(&xsk.child_index.index().to_le_bytes(), &tv.xsk[5..9]);
            assert_eq!(xsk.chain_code().as_bytes(), &tv.xsk[9..9 + 32]);
            assert_eq!(xsk.sk().to_bytes(), &tv.xsk[9 + 32..]);

            let fvk: FullViewingKey = (&xsk.sk()).into();
            assert_eq!(FvkFingerprint::from(&fvk).0, tv.fp);
        }
    }
}
