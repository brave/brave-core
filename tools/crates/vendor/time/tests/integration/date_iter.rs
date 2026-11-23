use std::vec::Vec;

use rstest::rstest;
use time::Date;
use time::iter::DateIter;
use time::macros::date;

const AFTER_MIN: Date = Date::MIN.next_day().unwrap();
const BEFORE_MAX: Date = Date::MAX.previous_day().unwrap();

#[rstest]
#[case(
    date!(2019-01-01),
    date!(2019-01-03),
    vec![date!(2019-01-01), date!(2019-01-02), date!(2019-01-03)]
)]
#[case(date!(2019-01-01), date!(2019-01-01), vec![date!(2019-01-01)])]
#[case(date!(2019-01-03), date!(2019-01-01), vec![])]
#[case(
    date!(2019-12-30),
    date!(2020-01-02),
    vec![date!(2019-12-30), date!(2019-12-31), date!(2020-01-01), date!(2020-01-02)]
)]
#[case(Date::MIN, AFTER_MIN, vec![Date::MIN, AFTER_MIN])]
#[case(BEFORE_MAX, Date::MAX, vec![BEFORE_MAX, Date::MAX])]
fn forward(#[case] start: Date, #[case] end: Date, #[case] expected: Vec<Date>) {
    let results: Vec<_> = start.iter_to(end).collect();
    assert_eq!(results, expected);
}

#[rstest]
fn double_ended() {
    let mut iter = date!(2019-01-01).iter_to(date!(2019-01-05));
    assert_eq!(iter.next(), Some(date!(2019-01-01)));
    assert_eq!(iter.next_back(), Some(date!(2019-01-05)));
    assert_eq!(iter.next_back(), Some(date!(2019-01-04)));
    assert_eq!(iter.next(), Some(date!(2019-01-02)));
    assert_eq!(iter.next(), Some(date!(2019-01-03)));
    assert_eq!(iter.next(), None);
}

#[rstest]
fn double_ended_to_self() {
    let d = date!(2019-06-15);
    let mut iter = d.iter_to(d);
    assert_eq!(iter.next_back(), Some(d));
    assert_eq!(iter.next_back(), None);
    assert_eq!(iter.next(), None);
}

#[rstest]
fn new() {
    let mut iter = DateIter::new(date!(2020-01-01), date!(2020-01-03));
    assert_eq!(iter.next(), Some(date!(2020-01-01)));
    assert_eq!(iter.next(), Some(date!(2020-01-02)));
    assert_eq!(iter.next(), Some(date!(2020-01-03)));
    assert_eq!(iter.next(), None);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-05), (5, Some(5)))]
#[case(date!(2019-01-05), date!(2019-01-01), (0, Some(0)))]
#[case(date!(2019-01-01), date!(2019-01-01), (1, Some(1)))]
fn size_hint(#[case] start: Date, #[case] end: Date, #[case] expected: (usize, Option<usize>)) {
    assert_eq!(start.iter_to(end).size_hint(), expected);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-05), 5)]
#[case(date!(2019-01-01), date!(2019-01-01), 1)]
#[case(date!(2019-01-05), date!(2019-01-01), 0)]
#[case(date!(-9999-01-01), date!(-9999-12-31), 365)]
fn count(#[case] start: Date, #[case] end: Date, #[case] expected: usize) {
    assert_eq!(start.iter_to(end).count(), expected);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-05), Some(date!(2019-01-05)))]
#[case(date!(2019-01-01), date!(2019-01-01), Some(date!(2019-01-01)))]
#[case(date!(2019-01-05), date!(2019-01-01), None)]
fn last(#[case] start: Date, #[case] end: Date, #[case] expected: Option<Date>) {
    assert_eq!(start.iter_to(end).last(), expected);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-05), Some(date!(2019-01-05)))]
#[case(date!(2019-01-01), date!(2019-01-01), Some(date!(2019-01-01)))]
#[case(date!(2019-01-05), date!(2019-01-01), None)]
fn max(#[case] start: Date, #[case] end: Date, #[case] expected: Option<Date>) {
    assert_eq!(start.iter_to(end).max(), expected);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-05), Some(date!(2019-01-01)))]
