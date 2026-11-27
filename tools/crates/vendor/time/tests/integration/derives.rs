use std::cmp::Ordering;
use std::collections::hash_map::DefaultHasher;
use std::fmt::Debug;
use std::hash::Hash;

use rstest::rstest;
#[expect(deprecated)]
use time::Instant;
use time::error::{self, ConversionRange, IndeterminateOffset, TryFromParsed};
use time::ext::NumericalDuration;
use time::format_description::{
    self, BorrowedFormatItem, Component, OwnedFormatItem, modifier, well_known,
};
use time::macros::{date, datetime, offset, time, utc_datetime};
use time::parsing::Parsed;
use time::{Error, Month, SignedDuration, Time, Weekday};

fn component_range_error() -> error::ComponentRange {
    Time::from_hms(24, 0, 0).expect_err("24 is not a valid hour")
}

fn invalid_format_description() -> error::InvalidFormatDescription {
    format_description::parse_borrowed::<3>("[").expect_err("format description is invalid")
}

#[rstest]
#[expect(deprecated)]
#[case(Instant::now())]
#[case(date!(2021-001))]
#[case(time!(0:00))]
#[case(offset!(UTC))]
#[case(datetime!(2021-001 0:00))]
#[case(datetime!(2021-001 0:00 UTC))]
#[case(utc_datetime!(2021-001 0:00))]
#[case(Weekday::Monday)]
#[case(Month::January)]
#[case(SignedDuration::ZERO)]
#[case(IndeterminateOffset)]
#[case(ConversionRange)]
#[case(invalid_format_description())]
#[case(TryFromParsed::InsufficientInformation)]
#[case(error::Parse::ParseFromDescription(error::ParseFromDescription::InvalidComponent("foo")))]
#[case(error::DifferentVariant)]
#[case(error::InvalidVariant)]
#[case(error::ParseFromDescription::InvalidComponent("foo"))]
#[case(Component::OffsetSecond(modifier::OffsetSecond::default()))]
#[case(well_known::Rfc2822)]
#[case(well_known::Rfc3339)]
#[case(well_known::Iso8601::DEFAULT)]
#[case(well_known::iso8601::FormattedComponents::None)]
#[case(well_known::iso8601::DateKind::Calendar)]
#[case(well_known::iso8601::TimePrecision::Hour { decimal_digits: None })]
#[case(well_known::iso8601::OffsetPrecision::Hour)]
#[case(well_known::iso8601::FormattedComponents::None)]
#[case(component_range_error())]
#[case(BorrowedFormatItem::StringLiteral(""))]
#[case(modifier::Day::default())]
#[case(modifier::MonthNumerical::default())]
#[case(modifier::MonthShort::default())]
#[case(modifier::MonthLong::default())]
#[expect(deprecated)]
#[case(modifier::MonthRepr::default())]
#[expect(deprecated)]
#[case(modifier::Month::default())]
#[case(modifier::Ordinal::default())]
#[expect(deprecated)]
#[case(modifier::WeekdayRepr::default())]
#[case(modifier::WeekdayShort::default())]
#[case(modifier::WeekdayLong::default())]
#[case(modifier::WeekdaySunday::default())]
#[case(modifier::WeekdayMonday::default())]
#[expect(deprecated)]
#[case(modifier::Weekday::default())]
#[expect(deprecated)]
#[case(modifier::WeekNumberRepr::default())]
#[case(modifier::WeekNumberIso::default())]
#[case(modifier::WeekNumberSunday::default())]
#[case(modifier::WeekNumberMonday::default())]
#[expect(deprecated)]
#[case(modifier::WeekNumber::default())]
#[expect(deprecated)]
#[case(modifier::YearRepr::default())]
#[case(modifier::CalendarYearFullExtendedRange::default())]
#[case(modifier::CalendarYearFullStandardRange::default())]
#[case(modifier::IsoYearFullExtendedRange::default())]
#[case(modifier::IsoYearFullStandardRange::default())]
#[case(modifier::CalendarYearCenturyExtendedRange::default())]
#[case(modifier::CalendarYearCenturyStandardRange::default())]
#[case(modifier::IsoYearCenturyExtendedRange::default())]
#[case(modifier::IsoYearCenturyStandardRange::default())]
#[case(modifier::CalendarYearLastTwo::default())]
#[case(modifier::IsoYearLastTwo::default())]
#[expect(deprecated)]
#[case(modifier::Year::default())]
#[case(modifier::Hour12::default())]
#[case(modifier::Hour24::default())]
#[expect(deprecated)]
#[case(modifier::Hour::default())]
#[case(modifier::Minute::default())]
#[case(modifier::Period::default())]
#[case(modifier::Second::default())]
#[case(modifier::SubsecondDigits::default())]
#[case(modifier::Subsecond::default())]
#[case(modifier::OffsetHour::default())]
#[case(modifier::OffsetMinute::default())]
#[case(modifier::OffsetSecond::default())]
#[case(modifier::Padding::default())]
fn clone(#[case] value: impl Clone + PartialEq + Debug) {
    assert_eq!(value.clone(), value);
}

