use std::fmt::Debug;
use std::num::NonZero;

use rstest::rstest;
use time::format_description::well_known::{Iso8601, Rfc2822, Rfc3339};
use time::format_description::{
    Component, FormatDescriptionV3, OwnedFormatItem, StaticFormatDescription, modifier,
};
use time::macros::{
    date, datetime, format_description as fd, offset, time, timestamp, utc_datetime,
};
use time::parsing::Parsed;
use time::{
    Date, Month, OffsetDateTime, PlainDateTime, Time, Timestamp, UtcDateTime, UtcOffset, Weekday,
    error, format_description as fd,
};

#[rstest]
#[case("Sat, 02 Jan 2021 03:04:05 GMT", datetime!(2021-01-02 03:04:05 UTC))]
#[case("Sat, 02 Jan 2021 03:04:05 UT", datetime!(2021-01-02 03:04:05 UTC))]
#[case("Sat, 02 Jan 2021 03:04:05 +0000", datetime!(2021-01-02 03:04:05 UTC))]
#[case("Sat, 02 Jan 2021 03:04:05 +0607", datetime!(2021-01-02 03:04:05 +06:07))]
#[case("Sat, 02 Jan 2021 03:04:05 -0607", datetime!(2021-01-02 03:04:05 -06:07))]
#[case("Fri, 31 Dec 2021 23:59:60 Z", datetime!(2021-12-31 23:59:59.999_999_999 UTC))]
#[case("Fri, 31 Dec 2021 23:59:60 z", datetime!(2021-12-31 23:59:59.999_999_999 UTC))]
#[case("Fri, 31 Dec 2021 23:59:60 a", datetime!(2021-12-31 23:59:59.999_999_999 UTC))]
#[case("Fri, 31 Dec 2021 23:59:60 A", datetime!(2021-12-31 23:59:59.999_999_999 UTC))]
#[case("Fri, 31 Dec 2021 17:52:60 -0607", datetime!(2021-12-31 17:52:59.999_999_999 -06:07))]
#[case("Sat, 01 Jan 2022 06:06:60 +0607", datetime!(2022-01-01 06:06:59.999_999_999 +06:07))]
fn rfc_2822_odt(#[case] input: &str, #[case] expected: OffsetDateTime) {
    assert_eq!(OffsetDateTime::parse(input, &Rfc2822).ok(), Some(expected));
}

#[rstest]
#[case("Sat, 02 Jan 2021 03:04:05 GMT", utc_datetime!(2021-01-02 03:04:05))]
#[case("Sat, 02 Jan 2021 03:04:05 UT", utc_datetime!(2021-01-02 03:04:05))]
#[case("Sat, 02 Jan 2021 03:04:05 +0000", utc_datetime!(2021-01-02 03:04:05))]
#[case("Sat, 02 Jan 2021 03:04:05 +0607", datetime!(2021-01-02 03:04:05 +06:07).to_utc())]
#[case("Sat, 02 Jan 2021 03:04:05 -0607", datetime!(2021-01-02 03:04:05 -06:07).to_utc())]
#[case("Fri, 31 Dec 2021 23:59:60 Z", utc_datetime!(2021-12-31 23:59:59.999_999_999))]
#[case("Fri, 31 Dec 2021 23:59:60 z", utc_datetime!(2021-12-31 23:59:59.999_999_999))]
#[case("Fri, 31 Dec 2021 23:59:60 a", utc_datetime!(2021-12-31 23:59:59.999_999_999))]
#[case("Fri, 31 Dec 2021 23:59:60 A", utc_datetime!(2021-12-31 23:59:59.999_999_999))]
#[case(
    "Fri, 31 Dec 2021 17:52:60 -0607",
    datetime!(2021-12-31 17:52:59.999_999_999 -06:07).to_utc()
)]
#[case(
    "Sat, 01 Jan 2022 06:06:60 +0607",
    datetime!(2022-01-01 06:06:59.999_999_999 +06:07).to_utc()
)]
fn rfc_2822_udt(#[case] input: &str, #[case] expected: UtcDateTime) {
    assert_eq!(UtcDateTime::parse(input, &Rfc2822).ok(), Some(expected));
}

#[rstest]
#[case("Sat, 02 Jan 2021 03:04:05 GMT", date!(2021-01-02))]
#[case("Sat, 02 Jan 2021 03:04:05 +0607", date!(2021-01-02))]
#[case("Sat, 02 Jan 2021 03:04:05 -0607", date!(2021-01-02))]
#[case("Sat, 02 Jan 21 03:04:05 -0607", date!(2021-01-02))]
#[case("Sat, 02 Jan 71 03:04:05 -0607", date!(1971-01-02))]
fn rfc_2822_date(#[case] input: &str, #[case] expected: Date) {
    assert_eq!(Date::parse(input, &Rfc2822).ok(), Some(expected));
}

#[rstest]
fn rfc_2822_time() {
    #[rustfmt::skip]
    assert_eq!(
        Time::parse(
            "  \t Sat,\r\n \
            (\tfoo012FOO!)\
            (\u{1}\u{b}\u{e}\u{7f})\
            (\\\u{0})\
            (\\\u{1}\\\u{9}\\\u{28}\\\u{29}\\\\u{5c}\\\u{7f})\
            (\\\n\\\u{b})\
            02 \r\n  \r\n Jan 2021 03:04:05 GMT",
            &Rfc2822
        ).ok(),
        Some(time!(03:04:05))
    );
}

#[rstest]
#[case("02 Jan 2021 03:04:05 +0607", datetime!(2021-01-02 03:04:05 +06:07))]
fn issue_661_odt(#[case] input: &str, #[case] expected: OffsetDateTime) {
    assert_eq!(OffsetDateTime::parse(input, &Rfc2822).ok(), Some(expected));
}

#[rstest]
#[case("02 Jan 2021 03:04:05 +0607", date!(2021-01-02))]
fn issue_661_date(#[case] input: &str, #[case] expected: Date) {
    assert_eq!(Date::parse(input, &Rfc2822).ok(), Some(expected));
}

#[rstest]
#[case(" \r\nM", "day")]
#[case("Mon, o2", "day")]
#[case("Mon, 02 jxn", "month")]
#[case("Mon, 02 Jan abcd", "year")]
#[case("Mon, 02 Jan 1899", "year")]
#[case("Mon, 02 Jan 2021 ab", "hour")]
#[case("Mon, 02 Jan 2021 03:04:ab", "second")]
#[case("Mon, 02 Jan 2021 03:04 6", "offset hour")]
#[case("Mon, 02 Jan 2021 03:04:05 -6", "offset hour")]
#[case("Mon, 02 Jan 2021 03:04:05 -060", "offset minute")]
fn rfc_2822_err_invalid_component(#[case] input: &str, #[case] component_name: &str) {
    assert!(matches!(
        OffsetDateTime::parse(input, &Rfc2822),
        Err(error::Parse::ParseFromDescription(error::ParseFromDescription::InvalidComponent(name)))
            if name == component_name
    ));
}

#[rstest]
#[case("Mon:")]
#[case("Mon, 02_")]
#[case("Mon, 02 Jan_")]
#[case("Mon, 02 Jan 2021_")]
#[case("Mon, 02 Jan 21_")]
#[case("Mon, 02 Jan 2021 03_")]
#[case("Mon, 02 Jan 2021 03:04_")]
#[case("Mon, 02 Jan 2021 03:04:05_")]
#[case("Mon, 02 Jan 2021 03:04:05:ab")]
fn rfc_2822_err_invalid_literal(#[case] input: &str) {
    assert!(matches!(
        OffsetDateTime::parse(input, &Rfc2822),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidLiteral { .. }
        ))
    ));
}

