use std::fmt::Debug;
use std::num::NonZero;

use rstest::rstest;
use time::format_description::modifier::{Hour12, WeekNumberIso};
use time::format_description::{BorrowedFormatItem, Component};
use time::parsing::Parsed;
use time::{Month, Time, Weekday, error};

#[rstest]
#[case(Parsed::set_year, Parsed::year, 5)]
#[case(Parsed::set_year_last_two, Parsed::year_last_two, 5)]
#[case(Parsed::set_iso_year, Parsed::iso_year, 5)]
#[case(Parsed::set_iso_year_last_two, Parsed::iso_year_last_two, 5)]
#[case(Parsed::set_month, Parsed::month, Month::May)]
#[case(Parsed::set_sunday_week_number, Parsed::sunday_week_number, 5)]
#[case(Parsed::set_monday_week_number, Parsed::monday_week_number, 5)]
#[case(Parsed::set_iso_week_number, Parsed::iso_week_number, const { NonZero::new(5).unwrap() })]
#[case(Parsed::set_weekday, Parsed::weekday, Weekday::Monday)]
#[case(Parsed::set_ordinal, Parsed::ordinal, const { NonZero::new(5).unwrap() })]
#[case(Parsed::set_day, Parsed::day, const { NonZero::new(5).unwrap() })]
#[case(Parsed::set_hour_24, Parsed::hour_24, 5)]
#[case(Parsed::set_hour_12, Parsed::hour_12, const { NonZero::new(5).unwrap() })]
#[case(Parsed::set_hour_12_is_pm, Parsed::hour_12_is_pm, true)]
#[case(Parsed::set_minute, Parsed::minute, 5)]
#[case(Parsed::set_second, Parsed::second, 5)]
#[case(Parsed::set_subsecond, Parsed::subsecond, 5)]
#[case(Parsed::set_offset_hour, Parsed::offset_hour, 5)]
#[expect(deprecated)]
#[case(Parsed::set_offset_minute, Parsed::offset_minute, 5)]
#[case(Parsed::set_offset_minute_signed, Parsed::offset_minute_signed, -5)]
#[expect(deprecated)]
#[case(Parsed::set_offset_second, Parsed::offset_second, 5)]
#[case(Parsed::set_offset_second_signed, Parsed::offset_second_signed, -5)]
fn getters_setters<T>(
    #[case] setter: fn(&mut Parsed, T) -> Option<()>,
    #[case] getter: fn(&Parsed) -> Option<T>,
    #[case] value: T,
) where
    T: PartialEq + Copy + Debug,
{
    let mut parsed = Parsed::new();
    assert!(setter(&mut parsed, value).is_some());
    assert_eq!(getter(&parsed), Some(value));
}

#[rstest]
#[expect(deprecated)]
#[case(Parsed::set_offset_minute, Parsed::offset_minute, 200)]
#[expect(deprecated)]
#[case(Parsed::set_offset_second, Parsed::offset_second, 200)]
fn getters_setters_fail<T>(
    #[case] setter: fn(&mut Parsed, T) -> Option<()>,
    #[case] getter: fn(&Parsed) -> Option<T>,
    #[case] value: T,
) {
    let mut parsed = Parsed::new();
    assert!(setter(&mut parsed, value).is_none());
    assert!(getter(&parsed).is_none());
}

