use std::cmp::Ordering;
use std::time::{Duration as StdDuration, SystemTime};

use rstest::rstest;
use time::Weekday::*;
use time::ext::{NumericalDuration, NumericalStdDuration};
use time::macros::{date, datetime, offset, time, utc_datetime};
use time::{Date, Month, OffsetDateTime, SignedDuration, Time, UtcDateTime, UtcOffset, Weekday};

#[rstest]
fn new() {
    let dt = UtcDateTime::new(date!(2023-12-18), time!(10:13:44.250 AM));
    assert_eq!(dt.year(), 2023);
    assert_eq!(dt.millisecond(), 250);
}

#[rstest]
fn now() {
    assert!(UtcDateTime::now().year() >= 2019);
}

#[rstest]
#[case(utc_datetime!(2000-01-01 0:00), offset!(-1), datetime!(1999-12-31 23:00 -1))]
#[case(utc_datetime!(0000-001 0:00), offset!(UTC), datetime!(0000-001 0:00 UTC))]
fn to_offset(
    #[case] udt: UtcDateTime,
    #[case] offset: UtcOffset,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(udt.to_offset(offset), expected);
}

#[rstest]
#[case(UtcDateTime::MAX, offset!(+1))]
#[case(UtcDateTime::MIN, offset!(-1))]
#[should_panic]
fn to_offset_panic(#[case] udt: UtcDateTime, #[case] offset: UtcOffset) {
    udt.to_offset(offset);
}

#[rstest]
#[case(utc_datetime!(2000-01-01 0:00), offset!(-1), 1999)]
#[case(UtcDateTime::MAX, offset!(+1), None)]
#[case(UtcDateTime::MIN, offset!(-1), None)]
#[case(UtcDateTime::MAX, offset!(+0:00:01), None)]
#[case(UtcDateTime::MIN, offset!(-0:00:01), None)]
fn checked_to_offset(
    #[case] udt: UtcDateTime,
    #[case] offset: UtcOffset,
    #[case] expected: impl Into<Option<i32>>,
) {
    assert_eq!(
        udt.checked_to_offset(offset).map(|odt| odt.year()),
        expected.into()
    );
}

#[rstest]
#[case(0, UtcDateTime::UNIX_EPOCH)]
#[case(1_546_300_800, utc_datetime!(2019-01-01 0:00))]
fn from_unix_timestamp(#[case] timestamp: i64, #[case] expected: UtcDateTime) {
    assert_eq!(UtcDateTime::from_unix_timestamp(timestamp), Ok(expected));
}

#[rstest]
#[case(0, UtcDateTime::UNIX_EPOCH)]
#[case(1_546_300_800_000_000_000, utc_datetime!(2019-01-01 0:00))]
#[case(i128::MAX, None)]
#[case((1i128 << 64) * 1_000_000_000, None)]
fn from_unix_timestamp_nanos(
    #[case] timestamp: i128,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(
        UtcDateTime::from_unix_timestamp_nanos(timestamp).ok(),
        expected.into()
    );
}

#[rstest]
#[case(UtcDateTime::UNIX_EPOCH, 0)]
fn unix_timestamp(#[case] udt: UtcDateTime, #[case] expected: i64) {
    assert_eq!(udt.unix_timestamp(), expected);
}

#[rstest]
#[case(UtcDateTime::UNIX_EPOCH, 0)]
fn unix_timestamp_nanos(#[case] udt: UtcDateTime, #[case] expected: i128) {
    assert_eq!(udt.unix_timestamp_nanos(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), date!(2019-01-01))]
#[case(utc_datetime!(2020-12-31 23:59), date!(2020-12-31))]
fn date(#[case] udt: UtcDateTime, #[case] expected: Date) {
    assert_eq!(udt.date(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), time!(0:00))]
