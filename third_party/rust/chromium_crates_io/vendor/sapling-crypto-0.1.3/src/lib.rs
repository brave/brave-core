//! # sapling
//!
//! ## Nomenclature
//!
//! All types in the `sapling-crypto` crate, unless otherwise specified, are
//! Sapling-specific types. For example, [`PaymentAddress`] is documented as being a
//! shielded payment address; we implicitly mean it is an Sapling payment address (as
//! opposed to e.g. an Orchard payment address, which is also shielded).
//!
//! ## Feature flags
#![doc = document_features::document_features!()]
//!

#![cfg_attr(docsrs, feature(doc_cfg))]
// Catch documentation errors caused by code changes.
#![deny(rustdoc::broken_intra_doc_links)]
#![deny(unsafe_code)]

mod address;
pub mod builder;
pub mod bundle;
pub mod circuit;
pub mod constants;
pub mod group_hash;
pub mod keys;
pub mod note;
pub mod note_encryption;
pub mod pedersen_hash;
pub mod prover;
mod spec;
mod tree;
pub mod util;
pub mod value;
mod verifier;
pub mod zip32;

pub use address::PaymentAddress;
pub use bundle::Bundle;
pub use keys::{Diversifier, NullifierDerivingKey, ProofGenerationKey, SaplingIvk, ViewingKey};
pub use note::{nullifier::Nullifier, Note, Rseed};
pub use tree::{
    merkle_hash, Anchor, CommitmentTree, IncrementalWitness, MerklePath, Node,
    NOTE_COMMITMENT_TREE_DEPTH,
};
pub use verifier::{BatchValidator, SaplingVerificationContext};

#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub mod testing {
    pub use super::{
        address::testing::arb_payment_address, keys::testing::arb_incoming_viewing_key,
        note::testing::arb_note, tree::testing::arb_node,
    };
}

#[cfg(test)]
mod test_vectors;
