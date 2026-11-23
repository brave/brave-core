#![expect(deprecated)]

use std::cmp::Ordering;
use std::thread;
use std::time::Instant as StdInstant;

use rstest::rstest;
use time::ext::{NumericalDuration, NumericalStdDuration};
use time::{Instant, SignedDuration};

#[rstest]
fn elapsed() {
    let instant = Instant::now();
    thread::sleep(1.std_milliseconds());
    assert!(instant.elapsed() >= 1.milliseconds());
}

#[rstest]
#[case(0.seconds())]
#[case(5.seconds())]
#[case((-5).seconds())]
fn checked_add(#[case] duration: SignedDuration) {
    let now = Instant::now();
    assert_eq!(now.checked_add(duration), Some(now + duration));
}

#[rstest]
#[case(0.seconds())]
#[case(5.seconds())]
#[case((-5).seconds())]
fn checked_sub(#[case] duration: SignedDuration) {
    let now = Instant::now();
    assert_eq!(now.checked_sub(duration), Some(now - duration));
}

#[rstest]
fn into_inner() {
    let now = Instant::now();
    assert_eq!(now.into_inner(), now.0);
}

#[rstest]
fn std_from() {
    let now_time = Instant::now();
    let now_std = StdInstant::from(now_time);
    assert_eq!(now_time, now_std);
}

#[rstest]
fn from_std() {
    let now_std = StdInstant::now();
    let now_time = Instant::from(now_std);
    assert_eq!(now_time, now_std);
}

#[rstest]
#[case(0)]
#[case(1)]
fn sub(#[case] ms: u64) {
    let start = Instant::now();
    thread::sleep(ms.std_milliseconds());
    assert!(Instant::now() - start >= ms.cast_signed().milliseconds());
}

#[rstest]
#[case(0)]
#[case(1)]
fn sub_std(#[case] ms: u64) {
    let start = StdInstant::now();
    thread::sleep(ms.std_milliseconds());
    assert!(Instant::now() - start >= ms.cast_signed().milliseconds());
}

#[rstest]
#[case(0)]
#[case(1)]
fn std_sub(#[case] ms: u64) {
    let start = Instant::now();
    thread::sleep(ms.std_milliseconds());
    assert!(StdInstant::now() - start >= ms.cast_signed().milliseconds());
}

#[rstest]
#[case(0)]
#[case(1)]
fn add_duration(#[case] ms: u64) {
    let start = Instant::now();
    thread::sleep(ms.std_milliseconds());
    assert!(start + ms.cast_signed().milliseconds() <= Instant::now());
}

#[rstest]
#[case(0)]
#[case(1)]
fn std_add_duration(#[case] ms: u64) {
    let start = StdInstant::now();
    thread::sleep(ms.std_milliseconds());
    assert!(start + ms.cast_signed().milliseconds() <= StdInstant::now());
}

#[rstest]
#[case(0)]
#[case(1)]
fn add_std_duration(#[case] ms: u64) {
    let start = Instant::now();
    thread::sleep(ms.std_milliseconds());
    assert!(start + ms.std_milliseconds() <= Instant::now());
}

#[rstest]
#[case(0)]
#[case(1)]
fn add_assign_duration(#[case] ms: u64) {
    let mut start = Instant::now();
    thread::sleep(ms.std_milliseconds());
    start += ms.cast_signed().milliseconds();
    assert!(start <= Instant::now());
}

#[rstest]
#[case(0)]
#[case(1)]
fn std_add_assign_duration(#[case] ms: u64) {
    let mut start = StdInstant::now();
    thread::sleep(ms.std_milliseconds());
    start += ms.cast_signed().milliseconds();
    assert!(start <= StdInstant::now());
}

#[rstest]
#[case(0)]
#[case(1)]
fn add_assign_std_duration(#[case] ms: u64) {
    let mut start = Instant::now();
    thread::sleep(ms.std_milliseconds());
    start += ms.std_milliseconds();
    assert!(start <= Instant::now());
}

