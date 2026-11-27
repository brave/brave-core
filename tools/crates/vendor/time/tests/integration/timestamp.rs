use std::cmp::Ordering;
use std::time::{Duration as StdDuration, SystemTime};

use rstest::rstest;
use time::Weekday::*;
use time::ext::{NumericalDuration, NumericalStdDuration};
use time::macros::{date, format_description, offset, time, timestamp, utc_datetime};
use time::{
    Date, Month, OffsetDateTime, SignedDuration, Time, Timestamp, UtcDateTime, UtcOffset, Weekday,
};

#[rstest]
fn unix_epoch() {
    assert_eq!(Timestamp::UNIX_EPOCH.as_seconds(), 0);
    assert_eq!(Timestamp::UNIX_EPOCH.as_nanoseconds(), 0);
}

#[rstest]
#[case(0, 0, timestamp!(0))]
#[case(1, 0, timestamp!(1))]
#[case(-1, 0, timestamp!(-1))]
#[case(1_546_398_245, 0, timestamp!(1_546_398_245))]
#[case(1_546_398_245, 1, timestamp!(1_546_398_245.000_000_001))]
#[case(i64::MAX, 0, None)]
#[case(i64::MIN, 0, None)]
#[case(0, 1_000_000_000, None)]
fn new(
    #[case] seconds: i64,
    #[case] nanoseconds: u32,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(Timestamp::new(seconds, nanoseconds).ok(), expected.into());
}

#[rstest]
#[case(0, timestamp!(0))]
#[case(1, timestamp!(1))]
#[case(-1, timestamp!(-1))]
#[case(i64::MAX, None)]
#[case(i64::MIN, None)]
fn from_seconds(#[case] seconds: i64, #[case] expected: impl Into<Option<Timestamp>>) {
    assert_eq!(Timestamp::from_seconds(seconds).ok(), expected.into());
}

#[rstest]
#[case(0, timestamp!(0))]
#[case(1_000, timestamp!(1))]
#[case(-1_000, timestamp!(-1))]
#[case(1_546_398_245_006, timestamp!(1_546_398_245.006))]
#[case(1_500, timestamp!(1.500))]
#[case(-1_500, timestamp!(-1.500))]
#[case(-500, timestamp!(-0.500))]
#[case(500, timestamp!(0.500))]
#[case(i64::MAX, None)]
#[case(i64::MIN, None)]
fn from_milliseconds(#[case] milliseconds: i64, #[case] expected: impl Into<Option<Timestamp>>) {
    assert_eq!(
        Timestamp::from_milliseconds(milliseconds).ok(),
        expected.into()
    );
}

#[rstest]
#[case(0, timestamp!(0))]
#[case(1_000_000, timestamp!(1))]
#[case(-1_000_000, timestamp!(-1))]
#[case(1_546_398_245_006_007, timestamp!(1_546_398_245.006_007))]
#[case(1_500_000, timestamp!(1.500_000))]
#[case(-1_500_000, timestamp!(-1.500_000))]
#[case(-500_000, timestamp!(-0.500_000))]
#[case(i128::MAX, None)]
#[case(i128::MIN, None)]
fn from_microseconds(#[case] microseconds: i128, #[case] expected: impl Into<Option<Timestamp>>) {
    assert_eq!(
        Timestamp::from_microseconds(microseconds).ok(),
        expected.into()
    );
}

#[rstest]
#[case(0, timestamp!(0))]
#[case(1_000_000_000, timestamp!(1))]
#[case(-1_000_000_000, timestamp!(-1))]
#[case(1_546_398_245_006_007_008, timestamp!(1_546_398_245.006_007_008))]
#[case(1_500_000_000, timestamp!(1.500_000_000))]
#[case(-1_500_000_000, timestamp!(-1.500_000_000))]
#[case(-500_000_000, timestamp!(-0.500_000_000))]
#[case(i128::MAX, None)]
#[case(i128::MIN, None)]
fn from_nanoseconds(#[case] nanoseconds: i128, #[case] expected: impl Into<Option<Timestamp>>) {
    assert_eq!(
        Timestamp::from_nanoseconds(nanoseconds).ok(),
        expected.into()
    );
}

#[rstest]
#[case(timestamp!(0), offset!(UTC), 0)]
#[case(timestamp!(1_546_398_245), offset!(+1), 4)]
fn to_offset(#[case] ts: Timestamp, #[case] offset: UtcOffset, #[case] expected_hour: u8) {
    assert_eq!(ts.to_offset(offset).hour(), expected_hour);
}

#[rstest]
#[case(timestamp!(1_546_398_245), offset!(+1), true)]
#[case(Timestamp::MAX, offset!(+1), false)]
#[case(Timestamp::MIN, offset!(-1), false)]
fn checked_to_offset(#[case] ts: Timestamp, #[case] offset: UtcOffset, #[case] expected: bool) {
    assert_eq!(ts.checked_to_offset(offset).is_some(), expected);
}

