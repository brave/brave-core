#![cfg(any(feature = "ring", feature = "aws_lc_rs"))]

use core::time::Duration;
use std::collections::HashMap;
use std::fs::File;

use base64::{engine::general_purpose, Engine as _};
use bzip2::read::BzDecoder;
use pki_types::{ServerName, UnixTime};
use serde::Deserialize;

use webpki::types::{CertificateDer, SignatureVerificationAlgorithm, TrustAnchor};
use webpki::{anchor_from_trusted_cert, KeyUsage};

// All of the BetterTLS testcases use P256 keys.
static ALGS: &[&dyn SignatureVerificationAlgorithm] = &[
    #[cfg(feature = "ring")]
    webpki::ring::ECDSA_P256_SHA256,
    #[cfg(feature = "aws_lc_rs")]
    webpki::aws_lc_rs::ECDSA_P256_SHA256,
];

#[ignore] // Runs slower than other unit tests - opt-in with `cargo test -- --ignored`
#[test]
fn path_building() {
    let better_tls = testdata();
    let root_der = &better_tls.root_der();
    let root_der = CertificateDer::from(root_der.as_slice());
    let roots = &[anchor_from_trusted_cert(&root_der).expect("invalid trust anchor")];

    let suite = "pathbuilding";
    run_testsuite(
        suite,
        better_tls
            .suites
            .get(suite)
            .unwrap_or_else(|| panic!("missing {suite} suite")),
        roots,
    );
}

#[ignore] // Runs slower than other unit tests - opt-in with `cargo test -- --ignored`
#[test]
fn name_constraints() {
    let better_tls = testdata();
    let root_der = &better_tls.root_der();
    let root_der = CertificateDer::from(root_der.as_slice());
    let roots = &[anchor_from_trusted_cert(&root_der).expect("invalid trust anchor")];

    let suite = "nameconstraints";
    run_testsuite(
        suite,
        better_tls
            .suites
            .get(suite)
            .unwrap_or_else(|| panic!("missing {suite} suite")),
        roots,
    );
}

fn run_testsuite(suite_name: &str, suite: &BetterTlsSuite, roots: &[TrustAnchor]) {
    for testcase in &suite.test_cases {
        println!("Testing {suite_name} test case {}", testcase.id);

        let certs_der = testcase.certs_der();
        let ee_der = CertificateDer::from(certs_der[0].as_slice());
        let intermediates = &certs_der[1..]
            .iter()
            .map(|cert| CertificateDer::from(cert.as_slice()))
            .collect::<Vec<_>>();

        let ee_cert = webpki::EndEntityCert::try_from(&ee_der).expect("invalid end entity cert");

        // Set the time to the time of test case generation. This ensures that the test case
        // certificates won't expire.
        let now = UnixTime::since_unix_epoch(Duration::from_secs(1_691_788_832));

        let result = ee_cert
            .verify_for_usage(
                ALGS,
                roots,
                intermediates,
                now,
                KeyUsage::server_auth(),
                None,
                None,
            )
            .and_then(|_| {
                ee_cert.verify_is_valid_for_subject_name(
                    &ServerName::try_from(testcase.hostname.as_str())
                        .expect("invalid testcase hostname"),
                )
            });

        match testcase.expected {
            ExpectedResult::Accept => assert!(result.is_ok(), "expected success, got {:?}", result),
            ExpectedResult::Reject => {
                assert!(result.is_err(), "expected failure, got {:?}", result)
            }
        }
    }
}

fn testdata() -> BetterTls {
    let mut data_file = File::open("third-party/bettertls/bettertls.tests.json.bz2")
        .expect("failed to open data file");
    let decompressor = BzDecoder::new(&mut data_file);

    let better_tls: BetterTls = serde_json::from_reader(decompressor).expect("invalid test JSON");
    println!("Testing BetterTLS revision {:?}", better_tls.revision);

    better_tls
}

#[derive(Deserialize, Debug)]
struct BetterTls {
    #[serde(rename(deserialize = "betterTlsRevision"))]
    revision: String,
    #[serde(rename(deserialize = "trustRoot"))]
    root: String,
    suites: HashMap<String, BetterTlsSuite>,
}

impl BetterTls {
    fn root_der(&self) -> Vec<u8> {
        general_purpose::STANDARD
            .decode(&self.root)
            .expect("invalid trust anchor base64")
    }
}

#[derive(Deserialize, Debug)]
struct BetterTlsSuite {
    #[serde(rename(deserialize = "testCases"))]
    test_cases: Vec<BetterTlsTest>,
}

#[derive(Deserialize, Debug)]
struct BetterTlsTest {
    id: u32,
    certificates: Vec<String>,
    hostname: String,
    expected: ExpectedResult,
}

impl BetterTlsTest {
    fn certs_der(&self) -> Vec<Vec<u8>> {
        self.certificates
            .iter()
            .map(|cert| {
                general_purpose::STANDARD
                    .decode(cert)
                    .expect("invalid cert base64")
            })
            .collect()
    }
}

#[derive(Deserialize, Debug)]
#[serde(rename_all = "UPPERCASE")]
enum ExpectedResult {
    Accept,
    Reject,
}
