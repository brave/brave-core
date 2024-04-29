use core::convert::TryFrom;

use jubjub::{AffinePoint, Fq};

use reddsa::*;

#[test]
fn identity_publickey_passes() {
    let identity = AffinePoint::identity();
    assert!(<bool>::from(identity.is_small_order()));
    let bytes = identity.to_bytes();
    let pk_bytes = VerificationKeyBytes::<sapling::SpendAuth>::from(bytes);
    assert!(VerificationKey::<sapling::SpendAuth>::try_from(pk_bytes).is_ok());
}

#[test]
fn smallorder_publickey_passes() {
    // (1,0) is a point of order 4 on any Edwards curve
    let order4 = AffinePoint::from_raw_unchecked(Fq::one(), Fq::zero());
    assert!(<bool>::from(order4.is_small_order()));
    let bytes = order4.to_bytes();
    let pk_bytes = VerificationKeyBytes::<sapling::SpendAuth>::from(bytes);
    assert!(VerificationKey::<sapling::SpendAuth>::try_from(pk_bytes).is_ok());
}