#[rstest]
#[case(timestamp!(0), UtcDateTime::UNIX_EPOCH)]
#[case(timestamp!(1_546_398_245), utc_datetime!(2019-01-02 3:04:05))]
fn to_utc(#[case] ts: Timestamp, #[case] expected: UtcDateTime) {
    assert_eq!(ts.to_utc(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(-1), -1)]
#[case(timestamp!(1_546_398_245), 1_546_398_245)]
#[case(timestamp!(1_546_398_245.006_007_008), 1_546_398_245)]
fn as_seconds(#[case] ts: Timestamp, #[case] expected: i64) {
    assert_eq!(ts.as_seconds(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1), 1_000)]
#[case(timestamp!(-1), -1_000)]
#[case(timestamp!(1_546_398_245.006), 1_546_398_245_006)]
#[case(timestamp!(1_546_398_245.006_007_008), 1_546_398_245_006)]
fn as_milliseconds(#[case] ts: Timestamp, #[case] expected: i64) {
    assert_eq!(ts.as_milliseconds(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1), 1_000_000)]
#[case(timestamp!(-1), -1_000_000)]
#[case(timestamp!(1_546_398_245.006_007), 1_546_398_245_006_007)]
#[case(timestamp!(1_546_398_245.006_007_008), 1_546_398_245_006_007)]
fn as_microseconds(#[case] ts: Timestamp, #[case] expected: i128) {
    assert_eq!(ts.as_microseconds(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1), 1_000_000_000)]
#[case(timestamp!(-1), -1_000_000_000)]
#[case(timestamp!(1_546_398_245.006_007_008), 1_546_398_245_006_007_008)]
fn as_nanoseconds(#[case] ts: Timestamp, #[case] expected: i128) {
    assert_eq!(ts.as_nanoseconds(), expected);
}

#[rstest]
#[case(timestamp!(0), date!(1970-01-01))]
#[case(timestamp!(1_546_398_245), date!(2019-01-02))]
#[case(timestamp!(-1), date!(1969-12-31))]
fn date(#[case] ts: Timestamp, #[case] expected: Date) {
    assert_eq!(ts.date(), expected);
}

#[rstest]
#[case(timestamp!(0), time!(0:00))]
#[case(timestamp!(1_546_398_245), time!(3:04:05))]
fn time_(#[case] ts: Timestamp, #[case] expected: Time) {
    assert_eq!(ts.time(), expected);
}

#[rstest]
#[case(timestamp!(0), 1970)]
#[case(timestamp!(1_546_398_245), 2019)]
#[case(timestamp!(-1), 1969)]
fn year(#[case] ts: Timestamp, #[case] expected: i32) {
    assert_eq!(ts.year(), expected);
}

#[rstest]
#[case(timestamp!(0), Month::January)]
#[case(timestamp!(1_546_398_245), Month::January)]
fn month(#[case] ts: Timestamp, #[case] expected: Month) {
    assert_eq!(ts.month(), expected);
}

#[rstest]
#[case(timestamp!(0), 1)]
#[case(timestamp!(1_546_398_245), 2)]
fn day(#[case] ts: Timestamp, #[case] expected: u8) {
    assert_eq!(ts.day(), expected);
}

#[rstest]
#[case(timestamp!(0), 1)]
#[case(timestamp!(1_546_398_245), 2)]
fn ordinal(#[case] ts: Timestamp, #[case] expected: u16) {
    assert_eq!(ts.ordinal(), expected);
}

#[rstest]
#[case(timestamp!(0), 1)]
#[case(timestamp!(1_546_398_245), 1)]
fn iso_week(#[case] ts: Timestamp, #[case] expected: u8) {
    assert_eq!(ts.iso_week(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1_546_398_245), 0)]
fn sunday_based_week(#[case] ts: Timestamp, #[case] expected: u8) {
    assert_eq!(ts.sunday_based_week(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1_546_398_245), 0)]
fn monday_based_week(#[case] ts: Timestamp, #[case] expected: u8) {
    assert_eq!(ts.monday_based_week(), expected);
}

#[rstest]
#[case(timestamp!(0), (1970, Month::January, 1))]
#[case(timestamp!(1_546_398_245), (2019, Month::January, 2))]
fn to_calendar_date(#[case] ts: Timestamp, #[case] expected: (i32, Month, u8)) {
    assert_eq!(ts.to_calendar_date(), expected);
}

#[rstest]
#[case(timestamp!(0), (1970, 1))]
#[case(timestamp!(1_546_398_245), (2019, 2))]
fn to_ordinal_date(#[case] ts: Timestamp, #[case] expected: (i32, u16)) {
    assert_eq!(ts.to_ordinal_date(), expected);
}

#[rstest]
#[case(timestamp!(0), (1970, 1, Thursday))]
#[case(timestamp!(1_546_398_245), (2019, 1, Wednesday))]
fn to_iso_week_date(#[case] ts: Timestamp, #[case] expected: (i32, u8, Weekday)) {
    assert_eq!(ts.to_iso_week_date(), expected);
}

#[rstest]
#[case(timestamp!(0), Thursday)]
#[case(timestamp!(1_546_398_245), Wednesday)]
fn weekday(#[case] ts: Timestamp, #[case] expected: Weekday) {
    assert_eq!(ts.weekday(), expected);
}

#[rstest]
#[case(timestamp!(0), Thursday)]
#[case(timestamp!(86_400), Friday)]
#[case(timestamp!(172_800), Saturday)]
#[case(timestamp!(259_200), Sunday)]
#[case(timestamp!(345_600), Monday)]
#[case(timestamp!(432_000), Tuesday)]
#[case(timestamp!(518_400), Wednesday)]
#[case(timestamp!(22_118_400), Monday)] // 256 days past epoch; > i8::MAX
#[case(timestamp!(-22_118_400), Sunday)] // 256 days before epoch; < i8::MIN
fn weekday_exhaustive(#[case] ts: Timestamp, #[case] expected: Weekday) {
    assert_eq!(ts.weekday(), expected);
}

#[rstest]
#[case(timestamp!(0), (0, 0, 0))]
#[case(timestamp!(1_546_398_245), (3, 4, 5))]
fn as_hms(#[case] ts: Timestamp, #[case] expected: (u8, u8, u8)) {
    assert_eq!(ts.as_hms(), expected);
}

#[rstest]
#[case(timestamp!(0), (0, 0, 0, 0))]
#[case(timestamp!(1_546_398_245.006), (3, 4, 5, 6))]
fn as_hms_milli(#[case] ts: Timestamp, #[case] expected: (u8, u8, u8, u16)) {
    assert_eq!(ts.as_hms_milli(), expected);
}

#[rstest]
#[case(timestamp!(0), (0, 0, 0, 0))]
#[case(timestamp!(1_546_398_245.006_007), (3, 4, 5, 6_007))]
fn as_hms_micro(#[case] ts: Timestamp, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(ts.as_hms_micro(), expected);
}

#[rstest]
#[case(timestamp!(0), (0, 0, 0, 0))]
#[case(timestamp!(1_546_398_245.006_007_008), (3, 4, 5, 6_007_008))]
fn as_hms_nano(#[case] ts: Timestamp, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(ts.as_hms_nano(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1_546_398_245), 3)]
#[case(timestamp!(86_399), 23)]
fn hour(#[case] ts: Timestamp, #[case] expected: u8) {
    assert_eq!(ts.hour(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1_546_398_245), 4)]
#[case(timestamp!(86_340), 59)]
#[case(timestamp!(-1), 59)]
#[case(timestamp!(-61), 58)]
fn minute(#[case] ts: Timestamp, #[case] expected: u8) {
    assert_eq!(ts.minute(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1_546_398_245), 5)]
#[case(timestamp!(86_399), 59)]
fn second(#[case] ts: Timestamp, #[case] expected: u8) {
    assert_eq!(ts.second(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1_546_398_245.006), 6)]
#[case(timestamp!(1_546_398_245.999), 999)]
fn millisecond(#[case] ts: Timestamp, #[case] expected: u16) {
    assert_eq!(ts.millisecond(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1_546_398_245.006_007), 6_007)]
#[case(timestamp!(1_546_398_245.999_999), 999_999)]
fn microsecond(#[case] ts: Timestamp, #[case] expected: u32) {
    assert_eq!(ts.microsecond(), expected);
}

#[rstest]
#[case(timestamp!(0), 0)]
#[case(timestamp!(1_546_398_245.006_007_008), 6_007_008)]
#[case(timestamp!(1_546_398_245.999_999_999), 999_999_999)]
fn nanosecond(#[case] ts: Timestamp, #[case] expected: u32) {
    assert_eq!(ts.nanosecond(), expected);
}

#[rstest]
#[case(timestamp!(1_546_398_245), time!(12:34:56.789), timestamp!(1_546_432_496.789))]
#[case(timestamp!(1_546_398_245), time!(23:59:59), timestamp!(1_546_473_599))]
#[case(timestamp!(-1), time!(0:00), timestamp!(-86_400))]
#[case(timestamp!(-86_401), time!(0:00), timestamp!(-172_800))]
fn replace_time(#[case] ts: Timestamp, #[case] replacement: Time, #[case] expected: Timestamp) {
    assert_eq!(ts.replace_time(replacement), expected);
}

#[rstest]
#[case(timestamp!(1_546_398_245), date!(2020-01-02), timestamp!(1_577_934_245))]
fn replace_date(#[case] ts: Timestamp, #[case] replacement: Date, #[case] expected: Timestamp) {
    assert_eq!(ts.replace_date(replacement), expected);
}

#[rstest]
#[case(timestamp!(1_546_398_245), 2020, timestamp!(1_577_934_245))]
#[case(timestamp!(1_546_398_245), -1_000_000, None)]
#[case(timestamp!(1_546_398_245), 1_000_000, None)]
fn replace_year(
    #[case] ts: Timestamp,
    #[case] year: i32,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_year(year).ok(), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245), Month::February, timestamp!(1_549_076_645))]
#[case(timestamp!(1_548_817_445), Month::February, None)]
fn replace_month(
    #[case] ts: Timestamp,
    #[case] month: Month,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_month(month).ok(), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245), 1, timestamp!(1_546_311_845))]
#[case(timestamp!(1_546_398_245), 0, None)]
#[case(timestamp!(1_546_398_245), 32, None)]
fn replace_day(
    #[case] ts: Timestamp,
    #[case] day: u8,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_day(day).ok(), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245), 1, timestamp!(1_546_311_845))]
#[case(timestamp!(1_546_398_245), 0, None)]
#[case(timestamp!(1_546_398_245), 366, None)]
fn replace_ordinal(
    #[case] ts: Timestamp,
    #[case] ordinal: u16,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_ordinal(ordinal).ok(), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245), 0, timestamp!(1_546_387_445))]
#[case(timestamp!(1_546_398_245), 24, None)]
#[case(timestamp!(-1), 0, timestamp!(-82_801))]
#[case(timestamp!(-86_401), 0, timestamp!(-169_201))]
fn replace_hour(
    #[case] ts: Timestamp,
    #[case] hour: u8,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_hour(hour).ok(), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245), 0, timestamp!(1_546_398_005))]
#[case(timestamp!(1_546_398_245), 60, None)]
#[case(timestamp!(-1), 0, timestamp!(-3_541))]
#[case(timestamp!(-86_401), 0, timestamp!(-89_941))]
fn replace_minute(
    #[case] ts: Timestamp,
    #[case] minute: u8,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_minute(minute).ok(), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245), 0, timestamp!(1_546_398_240))]
#[case(timestamp!(1_546_398_245), 60, None)]
#[case(timestamp!(-1), 0, timestamp!(-60))]
#[case(timestamp!(-86_401), 0, timestamp!(-86_460))]
fn replace_second(
    #[case] ts: Timestamp,
    #[case] second: u8,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_second(second).ok(), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245.006), 7, timestamp!(1_546_398_245.007))]
#[case(timestamp!(1_546_398_245.006), 1_000, None)]
#[case(timestamp!(-1), 400, timestamp!(-0.6))]
#[case(timestamp!(-1.5), 400, timestamp!(-1.4))]
#[case(timestamp!(-1.5), 0, timestamp!(-1))]
#[case(timestamp!(-1), 0, timestamp!(-1))]
#[case(Timestamp::MIN, 0, Timestamp::MIN)]
#[case(
    Timestamp::MIN,
    1,
    Timestamp::new(Timestamp::MIN.as_seconds(), 1_000_000).ok()
)]
fn replace_millisecond(
    #[case] ts: Timestamp,
    #[case] millisecond: u16,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_millisecond(millisecond).ok(), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245.006_007), 123_456, timestamp!(1_546_398_245.123_456))]
#[case(timestamp!(1_546_398_245.006_007), 1_000_000, None)]
#[case(
    timestamp!(-1_546_398_245.006_007),
    123_456,
    timestamp!(-1_546_398_245.123_456)
)]
#[case(timestamp!(-1_546_398_245.006_007), 0, timestamp!(-1_546_398_245))]
#[case(Timestamp::MIN, 0, Timestamp::MIN)]
#[case(
    Timestamp::MIN,
    1,
    Timestamp::new(Timestamp::MIN.as_seconds(), 1_000).ok()
)]
fn replace_microsecond(
    #[case] ts: Timestamp,
    #[case] microsecond: u32,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_microsecond(microsecond).ok(), expected.into());
}

#[rstest]
#[case(
    timestamp!(1_546_398_245.006_007_008),
    123_456_789,
    timestamp!(1_546_398_245.123_456_789),
)]
#[case(
    timestamp!(-1_546_398_245.006_007_008),
    123_456_789,
    timestamp!(-1_546_398_245.123_456_789),
)]
#[case(timestamp!(1_546_398_245.006_007_008), 1_000_000_000, None)]
#[case(Timestamp::MIN, 0, Timestamp::MIN)]
#[case(
    Timestamp::MIN,
    1,
    Timestamp::new(Timestamp::MIN.as_seconds(), 1).ok()
)]
fn replace_nanosecond(
    #[case] ts: Timestamp,
    #[case] nanosecond: u32,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.replace_nanosecond(nanosecond).ok(), expected.into());
}