#[case(date!(2019-01-01), date!(2019-01-01), Some(date!(2019-01-01)))]
#[case(date!(2019-01-05), date!(2019-01-01), None)]
fn min(#[case] start: Date, #[case] end: Date, #[case] expected: Option<Date>) {
    assert_eq!(start.iter_to(end).min(), expected);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-05), 0, Some(date!(2019-01-01)))]
#[case(date!(2019-01-01), date!(2019-01-05), 2, Some(date!(2019-01-03)))]
#[case(date!(2019-01-01), date!(2019-01-05), 4, Some(date!(2019-01-05)))]
#[case(date!(2019-01-01), date!(2019-01-05), 5, None)]
#[case(date!(2019-01-05), date!(2019-01-01), 0, None)]
#[case(Date::MAX, Date::MAX, 0, Some(Date::MAX))]
#[case(Date::MAX, Date::MAX, 1, None)]
#[case(BEFORE_MAX, Date::MAX, 1, Some(Date::MAX))]
#[case(BEFORE_MAX, Date::MAX, 2, None)]
#[case(date!(2019-12-30), date!(2020-01-01), 2, Some(date!(2020-01-01)))]
#[case(date!(2019-01-01), date!(2019-12-31), 364, Some(date!(2019-12-31)))]
#[case(date!(2020-01-01), date!(2020-12-31), 365, Some(date!(2020-12-31)))]
fn nth(#[case] start: Date, #[case] end: Date, #[case] n: usize, #[case] expected: Option<Date>) {
    let mut iter = start.iter_to(end);
    assert_eq!(iter.nth(n), expected);
    if (expected.is_none() || expected == Some(end)) && start <= end {
        assert_eq!(iter.next(), None);
    }
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-05), 0, Some(date!(2019-01-05)))]
#[case(date!(2019-01-01), date!(2019-01-05), 2, Some(date!(2019-01-03)))]
#[case(date!(2019-01-01), date!(2019-01-05), 4, Some(date!(2019-01-01)))]
#[case(date!(2019-01-01), date!(2019-01-05), 5, None)]
#[case(date!(2019-01-05), date!(2019-01-01), 0, None)]
#[case(Date::MIN, Date::MIN, 0, Some(Date::MIN))]
#[case(Date::MIN, Date::MIN, 1, None)]
#[case(Date::MIN, AFTER_MIN, 1, Some(Date::MIN))]
#[case(Date::MIN, AFTER_MIN, 2, None)]
#[case(date!(2019-12-30), date!(2020-01-01), 2, Some(date!(2019-12-30)))]
#[case(date!(2019-01-01), date!(2019-12-31), 364, Some(date!(2019-01-01)))]
#[case(date!(2020-01-01), date!(2020-12-31), 365, Some(date!(2020-01-01)))]
fn nth_back(
    #[case] start: Date,
    #[case] end: Date,
    #[case] n: usize,
    #[case] expected: Option<Date>,
) {
    let mut iter = start.iter_to(end);
    assert_eq!(iter.nth_back(n), expected);
    if (expected.is_none() || expected == Some(start)) && start <= end {
        assert_eq!(iter.next_back(), None);
    }
}

#[rstest]
fn nth_mixed() {
    let mut iter = date!(2019-01-01).iter_to(date!(2019-01-07));
    assert_eq!(iter.nth(2), Some(date!(2019-01-03)));
    assert_eq!(iter.nth_back(1), Some(date!(2019-01-06)));
    assert_eq!(iter.next(), Some(date!(2019-01-04)));
    assert_eq!(iter.next_back(), Some(date!(2019-01-05)));
    assert_eq!(iter.next(), None);
}

#[rstest]
fn nth_back_after_next() {
    let mut iter = date!(2019-01-01).iter_to(date!(2019-01-05));
    assert_eq!(iter.next(), Some(date!(2019-01-01)));
    assert_eq!(iter.nth_back(2), Some(date!(2019-01-03)));
    assert_eq!(iter.next(), Some(date!(2019-01-02)));
    assert_eq!(iter.next(), None);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-05), 5)]
