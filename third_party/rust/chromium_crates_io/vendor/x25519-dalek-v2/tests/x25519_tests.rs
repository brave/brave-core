use curve25519_dalek::edwards::EdwardsPoint;

use x25519_dalek::*;

#[test]
fn byte_basepoint_matches_edwards_scalar_mul() {
    let mut scalar_bytes = [0x37; 32];

    for i in 0..32 {
        scalar_bytes[i] += 2;

        let result = x25519(scalar_bytes, X25519_BASEPOINT_BYTES);
        let expected = EdwardsPoint::mul_base_clamped(scalar_bytes)
            .to_montgomery()
            .to_bytes();

        assert_eq!(result, expected);
    }
}

#[test]
#[cfg(feature = "serde")]
fn serde_bincode_public_key_roundtrip() {
    use bincode;

    let public_key = PublicKey::from(X25519_BASEPOINT_BYTES);

    let encoded = bincode::serialize(&public_key).unwrap();
    let decoded: PublicKey = bincode::deserialize(&encoded).unwrap();

    assert_eq!(encoded.len(), 32);
    assert_eq!(decoded.as_bytes(), public_key.as_bytes());
}

#[test]
#[cfg(feature = "serde")]
fn serde_bincode_public_key_matches_from_bytes() {
    use bincode;

    let expected = PublicKey::from(X25519_BASEPOINT_BYTES);
    let decoded: PublicKey = bincode::deserialize(&X25519_BASEPOINT_BYTES).unwrap();

    assert_eq!(decoded.as_bytes(), expected.as_bytes());
}

#[test]
#[cfg(feature = "serde")]
fn serde_bincode_static_secret_roundtrip() {
    use bincode;

    let static_secret = StaticSecret::from([0x24; 32]);
    let encoded = bincode::serialize(&static_secret).unwrap();
    let decoded: StaticSecret = bincode::deserialize(&encoded).unwrap();

    assert_eq!(encoded.len(), 32);
    assert_eq!(decoded.to_bytes(), static_secret.to_bytes());
}

#[test]
#[cfg(feature = "serde")]
fn serde_bincode_static_secret_matches_from_bytes() {
    use bincode;

    let expected = StaticSecret::from([0x24; 32]);
    let decoded: StaticSecret = bincode::deserialize(&[0x24; 32]).unwrap();

    assert_eq!(decoded.to_bytes(), expected.to_bytes());
}

fn do_rfc7748_ladder_test1(input_scalar: [u8; 32], input_point: [u8; 32], expected: [u8; 32]) {
    let result = x25519(input_scalar, input_point);

    assert_eq!(result, expected);
}

#[test]
fn rfc7748_ladder_test1_vectorset1() {
    let input_scalar: [u8; 32] = [
        0xa5, 0x46, 0xe3, 0x6b, 0xf0, 0x52, 0x7c, 0x9d, 0x3b, 0x16, 0x15, 0x4b, 0x82, 0x46, 0x5e,
        0xdd, 0x62, 0x14, 0x4c, 0x0a, 0xc1, 0xfc, 0x5a, 0x18, 0x50, 0x6a, 0x22, 0x44, 0xba, 0x44,
        0x9a, 0xc4,
    ];
    let input_point: [u8; 32] = [
        0xe6, 0xdb, 0x68, 0x67, 0x58, 0x30, 0x30, 0xdb, 0x35, 0x94, 0xc1, 0xa4, 0x24, 0xb1, 0x5f,
        0x7c, 0x72, 0x66, 0x24, 0xec, 0x26, 0xb3, 0x35, 0x3b, 0x10, 0xa9, 0x03, 0xa6, 0xd0, 0xab,
        0x1c, 0x4c,
    ];
    let expected: [u8; 32] = [
        0xc3, 0xda, 0x55, 0x37, 0x9d, 0xe9, 0xc6, 0x90, 0x8e, 0x94, 0xea, 0x4d, 0xf2, 0x8d, 0x08,
        0x4f, 0x32, 0xec, 0xcf, 0x03, 0x49, 0x1c, 0x71, 0xf7, 0x54, 0xb4, 0x07, 0x55, 0x77, 0xa2,
        0x85, 0x52,
    ];

    do_rfc7748_ladder_test1(input_scalar, input_point, expected);
}

#[test]
fn rfc7748_ladder_test1_vectorset2() {
    let input_scalar: [u8; 32] = [
        0x4b, 0x66, 0xe9, 0xd4, 0xd1, 0xb4, 0x67, 0x3c, 0x5a, 0xd2, 0x26, 0x91, 0x95, 0x7d, 0x6a,
        0xf5, 0xc1, 0x1b, 0x64, 0x21, 0xe0, 0xea, 0x01, 0xd4, 0x2c, 0xa4, 0x16, 0x9e, 0x79, 0x18,
        0xba, 0x0d,
    ];
    let input_point: [u8; 32] = [
        0xe5, 0x21, 0x0f, 0x12, 0x78, 0x68, 0x11, 0xd3, 0xf4, 0xb7, 0x95, 0x9d, 0x05, 0x38, 0xae,
        0x2c, 0x31, 0xdb, 0xe7, 0x10, 0x6f, 0xc0, 0x3c, 0x3e, 0xfc, 0x4c, 0xd5, 0x49, 0xc7, 0x15,
        0xa4, 0x93,
    ];
    let expected: [u8; 32] = [
        0x95, 0xcb, 0xde, 0x94, 0x76, 0xe8, 0x90, 0x7d, 0x7a, 0xad, 0xe4, 0x5c, 0xb4, 0xb8, 0x73,
        0xf8, 0x8b, 0x59, 0x5a, 0x68, 0x79, 0x9f, 0xa1, 0x52, 0xe6, 0xf8, 0xf7, 0x64, 0x7a, 0xac,
        0x79, 0x57,
    ];

    do_rfc7748_ladder_test1(input_scalar, input_point, expected);
}

