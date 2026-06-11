//! # orchard
//!
//! ## Nomenclature
//!
//! All types in the `orchard` crate, unless otherwise specified, are Orchard-specific
//! types. For example, [`Address`] is documented as being a shielded payment address; we
//! implicitly mean it is an Orchard payment address (as opposed to e.g. a Sapling payment
//! address, which is also shielded).

#![no_std]
#![cfg_attr(docsrs, feature(doc_cfg))]
// Temporary until we have more of the crate implemented.
#![allow(dead_code)]
// Catch documentation errors caused by code changes.
#![deny(rustdoc::broken_intra_doc_links)]
#![deny(missing_debug_implementations)]
#![deny(missing_docs)]
#![forbid(unsafe_code)]

#[macro_use]
extern crate alloc;

#[cfg(feature = "std")]
extern crate std;

use alloc::vec::Vec;

mod action;
mod address;
pub mod builder;
pub mod bundle;
#[cfg(feature = "circuit")]
pub mod circuit;
#[cfg(not(feature = "unstable-voting-circuits"))]
mod constants;
#[cfg(feature = "unstable-voting-circuits")]
pub mod constants;
pub mod keys;
pub mod note;
pub mod note_encryption;
pub mod pczt;
pub mod primitives;
#[cfg(not(feature = "unstable-voting-circuits"))]
mod spec;
#[cfg(feature = "unstable-voting-circuits")]
pub mod spec;
pub mod tree;
pub mod value;
pub mod zip32;

#[cfg(test)]
mod test_vectors;

pub use action::{Action, ActionFromPartsError};
pub use address::Address;
pub use bundle::Bundle;
pub use constants::MERKLE_DEPTH_ORCHARD as NOTE_COMMITMENT_TREE_DEPTH;
pub use constants::{L_ORCHARD_BASE, L_ORCHARD_SCALAR, L_VALUE};
pub use note::Note;
pub use tree::Anchor;

/// A proof of the validity of an Orchard [`Bundle`].
///
/// [`Bundle`]: crate::bundle::Bundle
#[derive(Clone)]
pub struct Proof(Vec<u8>);

impl core::fmt::Debug for Proof {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        if f.alternate() {
            f.debug_tuple("Proof").field(&self.0).finish()
        } else {
            // By default, only show the proof length, not its contents.
            f.debug_tuple("Proof")
                .field(&format_args!("{} bytes", self.0.len()))
                .finish()
        }
    }
}

impl AsRef<[u8]> for Proof {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

impl memuse::DynamicUsage for Proof {
    fn dynamic_usage(&self) -> usize {
        self.0.dynamic_usage()
    }

    fn dynamic_usage_bounds(&self) -> (usize, Option<usize>) {
        self.0.dynamic_usage_bounds()
    }
}

impl Proof {
    /// Constructs a new Proof value.
    pub fn new(bytes: Vec<u8>) -> Self {
        Proof(bytes)
    }

    /// The canonical byte length of a proof authorizing a bundle of `num_actions` actions.
    ///
    /// A valid Orchard proof always has exactly this length. The constants are fixed by the
    /// halo2 action circuit; they are cross-checked against [`halo2_proofs::dev::CircuitCost`]
    /// in the circuit tests. Use this to reject non-canonical (e.g. padded) proofs when
    /// constructing a bundle from untrusted bytes; see [`Bundle::try_from_parts`].
    ///
    /// [`Bundle::try_from_parts`]: crate::Bundle::try_from_parts
    pub const fn expected_proof_size(num_actions: usize) -> usize {
        // The proof is a fixed base size plus a fixed contribution per action. These constants
        // are determined by the halo2 action circuit; see the `circuit` module's `round_trip`
        // test, which cross-checks them against `CircuitCost::proof_size`.
        const BASE: usize = 2720;
        const PER_ACTION: usize = 2272;
        BASE + PER_ACTION * num_actions
    }
}
