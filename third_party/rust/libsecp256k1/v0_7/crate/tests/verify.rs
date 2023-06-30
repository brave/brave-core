use libsecp256k1::*;
use secp256k1_test::{
    key, rand::thread_rng, Error as SecpError, Message as SecpMessage, Secp256k1,
    Signature as SecpSignature,
};

#[cfg(feature = "hmac")]
mod signatures {
    use crate::{recover, sign, verify, Message, PublicKey, SecretKey, SharedSecret, Signature};
    use secp256k1_test::{
        ecdh::SharedSecret as SecpSharedSecret,
        key,
        rand::thread_rng,
        recovery::{
            RecoverableSignature as SecpRecoverableSignature, RecoveryId as SecpRecoveryId,
        },
        All, Message as SecpMessage, Secp256k1, Signature as SecpSignature,
    };
    use sha2::Sha256;

    fn genkey(
        secp256k1: &Secp256k1<All>,
    ) -> (key::PublicKey, key::SecretKey, PublicKey, SecretKey) {
        let (secp_privkey, secp_pubkey) = secp256k1.generate_keypair(&mut thread_rng());
        let pubkey_a = secp_pubkey.serialize_uncompressed();
        assert_eq!(pubkey_a.len(), 65);
        let pubkey = PublicKey::parse(&pubkey_a).unwrap();
        let mut seckey_a = [0u8; 32];
        for i in 0..32 {
            seckey_a[i] = secp_privkey[i];
        }
        let seckey = SecretKey::parse(&seckey_a).unwrap();

        (secp_pubkey, secp_privkey, pubkey, seckey)
    }

    #[test]
    fn test_signature_der() {
        let secp256k1 = Secp256k1::new();

        let message_arr = [5u8; 32];
        let (privkey, _) = secp256k1.generate_keypair(&mut thread_rng());

        assert!(privkey[..].len() == 32);
        let mut privkey_a = [0u8; 32];
        for i in 0..32 {
            privkey_a[i] = privkey[i];
        }

        let ctx_privkey = SecretKey::parse(&privkey_a).unwrap();
        let ctx_message = Message::parse(&message_arr);

        let (signature, _) = sign(&ctx_message, &ctx_privkey);
        let reconstructed = Signature::parse_der(signature.serialize_der().as_ref()).unwrap();
        assert_eq!(signature, reconstructed);
    }

    #[test]
    fn test_sign_verify() {
        let secp256k1 = Secp256k1::new();

        let message_arr = [6u8; 32];
        let (secp_privkey, secp_pubkey) = secp256k1.generate_keypair(&mut thread_rng());

        let secp_message = SecpMessage::from_slice(&message_arr).unwrap();
        let pubkey_a = secp_pubkey.serialize_uncompressed();
        assert_eq!(pubkey_a.len(), 65);
        let pubkey = PublicKey::parse(&pubkey_a).unwrap();
        let mut seckey_a = [0u8; 32];
        for i in 0..32 {
            seckey_a[i] = secp_privkey[i];
        }
        let seckey = SecretKey::parse(&seckey_a).unwrap();
        let message = Message::parse(&message_arr);

        let (sig, recid) = sign(&message, &seckey);

        // Self verify
        assert!(verify(&message, &sig, &pubkey));

        // Self recover
        let recovered_pubkey = recover(&message, &sig, &recid).unwrap();
        let rpa = recovered_pubkey.serialize();
        let opa = pubkey.serialize();
        let rpr: &[u8] = &rpa;
        let opr: &[u8] = &opa;
        assert_eq!(rpr, opr);

        let signature_a = sig.serialize();
        let secp_recid = SecpRecoveryId::from_i32(recid.into()).unwrap();
        let secp_rec_signature =
            SecpRecoverableSignature::from_compact(&signature_a, secp_recid).unwrap();
        let secp_signature = SecpSignature::from_compact(&signature_a).unwrap();

        // External verify
        secp256k1
            .verify(&secp_message, &secp_signature, &secp_pubkey)
            .unwrap();

        // External recover
        let recovered_pubkey = secp256k1
            .recover(&secp_message, &secp_rec_signature)
            .unwrap();
        let rpa = recovered_pubkey.serialize_uncompressed();
        let rpr: &[u8] = &rpa;
        assert_eq!(rpr, opr);
    }

