use rstest::rstest;
use rstest_reuse::{apply, template};
use time::Month::{self, *};
use time::iter::MonthIter;

#[template]
#[rstest]
fn all_months(
    #[values(
        January, February, March, April, May, June, July, August, September, October, November,
        December
    )]
    start: Month,
) {
}

#[apply(all_months)]
fn new(start: Month) {
    let mut iter = MonthIter::new(start);
    assert_eq!(iter.next(), Some(start));
    assert_eq!(iter.next(), Some(start.next()));
}

#[apply(all_months)]
fn iter_from(start: Month) {
    let mut iter = Month::iter_from(start);
    assert_eq!(iter.next(), Some(start));
    assert_eq!(iter.next(), Some(start.next()));
}

#[apply(all_months)]
fn size_hint(start: Month) {
    assert_eq!(MonthIter::new(start).size_hint(), (usize::MAX, None));
}

#[apply(all_months)]
fn max(start: Month) {
    assert_eq!(MonthIter::new(start).max(), Some(December));
}

#[apply(all_months)]
fn min(start: Month) {
    assert_eq!(MonthIter::new(start).min(), Some(January));
}

#[apply(all_months)]
fn is_sorted(start: Month) {
    assert!(!MonthIter::new(start).is_sorted());
}

#[apply(all_months)]
fn nth(start: Month, #[values(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)] n: u8) {
    let expected = start.nth_next(n);
    assert_eq!(MonthIter::new(start).nth(n as usize), Some(expected));
}

#[apply(all_months)]
fn any_checks_all_months(
    start: Month,
    #[values(
        January, February, March, April, May, June, July, August, September, October, November,
        December
    )]
    target: Month,
) {
    assert!(MonthIter::new(start).any(|m| m == target));
}

#[apply(all_months)]
fn any_terminates(start: Month) {
    assert!(!MonthIter::new(start).any(|_| false));
}

#[apply(all_months)]
fn all_checks_all_months(
    start: Month,
    #[values(
        January, February, March, April, May, June, July, August, September, October, November,
        December
    )]
    excludes: Month,
) {
    assert!(!MonthIter::new(start).all(|m| m != excludes));
}

#[apply(all_months)]
fn all_terminates(start: Month) {
    assert!(MonthIter::new(start).all(|_| true));
}

#[apply(all_months)]
fn rev_next(start: Month) {
    let mut iter = MonthIter::new(start).rev();
    assert_eq!(iter.next(), Some(start));
    assert_eq!(iter.next(), Some(start.previous()));
}

#[apply(all_months)]
fn rev_nth(start: Month, #[values(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)] n: u8) {
    let expected = start.nth_prev(n);
    assert_eq!(MonthIter::new(start).rev().nth(n as usize), Some(expected));
}

#[apply(all_months)]
fn rev_size_hint(start: Month) {
    assert_eq!(MonthIter::new(start).rev().size_hint(), (usize::MAX, None));
}

#[apply(all_months)]
fn rev_max(start: Month) {
    assert_eq!(MonthIter::new(start).rev().max(), Some(December));
}

#[apply(all_months)]
fn rev_min(start: Month) {
    assert_eq!(MonthIter::new(start).rev().min(), Some(January));
}

#[apply(all_months)]
fn rev_is_sorted(start: Month) {
    assert!(!MonthIter::new(start).rev().is_sorted());
}

#[apply(all_months)]
#[should_panic(expected = "infinite")]
fn count_panics(start: Month) {
    MonthIter::new(start).count();
}

#[apply(all_months)]
#[should_panic(expected = "infinite")]
fn last_panics(start: Month) {
    MonthIter::new(start).last();
}

#[apply(all_months)]
#[should_panic(expected = "infinite")]
fn rev_count_panics(start: Month) {
    MonthIter::new(start).rev().count();
}

#[apply(all_months)]
#[should_panic(expected = "infinite")]
fn rev_last_panics(start: Month) {
    MonthIter::new(start).rev().last();
}

#[apply(all_months)]
fn rev_any_checks_all_months(
    start: Month,
    #[values(
        January, February, March, April, May, June, July, August, September, October, November,
        December
    )]
    target: Month,
) {
    assert!(MonthIter::new(start).rev().any(|m| m == target));
}

#[apply(all_months)]
fn rev_any_terminates(start: Month) {
    assert!(!MonthIter::new(start).rev().any(|_| false));
}

#[apply(all_months)]
fn rev_all_checks_all_months(
    start: Month,
    #[values(
        January, February, March, April, May, June, July, August, September, October, November,
        December
    )]
    excludes: Month,
) {
    assert!(!MonthIter::new(start).rev().all(|m| m != excludes));
}

#[apply(all_months)]
fn rev_all_terminates(start: Month) {
    assert!(MonthIter::new(start).rev().all(|_| true));
}
