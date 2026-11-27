use std::cmp::Ordering;
use std::time::{Duration as StdDuration, SystemTime};

use rstest::rstest;
use time::Weekday::*;
use time::ext::{NumericalDuration, NumericalStdDuration};
use time::macros::{date, datetime, offset, time, utc_datetime};
use time::{
    Date, Month, OffsetDateTime, PlainDateTime, SignedDuration, Time, UtcDateTime, UtcOffset,
    Weekday,
};

#[rstest]
fn new_utc() {
    let dt = OffsetDateTime::new_utc(date!(2023-12-18), time!(10:13:44.250 AM));
    assert_eq!(dt.year(), 2023);
    assert_eq!(dt.millisecond(), 250);
    assert_eq!(dt.offset(), offset!(UTC));
}

#[rstest]
fn new_in_offset() {
    let dt = OffsetDateTime::new_in_offset(date!(2023-12-18), time!(10:13:44.250 AM), offset!(-4));
    assert_eq!(dt.year(), 2023);
    assert_eq!(dt.millisecond(), 250);
    assert_eq!(dt.offset().whole_hours(), -4);
}

#[rstest]
fn now_utc() {
    assert!(OffsetDateTime::now_utc().year() >= 2019);
    assert_eq!(OffsetDateTime::now_utc().offset(), offset!(UTC));
}

#[rstest]
fn now_local() {
    assert!(OffsetDateTime::now_local().is_ok());
}

#[rstest]
#[case(datetime!(2000-01-01 0:00 UTC), offset!(-1), datetime!(1999-12-31 23:00 -1))]
#[case(
    datetime!(0000-001 0:00 +0:00:02),
    offset!(-0:00:59),
    datetime!(-0001-365 23:58:59 -0:00:59),
)]
#[case(datetime!(0000-001 0:00 UTC), offset!(UTC), datetime!(0000-001 0:00 UTC))]
fn to_offset(
    #[case] input: OffsetDateTime,
    #[case] offset: UtcOffset,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input.to_offset(offset), expected);
}

#[rstest]
#[case(PlainDateTime::MAX.assume_utc(), offset!(+1))]
#[case(PlainDateTime::MIN.assume_utc(), offset!(-1))]
#[should_panic]
fn to_offset_panic(#[case] input: OffsetDateTime, #[case] offset: UtcOffset) {
    input.to_offset(offset);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 +13), offset!(-13), datetime!(2018-12-30 22:00:00 -13))]
fn to_offset_invalid_regression(
    #[case] input: OffsetDateTime,
    #[case] offset: UtcOffset,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input.to_offset(offset), expected);
}

#[rstest]
#[case(datetime!(2000-01-01 0:00 UTC), offset!(-1), 1999)]
#[case(PlainDateTime::MAX.assume_utc(), offset!(+1), None)]
#[case(PlainDateTime::MIN.assume_utc(), offset!(-1), None)]
#[case(PlainDateTime::MAX.assume_utc(), offset!(+0:00:01), None)]
#[case(PlainDateTime::MIN.assume_utc(), offset!(-0:00:01), None)]
fn checked_to_offset(
    #[case] input: OffsetDateTime,
    #[case] offset: UtcOffset,
    #[case] expected: impl Into<Option<i32>>,
) {
    assert_eq!(
        input.checked_to_offset(offset).map(|odt| odt.year()),
        expected.into()
    );
}

#[rstest]
#[case(datetime!(2000-01-01 0:00 +1), utc_datetime!(1999-12-31 23:00))]
#[case(datetime!(0000-001 0:00 UTC), utc_datetime!(0000-001 0:00))]
fn to_utc(#[case] input: OffsetDateTime, #[case] expected: UtcDateTime) {
    assert_eq!(input.to_utc(), expected);
}

#[rstest]
#[case(PlainDateTime::MAX.assume_offset(offset!(-1)))]
#[case(PlainDateTime::MIN.assume_offset(offset!(+1)))]
#[should_panic]
fn to_utc_panic(#[case] input: OffsetDateTime) {
    input.to_utc();
}

#[rstest]
#[case(datetime!(2000-01-01 0:00 +1), 1999)]
#[case(datetime!(+999999-12-31 23:59:59 -1), None)]
#[case(datetime!(-999999-01-01 00:00:00 +1), None)]
#[case(PlainDateTime::MAX.assume_offset(offset!(-1)), None)]
#[case(PlainDateTime::MIN.assume_offset(offset!(+1)), None)]
fn checked_to_utc(#[case] input: OffsetDateTime, #[case] expected: impl Into<Option<i32>>) {
    assert_eq!(
        input.checked_to_utc().map(|udt| udt.year()),
        expected.into()
    );
}

#[rstest]
#[case(0, OffsetDateTime::UNIX_EPOCH)]
#[case(1_546_300_800, datetime!(2019-01-01 0:00 UTC))]
fn from_unix_timestamp(#[case] input: i64, #[case] expected: OffsetDateTime) {
    assert_eq!(OffsetDateTime::from_unix_timestamp(input), Ok(expected));
}

#[rstest]
#[case(0, OffsetDateTime::UNIX_EPOCH)]
#[case(1_546_300_800_000_000_000, datetime!(2019-01-01 0:00 UTC))]
#[case(i128::MAX, None)]
#[case((1i128 << 64) * 1_000_000_000, None)]
fn from_unix_timestamp_nanos(
    #[case] input: i128,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(
        OffsetDateTime::from_unix_timestamp_nanos(input).ok(),
        expected.into()
    );
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), offset!(UTC))]
#[case(datetime!(2019-01-01 0:00 +1), offset!(+1))]
#[case(datetime!(2019-01-01 0:00 UTC).to_offset(offset!(+1)), offset!(+1))]
fn offset(#[case] input: OffsetDateTime, #[case] expected: UtcOffset) {
    assert_eq!(input.offset(), expected);
}