#[case(utc_datetime!(2019-01-01 23:59:59.999), time!(23:59:59.999))]
fn time_(#[case] udt: UtcDateTime, #[case] expected: Time) {
    assert_eq!(udt.time(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 2019)]
#[case(utc_datetime!(2020-12-31 23:59:59), 2020)]
fn year(#[case] udt: UtcDateTime, #[case] expected: i32) {
    assert_eq!(udt.year(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), Month::January)]
#[case(utc_datetime!(2019-12-31 0:00), Month::December)]
fn month(#[case] udt: UtcDateTime, #[case] expected: Month) {
    assert_eq!(udt.month(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 1)]
#[case(utc_datetime!(2019-12-31 0:00), 31)]
fn day(#[case] udt: UtcDateTime, #[case] expected: u8) {
    assert_eq!(udt.day(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 1)]
#[case(utc_datetime!(2020-02-29 0:00), 60)]
fn ordinal(#[case] udt: UtcDateTime, #[case] expected: u16) {
    assert_eq!(udt.ordinal(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 1)]
#[case(utc_datetime!(2020-01-01 0:00), 1)]
#[case(utc_datetime!(2020-12-31 0:00), 53)]
#[case(utc_datetime!(2021-01-01 0:00), 53)]
fn iso_week(#[case] udt: UtcDateTime, #[case] expected: u8) {
    assert_eq!(udt.iso_week(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 0)]
#[case(utc_datetime!(2020-01-01 0:00), 0)]
#[case(utc_datetime!(2020-12-31 0:00), 52)]
#[case(utc_datetime!(2021-01-01 0:00), 0)]
fn sunday_based_week(#[case] udt: UtcDateTime, #[case] expected: u8) {
    assert_eq!(udt.sunday_based_week(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 0)]
#[case(utc_datetime!(2020-01-01 0:00), 0)]
#[case(utc_datetime!(2020-12-31 0:00), 52)]
#[case(utc_datetime!(2021-01-01 0:00), 0)]
fn monday_based_week(#[case] udt: UtcDateTime, #[case] expected: u8) {
    assert_eq!(udt.monday_based_week(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-02 0:00), (2019, Month::January, 2))]
#[case(utc_datetime!(2020-12-31 0:00), (2020, Month::December, 31))]
fn to_calendar_date(#[case] udt: UtcDateTime, #[case] expected: (i32, Month, u8)) {
    assert_eq!(udt.to_calendar_date(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), (2019, 1))]
#[case(utc_datetime!(2020-12-31 0:00), (2020, 366))]
fn to_ordinal_date(#[case] udt: UtcDateTime, #[case] expected: (i32, u16)) {
    assert_eq!(udt.to_ordinal_date(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), (2019, 1, Tuesday))]
#[case(utc_datetime!(2019-10-04 0:00), (2019, 40, Friday))]
#[case(utc_datetime!(2020-01-01 0:00), (2020, 1, Wednesday))]
#[case(utc_datetime!(2020-12-31 0:00), (2020, 53, Thursday))]
#[case(utc_datetime!(2021-01-01 0:00), (2020, 53, Friday))]
fn to_iso_week_date(#[case] udt: UtcDateTime, #[case] expected: (i32, u8, Weekday)) {
    assert_eq!(udt.to_iso_week_date(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), Tuesday)]
#[case(utc_datetime!(2019-02-01 0:00), Friday)]
#[case(utc_datetime!(2019-03-01 0:00), Friday)]
fn weekday(#[case] udt: UtcDateTime, #[case] expected: Weekday) {
    assert_eq!(udt.weekday(), expected);
}

#[rstest]
#[case(utc_datetime!(-999_999-01-01 0:00), -363_521_074)]
#[case(utc_datetime!(-4713-11-24 0:00), 0)]
#[case(utc_datetime!(2000-01-01 0:00), 2_451_545)]
#[case(utc_datetime!(2019-01-01 0:00), 2_458_485)]
#[case(utc_datetime!(2019-12-31 0:00), 2_458_849)]
fn to_julian_day(#[case] udt: UtcDateTime, #[case] expected: i32) {
    assert_eq!(udt.to_julian_day(), expected);
}

#[rstest]
#[case(utc_datetime!(2020-01-01 1:02:03), (1, 2, 3))]
fn as_hms(#[case] udt: UtcDateTime, #[case] expected: (u8, u8, u8)) {
    assert_eq!(udt.as_hms(), expected);
}

#[rstest]
#[case(utc_datetime!(2020-01-01 1:02:03.004), (1, 2, 3, 4))]
fn as_hms_milli(#[case] udt: UtcDateTime, #[case] expected: (u8, u8, u8, u16)) {
    assert_eq!(udt.as_hms_milli(), expected);
}

#[rstest]
#[case(utc_datetime!(2020-01-01 1:02:03.004_005), (1, 2, 3, 4_005))]
fn as_hms_micro(#[case] udt: UtcDateTime, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(udt.as_hms_micro(), expected);
}

#[rstest]
#[case(utc_datetime!(2020-01-01 1:02:03.004_005_006), (1, 2, 3, 4_005_006))]
fn as_hms_nano(#[case] udt: UtcDateTime, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(udt.as_hms_nano(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 0)]
#[case(utc_datetime!(2019-01-01 23:59:59), 23)]
fn hour(#[case] udt: UtcDateTime, #[case] expected: u8) {
    assert_eq!(udt.hour(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 0)]
#[case(utc_datetime!(2019-01-01 0:01:00), 1)]
fn minute(#[case] udt: UtcDateTime, #[case] expected: u8) {
    assert_eq!(udt.minute(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 0)]
#[case(utc_datetime!(2019-01-01 0:00:01), 1)]
fn second(#[case] udt: UtcDateTime, #[case] expected: u8) {
    assert_eq!(udt.second(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 0)]
#[case(utc_datetime!(2019-01-01 23:59:59.999), 999)]
fn millisecond(#[case] udt: UtcDateTime, #[case] expected: u16) {
    assert_eq!(udt.millisecond(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 0)]
#[case(utc_datetime!(2019-01-01 23:59:59.999_999), 999_999)]
fn microsecond(#[case] udt: UtcDateTime, #[case] expected: u32) {
    assert_eq!(udt.microsecond(), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 0)]
#[case(utc_datetime!(2019-01-01 23:59:59.999_999_999), 999_999_999)]
fn nanosecond(#[case] udt: UtcDateTime, #[case] expected: u32) {
    assert_eq!(udt.nanosecond(), expected);
}

#[rstest]
#[case(utc_datetime!(2020-01-01 5:00), time!(12:00), utc_datetime!(2020-01-01 12:00))]
fn replace_time(
    #[case] udt: UtcDateTime,
    #[case] replacement: Time,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(udt.replace_time(replacement), expected);
}

#[rstest]
#[case(utc_datetime!(2020-01-01 12:00), date!(2020-01-30), utc_datetime!(2020-01-30 12:00))]
fn replace_date(
    #[case] udt: UtcDateTime,
    #[case] replacement: Date,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(udt.replace_date(replacement), expected);
}

#[rstest]
#[case(utc_datetime!(2022-02-18 12:00), 2019, utc_datetime!(2019-02-18 12:00))]
#[case(utc_datetime!(2022-02-18 12:00), -1_000_000_000, None)]
#[case(utc_datetime!(2022-02-18 12:00), 1_000_000_000, None)]
fn replace_year(
    #[case] udt: UtcDateTime,
    #[case] year: i32,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_year(year).ok(), expected.into());
}

#[rstest]
#[case(utc_datetime!(2022-02-18 12:00), Month::January, utc_datetime!(2022-01-18 12:00))]
#[case(utc_datetime!(2022-01-30 12:00), Month::February, None)]
fn replace_month(
    #[case] udt: UtcDateTime,
    #[case] month: Month,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_month(month).ok(), expected.into());
}

#[rstest]
#[case(utc_datetime!(2022-02-18 12:00), 1, utc_datetime!(2022-02-01 12:00))]
#[case(utc_datetime!(2022-02-18 12:00), 0, None)]
#[case(utc_datetime!(2022-02-18 12:00), 30, None)]
fn replace_day(
    #[case] udt: UtcDateTime,
    #[case] day: u8,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_day(day).ok(), expected.into());
}

#[rstest]
#[case(utc_datetime!(2022-02-18 12:00), 1, utc_datetime!(2022-001 12:00))]
#[case(utc_datetime!(2024-02-29 12:00), 366, utc_datetime!(2024-366 12:00))]
#[case(utc_datetime!(2022-049 12:00), 0, None)]
#[case(utc_datetime!(2022-049 12:00), 366, None)]
#[case(utc_datetime!(2022-049 12:00), 367, None)]
fn replace_ordinal(
    #[case] udt: UtcDateTime,
    #[case] ordinal: u16,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_ordinal(ordinal).ok(), expected.into());
}

#[rstest]
#[case(
    utc_datetime!(2022-02-18 01:02:03.004_005_006),
    7,
    utc_datetime!(2022-02-18 07:02:03.004_005_006),
)]
#[case(utc_datetime!(2022-02-18 01:02:03.004_005_006), 24, None)]
fn replace_hour(
    #[case] udt: UtcDateTime,
    #[case] value: u8,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_hour(value).ok(), expected.into());
}

#[rstest]
#[case(
    utc_datetime!(2022-02-18 01:02:03.004_005_006),
    7,
    utc_datetime!(2022-02-18 01:07:03.004_005_006),
)]
#[case(utc_datetime!(2022-02-18 01:02:03.004_005_006), 60, None)]
fn replace_minute(
    #[case] udt: UtcDateTime,
    #[case] value: u8,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_minute(value).ok(), expected.into());
}

#[rstest]
#[case(
    utc_datetime!(2022-02-18 01:02:03.004_005_006),
    7,
    utc_datetime!(2022-02-18 01:02:07.004_005_006),
)]
#[case(utc_datetime!(2022-02-18 01:02:03.004_005_006), 60, None)]
fn replace_second(
    #[case] udt: UtcDateTime,
    #[case] value: u8,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_second(value).ok(), expected.into());
}

#[rstest]
#[case(utc_datetime!(2022-02-18 01:02:03.004_005_006), 7, utc_datetime!(2022-02-18 01:02:03.007))]
#[case(utc_datetime!(2022-02-18 01:02:03.004_005_006), 1_000, None)]
fn replace_millisecond(
    #[case] udt: UtcDateTime,
    #[case] value: u16,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_millisecond(value).ok(), expected.into());
}

#[rstest]
#[case(
    utc_datetime!(2022-02-18 01:02:03.004_005_006),
    7_008,
    utc_datetime!(2022-02-18 01:02:03.007_008),
)]
#[case(utc_datetime!(2022-02-18 01:02:03.004_005_006), 1_000_000, None)]
fn replace_microsecond(
    #[case] udt: UtcDateTime,
    #[case] value: u32,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_microsecond(value).ok(), expected.into());
}

#[rstest]
#[case(
    utc_datetime!(2022-02-18 01:02:03.004_005_006),
    7_008_009,
    utc_datetime!(2022-02-18 01:02:03.007_008_009),
)]
#[case(utc_datetime!(2022-02-18 01:02:03.004_005_006), 1_000_000_000, None)]
fn replace_nanosecond(
    #[case] udt: UtcDateTime,
    #[case] value: u32,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(udt.replace_nanosecond(value).ok(), expected.into());
}

#[rstest]
#[case(utc_datetime!(2021-11-12 17:47:53.123_456_789), utc_datetime!(2021-11-12 0:00))]
#[case(utc_datetime!(2021-11-12 0:00), utc_datetime!(2021-11-12 0:00))]
fn truncate_to_day(#[case] udt: UtcDateTime, #[case] expected: UtcDateTime) {
    assert_eq!(udt.truncate_to_day(), expected);
}

#[rstest]
#[case(utc_datetime!(2021-11-12 17:47:53.123_456_789), utc_datetime!(2021-11-12 17:00))]
#[case(utc_datetime!(2021-11-12 0:00), utc_datetime!(2021-11-12 0:00))]
fn truncate_to_hour(#[case] udt: UtcDateTime, #[case] expected: UtcDateTime) {
    assert_eq!(udt.truncate_to_hour(), expected);
}

#[rstest]
#[case(utc_datetime!(2021-11-12 17:47:53.123_456_789), utc_datetime!(2021-11-12 17:47))]
#[case(utc_datetime!(2021-11-12 0:00), utc_datetime!(2021-11-12 0:00))]
fn truncate_to_minute(#[case] udt: UtcDateTime, #[case] expected: UtcDateTime) {
    assert_eq!(udt.truncate_to_minute(), expected);
}

#[rstest]
#[case(utc_datetime!(2021-11-12 17:47:53.123_456_789), utc_datetime!(2021-11-12 17:47:53))]
#[case(utc_datetime!(2021-11-12 0:00), utc_datetime!(2021-11-12 0:00))]
fn truncate_to_second(#[case] udt: UtcDateTime, #[case] expected: UtcDateTime) {
    assert_eq!(udt.truncate_to_second(), expected);
}

#[rstest]
#[case(utc_datetime!(2021-11-12 17:47:53.123_456_789), utc_datetime!(2021-11-12 17:47:53.123))]
#[case(utc_datetime!(2021-11-12 0:00), utc_datetime!(2021-11-12 0:00))]
fn truncate_to_millisecond(#[case] udt: UtcDateTime, #[case] expected: UtcDateTime) {
    assert_eq!(udt.truncate_to_millisecond(), expected);
}

#[rstest]
#[case(utc_datetime!(2021-11-12 17:47:53.123_456_789), utc_datetime!(2021-11-12 17:47:53.123_456))]
#[case(utc_datetime!(2021-11-12 0:00), utc_datetime!(2021-11-12 0:00))]
fn truncate_to_microsecond(#[case] udt: UtcDateTime, #[case] expected: UtcDateTime) {
    assert_eq!(udt.truncate_to_microsecond(), expected);
}

#[rstest]
#[case(utc_datetime!(2000-01-01 0:00), utc_datetime!(2000-01-01 0:00))]
fn partial_eq(#[case] a: UtcDateTime, #[case] b: UtcDateTime) {
    assert_eq!(a, b);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), utc_datetime!(2019-01-01 0:00), Ordering::Equal)]
