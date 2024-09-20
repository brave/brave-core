// -*- mode: rust; -*-
//
// This file is part of ed25519-dalek.
// Copyright (c) 2017-2019 isis lovecruft
// See LICENSE for licensing information.
//
// Authors:
// - isis agora lovecruft <isis@patternsinthevoid.net>

//! Integration tests for ed25519-dalek.

#![allow(clippy::items_after_test_module)]

use ed25519_dalek::*;

use hex::FromHex;
#[cfg(feature = "digest")]
use hex_literal::hex;

#[cfg(test)]
mod vectors {
    use super::*;

    use curve25519_dalek::{
        constants::ED25519_BASEPOINT_POINT,
        edwards::{CompressedEdwardsY, EdwardsPoint},
        scalar::Scalar,
        traits::IsIdentity,
    };
    use sha2::{digest::Digest, Sha512};

    use std::{
        convert::TryFrom,
        fs::File,
        io::{BufRead, BufReader},
        ops::Neg,
    };

    // TESTVECTORS is taken from sign.input.gz in agl's ed25519 Golang
    // package. It is a selection of test cases from
    // http://ed25519.cr.yp.to/python/sign.input
    #[test]
    fn against_reference_implementation() {
        // TestGolden
        let mut line: String;
        let mut lineno: usize = 0;

        let f = File::open("TESTVECTORS");
        if f.is_err() {
            println!(
                "This test is only available when the code has been cloned \
                 from the git repository, since the TESTVECTORS file is large \
                 and is therefore not included within the distributed crate."
            );
            panic!();
        }
        let file = BufReader::new(f.unwrap());

        for l in file.lines() {
            lineno += 1;
            line = l.unwrap();

            let parts: Vec<&str> = line.split(':').collect();
            assert_eq!(parts.len(), 5, "wrong number of fields in line {}", lineno);

            let sec_bytes: Vec<u8> = FromHex::from_hex(parts[0]).unwrap();
            let pub_bytes: Vec<u8> = FromHex::from_hex(parts[1]).unwrap();
            let msg_bytes: Vec<u8> = FromHex::from_hex(parts[2]).unwrap();
            let sig_bytes: Vec<u8> = FromHex::from_hex(parts[3]).unwrap();

            let sec_bytes = &sec_bytes[..SECRET_KEY_LENGTH].try_into().unwrap();
            let pub_bytes = &pub_bytes[..PUBLIC_KEY_LENGTH].try_into().unwrap();

            let signing_key = SigningKey::from_bytes(sec_bytes);
            let expected_verifying_key = VerifyingKey::from_bytes(pub_bytes).unwrap();
            assert_eq!(expected_verifying_key, signing_key.verifying_key());

            // The signatures in the test vectors also include the message
            // at the end, but we just want R and S.
            let sig1: Signature = Signature::try_from(&sig_bytes[..64]).unwrap();
            let sig2: Signature = signing_key.sign(&msg_bytes);

            assert!(sig1 == sig2, "Signature bytes not equal on line {}", lineno);
            assert!(
                signing_key.verify(&msg_bytes, &sig2).is_ok(),
                "Signature verification failed on line {}",
                lineno
            );
            assert!(
                expected_verifying_key
                    .verify_strict(&msg_bytes, &sig2)
                    .is_ok(),
                "Signature strict verification failed on line {}",
                lineno
            );
        }
    }

