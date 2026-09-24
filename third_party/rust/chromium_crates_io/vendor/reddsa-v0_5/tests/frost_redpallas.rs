#![cfg(feature = "frost")]

use frost_rerandomized::frost_core::{Ciphersuite, Group, GroupError};
use rand::thread_rng;

use reddsa::{frost::redpallas::PallasBlake2b512, orchard};

#[test]
fn check_sign_with_dealer() {
    let rng = thread_rng();

    frost_rerandomized::frost_core::tests::ciphersuite_generic::check_sign_with_dealer::<
        PallasBlake2b512,
        _,
    >(rng);
}

#[test]
fn check_randomized_sign_with_dealer() {
    let rng = thread_rng();

    let (msg, group_signature, group_pubkey) =
        frost_rerandomized::tests::check_randomized_sign_with_dealer::<PallasBlake2b512, _>(rng);

    // Check that the threshold signature can be verified by the `reddsa` crate
    // public key (interoperability test)

    let sig = {
        let bytes: [u8; 64] = group_signature.serialize().as_ref().try_into().unwrap();
        reddsa::Signature::<orchard::SpendAuth>::from(bytes)
    };
    let pk_bytes = {
        let bytes: [u8; 32] = group_pubkey.serialize().as_ref().try_into().unwrap();
        reddsa::VerificationKeyBytes::<orchard::SpendAuth>::from(bytes)
    };

    // Check that the verification key is a valid RedDSA verification key.
    let pub_key = reddsa::VerificationKey::try_from(pk_bytes)
        .expect("The test verification key to be well-formed.");

    // Check that signature validation has the expected result.
    assert!(pub_key.verify(&msg, &sig).is_ok());
}

#[test]
fn check_sign_with_dkg() {
    let rng = thread_rng();

    frost_rerandomized::frost_core::tests::ciphersuite_generic::check_sign_with_dkg::<
        PallasBlake2b512,
        _,
    >(rng);
}

#[test]
fn check_deserialize_identity() {
    let encoded_identity = <PallasBlake2b512 as Ciphersuite>::Group::serialize(
        &<PallasBlake2b512 as Ciphersuite>::Group::identity(),
    );
    let r = <PallasBlake2b512 as Ciphersuite>::Group::deserialize(&encoded_identity);
    assert_eq!(r, Err(GroupError::InvalidIdentityElement));
}

#[test]
fn check_deserialize_non_canonical() {
    let encoded_generator = <PallasBlake2b512 as Ciphersuite>::Group::serialize(
        &<PallasBlake2b512 as Ciphersuite>::Group::generator(),
    );
    let r = <PallasBlake2b512 as Ciphersuite>::Group::deserialize(&encoded_generator);
    assert!(r.is_ok());

    // This is x = p + 3 which is non-canonical and maps to a valid point.
    let encoded_point =
        hex::decode("04000000ed302d991bf94c09fc98462200000000000000000000000000000040")
            .unwrap()
            .try_into()
            .unwrap();
    let r = <PallasBlake2b512 as Ciphersuite>::Group::deserialize(&encoded_point);
    assert_eq!(r, Err(GroupError::MalformedElement));
}