#[rstest]
#[case("Fri, 31 Dec 2021 23:59:61 Z", false)]
#[case("Fri, 31 Dec 2021 03:04:60 Z", true)]
#[case("Fri, 30 Dec 2021 23:59:60 Z", true)]
fn rfc_2822_err_component_range(#[case] input: &str, #[case] conditional: bool) {
    assert!(matches!(
        OffsetDateTime::parse(input, &Rfc2822),
        Err(error::Parse::TryFromParsed(error::TryFromParsed::ComponentRange(component)))
            if component.name() == "second" && component.is_conditional() == conditional
    ));
}

#[rstest]
#[case("2021-01-02T03:04:05Z", datetime!(2021-01-02 03:04:05 UTC))]
#[case("2021-12-31T23:59:60Z", datetime!(2021-12-31 23:59:59.999_999_999 UTC))]
#[case("2015-07-01T00:59:60+01:00", datetime!(2015-06-30 23:59:59.999_999_999 UTC))]
#[case("2021-01-02T03:04:05.1Z", datetime!(2021-01-02 03:04:05.1 UTC))]
#[case("2021-01-02T03:04:05.12Z", datetime!(2021-01-02 03:04:05.12 UTC))]
#[case("2021-01-02T03:04:05.123Z", datetime!(2021-01-02 03:04:05.123 UTC))]
#[case("2021-01-02T03:04:05.1234Z", datetime!(2021-01-02 03:04:05.123_4 UTC))]
#[case("2021-01-02T03:04:05.12345Z", datetime!(2021-01-02 03:04:05.123_45 UTC))]
#[case("2021-01-02T03:04:05.123456Z", datetime!(2021-01-02 03:04:05.123_456 UTC))]
#[case("2021-01-02T03:04:05.1234567Z", datetime!(2021-01-02 03:04:05.123_456_7 UTC))]
#[case("2021-01-02T03:04:05.12345678Z", datetime!(2021-01-02 03:04:05.123_456_78 UTC))]
#[case("2021-01-02T03:04:05.123456789Z", datetime!(2021-01-02 03:04:05.123_456_789 UTC))]
#[case("2021-01-02T03:04:05.123456789-01:02", datetime!(2021-01-02 03:04:05.123_456_789 -01:02))]
#[case("2021-01-02T03:04:05.123456789+01:02", datetime!(2021-01-02 03:04:05.123_456_789 +01:02))]
#[case("2021-01-02T03:04:05.123-00:01", datetime!(2021-01-02 03:04:05.123 -00:01))]
#[case("2021-01-02 03:04:05Z", datetime!(2021-01-02 03:04:05 UTC))]
#[case("2021-01-02$03:04:05Z", datetime!(2021-01-02 03:04:05 UTC))]
fn rfc_3339_odt(#[case] input: &str, #[case] expected: OffsetDateTime) {
    assert_eq!(OffsetDateTime::parse(input, &Rfc3339).ok(), Some(expected));
}

#[rstest]
#[case("2021-01-02T03:04:05Z", date!(2021-01-02))]
#[case("2021-01-02T03:04:05.123+01:02", date!(2021-01-02))]
#[case("2021-01-02T03:04:05.123-01:02", date!(2021-01-02))]
fn rfc_3339_date(#[case] input: &str, #[case] expected: Date) {
    assert_eq!(Date::parse(input, &Rfc3339).ok(), Some(expected));
}

#[rstest]
#[case("2021-01-02T03:04:05Z", offset!(UTC))]
#[case("2021-01-02T03:04:05.123+01:02", offset!(+01:02))]
#[case("2021-01-02T03:04:05.123-01:02", offset!(-01:02))]
#[case("2021-01-02T03:04:05.123-00:01", offset!(-00:01))]
fn rfc_3339_utc_offset(#[case] input: &str, #[case] expected: UtcOffset) {
    assert_eq!(UtcOffset::parse(input, &Rfc3339).ok(), Some(expected));
}

#[rstest]
#[case("x", "year")]
#[case("2021-x", "month")]
#[case("2021-0", "month")]
#[case("2021-01-0", "day")]
#[case("2021-01-01", "separator")]
#[case("2021-01-01T0", "hour")]
#[case("2021-01-01T00:0", "minute")]
#[case("2021-01-01T00:00:0", "second")]
#[case("2021-01-01T00:00:00.x", "subsecond")]
#[case("2021-01-01T00:00:00x", "offset hour")]
#[case("2021-01-01T00:00:00+0", "offset hour")]
#[case("2021-01-01T00:00:00+00:0", "offset minute")]
#[case("2021-01-01T00:00:00+24:00", "offset hour")]
fn rfc_3339_err_invalid_component(#[case] input: &str, #[case] component_name: &str) {
    assert!(matches!(
        OffsetDateTime::parse(input, &Rfc3339),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case("2021x")]
#[case("2021-01x")]
#[case("2021-01-01T00x")]
#[case("2021-01-01T00:00x")]
#[case("2021-01-01T00:00:00+00x")]
fn rfc_3339_err_invalid_literal(#[case] input: &str) {
    assert!(matches!(
        OffsetDateTime::parse(input, &Rfc3339),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidLiteral { .. }
        ))
    ));
}

#[rstest]
#[case("2021-13-01T00:00:00Z", "month", false)]
#[case("2021-12-31T23:59:61Z", "second", false)]
#[case("2021-01-02T23:59:60Z", "second", true)]
#[case("2021-12-31T03:04:60Z", "second", true)]
#[case("2021-12-31T23:59:60+01:00", "second", true)]
#[case("2021-01-01T00:00:00+00:60", "offset minute", false)]
fn rfc_3339_err_component_range(
    #[case] input: &str,
    #[case] component_name: &str,
    #[case] is_conditional: bool,
) {
    assert!(matches!(
        OffsetDateTime::parse(input, &Rfc3339),
        Err(error::Parse::TryFromParsed(error::TryFromParsed::ComponentRange(component)))
            if component.name() == component_name && component.is_conditional() == is_conditional
    ));
}

#[rstest]
#[case("2021-01-02T03:04:05Z ")]
fn rfc_3339_err_unexpected_trailing_offset(#[case] input: &str) {
    assert!(matches!(
        OffsetDateTime::parse(input, &Rfc3339),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::UnexpectedTrailingCharacters { .. }
        ))
    ));
}

#[rstest]
#[case("2021-01-02T03:04:60Z", "second", false)]
fn rfc_3339_err_component_range_pdt(
    #[case] input: &str,
    #[case] component_name: &str,
    #[case] is_conditional: bool,
) {
    assert!(matches!(
        PlainDateTime::parse(input, &Rfc3339),
        Err(error::Parse::TryFromParsed(error::TryFromParsed::ComponentRange(component)))
            if component.name() == component_name && component.is_conditional() == is_conditional
    ));
}

#[rstest]
#[case("2021x")]
#[case("2021-01x")]
#[case("2021-01-01T00x")]
#[case("2021-01-01T00:00x")]
#[case("2021-01-01T00:00:00+00x")]
fn rfc_3339_err_invalid_literal_pdt(#[case] input: &str) {
    assert!(matches!(
        PlainDateTime::parse(input, &Rfc3339),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidLiteral { .. }
        ))
    ));
}