#[case(date!(2019-01-01), date!(2019-01-01), 1)]
#[case(date!(2019-01-05), date!(2019-01-01), 0)]
fn len(#[case] start: Date, #[case] end: Date, #[case] expected: usize) {
    assert_eq!(start.iter_to(end).len(), expected);
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-05))]
#[case(date!(2019-01-01), date!(2019-01-01))]
#[case(date!(2019-01-05), date!(2019-01-01))]
fn is_sorted(#[case] start: Date, #[case] end: Date) {
    assert!(start.iter_to(end).is_sorted());
}

#[rstest]
#[case(date!(2019-01-01), date!(2019-01-03))]
#[case(date!(2019-01-05), date!(2019-01-01))]
#[case(Date::MIN, Date::MIN)]
#[case(Date::MAX, Date::MAX)]
#[case(Date::MIN, AFTER_MIN)]
#[case(BEFORE_MAX, Date::MAX)]
fn fused(#[case] start: Date, #[case] end: Date) {
    let mut iter = start.iter_to(end);
    while iter.next().is_some() {}
    assert_eq!(iter.next(), None);
    assert_eq!(iter.next(), None);
    assert_eq!(iter.next_back(), None);
}

#[rstest]
#[case(Date::MIN)]
#[case(Date::MAX)]
fn boundary_single(#[case] d: Date) {
    let mut iter = d.iter_to(d);
    assert_eq!(iter.next(), Some(d));
    assert_eq!(iter.next(), None);
}

#[rstest]
fn boundary_two() {
    let mut iter = Date::MIN.iter_to(AFTER_MIN);
    assert_eq!(iter.next(), Some(Date::MIN));
    assert_eq!(iter.next(), Some(AFTER_MIN));
    assert_eq!(iter.next(), None);
}

#[rstest]
fn len_after_consumption() {
    let mut iter = date!(2019-01-01).iter_to(date!(2019-01-05));
    assert_eq!(iter.len(), 5);
    iter.next();
    assert_eq!(iter.len(), 4);
    iter.next_back();
    assert_eq!(iter.len(), 3);
    iter.nth(1);
    assert_eq!(iter.len(), 1);
    iter.next();
    assert_eq!(iter.len(), 0);
}

#[rstest]
#[case(date!(2019-12-01), date!(2020-01-31), 15, date!(2019-12-16), date!(2019-12-17))]
#[case(date!(2020-12-01), date!(2021-01-31), 15, date!(2020-12-16), date!(2020-12-17))]
#[case(date!(2019-12-30), date!(2020-01-05), 2, date!(2020-01-01), date!(2020-01-02))]
#[case(date!(2019-01-01), date!(2025-01-01), 400, date!(2020-02-05), date!(2020-02-06))]
fn nth_cross_year(
    #[case] start: Date,
    #[case] end: Date,
    #[case] n: usize,
    #[case] expected: Date,
    #[case] next_expected: Date,
) {
    let mut iter = start.iter_to(end);
    assert_eq!(iter.nth(n), Some(expected));
    assert_eq!(iter.next(), Some(next_expected));
}

#[rstest]
#[case(date!(2019-12-30), date!(2020-01-10), 9, date!(2020-01-01), date!(2019-12-31))]
#[case(date!(2019-12-30), date!(2020-01-05), 5, date!(2019-12-31), date!(2019-12-30))]
fn nth_back_cross_year(
    #[case] start: Date,
    #[case] end: Date,
    #[case] n: usize,
    #[case] expected: Date,
    #[case] next_expected: Date,
) {
    let mut iter = start.iter_to(end);
    assert_eq!(iter.nth_back(n), Some(expected));
    assert_eq!(iter.next_back(), Some(next_expected));
}

#[rstest]
#[case(date!(2019-12-30), date!(2020-01-02), 4)]
fn cross_year_span(#[case] start: Date, #[case] end: Date, #[case] expected: usize) {
    assert_eq!(start.iter_to(end).count(), expected);
    assert_eq!(start.iter_to(end).size_hint(), (expected, Some(expected)));
    assert_eq!(start.iter_to(end).len(), expected);
}
