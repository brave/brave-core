#![cfg(all(feature = "v2", feature = "std"))]

use cvss::v2::Vector;
use std::{fs, str::FromStr};

#[test]
fn cvss_v2_simple() {
    run_tests_from_file("vectors_simple2");
}

#[test]
fn cvss_v2_random() {
    run_tests_from_file("vectors_random2");
}

#[test]
fn cvss_v2_calculator() {
    run_tests_from_file("vectors_calculator2");
}

#[test]
fn cvss_v2_cvsslib() {
    run_tests_from_file("vectors_cvsslib2");
}

// Run the test set from Red Hat's Security Python implementation: https://github.com/RedHatProductSecurity/cvss
// Every line carries a "(base, temporal, environmental)" triple; all three scores are
// checked. The Python implementation reports None when a vector carries no metrics of the
// respective group, so those entries assert nothing about the group.
fn run_tests_from_file(name: &str) {
    let content = fs::read_to_string(format!("tests/cvss-redhat/tests/{}", name)).unwrap();
    for l in content.lines() {
        let parts = l.split(" - ").collect::<Vec<&str>>();
        let vector = parts[0];
        let scores = parts[1]
            .trim_start_matches('(')
            .trim_end_matches(')')
            .split(',')
            .map(|s| {
                let s = s.trim();
                (s != "None").then(|| s.parse::<f64>().unwrap())
            })
            .collect::<Vec<Option<f64>>>();
        let [base, temporal, environmental] = scores[..] else {
            panic!("malformed score triple: {}", l);
        };
        let base = base.expect("the base score is always present");

        let cvss = Vector::from_str(vector).unwrap();
        // Test correct serialization.
        assert_eq!(cvss.to_string(), parts[0]);
        assert!(cvss.score().value() >= 0.0);
        assert!(cvss.score().value() <= 10.0);
        assert!(
            (cvss.score().value() - base).abs() < 0.0001,
            "base {} for {}",
            cvss.score().value(),
            vector
        );
        if let Some(temporal) = temporal {
            assert!(
                (cvss.temporal_score().value() - temporal).abs() < 0.0001,
                "temporal {} for {}",
                cvss.temporal_score().value(),
                vector
            );
        }
        if let Some(environmental) = environmental {
            assert!(
                (cvss.environmental_score().value() - environmental).abs() < 0.0001,
                "environmental {} for {}",
                cvss.environmental_score().value(),
                vector
            );
        }
    }
}
