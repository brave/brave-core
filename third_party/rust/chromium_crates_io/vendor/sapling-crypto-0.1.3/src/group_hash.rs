//! Implementation of [group hashing into Jubjub][grouphash].
//!
//! [grouphash]: https://zips.z.cash/protocol/protocol.pdf#concretegrouphashjubjub

use ff::PrimeField;
use group::{cofactor::CofactorGroup, Group, GroupEncoding};

use super::constants;
use blake2s_simd::Params;

/// Produces a random point in the Jubjub curve.
/// The point is guaranteed to be prime order
/// and not the identity.
#[allow(clippy::assertions_on_constants)]
pub fn group_hash(tag: &[u8], personalization: &[u8]) -> Option<jubjub::SubgroupPoint> {
    assert_eq!(personalization.len(), 8);

    // Check to see that scalar field is 255 bits
    assert!(bls12_381::Scalar::NUM_BITS == 255);

    let h = Params::new()
        .hash_length(32)
        .personal(personalization)
        .to_state()
        .update(constants::GH_FIRST_BLOCK)
        .update(tag)
        .finalize();

    let p = jubjub::ExtendedPoint::from_bytes(h.as_array());
    if p.is_some().into() {
        // <ExtendedPoint as CofactorGroup>::clear_cofactor is implemented using
        // ExtendedPoint::mul_by_cofactor in the jubjub crate.
        let p = CofactorGroup::clear_cofactor(&p.unwrap());

        if p.is_identity().into() {
            None
        } else {
            Some(p)
        }
    } else {
        None
    }
}