#[rstest]
#[case("x", "year")]
#[case("2021-x", "month")]
#[case("2021-0", "month")]
#[case("2021-13-01T00:00:00Z", "month")]
#[case("2021-01-0", "day")]
#[case("2021-01-01", "separator")]
#[case("2021-01-01T0", "hour")]
#[case("2021-01-01T00:0", "minute")]
#[case("2021-01-01T00:00:0", "second")]
#[case("2021-01-01T00:00:00.x", "subsecond")]
#[case("2021-01-01T00:00:00x", "offset hour")]
#[case("2021-01-01T00:00:00+0", "offset hour")]
#[case("2021-01-01T00:00:00+00:0", "offset minute")]
#[case("2021-01-01T00:00:00+24:00", "offset hour")]
fn rfc_3339_err_invalid_component_pdt(#[case] input: &str, #[case] component_name: &str) {
    assert!(matches!(
        PlainDateTime::parse(input, &Rfc3339),
        Err(error::Parse::ParseFromDescription(error::ParseFromDescription::InvalidComponent(name)))
            if name == component_name
    ));
}

#[rstest]
#[case("2022-01-01T00:59:60+01:00", false)]
#[case("2021-12-31T23:04:60Z", false)]
fn rfc_3339_err_component_range_time(#[case] input: &str, #[case] is_conditional: bool) {
    assert!(matches!(
        Time::parse(input, &Rfc3339),
        Err(error::Parse::TryFromParsed(error::TryFromParsed::ComponentRange(component)))
            if component.name() == "second" && component.is_conditional() == is_conditional
    ));
}

#[rstest]
#[case("2021-01-02T03:04:05Z", datetime!(2021-01-02 03:04:05 UTC))]
#[case("2021-002T03:04:05Z", datetime!(2021-002 03:04:05 UTC))]
#[case("2021-W01-2T03:04:05Z", datetime!(2021-W01-2 03:04:05 UTC))]
#[case("-002021-01-02T03:04:05+01:00", datetime!(-002021-01-02 03:04:05 +01:00))]
#[case("20210102T03.1Z", datetime!(2021-01-02 03:06:00 UTC))]
#[case("2021002T0304.1Z", datetime!(2021-002 03:04:06 UTC))]
#[case("2021W012T030405.1-0100", datetime!(2021-W01-2 03:04:05.1 -01:00))]
#[case("20210102T03Z", datetime!(2021-01-02 03:00:00 UTC))]
#[case("20210102T0304Z", datetime!(2021-01-02 03:04:00 UTC))]
fn iso_8601_odt(#[case] input: &str, #[case] expected: OffsetDateTime) {
    assert_eq!(
        OffsetDateTime::parse(input, &Iso8601::DEFAULT).ok(),
        Some(expected)
    );
}

#[rstest]
#[case("2021-01-02T03:04:05Z", utc_datetime!(2021-01-02 03:04:05))]
#[case("2021-002T03:04:05Z", utc_datetime!(2021-002 03:04:05))]
#[case("2021-W01-2T03:04:05Z", utc_datetime!(2021-W01-2 03:04:05))]
#[case("-002021-01-02T03:04:05+01:00", datetime!(-002021-01-02 03:04:05 +01:00).to_utc())]
#[case("20210102T03.1Z", utc_datetime!(2021-01-02 03:06:00))]
#[case("2021002T0304.1Z", utc_datetime!(2021-002 03:04:06))]
#[case("2021W012T030405.1-0100", datetime!(2021-W01-2 03:04:05.1 -01:00).to_utc())]
#[case("20210102T03Z", utc_datetime!(2021-01-02 03:00:00))]
#[case("20210102T0304Z", utc_datetime!(2021-01-02 03:04:00))]
fn iso_8601_udt(#[case] input: &str, #[case] expected: UtcDateTime) {
    assert_eq!(
        UtcDateTime::parse(input, &Iso8601::DEFAULT).ok(),
        Some(expected)
    );
}

#[rstest]
#[case("+07", offset!(+7))]
#[case("+0304", offset!(+03:04))]
fn iso_8601_offset(#[case] input: &str, #[case] expected: UtcOffset) {
    assert_eq!(
        UtcOffset::parse(input, &Iso8601::DEFAULT).ok(),
        Some(expected)
    );
}

#[rstest]
#[case("2022-07-22T12:52:50.349409", datetime!(2022-07-22 12:52:50.349409000))]
fn iso_8601_pdt(#[case] input: &str, #[case] expected: PlainDateTime) {
    assert_eq!(
        PlainDateTime::parse(input, &Iso8601::DEFAULT).ok(),
        Some(expected)
    );
}

#[rstest]
#[case("T01:02:03.123456789", time!(01:02:03.123_456_789))]
fn iso_8601_time(#[case] input: &str, #[case] expected: Time) {
    assert_eq!(Time::parse(input, &Iso8601::DEFAULT).ok(), Some(expected));
}

#[rstest]
#[case("20210102T03:04Z")]
#[case("20210102T03.")]
#[case("2021-0102")]
#[case("2021-01-x")]
#[case("2021-Wx")]
#[case("2021-W012")]
#[case("2021-W01-x")]
#[case("2021-01-02T03:x")]
#[case("2021-01-02T03:04x")]
#[case("2021-01-02T03:04:")]
fn iso_8601_unexpected_trailing_odt(#[case] input: &str) {
    assert!(matches!(
        OffsetDateTime::parse(input, &Iso8601::DEFAULT),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::UnexpectedTrailingCharacters { .. }
        ))
    ));
}

#[rstest]
#[case("20210102T03:04Z")]
#[case("20210102T03.")]
#[case("2021-0102")]
#[case("2021-01-x")]
#[case("2021-Wx")]
#[case("2021-W012")]
#[case("2021-W01-x")]
#[case("2021-01-02T03:x")]
#[case("2021-01-02T03:04x")]
#[case("2021-01-02T03:04:")]
fn iso_8601_unexpected_trailing_udt(#[case] input: &str) {
    assert!(matches!(
        UtcDateTime::parse(input, &Iso8601::DEFAULT),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::UnexpectedTrailingCharacters { .. }
        ))
    ));
}

#[rstest]
fn iso_8601_error_insufficient_type_information() {
    assert_eq!(
        UtcDateTime::parse("01:02", &Iso8601::DEFAULT),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case(fd!("[hour repr:12] [period]"), "01 PM", time!(1 PM))]
#[case(fd!("[hour]"), "12", time!(12:00))]
#[case(fd!("[hour]:[minute]:[second]"), "13:02:03", time!(13:02:03))]
#[case(fd!("[hour repr:12]:[minute] [period]"), "01:02 PM", time!(1:02 PM))]
#[case(fd!("[hour]:[minute]"), "01:02", time!(1:02))]
#[case(fd!("[hour repr:12]:[minute] [period]"), "01:02 AM", time!(1:02 AM))]
#[case(fd!("[hour]:[minute]"), "01:02", time!(1:02))]
#[case(fd!("[hour repr:12] [period]"), "12 AM", time!(12 AM))]
#[case(fd!("[hour repr:12] [period]"), "12 PM", time!(12 PM))]
fn parse_time(
    #[case] format_description: StaticFormatDescription,
    #[case] input: &str,
    #[case] expected: Time,
) {
    assert_eq!(Time::parse(input, format_description).ok(), Some(expected));
    assert_eq!(
        Time::parse(input, &OwnedFormatItem::from(format_description)).ok(),
        Some(expected)
    );
    assert_eq!(
        Time::parse(
            input,
            [OwnedFormatItem::from(format_description)].as_slice()
        )
        .ok(),
        Some(expected)
    );
}

#[rstest]
fn parse_time_insufficient_information_standalone() {
    assert_eq!(
        Time::try_from(Parsed::new()),
        Err(error::TryFromParsed::InsufficientInformation)
    );
}

#[rstest]
#[case("", fd!(""))]
#[case("12:34", fd!("[hour]:[second]"))]
#[case("12:34", fd!("[hour]:[subsecond]"))]
fn parse_time_insufficient_information(#[case] input: &str, #[case] fd: StaticFormatDescription) {
    assert_eq!(
        Time::parse(input, fd),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case(" ", fd!(""))]
fn parse_time_unexpected_trailing_characters(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
) {
    assert!(matches!(
        Time::parse(input, fd),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::UnexpectedTrailingCharacters { .. }
        ))
    ));
}

