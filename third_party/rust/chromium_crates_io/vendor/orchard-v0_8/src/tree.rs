//! Types related to Orchard note commitment trees and anchors.

use core::iter;

use crate::{
    constants::{
        sinsemilla::{i2lebsp_k, L_ORCHARD_MERKLE, MERKLE_CRH_PERSONALIZATION},
        MERKLE_DEPTH_ORCHARD,
    },
    note::commitment::ExtractedNoteCommitment,
};

use halo2_gadgets::sinsemilla::primitives::HashDomain;
use incrementalmerkletree::{Hashable, Level};
use pasta_curves::pallas;

use ff::{Field, PrimeField, PrimeFieldBits};
use lazy_static::lazy_static;
use rand::RngCore;
use serde::de::{Deserializer, Error};
use serde::ser::Serializer;
use serde::{Deserialize, Serialize};
use subtle::{Choice, ConditionallySelectable, CtOption};

// The uncommitted leaf is defined as pallas::Base(2).
// <https://zips.z.cash/protocol/protocol.pdf#thmuncommittedorchard>
lazy_static! {
    static ref UNCOMMITTED_ORCHARD: pallas::Base = pallas::Base::from(2);
    pub(crate) static ref EMPTY_ROOTS: Vec<MerkleHashOrchard> = {
        iter::empty()
            .chain(Some(MerkleHashOrchard::empty_leaf()))
            .chain(
                (0..MERKLE_DEPTH_ORCHARD).scan(MerkleHashOrchard::empty_leaf(), |state, l| {
                    let l = l as u8;
                    *state = MerkleHashOrchard::combine(l.into(), state, state);
                    Some(*state)
                }),
            )
            .collect()
    };
}

/// The root of an Orchard commitment tree. This must be a value
/// in the range {0..=q_ℙ-1}
#[derive(Eq, PartialEq, Clone, Copy, Debug)]
pub struct Anchor(pallas::Base);

impl From<pallas::Base> for Anchor {
    fn from(anchor_field: pallas::Base) -> Anchor {
        Anchor(anchor_field)
    }
}

impl From<MerkleHashOrchard> for Anchor {
    fn from(anchor: MerkleHashOrchard) -> Anchor {
        Anchor(anchor.0)
    }
}

impl Anchor {
    /// The anchor of the empty Orchard note commitment tree.
    ///
    /// This anchor does not correspond to any valid anchor for a spend, so it
    /// may only be used for coinbase bundles or in circumstances where Orchard
    /// functionality is not active.
    pub fn empty_tree() -> Anchor {
        Anchor(MerkleHashOrchard::empty_root(Level::from(MERKLE_DEPTH_ORCHARD as u8)).0)
    }

    pub(crate) fn inner(&self) -> pallas::Base {
        self.0
    }

    /// Parses an Orchard anchor from a byte encoding.
    pub fn from_bytes(bytes: [u8; 32]) -> CtOption<Anchor> {
        pallas::Base::from_repr(bytes).map(Anchor)
    }

    /// Returns the byte encoding of this anchor.
    pub fn to_bytes(self) -> [u8; 32] {
        self.0.to_repr()
    }
}

/// The Merkle path from a leaf of the note commitment tree
/// to its anchor.
#[derive(Debug)]
pub struct MerklePath {
    position: u32,
    auth_path: [MerkleHashOrchard; MERKLE_DEPTH_ORCHARD],
}

#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
impl From<(incrementalmerkletree::Position, Vec<MerkleHashOrchard>)> for MerklePath {
    fn from(path: (incrementalmerkletree::Position, Vec<MerkleHashOrchard>)) -> Self {
        let position: u64 = path.0.into();
        Self {
            position: position as u32,
            auth_path: path.1.try_into().unwrap(),
        }
    }
}

impl From<incrementalmerkletree::MerklePath<MerkleHashOrchard, 32>> for MerklePath {
    fn from(path: incrementalmerkletree::MerklePath<MerkleHashOrchard, 32>) -> Self {
        let position: u64 = path.position().into();
        Self {
            position: position as u32,
            auth_path: path.path_elems().try_into().unwrap(),
        }
    }
}

