use bitvec::{order::Lsb0, view::AsBits};
use group::{ff::PrimeField, Curve};
use incrementalmerkletree::{Hashable, Level};
use lazy_static::lazy_static;
use subtle::CtOption;

use std::fmt;

use super::{
    note::ExtractedNoteCommitment,
    pedersen_hash::{pedersen_hash, Personalization},
};

pub const NOTE_COMMITMENT_TREE_DEPTH: u8 = 32;
pub type CommitmentTree =
    incrementalmerkletree::frontier::CommitmentTree<Node, NOTE_COMMITMENT_TREE_DEPTH>;
pub type IncrementalWitness =
    incrementalmerkletree::witness::IncrementalWitness<Node, NOTE_COMMITMENT_TREE_DEPTH>;
pub type MerklePath = incrementalmerkletree::MerklePath<Node, NOTE_COMMITMENT_TREE_DEPTH>;

lazy_static! {
    static ref UNCOMMITTED_SAPLING: bls12_381::Scalar = bls12_381::Scalar::one();
    static ref EMPTY_ROOTS: Vec<Node> = {
        let mut v = vec![Node::empty_leaf()];
        for d in 0..NOTE_COMMITMENT_TREE_DEPTH {
            let next = Node::combine(d.into(), &v[usize::from(d)], &v[usize::from(d)]);
            v.push(next);
        }
        v
    };
}

/// Compute a parent node in the Sapling commitment tree given its two children.
pub fn merkle_hash(depth: usize, lhs: &[u8; 32], rhs: &[u8; 32]) -> [u8; 32] {
    merkle_hash_field(depth, lhs, rhs).to_repr()
}

fn merkle_hash_field(depth: usize, lhs: &[u8; 32], rhs: &[u8; 32]) -> jubjub::Base {
    let lhs = {
        let mut tmp = [false; 256];
        for (a, b) in tmp.iter_mut().zip(lhs.as_bits::<Lsb0>()) {
            *a = *b;
        }
        tmp
    };

    let rhs = {
        let mut tmp = [false; 256];
        for (a, b) in tmp.iter_mut().zip(rhs.as_bits::<Lsb0>()) {
            *a = *b;
        }
        tmp
    };

    jubjub::ExtendedPoint::from(pedersen_hash(
        Personalization::MerkleTree(depth),
        lhs.iter()
            .copied()
            .take(bls12_381::Scalar::NUM_BITS as usize)
            .chain(
                rhs.iter()
                    .copied()
                    .take(bls12_381::Scalar::NUM_BITS as usize),
            ),
    ))
    .to_affine()
    .get_u()
}

/// The root of a Sapling commitment tree.
#[derive(Eq, PartialEq, Clone, Copy, Debug)]
pub struct Anchor(jubjub::Base);

impl From<jubjub::Base> for Anchor {
    fn from(anchor_field: jubjub::Base) -> Anchor {
        Anchor(anchor_field)
    }
}

impl From<Node> for Anchor {
    fn from(anchor: Node) -> Anchor {
        Anchor(anchor.0)
    }
}

impl Anchor {
    /// The anchor of the empty Sapling note commitment tree.
    ///
    /// This anchor does not correspond to any valid anchor for a spend, so it
    /// may only be used for coinbase bundles or in circumstances where Sapling
    /// functionality is not active.
    pub fn empty_tree() -> Anchor {
        Anchor(Node::empty_root(NOTE_COMMITMENT_TREE_DEPTH.into()).0)
    }

    /// Parses a Sapling anchor from a byte encoding.
    pub fn from_bytes(bytes: [u8; 32]) -> CtOption<Anchor> {
        jubjub::Base::from_repr(bytes).map(Self)
    }

    /// Returns the byte encoding of this anchor.
    pub fn to_bytes(self) -> [u8; 32] {
        self.0.to_repr()
    }
}

/// A node within the Sapling commitment tree.
#[derive(Clone, Copy, PartialEq, Eq)]
pub struct Node(jubjub::Base);

impl fmt::Debug for Node {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Node")
            .field("repr", &hex::encode(self.0.to_bytes()))
            .finish()
    }
}

impl Node {
    /// Creates a tree leaf from the given Sapling note commitment.
    pub fn from_cmu(value: &ExtractedNoteCommitment) -> Self {
        Node(value.inner())
    }

    /// Constructs a new note commitment tree node from a [`bls12_381::Scalar`]
    pub fn from_scalar(cmu: bls12_381::Scalar) -> Self {
        Self(cmu)
    }

    /// Parses a tree leaf from the bytes of a Sapling note commitment.
    ///
    /// Returns `None` if the provided bytes represent a non-canonical encoding.
    pub fn from_bytes(bytes: [u8; 32]) -> CtOption<Self> {
        jubjub::Base::from_repr(bytes).map(Self)
    }

    /// Returns the canonical byte representation of this node.
    pub fn to_bytes(&self) -> [u8; 32] {
        self.0.to_repr()
    }

    /// Returns the wrapped value
    pub(crate) fn inner(&self) -> &jubjub::Base {
        &self.0
    }
}

impl Hashable for Node {
    fn empty_leaf() -> Self {
        Node(*UNCOMMITTED_SAPLING)
    }

    fn combine(level: Level, lhs: &Self, rhs: &Self) -> Self {
        Node(merkle_hash_field(
            level.into(),
            &lhs.0.to_bytes(),
            &rhs.0.to_bytes(),
        ))
    }

    fn empty_root(level: Level) -> Self {
        EMPTY_ROOTS[<usize>::from(level)]
    }
}

impl From<Node> for bls12_381::Scalar {
    fn from(node: Node) -> Self {
        node.0
    }
}

#[cfg(any(test, feature = "test-dependencies"))]
pub(super) mod testing {
    use ff::Field;
    use proptest::prelude::*;
    use rand::distributions::{Distribution, Standard};

    use super::Node;
    use crate::note::testing::arb_cmu;

    prop_compose! {
        pub fn arb_node()(cmu in arb_cmu()) -> Node {
            Node::from_cmu(&cmu)
        }
    }

    impl Node {
        /// Return a random fake `MerkleHashOrchard`.
        pub fn random(rng: &mut impl RngCore) -> Self {
            Standard.sample(rng)
        }
    }

    impl Distribution<Node> for Standard {
        fn sample<R: Rng + ?Sized>(&self, rng: &mut R) -> Node {
            Node::from_scalar(bls12_381::Scalar::random(rng))
        }
    }
}