    // From https://tools.ietf.org/html/rfc8032#section-7.3
    #[cfg(feature = "digest")]
    #[test]
    fn ed25519ph_rf8032_test_vector_prehash() {
        let sec_bytes = hex!("833fe62409237b9d62ec77587520911e9a759cec1d19755b7da901b96dca3d42");
        let pub_bytes = hex!("ec172b93ad5e563bf4932c70e1245034c35467ef2efd4d64ebf819683467e2bf");
        let msg_bytes = hex!("616263");
        let sig_bytes = hex!("98a70222f0b8121aa9d30f813d683f809e462b469c7ff87639499bb94e6dae4131f85042463c2a355a2003d062adf5aaa10b8c61e636062aaad11c2a26083406");

        let signing_key = SigningKey::from_bytes(&sec_bytes);
        let expected_verifying_key = VerifyingKey::from_bytes(&pub_bytes).unwrap();
        assert_eq!(expected_verifying_key, signing_key.verifying_key());
        let sig1 = Signature::try_from(&sig_bytes[..]).unwrap();

        let mut prehash_for_signing = Sha512::default();
        let mut prehash_for_verifying = Sha512::default();

        prehash_for_signing.update(&msg_bytes[..]);
        prehash_for_verifying.update(&msg_bytes[..]);

        let sig2: Signature = signing_key
            .sign_prehashed(prehash_for_signing, None)
            .unwrap();

        assert!(
            sig1 == sig2,
            "Original signature from test vectors doesn't equal signature produced:\
             \noriginal:\n{:?}\nproduced:\n{:?}",
            sig1,
            sig2
        );
        assert!(
            signing_key
                .verify_prehashed(prehash_for_verifying.clone(), None, &sig2)
                .is_ok(),
            "Could not verify ed25519ph signature!"
        );
        assert!(
            expected_verifying_key
                .verify_prehashed_strict(prehash_for_verifying, None, &sig2)
                .is_ok(),
            "Could not strict-verify ed25519ph signature!"
        );
    }

    //
    // The remaining items in this mod are for the repudiation tests
    //

    // Taken from curve25519_dalek::constants::EIGHT_TORSION[4]
    const EIGHT_TORSION_4: [u8; 32] = [
        236, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 127,
    ];

    // Computes the prehashed or non-prehashed challenge, depending on whether context is given
    fn compute_challenge(
        message: &[u8],
        pub_key: &EdwardsPoint,
        signature_r: &EdwardsPoint,
        context: Option<&[u8]>,
    ) -> Scalar {
        let mut h = Sha512::default();
        if let Some(c) = context {
            h.update(b"SigEd25519 no Ed25519 collisions");
            h.update([1]);
            h.update([c.len() as u8]);
            h.update(c);
        }
        h.update(signature_r.compress().as_bytes());
        h.update(&pub_key.compress().as_bytes()[..]);
        h.update(message);
        Scalar::from_hash(h)
    }

    fn serialize_signature(r: &EdwardsPoint, s: &Scalar) -> Vec<u8> {
        [&r.compress().as_bytes()[..], &s.as_bytes()[..]].concat()
    }

    const WEAK_PUBKEY: CompressedEdwardsY = CompressedEdwardsY(EIGHT_TORSION_4);

    // Pick a random Scalar
    fn non_null_scalar() -> Scalar {
        let mut rng = rand::rngs::OsRng;
        let mut s_candidate = Scalar::random(&mut rng);
        while s_candidate == Scalar::ZERO {
            s_candidate = Scalar::random(&mut rng);
        }
        s_candidate
    }

    fn pick_r(s: Scalar) -> EdwardsPoint {
        let r0 = s * ED25519_BASEPOINT_POINT;
        // Pick a torsion point of order 2
        r0 + WEAK_PUBKEY.decompress().unwrap().neg()
    }

    // Tests that verify_strict() rejects small-order pubkeys. We test this by explicitly
    // constructing a pubkey-signature pair that verifies with respect to two distinct messages.
    // This should be accepted by verify(), but rejected by verify_strict().
    #[test]
    fn repudiation() {
        let message1 = b"Send 100 USD to Alice";
        let message2 = b"Send 100000 USD to Alice";

        let mut s: Scalar = non_null_scalar();
        let pubkey = WEAK_PUBKEY.decompress().unwrap();
        let mut r = pick_r(s);

        // Find an R such that
        //     H(R || A || M₁) · A == A == H(R || A || M₂) · A
        // This happens with high probability when A is low order.
        while !(pubkey.neg() + compute_challenge(message1, &pubkey, &r, None) * pubkey)
            .is_identity()
            || !(pubkey.neg() + compute_challenge(message2, &pubkey, &r, None) * pubkey)
                .is_identity()
        {
            // We pick an s and let R = sB - A where B is the basepoint
            s = non_null_scalar();
            r = pick_r(s);
        }

        // At this point, both verification equations hold:
        //     sB = R + H(R || A || M₁) · A
        //        = R + H(R || A || M₂) · A
        // Check that this is true
        let signature = serialize_signature(&r, &s);
        let vk = VerifyingKey::from_bytes(pubkey.compress().as_bytes()).unwrap();
        let sig = Signature::try_from(&signature[..]).unwrap();
        assert!(vk.verify(message1, &sig).is_ok());
        assert!(vk.verify(message2, &sig).is_ok());

        // Check that this public key appears as weak
        assert!(vk.is_weak());

        // Now check that the sigs fail under verify_strict. This is because verify_strict rejects
        // small order pubkeys.
        assert!(vk.verify_strict(message1, &sig).is_err());
        assert!(vk.verify_strict(message2, &sig).is_err());
    }

