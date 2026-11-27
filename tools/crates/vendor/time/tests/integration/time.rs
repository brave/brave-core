use std::cmp::Ordering;

use rstest::rstest;
use time::ext::{NumericalDuration, NumericalStdDuration};
use time::macros::time;
use time::{Result, SignedDuration, Time};

#[rstest]
#[case(1, 2, 3, time!(1:02:03))]
#[case(24, 0, 0, None)]
#[case(0, 60, 0, None)]
#[case(0, 0, 60, None)]
fn from_hms(
    #[case] hour: u8,
    #[case] minute: u8,
    #[case] second: u8,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(Time::from_hms(hour, minute, second).ok(), expected.into());
}

#[rstest]
#[case(1, 2, 3, 4, time!(1:02:03.004))]
#[case(24, 0, 0, 0, None)]
#[case(0, 60, 0, 0, None)]
#[case(0, 0, 60, 0, None)]
#[case(0, 0, 0, 1_000, None)]
fn from_hms_milli(
    #[case] hour: u8,
    #[case] minute: u8,
    #[case] second: u8,
    #[case] milli: u16,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(
        Time::from_hms_milli(hour, minute, second, milli).ok(),
        expected.into()
    );
}

#[rstest]
#[case(1, 2, 3, 4, time!(1:02:03.000_004))]
#[case(24, 0, 0, 0, None)]
#[case(0, 60, 0, 0, None)]
#[case(0, 0, 60, 0, None)]
#[case(0, 0, 0, 1_000_000, None)]
fn from_hms_micro(
    #[case] hour: u8,
    #[case] minute: u8,
    #[case] second: u8,
    #[case] micro: u32,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(
        Time::from_hms_micro(hour, minute, second, micro).ok(),
        expected.into()
    );
}

#[rstest]
#[case(1, 2, 3, 4, time!(1:02:03.000_000_004))]
#[case(24, 0, 0, 0, None)]
#[case(0, 60, 0, 0, None)]
#[case(0, 0, 60, 0, None)]
#[case(0, 0, 0, 1_000_000_000, None)]
fn from_hms_nano(
    #[case] hour: u8,
    #[case] minute: u8,
    #[case] second: u8,
    #[case] nano: u32,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(
        Time::from_hms_nano(hour, minute, second, nano).ok(),
        expected.into()
    );
}

#[rstest]
#[case(time!(1:02:03), (1, 2, 3))]
fn as_hms(#[case] datetime: Time, #[case] expected: (u8, u8, u8)) {
    assert_eq!(datetime.as_hms(), expected);
}

#[rstest]
#[case(time!(1:02:03.004), (1, 2, 3, 4))]
fn as_hms_milli(#[case] datetime: Time, #[case] expected: (u8, u8, u8, u16)) {
    assert_eq!(datetime.as_hms_milli(), expected);
}

#[rstest]
#[case(time!(1:02:03.004_005), (1, 2, 3, 4_005))]
fn as_hms_micro(#[case] datetime: Time, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(datetime.as_hms_micro(), expected);
}

#[rstest]
#[case(time!(1:02:03.004_005_006), (1, 2, 3, 4_005_006))]
fn as_hms_nano(#[case] datetime: Time, #[case] expected: (u8, u8, u8, u32)) {
    assert_eq!(datetime.as_hms_nano(), expected);
}

#[rstest]
#[case(0)]
#[case(12)]
#[case(23)]
fn hour(#[case] hour_: u8) -> Result<()> {
    assert_eq!(Time::from_hms(hour_, 0, 0)?.hour(), hour_);
    assert_eq!(Time::from_hms(hour_, 59, 59)?.hour(), hour_);
    Ok(())
}

#[rstest]
#[case(0)]
#[case(30)]
#[case(59)]
fn minute(#[case] minute_: u8) -> Result<()> {
    assert_eq!(Time::from_hms(0, minute_, 0)?.minute(), minute_);
    assert_eq!(Time::from_hms(23, minute_, 59)?.minute(), minute_);
    Ok(())
}