    #[test]
    fn test_failing_sign_verify() {
        let seckey_a: [u8; 32] = [
            169, 195, 92, 103, 2, 159, 75, 46, 158, 79, 249, 49, 208, 28, 48, 210, 5, 47, 136, 77,
            21, 51, 224, 54, 213, 165, 90, 122, 233, 199, 0, 248,
        ];
        let seckey = SecretKey::parse(&seckey_a).unwrap();
        let pubkey = PublicKey::from_secret_key(&seckey);
        let message_arr = [6u8; 32];
        let message = Message::parse(&message_arr);

        let (sig, recid) = sign(&message, &seckey);
        let tmp: u8 = recid.into();
        assert_eq!(tmp, 1u8);

        let recovered_pubkey = recover(&message, &sig, &recid).unwrap();
        let rpa = recovered_pubkey.serialize();
        let opa = pubkey.serialize();
        let rpr: &[u8] = &rpa;
        let opr: &[u8] = &opa;
        assert_eq!(rpr, opr);
    }

    #[test]
    fn test_shared_secret() {
        let secp256k1 = Secp256k1::new();

        let (spub1, ssec1, pub1, sec1) = genkey(&secp256k1);
        let (spub2, ssec2, pub2, sec2) = genkey(&secp256k1);

        let shared1 = SharedSecret::<Sha256>::new(&pub1, &sec2).unwrap();
        let shared2 = SharedSecret::<Sha256>::new(&pub2, &sec1).unwrap();

        let secp_shared1 = SecpSharedSecret::new(&spub1, &ssec2);
        let secp_shared2 = SecpSharedSecret::new(&spub2, &ssec1);

        assert_eq!(shared1.as_ref(), shared2.as_ref());

        for i in 0..32 {
            assert_eq!(shared1.as_ref()[i], secp_shared1[i]);
        }

        for i in 0..32 {
            assert_eq!(shared2.as_ref()[i], secp_shared2[i]);
        }
    }
}

#[test]
fn test_verify() {
    let secp256k1 = Secp256k1::new();

    let message_arr = [5u8; 32];
    let (privkey, pubkey) = secp256k1.generate_keypair(&mut thread_rng());
    let message = SecpMessage::from_slice(&message_arr).unwrap();
    let signature = secp256k1.sign(&message, &privkey);

    let pubkey_a = pubkey.serialize_uncompressed();
    assert_eq!(pubkey_a.len(), 65);

    let ctx_pubkey = PublicKey::parse(&pubkey_a).unwrap();
    let ctx_message = Message::parse(&message_arr);
    let signature_a = signature.serialize_compact();
    assert_eq!(signature_a.len(), 64);
    let ctx_sig = Signature::parse_standard(&signature_a).expect("signature is valid");

    secp256k1.verify(&message, &signature, &pubkey).unwrap();
    assert!(verify(&ctx_message, &ctx_sig, &ctx_pubkey));
    let mut f_ctx_sig = ctx_sig;
    f_ctx_sig.r.set_int(0);
    if f_ctx_sig.r != ctx_sig.r {
        assert!(!ECMULT_CONTEXT.verify_raw(
            &f_ctx_sig.r,
            &ctx_sig.s,
            &ctx_pubkey.into(),
            &ctx_message.0
        ));
    }
    f_ctx_sig.r.set_int(1);
    if f_ctx_sig.r != ctx_sig.r {
        assert!(!ECMULT_CONTEXT.verify_raw(
            &f_ctx_sig.r,
            &ctx_sig.s,
            &ctx_pubkey.into(),
            &ctx_message.0
        ));
    }
}

#[test]
fn secret_clear_on_drop() {
    let secret: [u8; 32] = [1; 32];
    let mut seckey = SecretKey::parse(&secret).unwrap();

    clear_on_drop::clear::Clear::clear(&mut seckey);
    assert_eq!(seckey, SecretKey::default());
}