    // Identical to repudiation() above, but testing verify_prehashed against
    // verify_prehashed_strict. See comments above for a description of what's happening.
    #[cfg(feature = "digest")]
    #[test]
    fn repudiation_prehash() {
        let message1 = Sha512::new().chain_update(b"Send 100 USD to Alice");
        let message2 = Sha512::new().chain_update(b"Send 100000 USD to Alice");
        let message1_bytes = message1.clone().finalize();
        let message2_bytes = message2.clone().finalize();

        let mut s: Scalar = non_null_scalar();
        let pubkey = WEAK_PUBKEY.decompress().unwrap();
        let mut r = pick_r(s);
        let context_str = Some(&b"edtest"[..]);

        while !(pubkey.neg()
            + compute_challenge(&message1_bytes, &pubkey, &r, context_str) * pubkey)
            .is_identity()
            || !(pubkey.neg()
                + compute_challenge(&message2_bytes, &pubkey, &r, context_str) * pubkey)
                .is_identity()
        {
            s = non_null_scalar();
            r = pick_r(s);
        }

        // Check that verify_prehashed succeeds on both sigs
        let signature = serialize_signature(&r, &s);
        let vk = VerifyingKey::from_bytes(pubkey.compress().as_bytes()).unwrap();
        let sig = Signature::try_from(&signature[..]).unwrap();
        assert!(vk
            .verify_prehashed(message1.clone(), context_str, &sig)
            .is_ok());
        assert!(vk
            .verify_prehashed(message2.clone(), context_str, &sig)
            .is_ok());

        // Check that verify_prehashed_strict fails on both sigs
        assert!(vk
            .verify_prehashed_strict(message1.clone(), context_str, &sig)
            .is_err());
        assert!(vk
            .verify_prehashed_strict(message2.clone(), context_str, &sig)
            .is_err());
    }
}

#[cfg(feature = "rand_core")]
mod integrations {
    use super::*;
    use rand::rngs::OsRng;
    #[cfg(feature = "digest")]
    use sha2::Sha512;
    use std::collections::HashMap;

    #[test]
    fn sign_verify() {
        // TestSignVerify

        let good: &[u8] = "test message".as_bytes();
        let bad: &[u8] = "wrong message".as_bytes();

        let mut csprng = OsRng;

        let signing_key: SigningKey = SigningKey::generate(&mut csprng);
        let verifying_key = signing_key.verifying_key();
        let good_sig: Signature = signing_key.sign(good);
        let bad_sig: Signature = signing_key.sign(bad);

        // Check that an honestly generated public key is not weak
        assert!(!verifying_key.is_weak());

        assert!(
            signing_key.verify(good, &good_sig).is_ok(),
            "Verification of a valid signature failed!"
        );
        assert!(
            verifying_key.verify_strict(good, &good_sig).is_ok(),
            "Strict verification of a valid signature failed!"
        );
        assert!(
            signing_key.verify(good, &bad_sig).is_err(),
            "Verification of a signature on a different message passed!"
        );
        assert!(
            verifying_key.verify_strict(good, &bad_sig).is_err(),
            "Strict verification of a signature on a different message passed!"
        );
        assert!(
            signing_key.verify(bad, &good_sig).is_err(),
            "Verification of a signature on a different message passed!"
        );
        assert!(
            verifying_key.verify_strict(bad, &good_sig).is_err(),
            "Strict verification of a signature on a different message passed!"
        );
    }

