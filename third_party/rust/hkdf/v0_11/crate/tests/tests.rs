use core::iter;

use hex;

use hkdf::{Hkdf, HkdfExtract};
use sha1::Sha1;
use sha2::{Sha256, Sha384, Sha512};

struct Test<'a> {
    ikm: &'a str,
    salt: &'a str,
    info: &'a str,
    length: usize,
    prk: &'a str,
    okm: &'a str,
}

// Test Vectors from https://tools.ietf.org/html/rfc5869.
fn tests_sha256<'a>() -> Vec<Test<'a>> {
    vec![
        Test {
            // Test Case 1
            ikm: "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
            salt: "000102030405060708090a0b0c",
            info: "f0f1f2f3f4f5f6f7f8f9",
            length: 42,
            prk: "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5",
            okm: "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b8\
                  87185865",
        },
        Test {
            // Test Case 2
            ikm: "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425\
                  262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b\
                  4c4d4e4f",
            salt: "606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f80818283848\
                   5868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aa\
                   abacadaeaf",
            info: "b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d\
                   5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fa\
                   fbfcfdfeff",
            length: 82,
            prk: "06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244",
            okm: "b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c59045a99cac7\
                  827271cb41c65e590e09da3275600c2f09b8367793a9aca3db71cc30c58179ec3e87c14c01d5\
                  c1f3434f1d87",
        },
        Test {
            // Test Case 3
            ikm: "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
            salt: "",
            info: "",
            length: 42,
            prk: "19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04",
            okm: "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4\
                  b61a96c8",
        },
    ]
}

#[test]
fn test_derive_sha256() {
    let tests = tests_sha256();
    for t in tests.iter() {
        let ikm = hex::decode(&t.ikm).unwrap();
        let salt = hex::decode(&t.salt).unwrap();
        let info = hex::decode(&t.info).unwrap();
        let salt = if salt.is_empty() {
            None
        } else {
            Some(&salt[..])
        };
        let (prk, hkdf) = Hkdf::<Sha256>::extract(salt, &ikm[..]);
        let mut okm = vec![0u8; t.length];
        assert!(hkdf.expand(&info[..], &mut okm).is_ok());

        assert_eq!(hex::encode(prk), t.prk);
        assert_eq!(hex::encode(&okm), t.okm);

        let prk = &hex::decode(&t.prk).unwrap();
        let hkdf = Hkdf::<Sha256>::from_prk(prk).unwrap();
        assert!(hkdf.expand(&info[..], &mut okm).is_ok());

        assert_eq!(hex::encode(&okm), t.okm);
    }
}

const MAX_SHA256_LENGTH: usize = 255 * (256 / 8); // =8160

#[test]
fn test_lengths() {
    let hkdf = Hkdf::<Sha256>::new(None, &[]);
    let mut longest = vec![0u8; MAX_SHA256_LENGTH];
    assert!(hkdf.expand(&[], &mut longest).is_ok());
    // Runtime is O(length), so exhaustively testing all legal lengths
    // would take too long (at least without --release). Only test a
    // subset: the first 500, the last 10, and every 100th in between.
    let lengths = (0..MAX_SHA256_LENGTH + 1)
        .filter(|&len| len < 500 || len > MAX_SHA256_LENGTH - 10 || len % 100 == 0);

    for length in lengths {
        let mut okm = vec![0u8; length];
        assert!(hkdf.expand(&[], &mut okm).is_ok());
        assert_eq!(okm.len(), length);
        assert_eq!(hex::encode(okm), hex::encode(longest[..length].iter()));
    }
}

#[test]
fn test_max_length() {
    let hkdf = Hkdf::<Sha256>::new(Some(&[]), &[]);
    let mut okm = vec![0u8; MAX_SHA256_LENGTH];
    assert!(hkdf.expand(&[], &mut okm).is_ok());
}

#[test]
fn test_max_length_exceeded() {
    let hkdf = Hkdf::<Sha256>::new(Some(&[]), &[]);
    let mut okm = vec![0u8; MAX_SHA256_LENGTH + 1];
    assert!(hkdf.expand(&[], &mut okm).is_err());
}

#[test]
fn test_unsupported_length() {
    let hkdf = Hkdf::<Sha256>::new(Some(&[]), &[]);
    let mut okm = vec![0u8; 90000];
    assert!(hkdf.expand(&[], &mut okm).is_err());
}

#[test]
fn test_prk_too_short() {
    use sha2::digest::generic_array::typenum::Unsigned;
    use sha2::digest::Digest;

    let output_len = <Sha256 as Digest>::OutputSize::to_usize();
    let prk = vec![0; output_len - 1];
    assert!(Hkdf::<Sha256>::from_prk(&prk).is_err());
}