#[rstest]
#[case(0)]
#[case(30)]
#[case(59)]
fn second(#[case] second_: u8) -> Result<()> {
    assert_eq!(Time::from_hms(0, 0, second_)?.second(), second_);
    assert_eq!(Time::from_hms(23, 59, second_)?.second(), second_);
    Ok(())
}

#[rstest]
#[case(0)]
#[case(500)]
#[case(999)]
fn millisecond(#[case] milli: u16) -> Result<()> {
    assert_eq!(Time::from_hms_milli(0, 0, 0, milli)?.millisecond(), milli);
    assert_eq!(
        Time::from_hms_milli(23, 59, 59, milli)?.millisecond(),
        milli
    );
    Ok(())
}

#[rstest]
#[case(0)]
#[case(4_005)]
#[case(999_000)]
fn microsecond(#[case] micro: u32) -> Result<()> {
    assert_eq!(Time::from_hms_micro(0, 0, 0, micro)?.microsecond(), micro);
    assert_eq!(
        Time::from_hms_micro(23, 59, 59, micro)?.microsecond(),
        micro
    );
    Ok(())
}

#[rstest]
#[case(0)]
#[case(4_005_006)]
#[case(999_000_000)]
fn nanosecond(#[case] nano: u32) -> Result<()> {
    assert_eq!(Time::from_hms_nano(0, 0, 0, nano)?.nanosecond(), nano);
    assert_eq!(Time::from_hms_nano(23, 59, 59, nano)?.nanosecond(), nano);
    Ok(())
}

#[rstest]
#[case(time!(18:00), Time::MIDNIGHT, 6.hours())]
#[case(time!(23:00), time!(1:00), 2.hours())]
#[case(time!(12:30), time!(14:00), 90.minutes())]
fn duration_until(#[case] start: Time, #[case] end: Time, #[case] expected: SignedDuration) {
    assert_eq!(start.duration_until(end), expected);
}

#[rstest]
#[case(Time::MIDNIGHT, time!(18:00), 6.hours())]
#[case(time!(1:00), time!(23:00), 2.hours())]
#[case(time!(14:00), time!(12:30), 90.minutes())]
fn duration_since(#[case] end: Time, #[case] start: Time, #[case] expected: SignedDuration) {
    assert_eq!(end.duration_since(start), expected);
}

#[rstest]
#[case(time!(1:02:03.004_005_006), 7, time!(7:02:03.004_005_006))]
#[case(time!(1:02:03.004_005_006), 24, None)]
fn replace_hour(
    #[case] time_val: Time,
    #[case] value: u8,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(time_val.replace_hour(value).ok(), expected.into());
}

#[rstest]
#[case(time!(1:02:03.004_005_006), 7, time!(1:07:03.004_005_006))]
#[case(time!(1:02:03.004_005_006), 60, None)]
fn replace_minute(
    #[case] time_val: Time,
    #[case] value: u8,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(time_val.replace_minute(value).ok(), expected.into());
}

#[rstest]
#[case(time!(1:02:03.004_005_006), 7, time!(1:02:07.004_005_006))]
#[case(time!(1:02:03.004_005_006), 60, None)]
fn replace_second(
    #[case] time_val: Time,
    #[case] value: u8,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(time_val.replace_second(value).ok(), expected.into());
}

#[rstest]
#[case(time!(1:02:03.004_005_006), 7, time!(1:02:03.007))]
#[case(time!(1:02:03.004_005_006), 1_000, None)]
fn replace_millisecond(
    #[case] time_val: Time,
    #[case] value: u16,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(time_val.replace_millisecond(value).ok(), expected.into());
}

#[rstest]
#[case(9999)]
#[case(4294)]
#[case(4295)]
fn replace_millisecond_regression(#[case] milli: u16) {
    assert!(Time::MIDNIGHT.replace_millisecond(milli).is_err());
}

#[rstest]
#[case(time!(1:02:03.004_005_006), 7_008, time!(1:02:03.007_008))]
#[case(time!(1:02:03.004_005_006), 1_000_000, None)]
fn replace_microsecond(
    #[case] time: Time,
    #[case] value: u32,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(time.replace_microsecond(value).ok(), expected.into());
}