    #[cfg(feature = "digest")]
    #[test]
    fn ed25519ph_sign_verify() {
        let good: &[u8] = b"test message";
        let bad: &[u8] = b"wrong message";

        let mut csprng = OsRng;

        // ugh… there's no `impl Copy for Sha512`… i hope we can all agree these are the same hashes
        let mut prehashed_good1: Sha512 = Sha512::default();
        prehashed_good1.update(good);
        let mut prehashed_good2: Sha512 = Sha512::default();
        prehashed_good2.update(good);
        let mut prehashed_good3: Sha512 = Sha512::default();
        prehashed_good3.update(good);

        let mut prehashed_bad1: Sha512 = Sha512::default();
        prehashed_bad1.update(bad);
        let mut prehashed_bad2: Sha512 = Sha512::default();
        prehashed_bad2.update(bad);

        let context: &[u8] = b"testing testing 1 2 3";

        let signing_key: SigningKey = SigningKey::generate(&mut csprng);
        let verifying_key = signing_key.verifying_key();
        let good_sig: Signature = signing_key
            .sign_prehashed(prehashed_good1, Some(context))
            .unwrap();
        let bad_sig: Signature = signing_key
            .sign_prehashed(prehashed_bad1, Some(context))
            .unwrap();

        assert!(
            signing_key
                .verify_prehashed(prehashed_good2.clone(), Some(context), &good_sig)
                .is_ok(),
            "Verification of a valid signature failed!"
        );
        assert!(
            verifying_key
                .verify_prehashed_strict(prehashed_good2, Some(context), &good_sig)
                .is_ok(),
            "Strict verification of a valid signature failed!"
        );
        assert!(
            signing_key
                .verify_prehashed(prehashed_good3.clone(), Some(context), &bad_sig)
                .is_err(),
            "Verification of a signature on a different message passed!"
        );
        assert!(
            verifying_key
                .verify_prehashed_strict(prehashed_good3, Some(context), &bad_sig)
                .is_err(),
            "Strict verification of a signature on a different message passed!"
        );
        assert!(
            signing_key
                .verify_prehashed(prehashed_bad2.clone(), Some(context), &good_sig)
                .is_err(),
            "Verification of a signature on a different message passed!"
        );
        assert!(
            verifying_key
                .verify_prehashed_strict(prehashed_bad2, Some(context), &good_sig)
                .is_err(),
            "Strict verification of a signature on a different message passed!"
        );
    }

    #[cfg(feature = "batch")]
    #[test]
    fn verify_batch_seven_signatures() {
        let messages: [&[u8]; 7] = [
            b"Watch closely everyone, I'm going to show you how to kill a god.",
            b"I'm not a cryptographer I just encrypt a lot.",
            b"Still not a cryptographer.",
            b"This is a test of the tsunami alert system. This is only a test.",
            b"Fuck dumbin' it down, spit ice, skip jewellery: Molotov cocktails on me like accessories.",
            b"Hey, I never cared about your bucks, so if I run up with a mask on, probably got a gas can too.",
            b"And I'm not here to fill 'er up. Nope, we came to riot, here to incite, we don't want any of your stuff.", ];
        let mut csprng = OsRng;
        let mut signing_keys: Vec<SigningKey> = Vec::new();
        let mut signatures: Vec<Signature> = Vec::new();

        for msg in messages {
            let signing_key: SigningKey = SigningKey::generate(&mut csprng);
            signatures.push(signing_key.sign(msg));
            signing_keys.push(signing_key);
        }
        let verifying_keys: Vec<VerifyingKey> =
            signing_keys.iter().map(|key| key.verifying_key()).collect();

        let result = verify_batch(&messages, &signatures, &verifying_keys);

        assert!(result.is_ok());
    }

    #[test]
    fn public_key_hash_trait_check() {
        let mut csprng = OsRng {};
        let secret: SigningKey = SigningKey::generate(&mut csprng);
        let public_from_secret: VerifyingKey = (&secret).into();

        let mut m = HashMap::new();
        m.insert(public_from_secret, "Example_Public_Key");

        m.insert(public_from_secret, "Updated Value");

        let (k, &v) = m.get_key_value(&public_from_secret).unwrap();
        assert_eq!(k, &public_from_secret);
        assert_eq!(v, "Updated Value");
        assert_eq!(m.len(), 1usize);

        let second_secret: SigningKey = SigningKey::generate(&mut csprng);
        let public_from_second_secret: VerifyingKey = (&second_secret).into();
        assert_ne!(public_from_secret, public_from_second_secret);
        m.insert(public_from_second_secret, "Second public key");

        let (k, &v) = m.get_key_value(&public_from_second_secret).unwrap();
        assert_eq!(k, &public_from_second_secret);
        assert_eq!(v, "Second public key");
        assert_eq!(m.len(), 2usize);
    }
}

