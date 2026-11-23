use rstest::rstest;
use serde_test2::Compact;
#[rustfmt::skip] // Tries to remove the leading `::`, which breaks compilation.
use ::serde::{Deserialize, Serialize};
use serde_test2::{
    Configure, Token, assert_de_tokens, assert_de_tokens_error, assert_ser_tokens_error,
    assert_tokens,
};
use time::format_description::BorrowedFormatItem;
use time::format_description::well_known::{Iso8601, iso8601};
use time::macros::{date, datetime, offset, time};
use time::{Date, OffsetDateTime, PlainDateTime, Time, UtcOffset, serde};

// Not used in the tests, but ensures that the macro compiles.
#[expect(dead_code)]
const ISO_FORMAT: Iso8601<{ iso8601::Config::DEFAULT.encode() }> =
    Iso8601::<{ iso8601::Config::DEFAULT.encode() }>;
time::serde::format_description!(my_format, OffsetDateTime, ISO_FORMAT);
time::serde::format_description!(
    my_format2,
    OffsetDateTime,
    Iso8601::<{ iso8601::Config::DEFAULT.encode() }>
);

mod nested {
    time::serde::format_description!(
        pub(super) offset_dt_format,
        OffsetDateTime,
        "custom format: [year]-[month]-[day] [hour]:[minute]:[second] [offset_hour \
         sign:mandatory]:[offset_minute]"
    );
    time::serde::format_description!(
        pub plain_dt_format,
        PlainDateTime,
        "custom format: [year]-[month]-[day] [hour]:[minute]:[second]"
    );
    time::serde::format_description!(
        pub(in crate::serde::macros) time_format,
        Time,
        "custom format: [minute]:[second]"
    );
}
serde::format_description!(date_format, Date, "custom format: [year]-[month]-[day]");
serde::format_description!(
    offset_format,
    UtcOffset,
    "custom format: [offset_hour sign:mandatory]:[offset_minute]"
);

const TIME_FORMAT_ALT: &[BorrowedFormatItem<'_>] =
    time::macros::format_description!("[hour]:[minute]");
serde::format_description!(time_format_alt, Time, TIME_FORMAT_ALT);
serde::format_description!(
    time_format_alt2,
    Time,
    time::macros::format_description!("[hour]:[minute]")
);

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
struct TestCustomFormat {
    #[serde(with = "nested::offset_dt_format")]
    offset_dt: OffsetDateTime,
    #[serde(with = "nested::plain_dt_format::option")]
    plain_dt: Option<PlainDateTime>,
    #[serde(with = "date_format")]
    date: Date,
    #[serde(with = "nested::time_format::option")]
    time: Option<Time>,
    #[serde(with = "offset_format")]
    offset: UtcOffset,
    #[serde(with = "time_format_alt")]
    time_alt: Time,
}

#[rstest]
#[case(
    TestCustomFormat {
        offset_dt: datetime!(2000-01-01 00:00 -4:00),
        plain_dt: Some(datetime!(2000-01-01 00:00)),
        date: date!(2000-01-01),
        time: None,
        offset: offset!(-4),
        time_alt: time!(12:34),
    }.compact(),
    &[
        Token::Struct {
            name: "TestCustomFormat",
            len: 6,
        },
        Token::Str("offset_dt"),
        Token::BorrowedStr("custom format: 2000-01-01 00:00:00 -04:00"),
        Token::Str("plain_dt"),
        Token::Some,
        Token::BorrowedStr("custom format: 2000-01-01 00:00:00"),
        Token::Str("date"),
        Token::BorrowedStr("custom format: 2000-01-01"),
        Token::Str("time"),
        Token::None,
        Token::Str("offset"),
        Token::BorrowedStr("custom format: -04:00"),
        Token::Str("time_alt"),
        Token::BorrowedStr("12:34"),
        Token::StructEnd,
    ]
)]
fn custom_serialize(#[case] value: Compact<TestCustomFormat>, #[case] tokens: &[Token]) {
    assert_tokens(&value, tokens);
}

