use std::fmt::Debug;
use std::marker::PhantomData;

use rstest::rstest;
use serde::{Deserialize, Serialize};
use serde_test2::{Token, assert_de_tokens_error, assert_ser_tokens_error, assert_tokens};
use time::OffsetDateTime;
use time::macros::datetime;

#[derive(Debug, PartialEq, Serialize, Deserialize)]
struct Rfc2822(
    #[serde(with = "time::serde::rfc2822")] OffsetDateTime,
    #[serde(with = "time::serde::rfc2822::option")] Option<OffsetDateTime>,
    #[serde(with = "time::serde::rfc2822::option")] Option<OffsetDateTime>,
);

#[derive(Debug, PartialEq, Serialize, Deserialize)]
struct Rfc3339(
    #[serde(with = "time::serde::rfc3339")] OffsetDateTime,
    #[serde(with = "time::serde::rfc3339::option")] Option<OffsetDateTime>,
    #[serde(with = "time::serde::rfc3339::option")] Option<OffsetDateTime>,
);

#[derive(Debug, PartialEq, Serialize, Deserialize)]
#[serde(transparent)]
struct Timestamp(#[serde(with = "time::serde::timestamp")] OffsetDateTime);

#[derive(Debug, PartialEq, Serialize, Deserialize)]
#[serde(transparent)]
struct OptTimestamp(#[serde(with = "time::serde::timestamp::option")] Option<OffsetDateTime>);

#[rstest]
#[case(
    Rfc2822(
        datetime!(2021-01-02 03:04:05 UTC),
        Some(datetime!(2021-01-02 03:04:05 UTC)),
        None
    ), &[
        Token::TupleStruct {
            name: "Rfc2822",
            len: 3,
        },
        Token::Str("Sat, 02 Jan 2021 03:04:05 +0000"),
        Token::Some,
        Token::Str("Sat, 02 Jan 2021 03:04:05 +0000"),
        Token::None,
        Token::TupleStructEnd,
    ],)
]
#[case(
    Rfc3339(
        datetime!(2021-01-02 03:04:05 UTC),
        Some(datetime!(2021-01-02 03:04:05 UTC)),
        None
    ), &[
        Token::TupleStruct {
            name: "Rfc3339",
            len: 3,
        },
        Token::Str("2021-01-02T03:04:05Z"),
        Token::Some,
        Token::Str("2021-01-02T03:04:05Z"),
        Token::None,
        Token::TupleStructEnd,
    ],
)]
#[case(Timestamp(datetime!(2021-01-02 03:04:05 UTC)), &[Token::I64(1_609_556_645)])]
#[case(
    OptTimestamp(Some(datetime!(2021-01-02 03:04:05 UTC))),
    &[Token::Some, Token::I64(1_609_556_645)],
)]
#[case(OptTimestamp(None), &[Token::None])]
fn success<T>(#[case] input: T, #[case] expected: &[Token])
where
    T: Debug + PartialEq + Serialize + for<'de> Deserialize<'de>,
{
    assert_tokens(&input, expected);
}

#[rstest]
#[case(
    Rfc2822(
        datetime!(2021-01-02 03:04:05 +0:00:01),
        Some(datetime!(2021-01-02 03:04:05 UTC)),
        None
    ),
    &[
        Token::TupleStruct {
            name: "Rfc2822",
            len: 3,
        },
    ],
    "The offset_second component cannot be formatted into the requested format.",
)]
#[case(
    Rfc2822(
        datetime!(2021-01-02 03:04:05 UTC),
        Some(datetime!(2021-01-02 03:04:05 +0:00:01)),
        None
    ),
    &[
        Token::TupleStruct {
            name: "Rfc2822",
            len: 3,
        },
        Token::Str("Sat, 02 Jan 2021 03:04:05 +0000"),
    ],
    "The offset_second component cannot be formatted into the requested format.",
)]
#[case(
    Rfc3339(
        datetime!(2021-01-02 03:04:05 +0:00:01),
        Some(datetime!(2021-01-02 03:04:05 UTC)),
        None
    ),
    &[
        Token::TupleStruct {
            name: "Rfc3339",
            len: 3,
        },
    ],
    "The offset_second component cannot be formatted into the requested format.",
)]
#[case(
    Rfc3339(
        datetime!(2021-01-02 03:04:05 UTC),
        Some(datetime!(2021-01-02 03:04:05 +0:00:01)),
        None
    ),
    &[
        Token::TupleStruct {
            name: "Rfc3339",
            len: 3,
        },
        Token::Str("2021-01-02T03:04:05Z"),
    ],
    "The offset_second component cannot be formatted into the requested format.",
)]
fn serialize_error<T>(#[case] value: T, #[case] tokens: &[Token], #[case] error: &str)
where
    T: Serialize,
{
    assert_ser_tokens_error(&value, tokens, error);
}

#[rstest]
#[case(
    PhantomData::<Rfc2822>,
    &[
        Token::TupleStruct {
            name: "Rfc2822",
            len: 3,
        },
        Token::Bool(false),
    ],
    "invalid type: boolean `false`, expected an RFC2822-formatted `OffsetDateTime`",
)]
#[case(
    PhantomData::<Rfc2822>,
    &[
        Token::TupleStruct {
            name: "Rfc2822",
            len: 3,
        },
        Token::Str("Sat, 02 Jan 2021 03:04:05 +0000"),
        Token::Bool(false),
    ],
    "invalid type: boolean `false`, expected an RFC2822-formatted `Option<OffsetDateTime>`",
)]
#[case(
    PhantomData::<Rfc2822>,
    &[
        Token::TupleStruct {
            name: "Rfc2822",
            len: 3,
        },
        Token::Str("Sat, 02 Jan 2021 03:04:05 +0000"),
        Token::Some,
        Token::Bool(false),
    ],
    "invalid type: boolean `false`, expected an RFC2822-formatted `OffsetDateTime`",
)]
#[case(
    PhantomData::<Rfc3339>,
    &[
        Token::TupleStruct {
            name: "Rfc3339",
            len: 3,
        },
        Token::Bool(false),
    ],
    "invalid type: boolean `false`, expected an RFC3339-formatted `OffsetDateTime`",
)]
#[case(
    PhantomData::<Rfc3339>,
    &[
        Token::TupleStruct {
            name: "Rfc3339",
            len: 3,
        },
        Token::Str("2021-01-02T03:04:05Z"),
        Token::Bool(false),
    ],
    "invalid type: boolean `false`, expected an RFC3339-formatted `Option<OffsetDateTime>`",
)]
#[case(
    PhantomData::<Rfc3339>,
    &[
        Token::TupleStruct {
            name: "Rfc3339",
            len: 3,
        },
        Token::Str("2021-01-02T03:04:05Z"),
        Token::Some,
        Token::Bool(false),
    ],
    "invalid type: boolean `false`, expected an RFC3339-formatted `OffsetDateTime`",
)]
#[case(
    PhantomData::<Timestamp>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected i64",
)]
#[case(
    PhantomData::<OptTimestamp>,
    &[Token::Some, Token::Bool(false)],
    "invalid type: boolean `false`, expected i64",
)]
#[case(
    PhantomData::<Timestamp>,
    &[Token::I64(100_000_000_000_000)],
    "invalid timestamp, expected an in-range value",
)]
#[case(
    PhantomData::<OptTimestamp>,
    &[Token::Some, Token::I64(-100_000_000_000_000)],
    "invalid timestamp, expected an in-range value",
)]
fn deserialize_error<T>(
    #[case] _type: PhantomData<T>,
    #[case] tokens: &[Token],
    #[case] error: &str,
) where
    T: for<'de> Deserialize<'de>,
{
    assert_de_tokens_error::<T>(tokens, error);
}
