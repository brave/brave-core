//! Linter tests

#![warn(rust_2018_idioms, unused_qualifications)]

use rustsec::advisory::Linter;

/// Example RustSec Advisory
const EXAMPLE_ADVISORY_PATH: &str = "./tests/support/example_advisory_v3.md";

/// Ensure example advisory passes lint
#[test]
fn valid_advisory() {
    let lint = Linter::lint_file(EXAMPLE_ADVISORY_PATH).unwrap();
    assert_eq!(lint.errors(), &[]);
}

/// Example advisory used in the subsequent `#[test]`
const INVALID_ADVISORY_MD: &str = r#"```toml
[advisory]
id = "LULZSEC-2001-2101"
package = "base"
collection = "crates"
date = "2001-02-03"
url = "ftp://www.youtube.com/watch?v=jQE66WA2s-A"
categories = ["invalid-category"]
keywords = ["how", "are", "you", "gentlemen"]
aliases = ["CVE-2001-2101"]
cvss = "CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:C/C:H/I:H/A:H"
invalid-advisory-key = "invalid"

[versions]
patched = [">= 1.2.3"]

[affected]
arch = ["x86"]
os = ["windows"]
functions = { "notyourbase::belongs::All" = ["< 1.2.3"] }

[invalid-section]
```

# All your base are belong to us

You have no chance to survive. Make your time.

"#;

/// Advisory which fails lint for multiple msgs
#[test]
fn invalid_example() {
    let lint = Linter::lint_string(INVALID_ADVISORY_MD, Some(false)).unwrap();

    // Do we get the expected number of errors?
    assert_eq!(lint.errors().len(), 7);

    // `invalid-category`
    let invalid_category = lint.errors()[0].to_string();
    assert_eq!(
        invalid_category,
        "invalid value `invalid-category` for key `category` in [advisory]: unknown category \"invalid-category\""
    );

    // explicit `collection` is disallowed
    let explicit_collection = lint.errors()[1].to_string();
    assert_eq!(
        explicit_collection,
        "malformed content in [advisory]: collection shouldn\'t be explicit; inferred by location"
    );

    // invalid advisory ID (LULZSEC)
    let invalid_advisory_id = lint.errors()[2].to_string();
    assert_eq!(
        invalid_advisory_id,
        "invalid value `\"LULZSEC-2001-2101\"` for key `id` in [advisory]: unknown advisory ID type"
    );

    // `invalid-advisory-key`
    let invalid_advisory_key = lint.errors()[3].to_string();
    assert_eq!(
        invalid_advisory_key,
        "invalid key `invalid-advisory-key` in [advisory]"
    );

    // invalid advisory URL (must start with https://)
    let invalid_advisory_url = lint.errors()[4].to_string();
    assert_eq!(
        invalid_advisory_url,
        "invalid value `\"ftp://www.youtube.com/watch?v=jQE66WA2s-A\"` \
         for key `url` in [advisory]: URL must start with https://"
    );

    // function path that doesn't match crate name
    let invalid_function_path = lint.errors()[5].to_string();
    assert_eq!(
        invalid_function_path,
        "invalid value `notyourbase::belongs::All` for key `functions` \
         in [affected]: function path must start with crate name"
    );

    // `invalid-section`
    let invalid_section = lint.errors()[6].to_string();
    assert_eq!(invalid_section, "invalid key `invalid-section` in toplevel");
}

/// A valid advisory with a real (non-placeholder) ID
const VALID_ADVISORY_MD: &str = include_str!("support/example_advisory_v3.md");