#[rstest]
#[case(OffsetDateTime::UNIX_EPOCH, 0)]
#[case(OffsetDateTime::UNIX_EPOCH.to_offset(offset!(+1)), 0)]
#[case(datetime!(1970-01-01 0:00 -1), 3_600)]
fn unix_timestamp(#[case] input: OffsetDateTime, #[case] expected: i64) {
    assert_eq!(input.unix_timestamp(), expected);
}

#[rstest]
#[case(datetime!(1970-01-01 0:00 UTC), 0)]
#[case(datetime!(1970-01-01 1:00 UTC).to_offset(offset!(-1)), 3_600_000_000_000)]
fn unix_timestamp_nanos(#[case] input: OffsetDateTime, #[case] expected: i128) {
    assert_eq!(input.unix_timestamp_nanos(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), date!(2019-01-01))]
#[case(datetime!(2019-01-01 0:00 UTC).to_offset(offset!(-1)), date!(2018-12-31))]
fn date(#[case] input: OffsetDateTime, #[case] expected: Date) {
    assert_eq!(input.date(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), time!(0:00))]
#[case(datetime!(2019-01-01 0:00 UTC).to_offset(offset!(-1)), time!(23:00))]
fn time_(#[case] input: OffsetDateTime, #[case] expected: Time) {
    assert_eq!(input.time(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 2019)]
#[case(datetime!(2019-12-31 23:00 UTC).to_offset(offset!(+1)), 2020)]
#[case(datetime!(2020-01-01 0:00 UTC), 2020)]
fn year(#[case] input: OffsetDateTime, #[case] expected: i32) {
    assert_eq!(input.year(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), Month::January)]
#[case(datetime!(2019-12-31 23:00 UTC).to_offset(offset!(+1)), Month::January)]
fn month(#[case] input: OffsetDateTime, #[case] expected: Month) {
    assert_eq!(input.month(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 1)]
#[case(datetime!(2019-12-31 23:00 UTC).to_offset(offset!(+1)), 1)]
fn day(#[case] input: OffsetDateTime, #[case] expected: u8) {
    assert_eq!(input.day(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 1)]
#[case(datetime!(2019-12-31 23:00 UTC).to_offset(offset!(+1)), 1)]
fn ordinal(#[case] input: OffsetDateTime, #[case] expected: u16) {
    assert_eq!(input.ordinal(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 1)]
#[case(datetime!(2020-01-01 0:00 UTC), 1)]
#[case(datetime!(2020-12-31 0:00 UTC), 53)]
#[case(datetime!(2021-01-01 0:00 UTC), 53)]
fn iso_week(#[case] input: OffsetDateTime, #[case] expected: u8) {
    assert_eq!(input.iso_week(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 0)]
#[case(datetime!(2020-01-01 0:00 UTC), 0)]
#[case(datetime!(2020-12-31 0:00 UTC), 52)]
#[case(datetime!(2021-01-01 0:00 UTC), 0)]
fn sunday_based_week(#[case] input: OffsetDateTime, #[case] expected: u8) {
    assert_eq!(input.sunday_based_week(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 0)]
#[case(datetime!(2020-01-01 0:00 UTC), 0)]
#[case(datetime!(2020-12-31 0:00 UTC), 52)]
#[case(datetime!(2021-01-01 0:00 UTC), 0)]
fn monday_based_week(#[case] input: OffsetDateTime, #[case] expected: u8) {
    assert_eq!(input.monday_based_week(), expected);
}

#[rstest]
#[case(datetime!(2019-01-02 0:00 UTC), (2019, Month::January, 2))]
fn to_calendar_date(#[case] input: OffsetDateTime, #[case] expected: (i32, Month, u8)) {
    assert_eq!(input.to_calendar_date(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), (2019, 1))]
fn to_ordinal_date(#[case] input: OffsetDateTime, #[case] expected: (i32, u16)) {
    assert_eq!(input.to_ordinal_date(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), (2019, 1, Tuesday))]
#[case(datetime!(2019-10-04 0:00 UTC), (2019, 40, Friday))]
#[case(datetime!(2020-01-01 0:00 UTC), (2020, 1, Wednesday))]
#[case(datetime!(2020-12-31 0:00 UTC), (2020, 53, Thursday))]
#[case(datetime!(2021-01-01 0:00 UTC), (2020, 53, Friday))]
fn to_iso_week_date(#[case] input: OffsetDateTime, #[case] expected: (i32, u8, Weekday)) {
    assert_eq!(input.to_iso_week_date(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), Tuesday)]
#[case(datetime!(2019-02-01 0:00 UTC), Friday)]
#[case(datetime!(2019-03-01 0:00 UTC), Friday)]
fn weekday(#[case] input: OffsetDateTime, #[case] expected: Weekday) {
    assert_eq!(input.weekday(), expected);
}

#[rstest]
#[case(datetime!(-999_999-01-01 0:00 UTC), -363_521_074)]
#[case(datetime!(-4713-11-24 0:00 UTC), 0)]
#[case(datetime!(2000-01-01 0:00 UTC), 2_451_545)]
#[case(datetime!(2019-01-01 0:00 UTC), 2_458_485)]
#[case(datetime!(2019-12-31 0:00 UTC), 2_458_849)]
fn to_julian_day(#[case] input: OffsetDateTime, #[case] expected: i32) {
    assert_eq!(input.to_julian_day(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 1:02:03 UTC), (1, 2, 3))]
fn to_hms(#[case] input: OffsetDateTime, #[case] expected: (u8, u8, u8)) {
    assert_eq!(input.to_hms(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 1:02:03.004 UTC), (1, 2, 3, 4))]
fn to_hms_milli(#[case] input: OffsetDateTime, #[case] expected: (u8, u8, u8, u16)) {
    assert_eq!(input.to_hms_milli(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 1:02:03.004_005 UTC), (1, 2, 3, 4_005))]
fn to_hms_micro(#[case] input: OffsetDateTime, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(input.to_hms_micro(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 1:02:03.004_005_006 UTC), (1, 2, 3, 4_005_006))]
fn to_hms_nano(#[case] input: OffsetDateTime, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(input.to_hms_nano(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 0)]
#[case(
    datetime!(2019-01-01 23:59:59 UTC).to_offset(offset!(-2)),
    21
)]
fn hour(#[case] input: OffsetDateTime, #[case] expected: u8) {
    assert_eq!(input.hour(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 0)]
#[case(
    datetime!(2019-01-01 23:59:59 UTC).to_offset(offset!(+0:30)),
    29
)]
fn minute(#[case] input: OffsetDateTime, #[case] expected: u8) {
    assert_eq!(input.minute(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 0)]
#[case(
    datetime!(2019-01-01 23:59:59 UTC).to_offset(offset!(+0:00:30)),
    29
)]
fn second(#[case] input: OffsetDateTime, #[case] expected: u8) {
    assert_eq!(input.second(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 0)]
#[case(datetime!(2019-01-01 23:59:59.999 UTC), 999)]
fn millisecond(#[case] input: OffsetDateTime, #[case] expected: u16) {
    assert_eq!(input.millisecond(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 0)]
#[case(datetime!(2019-01-01 23:59:59.999_999 UTC), 999_999)]
fn microsecond(#[case] input: OffsetDateTime, #[case] expected: u32) {
    assert_eq!(input.microsecond(), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 0)]
#[case(datetime!(2019-01-01 23:59:59.999_999_999 UTC), 999_999_999)]
fn nanosecond(#[case] input: OffsetDateTime, #[case] expected: u32) {
    assert_eq!(input.nanosecond(), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 5:00 UTC), time!(12:00), datetime!(2020-01-01 12:00 UTC))]
#[case(datetime!(2020-01-01 12:00 -5), time!(7:00), datetime!(2020-01-01 7:00 -5))]
#[case(datetime!(2020-01-01 0:00 +1), time!(12:00), datetime!(2020-01-01 12:00 +1))]
fn replace_time(
    #[case] input: OffsetDateTime,
    #[case] new_time: Time,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input.replace_time(new_time), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 12:00 UTC), date!(2020-01-30), datetime!(2020-01-30 12:00 UTC))]
#[case(datetime!(2020-01-01 0:00 +1), date!(2020-01-30), datetime!(2020-01-30 0:00 +1))]
fn replace_date(
    #[case] input: OffsetDateTime,
    #[case] new_date: Date,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input.replace_date(new_date), expected);
}

#[rstest]
#[case(
    datetime!(2020-01-01 12:00 UTC),
    datetime!(2020-01-30 16:00),
    datetime!(2020-01-30 16:00 UTC),
)]
#[case(datetime!(2020-01-01 12:00 +1), datetime!(2020-01-30 0:00), datetime!(2020-01-30 0:00 +1))]
fn replace_date_time(
    #[case] input: OffsetDateTime,
    #[case] new_datetime: PlainDateTime,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input.replace_date_time(new_datetime), expected);
}

#[rstest]
#[case(datetime!(2020-01-01 0:00 UTC), offset!(-5), datetime!(2020-01-01 0:00 -5))]
fn replace_offset(
    #[case] input: OffsetDateTime,
    #[case] new_offset: UtcOffset,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input.replace_offset(new_offset), expected);
}

#[rstest]
#[case(datetime!(2022-02-18 12:00 +01), 2019, datetime!(2019-02-18 12:00 +01))]
#[case(datetime!(2022-02-18 12:00 +01), -1_000_000_000, None)]
#[case(datetime!(2022-02-18 12:00 +01), 1_000_000_000, None)]
fn replace_year(
    #[case] input: OffsetDateTime,
    #[case] year: i32,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_year(year).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 12:00 +01), Month::January, datetime!(2022-01-18 12:00 +01))]
#[case(datetime!(2022-01-30 12:00 +01), Month::February, None)]
fn replace_month(
    #[case] input: OffsetDateTime,
    #[case] month: Month,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_month(month).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 12:00 +01), 1, datetime!(2022-02-01 12:00 +01))]
#[case(datetime!(2022-02-18 12:00 +01), 0, None)]
#[case(datetime!(2022-02-18 12:00 +01), 30, None)]
fn replace_day(
    #[case] input: OffsetDateTime,
    #[case] day: u8,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_day(day).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 12:00 +01), 1, datetime!(2022-001 12:00 +01))]
#[case(datetime!(2024-02-29 12:00 +01), 366, datetime!(2024-366 12:00 +01))]
#[case(datetime!(2022-049 12:00 +01), 0, None)]
#[case(datetime!(2022-049 12:00 +01), 366, None)]
#[case(datetime!(2022-049 12:00 +01), 367, None)]
fn replace_ordinal(
    #[case] input: OffsetDateTime,
    #[case] ordinal: u16,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_ordinal(ordinal).ok(), expected.into());
}

#[rstest]
#[case(
    datetime!(2022-02-18 01:02:03.004_005_006 +01),
    7,
    datetime!(2022-02-18 07:02:03.004_005_006 +01),
)]
#[case(datetime!(2022-02-18 01:02:03.004_005_006 +01), 24, None)]
fn replace_hour(
    #[case] input: OffsetDateTime,
    #[case] hour: u8,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_hour(hour).ok(), expected.into());
}

#[rstest]
#[case(
    datetime!(2022-02-18 01:02:03.004_005_006 +01),
    7,
    datetime!(2022-02-18 01:07:03.004_005_006 +01),
)]
#[case(datetime!(2022-02-18 01:02:03.004_005_006 +01), 60, None)]
fn replace_minute(
    #[case] input: OffsetDateTime,
    #[case] minute: u8,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_minute(minute).ok(), expected.into());
}

#[rstest]
#[case(
    datetime!(2022-02-18 01:02:03.004_005_006 +01),
    7,
    datetime!(2022-02-18 01:02:07.004_005_006 +01),
)]
fn replace_second(
    #[case] input: OffsetDateTime,
    #[case] second: u8,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_second(second).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2022-02-18 01:02:03.004_005_006 +01), 7, datetime!(2022-02-18 01:02:03.007 +01))]
#[case(datetime!(2022-02-18 01:02:03.004_005_006 +01), 1_000, None)]
fn replace_millisecond(
    #[case] input: OffsetDateTime,
    #[case] millisecond: u16,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_millisecond(millisecond).ok(), expected.into());
}

#[rstest]
#[case(
    datetime!(2022-02-18 01:02:03.004_005_006 +01),
    7_008,
    datetime!(2022-02-18 01:02:03.007_008 +01),
)]
#[case(datetime!(2022-02-18 01:02:03.004_005_006 +01), 1_000_000, None)]
fn replace_microsecond(
    #[case] input: OffsetDateTime,
    #[case] microsecond: u32,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_microsecond(microsecond).ok(), expected.into());
}

