use rstest::rstest;
use rstest_reuse::{apply, template};
use time::Weekday::{self, *};
use time::iter::WeekdayIter;

#[template]
#[rstest]
fn all_weekdays(
    #[values(Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday)] start: Weekday,
) {
}

#[apply(all_weekdays)]
fn new(start: Weekday) {
    let mut iter = WeekdayIter::new(start);
    assert_eq!(iter.next(), Some(start));
    assert_eq!(iter.next(), Some(start.next()));
}

#[apply(all_weekdays)]
fn iter_from(start: Weekday) {
    let mut iter = Weekday::iter_from(start);
    assert_eq!(iter.next(), Some(start));
    assert_eq!(iter.next(), Some(start.next()));
}

#[apply(all_weekdays)]
fn size_hint(start: Weekday) {
    assert_eq!(WeekdayIter::new(start).size_hint(), (usize::MAX, None));
}

#[apply(all_weekdays)]
fn nth(start: Weekday, #[values(1, 2, 3, 4, 5, 6, 7)] n: u8) {
    let expected = start.nth_next(n);
    assert_eq!(WeekdayIter::new(start).nth(n as usize), Some(expected));
}

#[apply(all_weekdays)]
fn any_checks_all_weekdays(
    start: Weekday,
    #[values(Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday)] target: Weekday,
) {
    assert!(WeekdayIter::new(start).any(|d| d == target));
}

#[apply(all_weekdays)]
fn any_terminates(start: Weekday) {
    assert!(!WeekdayIter::new(start).any(|_| false));
}

#[apply(all_weekdays)]
fn all_checks_all_weekdays(
    start: Weekday,
    #[values(Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday)] excludes: Weekday,
) {
    assert!(!WeekdayIter::new(start).all(|d| d != excludes));
}

#[apply(all_weekdays)]
fn all_terminates(start: Weekday) {
    assert!(WeekdayIter::new(start).all(|_| true));
}

#[apply(all_weekdays)]
#[should_panic(expected = "infinite")]
fn count_panics(start: Weekday) {
    WeekdayIter::new(start).count();
}

#[apply(all_weekdays)]
#[should_panic(expected = "infinite")]
fn last_panics(start: Weekday) {
    WeekdayIter::new(start).last();
}

#[apply(all_weekdays)]
fn rev_next(start: Weekday) {
    let mut iter = WeekdayIter::new(start).rev();
    assert_eq!(iter.next(), Some(start));
    assert_eq!(iter.next(), Some(start.previous()));
}

#[apply(all_weekdays)]
fn rev_nth(start: Weekday, #[values(1, 2, 3, 4, 5, 6, 7)] n: u8) {
    let expected = start.nth_prev(n);
    assert_eq!(
        WeekdayIter::new(start).rev().nth(n as usize),
        Some(expected)
    );
}

#[apply(all_weekdays)]
fn rev_size_hint(start: Weekday) {
    assert_eq!(
        WeekdayIter::new(start).rev().size_hint(),
        (usize::MAX, None)
    );
}

#[apply(all_weekdays)]
#[should_panic(expected = "infinite")]
fn rev_count_panics(start: Weekday) {
    WeekdayIter::new(start).rev().count();
}

#[apply(all_weekdays)]
#[should_panic(expected = "infinite")]
fn rev_last_panics(start: Weekday) {
    WeekdayIter::new(start).rev().last();
}

#[apply(all_weekdays)]
fn rev_any_checks_all_weekdays(
    start: Weekday,
    #[values(Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday)] target: Weekday,
) {
    assert!(WeekdayIter::new(start).rev().any(|d| d == target));
}

#[apply(all_weekdays)]
fn rev_any_terminates(start: Weekday) {
    assert!(!WeekdayIter::new(start).rev().any(|_| false));
}

#[apply(all_weekdays)]
fn rev_all_checks_all_weekdays(
    start: Weekday,
    #[values(Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday)] excludes: Weekday,
) {
    assert!(!WeekdayIter::new(start).rev().all(|d| d != excludes));
}

#[apply(all_weekdays)]
fn rev_all_terminates(start: Weekday) {
    assert!(WeekdayIter::new(start).rev().all(|_| true));
}