#[rstest]
#[case(timestamp!(0), 5.days(), timestamp!(432_000))]
#[case(timestamp!(0), 1.days(), timestamp!(86_400))]
#[case(timestamp!(1_546_398_245), 1.days(), timestamp!(1_546_484_645))]
#[case(timestamp!(1_546_398_245), (-1).days(), timestamp!(1_546_311_845))]
fn add_duration(
    #[case] ts: Timestamp,
    #[case] duration: SignedDuration,
    #[case] expected: Timestamp,
) {
    assert_eq!(ts + duration, expected);
}

#[rstest]
#[case(timestamp!(0), 5.std_days(), timestamp!(432_000))]
#[case(timestamp!(0), 1.std_days(), timestamp!(86_400))]
#[case(timestamp!(1_546_398_245), 1.std_days(), timestamp!(1_546_484_645))]
fn add_std_duration(
    #[case] ts: Timestamp,
    #[case] duration: StdDuration,
    #[case] expected: Timestamp,
) {
    assert_eq!(ts + duration, expected);
}

#[rstest]
#[case(Timestamp::MAX, 1.nanoseconds())]
#[should_panic(expected = "resulting value is out of range")]
fn add_duration_panics(#[case] ts: Timestamp, #[case] duration: SignedDuration) {
    let _ = ts + duration;
}