#[rstest]
#[case(
    datetime!(2022-02-18 01:02:03.004_005_006 +01),
    7_008_009,
    datetime!(2022-02-18 01:02:03.007_008_009 +01),
)]
#[case(datetime!(2022-02-18 01:02:03.004_005_006 +01), 1_000_000_000, None)]
fn replace_nanosecond(
    #[case] input: OffsetDateTime,
    #[case] nanosecond: u32,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.replace_nanosecond(nanosecond).ok(), expected.into());
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789 +1), datetime!(2021-11-12 0:00 +1))]
#[case(datetime!(2021-11-12 0:00 +1), datetime!(2021-11-12 0:00 +1))]
fn truncate_to_day(#[case] input: OffsetDateTime, #[case] expected: OffsetDateTime) {
    assert_eq!(input.truncate_to_day(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789 +1), datetime!(2021-11-12 17:00 +1))]
#[case(datetime!(2021-11-12 0:00 +1), datetime!(2021-11-12 0:00 +1))]
fn truncate_to_hour(#[case] input: OffsetDateTime, #[case] expected: OffsetDateTime) {
    assert_eq!(input.truncate_to_hour(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789 +1), datetime!(2021-11-12 17:47 +1))]
#[case(datetime!(2021-11-12 0:00 +1), datetime!(2021-11-12 0:00 +1))]
fn truncate_to_minute(#[case] input: OffsetDateTime, #[case] expected: OffsetDateTime) {
    assert_eq!(input.truncate_to_minute(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789 +1), datetime!(2021-11-12 17:47:53 +1))]