// Test Vectors from https://tools.ietf.org/html/rfc5869.
fn tests_sha1<'a>() -> Vec<Test<'a>> {
    vec![
        Test {
            // Test Case 4
            ikm: "0b0b0b0b0b0b0b0b0b0b0b",
            salt: "000102030405060708090a0b0c",
            info: "f0f1f2f3f4f5f6f7f8f9",
            length: 42,
            prk: "9b6c18c432a7bf8f0e71c8eb88f4b30baa2ba243",
            okm: "085a01ea1b10f36933068b56efa5ad81\
                  a4f14b822f5b091568a9cdd4f155fda2\
                  c22e422478d305f3f896",
        },
        Test {
            // Test Case 5
            ikm: "000102030405060708090a0b0c0d0e0f\
                  101112131415161718191a1b1c1d1e1f\
                  202122232425262728292a2b2c2d2e2f\
                  303132333435363738393a3b3c3d3e3f\
                  404142434445464748494a4b4c4d4e4f",
            salt: "606162636465666768696a6b6c6d6e6f\
                   707172737475767778797a7b7c7d7e7f\
                   808182838485868788898a8b8c8d8e8f\
                   909192939495969798999a9b9c9d9e9f\
                   a0a1a2a3a4a5a6a7a8a9aaabacadaeaf",
            info: "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf\
                   c0c1c2c3c4c5c6c7c8c9cacbcccdcecf\
                   d0d1d2d3d4d5d6d7d8d9dadbdcdddedf\
                   e0e1e2e3e4e5e6e7e8e9eaebecedeeef\
                   f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
            length: 82,
            prk: "8adae09a2a307059478d309b26c4115a224cfaf6",
            okm: "0bd770a74d1160f7c9f12cd5912a06eb\
                  ff6adcae899d92191fe4305673ba2ffe\
                  8fa3f1a4e5ad79f3f334b3b202b2173c\
                  486ea37ce3d397ed034c7f9dfeb15c5e\
                  927336d0441f4c4300e2cff0d0900b52\
                  d3b4",
        },
        Test {
            // Test Case 6
            ikm: "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b",
            salt: "",
            info: "",
            length: 42,
            prk: "da8c8a73c7fa77288ec6f5e7c297786aa0d32d01",
            okm: "0ac1af7002b3d761d1e55298da9d0506\
                  b9ae52057220a306e07b6b87e8df21d0\
                  ea00033de03984d34918",
        },
        Test {
            // Test Case 7
            ikm: "0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c",
            salt: "", // "Not Provided"
            info: "",
            length: 42,
            prk: "2adccada18779e7c2077ad2eb19d3f3e731385dd",
            okm: "2c91117204d745f3500d636a62f64f0a\
                  b3bae548aa53d423b0d1f27ebba6f5e5\
                  673a081d70cce7acfc48",
        },
    ]
}

#[test]
fn test_derive_sha1() {
    let tests = tests_sha1();
    for t in tests.iter() {
        let ikm = hex::decode(&t.ikm).unwrap();
        let salt = hex::decode(&t.salt).unwrap();
        let info = hex::decode(&t.info).unwrap();
        let salt = if salt.is_empty() {
            None
        } else {
            Some(&salt[..])
        };
        let (prk, hkdf) = Hkdf::<Sha1>::extract(salt, &ikm[..]);
        let mut okm = vec![0u8; t.length];
        assert!(hkdf.expand(&info[..], &mut okm).is_ok());

        assert_eq!(hex::encode(prk), t.prk);
        assert_eq!(hex::encode(&okm), t.okm);

        let prk = &hex::decode(&t.prk).unwrap();
        let hkdf = Hkdf::<Sha1>::from_prk(&prk).unwrap();
        assert!(hkdf.expand(&info[..], &mut okm).is_ok());

        assert_eq!(hex::encode(&okm), t.okm);
    }
}

#[test]
fn test_derive_sha1_with_none() {
    let ikm = hex::decode("0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c0c").unwrap();
    let salt = None;
    let info = hex::decode("").unwrap();
    let (prk, hkdf) = Hkdf::<Sha1>::extract(salt, &ikm[..]);
    let mut okm = vec![0u8; 42];
    assert!(hkdf.expand(&info[..], &mut okm).is_ok());

    assert_eq!(hex::encode(prk), "2adccada18779e7c2077ad2eb19d3f3e731385dd");
    assert_eq!(
        hex::encode(&okm),
        "2c91117204d745f3500d636a62f64f0a\
         b3bae548aa53d423b0d1f27ebba6f5e5\
         673a081d70cce7acfc48"
    );
}

