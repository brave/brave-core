use ed25519::signature::Verifier;
use ed25519_dalek::{Signature, VerifyingKey};

use serde::{de::Error as SError, Deserialize, Deserializer};
use std::{collections::BTreeSet as Set, fs::File};

/// The set of edge cases that [`VerifyingKey::verify()`] permits.
const VERIFY_ALLOWED_EDGECASES: &[Flag] = &[
    Flag::LowOrderA,
    Flag::LowOrderR,
    Flag::NonCanonicalA,
    Flag::LowOrderComponentA,
    Flag::LowOrderComponentR,
    // `ReencodedK` is not actually permitted by `verify()`, but it looks that way in the tests
    // because it sometimes occurs with a low-order A. 1/8 of the time, the resulting signature
    // will be identical the one made with a normal k. find_validation_criteria shows that indeed
    // this occurs 10/58 of the time
    Flag::ReencodedK,
];

/// The set of edge cases that [`VerifyingKey::verify_strict()`] permits
const VERIFY_STRICT_ALLOWED_EDGECASES: &[Flag] =
    &[Flag::LowOrderComponentA, Flag::LowOrderComponentR];

/// Each variant describes a specific edge case that can occur in an Ed25519 signature. Refer to
/// the test vector [README][] for more info.
///
/// [README]: https://github.com/C2SP/CCTV/blob/5ea85644bd035c555900a2f707f7e4c31ea65ced/ed25519vectors/README.md
#[derive(Deserialize, Debug, Copy, Clone, PartialOrd, Ord, Eq, PartialEq)]
enum Flag {
    #[serde(rename = "low_order")]
    LowOrder,
    #[serde(rename = "low_order_A")]
    LowOrderA,
    #[serde(rename = "low_order_R")]
    LowOrderR,
    #[serde(rename = "non_canonical_A")]
    NonCanonicalA,
    #[serde(rename = "non_canonical_R")]
    NonCanonicalR,
    #[serde(rename = "low_order_component_A")]
    LowOrderComponentA,
    #[serde(rename = "low_order_component_R")]
    LowOrderComponentR,
    #[serde(rename = "low_order_residue")]
    LowOrderResidue,
    #[serde(rename = "reencoded_k")]
    ReencodedK,
}

/// This is an intermediate representation between JSON and TestVector
#[derive(Deserialize)]
struct IntermediateTestVector {
    number: usize,
    #[serde(deserialize_with = "bytes_from_hex", rename = "key")]
    pubkey: Vec<u8>,
    #[serde(deserialize_with = "bytes_from_hex")]
    sig: Vec<u8>,
    msg: String,
    flags: Option<Set<Flag>>,
}

/// The test vector struct from [CCTV][]. `sig` may or may not be a valid signature of `msg` with
/// respect to `pubkey`, depending on the verification function's validation criteria. `flags`
/// describes all the edge cases which this test vector falls into.
///
/// [CCTV]: https://github.com/C2SP/CCTV/tree/5ea85644bd035c555900a2f707f7e4c31ea65ced/ed25519vectors
struct TestVector {
    number: usize,
    pubkey: VerifyingKey,
    sig: Signature,
    msg: Vec<u8>,
    flags: Set<Flag>,
}

impl From<IntermediateTestVector> for TestVector {
    fn from(tv: IntermediateTestVector) -> Self {
        let number = tv.number;
        let pubkey = {
            let mut buf = [0u8; 32];
            buf.copy_from_slice(&tv.pubkey);
            VerifyingKey::from_bytes(&buf).unwrap()
        };
        let sig = {
            let mut buf = [0u8; 64];
            buf.copy_from_slice(&tv.sig);
            Signature::from_bytes(&buf)
        };
        let msg = tv.msg.as_bytes().to_vec();

        // Unwrap the Option<Set<Flag>>
        let flags = tv.flags.unwrap_or_default();

        Self {
            number,
            pubkey,
            sig,
            msg,
            flags,
        }
    }
}

// Tells serde how to deserialize bytes from hex
fn bytes_from_hex<'de, D>(deserializer: D) -> Result<Vec<u8>, D::Error>
where
    D: Deserializer<'de>,
{
    let mut hex_str = String::deserialize(deserializer)?;
    // Prepend a 0 if it's not even length
    if hex_str.len() % 2 == 1 {
        hex_str.insert(0, '0');
    }
    hex::decode(hex_str).map_err(|e| SError::custom(format!("{:?}", e)))
}