/// The advisory ID's placeholder status must match the draft flag
#[test]
fn draft_id_mismatch() {
    let placeholder = VALID_ADVISORY_MD.replace("RUSTSEC-2001-2101", "RUSTSEC-0000-0000");

    // Real ID flagged as a draft is an error
    let lint = Linter::lint_string(VALID_ADVISORY_MD, Some(true)).unwrap();
    assert_eq!(
        lint.errors()
            .iter()
            .map(ToString::to_string)
            .collect::<Vec<_>>(),
        vec![
            "invalid value `\"RUSTSEC-2001-2101\"` for key `id` in [advisory]: \
             advisory ID draft status does not match file name"
        ]
    );

    // Placeholder ID flagged as non-draft is an error
    let lint = Linter::lint_string(&placeholder, Some(false)).unwrap();
    assert_eq!(
        lint.errors()
            .iter()
            .map(ToString::to_string)
            .collect::<Vec<_>>(),
        vec![
            "invalid value `\"RUSTSEC-0000-0000\"` for key `id` in [advisory]: \
             advisory ID draft status does not match file name"
        ]
    );

    // Matching draft status (or no draft info) produces no error
    assert_eq!(
        Linter::lint_string(VALID_ADVISORY_MD, Some(false))
            .unwrap()
            .errors(),
        &[]
    );
    assert_eq!(
        Linter::lint_string(&placeholder, Some(true))
            .unwrap()
            .errors(),
        &[]
    );
    assert_eq!(
        Linter::lint_string(&placeholder, None).unwrap().errors(),
        &[]
    );
}

/// The `url` field must not point at a GHSA that is already listed in `aliases`
#[test]
fn url_is_ghsa_alias() {
    let advisory = VALID_ADVISORY_MD
        .replace(
            "https://www.youtube.com/watch?v=jQE66WA2s-A",
            "https://github.com/advisories/GHSA-f8vr-r385-rh5r",
        )
        .replace(
            "aliases = [\"CVE-2001-2101\"]",
            "aliases = [\"CVE-2001-2101\", \"GHSA-f8vr-r385-rh5r\"]",
        );

    let lint = Linter::lint_string(&advisory, Some(false)).unwrap();
    assert_eq!(
        lint.errors()
            .iter()
            .map(ToString::to_string)
            .collect::<Vec<_>>(),
        vec![
            "invalid value `\"https://github.com/advisories/GHSA-f8vr-r385-rh5r\"` for key `url` \
             in [advisory]: URL refers to GHSA alias GHSA-f8vr-r385-rh5r; refer to an issue, PR or commit instead"
        ]
    );

    // A GHSA URL without a matching alias is fine
    let no_alias = VALID_ADVISORY_MD.replace(
        "https://www.youtube.com/watch?v=jQE66WA2s-A",
        "https://github.com/advisories/GHSA-f8vr-r385-rh5r",
    );
    assert_eq!(
        Linter::lint_string(&no_alias, Some(false))
            .unwrap()
            .errors(),
        &[]
    );
}

/// The `references` field must not contain a GHSA that is already listed in `aliases`
#[test]
fn references_include_ghsa_alias() {
    let advisory = VALID_ADVISORY_MD
        .replace(
            "aliases = [\"CVE-2001-2101\"]",
            "aliases = [\"CVE-2001-2101\", \"GHSA-f8vr-r385-rh5r\"]\n\
             references = [\"https://example.com/\", \"https://github.com/advisories/GHSA-f8vr-r385-rh5r\"]",
        );

    let lint = Linter::lint_string(&advisory, Some(false)).unwrap();
    assert_eq!(
        lint.errors()
            .iter()
            .map(ToString::to_string)
            .collect::<Vec<_>>(),
        vec![
            "invalid value `\"https://github.com/advisories/GHSA-f8vr-r385-rh5r\"` for key `references` \
             in [advisory]: reference to GHSA alias GHSA-f8vr-r385-rh5r is redundant"
        ]
    );

    // A GHSA reference without a matching alias is fine
    let no_alias = VALID_ADVISORY_MD.replace(
        "aliases = [\"CVE-2001-2101\"]",
        "aliases = [\"CVE-2001-2101\"]\n\
         references = [\"https://github.com/advisories/GHSA-f8vr-r385-rh5r\"]",
    );
    assert_eq!(
        Linter::lint_string(&no_alias, Some(false))
            .unwrap()
            .errors(),
        &[]
    );
}
