use core::num::NonZero;

use rstest::rstest;
use rstest_reuse::{apply, template};
use time::error::InvalidFormatDescription;
use time::format_description::modifier::*;
use time::format_description::{
    self, BorrowedFormatItem, Component, OwnedFormatItem, StaticFormatDescription,
};
use time::macros::format_description;

macro_rules! boxed {
    ($s:literal) => {
        $s.to_owned().into_boxed_str()
    };
}

/// A modifier with its value and string representation.
///
/// This alias is used to avoid repeating the tuple in countless locations.
type M<T> = (T, &'static str);

#[rustfmt::skip] // does not format well
#[template]
#[rstest]
fn modifiers(
    #[values(
        (Padding::Space, "padding:space"),
        (Padding::Zero, "padding:zero"),
        (Padding::None, "padding:none"),
    )]
    padding: _,
    #[values(
        (true, "case:upper"),
        (false, "case:lower"),
    )]
    period_is_uppercase: _,
    #[values(
        (SubsecondDigits::One, "digits:1"),
        (SubsecondDigits::Two, "digits:2"),
        (SubsecondDigits::Three, "digits:3"),
        (SubsecondDigits::Four, "digits:4"),
        (SubsecondDigits::Five, "digits:5"),
        (SubsecondDigits::Six, "digits:6"),
        (SubsecondDigits::Seven, "digits:7"),
        (SubsecondDigits::Eight, "digits:8"),
        (SubsecondDigits::Nine, "digits:9"),
        (SubsecondDigits::OneOrMore, "digits:1+"),
    )]
    subsecond_digits: _,
    #[values(
        (false, "sign:automatic"),
        (true, "sign:mandatory"),
    )]
    sign_is_mandatory: _,
    #[values(
        (true, "one_indexed:true"),
        (false, "one_indexed:false"),
    )]
    weekday_is_one_indexed: _,
    #[values(
        (true, "case_sensitive:true"),
        (false, "case_sensitive:false"),
    )]
    case_sensitive: _,
    #[values(
        (NonZero::new(1).unwrap(), "count:1"),
        (NonZero::new(2).unwrap(), "count:2"),
        (NonZero::new(3).unwrap(), "count:3"),
        (NonZero::new(10).unwrap(), "count:10"),
        (NonZero::new(100).unwrap(), "count:100"),
        (NonZero::new(1_000).unwrap(), "count:1000"),
    )]
    ignore_count: _,
    #[values(
        (TrailingInput::Prohibit, "trailing_input:prohibit"),
        (TrailingInput::Discard, "trailing_input:discard"),
    )]
    trailing_input: _,
) {}

#[rstest]
fn empty() {
    assert_eq!(format_description::parse_borrowed::<2>(""), Ok(vec![]));
    assert_eq!(
        format_description::parse_owned::<2>(""),
        Ok(OwnedFormatItem::Compound(Box::new([])))
    );
}

#[rstest]
#[case("foo bar", ["foo bar"])]
#[case("  leading spaces", ["  leading spaces"])]
#[case("trailing spaces  ", ["trailing spaces  "])]
#[case("     ", ["     "])]
#[case("[[", ["["])]
#[case("foo[[bar", ["foo", "[", "bar"])]
fn only_literal<const N: usize>(#[case] format_description: &str, #[case] expected: [&str; N]) {
    assert_eq!(
        format_description::parse_borrowed::<1>(format_description),
        Ok(expected
            .into_iter()
            .map(BorrowedFormatItem::StringLiteral)
            .collect())
    );
}

#[rstest]
#[case("[day]", Component::Day(Day::default()))]
#[case("[end]", Component::End(End::default()))]
#[case("[hour]", Component::Hour24(Hour24::default()))]
#[case("[minute]", Component::Minute(Minute::default()))]
#[case("[month]", Component::MonthNumerical(MonthNumerical::default()))]
#[case("[offset_hour]", Component::OffsetHour(OffsetHour::default()))]
#[case("[offset_minute]", Component::OffsetMinute(OffsetMinute::default()))]
#[case("[offset_second]", Component::OffsetSecond(OffsetSecond::default()))]
#[case("[ordinal]", Component::Ordinal(Ordinal::default()))]
#[case("[period]", Component::Period(Period::default()))]
#[case("[second]", Component::Second(Second::default()))]
#[case("[subsecond]", Component::Subsecond(Subsecond::default()))]
#[case(
    "[unix_timestamp]",
    Component::UnixTimestampSecond(UnixTimestampSecond::default())
)]
#[case("[weekday]", Component::WeekdayLong(WeekdayLong::default()))]
#[case("[week_number]", Component::WeekNumberIso(WeekNumberIso::default()))]
#[case(
    "[year]",
    Component::CalendarYearFullExtendedRange(CalendarYearFullExtendedRange::default())
)]
fn simple_component(#[case] format_description: &str, #[case] component: Component) {
    assert_eq!(
        format_description::parse_borrowed::<2>(format_description),
        Ok(vec![BorrowedFormatItem::Component(component)])
    );
}