impl MerklePath {
    /// Generates a dummy Merkle path for use in dummy spent notes.
    pub(crate) fn dummy(mut rng: &mut impl RngCore) -> Self {
        MerklePath {
            position: rng.next_u32(),
            auth_path: [(); MERKLE_DEPTH_ORCHARD]
                .map(|_| MerkleHashOrchard(pallas::Base::random(&mut rng))),
        }
    }

    /// Instantiates a new Merkle path given a leaf position and authentication path.
    pub(crate) fn new(position: u32, auth_path: [pallas::Base; MERKLE_DEPTH_ORCHARD]) -> Self {
        Self::from_parts(position, auth_path.map(MerkleHashOrchard))
    }

    /// Instantiates a new Merkle path given a leaf position and authentication path.
    pub fn from_parts(position: u32, auth_path: [MerkleHashOrchard; MERKLE_DEPTH_ORCHARD]) -> Self {
        Self {
            position,
            auth_path,
        }
    }

    /// <https://zips.z.cash/protocol/protocol.pdf#orchardmerklecrh>
    /// The layer with 2^n nodes is called "layer n":
    ///      - leaves are at layer MERKLE_DEPTH_ORCHARD = 32;
    ///      - the root is at layer 0.
    /// `l` is MERKLE_DEPTH_ORCHARD - layer - 1.
    ///      - when hashing two leaves, we produce a node on the layer above the leaves, i.e.
    ///        layer = 31, l = 0
    ///      - when hashing to the final root, we produce the anchor with layer = 0, l = 31.
    pub fn root(&self, cmx: ExtractedNoteCommitment) -> Anchor {
        self.auth_path
            .iter()
            .enumerate()
            .fold(MerkleHashOrchard::from_cmx(&cmx), |node, (l, sibling)| {
                let l = l as u8;
                if self.position & (1 << l) == 0 {
                    MerkleHashOrchard::combine(l.into(), &node, sibling)
                } else {
                    MerkleHashOrchard::combine(l.into(), sibling, &node)
                }
            })
            .into()
    }

    /// Returns the position of the leaf using this Merkle path.
    pub(crate) fn position(&self) -> u32 {
        self.position
    }

    /// Returns the authentication path.
    pub(crate) fn auth_path(&self) -> [MerkleHashOrchard; MERKLE_DEPTH_ORCHARD] {
        self.auth_path
    }
}

/// A newtype wrapper for leaves and internal nodes in the Orchard
/// incremental note commitment tree.
#[derive(Copy, Clone, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct MerkleHashOrchard(pallas::Base);

impl MerkleHashOrchard {
    /// Creates an incremental tree leaf digest from the specified
    /// Orchard extracted note commitment.
    pub fn from_cmx(value: &ExtractedNoteCommitment) -> Self {
        MerkleHashOrchard(value.inner())
    }

    /// Only used in the circuit.
    pub(crate) fn inner(&self) -> pallas::Base {
        self.0
    }

    /// Convert this digest to its canonical byte representation.
    pub fn to_bytes(&self) -> [u8; 32] {
        self.0.to_repr()
    }

    /// Parses a incremental tree leaf digest from the bytes of
    /// a note commitment.
    ///
    /// Returns the empty `CtOption` if the provided bytes represent
    /// a non-canonical encoding.
    pub fn from_bytes(bytes: &[u8; 32]) -> CtOption<Self> {
        pallas::Base::from_repr(*bytes).map(MerkleHashOrchard)
    }
}

impl ConditionallySelectable for MerkleHashOrchard {
    fn conditional_select(a: &Self, b: &Self, choice: Choice) -> Self {
        MerkleHashOrchard(pallas::Base::conditional_select(&a.0, &b.0, choice))
    }
}

impl Hashable for MerkleHashOrchard {
    fn empty_leaf() -> Self {
        MerkleHashOrchard(*UNCOMMITTED_ORCHARD)
    }

