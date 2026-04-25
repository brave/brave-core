//! Integration test against the live `advisory-db` repo on GitHub
#![cfg(feature = "git")]
#![warn(rust_2018_idioms, unused_qualifications)]

use std::time::Duration;

use rustsec::{
    Collection, Database, Lockfile, VersionReq, advisory, database::Query, repository::git,
};
use tempfile::tempdir;

/// Happy path integration test (has online dependency on GitHub)
#[test]
fn happy_path() {
    let db = Database::load_from_repo(&git::Repository::fetch_default_repo().unwrap()).unwrap();
    verify_rustsec_2017_0001(&db);
    verify_cve_2018_1000810(&db);
}

/// End-to-end integration test (has online dependency on GitHub) which looks
/// for the `RUSTSEC-2017-0001` vulnerability (`sodiumoxide` crate).
fn verify_rustsec_2017_0001(db: &Database) {
    let example_advisory_id = "RUSTSEC-2017-0001".parse::<advisory::Id>().unwrap();
    let example_advisory = db.get(&example_advisory_id).unwrap();
    let example_package = "sodiumoxide".parse().unwrap();

    assert_eq!(example_advisory.metadata.id, example_advisory_id);
    assert_eq!(example_advisory.metadata.package, example_package);
    assert_eq!(
        example_advisory.versions.patched()[0],
        VersionReq::parse(">= 0.0.14").unwrap()
    );
    assert_eq!(example_advisory.metadata.date.as_str(), "2017-01-26");
    assert_eq!(
        example_advisory.metadata.url.as_ref().unwrap().to_string(),
        "https://github.com/dnaq/sodiumoxide/issues/154"
    );
    assert_eq!(
        example_advisory.title(),
        "scalarmult() vulnerable to degenerate public keys"
    );
    assert_eq!(
        &example_advisory.description()[0..30],
        "The `scalarmult()` function in"
    );
    assert_eq!(
        example_advisory.metadata.collection.unwrap(),
        Collection::Crates
    );

    let crate_advisories = db.query(&Query::new().package_name(example_package).year(2017));
    assert_eq!(example_advisory, crate_advisories[0]);

    let lockfile = Lockfile::load("../Cargo.lock").unwrap();
    let _vulns = db.vulnerabilities(&lockfile);
}

/// End-to-end integration test (has online dependency on GitHub) which looks
/// for the `CVE-2018-1000810` vulnerability (`std::str::repeat`)
fn verify_cve_2018_1000810(db: &Database) {
    let example_advisory_id = "CVE-2018-1000810".parse::<advisory::Id>().unwrap();
    let example_advisory = db.get(&example_advisory_id).unwrap();
    let example_package = "std".parse().unwrap();

    assert_eq!(example_advisory.metadata.id, example_advisory_id);
    assert_eq!(example_advisory.metadata.package, example_package);
    assert_eq!(
        example_advisory.versions.patched()[0],
        VersionReq::parse(">= 1.29.1").unwrap()
    );
    assert_eq!(example_advisory.metadata.date.as_str(), "2018-09-21");
    assert_eq!(
        example_advisory.metadata.url.as_ref().unwrap().to_string(),
        "https://groups.google.com/forum/#!topic/rustlang-security-announcements/CmSuTm-SaU0"
    );
    assert_eq!(
        example_advisory.title(),
        "Buffer overflow vulnerability in str::repeat()"
    );
    assert_eq!(
        &example_advisory.description()[0..30],
        "The Rust team was recently not"
    );
    assert_eq!(
        example_advisory.metadata.collection.unwrap(),
        Collection::Rust
    );
}

/// Regression test for cloning into an existing directory
#[test]
fn clone_into_existing_directory() {
    // Make an empty temporary directory
    let tmp = tempdir().unwrap();

    // Attempt to fetch into it
    git::Repository::fetch(
        git::DEFAULT_URL,
        tmp.path(),
        true,
        Duration::from_secs(5 * 60),
    )
    .unwrap();
}