#[case(utc_datetime!(2020-01-01 0:00), utc_datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(utc_datetime!(2019-01-01 0:00), utc_datetime!(2020-01-01 0:00), Ordering::Less)]
fn partial_ord(#[case] a: UtcDateTime, #[case] b: UtcDateTime, #[case] expected: Ordering) {
    assert_eq!(a.partial_cmp(&b), Some(expected));
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), utc_datetime!(2019-01-01 0:00), Ordering::Equal)]
#[case(
    utc_datetime!(2019-01-01 0:00),
    utc_datetime!(2019-01-01 0:00:00.000_000_001),
    Ordering::Less,
)]
#[case(
    utc_datetime!(2019-01-01 0:00:00.000_000_001),
    utc_datetime!(2019-01-01 0:00),
    Ordering::Greater,
)]
fn ord(#[case] a: UtcDateTime, #[case] b: UtcDateTime, #[case] expected: Ordering) {
    assert_eq!(a.cmp(&b), expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00))]
#[case(utc_datetime!(2020-02-29 12:34:56.789))]
fn hash(#[case] udt: UtcDateTime) {
    use std::collections::hash_map::DefaultHasher;
    use std::hash::{Hash, Hasher};

    assert_eq!(
        {
            let mut hasher = DefaultHasher::new();
            udt.hash(&mut hasher);
            hasher.finish()
        },
        {
            let mut hasher = DefaultHasher::new();
            udt.hash(&mut hasher);
            hasher.finish()
        }
    );
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 5.days(), utc_datetime!(2019-01-06 0:00))]
#[case(utc_datetime!(2019-12-31 0:00), 1.days(), utc_datetime!(2020-01-01 0:00))]
#[case(utc_datetime!(2019-12-31 23:59:59), 2.seconds(), utc_datetime!(2020-01-01 0:00:01))]
#[case(utc_datetime!(2020-01-01 0:00:01), (-2).seconds(), utc_datetime!(2019-12-31 23:59:59))]
#[case(utc_datetime!(1999-12-31 23:00), 1.hours(), utc_datetime!(2000-01-01 0:00))]
fn add_duration(
    #[case] udt: UtcDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(udt + duration, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 5.std_days(), utc_datetime!(2019-01-06 0:00))]
#[case(utc_datetime!(2019-12-31 0:00), 1.std_days(), utc_datetime!(2020-01-01 0:00))]
#[case(utc_datetime!(2019-12-31 23:59:59), 2.std_seconds(), utc_datetime!(2020-01-01 0:00:01))]
fn add_std_duration(
    #[case] udt: UtcDateTime,
    #[case] duration: StdDuration,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(udt + duration, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 5.days(), utc_datetime!(2019-01-06 0:00))]
#[case(utc_datetime!(2019-12-31 0:00), 1.days(), utc_datetime!(2020-01-01 0:00))]
#[case(utc_datetime!(2019-12-31 23:59:59), 2.seconds(), utc_datetime!(2020-01-01 0:00:01))]
#[case(utc_datetime!(2020-01-01 0:00:01), (-2).seconds(), utc_datetime!(2019-12-31 23:59:59))]
fn add_assign_duration(
    #[case] mut udt: UtcDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    udt += duration;
    assert_eq!(udt, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), 5.std_days(), utc_datetime!(2019-01-06 0:00))]
#[case(utc_datetime!(2019-12-31 0:00), 1.std_days(), utc_datetime!(2020-01-01 0:00))]
#[case(utc_datetime!(2019-12-31 23:59:59), 2.std_seconds(), utc_datetime!(2020-01-01 0:00:01))]
fn add_assign_std_duration(
    #[case] mut udt: UtcDateTime,
    #[case] duration: StdDuration,
    #[case] expected: UtcDateTime,
) {
    udt += duration;
    assert_eq!(udt, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-06 0:00), 5.days(), utc_datetime!(2019-01-01 0:00))]