#[case(datetime!(2021-11-12 0:00 +1), datetime!(2021-11-12 0:00 +1))]
fn truncate_to_second(#[case] input: OffsetDateTime, #[case] expected: OffsetDateTime) {
    assert_eq!(input.truncate_to_second(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789 +1), datetime!(2021-11-12 17:47:53.123 +1))]
#[case(datetime!(2021-11-12 0:00 +1), datetime!(2021-11-12 0:00 +1))]
fn truncate_to_millisecond(#[case] input: OffsetDateTime, #[case] expected: OffsetDateTime) {
    assert_eq!(input.truncate_to_millisecond(), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47:53.123_456_789 +1), datetime!(2021-11-12 17:47:53.123_456 +1))]
#[case(datetime!(2021-11-12 0:00 +1), datetime!(2021-11-12 0:00 +1))]
fn truncate_to_microsecond(#[case] input: OffsetDateTime, #[case] expected: OffsetDateTime) {
    assert_eq!(input.truncate_to_microsecond(), expected);
}

#[rstest]
#[case(datetime!(2000-01-01 0:00 UTC).to_offset(offset!(-1)), datetime!(2000-01-01 0:00 UTC))]
fn partial_eq(#[case] a: OffsetDateTime, #[case] b: OffsetDateTime) {
    assert_eq!(a, b);
}

#[rstest]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    datetime!(2019-01-01 0:00 UTC).to_offset(offset!(-1)),
    Ordering::Equal,
)]
fn partial_ord(#[case] a: OffsetDateTime, #[case] b: OffsetDateTime, #[case] expected: Ordering) {
    assert_eq!(a.partial_cmp(&b), Some(expected));
}

#[rstest]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    datetime!(2019-01-01 0:00 UTC).to_offset(offset!(-1)),
    Ordering::Equal,
)]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    datetime!(2019-01-01 0:00:00.000_000_001 UTC),
    Ordering::Less,
)]
#[case(datetime!(-0001-01-01 0:00 UTC), datetime!(0001-01-01 0:00 UTC), Ordering::Less)]
fn ord(#[case] a: OffsetDateTime, #[case] b: OffsetDateTime, #[case] expected: Ordering) {
    assert_eq!(a.cmp(&b), expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), datetime!(2019-01-01 0:00 UTC).to_offset(offset!(-1)))]
fn hash(#[case] a: OffsetDateTime, #[case] b: OffsetDateTime) {
    use std::collections::hash_map::DefaultHasher;
    use std::hash::{Hash, Hasher};

    let hash_a = {
        let mut hasher = DefaultHasher::new();
        a.hash(&mut hasher);
        hasher.finish()
    };

    let hash_b = {
        let mut hasher = DefaultHasher::new();
        b.hash(&mut hasher);
        hasher.finish()
    };

    assert_eq!(hash_a, hash_b);
}

#[rstest]
fn arithmetic_regression() {
    let val = Date::MIN
        .next_day()
        .expect("date is not maximum")
        .midnight()
        .assume_offset(offset!(+12:00:01));

    assert_eq!(val + SignedDuration::ZERO, val);
    assert_eq!(val - SignedDuration::ZERO, val);
    assert_eq!(val + StdDuration::from_secs(0), val);
    assert_eq!(val - StdDuration::from_secs(0), val);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 5.days(), datetime!(2019-01-06 0:00 UTC))]