    /// Implements `MerkleCRH^Orchard` as defined in
    /// <https://zips.z.cash/protocol/protocol.pdf#orchardmerklecrh>
    ///
    /// The layer with 2^n nodes is called "layer n":
    ///      - leaves are at layer MERKLE_DEPTH_ORCHARD = 32;
    ///      - the root is at layer 0.
    /// `l` is MERKLE_DEPTH_ORCHARD - layer - 1.
    ///      - when hashing two leaves, we produce a node on the layer above the leaves, i.e.
    ///        layer = 31, l = 0
    ///      - when hashing to the final root, we produce the anchor with layer = 0, l = 31.
    fn combine(level: Level, left: &Self, right: &Self) -> Self {
        // MerkleCRH Sinsemilla hash domain.
        let domain = HashDomain::new(MERKLE_CRH_PERSONALIZATION);

        MerkleHashOrchard(
            domain
                .hash(
                    iter::empty()
                        .chain(i2lebsp_k(level.into()).iter().copied())
                        .chain(left.0.to_le_bits().iter().by_vals().take(L_ORCHARD_MERKLE))
                        .chain(right.0.to_le_bits().iter().by_vals().take(L_ORCHARD_MERKLE)),
                )
                .unwrap_or(pallas::Base::zero()),
        )
    }

    fn empty_root(level: Level) -> Self {
        EMPTY_ROOTS[<usize>::from(level)]
    }
}

impl Serialize for MerkleHashOrchard {
    fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
        self.to_bytes().serialize(serializer)
    }
}

impl<'de> Deserialize<'de> for MerkleHashOrchard {
    fn deserialize<D: Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        let parsed = <[u8; 32]>::deserialize(deserializer)?;
        <Option<_>>::from(Self::from_bytes(&parsed)).ok_or_else(|| {
            Error::custom(
            "Attempted to deserialize a non-canonical representation of a Pallas base field element.",
        )
        })
    }
}

/// Test utilities available under the `test-dependencies` feature flag.
#[cfg(feature = "test-dependencies")]
pub mod testing {
    use ff::Field;
    use rand::{
        distributions::{Distribution, Standard},
        RngCore,
    };

    use super::MerkleHashOrchard;

    impl MerkleHashOrchard {
        /// Return a random fake `MerkleHashOrchard`.
        pub fn random(rng: &mut impl RngCore) -> Self {
            Standard.sample(rng)
        }
    }

    impl Distribution<MerkleHashOrchard> for Standard {
        fn sample<R: rand::Rng + ?Sized>(&self, rng: &mut R) -> MerkleHashOrchard {
            MerkleHashOrchard(pasta_curves::Fp::random(rng))
        }
    }
}

#[cfg(test)]
mod tests {
    use {
        crate::tree::{MerkleHashOrchard, EMPTY_ROOTS},
        bridgetree::{BridgeTree, Frontier as BridgeFrontier},
        group::ff::PrimeField,
        incrementalmerkletree::Level,
        pasta_curves::pallas,
    };

    #[test]
    fn test_vectors() {
        let tv_empty_roots = crate::test_vectors::commitment_tree::test_vectors().empty_roots;

        for (height, root) in EMPTY_ROOTS.iter().enumerate() {
            assert_eq!(tv_empty_roots[height], root.to_bytes());
        }

        let mut tree = BridgeTree::<MerkleHashOrchard, u32, 4>::new(100);
        for (i, tv) in crate::test_vectors::merkle_path::test_vectors()
            .into_iter()
            .enumerate()
        {
            let cmx = MerkleHashOrchard::from_bytes(&tv.leaves[i]).unwrap();
            tree.append(cmx);
            let position = tree.mark().expect("tree is not empty");
            assert_eq!(position, (i as u64).into());

            let root = tree.root(0).unwrap();
            assert_eq!(root.0, pallas::Base::from_repr(tv.root).unwrap());

            // Check paths for all leaves up to this point. The test vectors include paths
            // for not-yet-appended leaves (using UNCOMMITTED_ORCHARD as the leaf value),
            // but BridgeTree doesn't encode these.
            for j in 0..=i {
                assert_eq!(
                    tree.witness(j.try_into().unwrap(), 0).ok(),
                    Some(
                        tv.paths[j]
                            .iter()
                            .map(|v| MerkleHashOrchard::from_bytes(v).unwrap())
                            .collect()
                    )
                );
            }
        }
    }

