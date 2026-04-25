#![cfg(all(feature = "v3", feature = "std"))]

use cvss::v3::Base;
use std::{fs, str::FromStr};

// Run the test set from Red Hat's Security Python implementation: https://github.com/RedHatProductSecurity/cvss

fn run_tests_from_file(name: &str) {
    let content = fs::read_to_string(format!("tests/cvss-redhat/tests/{}", name)).unwrap();
    for l in content.lines() {
        let parts = l.split(" - ").collect::<Vec<&str>>();
        let vector = parts[0];
        if vector.len() > 44 {
            // more than base, skip
            continue;
        }
        // "(base, _, _)"
        let score = parts[1].split(',').next().unwrap().trim_start_matches('(');

        let cvss = Base::from_str(vector).unwrap();
        // Test correct serialization.
        assert_eq!(cvss.to_string(), parts[0]);
        assert!(cvss.score().value() >= 0.0);
        assert!(cvss.score().value() <= 10.0);
        let diff: f64 = cvss.score().value() - score.parse::<f64>().unwrap();
        assert!(diff.abs() < 0.0001);
    }
}

#[test]
fn cvss_v3_simple() {
    run_tests_from_file("vectors_simple3");
    run_tests_from_file("vectors_simple31");
}

#[test]
fn cvss_v3_random() {
    run_tests_from_file("vectors_random3");
    run_tests_from_file("vectors_random31");
}