#[rstest]
#[case("13 PM", fd!("[hour repr:12] [period]"), "hour")]
#[case("a", fd!("[subsecond digits:1]"), "subsecond")]
#[case("1a", fd!("[subsecond digits:2]"), "subsecond")]
#[case("12a", fd!("[subsecond digits:3]"), "subsecond")]
#[case("123a", fd!("[subsecond digits:4]"), "subsecond")]
#[case("1234a", fd!("[subsecond digits:5]"), "subsecond")]
#[case("12345a", fd!("[subsecond digits:6]"), "subsecond")]
#[case("123456a", fd!("[subsecond digits:7]"), "subsecond")]
#[case("1234567a", fd!("[subsecond digits:8]"), "subsecond")]
#[case("12345678a", fd!("[subsecond digits:9]"), "subsecond")]
fn parse_time_invalid_component(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] component_name: &str,
) {
    assert!(matches!(
        Time::parse(input, fd),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case(fd!("[year]-[month]-[day]"), "2021-01-02", date!(2021-01-02))]
#[case(
    fd!("[year repr:century range:standard][year repr:last_two]-[month]-[day]"),
    "2021-01-02",
    date!(2021-01-02)
)]
#[case(fd!("[year]-[ordinal]"), "2021-002", date!(2021-002))]
#[case(
    fd!("[year base:iso_week]-W[week_number]-[weekday repr:monday]"),
    "2020-W53-6",
    date!(2021-01-02)
)]
#[case(
    fd!("[year]-W[week_number repr:monday]-[weekday repr:monday]"),
    "2021-W00-6",
    date!(2021-01-02)
)]
#[case(
    fd!("[year]-W[week_number repr:sunday]-[weekday repr:sunday]"),
    "2021-W00-6",
    date!(2021-01-02)
)]
#[case(
    fd!("[year]-W[week_number repr:sunday]-[weekday repr:sunday]"),
    "2023-W01-1",
    date!(2023-01-02)
)]
#[case(
    fd!("[year]-W[week_number repr:sunday]-[weekday repr:sunday]"),
    "2022-W00-7",
    date!(2022-01-02)
)]
#[case(
    fd!("[year]-W[week_number repr:sunday]-[weekday repr:sunday]"),
    "2026-W00-5",
    date!(2026-01-02)
)]
#[case(
    fd!("[year]-W[week_number repr:sunday]-[weekday repr:sunday]"),
    "2025-W00-4",
    date!(2025-01-02)
)]
#[case(
    fd!("[year]-W[week_number repr:sunday]-[weekday repr:sunday]"),
    "2019-W00-3",
    date!(2019-01-02)
)]
#[case(
    fd!("[year]-W[week_number repr:sunday]-[weekday repr:sunday]"),
    "2018-W01-2",
    date!(2018-01-02)
)]
#[case(
    fd!("[year padding:space]-W[week_number repr:sunday padding:none]-[weekday repr:sunday]"),
    " 201-W01-2",
    date!(201-01-06)
)]
fn parse_date(
    #[case] format_description: StaticFormatDescription,
    #[case] input: &str,
    #[case] expected: Date,
) {
    assert_eq!(Date::parse(input, format_description).ok(), Some(expected));
    assert_eq!(
        Date::parse(input, &OwnedFormatItem::from(format_description)).ok(),
        Some(expected)
    );
}

#[rstest]
fn parse_date_insufficient_information_standalone() {
    assert_eq!(
        Date::try_from(Parsed::new()),
        Err(error::TryFromParsed::InsufficientInformation)
    );
}

#[rstest]
#[case("", fd!(""))]
fn parse_date_insufficient_information(#[case] input: &str, #[case] fd: StaticFormatDescription) {
    assert!(matches!(
        Date::parse(input, fd),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    ));
}

#[rstest]
#[case("a", fd!("[year]"), "year")]
#[case("0001", fd!("[year sign:mandatory]"), "year")]
#[case("0a", fd!("[year repr:last_two]"), "year")]
#[case("2021-12-32", fd!("[year]-[month]-[day]"), "day")]
#[case(
    "2021-W54-1",
    fd!("[year base:iso_week]-W[week_number]-[weekday repr:monday]"),
    "week number"
)]
#[case("2021-W54-1", fd!("[year]-W[week_number repr:monday]-[weekday repr:monday]"), "week number")]
#[case("2021-W54-1", fd!("[year]-W[week_number repr:sunday]-[weekday repr:sunday]"), "week number")]
#[case("Ja", fd!("[month repr:short]"), "month")]
#[case("  2a21", fd!("[year padding:space]"), "year")]
fn parse_date_invalid_component(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] component_name: &str,
) {
    assert!(matches!(
        Date::parse(input, fd),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case("2021-366", fd!("[year]-[ordinal]"), "ordinal")]
#[case("2021-02-30", fd!("[year]-[month]-[day]"), "day")]
#[case("2019-W53-1", fd!("[year base:iso_week]-W[week_number]-[weekday repr:monday]"), "week")]
#[case("2019-W53-1", fd!("[year]-W[week_number repr:sunday]-[weekday repr:monday]"), "ordinal")]
#[case("2019-W53-1", fd!("[year]-W[week_number repr:monday]-[weekday repr:monday]"), "ordinal")]
fn parse_date_component_range(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] component_name: &str,
) {
    assert!(matches!(
        Date::parse(input, fd),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::ComponentRange(component)
        )) if component.name() == component_name
    ));
}

#[rstest]
#[case("-00:01", fd!("[offset_hour sign:mandatory]:[offset_minute]"), offset!(-00:01))]
#[case(
    "-00:00:01",
    fd!("[offset_hour sign:mandatory]:[offset_minute]:[offset_second]"),
    offset!(-00:00:01)
)]
fn parse_offset(
    #[case] input: &str,
    #[case] format_description: StaticFormatDescription,
    #[case] expected: UtcOffset,
) {
    assert_eq!(
        UtcOffset::parse(input, format_description).ok(),
        Some(expected)
    );
    assert_eq!(
        UtcOffset::parse(input, &OwnedFormatItem::from(format_description)).ok(),
        Some(expected)
    );
}

#[rstest]
fn parse_offset_insufficient_information_standalone() {
    assert_eq!(
        UtcOffset::try_from(Parsed::new()),
        Err(error::TryFromParsed::InsufficientInformation)
    );
}