#[case(datetime!(2019-12-31 0:00 UTC), 1.days(), datetime!(2020-01-01 0:00 UTC))]
#[case(datetime!(2019-12-31 23:59:59 UTC), 2.seconds(), datetime!(2020-01-01 0:00:01 UTC))]
#[case(datetime!(2020-01-01 0:00:01 UTC), (-2).seconds(), datetime!(2019-12-31 23:59:59 UTC))]
#[case(datetime!(1999-12-31 23:00 UTC), 1.hours(), datetime!(2000-01-01 0:00 UTC))]
fn add_duration(
    #[case] input: OffsetDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input + duration, expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 5.std_days(), datetime!(2019-01-06 0:00 UTC))]
#[case(datetime!(2019-12-31 0:00 UTC), 1.std_days(), datetime!(2020-01-01 0:00 UTC))]
#[case(datetime!(2019-12-31 23:59:59 UTC), 2.std_seconds(), datetime!(2020-01-01 0:00:01 UTC))]
fn add_std_duration(
    #[case] input: OffsetDateTime,
    #[case] duration: StdDuration,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input + duration, expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 5.days(), datetime!(2019-01-06 0:00 UTC))]
#[case(datetime!(2019-12-31 0:00 UTC), 1.days(), datetime!(2020-01-01 0:00 UTC))]
#[case(datetime!(2019-12-31 23:59:59 UTC), 2.seconds(), datetime!(2020-01-01 0:00:01 UTC))]
#[case(datetime!(2020-01-01 0:00:01 UTC), (-2).seconds(), datetime!(2019-12-31 23:59:59 UTC))]
fn add_assign_duration(
    #[case] mut input: OffsetDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: OffsetDateTime,
) {
    input += duration;
    assert_eq!(input, expected);
}

#[rstest]
#[case(datetime!(2019-01-01 0:00 UTC), 5.std_days(), datetime!(2019-01-06 0:00 UTC))]
#[case(datetime!(2019-12-31 0:00 UTC), 1.std_days(), datetime!(2020-01-01 0:00 UTC))]
#[case(datetime!(2019-12-31 23:59:59 UTC), 2.std_seconds(), datetime!(2020-01-01 0:00:01 UTC))]
fn add_assign_std_duration(
    #[case] mut input: OffsetDateTime,
    #[case] duration: StdDuration,
    #[case] expected: OffsetDateTime,
) {
    input += duration;
    assert_eq!(input, expected);
}

#[rstest]
#[case(datetime!(2019-01-06 0:00 UTC), 5.days(), datetime!(2019-01-01 0:00 UTC))]
#[case(datetime!(2020-01-01 0:00 UTC), 1.days(), datetime!(2019-12-31 0:00 UTC))]
#[case(datetime!(2020-01-01 0:00:01 UTC), 2.seconds(), datetime!(2019-12-31 23:59:59 UTC))]
#[case(datetime!(2019-12-31 23:59:59 UTC), (-2).seconds(), datetime!(2020-01-01 0:00:01 UTC))]
#[case(datetime!(1999-12-31 23:00 UTC), (-1).hours(), datetime!(2000-01-01 0:00 UTC))]
fn sub_duration(
    #[case] input: OffsetDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input - duration, expected);
}

#[rstest]
#[case(datetime!(2019-01-06 0:00 UTC), 5.std_days(), datetime!(2019-01-01 0:00 UTC))]
#[case(datetime!(2020-01-01 0:00 UTC), 1.std_days(), datetime!(2019-12-31 0:00 UTC))]
#[case(datetime!(2020-01-01 0:00:01 UTC), 2.std_seconds(), datetime!(2019-12-31 23:59:59 UTC))]
fn sub_std_duration(
    #[case] input: OffsetDateTime,
    #[case] duration: StdDuration,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input - duration, expected);
}

#[rstest]
#[case(datetime!(2019-01-06 0:00 UTC), 5.days(), datetime!(2019-01-01 0:00 UTC))]
#[case(datetime!(2020-01-01 0:00 UTC), 1.days(), datetime!(2019-12-31 0:00 UTC))]
#[case(datetime!(2020-01-01 0:00:01 UTC), 2.seconds(), datetime!(2019-12-31 23:59:59 UTC))]
#[case(datetime!(2019-12-31 23:59:59 UTC), (-2).seconds(), datetime!(2020-01-01 0:00:01 UTC))]
fn sub_assign_duration(
    #[case] mut input: OffsetDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: OffsetDateTime,
) {
    input -= duration;
    assert_eq!(input, expected);
}

#[rstest]
#[case(datetime!(2019-01-06 0:00 UTC), 5.std_days(), datetime!(2019-01-01 0:00 UTC))]
#[case(datetime!(2020-01-01 0:00 UTC), 1.std_days(), datetime!(2019-12-31 0:00 UTC))]
#[case(datetime!(2020-01-01 0:00:01 UTC), 2.std_seconds(), datetime!(2019-12-31 23:59:59 UTC))]
fn sub_assign_std_duration(
    #[case] mut input: OffsetDateTime,
    #[case] duration: StdDuration,
    #[case] expected: OffsetDateTime,
) {
    input -= duration;
    assert_eq!(input, expected);
}

#[rstest]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    0.seconds(),
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    5.days(),
    SystemTime::from(datetime!(2019-01-06 0:00 UTC)),
)]
#[case(
    SystemTime::from(datetime!(2019-12-31 0:00 UTC)),
    1.days(),
    SystemTime::from(datetime!(2020-01-01 0:00 UTC)),
)]
#[case(
    SystemTime::from(datetime!(2019-12-31 23:59:59 UTC)),
    2.seconds(),
    SystemTime::from(datetime!(2020-01-01 0:00:01 UTC)),
)]
#[case(
    SystemTime::from(datetime!(2020-01-01 0:00:01 UTC)),
    (-2).seconds(),
    SystemTime::from(datetime!(2019-12-31 23:59:59 UTC)),
)]
fn std_add_duration(
    #[case] input: SystemTime,
    #[case] duration: SignedDuration,
    #[case] expected: SystemTime,
) {
    assert_eq!(input + duration, expected);
}