#[rstest]
#[case(Parsed::with_year, Parsed::year, 5)]
#[case(Parsed::with_year_last_two, Parsed::year_last_two, 5)]
#[case(Parsed::with_iso_year, Parsed::iso_year, 5)]
#[case(Parsed::with_iso_year_last_two, Parsed::iso_year_last_two, 5)]
#[case(Parsed::with_month, Parsed::month, Month::May)]
#[case(Parsed::with_sunday_week_number, Parsed::sunday_week_number, 5)]
#[case(Parsed::with_monday_week_number, Parsed::monday_week_number, 5)]
#[case(Parsed::with_iso_week_number, Parsed::iso_week_number, const { NonZero::new(5).unwrap() })]
#[case(Parsed::with_weekday, Parsed::weekday, Weekday::Monday)]
#[case(Parsed::with_ordinal, Parsed::ordinal, const { NonZero::new(5).unwrap() })]
#[case(Parsed::with_day, Parsed::day, const { NonZero::new(5).unwrap() })]
#[case(Parsed::with_hour_24, Parsed::hour_24, 5)]
#[case(Parsed::with_hour_12, Parsed::hour_12, const { NonZero::new(5).unwrap() })]
#[case(Parsed::with_hour_12_is_pm, Parsed::hour_12_is_pm, true)]
#[case(Parsed::with_minute, Parsed::minute, 5)]
#[case(Parsed::with_second, Parsed::second, 5)]
#[case(Parsed::with_subsecond, Parsed::subsecond, 5)]
#[case(Parsed::with_offset_hour, Parsed::offset_hour, 5)]
#[expect(deprecated)]
#[case(Parsed::with_offset_minute, Parsed::offset_minute, 5)]
#[case(Parsed::with_offset_minute_signed, Parsed::offset_minute_signed, -5)]
#[expect(deprecated)]
#[case(Parsed::with_offset_second, Parsed::offset_second, 5)]
#[case(Parsed::with_offset_second_signed, Parsed::offset_second_signed, -5)]
fn builder_methods<T>(
    #[case] builder: fn(Parsed, T) -> Option<Parsed>,
    #[case] getter: fn(&Parsed) -> Option<T>,
    #[case] value: T,
) where
    T: PartialEq + Copy + Debug,
{
    let parsed = builder(Parsed::new(), value).expect("valid value");
    assert_eq!(getter(&parsed), Some(value));
}

#[rstest]
#[expect(deprecated)]
#[case(Parsed::with_offset_minute, Parsed::offset_minute, 200)]
#[expect(deprecated)]
#[case(Parsed::with_offset_second, Parsed::offset_second, 200)]
fn builder_methods_fail<T>(
    #[case] builder: fn(Parsed, T) -> Option<Parsed>,
    #[case] getter: fn(&Parsed) -> Option<T>,
    #[case] value: T,
) {
    assert!(builder(Parsed::new(), value).is_none());
    assert!(getter(&Parsed::new()).is_none());
}

#[rstest]
#[case("a", BorrowedFormatItem::StringLiteral("a"))]
#[case("b", BorrowedFormatItem::StringLiteral("a"))]
fn single_item_parse(#[case] input: &str, #[case] format_item: BorrowedFormatItem) {
    assert!(Time::parse(input, &format_item).is_err());
}

#[rstest]
#[case("day", Component::Day(<_>::default()), b"")]
#[case("month", Component::MonthNumerical(<_>::default()), b"")]
#[case("ordinal", Component::Ordinal(<_>::default()), b"")]
#[case("weekday", Component::WeekdayLong(<_>::default()), b"")]
#[case("week number", Component::WeekNumberIso(<_>::default()), b"")]
#[case("year", Component::CalendarYearFullExtendedRange(<_>::default()), b"")]
#[case("minute", Component::Minute(<_>::default()), b"")]
#[case("period", Component::Period(<_>::default()), b"")]
#[case("second", Component::Second(<_>::default()), b"")]
#[case("subsecond", Component::Subsecond(<_>::default()), b"")]
#[case("offset hour", Component::OffsetHour(<_>::default()), b"")]
#[case("offset minute", Component::OffsetMinute(<_>::default()), b"")]
#[case("offset second", Component::OffsetSecond(<_>::default()), b"")]
#[case("unix_timestamp", Component::UnixTimestampSecond(<_>::default()), b"")]
#[case(
    "week number",
    Component::WeekNumberIso(WeekNumberIso::default()),
    b"00"
)]
#[case("hour", Component::Hour12(Hour12::default()), b"00")]
fn component_err(
    #[case] component_name: &'static str,
    #[case] component: Component,
    #[case] input: &[u8],
) {
    let mut parsed = Parsed::new();
    assert_eq!(
        parsed.parse_component(input, component),
        Err(error::ParseFromDescription::InvalidComponent(
            component_name
        ))
    );
}