#[rstest]
#[case(time!(1:02:03.004_005_006), 7_008_009, time!(1:02:03.007_008_009))]
#[case(time!(1:02:03.004_005_006), 1_000_000_000, None)]
fn replace_nanosecond(
    #[case] time: Time,
    #[case] value: u32,
    #[case] expected: impl Into<Option<Time>>,
) {
    assert_eq!(time.replace_nanosecond(value).ok(), expected.into());
}

#[rstest]
#[case(time!(1:02:03.004_005_006), time!(1:00))]
#[case(Time::MIDNIGHT, Time::MIDNIGHT)]
fn truncate_to_hour(#[case] time: Time, #[case] expected: Time) {
    assert_eq!(time.truncate_to_hour(), expected);
}

#[rstest]
#[case(time!(1:02:03.004_005_006), time!(1:02))]
#[case(Time::MIDNIGHT, Time::MIDNIGHT)]
fn truncate_to_minute(#[case] time: Time, #[case] expected: Time) {
    assert_eq!(time.truncate_to_minute(), expected);
}

#[rstest]
#[case(time!(1:02:03.004_005_006), time!(1:02:03))]
#[case(Time::MIDNIGHT, Time::MIDNIGHT)]
fn truncate_to_second(#[case] time: Time, #[case] expected: Time) {
    assert_eq!(time.truncate_to_second(), expected);
}

#[rstest]
#[case(time!(1:02:03.004_005_006), time!(1:02:03.004))]
#[case(Time::MIDNIGHT, Time::MIDNIGHT)]
fn truncate_to_millisecond(#[case] itimeput: Time, #[case] expected: Time) {
    assert_eq!(itimeput.truncate_to_millisecond(), expected);
}

#[rstest]
#[case(time!(1:02:03.004_005_006), time!(1:02:03.004_005))]
#[case(Time::MIDNIGHT, Time::MIDNIGHT)]
fn truncate_to_microsecond(#[case] time: Time, #[case] expected: Time) {
    assert_eq!(time.truncate_to_microsecond(), expected);
}

#[rstest]
#[case(time!(0:00), 1.seconds(), time!(0:00:01))]
#[case(time!(0:00), 1.minutes(), time!(0:01))]
#[case(time!(0:00), 1.hours(), time!(1:00))]
#[case(time!(0:00), 1.days(), time!(0:00))]
fn add_duration(#[case] input: Time, #[case] duration: SignedDuration, #[case] expected: Time) {
    assert_eq!(input + duration, expected);
}

#[rstest]
#[case(time!(0:00), 1.seconds(), time!(0:00:01))]
#[case(time!(0:00), 1.minutes(), time!(0:01:00))]
#[case(time!(0:00), 1.hours(), time!(1:00:00))]
#[case(time!(0:00), 1.days(), time!(0:00))]
fn add_assign_duration(
    #[case] mut time: Time,
    #[case] duration: SignedDuration,
    #[case] expected: Time,
) {
    time += duration;
    assert_eq!(time, expected);
}

#[rstest]
#[case(time!(12:00), 1.hours(), time!(11:00))]
#[case(time!(0:00), 1.seconds(), time!(23:59:59))]
#[case(time!(0:00), 1.minutes(), time!(23:59))]
#[case(time!(0:00), 1.hours(), time!(23:00))]
#[case(time!(0:00), 1.days(), time!(0:00))]
fn sub_duration(#[case] time: Time, #[case] duration: SignedDuration, #[case] expected: Time) {
    assert_eq!(time - duration, expected);
}

#[rstest]
#[case(time!(0:00), 1.seconds(), time!(23:59:59))]
#[case(time!(0:00), 1.minutes(), time!(23:59))]
#[case(time!(0:00), 1.hours(), time!(23:00))]
#[case(time!(0:00), 1.days(), time!(0:00))]
fn sub_assign_duration(
    #[case] mut time: Time,
    #[case] duration: SignedDuration,
    #[case] expected: Time,
) {
    time -= duration;
    assert_eq!(time, expected);
}

