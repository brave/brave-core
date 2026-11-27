use core::num::NonZero;

use rstest::rstest;
use time::format_description::modifier::{
    CalendarYearFullExtendedRange, Day, End, Ignore, MonthNumerical, OffsetMinute, OffsetSecond,
    Ordinal, Padding, Period, TrailingInput, UnixTimestampNanosecond, WeekNumberIso, WeekdayLong,
};
use time::format_description::{BorrowedFormatItem, Component};
use time::macros::{date, format_description, time};
use time::{Date, Time};

#[rstest]
fn nontrivial_string() {
    assert!(format_description!(r"").is_empty());
    assert!(format_description!(r###""###).is_empty());
    assert!(format_description!(b"").is_empty());
    assert!(format_description!(br"").is_empty());
    assert!(format_description!(br###""###).is_empty());
    #[rustfmt::skip]
    assert_eq!(
        format_description!("foo\
        bar\n\r\t\\\"\'\0\x20\x4E\x4e\u{20}\u{4E}\u{4_e}"),
        &[BorrowedFormatItem::StringLiteral("foobar\n\r\t\\\"'\0 NN NN")]
    );
    #[rustfmt::skip]
    assert_eq!(
        format_description!(b"foo\
        bar\n\r\t\\\"\'\0\x20\x4E\x4e"),
        &[BorrowedFormatItem::StringLiteral("foobar\n\r\t\\\"'\0 NN")]
    );
}

#[rstest]
fn format_description_version() {
    assert_eq!(
        format_description!(version = 1, "[["),
        &[BorrowedFormatItem::StringLiteral("[")]
    );
    assert_eq!(
        format_description!(version = 1, r"\\"),
        &[BorrowedFormatItem::StringLiteral(r"\\")]
    );
    assert_eq!(
        format_description!(version = 2, r"\\"),
        &[BorrowedFormatItem::StringLiteral(r"\")]
    );
}

#[rstest]
fn nested_v1() {
    assert_eq!(
        format_description!(version = 1, "[optional [[[]]"),
        &[BorrowedFormatItem::Optional(
            &BorrowedFormatItem::StringLiteral("[")
        )]
    );
    assert_eq!(
        format_description!(version = 1, "[optional [ [[ ]]"),
        &[BorrowedFormatItem::Optional(
            &BorrowedFormatItem::StringLiteral(" [ ")
        )]
    );
    assert_eq!(
        format_description!(version = 1, "[first [a][[[]]"),
        &[BorrowedFormatItem::First(&[
            BorrowedFormatItem::StringLiteral("a"),
            BorrowedFormatItem::StringLiteral("[")
        ])]
    );
}

#[rstest]
fn optional() {
    assert_eq!(
        format_description!(version = 2, "[optional [:[year]]]"),
        &[BorrowedFormatItem::Optional(&BorrowedFormatItem::Compound(
            &[
                BorrowedFormatItem::StringLiteral(":"),
                BorrowedFormatItem::Component(Component::CalendarYearFullExtendedRange(
                    Default::default()
                ))
            ]
        ))]
    );
    assert_eq!(
        format_description!(version = 2, "[optional [[year]]]"),
        &[BorrowedFormatItem::Optional(
            &BorrowedFormatItem::Component(Component::CalendarYearFullExtendedRange(
                Default::default()
            ))
        )]
    );
    assert_eq!(
        format_description!(version = 2, r"[optional [\[]]"),
        &[BorrowedFormatItem::Optional(
            &BorrowedFormatItem::StringLiteral("[")
        )]
    );
    assert_eq!(
        format_description!(version = 2, r"[optional [ \[ ]]"),
        &[BorrowedFormatItem::Optional(
            &BorrowedFormatItem::StringLiteral(" [ ")
        )]
    );
}

#[rstest]
fn first() {
    assert_eq!(
        format_description!(version = 2, "[first [a]]"),
        &[BorrowedFormatItem::StringLiteral("a")]
    );
    assert_eq!(
        format_description!(version = 2, "[first [a] [b]]"),
        &[BorrowedFormatItem::First(&[
            BorrowedFormatItem::StringLiteral("a"),
            BorrowedFormatItem::StringLiteral("b"),
        ])]
    );
    assert_eq!(
        format_description!(version = 2, "[first [a][b]]"),
        &[BorrowedFormatItem::First(&[
            BorrowedFormatItem::StringLiteral("a"),
            BorrowedFormatItem::StringLiteral("b"),
        ])]
    );
    assert_eq!(
        format_description!(version = 2, r"[first [a][\[]]"),
        &[BorrowedFormatItem::First(&[
            BorrowedFormatItem::StringLiteral("a"),
            BorrowedFormatItem::StringLiteral("["),
        ])]
    );
    assert_eq!(
        format_description!(version = 2, r"[first [a][\[\[]]"),
        &[BorrowedFormatItem::First(&[
            BorrowedFormatItem::StringLiteral("a"),
            BorrowedFormatItem::StringLiteral("[["),
        ])]
    );
    assert_eq!(
        format_description!(
            version = 2,
            "[first [[period case:upper]] [[period case:lower]] ]"
        ),
        &[BorrowedFormatItem::First(&[
            BorrowedFormatItem::Component(Component::Period(
                Period::default()
                    .with_is_uppercase(true)
                    .with_case_sensitive(true)
            )),
            BorrowedFormatItem::Component(Component::Period(
                Period::default()
                    .with_is_uppercase(false)
                    .with_case_sensitive(true)
            )),
        ])]
    );
}

#[rstest]
fn backslash_escape() {
    assert_eq!(
        format_description!(version = 2, r"[optional [\]]]"),
        &[BorrowedFormatItem::Optional(
            &BorrowedFormatItem::StringLiteral("]")
        )]
    );
    assert_eq!(
        format_description!(version = 2, r"[optional [\[]]"),
        &[BorrowedFormatItem::Optional(
            &BorrowedFormatItem::StringLiteral("[")
        )]
    );
    assert_eq!(
        format_description!(version = 2, r"[optional [\\]]"),
        &[BorrowedFormatItem::Optional(
            &BorrowedFormatItem::StringLiteral(r"\")
        )]
    );
    assert_eq!(
        format_description!(version = 2, r"\\"),
        &[BorrowedFormatItem::StringLiteral(r"\")]
    );
    assert_eq!(
        format_description!(version = 2, r"\["),
        &[BorrowedFormatItem::StringLiteral(r"[")]
    );
    assert_eq!(
        format_description!(version = 2, r"\]"),
        &[BorrowedFormatItem::StringLiteral(r"]")]
    );
    assert_eq!(
        format_description!(version = 2, r"foo\\"),
        &[BorrowedFormatItem::StringLiteral(r"foo\"),]
    );
    assert_eq!(
        format_description!(version = 2, r"\\"),
        &[BorrowedFormatItem::StringLiteral(r"\")]
    );
    assert_eq!(
        format_description!(version = 2, r"\["),
        &[BorrowedFormatItem::StringLiteral(r"[")]
    );
    assert_eq!(
        format_description!(version = 2, r"\]"),
        &[BorrowedFormatItem::StringLiteral(r"]")]
    );
    assert_eq!(
        format_description!(version = 2, r"foo\\"),
        &[BorrowedFormatItem::StringLiteral(r"foo\")]
    );
}

#[rstest]
fn format_description_coverage() {
    assert_eq!(
        format_description!("[day padding:space][day padding:zero][day padding:none]"),
        &[
            BorrowedFormatItem::Component(Component::Day(
                Day::default().with_padding(Padding::Space)
            )),
            BorrowedFormatItem::Component(Component::Day(
                Day::default().with_padding(Padding::Zero)
            )),
            BorrowedFormatItem::Component(Component::Day(
                Day::default().with_padding(Padding::None)
            ))
        ]
    );
    assert_eq!(
        format_description!(
            "[offset_minute padding:space][offset_minute padding:zero][offset_minute padding:none]"
        ),
        &[
            BorrowedFormatItem::Component(Component::OffsetMinute(
                OffsetMinute::default().with_padding(Padding::Space)
            )),
            BorrowedFormatItem::Component(Component::OffsetMinute(
                OffsetMinute::default().with_padding(Padding::Zero)
            )),
            BorrowedFormatItem::Component(Component::OffsetMinute(
                OffsetMinute::default().with_padding(Padding::None)
            ))
        ]
    );
    assert_eq!(
        format_description!(
            "[offset_second padding:space][offset_second padding:zero][offset_second padding:none]"
        ),
        &[
            BorrowedFormatItem::Component(Component::OffsetSecond(
                OffsetSecond::default().with_padding(Padding::Space)
            )),
            BorrowedFormatItem::Component(Component::OffsetSecond(
                OffsetSecond::default().with_padding(Padding::Zero)
            )),
            BorrowedFormatItem::Component(Component::OffsetSecond(
                OffsetSecond::default().with_padding(Padding::None)
            )),
        ]
    );
    assert_eq!(
        format_description!("[ordinal padding:space][ordinal padding:zero][ordinal padding:none]"),
        &[
            BorrowedFormatItem::Component(Component::Ordinal(
                Ordinal::default().with_padding(Padding::Space)
            )),
            BorrowedFormatItem::Component(Component::Ordinal(
                Ordinal::default().with_padding(Padding::Zero)
            )),
            BorrowedFormatItem::Component(Component::Ordinal(
                Ordinal::default().with_padding(Padding::None)
            )),
        ]
    );
    assert_eq!(
        format_description!("[month repr:numerical]"),
        &[BorrowedFormatItem::Component(Component::MonthNumerical(
            MonthNumerical::default().with_padding(Padding::Zero)
        ))]
    );
    assert_eq!(
        format_description!("[week_number repr:iso ]"),
        &[BorrowedFormatItem::Component(Component::WeekNumberIso(
            WeekNumberIso::default().with_padding(Padding::Zero)
        ))]
    );
    assert_eq!(
        format_description!("[weekday repr:long]"),
        &[BorrowedFormatItem::Component(Component::WeekdayLong(
            WeekdayLong::default()
        ))]
    );
    assert_eq!(
        format_description!("[year repr:full base:calendar]"),
        &[BorrowedFormatItem::Component(
            Component::CalendarYearFullExtendedRange(
                CalendarYearFullExtendedRange::default()
                    .with_padding(Padding::Zero)
                    .with_sign_is_mandatory(false)
            )
        )]
    );
    assert_eq!(
        format_description!("[[ "),
        &[BorrowedFormatItem::StringLiteral("[ ")]
    );
    assert_eq!(
        format_description!("[ignore count:2]"),
        &[BorrowedFormatItem::Component(Component::Ignore(
            Ignore::count(const { NonZero::new(2).unwrap() })
        ))]
    );
    assert_eq!(
        format_description!("[unix_timestamp precision:nanosecond sign:mandatory]"),
        &[BorrowedFormatItem::Component(
            Component::UnixTimestampNanosecond(
                UnixTimestampNanosecond::default().with_sign_is_mandatory(true)
            )
        )]
    );
    assert_eq!(
        format_description!("[end]"),
        &[BorrowedFormatItem::Component(
            Component::End(End::default())
        )]
    );
    assert_eq!(
        format_description!("[end trailing_input:prohibit]"),
        &[BorrowedFormatItem::Component(Component::End(
            End::default().with_trailing_input(TrailingInput::Prohibit)
        ))]
    );
    assert_eq!(
        format_description!("[end trailing_input:discard]"),
        &[BorrowedFormatItem::Component(Component::End(
            End::default().with_trailing_input(TrailingInput::Discard)
        ))]
    );
}

#[rstest]
fn date_coverage() {
    assert_eq!(Ok(date!(2000-001)), Date::from_ordinal_date(2000, 1));
    assert_eq!(Ok(date!(2019-W01-1)), Date::from_ordinal_date(2018, 365));
    assert_eq!(Ok(date!(2021-W52-6)), Date::from_ordinal_date(2022, 1));
    assert_eq!(Ok(date!(2021-W34-5)), Date::from_ordinal_date(2021, 239));
}

#[rstest]
fn time_coverage() {
    assert_eq!(time!(12 AM), Time::MIDNIGHT);
    assert_eq!(Ok(time!(12 PM)), Time::from_hms(12, 0, 0));
}

mod demo {
    #[expect(dead_code)]
    type Result<T> = core::result::Result<T, ()>;
    #[expect(dead_code)]
    type Option = core::option::Option<()>;

    #[expect(dead_code)]
    fn serialize() {}
    #[expect(dead_code)]
    fn deserialize() {}

    time::serde::format_description!(
        seconds,
        OffsetDateTime,
        "[year]-[month]-[day]T[hour]:[minute]:[second]Z"
    );
}