#[case(utc_datetime!(2020-01-01 0:00), 1.days(), utc_datetime!(2019-12-31 0:00))]
#[case(utc_datetime!(2020-01-01 0:00:01), 2.seconds(), utc_datetime!(2019-12-31 23:59:59))]
#[case(utc_datetime!(2019-12-31 23:59:59), (-2).seconds(), utc_datetime!(2020-01-01 0:00:01))]
#[case(utc_datetime!(1999-12-31 23:00), (-1).hours(), utc_datetime!(2000-01-01 0:00))]
fn sub_duration(
    #[case] udt: UtcDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(udt - duration, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-06 0:00), 5.std_days(), utc_datetime!(2019-01-01 0:00))]
#[case(utc_datetime!(2020-01-01 0:00), 1.std_days(), utc_datetime!(2019-12-31 0:00))]
#[case(utc_datetime!(2020-01-01 0:00:01), 2.std_seconds(), utc_datetime!(2019-12-31 23:59:59))]
fn sub_std_duration(
    #[case] udt: UtcDateTime,
    #[case] duration: StdDuration,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(udt - duration, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-06 0:00), 5.days(), utc_datetime!(2019-01-01 0:00))]
#[case(utc_datetime!(2020-01-01 0:00), 1.days(), utc_datetime!(2019-12-31 0:00))]
#[case(utc_datetime!(2020-01-01 0:00:01), 2.seconds(), utc_datetime!(2019-12-31 23:59:59))]
#[case(utc_datetime!(2019-12-31 23:59:59), (-2).seconds(), utc_datetime!(2020-01-01 0:00:01))]
fn sub_assign_duration(
    #[case] mut udt: UtcDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    udt -= duration;
    assert_eq!(udt, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-06 0:00), 5.std_days(), utc_datetime!(2019-01-01 0:00))]
#[case(utc_datetime!(2020-01-01 0:00), 1.std_days(), utc_datetime!(2019-12-31 0:00))]
#[case(utc_datetime!(2020-01-01 0:00:01), 2.std_seconds(), utc_datetime!(2019-12-31 23:59:59))]
fn sub_assign_std_duration(
    #[case] mut udt: UtcDateTime,
    #[case] duration: StdDuration,
    #[case] expected: UtcDateTime,
) {
    udt -= duration;
    assert_eq!(udt, expected);
}

#[rstest]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    0.seconds(),
    utc_datetime!(2019-01-01 0:00),
)]
#[case(SystemTime::from(utc_datetime!(2019-01-01 0:00)), 5.days(), utc_datetime!(2019-01-06 0:00))]
#[case(SystemTime::from(utc_datetime!(2019-12-31 0:00)), 1.days(), utc_datetime!(2020-01-01 0:00))]
#[case(
    SystemTime::from(utc_datetime!(2019-12-31 23:59:59)),
    2.seconds(),
    utc_datetime!(2020-01-01 0:00:01),
)]
#[case(
    SystemTime::from(utc_datetime!(2020-01-01 0:00:01)),
    (-2).seconds(),
    utc_datetime!(2019-12-31 23:59:59),
)]
fn std_add_duration(
    #[case] lhs: SystemTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(lhs + duration, expected);
}