#[rstest]
#[case(time!(0:00), 1.std_milliseconds(), time!(0:00:00.001))]
#[case(time!(0:00), 1.std_seconds(), time!(0:00:01))]
#[case(time!(0:00), 1.std_minutes(), time!(0:01))]
#[case(time!(0:00), 1.std_hours(), time!(1:00))]
#[case(time!(0:00), 1.std_days(), time!(0:00))]
fn add_std_duration(
    #[case] time: Time,
    #[case] duration: std::time::Duration,
    #[case] expected: Time,
) {
    assert_eq!(time + duration, expected);
}

#[rstest]
#[case(time!(0:00), 1.std_seconds(), time!(0:00:01))]
#[case(time!(0:00), 1.std_minutes(), time!(0:01))]
#[case(time!(0:00), 1.std_hours(), time!(1:00))]
#[case(time!(0:00), 1.std_days(), time!(0:00))]
fn add_assign_std_duration(
    #[case] mut time: Time,
    #[case] duration: std::time::Duration,
    #[case] expected: Time,
) {
    time += duration;
    assert_eq!(time, expected);
}

#[rstest]
#[case(time!(12:00), 1.std_hours(), time!(11:00))]
#[case(time!(0:00), 1.std_milliseconds(), time!(23:59:59.999))]
#[case(time!(0:00), 1.std_seconds(), time!(23:59:59))]
#[case(time!(0:00), 1.std_minutes(), time!(23:59))]
#[case(time!(0:00), 1.std_hours(), time!(23:00))]
#[case(time!(0:00), 1.std_days(), time!(0:00))]
fn sub_std_duration(
    #[case] time: Time,
    #[case] duration: std::time::Duration,
    #[case] expected: Time,
) {
    assert_eq!(time - duration, expected);
}

#[rstest]
#[case(time!(0:00), 1.std_seconds(), time!(23:59:59))]
#[case(time!(0:00), 1.std_minutes(), time!(23:59))]
#[case(time!(0:00), 1.std_hours(), time!(23:00))]
#[case(time!(0:00), 1.std_days(), time!(0:00))]
fn sub_assign_std_duration(
    #[case] mut time: Time,
    #[case] duration: std::time::Duration,
    #[case] expected: Time,
) {
    time -= duration;
    assert_eq!(time, expected);
}

#[rstest]
#[case(time!(0:00), time!(0:00), 0.seconds())]
#[case(time!(1:00), time!(0:00), 1.hours())]
#[case(time!(1:00), time!(0:00:01), 59.minutes() + 59.seconds())]
fn sub_time(#[case] a: Time, #[case] b: Time, #[case] expected: SignedDuration) {
    assert_eq!(a - b, expected);
}

#[rstest]
#[case(time!(0:00), time!(0:00:00.000_000_001), Ordering::Less)]
#[case(time!(0:00), time!(0:00:01), Ordering::Less)]
#[case(time!(12:00), time!(11:00), Ordering::Greater)]
#[case(time!(0:00), time!(0:00), Ordering::Equal)]
fn ordering(#[case] a: Time, #[case] b: Time, #[case] expected: Ordering) {
    assert_eq!(a.cmp(&b), expected);
}

#[rstest]
#[case(time!(00:00:00.4), time!(00:00:00.1))]
#[case(time!(01:00:00), time!(00:01:00.0))]
#[case(time!(01:00:00), time!(00:00:01.0))]
#[case(time!(01:00:00), time!(00:00:00.1))]
#[case(time!(00:01:00), time!(00:00:01.0))]
#[case(time!(00:01:00), time!(00:00:00.1))]
#[case(time!(00:00:01), time!(00:00:00.1))]
fn ordering_lexico_endianness(#[case] higher: Time, #[case] lower: Time) {
    assert!(higher > lower);
}

#[rstest]
#[case(time!(0:00), time!(01:00:00.1), (-3600.1).seconds())]
#[case(time!(0:00), time!(23:59:59.999_999_999), (-86_399.999_999_999).seconds())]
fn issue_481(#[case] a: Time, #[case] b: Time, #[case] expected: SignedDuration) {
    assert_eq!(a - b, expected);
}