#[rstest]
#[case(SystemTime::from(datetime!(2019-01-01 0:00 UTC)), 5.days(), datetime!(2019-01-06 0:00 UTC))]
#[case(SystemTime::from(datetime!(2019-12-31 0:00 UTC)), 1.days(), datetime!(2020-01-01 0:00 UTC))]
#[case(
    SystemTime::from(datetime!(2019-12-31 23:59:59 UTC)),
    2.seconds(),
    datetime!(2020-01-01 0:00:01 UTC),
)]
#[case(
    SystemTime::from(datetime!(2020-01-01 0:00:01 UTC)),
    (-2).seconds(),
    datetime!(2019-12-31 23:59:59 UTC),
)]
fn std_add_assign_duration(
    #[case] mut input: SystemTime,
    #[case] duration: SignedDuration,
    #[case] expected: OffsetDateTime,
) {
    input += duration;
    assert_eq!(input, expected);
}

#[rstest]
#[case(
    SystemTime::from(datetime!(2019-01-06 0:00 UTC)),
    5.days(),
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
)]
#[case(
    SystemTime::from(datetime!(2020-01-01 0:00 UTC)),
    1.days(),
    SystemTime::from(datetime!(2019-12-31 0:00 UTC)),
)]
#[case(
    SystemTime::from(datetime!(2020-01-01 0:00:01 UTC)),
    2.seconds(),
    SystemTime::from(datetime!(2019-12-31 23:59:59 UTC)),
)]
#[case(
    SystemTime::from(datetime!(2019-12-31 23:59:59 UTC)),
    (-2).seconds(),
    SystemTime::from(datetime!(2020-01-01 0:00:01 UTC)),
)]
fn std_sub_duration(
    #[case] input: SystemTime,
    #[case] duration: SignedDuration,
    #[case] expected: SystemTime,
) {
    assert_eq!(input - duration, expected);
}

#[rstest]
#[case(SystemTime::from(datetime!(2019-01-06 0:00 UTC)), 5.days(), datetime!(2019-01-01 0:00 UTC))]
#[case(SystemTime::from(datetime!(2020-01-01 0:00 UTC)), 1.days(), datetime!(2019-12-31 0:00 UTC))]
#[case(
    SystemTime::from(datetime!(2020-01-01 0:00:01 UTC)),
    2.seconds(),
    datetime!(2019-12-31 23:59:59 UTC),
)]
#[case(
    SystemTime::from(datetime!(2019-12-31 23:59:59 UTC)),
    (-2).seconds(),
    datetime!(2020-01-01 0:00:01 UTC),
)]
fn std_sub_assign_duration(
    #[case] mut input: SystemTime,
    #[case] duration: SignedDuration,
    #[case] expected: OffsetDateTime,
) {
    input -= duration;
    assert_eq!(input, expected);
}

#[rstest]
#[case(datetime!(2019-01-02 0:00 UTC), datetime!(2019-01-01 0:00 UTC), 1.days())]
#[case(datetime!(2019-01-01 0:00 UTC), datetime!(2019-01-02 0:00 UTC), (-1).days())]
#[case(datetime!(2020-01-01 0:00 UTC), datetime!(2019-12-31 0:00 UTC), 1.days())]
#[case(datetime!(2019-12-31 0:00 UTC), datetime!(2020-01-01 0:00 UTC), (-1).days())]
#[case(
    datetime!(+999_999-12-31 23:59:59.999_999_999 -23:59:59),
    datetime!(-999_999-01-01 0:00 +23:59:59),
    SignedDuration::new(63_113_872_550_397, 999_999_999),
)]
fn sub_self(
    #[case] a: OffsetDateTime,
    #[case] b: OffsetDateTime,
    #[case] expected: SignedDuration,
) {
    assert_eq!(a - b, expected);
}

#[rstest]
#[case(SystemTime::from(datetime!(2019-01-02 0:00 UTC)), datetime!(2019-01-01 0:00 UTC), 1.days())]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    datetime!(2019-01-02 0:00 UTC),
    (-1).days(),
)]
#[case(SystemTime::from(datetime!(2020-01-01 0:00 UTC)), datetime!(2019-12-31 0:00 UTC), 1.days())]
#[case(
    SystemTime::from(datetime!(2019-12-31 0:00 UTC)),
    datetime!(2020-01-01 0:00 UTC),
    (-1).days(),
)]
fn std_sub(
    #[case] system: SystemTime,
    #[case] datetime: OffsetDateTime,
    #[case] expected: SignedDuration,
) {
    assert_eq!(system - datetime, expected);
}

#[rstest]
#[case(datetime!(2019-01-02 0:00 UTC), SystemTime::from(datetime!(2019-01-01 0:00 UTC)), 1.days())]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    SystemTime::from(datetime!(2019-01-02 0:00 UTC)),
    (-1).days(),
)]
#[case(datetime!(2020-01-01 0:00 UTC), SystemTime::from(datetime!(2019-12-31 0:00 UTC)), 1.days())]
#[case(
    datetime!(2019-12-31 0:00 UTC),
    SystemTime::from(datetime!(2020-01-01 0:00 UTC)),
    (-1).days(),
)]
fn sub_std(
    #[case] input: OffsetDateTime,
    #[case] system: SystemTime,
    #[case] expected: SignedDuration,
) {
    assert_eq!(input - system, expected);
}

#[rstest]
#[case(OffsetDateTime::now_utc())]
fn eq_std(#[case] now_datetime: OffsetDateTime) {
    let now_systemtime = SystemTime::from(now_datetime);
    assert_eq!(now_datetime, now_systemtime);
}