#[rstest]
#[case(SystemTime::from(utc_datetime!(2019-01-01 0:00)), 5.days(), utc_datetime!(2019-01-06 0:00))]
#[case(SystemTime::from(utc_datetime!(2019-12-31 0:00)), 1.days(), utc_datetime!(2020-01-01 0:00))]
#[case(
    SystemTime::from(utc_datetime!(2019-12-31 23:59:59)),
    2.seconds(),
    utc_datetime!(2020-01-01 0:00:01),
)]
#[case(
    SystemTime::from(utc_datetime!(2020-01-01 0:00:01)),
    (-2).seconds(),
    utc_datetime!(2019-12-31 23:59:59),
)]
fn std_add_assign_duration(
    #[case] mut lhs: SystemTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    lhs += duration;
    assert_eq!(lhs, expected);
}

#[rstest]
#[case(SystemTime::from(utc_datetime!(2019-01-06 0:00)), 5.days(), utc_datetime!(2019-01-01 0:00))]
#[case(SystemTime::from(utc_datetime!(2020-01-01 0:00)), 1.days(), utc_datetime!(2019-12-31 0:00))]
#[case(
    SystemTime::from(utc_datetime!(2020-01-01 0:00:01)),
    2.seconds(),
    utc_datetime!(2019-12-31 23:59:59),
)]
#[case(
    SystemTime::from(utc_datetime!(2019-12-31 23:59:59)),
    (-2).seconds(),
    utc_datetime!(2020-01-01 0:00:01),
)]
fn std_sub_duration(
    #[case] lhs: SystemTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(lhs - duration, expected);
}

#[rstest]
#[case(SystemTime::from(utc_datetime!(2019-01-06 0:00)), 5.days(), utc_datetime!(2019-01-01 0:00))]
#[case(SystemTime::from(utc_datetime!(2020-01-01 0:00)), 1.days(), utc_datetime!(2019-12-31 0:00))]
#[case(
    SystemTime::from(utc_datetime!(2020-01-01 0:00:01)),
    2.seconds(),
    utc_datetime!(2019-12-31 23:59:59),
)]
#[case(
    SystemTime::from(utc_datetime!(2019-12-31 23:59:59)),
    (-2).seconds(),
    utc_datetime!(2020-01-01 0:00:01),
)]
fn std_sub_assign_duration(
    #[case] mut lhs: SystemTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    lhs -= duration;
    assert_eq!(lhs, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-02 0:00), utc_datetime!(2019-01-01 0:00), 1.days())]
#[case(utc_datetime!(2019-01-01 0:00), utc_datetime!(2019-01-02 0:00), (-1).days())]
#[case(utc_datetime!(2020-01-01 0:00), utc_datetime!(2019-12-31 0:00), 1.days())]
#[case(utc_datetime!(2019-12-31 0:00), utc_datetime!(2020-01-01 0:00), (-1).days())]
fn sub_self(#[case] lhs: UtcDateTime, #[case] rhs: UtcDateTime, #[case] expected: SignedDuration) {
    assert_eq!(lhs - rhs, expected);
}

#[rstest]
#[case(SystemTime::from(utc_datetime!(2019-01-02 0:00)), utc_datetime!(2019-01-01 0:00), 1.days())]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    utc_datetime!(2019-01-02 0:00),
    (-1).days(),
)]
#[case(SystemTime::from(utc_datetime!(2020-01-01 0:00)), utc_datetime!(2019-12-31 0:00), 1.days())]
#[case(
    SystemTime::from(utc_datetime!(2019-12-31 0:00)),
    utc_datetime!(2020-01-01 0:00),
    (-1).days(),
)]
fn std_sub(#[case] lhs: SystemTime, #[case] rhs: UtcDateTime, #[case] expected: SignedDuration) {
    assert_eq!(lhs - rhs, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-02 0:00), SystemTime::from(utc_datetime!(2019-01-01 0:00)), 1.days())]
