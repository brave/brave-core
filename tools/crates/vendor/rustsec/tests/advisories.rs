//! Tests for parsing RustSec advisories

#![warn(rust_2018_idioms, unused_qualifications)]

use cvss::Cvss;
use rustsec::advisory::{Category, License};
use std::path::Path;

/// Load example advisory from the filesystem
fn load_advisory(case: &str) -> rustsec::Advisory {
    rustsec::Advisory::load_file(Path::new(&format!(
        "./tests/support/example_advisory_{case}.md"
    )))
    .unwrap()
}

/// Basic metadata
#[test]
fn parse_metadata() {
    for advisory in &[load_advisory("v3"), load_advisory("v4")] {
        assert_eq!(advisory.metadata.id.as_str(), "RUSTSEC-2001-2101");
        assert_eq!(advisory.metadata.package.as_str(), "base");
        assert_eq!(advisory.title(), "All your base are belong to us");
        assert_eq!(
            advisory.description(),
            "You have no chance to survive. Make your time."
        );
        assert_eq!(advisory.metadata.date.as_str(), "2001-02-03");
        assert_eq!(
            advisory.metadata.url.as_ref().unwrap().to_string(),
            "https://www.youtube.com/watch?v=jQE66WA2s-A"
        );

        for (i, category) in [Category::CodeExecution, Category::PrivilegeEscalation]
            .iter()
            .enumerate()
        {
            assert_eq!(*category, advisory.metadata.categories[i]);
        }

        for (i, kw) in ["how", "are", "you", "gentlemen"].iter().enumerate() {
            assert_eq!(*kw, advisory.metadata.keywords[i].as_str());
        }

        assert_eq!(advisory.metadata.license, License::CcZero10);
    }

    // Test fields specific to advisories imported from GitHub
    let ghsa = load_advisory("v4_from_ghsa");
    assert_eq!(ghsa.metadata.license, License::CcBy40);

    // Test advisory with unknown license
    let ghsa = load_advisory("v4_unknown_license");
    assert_eq!(ghsa.metadata.license, License::Other("MPL-2.0".to_string()));
}

/// Parsing of impact metadata
#[test]
fn parse_affected() {
    let affected = load_advisory("v3").affected.unwrap();
    assert_eq!(affected.arch[0], platforms::target::Arch::X86);
    assert_eq!(affected.os[0], platforms::target::OS::Windows);

    let example_function = "base::belongs::All".parse().unwrap();
    let req = &affected.functions.get(&example_function).unwrap()[0];
    assert!(req.matches(&"1.2.2".parse().unwrap()));
    assert!(!req.matches(&"1.2.3".parse().unwrap()));
}

/// Parsing of other aliased advisory IDs
#[test]
fn parse_aliases() {
    let alias = &load_advisory("v3").metadata.aliases[0];
    assert!(alias.is_cve());
    assert_eq!(alias.year().unwrap(), 2001);
}

/// Parsing of CVSS v3.1 severity vector strings
#[test]
fn parse_cvss_vector_string() {
    let advisory = load_advisory("v3");
    assert_eq!(
        advisory.severity().unwrap(),
        rustsec::advisory::Severity::Critical
    );

    let Cvss::CvssV31(cvss) = advisory.metadata.cvss.unwrap() else {
        panic!("expected CVSS v3.1");
    };

    assert_eq!(cvss.av.unwrap(), cvss::v3::base::AttackVector::Network);
    assert_eq!(cvss.ac.unwrap(), cvss::v3::base::AttackComplexity::Low);
    assert_eq!(cvss.pr.unwrap(), cvss::v3::base::PrivilegesRequired::None);
    assert_eq!(cvss.ui.unwrap(), cvss::v3::base::UserInteraction::None);
    assert_eq!(cvss.s.unwrap(), cvss::v3::base::Scope::Changed);
    assert_eq!(cvss.c.unwrap(), cvss::v3::base::Confidentiality::High);
    assert_eq!(cvss.i.unwrap(), cvss::v3::base::Integrity::High);
    assert_eq!(cvss.a.unwrap(), cvss::v3::base::Availability::High);
    assert_eq!(cvss.score().value(), 10.0);
}

/// Parsing of patched version reqs
#[test]
fn parse_patched_version_reqs() {
    let advisory = load_advisory("v3");
    let req = &advisory.versions.patched()[0];
    assert!(!req.matches(&"1.2.2".parse().unwrap()));
    assert!(req.matches(&"1.2.3".parse().unwrap()));
    assert!(req.matches(&"1.2.4".parse().unwrap()));
}
