use rstest::rstest;
use serde::{Deserialize, Serialize};
use serde_test2::{
    Compact, Configure, Token, assert_de_tokens, assert_de_tokens_error, assert_ser_tokens_error,
    assert_tokens,
};
use time::OffsetDateTime;
use time::macros::datetime;
use time::serde::iso8601;

#[derive(Serialize, Deserialize, Debug, Eq, PartialEq)]
struct Test {
    #[serde(with = "iso8601")]
    dt: OffsetDateTime,
    #[serde(with = "iso8601::option")]
    option_dt: Option<OffsetDateTime>,
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
        Token::BorrowedStr("+002000-01-01T00:00:00.000000000Z"),
        Token::Str("option_dt"),
        Token::Some,
        Token::BorrowedStr("+002000-01-01T00:00:00.000000000Z"),
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
        Token::BorrowedStr("+002000-01-01T00:00:00.000000000Z"),
        Token::Str("option_dt"),
        Token::None,
        Token::StructEnd,
    ]
)]
fn serialize(#[case] test: Compact<Test>, #[case] tokens: &[Token]) {
    assert_tokens(&test, tokens);
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
    "The offset_second component cannot be formatted into the requested format.",
)]
fn serialize_error(#[case] value: Test, #[case] tokens: &[Token], #[case] error: &str) {
    assert_ser_tokens_error::<Test>(&value, tokens, error);
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
fn deserialize_error(#[case] tokens: &[Token], #[case] error: &str) {
    assert_de_tokens_error::<Test>(tokens, error);
}

#[rstest]
fn issue_674_leap_second_support() {
    assert_de_tokens::<Test>(
        &Test {
            dt: datetime!(2016-12-31 23:59:59.999999999 UTC),
            option_dt: Some(datetime!(2016-12-31 23:59:59.999999999 UTC)),
        },
        &[
            Token::Struct {
                name: "Test",
                len: 2,
            },
            Token::Str("dt"),
            Token::BorrowedStr("2016-12-31T23:59:60.000Z"),
            Token::Str("option_dt"),
            Token::Some,
            Token::BorrowedStr("2016-12-31T23:59:60.000Z"),
            Token::StructEnd,
        ],
    );
}

#[rstest]
fn issue_724_truncation() {
    #[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
    struct Demo(#[serde(with = "time::serde::iso8601")] OffsetDateTime);

    let value = datetime!(2025-01-10 23:01:16.000081999 UTC);
    let info = Demo(value);

    let serialized = serde_json::to_string(&info).expect("serialization failed");
    assert_eq!(serialized, r#""+002025-01-10T23:01:16.000081999Z""#);

    let deserialized: Demo = serde_json::from_str(&serialized).expect("deserialization failed");
    assert_eq!(info, deserialized);
}