#[case(
    utc_datetime!(2019-01-01 0:00),
    SystemTime::from(utc_datetime!(2019-01-02 0:00)),
    (-1).days(),
)]
#[case(utc_datetime!(2020-01-01 0:00), SystemTime::from(utc_datetime!(2019-12-31 0:00)), 1.days())]
#[case(
    utc_datetime!(2019-12-31 0:00),
    SystemTime::from(utc_datetime!(2020-01-01 0:00)),
    (-1).days(),
)]
fn sub_std(#[case] udt: UtcDateTime, #[case] rhs: SystemTime, #[case] expected: SignedDuration) {
    assert_eq!(udt - rhs, expected);
}

#[rstest]
#[case(datetime!(2019-01-02 0:00 UTC), utc_datetime!(2019-01-01 0:00), 1.days())]
#[case(datetime!(2019-01-01 0:00 UTC), utc_datetime!(2019-01-02 0:00), (-1).days())]
#[case(datetime!(2020-01-01 0:00 UTC), utc_datetime!(2019-12-31 0:00), 1.days())]
#[case(datetime!(2019-12-31 0:00 UTC), utc_datetime!(2020-01-01 0:00), (-1).days())]
fn odt_sub(
    #[case] lhs: OffsetDateTime,
    #[case] rhs: UtcDateTime,
    #[case] expected: SignedDuration,
) {
    assert_eq!(lhs - rhs, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-02 0:00), datetime!(2019-01-01 0:00 UTC), 1.days())]
#[case(utc_datetime!(2019-01-01 0:00), datetime!(2019-01-02 0:00 UTC), (-1).days())]
#[case(utc_datetime!(2020-01-01 0:00), datetime!(2019-12-31 0:00 UTC), 1.days())]
#[case(utc_datetime!(2019-12-31 0:00), datetime!(2020-01-01 0:00 UTC), (-1).days())]
fn sub_odt(
    #[case] lhs: UtcDateTime,
    #[case] rhs: OffsetDateTime,
    #[case] expected: SignedDuration,
) {
    assert_eq!(lhs - rhs, expected);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00))]
fn eq_std(#[case] now_datetime: UtcDateTime) {
    let now_systemtime = SystemTime::from(now_datetime);
    assert_eq!(now_datetime, now_systemtime);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00))]
fn std_eq(#[case] now_datetime: UtcDateTime) {
    let now_systemtime = SystemTime::from(now_datetime);
    assert_eq!(now_systemtime, now_datetime);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00))]
fn eq_odt(#[case] now_datetime: UtcDateTime) {
    let now_odt = OffsetDateTime::from(now_datetime);
    assert_eq!(now_datetime, now_odt);
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00))]
fn odt_eq(#[case] now_datetime: UtcDateTime) {
    let now_odt = OffsetDateTime::from(now_datetime);
    assert_eq!(now_odt, now_datetime);
}

#[rstest]
#[case(
    utc_datetime!(2019-01-01 0:00),
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    Ordering::Equal,
)]
#[case(
    utc_datetime!(2019-01-01 0:00),
    SystemTime::from(utc_datetime!(2020-01-01 0:00)),
    Ordering::Less,
)]
#[case(
    utc_datetime!(2019-01-01 0:00),
    SystemTime::from(utc_datetime!(2019-02-01 0:00)),
    Ordering::Less,
)]
#[case(
    utc_datetime!(2019-01-01 0:00),
    SystemTime::from(utc_datetime!(2019-01-02 0:00)),
    Ordering::Less,
)]
#[case(
    utc_datetime!(2019-01-01 0:00),
    SystemTime::from(utc_datetime!(2019-01-01 1:00:00)),
    Ordering::Less,
)]
#[case(
    utc_datetime!(2019-01-01 0:00),
    SystemTime::from(utc_datetime!(2019-01-01 0:01:00)),
    Ordering::Less,
)]
#[case(
    utc_datetime!(2019-01-01 0:00),
    SystemTime::from(utc_datetime!(2019-01-01 0:00:01)),
    Ordering::Less,
)]
#[case(
    utc_datetime!(2019-01-01 0:00),
    SystemTime::from(utc_datetime!(2019-01-01 0:00:00.001)),
    Ordering::Less,
)]
#[case(
    utc_datetime!(2020-01-01 0:00),
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    Ordering::Greater,
)]
#[case(
    utc_datetime!(2019-02-01 0:00),
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    Ordering::Greater,
)]
#[case(
    utc_datetime!(2019-01-02 0:00),
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    Ordering::Greater,
)]
#[case(
    utc_datetime!(2019-01-01 1:00:00),
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    Ordering::Greater,
)]
#[case(
    utc_datetime!(2019-01-01 0:01:00),
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    Ordering::Greater,
)]
#[case(
    utc_datetime!(2019-01-01 0:00:01),
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    Ordering::Greater,
)]
#[case(
    utc_datetime!(2019-01-01 0:00:00.000_000_001),
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    Ordering::Greater,
)]
fn ord_std(#[case] lhs: UtcDateTime, #[case] rhs: SystemTime, #[case] expected: Ordering) {
    assert_eq!(lhs.partial_cmp(&rhs), Some(expected));
}