#[rstest]
fn errors() {
    use InvalidFormatDescription::*;

    macro_rules! assert_errs {
        ($($format_description:literal, $error:pat $(if $condition:expr)?,)*) => {$(
            assert!(matches!(
                format_description::parse_borrowed::<3>($format_description),
                Err($error) $(if $condition)?
            ));
            assert!(matches!(
                format_description::parse_owned::<3>($format_description),
                Err($error) $(if $condition)?
            ));
        )*};
    }

    assert_errs! {
        "[ invalid ]", InvalidComponentName { name, index: 2, .. } if name == "invalid",
        "[", MissingComponentName { index: 0, .. },
        "[ ", MissingComponentName { index: 1, .. },
        "[]", MissingComponentName { index: 0, .. },
        "[day sign:mandatory]", InvalidModifier { value, index: 5, .. } if value == "sign",
        "[day sign:]", InvalidModifier { value, index: 9,.. } if value.is_empty(),
        "[day :mandatory]", InvalidModifier { value, index: 5,.. } if value.is_empty(),
        "[day sign:mandatory", UnclosedOpeningBracket { index: 0, .. },
        "[day padding:invalid]", InvalidModifier { value, index: 13, .. } if value == "invalid",
        "[ignore]", MissingRequiredModifier { name: "count", index: 1, .. },
        "[ignore count:70000]", InvalidModifier { value, index: 14, .. } if value == "70000",
    }
}

macro_rules! placeholder {
    ($($x:tt)*) => {
        " {}"
    };
}

macro_rules! parse_with_modifiers {
    ($modifier_name:literal, $($modifier:ident),+) => {
        format_description::parse_borrowed::<2>(
            &format!(
                concat!(
                    "[",
                    $modifier_name,
                    $(placeholder!($modifier),)+
                    "]",
                ),
                $($modifier.1),+
            )
        )
    };
}

#[apply(modifiers)]
fn day_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("day", padding),
        Ok(vec![BorrowedFormatItem::Component(Component::Day(
            Day::default().with_padding(padding.0)
        ))])
    );
}

#[apply(modifiers)]
fn end_component(trailing_input: M<TrailingInput>) {
    assert_eq!(
        parse_with_modifiers!("end", trailing_input),
        Ok(vec![BorrowedFormatItem::Component(Component::End(
            End::default().with_trailing_input(trailing_input.0)
        ))])
    );
}

#[apply(modifiers)]
fn minute_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("minute", padding),
        Ok(vec![BorrowedFormatItem::Component(Component::Minute(
            Minute::default().with_padding(padding.0)
        ))])
    );
}

#[apply(modifiers)]
fn offset_minute_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("offset_minute", padding),
        Ok(vec![BorrowedFormatItem::Component(
            Component::OffsetMinute(OffsetMinute::default().with_padding(padding.0))
        )])
    );
}

#[apply(modifiers)]
fn offset_second_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("offset_second", padding),
        Ok(vec![BorrowedFormatItem::Component(
            Component::OffsetSecond(OffsetSecond::default().with_padding(padding.0))
        )])
    );
}

#[apply(modifiers)]
fn ordinal_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("ordinal", padding),
        Ok(vec![BorrowedFormatItem::Component(Component::Ordinal(
            Ordinal::default().with_padding(padding.0)
        ))])
    );
}

#[apply(modifiers)]
fn second_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("second", padding),
        Ok(vec![BorrowedFormatItem::Component(Component::Second(
            Second::default().with_padding(padding.0)
        ))])
    );
}

#[apply(modifiers)]
fn hour_12_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("hour repr:12", padding),
        Ok(vec![BorrowedFormatItem::Component(Component::Hour12(
            Hour12::default().with_padding(padding.0)
        ))])
    );
}