#[rstest]
#[case("01", fd!("[offset_hour sign:mandatory]"), "offset hour")]
#[case("26", fd!("[offset_hour]"), "offset hour")]
#[case("00:60", fd!("[offset_hour]:[offset_minute]"), "offset minute")]
#[case("00:00:60", fd!("[offset_hour]:[offset_minute]:[offset_second]"), "offset second")]
fn parse_offset_invalid_component(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] component_name: &str,
) {
    assert!(matches!(
        UtcOffset::parse(input, fd),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case("2023-07-27 23", fd!("[year]-[month]-[day] [hour]"), datetime!(2023-07-27 23:00))]
fn parse_plain_date_time(
    #[case] input: &str,
    #[case] format_description: StaticFormatDescription,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(
        PlainDateTime::parse(input, format_description).ok(),
        Some(expected)
    );
    assert_eq!(
        PlainDateTime::parse(input, &OwnedFormatItem::from(format_description)).ok(),
        Some(expected)
    );
}

#[rstest]
fn parse_plain_date_time_insufficient_information_standalone() {
    assert_eq!(
        PlainDateTime::try_from(Parsed::new()),
        Err(error::TryFromParsed::InsufficientInformation)
    );
}

#[rstest]
#[case("2021-001 13 PM", fd!("[year]-[ordinal] [hour repr:12] [period]"), "hour")]
fn parse_plain_date_time_invalid_component(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] component_name: &str,
) {
    assert!(matches!(
        PlainDateTime::parse(input, fd),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case("2023-07-27 23:30", fd!("[year]-[month]-[day] [hour]"))]
fn parse_plain_date_time_unexpected_trailing_characters(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
) {
    assert!(matches!(
        PlainDateTime::parse(input, fd),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::UnexpectedTrailingCharacters { .. }
        ))
    ));
}

#[rstest]
fn parse_offset_date_time_insufficient_information_standalone() {
    assert_eq!(
        OffsetDateTime::try_from(Parsed::new()),
        Err(error::TryFromParsed::InsufficientInformation)
    );
}

#[rstest]
#[case("", fd!(""))]
fn parse_offset_date_time_insufficient_information(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
) {
    assert_eq!(
        OffsetDateTime::parse(input, fd),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case("x", fd!("[year]"), "year")]
#[case(
    "2021-001 12 PM +26",
    fd!("[year]-[ordinal] [hour repr:12] [period] [offset_hour sign:mandatory]"),
    "offset hour"
)]
fn parse_offset_date_time_invalid_component(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] component_name: &str,
) {
    assert!(matches!(
        OffsetDateTime::parse(input, fd),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case("2023-07-27 23", fd!("[year]-[month]-[day] [hour]"), utc_datetime!(2023-07-27 23:00))]
fn parse_utc_date_time(
    #[case] input: &str,
    #[case] format_description: StaticFormatDescription,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(
        UtcDateTime::parse(input, format_description).ok(),
        Some(expected)
    );
    assert_eq!(
        UtcDateTime::parse(input, &OwnedFormatItem::from(format_description)).ok(),
        Some(expected)
    );
}

#[rstest]
fn parse_utc_date_time_insufficient_information_standalone() {
    assert_eq!(
        UtcDateTime::try_from(Parsed::new()),
        Err(error::TryFromParsed::InsufficientInformation)
    );
}

#[rstest]
#[case("", fd!(""))]
fn parse_utc_date_time_insufficient_information(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
) {
    assert_eq!(
        UtcDateTime::parse(input, fd),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case("2021 001 13 PM", fd!("[year] [ordinal] [hour repr:12] [period]"), "hour")]
#[case("x", fd!("[year]"), "year")]
#[case(
    "2021-001 12 PM +26",
    fd!("[year]-[ordinal] [hour repr:12] [period] [offset_hour sign:mandatory]"),
    "offset hour"
)]
fn parse_utc_date_time_invalid_component(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] component_name: &str,
) {
    assert!(matches!(
        UtcDateTime::parse(input, fd),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
fn parse_utc_date_time_unexpected_trailing_characters() {
    assert!(matches!(
        UtcDateTime::parse("2023-07-27 23:30", &fd!("[year]-[month]-[day] [hour]")),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::UnexpectedTrailingCharacters { .. }
        ))
    ));
}

#[rstest]
#[case(
    Component::CalendarYearFullExtendedRange(
        modifier::CalendarYearFullExtendedRange::default()
            .with_padding(modifier::Padding::Zero)
            .with_sign_is_mandatory(false)
    ),
    "2021",
    Parsed::year,
    2021,
)]
#[case(
    Component::CalendarYearCenturyExtendedRange(
        modifier::CalendarYearCenturyExtendedRange::default()
            .with_padding(modifier::Padding::Zero)
            .with_sign_is_mandatory(false)
    ),
    "20",
    Parsed::year_century,
    20,
)]
#[case(
    Component::CalendarYearCenturyExtendedRange(
        modifier::CalendarYearCenturyExtendedRange::default()
            .with_padding(modifier::Padding::Zero)
            .with_sign_is_mandatory(false)
    ),
    "20",
    Parsed::year_century_is_negative,
    false,
)]
#[case(
    Component::CalendarYearLastTwo(
        modifier::CalendarYearLastTwo::default().with_padding(modifier::Padding::Zero)
    ),
    "21",
    Parsed::year_last_two,
    21,
)]
#[case(
    Component::IsoYearFullExtendedRange(
        modifier::IsoYearFullExtendedRange::default()
            .with_padding(modifier::Padding::Zero)
            .with_sign_is_mandatory(false)
    ),
    "2021",
    Parsed::iso_year,
    2021,
)]
#[case(
    Component::IsoYearCenturyExtendedRange(modifier::IsoYearCenturyExtendedRange::default()
        .with_padding(modifier::Padding::Zero)
        .with_sign_is_mandatory(false)
    ),
    "20",
    Parsed::iso_year_century,
    20,
)]
#[case(
    Component::IsoYearCenturyExtendedRange(modifier::IsoYearCenturyExtendedRange::default()
        .with_padding(modifier::Padding::Zero)
        .with_sign_is_mandatory(false)
    ),
    "20",
    Parsed::iso_year_century_is_negative,
    false,
)]
#[case(
    Component::IsoYearLastTwo(
        modifier::IsoYearLastTwo::default().with_padding(modifier::Padding::Zero)
    ),
    "21",
    Parsed::iso_year_last_two,
    21,
)]
#[case(
    Component::MonthNumerical(
        modifier::MonthNumerical::default().with_padding(modifier::Padding::Space)
    ),
    " 1",
    Parsed::month,
    Month::January,
)]
#[case(
    Component::MonthShort(modifier::MonthShort::default().with_case_sensitive(true)),
    "Jan",
    Parsed::month,
    Month::January,
)]
#[case(
    Component::MonthShort(modifier::MonthShort::default().with_case_sensitive(false)),
    "jAn",
    Parsed::month,
    Month::January,
)]
#[case(
    Component::MonthLong(modifier::MonthLong::default().with_case_sensitive(true)),
    "January",
    Parsed::month,
    Month::January,
)]
#[case(
    Component::MonthLong(modifier::MonthLong::default().with_case_sensitive(false)),
    "jAnUaRy",
    Parsed::month,
    Month::January,
)]
#[case(
    Component::Ordinal(modifier::Ordinal::default().with_padding(modifier::Padding::Zero)),
    "012",
    Parsed::ordinal,
    12.try_into().ok(),
)]
#[case(
    Component::WeekdayShort(modifier::WeekdayShort::default().with_case_sensitive(true)),
    "Sun",
    Parsed::weekday,
    Weekday::Sunday,
)]
#[case(
    Component::WeekdayShort(modifier::WeekdayShort::default().with_case_sensitive(false)),
    "sUn",
    Parsed::weekday,
    Weekday::Sunday,
)]
#[case(
    Component::WeekdayLong(modifier::WeekdayLong::default().with_case_sensitive(true)),
    "Sunday",
    Parsed::weekday,
    Weekday::Sunday,
)]
#[case(
    Component::WeekdayLong(modifier::WeekdayLong::default().with_case_sensitive(false)),
    "sUnDaY",
    Parsed::weekday,
    Weekday::Sunday,
)]
#[case(
    Component::WeekdaySunday(modifier::WeekdaySunday::default().with_one_indexed(false)),
    "0",
    Parsed::weekday,
    Weekday::Sunday,
)]
#[case(
    Component::WeekdaySunday(modifier::WeekdaySunday::default().with_one_indexed(true)),
    "1",
    Parsed::weekday,
    Weekday::Sunday,
)]
#[case(
    Component::WeekdayMonday(modifier::WeekdayMonday::default().with_one_indexed(false)),
    "6",
    Parsed::weekday,
    Weekday::Sunday,
)]
#[case(
    Component::WeekdayMonday(modifier::WeekdayMonday::default().with_one_indexed(true)),
    "7",
    Parsed::weekday,
    Weekday::Sunday,
)]
#[case(
    Component::WeekNumberSunday(
        modifier::WeekNumberSunday::default().with_padding(modifier::Padding::None)
    ),
    "2",
    Parsed::sunday_week_number,
    2,
)]
#[case(
    Component::WeekNumberMonday(
        modifier::WeekNumberMonday::default().with_padding(modifier::Padding::None)
    ),
    "2",
    Parsed::monday_week_number,
    2,
)]
#[case(
    Component::WeekNumberIso(
        modifier::WeekNumberIso::default().with_padding(modifier::Padding::None)
    ),
    "2",
    Parsed::iso_week_number,
    2.try_into().ok(),
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::One)
    ),
    "1",
    Parsed::subsecond,
    100_000_000,
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::Two)
    ),
    "12",
    Parsed::subsecond,
    120_000_000,
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::Three)
    ),
    "123",
    Parsed::subsecond,
    123_000_000,
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::Four)
    ),
    "1234",
    Parsed::subsecond,
    123_400_000,
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::Five)
    ),
    "12345",
    Parsed::subsecond,
    123_450_000,
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::Six)
    ),
    "123456",
    Parsed::subsecond,
    123_456_000,
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::Seven)
    ),
    "1234567",
    Parsed::subsecond,
    123_456_700,
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::Eight)
    ),
    "12345678",
    Parsed::subsecond,
    123_456_780,
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::Nine)
    ),
    "123456789",
    Parsed::subsecond,
    123_456_789,
)]
#[case(
    Component::Subsecond(
        modifier::Subsecond::default().with_digits(modifier::SubsecondDigits::OneOrMore)
    ),
    "123456789",
    Parsed::subsecond,
    123_456_789,
)]
#[case(
    Component::Period(
        modifier::Period::default()
            .with_is_uppercase(false)
            .with_case_sensitive(true)
    ),
    "am",
    Parsed::hour_12_is_pm,
    false,
)]
#[case(
    Component::Period(
        modifier::Period::default()
            .with_is_uppercase(false)
            .with_case_sensitive(false)
    ),
    "aM",
    Parsed::hour_12_is_pm,
    false,
)]
#[case(
    Component::UnixTimestampSecond(
        modifier::UnixTimestampSecond::default().with_sign_is_mandatory(false)
    ),
    "1234567890",
    Parsed::unix_timestamp_nanos,
    1_234_567_890_000_000_000,
)]
#[case(
    Component::UnixTimestampMillisecond(
        modifier::UnixTimestampMillisecond::default().with_sign_is_mandatory(false)
    ),
    "1234567890123",
    Parsed::unix_timestamp_nanos,
    1_234_567_890_123_000_000,
)]
#[case(
    Component::UnixTimestampMicrosecond(
        modifier::UnixTimestampMicrosecond::default().with_sign_is_mandatory(false)
    ),
    "1234567890123456",
    Parsed::unix_timestamp_nanos,
    1_234_567_890_123_456_000,
)]
#[case(
    Component::UnixTimestampNanosecond(
        modifier::UnixTimestampNanosecond::default().with_sign_is_mandatory(false)
    ),
    "1234567890123456789",
    Parsed::unix_timestamp_nanos,
    1_234_567_890_123_456_789,
)]
#[case(
    Component::UnixTimestampNanosecond(
        modifier::UnixTimestampNanosecond::default().with_sign_is_mandatory(false)
    ),
    "-1234567890123456789",
    Parsed::unix_timestamp_nanos,
    -1_234_567_890_123_456_789,
)]
fn parse_component<T>(
    #[case] component: Component,
    #[case] input: &str,
    #[case] property_accessor: fn(&Parsed) -> Option<T>,
    #[case] property_value: impl Into<Option<T>>,
) where
    T: PartialEq + Debug,
{
    let mut parsed = Parsed::new();
    parsed
        .parse_component(input.as_bytes(), component)
        .expect("parsing should succeed");
    assert_eq!(property_accessor(&parsed), property_value.into());
}