#[rstest]
#[case(Timestamp::MAX, 1.std_nanoseconds())]
#[case(Timestamp::MIN, StdDuration::new(u64::MAX, 0))]
#[case(Timestamp::UNIX_EPOCH, StdDuration::new(u64::MAX, 0))]
#[should_panic(expected = "resulting value is out of range")]
fn add_std_panics(#[case] ts: Timestamp, #[case] duration: StdDuration) {
    let _ = ts + duration;
}

#[rstest]
#[case(timestamp!(432_000), 5.days(), timestamp!(0))]
#[case(timestamp!(86_400), 1.days(), timestamp!(0))]
#[case(timestamp!(1_546_398_245), 1.days(), timestamp!(1_546_311_845))]
#[case(timestamp!(1_546_398_245), (-1).days(), timestamp!(1_546_484_645))]
fn sub_duration(
    #[case] ts: Timestamp,
    #[case] duration: SignedDuration,
    #[case] expected: Timestamp,
) {
    assert_eq!(ts - duration, expected);
}

#[rstest]
#[case(timestamp!(432_000), 5.std_days(), timestamp!(0))]
#[case(timestamp!(86_400), 1.std_days(), timestamp!(0))]
#[case(timestamp!(1_546_484_645), 1.std_days(), timestamp!(1_546_398_245))]
fn sub_std_duration(
    #[case] ts: Timestamp,
    #[case] duration: StdDuration,
    #[case] expected: Timestamp,
) {
    assert_eq!(ts - duration, expected);
}