#[apply(modifiers)]
fn hour_24_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("hour repr:24", padding),
        Ok(vec![BorrowedFormatItem::Component(Component::Hour24(
            Hour24::default().with_padding(padding.0)
        ))])
    );
}

#[apply(modifiers)]
fn month_numerical_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("month repr:numerical", padding),
        Ok(vec![BorrowedFormatItem::Component(
            Component::MonthNumerical(MonthNumerical::default().with_padding(padding.0))
        )])
    );
}

#[apply(modifiers)]
fn month_short_component(case_sensitive: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("month repr:short", case_sensitive),
        Ok(vec![BorrowedFormatItem::Component(Component::MonthShort(
            MonthShort::default().with_case_sensitive(case_sensitive.0)
        ))])
    );
}

#[apply(modifiers)]
fn month_long_component(case_sensitive: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("month repr:long", case_sensitive),
        Ok(vec![BorrowedFormatItem::Component(Component::MonthLong(
            MonthLong::default().with_case_sensitive(case_sensitive.0)
        ))])
    );
}

#[apply(modifiers)]
fn period_component(case_sensitive: M<bool>, period_is_uppercase: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("period", period_is_uppercase, case_sensitive),
        Ok(vec![BorrowedFormatItem::Component(Component::Period(
            Period::default()
                .with_is_uppercase(period_is_uppercase.0)
                .with_case_sensitive(case_sensitive.0)
        ))])
    );
}

#[apply(modifiers)]
fn weekday_short_component(case_sensitive: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("weekday repr:short", case_sensitive),
        Ok(vec![BorrowedFormatItem::Component(
            Component::WeekdayShort(WeekdayShort::default().with_case_sensitive(case_sensitive.0))
        )])
    );
}

#[apply(modifiers)]
fn weekday_long_component(case_sensitive: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("weekday repr:long", case_sensitive),
        Ok(vec![BorrowedFormatItem::Component(Component::WeekdayLong(
            WeekdayLong::default().with_case_sensitive(case_sensitive.0)
        ))])
    );
}