fn get_test_vectors() -> impl Iterator<Item = TestVector> {
    let f = File::open("VALIDATIONVECTORS").expect(
        "This test is only available when the code has been cloned from the git repository, since
        the VALIDATIONVECTORS file is large and is therefore not included within the distributed \
        crate.",
    );

    serde_json::from_reader::<_, Vec<IntermediateTestVector>>(f)
        .unwrap()
        .into_iter()
        .map(TestVector::from)
}

/// Tests that the verify() and verify_strict() functions succeed only on test cases whose flags
/// (i.e., edge cases it falls into) are a subset of VERIFY_ALLOWED_EDGECASES and
/// VERIFY_STRICT_ALLOWED_EDGECASES, respectively
#[test]
fn check_validation_criteria() {
    let verify_allowed_edgecases = Set::from_iter(VERIFY_ALLOWED_EDGECASES.to_vec());
    let verify_strict_allowed_edgecases = Set::from_iter(VERIFY_STRICT_ALLOWED_EDGECASES.to_vec());

    for TestVector {
        number,
        pubkey,
        msg,
        sig,
        flags,
    } in get_test_vectors()
    {
        // If all the verify-permitted flags here are ones we permit, then verify() should succeed.
        // Otherwise, it should not.
        let success = pubkey.verify(&msg, &sig).is_ok();
        if flags.is_subset(&verify_allowed_edgecases) {
            assert!(success, "verify() expected success in testcase #{number}",);
        } else {
            assert!(!success, "verify() expected failure in testcase #{number}",);
        }

        // If all the verify_strict-permitted flags here are ones we permit, then verify_strict()
        // should succeed. Otherwise, it should not.
        let success = pubkey.verify_strict(&msg, &sig).is_ok();
        if flags.is_subset(&verify_strict_allowed_edgecases) {
            assert!(
                success,
                "verify_strict() expected success in testcase #{number}",
            );
        } else {
            assert!(
                !success,
                "verify_strict() expected failure in testcase #{number}",
            );
        }
    }
}

/// Prints the flags that are consistently permitted by verify() and verify_strict()
#[test]
fn find_validation_criteria() {
    let mut verify_allowed_edgecases = Set::new();
    let mut verify_strict_allowed_edgecases = Set::new();

    // Counts the number of times a signature with a re-encoded k and a low-order A verified. This
    // happens with 1/8 probability, assuming the usual verification equation(s).
    let mut num_lucky_reencoded_k = 0;
    let mut num_reencoded_k = 0;

    for TestVector {
        number: _,
        pubkey,
        msg,
        sig,
        flags,
    } in get_test_vectors()
    {
        // If verify() was a success, add all the associated flags to verify-permitted set
        let success = pubkey.verify(&msg, &sig).is_ok();

        // If this is ReencodedK && LowOrderA, log some statistics
        if flags.contains(&Flag::ReencodedK) && flags.contains(&Flag::LowOrderA) {
            num_reencoded_k += 1;
            num_lucky_reencoded_k += success as u8;
        }

        if success {
            for flag in &flags {
                // Don't count re-encoded k when A is low-order. This is because the
                // re-encoded k might be a multiple of 8 by accident
                if *flag == Flag::ReencodedK && flags.contains(&Flag::LowOrderA) {
                    continue;
                } else {
                    verify_allowed_edgecases.insert(*flag);
                }
            }
        }

        // If verify_strict() was a success, add all the associated flags to
        // verify_strict-permitted set
        let success = pubkey.verify_strict(&msg, &sig).is_ok();
        if success {
            for flag in &flags {
                verify_strict_allowed_edgecases.insert(*flag);
            }
        }
    }

    println!("VERIFY_ALLOWED_EDGECASES: {:?}", verify_allowed_edgecases);
    println!(
        "VERIFY_STRICT_ALLOWED_EDGECASES: {:?}",
        verify_strict_allowed_edgecases
    );
    println!(
        "re-encoded k && low-order A yielded a valid signature {}/{} of the time",
        num_lucky_reencoded_k, num_reencoded_k
    );
}