#[cfg(all(test, feature = "serde"))]
#[derive(Debug, serde::Serialize, serde::Deserialize)]
#[serde(crate = "serde")]
struct Demo {
    signing_key: SigningKey,
}

#[cfg(all(test, feature = "serde"))]
mod serialisation {
    #![allow(clippy::zero_prefixed_literal)]

    use super::*;

    // The size for bincode to serialize the length of a byte array.
    static BINCODE_INT_LENGTH: usize = 8;

    static PUBLIC_KEY_BYTES: [u8; PUBLIC_KEY_LENGTH] = [
        130, 039, 155, 015, 062, 076, 188, 063, 124, 122, 026, 251, 233, 253, 225, 220, 014, 041,
        166, 120, 108, 035, 254, 077, 160, 083, 172, 058, 219, 042, 086, 120,
    ];

    static SECRET_KEY_BYTES: [u8; SECRET_KEY_LENGTH] = [
        062, 070, 027, 163, 092, 182, 011, 003, 077, 234, 098, 004, 011, 127, 079, 228, 243, 187,
        150, 073, 201, 137, 076, 022, 085, 251, 152, 002, 241, 042, 072, 054,
    ];

    /// Signature with the above signing_key of a blank message.
    static SIGNATURE_BYTES: [u8; SIGNATURE_LENGTH] = [
        010, 126, 151, 143, 157, 064, 047, 001, 196, 140, 179, 058, 226, 152, 018, 102, 160, 123,
        080, 016, 210, 086, 196, 028, 053, 231, 012, 157, 169, 019, 158, 063, 045, 154, 238, 007,
        053, 185, 227, 229, 079, 108, 213, 080, 124, 252, 084, 167, 216, 085, 134, 144, 129, 149,
        041, 081, 063, 120, 126, 100, 092, 059, 050, 011,
    ];

    #[test]
    fn serialize_deserialize_signature_bincode() {
        let signature: Signature = Signature::from_bytes(&SIGNATURE_BYTES);
        let encoded_signature: Vec<u8> = bincode::serialize(&signature).unwrap();
        let decoded_signature: Signature = bincode::deserialize(&encoded_signature).unwrap();

        assert_eq!(signature, decoded_signature);
    }

    #[test]
    fn serialize_deserialize_signature_json() {
        let signature: Signature = Signature::from_bytes(&SIGNATURE_BYTES);
        let encoded_signature = serde_json::to_string(&signature).unwrap();
        let decoded_signature: Signature = serde_json::from_str(&encoded_signature).unwrap();

        assert_eq!(signature, decoded_signature);
    }

    #[test]
    fn serialize_deserialize_verifying_key_bincode() {
        let verifying_key: VerifyingKey = VerifyingKey::from_bytes(&PUBLIC_KEY_BYTES).unwrap();
        let encoded_verifying_key: Vec<u8> = bincode::serialize(&verifying_key).unwrap();
        let decoded_verifying_key: VerifyingKey =
            bincode::deserialize(&encoded_verifying_key).unwrap();

        assert_eq!(
            &PUBLIC_KEY_BYTES[..],
            &encoded_verifying_key[encoded_verifying_key.len() - PUBLIC_KEY_LENGTH..]
        );
        assert_eq!(verifying_key, decoded_verifying_key);
    }

    #[test]
    fn serialize_deserialize_verifying_key_json() {
        let verifying_key: VerifyingKey = VerifyingKey::from_bytes(&PUBLIC_KEY_BYTES).unwrap();
        let encoded_verifying_key = serde_json::to_string(&verifying_key).unwrap();
        let decoded_verifying_key: VerifyingKey =
            serde_json::from_str(&encoded_verifying_key).unwrap();

        assert_eq!(verifying_key, decoded_verifying_key);
    }

    #[test]
    fn serialize_deserialize_verifying_key_json_too_long() {
        // derived from `serialize_deserialize_verifying_key_json` test
        // trailing zero elements makes key too long (34 bytes)
        let encoded_verifying_key_too_long = "[130,39,155,15,62,76,188,63,124,122,26,251,233,253,225,220,14,41,166,120,108,35,254,77,160,83,172,58,219,42,86,120,0,0]";
        let de_err = serde_json::from_str::<VerifyingKey>(encoded_verifying_key_too_long)
            .unwrap_err()
            .to_string();
        assert!(
            de_err.contains("invalid length 34"),
            "expected invalid length error, got: {de_err}",
        );
    }