#[test]
fn test_recover() {
    let secp256k1 = Secp256k1::new();

    let message_arr = [5u8; 32];
    let (privkey, pubkey) = secp256k1.generate_keypair(&mut thread_rng());
    let message = SecpMessage::from_slice(&message_arr).unwrap();
    let signature = secp256k1.sign_recoverable(&message, &privkey);

    let pubkey_a = pubkey.serialize_uncompressed();
    assert_eq!(pubkey_a.len(), 65);

    let ctx_message = Message::parse(&message_arr);
    let (rec_id, signature_a) = signature.serialize_compact();
    assert_eq!(signature_a.len(), 64);
    let ctx_sig = Signature::parse_standard(&signature_a).expect("signature is valid");

    // secp256k1.recover(&message, &signature).unwrap();
    let ctx_pubkey = recover(
        &ctx_message,
        &ctx_sig,
        &RecoveryId::parse(rec_id.to_i32() as u8).unwrap(),
    )
    .unwrap();
    let sp = ctx_pubkey.serialize();

    let sps: &[u8] = &sp;
    let gps: &[u8] = &pubkey_a;
    assert_eq!(sps, gps);
}

fn from_hex(hex: &str, target: &mut [u8]) -> Result<usize, ()> {
    if hex.len() % 2 == 1 || hex.len() > target.len() * 2 {
        return Err(());
    }

    let mut b = 0;
    let mut idx = 0;
    for c in hex.bytes() {
        b <<= 4;
        match c {
            b'A'..=b'F' => b |= c - b'A' + 10,
            b'a'..=b'f' => b |= c - b'a' + 10,
            b'0'..=b'9' => b |= c - b'0',
            _ => return Err(()),
        }
        if (idx & 1) == 1 {
            target[idx / 2] = b;
            b = 0;
        }
        idx += 1;
    }
    Ok(idx / 2)
}

macro_rules! hex {
    ($hex:expr) => {{
        let mut result = vec![0; $hex.len() / 2];
        from_hex($hex, &mut result).expect("valid hex string");
        result
    }};
}

#[test]
fn test_signature_der_lax() {
    macro_rules! check_lax_sig {
        ($hex:expr) => {{
            let sig = hex!($hex);
            assert!(Signature::parse_der_lax(&sig[..]).is_ok());
        }};
    }

    check_lax_sig!("304402204c2dd8a9b6f8d425fcd8ee9a20ac73b619906a6367eac6cb93e70375225ec0160220356878eff111ff3663d7e6bf08947f94443845e0dcc54961664d922f7660b80c");
    check_lax_sig!("304402202ea9d51c7173b1d96d331bd41b3d1b4e78e66148e64ed5992abd6ca66290321c0220628c47517e049b3e41509e9d71e480a0cdc766f8cdec265ef0017711c1b5336f");
    check_lax_sig!("3045022100bf8e050c85ffa1c313108ad8c482c4849027937916374617af3f2e9a881861c9022023f65814222cab09d5ec41032ce9c72ca96a5676020736614de7b78a4e55325a");
    check_lax_sig!("3046022100839c1fbc5304de944f697c9f4b1d01d1faeba32d751c0f7acb21ac8a0f436a72022100e89bd46bb3a5a62adc679f659b7ce876d83ee297c7a5587b2011c4fcc72eab45");
    check_lax_sig!("3046022100eaa5f90483eb20224616775891397d47efa64c68b969db1dacb1c30acdfc50aa022100cf9903bbefb1c8000cf482b0aeeb5af19287af20bd794de11d82716f9bae3db1");
    check_lax_sig!("3045022047d512bc85842ac463ca3b669b62666ab8672ee60725b6c06759e476cebdc6c102210083805e93bd941770109bcc797784a71db9e48913f702c56e60b1c3e2ff379a60");
    check_lax_sig!("3044022023ee4e95151b2fbbb08a72f35babe02830d14d54bd7ed1320e4751751d1baa4802206235245254f58fd1be6ff19ca291817da76da65c2f6d81d654b5185dd86b8acf");
}

