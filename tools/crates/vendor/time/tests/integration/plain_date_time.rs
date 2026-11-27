use std::cmp::Ordering;

use rstest::rstest;
use time::ext::{NumericalDuration, NumericalStdDuration};
use time::macros::{date, datetime, offset, time};
use time::{Date, Month, PlainDateTime, SignedDuration, Time, UtcOffset, Weekday};

#[rstest]
#[case(PlainDateTime::new(date!(2019-01-01), time!(0:00)), datetime!(2019-01-01 0:00))]
fn new(#[case] input: PlainDateTime, #[case] expected: PlainDateTime) {
    assert_eq!(input, expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), date!(2019-01-01))]
fn date(#[case] datetime: PlainDateTime, #[case] expected: Date) {
    assert_eq!(datetime.date(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), time!(0:00))]
fn time_(#[case] datetime: PlainDateTime, #[case] expected: Time) {
    assert_eq!(datetime.time(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 2019)]
#[case(datetime!(2019-12-31 0:00), 2019)]
#[case(datetime!(2020-01-01 0:00), 2020)]
fn year(#[case] datetime: PlainDateTime, #[case] expected: i32) {
    assert_eq!(datetime.year(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), Month::January)]
#[case(datetime!(2019-12-31 0:00), Month::December)]
fn month(#[case] datetime: PlainDateTime, #[case] expected: Month) {
    assert_eq!(datetime.month(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 1)]
#[case(datetime!(2019-12-31 0:00), 31)]
fn day(#[case] datetime: PlainDateTime, #[case] expected: u8) {
    assert_eq!(datetime.day(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 1)]
#[case(datetime!(2019-12-31 0:00), 365)]
fn ordinal(#[case] datetime: PlainDateTime, #[case] expected: u16) {
    assert_eq!(datetime.ordinal(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 1)]
#[case(datetime!(2019-10-04 0:00), 40)]
#[case(datetime!(2020-01-01 0:00), 1)]
#[case(datetime!(2020-12-31 0:00), 53)]
#[case(datetime!(2021-01-01 0:00), 53)]
fn iso_week(#[case] datetime: PlainDateTime, #[case] expected: u8) {
    assert_eq!(datetime.iso_week(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 0)]
#[case(datetime!(2020-01-01 0:00), 0)]
#[case(datetime!(2020-12-31 0:00), 52)]
#[case(datetime!(2021-01-01 0:00), 0)]
fn sunday_based_week(#[case] datetime: PlainDateTime, #[case] expected: u8) {
    assert_eq!(datetime.sunday_based_week(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 0)]
#[case(datetime!(2020-01-01 0:00), 0)]
#[case(datetime!(2020-12-31 0:00), 52)]
#[case(datetime!(2021-01-01 0:00), 0)]
fn monday_based_week(#[case] datetime: PlainDateTime, #[case] expected: u8) {
    assert_eq!(datetime.monday_based_week(), expected);
}

#[rstest]
#[case(datetime!(2019-01-02 0:00), (2019, Month::January, 2))]
fn to_calendar_date(#[case] datetime: PlainDateTime, #[case] expected: (i32, Month, u8)) {
    assert_eq!(datetime.to_calendar_date(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), (2019, 1))]
fn to_ordinal_date(#[case] datetime: PlainDateTime, #[case] expected: (i32, u16)) {
    assert_eq!(datetime.to_ordinal_date(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), (2019, 1, Weekday::Tuesday))]
#[case(datetime!(2019-10-04 0:00), (2019, 40, Weekday::Friday))]
#[case(datetime!(2020-01-01 0:00), (2020, 1, Weekday::Wednesday))]
#[case(datetime!(2020-12-31 0:00), (2020, 53, Weekday::Thursday))]
#[case(datetime!(2021-01-01 0:00), (2020, 53, Weekday::Friday))]
fn to_iso_week_date(#[case] datetime: PlainDateTime, #[case] expected: (i32, u8, Weekday)) {
    assert_eq!(datetime.to_iso_week_date(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), Weekday::Tuesday)]
#[case(datetime!(2019-02-01 0:00), Weekday::Friday)]
#[case(datetime!(2019-03-01 0:00), Weekday::Friday)]
#[case(datetime!(2019-04-01 0:00), Weekday::Monday)]
#[case(datetime!(2019-05-01 0:00), Weekday::Wednesday)]
#[case(datetime!(2019-06-01 0:00), Weekday::Saturday)]
#[case(datetime!(2019-07-01 0:00), Weekday::Monday)]
#[case(datetime!(2019-08-01 0:00), Weekday::Thursday)]
#[case(datetime!(2019-09-01 0:00), Weekday::Sunday)]
#[case(datetime!(2019-10-01 0:00), Weekday::Tuesday)]
#[case(datetime!(2019-11-01 0:00), Weekday::Friday)]
#[case(datetime!(2019-12-01 0:00), Weekday::Sunday)]
fn weekday(#[case] datetime: PlainDateTime, #[case] expected: Weekday) {
    assert_eq!(datetime.weekday(), expected);
}

#[rstest]
#[case(datetime!(-999_999-01-01 0:00), -363_521_074)]
#[case(datetime!(-4713-11-24 0:00), 0)]
#[case(datetime!(2000-01-01 0:00), 2_451_545)]
#[case(datetime!(2019-01-01 0:00), 2_458_485)]
#[case(datetime!(2019-12-31 0:00), 2_458_849)]
fn to_julian_day(#[case] datetime: PlainDateTime, #[case] expected: i32) {
    assert_eq!(datetime.to_julian_day(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 1:02:03), (1, 2, 3))]
fn as_hms(#[case] datetime: PlainDateTime, #[case] expected: (u8, u8, u8)) {
    assert_eq!(datetime.as_hms(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 1:02:03.004), (1, 2, 3, 4))]
fn as_hms_milli(#[case] datetime: PlainDateTime, #[case] expected: (u8, u8, u8, u16)) {
    assert_eq!(datetime.as_hms_milli(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 1:02:03.004_005), (1, 2, 3, 4_005))]
fn as_hms_micro(#[case] datetime: PlainDateTime, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(datetime.as_hms_micro(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 1:02:03.004_005_006), (1, 2, 3, 4_005_006))]
fn as_hms_nano(#[case] datetime: PlainDateTime, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(datetime.as_hms_nano(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 0)]
#[case(datetime!(2019-01-01 23:59:59), 23)]
fn hour(#[case] datetime: PlainDateTime, #[case] expected: u8) {
    assert_eq!(datetime.hour(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 0)]
#[case(datetime!(2019-01-01 23:59:59), 59)]
fn minute(#[case] datetime: PlainDateTime, #[case] expected: u8) {
    assert_eq!(datetime.minute(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 0)]
#[case(datetime!(2019-01-01 23:59:59), 59)]
fn second(#[case] datetime: PlainDateTime, #[case] expected: u8) {
    assert_eq!(datetime.second(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 0)]
#[case(datetime!(2019-01-01 23:59:59.999), 999)]
fn millisecond(#[case] datetime: PlainDateTime, #[case] expected: u16) {
    assert_eq!(datetime.millisecond(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 0)]
#[case(datetime!(2019-01-01 23:59:59.999_999), 999_999)]
fn microsecond(#[case] datetime: PlainDateTime, #[case] expected: u32) {
    assert_eq!(datetime.microsecond(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 0)]
#[case(datetime!(2019-01-01 23:59:59.999_999_999), 999_999_999)]
fn nanosecond(#[case] datetime: PlainDateTime, #[case] expected: u32) {
    assert_eq!(datetime.nanosecond(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), offset!(UTC), 1_546_300_800)]
#[case(datetime!(2019-01-01 0:00), offset!(-1), 1_546_304_400)]
fn assume_offset(
    #[case] datetime: PlainDateTime,
    #[case] offset: UtcOffset,
    #[case] expected: i64,
) {
    assert_eq!(datetime.assume_offset(offset).unix_timestamp(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 1_546_300_800)]
fn assume_utc(#[case] datetime: PlainDateTime, #[case] expected: i64) {
    assert_eq!(datetime.assume_utc().unix_timestamp(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 12:00), time!(5:00), datetime!(2020-01-01 5:00))]
fn replace_time(
    #[case] datetime: PlainDateTime,
    #[case] replacement: Time,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(datetime.replace_time(replacement), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 12:00), date!(2020-01-30), datetime!(2020-01-30 12:00))]
fn replace_date(
    #[case] datetime: PlainDateTime,
    #[case] replacement: Date,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(datetime.replace_date(replacement), expected);
}

#[rstest]
#[case(datetime!(2022-02-18 12:00), 2019, datetime!(2019-02-18 12:00))]
#[case(datetime!(2022-02-18 12:00), -1_000_000_000, None)]
#[case(datetime!(2022-02-18 12:00), 1_000_000_000, None)]
fn replace_year(
    #[case] datetime: PlainDateTime,
    #[case] year: i32,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_year(year).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 12:00), Month::January, datetime!(2022-01-18 12:00))]
#[case(datetime!(2022-01-30 12:00), Month::February, None)]
fn replace_month(
    #[case] datetime: PlainDateTime,
    #[case] month: Month,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_month(month).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 12:00), 1, datetime!(2022-02-01 12:00))]
#[case(datetime!(2022-02-18 12:00), 0, None)]
#[case(datetime!(2022-02-18 12:00), 30, None)]
fn replace_day(
    #[case] datetime: PlainDateTime,
    #[case] day: u8,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_day(day).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 12:00), 1, datetime!(2022-001 12:00))]
#[case(datetime!(2024-02-29 12:00), 366, datetime!(2024-366 12:00))]
#[case(datetime!(2022-049 12:00), 0, None)]
#[case(datetime!(2022-049 12:00), 366, None)]
#[case(datetime!(2022-049 12:00), 367, None)]
fn replace_ordinal(
    #[case] datetime: PlainDateTime,
    #[case] ordinal: u16,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_ordinal(ordinal).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 7, datetime!(2022-02-18 07:02:03.004_005_006))]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 24, None)]
fn replace_hour(
    #[case] datetime: PlainDateTime,
    #[case] value: u8,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_hour(value).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 7, datetime!(2022-02-18 01:07:03.004_005_006))]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 60, None)]
fn replace_minute(
    #[case] datetime: PlainDateTime,
    #[case] value: u8,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_minute(value).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 7, datetime!(2022-02-18 01:02:07.004_005_006))]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 60, None)]
fn replace_second(
    #[case] datetime: PlainDateTime,
    #[case] value: u8,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_second(value).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 7, datetime!(2022-02-18 01:02:03.007))]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 1_000, None)]
fn replace_millisecond(
    #[case] datetime: PlainDateTime,
    #[case] value: u16,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_millisecond(value).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 7_008, datetime!(2022-02-18 01:02:03.007_008))]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 1_000_000, None)]
fn replace_microsecond(
    #[case] datetime: PlainDateTime,
    #[case] value: u32,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_microsecond(value).ok(), expected.into());
}

#[rstest]
#[case(
    datetime!(2022-02-18 01:02:03.004_005_006),
    7_008_009,
    datetime!(2022-02-18 01:02:03.007_008_009),
)]
#[case(datetime!(2022-02-18 01:02:03.004_005_006), 1_000_000_000, None)]
fn replace_nanosecond(
    #[case] datetime: PlainDateTime,
    #[case] value: u32,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.replace_nanosecond(value).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789), datetime!(2021-11-12 0:00))]
#[case(datetime!(2021-11-12 0:00), datetime!(2021-11-12 0:00))]
fn truncate_to_day(#[case] datetime: PlainDateTime, #[case] expected: PlainDateTime) {
    assert_eq!(datetime.truncate_to_day(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789), datetime!(2021-11-12 17:00))]
#[case(datetime!(2021-11-12 0:00), datetime!(2021-11-12 0:00))]
fn truncate_to_hour(#[case] datetime: PlainDateTime, #[case] expected: PlainDateTime) {
    assert_eq!(datetime.truncate_to_hour(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789), datetime!(2021-11-12 17:47))]
#[case(datetime!(2021-11-12 0:00), datetime!(2021-11-12 0:00))]
fn truncate_to_minute(#[case] datetime: PlainDateTime, #[case] expected: PlainDateTime) {
    assert_eq!(datetime.truncate_to_minute(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789), datetime!(2021-11-12 17:47:53))]
#[case(datetime!(2021-11-12 0:00), datetime!(2021-11-12 0:00))]
fn truncate_to_second(#[case] datetime: PlainDateTime, #[case] expected: PlainDateTime) {
    assert_eq!(datetime.truncate_to_second(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789), datetime!(2021-11-12 17:47:53.123))]
#[case(datetime!(2021-11-12 0:00), datetime!(2021-11-12 0:00))]
fn truncate_to_millisecond(#[case] datetime: PlainDateTime, #[case] expected: PlainDateTime) {
    assert_eq!(datetime.truncate_to_millisecond(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789), datetime!(2021-11-12 17:47:53.123_456))]
#[case(datetime!(2021-11-12 0:00), datetime!(2021-11-12 0:00))]
fn truncate_to_microsecond(#[case] datetime: PlainDateTime, #[case] expected: PlainDateTime) {
    assert_eq!(datetime.truncate_to_microsecond(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 5.days(), datetime!(2019-01-06 0:00))]
#[case(datetime!(2019-12-31 0:00), 1.days(), datetime!(2020-01-01 0:00))]
#[case(datetime!(2019-12-31 23:59:59), 2.seconds(), datetime!(2020-01-01 0:00:01))]
#[case(datetime!(2020-01-01 0:00:01), (-2).seconds(), datetime!(2019-12-31 23:59:59))]
#[case(datetime!(1999-12-31 23:00), 1.hours(), datetime!(2000-01-01 0:00))]
fn add_duration(
    #[case] datetime: PlainDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(datetime + duration, expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 5.std_days(), datetime!(2019-01-06 0:00))]
#[case(datetime!(2019-12-31 0:00), 1.std_days(), datetime!(2020-01-01 0:00))]
#[case(datetime!(2019-12-31 23:59:59), 2.std_seconds(), datetime!(2020-01-01 0:00:01))]
fn add_std_duration(
    #[case] datetime: PlainDateTime,
    #[case] duration: std::time::Duration,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(datetime + duration, expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 5.days(), datetime!(2019-01-06 0:00))]
#[case(datetime!(2019-12-31 0:00), 1.days(), datetime!(2020-01-01 0:00))]
#[case(datetime!(2019-12-31 23:59:59), 2.seconds(), datetime!(2020-01-01 0:00:01))]
#[case(datetime!(2020-01-01 0:00:01), (-2).seconds(), datetime!(2019-12-31 23:59:59))]
fn add_assign_duration(
    #[case] mut datetime: PlainDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: PlainDateTime,
) {
    datetime += duration;
    assert_eq!(datetime, expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), 5.std_days(), datetime!(2019-01-06 0:00))]
#[case(datetime!(2019-12-31 0:00), 1.std_days(), datetime!(2020-01-01 0:00))]
#[case(datetime!(2019-12-31 23:59:59), 2.std_seconds(), datetime!(2020-01-01 0:00:01))]
fn add_assign_std_duration(
    #[case] mut datetime: PlainDateTime,
    #[case] duration: std::time::Duration,
    #[case] expected: PlainDateTime,
) {
    datetime += duration;
    assert_eq!(datetime, expected);
}

#[rstest]
#[case(datetime!(2019-01-06 0:00), 5.days(), datetime!(2019-01-01 0:00))]
#[case(datetime!(2020-01-01 0:00), 1.days(), datetime!(2019-12-31 0:00))]
#[case(datetime!(2020-01-01 0:00:01), 2.seconds(), datetime!(2019-12-31 23:59:59))]
#[case(datetime!(2019-12-31 23:59:59), (-2).seconds(), datetime!(2020-01-01 0:00:01))]
#[case(datetime!(1999-12-31 23:00), (-1).hours(), datetime!(2000-01-01 0:00))]
fn sub_duration(
    #[case] datetime: PlainDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(datetime - duration, expected);
}

#[rstest]
#[case(datetime!(2019-01-06 0:00), 5.std_days(), datetime!(2019-01-01 0:00))]
#[case(datetime!(2020-01-01 0:00), 1.std_days(), datetime!(2019-12-31 0:00))]
#[case(datetime!(2020-01-01 0:00:01), 2.std_seconds(), datetime!(2019-12-31 23:59:59))]
fn sub_std_duration(
    #[case] datetime: PlainDateTime,
    #[case] duration: std::time::Duration,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(datetime - duration, expected);
}

#[rstest]
#[case(datetime!(2019-01-06 0:00), 5.days(), datetime!(2019-01-01 0:00))]
#[case(datetime!(2020-01-01 0:00), 1.days(), datetime!(2019-12-31 0:00))]
#[case(datetime!(2020-01-01 0:00:01), 2.seconds(), datetime!(2019-12-31 23:59:59))]
#[case(datetime!(2019-12-31 23:59:59), (-2).seconds(), datetime!(2020-01-01 0:00:01))]
fn sub_assign_duration(
    #[case] mut datetime: PlainDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: PlainDateTime,
) {
    datetime -= duration;
    assert_eq!(datetime, expected);
}

#[rstest]
#[case(datetime!(2019-01-06 0:00), 5.std_days(), datetime!(2019-01-01 0:00))]
#[case(datetime!(2020-01-01 0:00), 1.std_days(), datetime!(2019-12-31 0:00))]
#[case(datetime!(2020-01-01 0:00:01), 2.std_seconds(), datetime!(2019-12-31 23:59:59))]
fn sub_assign_std_duration(
    #[case] mut datetime: PlainDateTime,
    #[case] duration: std::time::Duration,
    #[case] expected: PlainDateTime,
) {
    datetime -= duration;
    assert_eq!(datetime, expected);
}

#[rstest]
#[case(datetime!(2019-01-02 0:00), datetime!(2019-01-01 0:00), 1.days())]
#[case(datetime!(2019-01-01 0:00), datetime!(2019-01-02 0:00), (-1).days())]
#[case(datetime!(2020-01-01 0:00), datetime!(2019-12-31 0:00), 1.days())]
#[case(datetime!(2019-12-31 0:00), datetime!(2020-01-01 0:00), (-1).days())]
fn sub_datetime(
    #[case] lhs: PlainDateTime,
    #[case] rhs: PlainDateTime,
    #[case] expected: SignedDuration,
) {
    assert_eq!(lhs - rhs, expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00), datetime!(2019-01-01 0:00), Ordering::Equal)]
#[case(datetime!(2019-01-01 0:00), datetime!(2020-01-01 0:00), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00), datetime!(2019-02-01 0:00), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00), datetime!(2019-01-02 0:00), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00), datetime!(2019-01-01 1:00), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00), datetime!(2019-01-01 0:01), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00), datetime!(2019-01-01 0:00:01), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00), datetime!(2019-01-01 0:00:00.000_000_001), Ordering::Less)]
#[case(datetime!(2020-01-01 0:00), datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-02-01 0:00), datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-02 0:00), datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-01 1:00), datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-01 0:01), datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-01 0:00:01), datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-01 0:00:00.000_000_001), datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(-0001-01-01 0:00), datetime!(0001-01-01 0:00), Ordering::Less)]
fn ord(
    #[case] lhs: PlainDateTime,
    #[case] rhs: PlainDateTime,
    #[case] expected: impl Into<Option<Ordering>>,
) {
    assert_eq!(lhs.partial_cmp(&rhs), expected.into());
}

#[rstest]
#[case(
    datetime!(2021-10-25 14:01:53.45),
    5.nanoseconds(),
    datetime!(2021-10-25 14:01:53.450_000_005),
)]
#[case(datetime!(2021-10-25 14:01:53.45), 4.seconds(), datetime!(2021-10-25 14:01:57.45))]
#[case(datetime!(2021-10-25 14:01:53.45), 2.days(), datetime!(2021-10-27 14:01:53.45))]
#[case(datetime!(2021-10-25 14:01:53.45), 1.weeks(), datetime!(2021-11-01 14:01:53.45))]
#[case(
    datetime!(2021-10-25 14:01:53.45),
    (-5).nanoseconds(),
    datetime!(2021-10-25 14:01:53.449_999_995),
)]
#[case(datetime!(2021-10-25 14:01:53.45), (-4).seconds(), datetime!(2021-10-25 14:01:49.45))]
#[case(datetime!(2021-10-25 14:01:53.45), (-2).days(), datetime!(2021-10-23 14:01:53.45))]
#[case(datetime!(2021-10-25 14:01:53.45), (-1).weeks(), datetime!(2021-10-18 14:01:53.45))]
#[case::underflow(PlainDateTime::MIN, (-1).nanoseconds(), None)]
#[case::underflow(PlainDateTime::MIN, SignedDuration::MIN, None)]
#[case::underflow(PlainDateTime::MIN, (-530).weeks(), None)]
#[case::overflow(PlainDateTime::MAX, 1.nanoseconds(), None)]
#[case::overflow(PlainDateTime::MAX, SignedDuration::MAX, None)]
#[case::overflow(PlainDateTime::MAX, 530.weeks(), None)]
fn checked_add_duration(
    #[case] datetime: PlainDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.checked_add(duration), expected.into());
}

#[rstest]
#[case(
    datetime!(2021-10-25 14:01:53.45),
    (-5).nanoseconds(),
    datetime!(2021-10-25 14:01:53.450_000_005),
)]
#[case(datetime!(2021-10-25 14:01:53.45), (-4).seconds(), datetime!(2021-10-25 14:01:57.45))]
#[case(datetime!(2021-10-25 14:01:53.45), (-2).days(), datetime!(2021-10-27 14:01:53.45))]
#[case(datetime!(2021-10-25 14:01:53.45), (-1).weeks(), datetime!(2021-11-01 14:01:53.45))]
#[case(
    datetime!(2021-10-25 14:01:53.45),
    5.nanoseconds(),
    datetime!(2021-10-25 14:01:53.449_999_995),
)]
#[case(datetime!(2021-10-25 14:01:53.45), 4.seconds(), datetime!(2021-10-25 14:01:49.45))]
#[case(datetime!(2021-10-25 14:01:53.45), 2.days(), datetime!(2021-10-23 14:01:53.45))]
#[case(datetime!(2021-10-25 14:01:53.45), 1.weeks(), datetime!(2021-10-18 14:01:53.45))]
#[case::underflow(PlainDateTime::MIN, 1.nanoseconds(), None)]
#[case::underflow(PlainDateTime::MIN, SignedDuration::MIN, None)]
#[case::underflow(PlainDateTime::MIN, 530.weeks(), None)]
#[case::overflow(PlainDateTime::MAX, (-1).nanoseconds(), None)]
#[case::overflow(PlainDateTime::MAX, SignedDuration::MAX, None)]
#[case::overflow(PlainDateTime::MAX, (-530).weeks(), None)]
fn checked_sub_duration(
    #[case] datetime: PlainDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<PlainDateTime>>,
) {
    assert_eq!(datetime.checked_sub(duration), expected.into());
}

#[rstest]
#[case(datetime!(2021-11-12 17:47), 2.days(), datetime!(2021-11-14 17:47))]
#[case(datetime!(2021-11-12 17:47), (-2).days(), datetime!(2021-11-10 17:47))]
#[case::underflow(PlainDateTime::MIN, (-10).days(), PlainDateTime::MIN)]
#[case::overflow(PlainDateTime::MAX, 10.days(), PlainDateTime::MAX)]
#[case::zero_duration_min(PlainDateTime::MIN, SignedDuration::ZERO, PlainDateTime::MIN)]
#[case::zero_duration_max(PlainDateTime::MAX, SignedDuration::ZERO, PlainDateTime::MAX)]
fn saturating_add_duration(
    #[case] datetime: PlainDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(datetime.saturating_add(duration), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47), 2.days(), datetime!(2021-11-10 17:47))]
#[case(datetime!(2021-11-12 17:47), (-2).days(), datetime!(2021-11-14 17:47))]
#[case::underflow(PlainDateTime::MIN, 10.days(), PlainDateTime::MIN)]
#[case::overflow(PlainDateTime::MAX, (-10).days(), PlainDateTime::MAX)]
#[case::zero_duration_min(PlainDateTime::MIN, SignedDuration::ZERO, PlainDateTime::MIN)]
#[case::zero_duration_max(PlainDateTime::MAX, SignedDuration::ZERO, PlainDateTime::MAX)]
fn saturating_sub_duration(
    #[case] datetime: PlainDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(datetime.saturating_sub(duration), expected);
}