#[rstest]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    utc_datetime!(2019-01-01 0:00),
    Ordering::Equal,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    utc_datetime!(2020-01-01 0:00),
    Ordering::Less,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    utc_datetime!(2019-02-01 0:00),
    Ordering::Less,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    utc_datetime!(2019-01-02 0:00),
    Ordering::Less,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    utc_datetime!(2019-01-01 1:00:00),
    Ordering::Less,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    utc_datetime!(2019-01-01 0:01:00),
    Ordering::Less,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    utc_datetime!(2019-01-01 0:00:01),
    Ordering::Less,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00)),
    utc_datetime!(2019-01-01 0:00:00.000_000_001),
    Ordering::Less,
)]
#[case(
    SystemTime::from(utc_datetime!(2020-01-01 0:00)),
    utc_datetime!(2019-01-01 0:00),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-02-01 0:00)),
    utc_datetime!(2019-01-01 0:00),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-02 0:00)),
    utc_datetime!(2019-01-01 0:00),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 1:00:00)),
    utc_datetime!(2019-01-01 0:00),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:01:00)),
    utc_datetime!(2019-01-01 0:00),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00:01)),
    utc_datetime!(2019-01-01 0:00),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(utc_datetime!(2019-01-01 0:00:00.001)),
    utc_datetime!(2019-01-01 0:00),
    Ordering::Greater,
)]
fn std_ord(#[case] lhs: SystemTime, #[case] rhs: UtcDateTime, #[case] expected: Ordering) {
    assert_eq!(lhs.partial_cmp(&rhs), Some(expected));
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), utc_datetime!(2019-01-01 0:00), Ordering::Equal)]
#[case(datetime!(2019-01-01 0:00 UTC), utc_datetime!(2020-01-01 0:00), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00 UTC), utc_datetime!(2019-02-01 0:00), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00 UTC), utc_datetime!(2019-01-02 0:00), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00 UTC), utc_datetime!(2019-01-01 1:00:00), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00 UTC), utc_datetime!(2019-01-01 0:01:00), Ordering::Less)]
#[case(datetime!(2019-01-01 0:00 UTC), utc_datetime!(2019-01-01 0:00:01), Ordering::Less)]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    utc_datetime!(2019-01-01 0:00:00.000_000_001),
    Ordering::Less,
)]
#[case(datetime!(2020-01-01 0:00 UTC), utc_datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-02-01 0:00 UTC), utc_datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-02 0:00 UTC), utc_datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-01 1:00:00 UTC), utc_datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-01 0:01:00 UTC), utc_datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-01 0:00:01 UTC), utc_datetime!(2019-01-01 0:00), Ordering::Greater)]
#[case(datetime!(2019-01-01 0:00:00.001 UTC), utc_datetime!(2019-01-01 0:00), Ordering::Greater)]
fn odt_ord(#[case] lhs: OffsetDateTime, #[case] rhs: UtcDateTime, #[case] expected: Ordering) {
    assert_eq!(lhs.partial_cmp(&rhs), Some(expected));
}

#[rstest]
#[case(utc_datetime!(2019-01-01 0:00), datetime!(2019-01-01 0:00 UTC), Ordering::Equal)]
#[case(utc_datetime!(2019-01-01 0:00), datetime!(2020-01-01 0:00 UTC), Ordering::Less)]
#[case(utc_datetime!(2019-01-01 0:00), datetime!(2019-02-01 0:00 UTC), Ordering::Less)]
#[case(utc_datetime!(2019-01-01 0:00), datetime!(2019-01-02 0:00 UTC), Ordering::Less)]
#[case(utc_datetime!(2019-01-01 0:00), datetime!(2019-01-01 1:00:00 UTC), Ordering::Less)]
#[case(utc_datetime!(2019-01-01 0:00), datetime!(2019-01-01 0:01:00 UTC), Ordering::Less)]
#[case(utc_datetime!(2019-01-01 0:00), datetime!(2019-01-01 0:00:01 UTC), Ordering::Less)]
#[case(utc_datetime!(2019-01-01 0:00), datetime!(2019-01-01 0:00:00.001 UTC), Ordering::Less)]
#[case(utc_datetime!(2020-01-01 0:00), datetime!(2019-01-01 0:00 UTC), Ordering::Greater)]
#[case(utc_datetime!(2019-02-01 0:00), datetime!(2019-01-01 0:00 UTC), Ordering::Greater)]
#[case(utc_datetime!(2019-01-02 0:00), datetime!(2019-01-01 0:00 UTC), Ordering::Greater)]
#[case(utc_datetime!(2019-01-01 1:00:00), datetime!(2019-01-01 0:00 UTC), Ordering::Greater)]
#[case(utc_datetime!(2019-01-01 0:01:00), datetime!(2019-01-01 0:00 UTC), Ordering::Greater)]
#[case(utc_datetime!(2019-01-01 0:00:01), datetime!(2019-01-01 0:00 UTC), Ordering::Greater)]
#[case(
    utc_datetime!(2019-01-01 0:00:00.000_000_001),
    datetime!(2019-01-01 0:00 UTC),
    Ordering::Greater,
)]
fn ord_odt(#[case] lhs: UtcDateTime, #[case] rhs: OffsetDateTime, #[case] expected: Ordering) {
    assert_eq!(lhs.partial_cmp(&rhs), Some(expected));
}

#[rstest]
#[case(SystemTime::UNIX_EPOCH)]
#[case(SystemTime::UNIX_EPOCH - 1.std_days())]
#[case(SystemTime::UNIX_EPOCH + 1.std_days())]
fn from_std(#[case] input: SystemTime) {
    assert_eq!(UtcDateTime::from(input), input);
}

#[rstest]
#[case(UtcDateTime::UNIX_EPOCH)]
#[case(UtcDateTime::UNIX_EPOCH + 1.days())]
#[case(UtcDateTime::UNIX_EPOCH - 1.days())]
fn to_std(#[case] input: UtcDateTime) {
    assert_eq!(SystemTime::from(input), input);
}

#[rstest]
#[case(OffsetDateTime::UNIX_EPOCH)]
#[case(OffsetDateTime::UNIX_EPOCH - 1.std_days())]
#[case(OffsetDateTime::UNIX_EPOCH + 1.std_days())]
fn from_odt(#[case] input: OffsetDateTime) {
    assert_eq!(UtcDateTime::from(input), input);
}

#[rstest]
#[case(UtcDateTime::UNIX_EPOCH)]
#[case(UtcDateTime::UNIX_EPOCH + 1.days())]
#[case(UtcDateTime::UNIX_EPOCH - 1.days())]
fn to_odt(#[case] input: UtcDateTime) {
    assert_eq!(OffsetDateTime::from(input), input);
}

