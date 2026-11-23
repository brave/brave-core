use rstest::rstest;
use serde::{Deserialize, Serialize};
use serde_test2::{
    Compact, Configure, Token, assert_de_tokens_error, assert_ser_tokens_error, assert_tokens,
};
use time::OffsetDateTime;
use time::macros::datetime;
use time::serde::rfc3339;

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
struct Test {
    #[serde(with = "rfc3339")]
    dt: OffsetDateTime,
    #[serde(with = "rfc3339::option")]
    option_dt: Option<OffsetDateTime>,
}

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
struct TestWithDefault {
    #[serde(with = "rfc3339")]
    date: OffsetDateTime,
    #[serde(with = "rfc3339::option", default)]
    maybe_date: Option<OffsetDateTime>,
}

#[rstest]
#[case(
    Test {
        dt: datetime!(2000-01-01 00:00:00 UTC),
        option_dt: Some(datetime!(2000-01-01 00:00:00 UTC)),
    }.compact(),
    &[
        Token::Struct {
            name: "Test",
            len: 2,
        },
        Token::Str("dt"),
        Token::BorrowedStr("2000-01-01T00:00:00Z"),
        Token::Str("option_dt"),
        Token::Some,
        Token::BorrowedStr("2000-01-01T00:00:00Z"),
        Token::StructEnd,
    ]
)]
#[case(
    Test {
        dt: datetime!(2000-01-01 00:00:00 UTC),
        option_dt: None,
    }.compact(),
    &[
        Token::Struct {
            name: "Test",
            len: 2,
        },
        Token::Str("dt"),
        Token::BorrowedStr("2000-01-01T00:00:00Z"),
        Token::Str("option_dt"),
        Token::None,
        Token::StructEnd,
    ]
)]
fn serialize_deserialize(#[case] value: Compact<Test>, #[case] tokens: &[Token]) {
    assert_tokens(&value, tokens);
}

#[rstest]
#[case(
    &[
        Token::Struct {
            name: "Test",
            len: 2,
        },
        Token::Str("dt"),
        Token::BorrowedStr("bad"),
        Token::StructEnd,
    ],
    "the 'year' component could not be parsed"
)]
fn deserialize_error(#[case] tokens: &[Token], #[case] expected: &str) {
    assert_de_tokens_error::<Test>(tokens, expected);
}

#[rstest]
#[case(
    Test {
        dt: datetime!(2000-01-01 00:00:00 +00:00:01),
        option_dt: None,
    },
    &[
        Token::Struct {
            name: "Test",
            len: 2,
        },
        Token::Str("dt"),
    ],
    "The offset_second component cannot be formatted into the requested format."
)]
fn serialize_error(#[case] value: Test, #[case] tokens: &[Token], #[case] expected: &str) {
    assert_ser_tokens_error::<Test>(&value, tokens, expected);
}

#[rstest]
#[case(
    r#"{"dt": "2000-01-01T00:00:00Z", "option_dt": null}"#,
    Test {
        dt: datetime!(2000-01-01 00:00:00 UTC),
        option_dt: None,
    }
)]
fn parse_json(#[case] json: &str, #[case] expected: Test) {
    assert_eq!(serde_json::from_str::<Test>(json).ok(), Some(expected));
}

#[rstest]
#[case(
    r#"{"date": "2022-05-01T10:20:42.123Z"}"#,
    TestWithDefault {
        date: datetime!(2022-05-01 10:20:42.123 UTC),
        maybe_date: None,
    }
)]
#[case(
    r#"{"date": "2022-05-01T10:20:42.123Z", "maybe_date": "2022-05-01T10:20:42.123Z"}"#,
    TestWithDefault {
        date: datetime!(2022-05-01 10:20:42.123 UTC),
        maybe_date: Some(datetime!(2022-05-01 10:20:42.123 UTC)),
    }
)]
fn issue_479(#[case] json: &str, #[case] expected: TestWithDefault) {
    assert_eq!(
        serde_json::from_str::<TestWithDefault>(json).ok(),
        Some(expected)
    );
}
