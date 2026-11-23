#![cfg(all( // require `--all-features` to be passed
    feature = "default",
    feature = "alloc",
    feature = "formatting",
    feature = "large-dates",
    feature = "local-offset",
    feature = "macros",
    feature = "parsing",
    feature = "quickcheck",
    feature = "serde-human-readable",
    feature = "serde-well-known",
    feature = "std",
    feature = "rand",
    feature = "serde",
))]
#![allow(
    clippy::cognitive_complexity,
    reason = "many tests in one function is okay"
)]
#![allow(clippy::std_instead_of_core, reason = "irrelevant for tests")]

//! Tests for internal details.
//!
//! This module should only be used when it is not possible to test the implementation in a
//! reasonable manner externally.

use std::format;

use rstest::rstest;

use crate::ext::DigitCount;
use crate::parsing::combinator::rfc::iso8601;
use crate::{format_description, parsing, signed_duration};

#[rstest]
#[case(1, 1)]
#[case(9, 1)]
#[case(10, 2)]
#[case(99, 2)]
#[case(100, 3)]
fn digit_count_u8(#[case] input: u8, #[case] expected: u8) {
    assert_eq!(input.num_digits(), expected);
}

#[rstest]
#[case(1, 1)]
#[case(9, 1)]
#[case(10, 2)]
#[case(99, 2)]
#[case(100, 3)]
#[case(999, 3)]
#[case(1_000, 4)]
#[case(9_999, 4)]
#[case(10_000, 5)]
fn digit_count_u16(#[case] input: u16, #[case] expected: u8) {
    assert_eq!(input.num_digits(), expected);
}

#[rstest]
#[case(1, 1)]
#[case(9, 1)]
#[case(10, 2)]
#[case(99, 2)]
#[case(100, 3)]
#[case(999_999, 6)]
#[case(1_000_000, 7)]
#[case(9_999_999, 7)]
#[case(10_000_000, 8)]
#[case(99_999_999, 8)]
#[case(100_000_000, 9)]
#[case(999_999_999, 9)]
#[case(1_000_000_000, 10)]
fn digit_count_u32(#[case] input: u32, #[case] expected: u8) {
    assert_eq!(input.num_digits(), expected);
}

#[rstest]
#[case(signed_duration::Padding::Optimize)]
#[case(parsing::ParsedItem(b"", 0))]
#[case(format_description::Period::Am)]
#[case(iso8601::ExtendedKind::Basic)]
fn debug(#[case] input: impl std::fmt::Debug) {
    drop(format!("{input:?}"));
}

#[rstest]
#[case(format_description::Period::Am)]
#[case(crate::time::Padding::Optimize)]
fn clone(#[case] input: impl Clone + PartialEq) {
    assert!(input.clone() == input);
}

#[rstest]
#[expect(clippy::clone_on_copy, reason = "purpose of the test")]
fn clone_matches() {
    assert!(matches!(
        iso8601::ExtendedKind::Basic.clone(),
        iso8601::ExtendedKind::Basic
    ));
}

#[rstest]
fn parsing_internals() {
    assert!(
        parsing::ParsedItem(b"", ())
            .flat_map(|_| None::<()>)
            .is_none()
    );
}