#[rstest]
#[case(OffsetDateTime::now_utc())]
fn std_eq(#[case] now_datetime: OffsetDateTime) {
    let now_systemtime = SystemTime::from(now_datetime);
    assert_eq!(now_systemtime, now_datetime);
}

#[rstest]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    Ordering::Equal,
)]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    SystemTime::from(datetime!(2020-01-01 0:00 UTC)),
    Ordering::Less,
)]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    SystemTime::from(datetime!(2019-02-01 0:00 UTC)),
    Ordering::Less,
)]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    SystemTime::from(datetime!(2019-01-02 0:00 UTC)),
    Ordering::Less,
)]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    SystemTime::from(datetime!(2019-01-01 1:00:00 UTC)),
    Ordering::Less,
)]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    SystemTime::from(datetime!(2019-01-01 0:01:00 UTC)),
    Ordering::Less,
)]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    SystemTime::from(datetime!(2019-01-01 0:00:01 UTC)),
    Ordering::Less,
)]
#[case(
    datetime!(2019-01-01 0:00 UTC),
    SystemTime::from(datetime!(2019-01-01 0:00:00.000_001 UTC)),
    Ordering::Less,
)]
#[case(
    datetime!(2020-01-01 0:00 UTC),
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    Ordering::Greater,
)]
#[case(
    datetime!(2019-02-01 0:00 UTC),
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    Ordering::Greater,
)]
#[case(
    datetime!(2019-01-02 0:00 UTC),
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    Ordering::Greater,
)]
#[case(
    datetime!(2019-01-01 1:00:00 UTC),
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    Ordering::Greater,
)]
#[case(
    datetime!(2019-01-01 0:01:00 UTC),
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    Ordering::Greater,
)]
#[case(
    datetime!(2019-01-01 0:00:01 UTC),
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    Ordering::Greater,
)]
fn ord_std(#[case] a: OffsetDateTime, #[case] b: SystemTime, #[case] expected: Ordering) {
    assert_eq!(a.partial_cmp(&b), Some(expected));
}

#[rstest]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    datetime!(2019-01-01 0:00 UTC),
    Ordering::Equal,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    datetime!(2020-01-01 0:00 UTC),
    Ordering::Less,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    datetime!(2019-02-01 0:00 UTC),
    Ordering::Less,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    datetime!(2019-01-02 0:00 UTC),
    Ordering::Less,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    datetime!(2019-01-01 1:00:00 UTC),
    Ordering::Less,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    datetime!(2019-01-01 0:01:00 UTC),
    Ordering::Less,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    datetime!(2019-01-01 0:00:01 UTC),
    Ordering::Less,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00 UTC)),
    datetime!(2019-01-01 0:00:00.000_000_001 UTC),
    Ordering::Less,
)]
#[case(
    SystemTime::from(datetime!(2020-01-01 0:00 UTC)),
    datetime!(2019-01-01 0:00 UTC),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(datetime!(2019-02-01 0:00 UTC)),
    datetime!(2019-01-01 0:00 UTC),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(datetime!(2019-01-02 0:00 UTC)),
    datetime!(2019-01-01 0:00 UTC),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 1:00:00 UTC)),
    datetime!(2019-01-01 0:00 UTC),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:01:00 UTC)),
    datetime!(2019-01-01 0:00 UTC),
    Ordering::Greater,
)]
#[case(
    SystemTime::from(datetime!(2019-01-01 0:00:01 UTC)),
    datetime!(2019-01-01 0:00 UTC),
    Ordering::Greater,
)]
fn std_ord(#[case] a: SystemTime, #[case] b: OffsetDateTime, #[case] expected: Ordering) {
    assert_eq!(a.partial_cmp(&b), Some(expected));
}

#[rstest]
#[case(SystemTime::UNIX_EPOCH, OffsetDateTime::UNIX_EPOCH)]
#[case(SystemTime::UNIX_EPOCH - 1.std_days(), OffsetDateTime::UNIX_EPOCH - 1.days())]
#[case(SystemTime::UNIX_EPOCH + 1.std_days(), OffsetDateTime::UNIX_EPOCH + 1.days())]
fn from_std(#[case] input: SystemTime, #[case] expected: OffsetDateTime) {
    assert_eq!(OffsetDateTime::from(input), expected);
}

#[rstest]
#[case(OffsetDateTime::UNIX_EPOCH, SystemTime::UNIX_EPOCH)]
#[case(OffsetDateTime::UNIX_EPOCH + 1.days(), SystemTime::UNIX_EPOCH + 1.std_days())]
#[case(OffsetDateTime::UNIX_EPOCH - 1.days(), SystemTime::UNIX_EPOCH - 1.std_days())]
fn to_std(#[case] input: OffsetDateTime, #[case] expected: SystemTime) {
    assert_eq!(SystemTime::from(input), expected);
}

#[rstest]
#[case(
    datetime!(2021-10-25 14:01:53.45 UTC),
    5.nanoseconds(),
    datetime!(2021-10-25 14:01:53.450_000_005 UTC),
)]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), 4.seconds(), datetime!(2021-10-25 14:01:57.45 UTC))]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), 2.days(), datetime!(2021-10-27 14:01:53.45 UTC))]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), 1.weeks(), datetime!(2021-11-01 14:01:53.45 UTC))]
#[case(
    datetime!(2021-10-25 14:01:53.45 UTC),
    (-5).nanoseconds(),
    datetime!(2021-10-25 14:01:53.449_999_995 UTC),
)]
#[case(
    datetime!(2021-10-25 14:01:53.45 UTC),
    (-4).seconds(),
    datetime!(2021-10-25 14:01:49.45 UTC),
)]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), (-2).days(), datetime!(2021-10-23 14:01:53.45 UTC))]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), (-1).weeks(), datetime!(2021-10-18 14:01:53.45 UTC))]
#[case(datetime!(-999_999-01-01 0:00 UTC), (-1).nanoseconds(), None)]
#[case(datetime!(-999_999-01-01 0:00 UTC), SignedDuration::MIN, None)]
#[case(datetime!(-999_990-01-01 0:00 UTC), (-530).weeks(), None)]
#[case(datetime!(+999_999-12-31 23:59:59.999_999_999 UTC), 1.nanoseconds(), None)]
#[case(datetime!(+999_999-12-31 23:59:59.999_999_999 UTC), SignedDuration::MAX, None)]
#[case(datetime!(+999_990-12-31 23:59:59.999_999_999 UTC), 530.weeks(), None)]
#[case(
    datetime!(+999_999-12-31 23:59:59.999_999_999 -10:00),
    SignedDuration::ZERO,
    datetime!(+999_999-12-31 23:59:59.999_999_999 -10:00),
)]
#[case(
    datetime!(-999_999-01-01 0:00 +10:00),
    SignedDuration::ZERO,
    datetime!(-999_999-01-01 0:00 +10:00),
)]
fn checked_add_duration(
    #[case] input: OffsetDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.checked_add(duration), expected.into());
}