#[test]
fn test_expand_multi_info() {
    let info_components = &[
        &b"09090909090909090909090909090909090909090909"[..],
        &b"8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a"[..],
        &b"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0"[..],
        &b"4c4c4c4c4c4c4c4c4c4c4c4c4c4c4c4c4c4c4"[..],
        &b"1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d"[..],
    ];

    let (_, hkdf_ctx) = Hkdf::<Sha256>::extract(None, b"some ikm here");

    // Compute HKDF-Expand on the concatenation of all the info components
    let mut oneshot_res = [0u8; 16];
    hkdf_ctx
        .expand(&info_components.concat(), &mut oneshot_res)
        .unwrap();

    // Now iteratively join the components of info_components until it's all 1 component. The value
    // of HKDF-Expand should be the same throughout
    let mut num_concatted = 0;
    let mut info_head = Vec::new();

    while num_concatted < info_components.len() {
        info_head.extend(info_components[num_concatted]);

        // Build the new input to be the info head followed by the remaining components
        let input: Vec<&[u8]> = iter::once(info_head.as_slice())
            .chain(info_components.iter().cloned().skip(num_concatted + 1))
            .collect();

        // Compute and compare to the one-shot answer
        let mut multipart_res = [0u8; 16];
        hkdf_ctx
            .expand_multi_info(&input, &mut multipart_res)
            .unwrap();
        assert_eq!(multipart_res, oneshot_res);

        num_concatted += 1;
    }
}

#[test]
fn test_extract_streaming() {
    let ikm_components = &[
        &b"09090909090909090909090909090909090909090909"[..],
        &b"8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a8a"[..],
        &b"0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0"[..],
        &b"4c4c4c4c4c4c4c4c4c4c4c4c4c4c4c4c4c4c4"[..],
        &b"1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d1d"[..],
    ];
    let salt = b"mysalt";

    // Compute HKDF-Extract on the concatenation of all the IKM components
    let (oneshot_res, _) = Hkdf::<Sha256>::extract(Some(&salt[..]), &ikm_components.concat());

    // Now iteratively join the components of ikm_components until it's all 1 component. The value
    // of HKDF-Extract should be the same throughout
    let mut num_concatted = 0;
    let mut ikm_head = Vec::new();

    while num_concatted < ikm_components.len() {
        ikm_head.extend(ikm_components[num_concatted]);

        // Make a new extraction context and build the new input to be the IKM head followed by the
        // remaining components
        let mut extract_ctx = HkdfExtract::<Sha256>::new(Some(&salt[..]));
        let input = iter::once(ikm_head.as_slice())
            .chain(ikm_components.iter().cloned().skip(num_concatted + 1));

        // Stream in the IKM input in the chunks specified
        for ikm in input {
            extract_ctx.input_ikm(ikm);
        }

        // Finalize and compare to the one-shot answer
        let (multipart_res, _) = extract_ctx.finalize();
        assert_eq!(multipart_res, oneshot_res);

        num_concatted += 1;
    }
}

/// Define test
macro_rules! new_test {
    ($name:ident, $test_name:expr, $hkdf:ty) => {
        #[test]
        fn $name() {
            use blobby::Blob4Iterator;

            fn run_test(ikm: &[u8], salt: &[u8], info: &[u8], okm: &[u8]) -> Option<&'static str> {
                let prk = <$hkdf>::new(Some(salt), ikm);
                let mut got_okm = vec![0; okm.len()];

                if prk.expand(info, &mut got_okm).is_err() {
                    return Some("prk expand");
                }
                if got_okm != okm {
                    return Some("mismatch in okm");
                }
                None
            }

            let data = include_bytes!(concat!("data/", $test_name, ".blb"));

            for (i, row) in Blob4Iterator::new(data).unwrap().enumerate() {
                let [ikm, salt, info, okm] = row.unwrap();
                if let Some(desc) = run_test(ikm, salt, info, okm) {
                    panic!(
                        "\n\
                         Failed test №{}: {}\n\
                         ikm:\t{:?}\n\
                         salt:\t{:?}\n\
                         info:\t{:?}\n\
                         okm:\t{:?}\n",
                        i, desc, ikm, salt, info, okm
                    );
                }
            }
        }
    };
}

new_test!(wycheproof_sha1, "wycheproof-sha1", Hkdf::<Sha1>);
new_test!(wycheproof_sha256, "wycheproof-sha256", Hkdf::<Sha256>);
new_test!(wycheproof_sha384, "wycheproof-sha384", Hkdf::<Sha384>);
new_test!(wycheproof_sha512, "wycheproof-sha512", Hkdf::<Sha512>);