    #[test]
    fn serialize_deserialize_verifying_key_json_too_short() {
        // derived from `serialize_deserialize_verifying_key_json` test
        let encoded_verifying_key_too_long = "[130,39,155,15]";
        let de_err = serde_json::from_str::<VerifyingKey>(encoded_verifying_key_too_long)
            .unwrap_err()
            .to_string();
        assert!(
            de_err.contains("invalid length 4"),
            "expected invalid length error, got: {de_err}"
        );
    }

    #[test]
    fn serialize_deserialize_signing_key_bincode() {
        let signing_key = SigningKey::from_bytes(&SECRET_KEY_BYTES);
        let encoded_signing_key: Vec<u8> = bincode::serialize(&signing_key).unwrap();
        let decoded_signing_key: SigningKey = bincode::deserialize(&encoded_signing_key).unwrap();

        #[allow(clippy::needless_range_loop)]
        for i in 0..SECRET_KEY_LENGTH {
            assert_eq!(SECRET_KEY_BYTES[i], decoded_signing_key.to_bytes()[i]);
        }
    }

    #[test]
    fn serialize_deserialize_signing_key_json() {
        let signing_key = SigningKey::from_bytes(&SECRET_KEY_BYTES);
        let encoded_signing_key = serde_json::to_string(&signing_key).unwrap();
        let decoded_signing_key: SigningKey = serde_json::from_str(&encoded_signing_key).unwrap();

        #[allow(clippy::needless_range_loop)]
        for i in 0..SECRET_KEY_LENGTH {
            assert_eq!(SECRET_KEY_BYTES[i], decoded_signing_key.to_bytes()[i]);
        }
    }

    #[test]
    fn serialize_deserialize_signing_key_json_too_long() {
        // derived from `serialize_deserialize_signing_key_json` test
        // trailing zero elements makes key too long (34 bytes)
        let encoded_signing_key_too_long = "[62,70,27,163,92,182,11,3,77,234,98,4,11,127,79,228,243,187,150,73,201,137,76,22,85,251,152,2,241,42,72,54,0,0]";
        let de_err = serde_json::from_str::<SigningKey>(encoded_signing_key_too_long)
            .unwrap_err()
            .to_string();
        assert!(
            de_err.contains("invalid length 34"),
            "expected invalid length error, got: {de_err}",
        );
    }

    #[test]
    fn serialize_deserialize_signing_key_json_too_short() {
        // derived from `serialize_deserialize_signing_key_json` test
        let encoded_signing_key_too_long = "[62,70,27,163]";
        let de_err = serde_json::from_str::<SigningKey>(encoded_signing_key_too_long)
            .unwrap_err()
            .to_string();
        assert!(
            de_err.contains("invalid length 4"),
            "expected invalid length error, got: {de_err}"
        );
    }

    #[test]
    fn serialize_deserialize_signing_key_toml() {
        let demo = Demo {
            signing_key: SigningKey::from_bytes(&SECRET_KEY_BYTES),
        };

        println!("\n\nWrite to toml");
        let demo_toml = toml::to_string(&demo).unwrap();
        println!("{}", demo_toml);
        let demo_toml_rebuild: Result<Demo, _> = toml::from_str(&demo_toml);
        println!("{:?}", demo_toml_rebuild);
    }

    #[test]
    fn serialize_verifying_key_size() {
        let verifying_key: VerifyingKey = VerifyingKey::from_bytes(&PUBLIC_KEY_BYTES).unwrap();
        assert_eq!(
            bincode::serialized_size(&verifying_key).unwrap() as usize,
            BINCODE_INT_LENGTH + PUBLIC_KEY_LENGTH
        );
    }

    #[test]
    fn serialize_signature_size() {
        let signature: Signature = Signature::from_bytes(&SIGNATURE_BYTES);
        assert_eq!(
            bincode::serialized_size(&signature).unwrap() as usize,
            SIGNATURE_LENGTH
        );
    }

    #[test]
    fn serialize_signing_key_size() {
        let signing_key = SigningKey::from_bytes(&SECRET_KEY_BYTES);
        assert_eq!(
            bincode::serialized_size(&signing_key).unwrap() as usize,
            BINCODE_INT_LENGTH + SECRET_KEY_LENGTH
        );
    }
}