#[test]
fn test_low_s() {
    // nb this is a transaction on testnet
    // txid 8ccc87b72d766ab3128f03176bb1c98293f2d1f85ebfaf07b82cc81ea6891fa9
    //      input number 3
    let sig = hex!("3046022100839c1fbc5304de944f697c9f4b1d01d1faeba32d751c0f7acb21ac8a0f436a72022100e89bd46bb3a5a62adc679f659b7ce876d83ee297c7a5587b2011c4fcc72eab45");
    let pk = hex!("031ee99d2b786ab3b0991325f2de8489246a6a3fdb700f6d0511b1d80cf5f4cd43");
    let msg = hex!("a4965ca63b7d8562736ceec36dfa5a11bf426eb65be8ea3f7a49ae363032da0d");

    let secp = Secp256k1::new();
    let mut sig = Signature::parse_der(&sig[..]).unwrap();
    let pk = key::PublicKey::from_slice(&pk[..]).unwrap();
    let msg = SecpMessage::from_slice(&msg[..]).unwrap();

    // without normalization we expect this will fail
    assert_eq!(
        secp.verify(
            &msg,
            &SecpSignature::from_compact(&sig.serialize()).unwrap(),
            &pk
        ),
        Err(SecpError::IncorrectSignature)
    );
    // after normalization it should pass
    sig.normalize_s();
    assert_eq!(
        secp.verify(
            &msg,
            &SecpSignature::from_compact(&sig.serialize()).unwrap(),
            &pk
        ),
        Ok(())
    );
}

#[test]
fn test_convert_key1() {
    let secret: [u8; 32] = [
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,
    ];
    let expected: &[u8] = &[
        0x04, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62, 0x95, 0xce, 0x87,
        0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16,
        0xf8, 0x17, 0x98, 0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65, 0x5d, 0xa4, 0xfb, 0xfc,
        0x0e, 0x11, 0x08, 0xa8, 0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19, 0x9c, 0x47, 0xd0,
        0x8f, 0xfb, 0x10, 0xd4, 0xb8,
    ];
    let seckey = SecretKey::parse(&secret).unwrap();
    let pubkey = PublicKey::from_secret_key(&seckey);
    assert_eq!(expected, &pubkey.serialize()[..]);
    let pubkey_compressed = PublicKey::parse_compressed(&pubkey.serialize_compressed()).unwrap();
    assert_eq!(expected, &pubkey_compressed.serialize()[..]);
}

#[test]
fn test_convert_key2() {
    let secret: [u8; 32] = [
        0x4d, 0x5d, 0xb4, 0x10, 0x7d, 0x23, 0x7d, 0xf6, 0xa3, 0xd5, 0x8e, 0xe5, 0xf7, 0x0a, 0xe6,
        0x3d, 0x73, 0xd7, 0x65, 0x8d, 0x40, 0x26, 0xf2, 0xee, 0xfd, 0x2f, 0x20, 0x4c, 0x81, 0x68,
        0x2c, 0xb7,
    ];
    let expected: &[u8] = &[
        0x04, 0x3f, 0xa8, 0xc0, 0x8c, 0x65, 0xa8, 0x3f, 0x6b, 0x4e, 0xa3, 0xe0, 0x4e, 0x1c, 0xc7,
        0x0c, 0xbe, 0x3c, 0xd3, 0x91, 0x49, 0x9e, 0x3e, 0x05, 0xab, 0x7d, 0xed, 0xf2, 0x8a, 0xff,
        0x9a, 0xfc, 0x53, 0x82, 0x00, 0xff, 0x93, 0xe3, 0xf2, 0xb2, 0xcb, 0x50, 0x29, 0xf0, 0x3c,
        0x7e, 0xbe, 0xe8, 0x20, 0xd6, 0x3a, 0x4c, 0x5a, 0x95, 0x41, 0xc8, 0x3a, 0xce, 0xbe, 0x29,
        0x3f, 0x54, 0xca, 0xcf, 0x0e,
    ];
    let seckey = SecretKey::parse(&secret).unwrap();
    let pubkey = PublicKey::from_secret_key(&seckey);
    assert_eq!(expected, &pubkey.serialize()[..]);
    let pubkey_compressed = PublicKey::parse_compressed(&pubkey.serialize_compressed()).unwrap();
    assert_eq!(expected, &pubkey_compressed.serialize()[..]);
}