#[rstest]
#[case(Parsed::new())]
fn clone_coverage(#[case] value: impl Clone) {
    #[expect(clippy::redundant_clone, reason = "intended for test coverage")]
    drop(value.clone());
}

#[rstest]
#[case(date!(2021-001))]
#[case(time!(0:00))]
#[case(offset!(UTC))]
#[case(datetime!(2021-001 0:00))]
#[case(datetime!(2021-001 0:00 UTC))]
#[case(utc_datetime!(2021-001 0:00))]
#[case(Weekday::Monday)]
#[case(Month::January)]
#[expect(deprecated)]
#[case(Instant::now())]
#[case(SignedDuration::ZERO)]
#[case(component_range_error())]
fn hash(#[case] value: impl Hash) {
    let mut hasher = DefaultHasher::new();
    value.hash(&mut hasher);
}

#[rstest]
#[case(offset!(UTC), offset!(+1), Ordering::Less)]
#[case(offset!(+1), offset!(UTC), Ordering::Greater)]
#[expect(deprecated)]
#[case(Instant::now() - 1.seconds(), Instant::now(), Ordering::Less)]
#[expect(deprecated)]
#[case(Instant::now() + 1.seconds(), Instant::now(), Ordering::Greater)]
fn partial_ord<T>(#[case] a: T, #[case] b: T, #[case] ordering: Ordering)
where
    T: PartialOrd,
{
    assert_eq!(a.partial_cmp(&b), Some(ordering));
}

#[rstest]
#[case(offset!(UTC), offset!(+1), Ordering::Less)]
#[case(offset!(+1), offset!(UTC), Ordering::Greater)]
#[case(offset!(UTC), offset!(UTC), Ordering::Equal)]
fn ord(#[case] a: time::UtcOffset, #[case] b: time::UtcOffset, #[case] ordering: Ordering) {
    assert_eq!(a.cmp(&b), ordering);
}

#[rstest]
#[case(utc_datetime!(2021-001 0:00))]
#[case(SignedDuration::ZERO)]
#[case(IndeterminateOffset)]
#[case(ConversionRange)]
#[case(TryFromParsed::InsufficientInformation)]
#[case(Parsed::new())]
#[expect(deprecated)]
#[case(Instant::now())]
#[case(error::ParseFromDescription::InvalidComponent("foo"))]
#[case(error::Format::InvalidComponent("foo"))]
#[case(well_known::Rfc2822)]
#[case(well_known::Rfc3339)]
#[case(well_known::Iso8601::DEFAULT)]
#[case(well_known::iso8601::FormattedComponents::None)]
#[case(well_known::iso8601::DateKind::Calendar)]
#[case(well_known::iso8601::TimePrecision::Hour { decimal_digits: None })]
#[case(well_known::iso8601::OffsetPrecision::Hour)]
#[case(well_known::iso8601::Config::DEFAULT)]
#[case(component_range_error())]
#[case(Error::ConversionRange(ConversionRange))]
#[case(modifier::Day::default())]
#[expect(deprecated)]
#[case(modifier::MonthRepr::default())]
#[case(modifier::MonthNumerical::default())]
#[case(modifier::MonthShort::default())]
#[case(modifier::MonthLong::default())]
#[expect(deprecated)]
#[case(modifier::Month::default())]
#[case(modifier::Ordinal::default())]
#[expect(deprecated)]
#[case(modifier::WeekdayRepr::default())]
#[case(modifier::WeekdayShort::default())]
#[case(modifier::WeekdayLong::default())]
#[case(modifier::WeekdaySunday::default())]
#[case(modifier::WeekdayMonday::default())]
#[expect(deprecated)]
#[case(modifier::Weekday::default())]
#[expect(deprecated)]
#[case(modifier::WeekNumberRepr::default())]
#[case(modifier::WeekNumberIso::default())]
#[case(modifier::WeekNumberSunday::default())]
#[case(modifier::WeekNumberMonday::default())]
#[expect(deprecated)]
#[case(modifier::WeekNumber::default())]
#[expect(deprecated)]
#[case(modifier::YearRepr::default())]
#[case(modifier::CalendarYearFullExtendedRange::default())]
#[case(modifier::CalendarYearFullStandardRange::default())]
#[case(modifier::IsoYearFullExtendedRange::default())]
#[case(modifier::IsoYearFullStandardRange::default())]
#[case(modifier::CalendarYearCenturyExtendedRange::default())]
#[case(modifier::CalendarYearCenturyStandardRange::default())]
#[case(modifier::IsoYearCenturyExtendedRange::default())]
#[case(modifier::IsoYearCenturyStandardRange::default())]
#[case(modifier::CalendarYearLastTwo::default())]
#[case(modifier::IsoYearLastTwo::default())]
#[expect(deprecated)]
#[case(modifier::Year::default())]
#[case(modifier::Hour12::default())]
#[case(modifier::Hour24::default())]
#[expect(deprecated)]
#[case(modifier::Hour::default())]
#[case(modifier::Minute::default())]
#[case(modifier::Period::default())]
#[case(modifier::Second::default())]
#[case(modifier::SubsecondDigits::default())]
#[case(modifier::Subsecond::default())]
#[case(modifier::OffsetHour::default())]
#[case(modifier::OffsetMinute::default())]
#[case(modifier::OffsetSecond::default())]
#[case(modifier::Padding::default())]
#[expect(deprecated)]
#[case(BorrowedFormatItem::Literal(b"abcdef"))]
#[case(BorrowedFormatItem::StringLiteral("abcdef"))]
#[case(BorrowedFormatItem::Compound(
    const { &[BorrowedFormatItem::Component(Component::Day(modifier::Day::default()))] }
))]
#[case(BorrowedFormatItem::Optional(&BorrowedFormatItem::Compound(&[])))]
#[case(BorrowedFormatItem::First(&[]))]
#[expect(deprecated)]
#[case(OwnedFormatItem::from(BorrowedFormatItem::Literal(b"abcdef")))]
#[case(OwnedFormatItem::from(BorrowedFormatItem::StringLiteral("abcdef")))]
#[case(OwnedFormatItem::from(BorrowedFormatItem::Compound(
    &[BorrowedFormatItem::Component(Component::Day(modifier::Day::default()))]
)))]
#[case(OwnedFormatItem::from(BorrowedFormatItem::Optional(&BorrowedFormatItem::Compound(&[]))))]
#[case(OwnedFormatItem::from(BorrowedFormatItem::First(&[])))]
fn debug(#[case] value: impl Debug) {
    let _unused = format!("{value:?}");
}