#[rstest]
#[case("abcdef", const { NonZero::new(3).unwrap() })]
fn parse_ignore_component(#[case] input: &str, #[case] count: NonZero<u16>) {
    let mut parsed = Parsed::new();
    let result = parsed.parse_component(
        input.as_bytes(),
        Component::Ignore(modifier::Ignore::count(count)),
    );
    assert_eq!(result, Ok(b"def".as_slice()));
}

#[rstest]
#[case("abcdef", const { NonZero::new(7).unwrap() })]
fn parse_ignore_component_too_short(#[case] input: &str, #[case] count: NonZero<u16>) {
    let mut parsed = Parsed::new();
    let result = parsed.parse_component(
        input.as_bytes(),
        Component::Ignore(modifier::Ignore::count(count)),
    );
    assert!(matches!(
        result,
        Err(error::ParseFromDescription::InvalidComponent("ignore"))
    ));
}

#[rstest]
#[case("£", fd!(version = 3, "[ignore count:1]"))]
#[case("€", fd!(version = 3, "[ignore count:1]"))]
#[case("€", fd!(version = 3, "[ignore count:2]"))]
#[case("🦀", fd!(version = 3, "[ignore count:1]"))]
#[case("🦀", fd!(version = 3, "[ignore count:2]"))]
#[case("🦀", fd!(version = 3, "[ignore count:3]"))]
fn parse_ignore_component_v3_not_utf8_boundary(
    #[case] input: &str,
    #[case] fd: FormatDescriptionV3<'_>,
) {
    let result = Date::parse(input, &fd);
    assert_eq!(
        result,
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent("ignore")
        ))
    );
}

#[rstest]
#[case("£", const { NonZero::new(1).unwrap() })]
#[case("€", const { NonZero::new(1).unwrap() })]
#[case("€", const { NonZero::new(2).unwrap() })]
#[case("🦀", const { NonZero::new(1).unwrap() })]
#[case("🦀", const { NonZero::new(2).unwrap() })]
#[case("🦀", const { NonZero::new(3).unwrap() })]
fn parse_ignore_component_v2_not_utf8_boundary(#[case] input: &str, #[case] count: NonZero<u16>) {
    let mut parsed = Parsed::new();
    let result = parsed.parse_component(
        input.as_bytes(),
        Component::Ignore(modifier::Ignore::count(count)),
    );
    assert!(result.is_ok());
}

#[rstest]
fn year_default_modifier_by_version_runtime() -> time::Result<()> {
    let input = "+10000-01-01";

    let fd_v2 = fd::parse_borrowed::<2>("[year]-[month]-[day]")?;
    let fd_v3 = fd::parse_borrowed::<3>("[year]-[month]-[day]")?;

    // v2 defaults to the extended range and should accept +10000
    assert!(Date::parse(input, &fd_v2).is_ok());

    // v3 should fail because the runtime parser defaults to the standard range. This will consume
    // the sign and four digits, attempting to parse a `-` where the final `0` in the year exists.
    assert!(matches!(
        Date::parse(input, &fd_v3),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidLiteral { .. }
        ))
    ));

    Ok(())
}