#[test]
#[ignore] // Run only if you want to burn a lot of CPU doing 1,000,000 DH operations
fn rfc7748_ladder_test2() {
    use curve25519_dalek::constants::X25519_BASEPOINT;

    let mut k: [u8; 32] = X25519_BASEPOINT.0;
    let mut u: [u8; 32] = X25519_BASEPOINT.0;
    let mut result: [u8; 32];

    macro_rules! do_iterations {
        ($n:expr) => {
            for _ in 0..$n {
                result = x25519(k, u);
                // OBVIOUS THING THAT I'M GOING TO NOTE ANYWAY BECAUSE I'VE
                // SEEN PEOPLE DO THIS WITH GOLANG'S STDLIB AND YOU SURE AS
                // HELL SHOULDN'T DO HORRIBLY STUPID THINGS LIKE THIS WITH
                // MY LIBRARY:
                //
                // NEVER EVER TREAT SCALARS AS POINTS AND/OR VICE VERSA.
                //
                //                ↓↓ DON'T DO THIS ↓↓
                u = k.clone();
                k = result;
            }
        };
    }

    // After one iteration:
    //     422c8e7a6227d7bca1350b3e2bb7279f7897b87bb6854b783c60e80311ae3079
    // After 1,000 iterations:
    //     684cf59ba83309552800ef566f2f4d3c1c3887c49360e3875f2eb94d99532c51
    // After 1,000,000 iterations:
    //     7c3911e0ab2586fd864497297e575e6f3bc601c0883c30df5f4dd2d24f665424

    do_iterations!(1);
    assert_eq!(
        k,
        [
            0x42, 0x2c, 0x8e, 0x7a, 0x62, 0x27, 0xd7, 0xbc, 0xa1, 0x35, 0x0b, 0x3e, 0x2b, 0xb7,
            0x27, 0x9f, 0x78, 0x97, 0xb8, 0x7b, 0xb6, 0x85, 0x4b, 0x78, 0x3c, 0x60, 0xe8, 0x03,
            0x11, 0xae, 0x30, 0x79,
        ]
    );
    do_iterations!(999);
    assert_eq!(
        k,
        [
            0x68, 0x4c, 0xf5, 0x9b, 0xa8, 0x33, 0x09, 0x55, 0x28, 0x00, 0xef, 0x56, 0x6f, 0x2f,
            0x4d, 0x3c, 0x1c, 0x38, 0x87, 0xc4, 0x93, 0x60, 0xe3, 0x87, 0x5f, 0x2e, 0xb9, 0x4d,
            0x99, 0x53, 0x2c, 0x51,
        ]
    );
    do_iterations!(999_000);
    assert_eq!(
        k,
        [
            0x7c, 0x39, 0x11, 0xe0, 0xab, 0x25, 0x86, 0xfd, 0x86, 0x44, 0x97, 0x29, 0x7e, 0x57,
            0x5e, 0x6f, 0x3b, 0xc6, 0x01, 0xc0, 0x88, 0x3c, 0x30, 0xdf, 0x5f, 0x4d, 0xd2, 0xd2,
            0x4f, 0x66, 0x54, 0x24,
        ]
    );
}

mod rand_core {

    use super::*;
    use ::rand_core::OsRng;

    #[test]
    fn ephemeral_from_rng() {
        #[allow(deprecated)]
        EphemeralSecret::new(OsRng);
        EphemeralSecret::random_from_rng(OsRng);
    }

    #[test]
    #[cfg(feature = "reusable_secrets")]
    fn reusable_from_rng() {
        #[allow(deprecated)]
        ReusableSecret::new(OsRng);
        ReusableSecret::random_from_rng(OsRng);
    }

    #[test]
    #[cfg(feature = "static_secrets")]
    fn static_from_rng() {
        #[allow(deprecated)]
        StaticSecret::new(OsRng);
        StaticSecret::random_from_rng(OsRng);
    }
}

#[cfg(feature = "getrandom")]
mod getrandom {

    use super::*;

    #[test]
    fn ephemeral_random() {
        EphemeralSecret::random();
    }

    #[test]
    #[cfg(feature = "reusable_secrets")]
    fn reusable_random() {
        ReusableSecret::random();
    }

    #[test]
    #[cfg(feature = "static_secrets")]
    fn static_random() {
        StaticSecret::random();
    }
}