#[rstest]
#[case(
    &[
        Token::Struct {
            name: "TestCustomFormat",
            len: 6,
        },
        Token::Str("offset_dt"),
        Token::BorrowedStr("custom format: 2000-01-01 0:00:00 -04:00"),
    ],
    "the 'hour' component could not be parsed",
)]
#[case(
    &[
        Token::Struct {
            name: "TestCustomFormat",
            len: 6,
        },
        Token::Str("offset_dt"),
        Token::BorrowedStr("custom format: 2000-01-01 00:00:00 -04:00"),
        Token::Str("plain_dt"),
        Token::Some,
        Token::BorrowedStr("custom format: 2000-01-01 0:00:00 -04:00"),
    ],
    "the 'hour' component could not be parsed"
)]
#[case(
    &[
        Token::Struct {
            name: "TestCustomFormat",
            len: 6,
        },
        Token::Str("offset_dt"),
        Token::Bool(false),
    ],
    "invalid type: boolean `false`, expected a(n) `OffsetDateTime` in the format \"custom format: \
    [year]-[month]-[day] [hour]:[minute]:[second] [offset_hour sign:mandatory]:[offset_minute]\""
)]
#[case(
    &[
        Token::Struct {
            name: "TestCustomFormat",
            len: 6,
        },
        Token::Str("offset_dt"),
        Token::BorrowedStr("custom format: 2000-01-01 00:00:00 -04:00"),
        Token::Str("plain_dt"),
        Token::Bool(false),
    ],
    "invalid type: boolean `false`, expected an `Option<PlainDateTime>` in the format \"custom \
    format: [year]-[month]-[day] [hour]:[minute]:[second]\""
)]
fn custom_deserialize_error(#[case] tokens: &[Token], #[case] error: &str) {
    assert_de_tokens_error::<TestCustomFormat>(tokens, error);
}

// This format string has offset_hour and offset_minute, but is for formatting PlainDateTime.
serde::format_description!(
    plain_date_time_format_bad,
    PlainDateTime,
    "[offset_hour]:[offset_minute]"
);

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
struct TestCustomFormatPlainDateTimeBad {
    #[serde(with = "plain_date_time_format_bad")]
    dt: PlainDateTime,
}

#[rstest]
#[case(
    TestCustomFormatPlainDateTimeBad {
        dt: datetime!(2000-01-01 00:00),
    },
    &[
        Token::Struct {
            name: "TestCustomFormatPlainDateTimeBad",
            len: 1,
        },
        Token::Str("dt"),
    ],
    "The type being formatted does not contain sufficient information to format a component.",
)]
fn custom_serialize_bad_type_error(
    #[case] value: TestCustomFormatPlainDateTimeBad,
    #[case] tokens: &[Token],
    #[case] error: &str,
) {
    assert_ser_tokens_error::<TestCustomFormatPlainDateTimeBad>(&value, tokens, error);
}

// Test the behavior of versioning.
serde::format_description!(version = 1, version_test_1, Time, "[[ [hour]:[minute]");
serde::format_description!(version = 1, version_test_2, Time, r"\\ [hour]:[minute]");
serde::format_description!(version = 2, version_test_3, Time, r"\\ [hour]:[minute]");
serde::format_description!(version, Time, "[hour]:[minute]");

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
struct TestVersioning {
    #[serde(with = "version_test_1")]
    time_1: Time,
    #[serde(with = "version_test_2")]
    time_2: Time,
    #[serde(with = "version_test_3")]
    time_3: Time,
    #[serde(with = "version")]
    time_4: Time,
}

#[rstest]
fn versioning() {
    let value = TestVersioning {
        time_1: Time::MIDNIGHT,
        time_2: Time::MIDNIGHT,
        time_3: Time::MIDNIGHT,
        time_4: Time::MIDNIGHT,
    };
    assert_tokens(
        &value,
        &[
            Token::Struct {
                name: "TestVersioning",
                len: 4,
            },
            Token::Str("time_1"),
            Token::Str("[ 00:00"),
            Token::Str("time_2"),
            Token::Str(r"\\ 00:00"),
            Token::Str("time_3"),
            Token::Str(r"\ 00:00"),
            Token::Str("time_4"),
            Token::Str("00:00"),
            Token::StructEnd,
        ],
    );
}

serde::format_description!(
    version = 1,
    nested_v1,
    Time,
    "[hour]:[minute][optional [:[second]]]"
);
serde::format_description!(
    version = 2,
    nested_v2,
    Time,
    "[hour]:[minute][optional [:[second]]]"
);

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
struct TestNested {
    #[serde(with = "nested_v1")]
    time_1: Time,
    #[serde(with = "nested_v2")]
    time_2: Time,
}

#[rstest]
#[case(
    TestNested {
        time_1: time!(12:34:56),
        time_2: time!(12:34:56),
    },
    &[
        Token::Struct {
            name: "TestNested",
            len: 2,
        },
        Token::Str("time_1"),
        Token::Str("12:34:56"),
        Token::Str("time_2"),
        Token::Str("12:34:56"),
        Token::StructEnd,
    ]
)]
fn nested_roundtrip(#[case] value: TestNested, #[case] tokens: &[Token]) {
    assert_tokens(&value, tokens);
}

#[rstest]
#[case(
    TestNested {
        time_1: time!(12:34),
        time_2: time!(12:34),
    },
    &[
        Token::Struct {
            name: "TestNested",
            len: 2,
        },
        Token::Str("time_1"),
        Token::Str("12:34"),
        Token::Str("time_2"),
        Token::Str("12:34"),
        Token::StructEnd,
    ]
)]
fn nested_deserialize(#[case] value: TestNested, #[case] tokens: &[Token]) {
    assert_de_tokens(&value, tokens);
}