#[apply(modifiers)]
fn weekday_sunday_component(weekday_is_one_indexed: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("weekday repr:sunday", weekday_is_one_indexed),
        Ok(vec![BorrowedFormatItem::Component(
            Component::WeekdaySunday(
                WeekdaySunday::default().with_one_indexed(weekday_is_one_indexed.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn weekday_monday_component(weekday_is_one_indexed: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("weekday repr:monday", weekday_is_one_indexed),
        Ok(vec![BorrowedFormatItem::Component(
            Component::WeekdayMonday(
                WeekdayMonday::default().with_one_indexed(weekday_is_one_indexed.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn week_number_iso_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("week_number repr:iso", padding),
        Ok(vec![BorrowedFormatItem::Component(
            Component::WeekNumberIso(WeekNumberIso::default().with_padding(padding.0))
        )])
    );
}

#[apply(modifiers)]
fn week_number_sunday_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("week_number repr:sunday", padding),
        Ok(vec![BorrowedFormatItem::Component(
            Component::WeekNumberSunday(WeekNumberSunday::default().with_padding(padding.0))
        )])
    );
}

#[apply(modifiers)]
fn week_number_monday_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("week_number repr:monday", padding),
        Ok(vec![BorrowedFormatItem::Component(
            Component::WeekNumberMonday(WeekNumberMonday::default().with_padding(padding.0))
        )])
    );
}

#[apply(modifiers)]
fn offset_hour_component(padding: M<Padding>, sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("offset_hour", padding, sign_is_mandatory),
        Ok(vec![BorrowedFormatItem::Component(Component::OffsetHour(
            OffsetHour::default()
                .with_padding(padding.0)
                .with_sign_is_mandatory(sign_is_mandatory.0)
        ))])
    );
}

#[apply(modifiers)]
fn calendar_year_full_extended_range_component(padding: M<Padding>, sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!(
            "year base:calendar repr:full range:extended",
            padding,
            sign_is_mandatory
        ),
        Ok(vec![BorrowedFormatItem::Component(
            Component::CalendarYearFullExtendedRange(
                CalendarYearFullExtendedRange::default()
                    .with_padding(padding.0)
                    .with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn calendar_year_full_standard_range_component(padding: M<Padding>, sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!(
            "year base:calendar repr:full range:standard",
            padding,
            sign_is_mandatory
        ),
        Ok(vec![BorrowedFormatItem::Component(
            Component::CalendarYearFullStandardRange(
                CalendarYearFullStandardRange::default()
                    .with_padding(padding.0)
                    .with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn iso_year_full_extended_range_component(padding: M<Padding>, sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!(
            "year base:iso_week repr:full range:extended",
            padding,
            sign_is_mandatory
        ),
        Ok(vec![BorrowedFormatItem::Component(
            Component::IsoYearFullExtendedRange(
                IsoYearFullExtendedRange::default()
                    .with_padding(padding.0)
                    .with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn iso_year_full_standard_range_component(padding: M<Padding>, sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!(
            "year base:iso_week repr:full range:standard",
            padding,
            sign_is_mandatory
        ),
        Ok(vec![BorrowedFormatItem::Component(
            Component::IsoYearFullStandardRange(
                IsoYearFullStandardRange::default()
                    .with_padding(padding.0)
                    .with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn calendar_year_century_extended_range_component(padding: M<Padding>, sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!(
            "year base:calendar repr:century range:extended",
            padding,
            sign_is_mandatory
        ),
        Ok(vec![BorrowedFormatItem::Component(
            Component::CalendarYearCenturyExtendedRange(
                CalendarYearCenturyExtendedRange::default()
                    .with_padding(padding.0)
                    .with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn calendar_year_century_standard_range_component(padding: M<Padding>, sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!(
            "year base:calendar repr:century range:standard",
            padding,
            sign_is_mandatory
        ),
        Ok(vec![BorrowedFormatItem::Component(
            Component::CalendarYearCenturyStandardRange(
                CalendarYearCenturyStandardRange::default()
                    .with_padding(padding.0)
                    .with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn iso_year_century_extended_range_component(padding: M<Padding>, sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!(
            "year base:iso_week repr:century range:extended",
            padding,
            sign_is_mandatory
        ),
        Ok(vec![BorrowedFormatItem::Component(
            Component::IsoYearCenturyExtendedRange(
                IsoYearCenturyExtendedRange::default()
                    .with_padding(padding.0)
                    .with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn iso_year_century_standard_range_component(padding: M<Padding>, sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!(
            "year base:iso_week repr:century range:standard",
            padding,
            sign_is_mandatory
        ),
        Ok(vec![BorrowedFormatItem::Component(
            Component::IsoYearCenturyStandardRange(
                IsoYearCenturyStandardRange::default()
                    .with_padding(padding.0)
                    .with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn calendar_year_last_two_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("year base:calendar repr:last_two", padding),
        Ok(vec![BorrowedFormatItem::Component(
            Component::CalendarYearLastTwo(CalendarYearLastTwo::default().with_padding(padding.0))
        )])
    );
}

#[apply(modifiers)]
fn iso_year_last_two_component(padding: M<Padding>) {
    assert_eq!(
        parse_with_modifiers!("year base:iso_week repr:last_two", padding),
        Ok(vec![BorrowedFormatItem::Component(
            Component::IsoYearLastTwo(IsoYearLastTwo::default().with_padding(padding.0))
        )])
    );
}

#[apply(modifiers)]
fn unix_timestamp_second_component(sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("unix_timestamp precision:second", sign_is_mandatory),
        Ok(vec![BorrowedFormatItem::Component(
            Component::UnixTimestampSecond(
                UnixTimestampSecond::default().with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn unix_timestamp_millisecond_component(sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("unix_timestamp precision:millisecond", sign_is_mandatory),
        Ok(vec![BorrowedFormatItem::Component(
            Component::UnixTimestampMillisecond(
                UnixTimestampMillisecond::default().with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn unix_timestamp_microsecond_component(sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("unix_timestamp precision:microsecond", sign_is_mandatory),
        Ok(vec![BorrowedFormatItem::Component(
            Component::UnixTimestampMicrosecond(
                UnixTimestampMicrosecond::default().with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn unix_timestamp_nanosecond_component(sign_is_mandatory: M<bool>) {
    assert_eq!(
        parse_with_modifiers!("unix_timestamp precision:nanosecond", sign_is_mandatory),
        Ok(vec![BorrowedFormatItem::Component(
            Component::UnixTimestampNanosecond(
                UnixTimestampNanosecond::default().with_sign_is_mandatory(sign_is_mandatory.0)
            )
        )])
    );
}

#[apply(modifiers)]
fn subsecond_component(subsecond_digits: M<SubsecondDigits>) {
    assert_eq!(
        parse_with_modifiers!("subsecond", subsecond_digits),
        Ok(vec![BorrowedFormatItem::Component(Component::Subsecond(
            Subsecond::default().with_digits(subsecond_digits.0)
        ))]),
    );
}

#[apply(modifiers)]
fn ignore_component(ignore_count: M<NonZero<u16>>) {
    assert_eq!(
        parse_with_modifiers!("ignore", ignore_count),
        Ok(vec![BorrowedFormatItem::Component(Component::Ignore(
            Ignore::count(ignore_count.0)
        ))])
    );
}

#[rstest]
#[case(
    "[optional [:[year]]]",
    OwnedFormatItem::Optional(Box::new(
    OwnedFormatItem::Compound(Box::new([
        OwnedFormatItem::StringLiteral(boxed!(":")),
        OwnedFormatItem::Component(Component::CalendarYearFullExtendedRange(
            Default::default()
        ))
    ]))
)))]
#[case(
    "[optional [[year]]]",
    OwnedFormatItem::Optional(Box::new(OwnedFormatItem::Component(
        Component::CalendarYearFullExtendedRange(Default::default())
    )))
)]
#[case(
    r"[optional [\[]]",
    OwnedFormatItem::Optional(Box::new(
    OwnedFormatItem::StringLiteral(boxed!("["))
)))]
#[case(
    r"[optional [ \[ ]]",
    OwnedFormatItem::Optional(Box::new(
        OwnedFormatItem::Compound(Box::new([
            OwnedFormatItem::StringLiteral(boxed!(" ")),
            OwnedFormatItem::StringLiteral(boxed!("[")),
            OwnedFormatItem::StringLiteral(boxed!(" ")),
        ]))
    ))
)]
fn optional(#[case] format_description: &str, #[case] expected: OwnedFormatItem) {
    assert_eq!(
        format_description::parse_owned::<2>(format_description),
        Ok(expected)
    );
}

#[rstest]
#[case(
    "[first [a]]",
    OwnedFormatItem::First(Box::new([
        OwnedFormatItem::StringLiteral(boxed!("a"))
    ]))
)]
#[case(
    "[first [a] [b]]",
    OwnedFormatItem::First(Box::new([
        OwnedFormatItem::StringLiteral(boxed!("a")),
        OwnedFormatItem::StringLiteral(boxed!("b")),
    ]))
)]
#[case(
    "[first [a] [b] ]",
    OwnedFormatItem::First(Box::new([
        OwnedFormatItem::StringLiteral(boxed!("a")),
        OwnedFormatItem::StringLiteral(boxed!("b")),
    ]))
)]
#[case(
    r"[first [a][\[]]",
    OwnedFormatItem::First(Box::new([
        OwnedFormatItem::StringLiteral(boxed!("a")),
        OwnedFormatItem::StringLiteral(boxed!("[")),
    ]))
)]
#[case(
    r"[first [a][\[\[]]",
    OwnedFormatItem::First(Box::new([
        OwnedFormatItem::StringLiteral(boxed!("a")),
        OwnedFormatItem::Compound(Box::new([
            OwnedFormatItem::StringLiteral(boxed!("[")),
            OwnedFormatItem::StringLiteral(boxed!("[")),
        ]))
    ]))
)]
#[case(
    "[first [[period case:upper]] [[period case:lower]] ]",
    OwnedFormatItem::First(Box::new([
        OwnedFormatItem::Component(Component::Period(
            Period::default()
                .with_is_uppercase(true)
                .with_case_sensitive(true)
        )),
        OwnedFormatItem::Component(Component::Period(
            Period::default()
                .with_is_uppercase(false)
                .with_case_sensitive(true)
        )),
    ]))
)]
fn first(#[case] format_description: &str, #[case] expected: OwnedFormatItem) {
    assert_eq!(
        format_description::parse_owned::<2>(format_description),
        Ok(expected)
    );
}

#[rstest]
#[case(
    "[first [[period case:upper]] [[period case:lower]] ]",
    OwnedFormatItem::First(Box::new([
        OwnedFormatItem::Component(Component::Period(
            Period::default()
                .with_is_uppercase(true)
                .with_case_sensitive(true)
        )),
        OwnedFormatItem::Component(Component::Period(
            Period::default()
                .with_is_uppercase(false)
                .with_case_sensitive(true)
        )),
    ]))
)]
fn first_v1(#[case] format_description: &str, #[case] expected: OwnedFormatItem) {
    assert_eq!(
        format_description::parse_owned::<1>(format_description),
        Ok(expected)
    );
}

#[rstest]
fn backslash_escape() {
    assert_eq!(
        format_description::parse_owned::<2>(r"[optional [\]]]"),
        Ok(OwnedFormatItem::Optional(Box::new(
            OwnedFormatItem::StringLiteral(boxed!("]"))
        )))
    );
    assert_eq!(
        format_description::parse_owned::<2>(r"[optional [\[]]"),
        Ok(OwnedFormatItem::Optional(Box::new(
            OwnedFormatItem::StringLiteral(boxed!("["))
        )))
    );
    assert_eq!(
        format_description::parse_owned::<2>(r"[optional [\\]]"),
        Ok(OwnedFormatItem::Optional(Box::new(
            OwnedFormatItem::StringLiteral(boxed!(r"\"))
        )))
    );
    assert_eq!(
        format_description::parse_owned::<2>(r"\\"),
        Ok(OwnedFormatItem::StringLiteral(boxed!(r"\")))
    );
    assert_eq!(
        format_description::parse_owned::<2>(r"\["),
        Ok(OwnedFormatItem::StringLiteral(boxed!(r"[")))
    );
    assert_eq!(
        format_description::parse_owned::<2>(r"\]"),
        Ok(OwnedFormatItem::StringLiteral(boxed!(r"]")))
    );
    assert_eq!(
        format_description::parse_owned::<2>(r"foo\\"),
        Ok(OwnedFormatItem::Compound(Box::new([
            OwnedFormatItem::StringLiteral(boxed!("foo")),
            OwnedFormatItem::StringLiteral(boxed!(r"\")),
        ])))
    );
    assert_eq!(
        format_description::parse_borrowed::<2>(r"\\"),
        Ok(vec![BorrowedFormatItem::StringLiteral(r"\")])
    );
    assert_eq!(
        format_description::parse_borrowed::<2>(r"\["),
        Ok(vec![BorrowedFormatItem::StringLiteral(r"[")])
    );
    assert_eq!(
        format_description::parse_borrowed::<2>(r"\]"),
        Ok(vec![BorrowedFormatItem::StringLiteral(r"]")])
    );
    assert_eq!(
        format_description::parse_borrowed::<2>(r"foo\\"),
        Ok(vec![
            BorrowedFormatItem::StringLiteral("foo"),
            BorrowedFormatItem::StringLiteral(r"\"),
        ])
    );
}

#[rstest]
#[case(r"\a", 1)]
#[case(r"\", 0)]
fn backslash_escape_error(#[case] format_description: &str, #[case] expected_index: usize) {
    assert!(matches!(
        format_description::parse_owned::<2>(format_description),
        Err(InvalidFormatDescription::Expected {
            what: "valid escape sequence",
            index,
            ..
        }) if index == expected_index
    ));
    assert!(matches!(
        format_description::parse_borrowed::<2>(format_description),
        Err(InvalidFormatDescription::Expected {
            what: "valid escape sequence",
            index,
            ..
        }) if index == expected_index
    ));
}

#[rstest]
#[case("[]", 0)]
#[case("[   ]", 1)]
fn nested_v2_error_missing_component_name(
    #[case] format_description: &str,
    #[case] expected_index: usize,
) {
    assert!(matches!(
        format_description::parse_owned::<2>(format_description),
        Err(InvalidFormatDescription::MissingComponentName { index, .. }) if index == expected_index
    ));
}

#[rstest]
#[case("[first [a][[[]]", 0)]
#[case("[optional []", 0)]
#[case("[first []", 0)]
#[case("[optional [", 0)]
#[case("[optional [[year", 0)]
// A component header immediately followed by an unclosed nested bracket (no whitespace, so the
// nested attempt is swallowed and the lexer continues with the outer component). These previously
// tripped `debug_assert!(self.context().is_component())` instead of reporting the unclosed bracket.
#[case("[a[", 0)]
#[case("[hour[", 0)]
fn nested_v2_error_unclosed(#[case] format_description: &str, #[case] expected_index: usize) {
    assert!(matches!(
        format_description::parse_owned::<2>(format_description),
        Err(InvalidFormatDescription::UnclosedOpeningBracket { index, .. })
            if index == expected_index
    ));
}

#[rstest]
#[case(
    "[optional []]",
    "optional item",
    "runtime-parsed format descriptions",
    0
)]
#[case("[first []]", "'first' item", "runtime-parsed format descriptions", 0)]
fn nested_v1_error_not_supported(
    #[case] format_description: &str,
    #[case] expected_what: &str,
    #[case] expected_context: &str,
    #[case] expected_index: usize,
) {
    assert!(matches!(
        format_description::parse_borrowed::<1>(format_description),
        Err(InvalidFormatDescription::NotSupported { what, context, index, .. })
            if what == expected_what
                && context == expected_context
                && index == expected_index
    ));
}

#[rstest]
#[case("[year foo]", "foo", 6)]
fn nested_v2_error_invalid_modifier(
    #[case] format_description: &str,
    #[case] expected_value: &str,
    #[case] expected_index: usize,
) {
    assert!(matches!(
        format_description::parse_owned::<2>(format_description),
        Err(InvalidFormatDescription::InvalidModifier { value, index, .. })
            if value == expected_value && index == expected_index
    ));
}

#[rstest]
#[case("[optional[]]", "whitespace before nested format description", 9)]
#[case("[first[]]", "whitespace before nested format description", 6)]
#[case("[optional]", "nested format description", 9)]
fn nested_v2_error_expected(
    #[case] format_description: &str,
    #[case] expected_what: &str,
    #[case] expected_index: usize,
) {
    assert!(matches!(
        format_description::parse_owned::<2>(format_description),
        Err(InvalidFormatDescription::Expected { what, index, .. })
            if what == expected_what && index == expected_index
    ));
}

#[rstest]
#[case("[", "missing component name at byte index 0")]
#[case("[foo", "unclosed opening bracket at byte index 0")]
#[case("[foo]", "invalid component name `foo` at byte index 1")]
#[case("[day bar]", "invalid modifier `bar` at byte index 5")]
#[case("[]", "missing component name at byte index 0")]
#[case(
    "[optional []]",
    "optional item is not supported in runtime-parsed format descriptions at byte index 0"
)]
#[case(
    "[ignore]",
    "missing required modifier `count` for component at byte index 1"
)]
fn error_display(#[case] format_description: &str, #[case] error: &str) {
    // la10736/rstest#217
    #[expect(clippy::unwrap_used, reason = "purpose of the test")]
    let test = || {
        assert_eq!(
            format_description::parse_borrowed::<1>(format_description)
                .unwrap_err()
                .to_string(),
            error
        );
    };

    test();
}

#[rstest]
#[case("[optional ", "unclosed opening bracket at byte index 0")]
fn error_display_owned(#[case] format_description: &str, #[case] error: &str) {
    // la10736/rstest#217
    #[expect(clippy::unwrap_used, reason = "purpose of the test")]
    let test = || {
        assert_eq!(
            format_description::parse_owned::<2>(format_description)
                .unwrap_err()
                .to_string(),
            error
        )
    };

    test();
}

#[rstest]
fn rfc_3339() {
    assert_eq!(
        format_description::parse_borrowed::<2>(
            "[year]-[month repr:numerical]-[day]T[hour]:[minute]:[second].[subsecond][offset_hour \
             sign:mandatory]:[offset_minute]"
        ),
        Ok(vec![
            BorrowedFormatItem::Component(Component::CalendarYearFullExtendedRange(
                CalendarYearFullExtendedRange::default()
                    .with_padding(Padding::Zero)
                    .with_sign_is_mandatory(false)
            )),
            BorrowedFormatItem::StringLiteral("-"),
            BorrowedFormatItem::Component(Component::MonthNumerical(
                MonthNumerical::default().with_padding(Padding::Zero)
            )),
            BorrowedFormatItem::StringLiteral("-"),
            BorrowedFormatItem::Component(Component::Day(
                Day::default().with_padding(Padding::Zero)
            )),
            BorrowedFormatItem::StringLiteral("T"),
            BorrowedFormatItem::Component(Component::Hour24(
                Hour24::default().with_padding(Padding::Zero)
            )),
            BorrowedFormatItem::StringLiteral(":"),
            BorrowedFormatItem::Component(Component::Minute(
                Minute::default().with_padding(Padding::Zero)
            )),
            BorrowedFormatItem::StringLiteral(":"),
            BorrowedFormatItem::Component(Component::Second(
                Second::default().with_padding(Padding::Zero)
            )),
            BorrowedFormatItem::StringLiteral("."),
            BorrowedFormatItem::Component(Component::Subsecond(
                Subsecond::default().with_digits(SubsecondDigits::OneOrMore)
            )),
            BorrowedFormatItem::Component(Component::OffsetHour(
                OffsetHour::default()
                    .with_padding(Padding::Zero)
                    .with_sign_is_mandatory(true)
            )),
            BorrowedFormatItem::StringLiteral(":"),
            BorrowedFormatItem::Component(Component::OffsetMinute(
                OffsetMinute::default().with_padding(Padding::Zero)
            ))
        ])
    );
}

#[rstest]
#[case("foo", format_description!("foo"))]
#[case("%a", format_description!("[weekday repr:short]"))]
#[case("%A", format_description!("[weekday]"))]
#[case("%b", format_description!("[month repr:short]"))]
#[case("%B", format_description!("[month repr:long]"))]
#[case("%C", format_description!("[year repr:century]"))]
#[case("%d", format_description!("[day]"))]
#[case("%e", format_description!("[day padding:space]"))]
#[case("%g", format_description!("[year repr:last_two base:iso_week]"))]
#[case("%G", format_description!("[year base:iso_week]"))]
#[case("%h", format_description!("[month repr:short]"))]
#[case("%H", format_description!("[hour]"))]
#[case("%I", format_description!("[hour repr:12]"))]
#[case("%j", format_description!("[ordinal]"))]
#[case("%k", format_description!("[hour padding:space]"))]
#[case("%l", format_description!("[hour repr:12 padding:space]"))]
#[case("%m", format_description!("[month]"))]
#[case("%M", format_description!("[minute]"))]
#[case("%n", format_description!("\n"))]
#[case("%p", format_description!("[period]"))]
#[case("%P", format_description!("[period case:lower]"))]
#[case("%s", format_description!("[unix_timestamp]"))]
#[case("%S", format_description!("[second]"))]
#[case("%t", format_description!("\t"))]
#[case("%u", format_description!("[weekday repr:monday]"))]
#[case("%U", format_description!("[week_number repr:sunday]"))]
#[case("%V", format_description!("[week_number]"))]
#[case("%w", format_description!("[weekday repr:sunday]"))]
#[case("%W", format_description!("[week_number repr:monday]"))]
#[case("%y", format_description!("[year repr:last_two]"))]
#[case("%Y", format_description!("[year]"))]
#[case("%%", format_description!("%"))]
fn strftime_equivalence(
    #[case] strftime: &str,
    #[case] custom: StaticFormatDescription,
) -> time::Result<()> {
    let borrowed = format_description::parse_strftime_borrowed(strftime)?;
    let owned = format_description::parse_strftime_owned(strftime)?;

    assert_eq!(borrowed, custom);
    assert_eq!(owned, OwnedFormatItem::from(custom));

    Ok(())
}

#[rstest]
#[case(
    "%c",
    "[weekday repr:short] [month repr:short] [day padding:space] [hour]:[minute]:[second] [year]"
)]
#[case("%D", "[month]/[day]/[year repr:last_two]")]
#[case("%F", "[year]-[month repr:numerical]-[day]")]
#[case("%r", "[hour repr:12]:[minute]:[second] [period]")]
#[case("%R", "[hour]:[minute]")]
#[case("%T", "[hour]:[minute]:[second]")]
#[case("%x", "[month]/[day]/[year repr:last_two]")]
#[case("%X", "[hour]:[minute]:[second]")]
#[case("%z", "[offset_hour sign:mandatory][offset_minute]")]
fn strftime_compound_equivalence(#[case] strftime: &str, #[case] custom: &str) -> time::Result<()> {
    let borrowed = format_description::parse_strftime_borrowed(strftime)?;
    let owned = format_description::parse_strftime_owned(strftime)?;
    let custom = format_description::parse_borrowed::<2>(custom)?;
    // Until equality is implemented better, we need to convert to a compound.
    let custom = vec![BorrowedFormatItem::Compound(&custom)];

    assert_eq!(borrowed, custom);
    assert_eq!(owned, OwnedFormatItem::from(custom));

    Ok(())
}

#[rstest]
#[case("%_", 2)]
#[case("%-", 2)]
#[case("%0", 2)]
fn strftime_padding_missing_component(#[case] strftime: &str, #[case] expected_index: usize) {
    assert!(matches!(
        format_description::parse_strftime_borrowed(strftime),
        Err(InvalidFormatDescription::Expected {
            what: "valid escape sequence",
            index,
            ..
        }) if index == expected_index
    ));
}