#[rstest]
#[case(
    datetime!(2021-10-25 14:01:53.45 UTC),
    (-5).nanoseconds(),
    datetime!(2021-10-25 14:01:53.450_000_005 UTC),
)]
#[case(
    datetime!(2021-10-25 14:01:53.45 UTC),
    (-4).seconds(),
    datetime!(2021-10-25 14:01:57.45 UTC),
)]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), (-2).days(), datetime!(2021-10-27 14:01:53.45 UTC))]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), (-1).weeks(), datetime!(2021-11-01 14:01:53.45 UTC))]
#[case(
    datetime!(2021-10-25 14:01:53.45 UTC),
    5.nanoseconds(),
    datetime!(2021-10-25 14:01:53.449_999_995 UTC),
)]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), 4.seconds(), datetime!(2021-10-25 14:01:49.45 UTC))]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), 2.days(), datetime!(2021-10-23 14:01:53.45 UTC))]
#[case(datetime!(2021-10-25 14:01:53.45 UTC), 1.weeks(), datetime!(2021-10-18 14:01:53.45 UTC))]
#[case(datetime!(-999_999-01-01 0:00 UTC), 1.nanoseconds(), None)]
#[case(datetime!(-999_999-01-01 0:00 UTC), SignedDuration::MAX, None)]
#[case(datetime!(-999_990-01-01 0:00 UTC), 530.weeks(), None)]
#[case(datetime!(+999_999-12-31 23:59:59.999_999_999 UTC), (-1).nanoseconds(), None)]
#[case(datetime!(+999_999-12-31 23:59:59.999_999_999 UTC), SignedDuration::MIN, None)]
#[case(datetime!(+999_990-12-31 23:59:59.999_999_999 UTC), (-530).weeks(), None)]
#[case(
    datetime!(+999_999-12-31 23:59:59.999_999_999 -10),
    SignedDuration::ZERO,
    datetime!(+999_999-12-31 23:59:59.999_999_999 -10),
)]
#[case(
    datetime!(-999_999-01-01 0:00 +10),
    SignedDuration::ZERO,
    datetime!(-999_999-01-01 0:00 +10),
)]
fn checked_sub_duration(
    #[case] input: OffsetDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: impl Into<Option<OffsetDateTime>>,
) {
    assert_eq!(input.checked_sub(duration), expected.into());
}

#[rstest]
#[case(datetime!(2021-11-12 17:47 +10), 2.days(), datetime!(2021-11-14 17:47 +10))]
#[case(datetime!(2021-11-12 17:47 +10), (-2).days(), datetime!(2021-11-10 17:47 +10))]
#[case(datetime!(-999999-01-01 0:00 +10), (-10).days(), datetime!(-999999-01-01 0:00 +10))]
#[case(
    datetime!(+999999-12-31 23:59:59.999_999_999 +10),
    10.days(),
    datetime!(+999999-12-31 23:59:59.999_999_999 +10),
)]
#[case(datetime!(-999999-01-01 0:00 +10), SignedDuration::ZERO, datetime!(-999999-01-01 0:00 +10))]
#[case(
    datetime!(+999999-12-31 23:59:59.999_999_999 +10),
    SignedDuration::ZERO,
    datetime!(+999999-12-31 23:59:59.999_999_999 +10),
)]
fn saturating_add_duration(
    #[case] input: OffsetDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input.saturating_add(duration), expected);
}

#[rstest]
#[case(datetime!(2021-11-12 17:47 +10), 2.days(), datetime!(2021-11-10 17:47 +10))]
#[case(datetime!(2021-11-12 17:47 +10), (-2).days(), datetime!(2021-11-14 17:47 +10))]
#[case(datetime!(-999999-01-01 0:00 +10), 10.days(), datetime!(-999999-01-01 0:00 +10))]
#[case(
    datetime!(+999999-12-31 23:59:59.999_999_999 +10),
    (-10).days(),
    datetime!(+999999-12-31 23:59:59.999_999_999 +10),
)]
#[case(datetime!(-999999-01-01 0:00 +10), SignedDuration::ZERO, datetime!(-999999-01-01 0:00 +10))]
#[case(
    datetime!(+999999-12-31 23:59:59.999_999_999 +10),
    SignedDuration::ZERO,
    datetime!(+999999-12-31 23:59:59.999_999_999 +10),
)]
fn saturating_sub_duration(
    #[case] input: OffsetDateTime,
    #[case] duration: SignedDuration,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(input.saturating_sub(duration), expected);
}

#[rstest]
#[should_panic = "overflow adding duration to date"]
fn issue_621() {
    let _ = OffsetDateTime::UNIX_EPOCH + StdDuration::from_secs(18_157_382_926_370_278_155);
}

#[rstest]
fn to_offset_regression() {
    let value = datetime!(0000-01-01 0:00 +24:59).to_offset(offset!(-24:59));
    assert_eq!(value, datetime!(-0001-12-29 22:02 -24:59));
}
