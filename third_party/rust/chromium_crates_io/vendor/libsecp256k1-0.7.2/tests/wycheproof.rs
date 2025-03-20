use serde::Deserialize;
use sha2::Digest;
use std::collections::HashMap;

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
#[allow(dead_code)]
struct TestCollection {
    algorithm: String,
    generator_version: String,
    number_of_tests: usize,
    header: Vec<String>,
    notes: HashMap<String, String>,
    schema: String,
    test_groups: Vec<TestGroup>,
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
#[allow(dead_code)]
struct TestGroup {
    key: TestKey,
    key_der: String,
    key_pem: String,
    sha: String,
    #[serde(rename = "type")]
    typ: String,
    tests: Vec<TestUnit>,
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
#[allow(dead_code)]
struct TestKey {
    curve: String,
    key_size: usize,
    #[serde(rename = "type")]
    typ: String,
    uncompressed: String,
    wx: String,
    wy: String,
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
#[allow(dead_code)]
struct TestUnit {
    tc_id: usize,
    comment: String,
    msg: String,
    sig: String,
    result: TestResult,
    flags: Vec<String>,
}

#[derive(Deserialize, PartialEq, Eq, Debug)]
#[serde(rename_all = "camelCase")]
enum TestResult {
    Valid,
    Acceptable,
    Invalid,
}

enum TestError {
    MessageDecoding,
    SignatureDecoding,
    Verification,
}

fn test_unit(test: &TestUnit, key: &libsecp256k1::PublicKey) -> Result<(), TestError> {
    println!("tcId: {}, comment: {}", test.tc_id, test.comment);

    let msg_raw = hex::decode(&test.msg).unwrap();
    let sig_raw = hex::decode(&test.sig).unwrap();

    let msg_hashed_raw = sha2::Sha256::digest(&msg_raw);
    let msg = libsecp256k1::Message::parse_slice(&msg_hashed_raw)
        .map_err(|_| TestError::MessageDecoding)?;
    let sig =
        libsecp256k1::Signature::parse_der(&sig_raw).map_err(|_| TestError::SignatureDecoding)?;

    if libsecp256k1::verify(&msg, &sig, &key) {
        Ok(())
    } else {
        Err(TestError::Verification)
    }
}

#[test]
fn test_wycheproof() {
    let test_collection_str = include_str!("../res/ecdsa_secp256k1_sha256_test.json");
    let test_collection: TestCollection = serde_json::from_str(test_collection_str).unwrap();

    for test_group in test_collection.test_groups {
        assert_eq!(test_group.key.typ, "EcPublicKey");
        assert_eq!(test_group.key.curve, "secp256k1");
        assert_eq!(test_group.key.key_size, 256);

        let key_raw = hex::decode(test_group.key.uncompressed).unwrap();
        let key = libsecp256k1::PublicKey::parse_slice(&key_raw, None).unwrap();

        for test in test_group.tests {
            let res = test_unit(&test, &key);

            match res {
                Ok(()) => assert!(test.result == TestResult::Valid),
                Err(TestError::Verification) => assert_eq!(test.result, TestResult::Invalid),
                Err(TestError::MessageDecoding) => assert_eq!(test.result, TestResult::Invalid),
                // libsecp256k1 do not use any legacy formats, so "acceptable"
                // result in wycheproof is considered the same as invalid.
                Err(TestError::SignatureDecoding) => assert!(
                    test.result == TestResult::Acceptable || test.result == TestResult::Invalid
                ),
            }
        }
    }
}