#[rstest]
fn year_default_modifier_by_version_macro() {
    let input = "+10000-01-01";

    let fd_v2 = fd!(version = 2, "[year]-[month]-[day]");
    let fd_v3 = fd!(version = 3, "[year]-[month]-[day]");

    // v2 defaults to the extended range and should accept +10000
    assert!(Date::parse(input, &fd_v2).is_ok());

    // v3 should fail because the runtime parser defaults to the standard range. This will consume
    // the sign and four digits, attempting to parse a `-` where the final `0` in the year exists.
    assert!(matches!(
        Date::parse(input, &fd_v3),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidLiteral { .. }
        ))
    ));
}

#[rstest]
#[case(
    "2021-01-02",
    fd!(version = 2, "[optional [[year]-[month]-[day]]]"),
    "",
    (2021, Month::January, 2)
)]
#[case(
    "2021-01",
    fd!(version = 2, "[optional [[year]-[month]-[day]]]"),
    "2021-01",
    (None, None, None),
)]
fn parse_optional(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] remaining_input: &str,
    #[case] (year, month, day): (
        impl Into<Option<i32>>,
        impl Into<Option<Month>>,
        impl Into<Option<u8>>,
    ),
) {
    let mut parsed = Parsed::new();
    let remaining = parsed
        .parse_items(input.as_bytes(), fd)
        .expect("parsing should succeed");
    assert_eq!(remaining, remaining_input.as_bytes());
    assert_eq!(parsed.year(), year.into());
    assert_eq!(parsed.month(), month.into());
    assert_eq!(parsed.day().map(NonZero::get), day.into());
}

#[rstest]
#[case(
    "2021-01-02",
    fd!(version = 2, "[first [[year]-[month]-[day]]]"),
    "",
    (2021, Month::January, 2)
)]
#[case("2021-01-02", fd!(version = 2, "[first []]"), "2021-01-02", (None, None, None))]
#[case(
    "2021-01-02",
    fd!(version = 2, "[first [[period]][x][[year]-[month]-[day]]]"),
    "",
    (2021, Month::January, 2)
)]
fn parse_first_item(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] remaining_input: &str,
    #[case] (year, month, day): (
        impl Into<Option<i32>>,
        impl Into<Option<Month>>,
        impl Into<Option<u8>>,
    ),
) {
    let mut parsed = Parsed::new();
    let remaining = parsed
        .parse_items(input.as_bytes(), fd)
        .expect("parsing should succeed");
    assert_eq!(remaining, remaining_input.as_bytes());
    assert_eq!(parsed.year(), year.into());
    assert_eq!(parsed.month(), month.into());
    assert_eq!(parsed.day().map(NonZero::get), day.into());
}

#[rstest]
#[case("2021-01-02", fd!(version = 2, "[first [[period]][x]]"), "period")]
fn parse_first_item_err(
    #[case] input: &str,
    #[case] fd: StaticFormatDescription,
    #[case] component_name: &str,
) {
    let mut parsed = Parsed::new();
    let err = parsed
        .parse_items(input.as_bytes(), fd)
        .expect_err("parsing should fail");
    assert!(matches!(
        err,
        error::ParseFromDescription::InvalidComponent(name) if name == component_name
    ));
}

#[rstest]
#[case("1234567890", fd!("[unix_timestamp]"), datetime!(2009-02-13 23:31:30 UTC))]
#[case(
    "1234567890123",
    fd!("[unix_timestamp precision:millisecond]"),
    datetime!(2009-02-13 23:31:30.123 UTC)
)]
#[case(
    "1234567890123456",
    fd!("[unix_timestamp precision:microsecond]"),
    datetime!(2009-02-13 23:31:30.123456 UTC)
)]
#[case(
    "1234567890123456789",
    fd!("[unix_timestamp precision:nanosecond]"),
    datetime!(2009-02-13 23:31:30.123456789 UTC)
)]
fn parse_unix_timestamp(
    #[case] input: &str,
    #[case] format_description: StaticFormatDescription,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(
        OffsetDateTime::parse(input, format_description).ok(),
        Some(expected)
    );
    assert_eq!(
        OffsetDateTime::parse(input, &OwnedFormatItem::from(format_description)).ok(),
        Some(expected)
    );
}

#[rstest]
#[case("1234567890", fd!("[unix_timestamp sign:mandatory]"))]
#[case("a", fd!("[unix_timestamp precision:second]"))]
#[case("a", fd!("[unix_timestamp precision:millisecond]"))]
#[case("a", fd!("[unix_timestamp precision:microsecond]"))]
#[case("a", fd!("[unix_timestamp precision:nanosecond]"))]
fn parse_unix_timestamp_err(
    #[case] input: &str,
    #[case] format_description: StaticFormatDescription,
) {
    assert!(matches!(
        OffsetDateTime::parse(input, format_description),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent("unix_timestamp")
        ))
    ));
}

#[rstest]
fn issue_601() {
    let date = OffsetDateTime::parse(
        "1234567890.123",
        &fd::parse_borrowed::<1>("[unix_timestamp].[subsecond digits:3]")
            .expect("format description is valid"),
    );

    assert_eq!(date, Ok(datetime!(2009-02-13 23:31:30.123 +00:00:00)));
}

#[rstest]
#[case("00:00", fd!("[hour]:[minute][end]"), time!(0:00))]
#[case("00:00abcdef", fd!("[hour]:[minute][end trailing_input:discard]"), time!(0:00))]
#[case("00:00:00", fd!(version = 2, "[hour]:[minute][optional [[end]]]:[second]"), time!(0:00))]
fn end(
    #[case] input: &str,
    #[case] format_description: StaticFormatDescription,
    #[case] expected: Time,
) {
    assert_eq!(Time::parse(input, format_description).ok(), Some(expected));
    assert_eq!(
        Time::parse(input, &OwnedFormatItem::from(format_description)).ok(),
        Some(expected)
    );
}

#[rstest]
#[case("00:00:00", fd!("[hour]:[minute][end]:[second]"))]
fn end_err_unexpected_trailing(
    #[case] input: &str,
    #[case] format_description: StaticFormatDescription,
) {
    assert!(matches!(
        Time::parse(input, format_description),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::UnexpectedTrailingCharacters { .. }
        ))
    ));
}

#[rstest]
#[case("00:00:00", fd!("[hour]:[minute][end trailing_input:discard]:[second]"))]
fn end_err_invalid_literal(
    #[case] input: &str,
    #[case] format_description: StaticFormatDescription,
) {
    assert!(matches!(
        Time::parse(input, format_description),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidLiteral { .. }
        ))
    ));
}

#[rstest]
#[case(
    fd!("[month]-[day]"),
    b"01-15",
    Parsed::new().with_year(2020).expect("value is valid"),
    date!(2020-01-15)
)]
#[case(
    fd!("[year]-[month]-[day]"),
    b"2020-01-15",
    Parsed::new().with_year(1999).expect("value is valid"),
    date!(2020-01-15)
)]
fn parse_with_defaults_date_success(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] expected: Date,
) {
    assert_eq!(
        Date::parse_with_defaults(input, &format, defaults).ok(),
        Some(expected)
    );
}

#[rstest]
#[case(fd!("[month]"), b"01", Parsed::new())]
fn parse_with_defaults_date_insufficient_information(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
) {
    assert_eq!(
        Date::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case(
    fd!("[month]-[day]"),
    b"Ja-15",
    Parsed::new().with_year(2020).expect("value is valid"),
    "month"
)]
fn parse_with_defaults_date_invalid_component(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] component_name: &str,
) {
    assert!(matches!(
        Date::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case(
    fd!("[month]-[day]"),
    b"02-30",
    Parsed::new().with_year(2020).expect("value is valid"),
    "day"
)]
fn parse_with_defaults_date_component_range(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] component_name: &str,
) {
    assert!(matches!(
        Date::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::ComponentRange(component)
        )) if component.name() == component_name
    ));
}