#[test]
fn test_convert_anykey() {
    let secp256k1 = Secp256k1::new();
    let (secp_privkey, secp_pubkey) = secp256k1.generate_keypair(&mut thread_rng());

    let mut secret = [0u8; 32];
    for i in 0..32 {
        secret[i] = secp_privkey[i];
    }

    let seckey = SecretKey::parse(&secret).unwrap();
    let pubkey = PublicKey::from_secret_key(&seckey);
    let public = pubkey.serialize();
    let public_compressed = pubkey.serialize_compressed();
    let pubkey_r: &[u8] = &public;
    let pubkey_compressed_r: &[u8] = &public_compressed;

    let secp_pubkey_a = secp_pubkey.serialize_uncompressed();
    assert_eq!(secp_pubkey_a.len(), 65);
    let secp_pubkey_compressed_a = secp_pubkey.serialize();
    assert_eq!(secp_pubkey_compressed_a.len(), 33);
    let secp_pubkey_r: &[u8] = &secp_pubkey_a;
    let secp_pubkey_compressed_r: &[u8] = &secp_pubkey_compressed_a;

    assert_eq!(secp_pubkey_r, pubkey_r);
    assert_eq!(secp_pubkey_compressed_r, pubkey_compressed_r);
}

#[test]
fn test_pubkey_combine() {
    let pk1 = PublicKey::parse(&[
        4, 126, 60, 36, 91, 73, 177, 194, 111, 11, 3, 99, 246, 204, 86, 122, 109, 85, 28, 43, 169,
        243, 35, 76, 152, 90, 76, 241, 17, 108, 232, 215, 115, 15, 19, 23, 164, 151, 43, 28, 44,
        59, 141, 167, 134, 112, 105, 251, 15, 193, 183, 224, 238, 154, 204, 230, 163, 216, 235,
        112, 77, 239, 98, 135, 132,
    ])
    .unwrap();
    let pk2 = PublicKey::parse(&[
        4, 40, 127, 167, 223, 38, 53, 6, 223, 67, 83, 204, 60, 226, 227, 107, 231, 172, 34, 3, 187,
        79, 112, 167, 0, 217, 118, 69, 218, 189, 208, 150, 190, 54, 186, 220, 95, 80, 220, 183,
        202, 117, 160, 18, 84, 245, 181, 23, 32, 51, 73, 178, 173, 92, 118, 92, 122, 83, 49, 54,
        195, 194, 16, 229, 39,
    ])
    .unwrap();
    let cpk = PublicKey::parse(&[
        4, 101, 166, 20, 152, 34, 76, 121, 113, 139, 80, 13, 92, 122, 96, 38, 194, 205, 149, 93,
        19, 147, 132, 195, 173, 42, 86, 26, 221, 170, 127, 180, 168, 145, 21, 75, 45, 248, 90, 114,
        118, 62, 196, 194, 143, 245, 204, 184, 16, 175, 202, 175, 228, 207, 112, 219, 94, 237, 75,
        105, 186, 56, 102, 46, 147,
    ])
    .unwrap();

    assert_eq!(PublicKey::combine(&[pk1, pk2]).unwrap(), cpk);
}

#[test]
fn test_pubkey_equality() {
    for _ in 0..10 {
        let secret = SecretKey::random(&mut rand::rngs::OsRng);
        let public = PublicKey::from_secret_key(&secret);

        let public2 = PublicKey::parse(&public.serialize()).unwrap();
        let public3 = PublicKey::parse_compressed(&public.serialize_compressed()).unwrap();

        // Reflexivity
        assert_eq!(public, public);
        assert_eq!(public2, public2);
        assert_eq!(public3, public3);

        // Symmetry
        assert_eq!(public2, public);
        assert_eq!(public, public2);
        assert_eq!(public2, public3);
        assert_eq!(public3, public2);

        // Transitivity
        assert_eq!(public, public3);
        assert_eq!(public3, public);
    }
}
