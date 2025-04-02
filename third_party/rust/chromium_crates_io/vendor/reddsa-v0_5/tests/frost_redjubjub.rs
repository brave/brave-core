#![cfg(feature = "frost")]

use frost_rerandomized::frost_core::{Ciphersuite, Group, GroupError};
use rand::thread_rng;

use reddsa::{frost::redjubjub::JubjubBlake2b512, sapling};

#[test]
fn check_sign_with_dealer() {
    let rng = thread_rng();

    frost_rerandomized::frost_core::tests::ciphersuite_generic::check_sign_with_dealer::<
        JubjubBlake2b512,
        _,
    >(rng);
}

#[test]
fn check_randomized_sign_with_dealer() {
    let rng = thread_rng();

    let (msg, group_signature, group_pubkey) =
        frost_rerandomized::tests::check_randomized_sign_with_dealer::<JubjubBlake2b512, _>(rng);

    // Check that the threshold signature can be verified by the `reddsa` crate
    // public key (interoperability test)

    let sig = {
        let bytes: [u8; 64] = group_signature.serialize().as_ref().try_into().unwrap();
        reddsa::Signature::<sapling::SpendAuth>::from(bytes)
    };
    let pk_bytes = {
        let bytes: [u8; 32] = group_pubkey.serialize().as_ref().try_into().unwrap();
        reddsa::VerificationKeyBytes::<sapling::SpendAuth>::from(bytes)
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
        JubjubBlake2b512,
        _,
    >(rng);
}

#[test]
fn check_deserialize_identity() {
    let encoded_identity = <JubjubBlake2b512 as Ciphersuite>::Group::serialize(
        &<JubjubBlake2b512 as Ciphersuite>::Group::identity(),
    );
    let r = <JubjubBlake2b512 as Ciphersuite>::Group::deserialize(&encoded_identity);
    assert_eq!(r, Err(GroupError::InvalidIdentityElement));
}

#[test]
fn check_deserialize_non_canonical() {
    let encoded_generator = <JubjubBlake2b512 as Ciphersuite>::Group::serialize(
        &<JubjubBlake2b512 as Ciphersuite>::Group::generator(),
    );
    let r = <JubjubBlake2b512 as Ciphersuite>::Group::deserialize(&encoded_generator);
    assert!(r.is_ok());

    // This is x = p + 3 which is non-canonical and maps to a valid point.
    let encoded_point =
        hex::decode("04000000fffffffffe5bfeff02a4bd5305d8a10908d83933487d9d2953a7ed73")
            .unwrap()
            .try_into()
            .unwrap();
    let r = <JubjubBlake2b512 as Ciphersuite>::Group::deserialize(&encoded_point);
    assert_eq!(r, Err(GroupError::MalformedElement));
}

#[test]
fn check_deserialize_non_prime_order() {
    let encoded_point =
        hex::decode("0300000000000000000000000000000000000000000000000000000000000000")
            .unwrap()
            .try_into()
            .unwrap();
    let r = <JubjubBlake2b512 as Ciphersuite>::Group::deserialize(&encoded_point);
    assert_eq!(r, Err(GroupError::InvalidNonPrimeOrderElement));
}
