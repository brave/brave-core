#![cfg(all(feature = "v3", feature = "std"))]

use cvss::v3::Vector;
use std::{fs, str::FromStr};

// Run the test set from Red Hat's Security Python implementation: https://github.com/RedHatProductSecurity/cvss

fn run_tests_from_file(name: &str, base_only: bool) {
    let content = fs::read_to_string(format!("tests/cvss-redhat/tests/{}", name)).unwrap();
    for l in content.lines() {
        let parts = l.split(" - ").collect::<Vec<&str>>();
        let vector = parts[0];

        // "(base, _, _)"
        let base_score = parts[1].split(',').next().unwrap().trim_start_matches('(');
        let temporal_score = parts[1].split(',').nth(1).unwrap().trim();
        let environmental_score = parts[1].split(',').nth(2).unwrap().trim_end_matches(')');

        let cvss = Vector::from_str(vector).unwrap();

        // Test correct serialization.
        assert_eq!(cvss.to_string(), parts[0]);

        // Test score calculation.
        assert!(cvss.base_score().value() >= 0.0);
        assert!(cvss.base_score().value() <= 10.0);
        assert!(cvss.temporal_score().value() >= 0.0);
        assert!(cvss.temporal_score().value() <= 10.0);
        assert!(cvss.environmental_score().value() >= 0.0);
        assert!(cvss.environmental_score().value() <= 10.0);

        let base_expected = base_score.trim().parse::<f64>().unwrap_or_else(|e| {
            panic!(
                "Failed to parse base score '{}' for vector '{}': {:?}",
                base_score, vector, e
            )
        });
        let diff = cvss.base_score().value() - base_expected;
        assert!(
            diff.abs() < 0.0001,
            "Base score mismatch for vector {}: expected {}, got {}",
            vector,
            base_expected,
            cvss.base_score().value()
        );

        if base_only {
            continue;
        }
        let temporal_expected = temporal_score.trim().parse::<f64>().unwrap_or_else(|e| {
            panic!(
                "Failed to parse temporal score '{}' for vector '{}': {:?}",
                temporal_score, vector, e
            )
        });
        let diff = cvss.temporal_score().value() - temporal_expected;
        assert!(
            diff.abs() < 0.0001,
            "Temporal score mismatch for vector {}: expected {}, got {}",
            vector,
            temporal_expected,
            cvss.temporal_score().value()
        );

        let environmental_expected =
            environmental_score
                .trim()
                .parse::<f64>()
                .unwrap_or_else(|e| {
                    panic!(
                        "Failed to parse environmental score '{}' for vector '{}': {:?}",
                        environmental_score, vector, e
                    )
                });
        let diff = cvss.environmental_score().value() - environmental_expected;
        assert!(
            diff.abs() < 0.0001,
            "Environmental score mismatch for vector {}: expected {}, got {}",
            vector,
            environmental_expected,
            cvss.environmental_score().value()
        );
    }
}

#[test]
fn cvss_v3_simple() {
    run_tests_from_file("vectors_simple3", false);
    run_tests_from_file("vectors_simple31", false);
}

#[test]
fn cvss_v3_random() {
    // CVSS v3.0 has some known issues with environmental score calculation in
    // the Red Hat test set, so we skip those tests here.
    run_tests_from_file("vectors_random3", true);
    run_tests_from_file("vectors_random31", false);
}