#[rstest]
#[case(
    fd!("[hour]"),
    b"12",
    Parsed::new().with_minute(30).expect("value is valid"),
    time!(12:30)
)]
#[case(
    fd!("[hour]:[minute]"),
    b"12:30",
    Parsed::new().with_minute(0).expect("value is valid"),
    time!(12:30)
)]
fn parse_with_defaults_time_success(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] expected: Time,
) {
    assert_eq!(
        Time::parse_with_defaults(input, &format, defaults).ok(),
        Some(expected)
    );
}

#[rstest]
#[case(fd!(""), b"", Parsed::new())]
fn parse_with_defaults_time_insufficient_information(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
) {
    assert_eq!(
        Time::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case(fd!("[hour]"), b"a", Parsed::new().with_minute(30).expect("value is valid"), "hour")]
fn parse_with_defaults_time_invalid_component(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] component_name: &str,
) {
    assert!(matches!(
        Time::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case(
    fd!("[offset_hour sign:mandatory]"),
    b"+05",
    Parsed::new().with_offset_minute_signed(30).expect("value is valid"),
    offset!(+5:30),
)]
#[case(
    fd!("[offset_hour sign:mandatory]:[offset_minute]"),
    b"+05:30",
    Parsed::new().with_offset_minute_signed(0).expect("value is valid"),
    offset!(+5:30),
)]
fn parse_with_defaults_utc_offset_success(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] expected: UtcOffset,
) {
    assert_eq!(
        UtcOffset::parse_with_defaults(input, &format, defaults).ok(),
        Some(expected)
    );
}

#[rstest]
#[case(fd!(""), b"", Parsed::new())]
fn parse_with_defaults_utc_offset_insufficient_information(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
) {
    assert_eq!(
        UtcOffset::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case(
    fd!("[offset_hour sign:mandatory]"),
    b"*05",
    Parsed::new().with_offset_minute_signed(30).expect("value is valid"),
    "offset hour",
)]
fn parse_with_defaults_utc_offset_invalid_component(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] component_name: &str,
) {
    assert!(matches!(
        UtcOffset::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case(
    fd!("[year]-[month]-[day]"),
    b"2020-01-02",
    Parsed::new().with_hour_24(12).expect("value is valid"),
    datetime!(2020-01-02 12:00),
)]
#[case(
    fd!("[year]-[month]-[day] [hour]"),
    b"2020-01-02 12",
    Parsed::new().with_hour_24(0).expect("value is valid"),
    datetime!(2020-01-02 12:00),
)]
fn parse_with_defaults_plain_date_time_success(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] expected: PlainDateTime,
) {
    assert_eq!(
        PlainDateTime::parse_with_defaults(input, &format, defaults).ok(),
        Some(expected)
    );
}

#[rstest]
#[case(fd!("[month]-[day]"), b"01-15", Parsed::new())]
fn parse_with_defaults_plain_date_time_insufficient_information(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
) {
    assert_eq!(
        PlainDateTime::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case(
    fd!("[year]-[month]-[day] [hour]"),
    b"2020-01-02 Ja",
    Parsed::new().with_hour_24(12).expect("value is valid"),
    "hour",
)]
fn parse_with_defaults_plain_date_time_invalid_component(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] component_name: &str,
) {
    assert!(matches!(
        PlainDateTime::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case(
    fd!("[year]-[month]-[day]"),
    b"2020-01-02",
    Parsed::new().with_hour_24(12).expect("value is valid"),
    utc_datetime!(2020-01-02 12:00),
)]
#[case(
    fd!("[year]-[month]-[day] [hour]"),
    b"2020-01-02 12",
    Parsed::new().with_hour_24(0).expect("value is valid"),
    utc_datetime!(2020-01-02 12:00),
)]
fn parse_with_defaults_utc_date_time_success(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] expected: UtcDateTime,
) {
    assert_eq!(
        UtcDateTime::parse_with_defaults(input, &format, defaults).ok(),
        Some(expected)
    );
}

#[rstest]
#[case(fd!("[month]-[day]"), b"01-15", Parsed::new())]
fn parse_with_defaults_utc_date_time_insufficient_information(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
) {
    assert_eq!(
        UtcDateTime::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case(
    fd!("[year]-[month]-[day] [hour]"),
    b"2020-01-02 Ja",
    Parsed::new().with_hour_24(12).expect("value is valid"),
    "hour",
)]
fn parse_with_defaults_utc_date_time_invalid_component(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] component_name: &str,
) {
    assert!(matches!(
        UtcDateTime::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case(
    fd!("[year]-[month]-[day] [hour]:[minute]"),
    b"2020-01-02 03:04",
    Parsed::new()
        .with_offset_hour(0)
        .and_then(|p| p.with_offset_minute_signed(0))
        .expect("value is valid"),
    datetime!(2020-01-02 03:04 +0:00),
)]
#[case(
    fd!("[year]-[month]-[day] [hour]:[minute][offset_hour sign:mandatory]:[offset_minute]"),
    b"2020-01-02 03:04+05:30",
    Parsed::new().with_offset_hour(0).expect("value is valid"),
    datetime!(2020-01-02 03:04 +5:30),
)]
fn parse_with_defaults_offset_date_time_success(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] expected: OffsetDateTime,
) {
    assert_eq!(
        OffsetDateTime::parse_with_defaults(input, &format, defaults).ok(),
        Some(expected)
    );
}

#[rstest]
#[case(fd!("[year]-[month]-[day] [hour]:[minute]"), b"2020-01-02 03:04", Parsed::new())]
fn parse_with_defaults_offset_date_time_insufficient_information(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
) {
    assert_eq!(
        OffsetDateTime::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}

#[rstest]
#[case(
    fd!("[year]-[month]-[day] [hour]:[minute]"),
    b"2020-01-a 03:04",
    Parsed::new()
        .with_offset_hour(0)
        .and_then(|p| p.with_offset_minute_signed(0))
        .expect("value is valid"),
    "day",
)]
fn parse_with_defaults_offset_date_time_invalid_component(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] component_name: &str,
) {
    assert!(matches!(
        OffsetDateTime::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::ParseFromDescription(
            error::ParseFromDescription::InvalidComponent(name)
        )) if name == component_name
    ));
}

#[rstest]
#[case(
    fd!("[year]-[month]-[day]"),
    b"2020-01-02",
    Parsed::new().with_hour_24(0).expect("value is valid"),
    timestamp!(1_577_923_200),
)]
#[case(
    fd!("[year]-[month]-[day] [hour]:[minute]"),
    b"2020-01-02 12:30",
    Parsed::new()
        .with_hour_24(0)
        .and_then(|p| p.with_minute(0))
        .expect("value is valid"),
    timestamp!(1_577_968_200),
)]
fn parse_with_defaults_timestamp_success(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
    #[case] expected: Timestamp,
) {
    assert_eq!(
        Timestamp::parse_with_defaults(input, &format, defaults).ok(),
        Some(expected)
    );
}

#[rstest]
#[case(fd!("[month]-[day]"), b"01-15", Parsed::new())]
fn parse_with_defaults_timestamp_insufficient_information(
    #[case] format: StaticFormatDescription,
    #[case] input: &[u8],
    #[case] defaults: Parsed,
) {
    assert_eq!(
        Timestamp::parse_with_defaults(input, &format, defaults),
        Err(error::Parse::TryFromParsed(
            error::TryFromParsed::InsufficientInformation
        ))
    );
}