#[rstest]
#[case(
    utc_datetime!(2021-10-25 14:01:53.45),
    5.nanoseconds(),
    utc_datetime!(2021-10-25 14:01:53.450_000_005),
)]
#[case(utc_datetime!(2021-10-25 14:01:53.45), 4.seconds(), utc_datetime!(2021-10-25 14:01:57.45))]
#[case(utc_datetime!(2021-10-25 14:01:53.45), 2.days(), utc_datetime!(2021-10-27 14:01:53.45))]
#[case(utc_datetime!(2021-10-25 14:01:53.45), 1.weeks(), utc_datetime!(2021-11-01 14:01:53.45))]
#[case(
    utc_datetime!(2021-10-25 14:01:53.45),
    (-5).nanoseconds(),
    utc_datetime!(2021-10-25 14:01:53.449_999_995),
)]
#[case(
    utc_datetime!(2021-10-25 14:01:53.45),
    (-4).seconds(),
    utc_datetime!(2021-10-25 14:01:49.45),
)]
#[case(utc_datetime!(2021-10-25 14:01:53.45), (-2).days(), utc_datetime!(2021-10-23 14:01:53.45))]
#[case(utc_datetime!(2021-10-25 14:01:53.45), (-1).weeks(), utc_datetime!(2021-10-18 14:01:53.45))]
#[case(UtcDateTime::MIN, (-1).nanoseconds(), None)]
#[case(UtcDateTime::MIN, SignedDuration::MIN, None)]
#[case(UtcDateTime::MIN, (-530).weeks(), None)]
#[case(UtcDateTime::MAX, 1.nanoseconds(), None)]
#[case(UtcDateTime::MAX, SignedDuration::MAX, None)]
#[case(UtcDateTime::MAX, 530.weeks(), None)]
fn checked_add_duration(
    #[case] input: UtcDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(input.checked_add(duration), expected.into());
}

#[rstest]
#[case(
    utc_datetime!(2021-10-25 14:01:53.45),
    (-5).nanoseconds(),
    utc_datetime!(2021-10-25 14:01:53.450_000_005),
)]
#[case(
    utc_datetime!(2021-10-25 14:01:53.45),
    (-4).seconds(),
    utc_datetime!(2021-10-25 14:01:57.45),
)]
#[case(utc_datetime!(2021-10-25 14:01:53.45), (-2).days(), utc_datetime!(2021-10-27 14:01:53.45))]
#[case(utc_datetime!(2021-10-25 14:01:53.45), (-1).weeks(), utc_datetime!(2021-11-01 14:01:53.45))]
#[case(
    utc_datetime!(2021-10-25 14:01:53.45),
    5.nanoseconds(),
    utc_datetime!(2021-10-25 14:01:53.449_999_995),
)]
#[case(utc_datetime!(2021-10-25 14:01:53.45), 4.seconds(), utc_datetime!(2021-10-25 14:01:49.45))]
#[case(utc_datetime!(2021-10-25 14:01:53.45), 2.days(), utc_datetime!(2021-10-23 14:01:53.45))]
#[case(utc_datetime!(2021-10-25 14:01:53.45), 1.weeks(), utc_datetime!(2021-10-18 14:01:53.45))]
#[case(UtcDateTime::MIN, 1.nanoseconds(), None)]
#[case(UtcDateTime::MIN, SignedDuration::MAX, None)]
#[case(UtcDateTime::MIN, 530.weeks(), None)]
#[case(UtcDateTime::MAX, (-1).nanoseconds(), None)]
#[case(UtcDateTime::MAX, SignedDuration::MIN, None)]
#[case(UtcDateTime::MAX, (-530).weeks(), None)]
#[case(UtcDateTime::MAX, SignedDuration::ZERO, UtcDateTime::MAX)]
#[case(UtcDateTime::MIN, SignedDuration::ZERO, UtcDateTime::MIN)]
fn checked_sub_duration(
    #[case] input: UtcDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<UtcDateTime>>,
) {
    assert_eq!(input.checked_sub(duration), expected.into());
}

#[rstest]
#[case(utc_datetime!(2021-11-12 17:47), 2.days(), utc_datetime!(2021-11-14 17:47))]
#[case(utc_datetime!(2021-11-12 17:47), (-2).days(), utc_datetime!(2021-11-10 17:47))]
#[case(UtcDateTime::MIN, (-10).days(), UtcDateTime::MIN)]
#[case(UtcDateTime::MAX, 10.days(), UtcDateTime::MAX)]
#[case(UtcDateTime::MIN, SignedDuration::ZERO, UtcDateTime::MIN)]
#[case(UtcDateTime::MAX, SignedDuration::ZERO, UtcDateTime::MAX)]
fn saturating_add_duration(
    #[case] input: UtcDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(input.saturating_add(duration), expected);
}

#[rstest]
#[case(utc_datetime!(2021-11-12 17:47), 2.days(), utc_datetime!(2021-11-10 17:47))]
#[case(utc_datetime!(2021-11-12 17:47), (-2).days(), utc_datetime!(2021-11-14 17:47))]
#[case(UtcDateTime::MIN, 10.days(), UtcDateTime::MIN)]
#[case(UtcDateTime::MAX, (-10).days(), UtcDateTime::MAX)]
#[case(UtcDateTime::MIN, SignedDuration::ZERO, UtcDateTime::MIN)]
#[case(UtcDateTime::MAX, SignedDuration::ZERO, UtcDateTime::MAX)]
fn saturating_sub_duration(
    #[case] input: UtcDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(input.saturating_sub(duration), expected);
}

#[rstest]
#[should_panic = "overflow adding duration to date"]
fn issue_621() {
    let _ = UtcDateTime::UNIX_EPOCH + StdDuration::from_secs(18_157_382_926_370_278_155);
}

#[rstest]
fn to_offset_regression() {
    let value = utc_datetime!(0000-01-01 23:59).to_offset(offset!(+24:59));
    assert_eq!(value, datetime!(0000-01-03 0:58 +24:59));
}