#[rstest]
#[case(Timestamp::MIN, 1.nanoseconds())]
#[should_panic(expected = "resulting value is out of range")]
fn sub_duration_panics(#[case] ts: Timestamp, #[case] duration: SignedDuration) {
    let _ = ts - duration;
}

#[rstest]
#[case(Timestamp::MIN, 1.std_nanoseconds())]
#[case(Timestamp::MAX, StdDuration::new(u64::MAX, 0))]
#[case(Timestamp::UNIX_EPOCH, StdDuration::new(u64::MAX, 0))]
#[should_panic(expected = "resulting value is out of range")]
fn sub_std_panics(#[case] ts: Timestamp, #[case] duration: StdDuration) {
    let _ = ts - duration;
}

#[rstest]
#[case(timestamp!(0), 5.days(), timestamp!(432_000))]
#[case(timestamp!(0), 1.days(), timestamp!(86_400))]
#[case(timestamp!(1_546_398_245), 1.days(), timestamp!(1_546_484_645))]
#[case(timestamp!(1_546_398_245), (-1).days(), timestamp!(1_546_311_845))]
fn add_assign_duration(
    #[case] mut ts: Timestamp,
    #[case] duration: SignedDuration,
    #[case] expected: Timestamp,
) {
    ts += duration;
    assert_eq!(ts, expected);
}

#[rstest]
#[case(timestamp!(0), 5.std_days(), timestamp!(432_000))]
#[case(timestamp!(0), 1.std_days(), timestamp!(86_400))]
#[case(timestamp!(1_546_398_245), 1.std_days(), timestamp!(1_546_484_645))]
fn add_assign_std_duration(
    #[case] mut ts: Timestamp,
    #[case] duration: StdDuration,
    #[case] expected: Timestamp,
) {
    ts += duration;
    assert_eq!(ts, expected);
}

#[rstest]
#[case(timestamp!(432_000), 5.days(), timestamp!(0))]
#[case(timestamp!(86_400), 1.days(), timestamp!(0))]
#[case(timestamp!(1_546_398_245), 1.days(), timestamp!(1_546_311_845))]
#[case(timestamp!(1_546_398_245), (-1).days(), timestamp!(1_546_484_645))]
fn sub_assign_duration(
    #[case] mut ts: Timestamp,
    #[case] duration: SignedDuration,
    #[case] expected: Timestamp,
) {
    ts -= duration;
    assert_eq!(ts, expected);
}

#[rstest]
#[case(timestamp!(432_000), 5.std_days(), timestamp!(0))]
#[case(timestamp!(86_400), 1.std_days(), timestamp!(0))]
#[case(timestamp!(1_546_484_645), 1.std_days(), timestamp!(1_546_398_245))]
fn sub_assign_std_duration(
    #[case] mut ts: Timestamp,
    #[case] duration: StdDuration,
    #[case] expected: Timestamp,
) {
    ts -= duration;
    assert_eq!(ts, expected);
}

#[rstest]
#[case(timestamp!(0), timestamp!(0), SignedDuration::ZERO)]
#[case(timestamp!(0), timestamp!(0.000_000_001), (-1).nanoseconds())]
#[case(timestamp!(0.000_000_001), timestamp!(0), 1.nanoseconds())]
#[case(timestamp!(-1), timestamp!(-2), 1.seconds())]
#[case(timestamp!(1_546_484_645), timestamp!(1_546_398_245), 1.days())]
#[case(timestamp!(1_546_398_245), timestamp!(1_546_484_645), (-1).days())]
fn sub_timestamp(#[case] lhs: Timestamp, #[case] rhs: Timestamp, #[case] expected: SignedDuration) {
    assert_eq!(lhs - rhs, expected);
}

