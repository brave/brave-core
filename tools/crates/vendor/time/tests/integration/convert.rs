use rstest::rstest;
use time::unit::*;

#[rstest]
fn issue_749() {
    assert_eq!(Nanosecond::per(Second), 1_000_000_000u32);
}