#[rstest]
#[case(0)]
#[case(100)]
fn sub_duration(#[case] ms: i64) {
    let instant = Instant::now();
    assert!(instant - ms.milliseconds() <= Instant::now());
    assert_eq!(instant - SignedDuration::ZERO, instant);
}

#[rstest]
#[case(0)]
#[case(100)]
fn std_sub_duration(#[case] ms: i64) {
    let instant = StdInstant::now();
    assert!(instant - ms.milliseconds() <= StdInstant::now());
}

#[rstest]
#[case(0)]
#[case(100)]
fn sub_std_duration(#[case] ms: u64) {
    let instant = Instant::now();
    assert!(instant - ms.std_milliseconds() <= Instant::now());
}

#[rstest]
#[case(0)]
#[case(100)]
fn sub_assign_duration(#[case] ms: i64) {
    let mut instant = Instant::now();
    instant -= ms.milliseconds();
    assert!(instant <= Instant::now());
}

#[rstest]
#[case(0)]
#[case(100)]
fn std_sub_assign_duration(#[case] ms: i64) {
    let mut instant = StdInstant::now();
    instant -= ms.milliseconds();
    assert!(instant <= StdInstant::now());
}

#[rstest]
#[case(0)]
#[case(100)]
fn sub_assign_std_duration(#[case] ms: u64) {
    let mut instant = Instant::now();
    instant -= ms.std_milliseconds();
    assert!(instant <= Instant::now());
}

#[rstest]
fn eq_std() {
    let now_time = Instant::now();
    let now_std = StdInstant::from(now_time);
    assert_eq!(now_time, now_std);
}

#[rstest]
fn std_eq() {
    let now_time = Instant::now();
    let now_std = StdInstant::from(now_time);
    assert_eq!(now_std, now_time);
}

#[rstest]
#[case(1.seconds(), Ordering::Less)]
#[case((-1).seconds(), Ordering::Greater)]
fn ord(#[case] duration: SignedDuration, #[case] expected: Ordering) {
    let now_time = Instant::now();
    let now_std = now_time + duration;
    assert_eq!(now_time.cmp(&now_std), expected);
}

#[rstest]
#[case(1.seconds(), Ordering::Less)]
#[case((-1).seconds(), Ordering::Greater)]
fn partial_ord_std(#[case] duration: SignedDuration, #[case] expected: Ordering) {
    let now_time = Instant::now();
    let now_std = StdInstant::from(now_time) + duration;
    assert_eq!(now_time.partial_cmp(&now_std), Some(expected));
}

#[rstest]
#[case(1.seconds(), Ordering::Greater)]
#[case((-1).seconds(), Ordering::Less)]
fn std_partial_ord(#[case] duration: SignedDuration, #[case] expected: Ordering) {
    let now_time = Instant::now();
    let now_std = StdInstant::from(now_time) + duration;
    assert_eq!(now_std.partial_cmp(&now_time), Some(expected));
}

#[rstest]
fn sub_regression() {
    let now = Instant::now();
    let future = now + SignedDuration::seconds(5);
    let past = now - SignedDuration::seconds(5);

    assert_eq!(future - now, SignedDuration::seconds(5));
    assert_eq!(now - past, SignedDuration::seconds(5));
    assert_eq!(future - past, SignedDuration::seconds(10));

    assert_eq!(now - future, SignedDuration::seconds(-5));
    assert_eq!(past - now, SignedDuration::seconds(-5));
    assert_eq!(past - future, SignedDuration::seconds(-10));
}

#[rstest]
fn as_ref() {
    let now = Instant::now();
    assert_eq!(now.as_ref(), now.as_ref());
}

#[rstest]
fn borrow() {
    use std::borrow::Borrow;
    let now = Instant::now();
    assert_eq!(
        <Instant as Borrow<StdInstant>>::borrow(&now),
        <Instant as Borrow<StdInstant>>::borrow(&now)
    );
}