    #[test]
    fn empty_roots_incremental() {
        use incrementalmerkletree::Hashable;

        let tv_empty_roots = crate::test_vectors::commitment_tree::test_vectors().empty_roots;

        for (level, tv_root) in tv_empty_roots.iter().enumerate() {
            assert_eq!(
                MerkleHashOrchard::empty_root(Level::from(level as u8))
                    .0
                    .to_repr(),
                *tv_root,
                "Empty root mismatch at level {}",
                level
            );
        }
    }

    #[test]
    fn anchor_incremental() {
        // These commitment values are derived from the bundle data that was generated for
        // testing commitment tree construction inside of zcashd here.
        // https://github.com/zcash/zcash/blob/ecec1f9769a5e37eb3f7fd89a4fcfb35bc28eed7/src/test/data/merkle_roots_orchard.h
        let commitments = [
            [
                0x68, 0x13, 0x5c, 0xf4, 0x99, 0x33, 0x22, 0x90, 0x99, 0xa4, 0x4e, 0xc9, 0x9a, 0x75,
                0xe1, 0xe1, 0xcb, 0x46, 0x40, 0xf9, 0xb5, 0xbd, 0xec, 0x6b, 0x32, 0x23, 0x85, 0x6f,
                0xea, 0x16, 0x39, 0x0a,
            ],
            [
                0x78, 0x31, 0x50, 0x08, 0xfb, 0x29, 0x98, 0xb4, 0x30, 0xa5, 0x73, 0x1d, 0x67, 0x26,
                0x20, 0x7d, 0xc0, 0xf0, 0xec, 0x81, 0xea, 0x64, 0xaf, 0x5c, 0xf6, 0x12, 0x95, 0x69,
                0x01, 0xe7, 0x2f, 0x0e,
            ],
            [
                0xee, 0x94, 0x88, 0x05, 0x3a, 0x30, 0xc5, 0x96, 0xb4, 0x30, 0x14, 0x10, 0x5d, 0x34,
                0x77, 0xe6, 0xf5, 0x78, 0xc8, 0x92, 0x40, 0xd1, 0xd1, 0xee, 0x17, 0x43, 0xb7, 0x7b,
                0xb6, 0xad, 0xc4, 0x0a,
            ],
            [
                0x9d, 0xdc, 0xe7, 0xf0, 0x65, 0x01, 0xf3, 0x63, 0x76, 0x8c, 0x5b, 0xca, 0x3f, 0x26,
                0x46, 0x60, 0x83, 0x4d, 0x4d, 0xf4, 0x46, 0xd1, 0x3e, 0xfc, 0xd7, 0xc6, 0xf1, 0x7b,
                0x16, 0x7a, 0xac, 0x1a,
            ],
            [
                0xbd, 0x86, 0x16, 0x81, 0x1c, 0x6f, 0x5f, 0x76, 0x9e, 0xa4, 0x53, 0x9b, 0xba, 0xff,
                0x0f, 0x19, 0x8a, 0x6c, 0xdf, 0x3b, 0x28, 0x0d, 0xd4, 0x99, 0x26, 0x16, 0x3b, 0xd5,
                0x3f, 0x53, 0xa1, 0x21,
            ],
        ];

        // This value was produced by the Python test vector generation code implemented here:
        // https://github.com/zcash-hackworks/zcash-test-vectors/blob/f4d756410c8f2456f5d84cedf6dac6eb8c068eed/orchard_merkle_tree.py
        let anchor = [
            0xc8, 0x75, 0xbe, 0x2d, 0x60, 0x87, 0x3f, 0x8b, 0xcd, 0xeb, 0x91, 0x28, 0x2e, 0x64,
            0x2e, 0x0c, 0xc6, 0x5f, 0xf7, 0xd0, 0x64, 0x2d, 0x13, 0x7b, 0x28, 0xcf, 0x28, 0xcc,
            0x9c, 0x52, 0x7f, 0x0e,
        ];

        let mut frontier = BridgeFrontier::<MerkleHashOrchard, 32>::empty();
        for commitment in commitments.iter() {
            let cmx = MerkleHashOrchard(pallas::Base::from_repr(*commitment).unwrap());
            frontier.append(cmx);
        }
        assert_eq!(frontier.root().0, pallas::Base::from_repr(anchor).unwrap());
    }
}
