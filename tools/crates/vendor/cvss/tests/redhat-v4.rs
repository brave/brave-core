#![cfg(all(feature = "v4", feature = "std"))]

use cvss::v4::Vector;
use std::{fs, str::FromStr};

// We run the test set from Red Hat's Security Python implementation: https://github.com/RedHatProductSecurity/cvss
// It seems to be the best test set available (at least for CVSS v4.0).

fn run_tests_from_file(name: &str, test_serialization: bool) {
    let content = fs::read_to_string(format!("tests/cvss-redhat/tests/{}", name)).unwrap();
    for l in content.lines() {
        let parts = l.split(" - ").collect::<Vec<&str>>();
        let cvss = Vector::from_str(parts[0]).unwrap();
        if test_serialization {
            // Test correct serialization.
            assert_eq!(cvss.to_string(), parts[0]);
        }
        assert!(cvss.score().value() >= 0.0);
        assert!(cvss.score().value() <= 10.0);
        let diff: f64 = cvss.score().value() - parts[1].parse::<f64>().unwrap();
        assert!(diff.abs() < 0.0001);
    }
}

#[test]
fn cvss_v4_base() {
    // All vector combinations with only mandatory fields, 104,976 vectors.
    run_tests_from_file("vectors_base4", true);
}

#[test]
fn cvss_v4_modified() {
    // All vector combinations of modified environmental fields, 373,248 vectors.
    run_tests_from_file("vectors_modified4", true);
}

#[test]
fn cvss_v4_supplemental() {
    // All vector combinations of supplemental fields, 576 vectors.
    run_tests_from_file("vectors_supplemental4", true);
}

#[test]
fn cvss_v4_security() {
    // All vector combinations of security fields, 54 vectors.
    run_tests_from_file("vectors_security4", true);
}

#[test]
fn cvss_v4_threat() {
    // All vector combinations of threat fields, 6 vectors.
    run_tests_from_file("vectors_threat4", true);
}

#[test]
fn cvss_v4_random() {
    // Random vector combinations across all fields, 10,000 vectors.
    run_tests_from_file("vectors_random4", false);
}