#[rstest]
#[case(timestamp!(1_546_398_245), 5.nanoseconds(), timestamp!(1_546_398_245.000_000_005))]
#[case(timestamp!(1_546_398_245), 4.seconds(), timestamp!(1_546_398_249))]
#[case(timestamp!(1_546_398_245), 2.days(), timestamp!(1_546_571_045))]
#[case(timestamp!(1_546_398_245), 1.weeks(), timestamp!(1_547_003_045))]
#[case(timestamp!(1_546_398_245), (-5).nanoseconds(), timestamp!(1_546_398_244.999_999_995))]
#[case(timestamp!(1_546_398_245), (-4).seconds(), timestamp!(1_546_398_241))]
#[case(timestamp!(1_546_398_245), (-2).days(), timestamp!(1_546_225_445))]
#[case(timestamp!(1_546_398_245), (-1).weeks(), timestamp!(1_545_793_445))]
#[case(Timestamp::MIN, (-1).nanoseconds(), None)]
#[case(Timestamp::MAX, 1.nanoseconds(), None)]
#[case(Timestamp::MIN, SignedDuration::MIN, None)]
#[case(Timestamp::MAX, SignedDuration::MAX, None)]
#[case(timestamp!(0.999_999_999), i64::MAX.seconds() + 1.nanoseconds(), None)]
#[case(timestamp!(0), SignedDuration::MIN, None)]
fn checked_add_duration(
    #[case] ts: Timestamp,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.checked_add(duration), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245.000_000_005), 5.nanoseconds(), timestamp!(1_546_398_245))]
#[case(timestamp!(1_546_398_249), 4.seconds(), timestamp!(1_546_398_245))]
#[case(timestamp!(1_546_571_045), 2.days(), timestamp!(1_546_398_245))]
#[case(timestamp!(1_547_003_045), 1.weeks(), timestamp!(1_546_398_245))]
#[case(timestamp!(1_546_398_245), (-1).days(), timestamp!(1_546_484_645))]
#[case(timestamp!(1_546_398_245), (-1).weeks(), timestamp!(1_547_003_045))]
#[case(Timestamp::MIN, 1.nanoseconds(), None)]
#[case(Timestamp::MAX, (-1).nanoseconds(), None)]
#[case(Timestamp::MIN, SignedDuration::MIN, None)]
#[case(Timestamp::MIN, SignedDuration::MAX, None)]
#[case(timestamp!(-1), i64::MAX.seconds() + 1.nanoseconds(), None)]
#[case(timestamp!(0.999_999_999), (-i64::MAX).seconds() - 1.nanoseconds(), None)]
fn checked_sub_duration(
    #[case] ts: Timestamp,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<Timestamp>>,
) {
    assert_eq!(ts.checked_sub(duration), expected.into());
}

#[rstest]
#[case(timestamp!(1_546_398_245), 2.days(), timestamp!(1_546_571_045))]
#[case(timestamp!(1_546_398_245), (-2).days(), timestamp!(1_546_225_445))]
#[case(Timestamp::MIN, (-10).days(), Timestamp::MIN)]
#[case(Timestamp::MAX, 10.days(), Timestamp::MAX)]
#[case(Timestamp::MIN, (-1).nanoseconds(), Timestamp::MIN)]
#[case(Timestamp::MAX, 1.nanoseconds(), Timestamp::MAX)]
fn saturating_add_duration(
    #[case] ts: Timestamp,
    #[case] duration: SignedDuration,
    #[case] expected: Timestamp,
) {
    assert_eq!(ts.saturating_add(duration), expected);
}

#[rstest]
#[case(timestamp!(1_546_571_045), 2.days(), timestamp!(1_546_398_245))]
#[case(timestamp!(1_546_571_045), (-2).days(), timestamp!(1_546_743_845))]
#[case(Timestamp::MIN, 10.days(), Timestamp::MIN)]
#[case(Timestamp::MAX, (-10).days(), Timestamp::MAX)]
#[case(Timestamp::MIN, 1.nanoseconds(), Timestamp::MIN)]
#[case(Timestamp::MAX, (-1).nanoseconds(), Timestamp::MAX)]
fn saturating_sub_duration(
    #[case] ts: Timestamp,
    #[case] duration: SignedDuration,
    #[case] expected: Timestamp,
) {
    assert_eq!(ts.saturating_sub(duration), expected);
}

#[rstest]
#[case(Timestamp::MAX, 1.nanoseconds())]
#[should_panic]
fn add_panics(#[case] ts: Timestamp, #[case] duration: SignedDuration) {
    let _ = ts + duration;
}

#[rstest]
#[case(Timestamp::MIN, 1.nanoseconds())]
#[should_panic]
fn sub_panics(#[case] ts: Timestamp, #[case] duration: SignedDuration) {
    let _ = ts - duration;
}

#[rstest]
#[case(timestamp!(0), timestamp!(0), Ordering::Equal)]
#[case(timestamp!(0), timestamp!(1), Ordering::Less)]
#[case(timestamp!(0), timestamp!(-1), Ordering::Greater)]
#[case(timestamp!(1_546_398_245), timestamp!(1_546_398_245), Ordering::Equal)]
#[case(timestamp!(1_546_398_245), timestamp!(1_546_398_246), Ordering::Less)]
#[case(timestamp!(1_546_398_246), timestamp!(1_546_398_245), Ordering::Greater)]
#[case(timestamp!(-1), timestamp!(0), Ordering::Less)]
#[case(timestamp!(1), timestamp!(0), Ordering::Greater)]
#[case(timestamp!(0), timestamp!(0.000_000_001), Ordering::Less)]
#[case(timestamp!(0.000_000_001), timestamp!(0), Ordering::Greater)]
fn ord(#[case] lhs: Timestamp, #[case] rhs: Timestamp, #[case] expected: Ordering) {
    assert_eq!(lhs.cmp(&rhs), expected);
}

#[rstest]
#[case(timestamp!(0), timestamp!(0), true)]
#[case(timestamp!(0), timestamp!(1), false)]
#[case(timestamp!(0), timestamp!(-1), false)]
#[case(timestamp!(1_546_398_245.006_007_008), timestamp!(1_546_398_245.006_007_008), true)]
#[case(timestamp!(1_546_398_245.006_007_008), timestamp!(1_546_398_245.006_007_009), false)]
fn eq(#[case] lhs: Timestamp, #[case] rhs: Timestamp, #[case] expected: bool) {
    assert_eq!(lhs == rhs, expected);
}

#[rstest]
#[case(timestamp!(0), timestamp!(0))]
#[case(timestamp!(1_546_398_245), timestamp!(1_546_398_245))]
fn hash(#[case] a: Timestamp, #[case] b: Timestamp) {
    use std::collections::hash_map::DefaultHasher;
    use std::hash::{Hash, Hasher};

    let mut hasher_a = DefaultHasher::new();
    let mut hasher_b = DefaultHasher::new();
    a.hash(&mut hasher_a);
    b.hash(&mut hasher_b);
    assert_eq!(hasher_a.finish(), hasher_b.finish());
}

#[rstest]
#[case(timestamp!(0), "0")]
#[case(timestamp!(1), "1")]
#[case(timestamp!(-1), "-1")]
#[case(timestamp!(1_234_567_890.001), "1234567890.001")]
#[case(timestamp!(1_234_567_890.123_456_789), "1234567890.123456789")]
#[case(timestamp!(1_234_567_890.123_456), "1234567890.123456")]
#[case(timestamp!(1_234_567_890.123), "1234567890.123")]
#[case(timestamp!(-0.5), "-0.5")]
#[case(timestamp!(-1) + 400_000_000.nanoseconds(), "-0.6")]
#[case(timestamp!(-123_456_789) + 123_456_789.nanoseconds(), "-123456788.876543211")]
fn display(#[case] ts: Timestamp, #[case] expected: &str) {
    assert_eq!(ts.to_string(), expected);
}

#[rstest]
#[case(timestamp!(0), "0")]
#[case(timestamp!(1_546_398_245), "1546398245")]
fn debug(#[case] ts: Timestamp, #[case] expected: &str) {
    assert_eq!(format!("{ts:?}"), expected);
}

#[rstest]
fn now() {
    assert!(Timestamp::now().year() >= 2019);
}

#[rstest]
#[case(timestamp!(0), 2_440_588)]
#[case(timestamp!(1_546_398_245), 2_458_486)]
fn to_julian_day(#[case] ts: Timestamp, #[case] expected: i32) {
    assert_eq!(ts.to_julian_day(), expected);
}

#[rstest]
fn unix_timestamp_format() {
    let format = format_description!("[unix_timestamp]");
    assert_eq!(
        Timestamp::parse("1546398245", &format),
        Ok(timestamp!(1_546_398_245)),
    );
    assert!(matches!(
        timestamp!(1_546_398_245).format(&format).as_deref(),
        Ok("1546398245")
    ));
}

#[rstest]
#[case(utc_datetime!(1970-01-01 0:00), timestamp!(0))]
#[case(utc_datetime!(2019-01-02 3:04:05), timestamp!(1_546_398_245))]
fn from_utc_datetime_into_timestamp(#[case] source: UtcDateTime, #[case] expected: Timestamp) {
    assert_eq!(Timestamp::from(source), expected);
}

#[rstest]
#[case(timestamp!(0), utc_datetime!(1970-01-01 0:00))]
#[case(timestamp!(1_546_398_245), utc_datetime!(2019-01-02 3:04:05))]
fn from_timestamp_into_utc_datetime(#[case] source: Timestamp, #[case] expected: UtcDateTime) {
    assert_eq!(UtcDateTime::from(source), expected);
}

#[rstest]
fn utc_datetime_sub_timestamp() {
    assert_eq!(
        utc_datetime!(2019-01-02 3:04:05) - timestamp!(1_546_398_245),
        SignedDuration::ZERO,
    );
}

#[rstest]
fn timestamp_utc_datetime_partial_eq() {
    assert_eq!(timestamp!(0), utc_datetime!(1970-01-01 0:00));
    assert_eq!(utc_datetime!(1970-01-01 0:00), timestamp!(0));
    assert_ne!(timestamp!(1), utc_datetime!(1970-01-01 0:00));
}

#[rstest]
fn timestamp_utc_datetime_partial_ord() {
    assert!(timestamp!(0) < utc_datetime!(1970-01-02 0:00));
    assert!(utc_datetime!(1970-01-02 0:00) > timestamp!(0));
}

#[rstest]
#[case(timestamp!(0))]
#[case(timestamp!(1_546_398_245))]
fn from_offset_date_time(#[case] expected: Timestamp) {
    assert!(
        OffsetDateTime::from_unix_timestamp(expected.as_seconds())
            .is_ok_and(|odt| Timestamp::from(odt) == expected)
    );
}

#[rstest]
#[case(timestamp!(0))]
#[case(timestamp!(1_546_398_245))]
fn into_offset_date_time(#[case] source: Timestamp) {
    let odt = OffsetDateTime::from(source);
    assert_eq!(odt.unix_timestamp(), source.as_seconds());
    assert_eq!(odt.nanosecond(), source.nanosecond());
}

#[rstest]
#[case(timestamp!(1) - utc_datetime!(1970-01-01 0:00), 1.seconds())]
#[case(timestamp!(0) - utc_datetime!(1970-01-01 0:00), SignedDuration::ZERO)]
fn sub_utc_date_time_implicit_utc(
    #[case] result: SignedDuration,
    #[case] expected: SignedDuration,
) {
    assert_eq!(result, expected);
}

#[rstest]
fn sub_offset_date_time() {
    assert_eq!(
        OffsetDateTime::from_unix_timestamp(1_546_398_245)
            .map(|odt| odt - timestamp!(1_546_311_845)),
        Ok(86_400.seconds()),
    );
}

#[rstest]
fn eq_offset_date_time() {
    assert!(OffsetDateTime::from_unix_timestamp(0).is_ok_and(|odt| timestamp!(0) == odt));
    assert!(OffsetDateTime::from_unix_timestamp(0).is_ok_and(|odt| odt == timestamp!(0)));
    assert!(OffsetDateTime::from_unix_timestamp(0).is_ok_and(|odt| timestamp!(1) != odt));
}

#[rstest]
fn ord_offset_date_time() {
    assert!(
        OffsetDateTime::from_unix_timestamp(0).is_ok_and(|earlier| timestamp!(86_400) > earlier)
    );
    assert!(OffsetDateTime::from_unix_timestamp(86_400).is_ok_and(|later| timestamp!(0) < later));
}

#[rstest]
fn from_system_time_into_timestamp() {
    assert_eq!(Timestamp::from(SystemTime::UNIX_EPOCH), timestamp!(0));
    assert_eq!(
        Ok(Timestamp::from(
            SystemTime::UNIX_EPOCH - 5.seconds() - 1.milliseconds()
        )),
        Timestamp::from_milliseconds(-5_001),
    );
    assert_eq!(
        Ok(Timestamp::from(
            SystemTime::UNIX_EPOCH + 5.seconds() + 1.milliseconds()
        )),
        Timestamp::from_milliseconds(5_001),
    );
}

#[rstest]
fn from_timestamp_into_system_time() {
    let st = SystemTime::from(timestamp!(0));
    assert_eq!(st, SystemTime::UNIX_EPOCH);
}

#[rstest]
fn timestamp_sub_system_time() {
    let duration = timestamp!(86_400) - SystemTime::UNIX_EPOCH;
    assert_eq!(duration, 86_400.seconds());
    let duration_before = timestamp!(0) - (SystemTime::UNIX_EPOCH - StdDuration::new(5, 0));
    assert_eq!(duration_before, 5.seconds());
}

#[rstest]
fn system_time_sub_timestamp() {
    let duration = SystemTime::UNIX_EPOCH - timestamp!(0);
    assert_eq!(duration, SignedDuration::ZERO);
    let duration_before = (SystemTime::UNIX_EPOCH - StdDuration::new(5, 0)) - timestamp!(0);
    assert_eq!(duration_before, (-5).seconds());
}

#[rstest]
fn timestamp_system_time_partial_eq() {
    assert_eq!(timestamp!(0), SystemTime::UNIX_EPOCH);
    assert_eq!(SystemTime::UNIX_EPOCH, timestamp!(0));
    assert_ne!(timestamp!(1), SystemTime::UNIX_EPOCH);
}

#[rstest]
fn timestamp_system_time_partial_ord() {
    assert!(timestamp!(0) < SystemTime::UNIX_EPOCH + 1.seconds());
    assert!(SystemTime::UNIX_EPOCH + 1.seconds() > timestamp!(0));
}

#[rstest]
fn format_into_timestamp() {
    let mut output = Vec::new();
    let format = format_description!("[unix_timestamp]");
    match timestamp!(1_546_398_245).format_into(&mut output, &format) {
        Ok(count) => assert_eq!(count, 10),
        Err(_) => unreachable!("format should succeed"),
    }
    assert_eq!(output, b"1546398245");
}
