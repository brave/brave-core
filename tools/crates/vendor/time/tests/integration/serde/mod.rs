use std::fmt::Debug;
use std::marker::PhantomData;

use rstest::rstest;
use serde::{Deserialize, Serialize};
use serde_test2::{
    Compact, Configure, Readable, Token, assert_de_tokens, assert_de_tokens_error, assert_tokens,
};
use time::Month::*;
use time::Weekday::*;
use time::macros::{date, datetime, offset, time};
use time::{Date, Month, OffsetDateTime, PlainDateTime, SignedDuration, Time, UtcOffset, Weekday};

mod error_conditions;
mod iso8601;
mod json;
mod macros;
mod rfc2822;
mod rfc3339;
mod timestamps;

#[rstest]
#[case(
    Time::MIDNIGHT.compact(),
    &[
        Token::Tuple { len: 4 },
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(0),
        Token::TupleEnd,
    ],
)]
#[case(
    time!(23:58:59.123_456_789).compact(),
    &[
        Token::Tuple { len: 4 },
        Token::U8(23),
        Token::U8(58),
        Token::U8(59),
        Token::U32(123_456_789),
        Token::TupleEnd,
    ],
)]
#[case(Time::MIDNIGHT.readable(), &[Token::BorrowedStr("00:00:00.0")])]
#[case(time!(23:58:59.123_456_789).readable(), &[Token::BorrowedStr("23:58:59.123456789")])]
#[case(
    date!(-9999-001).compact(),
    &[
        Token::Tuple { len: 2 },
        Token::I32(-9999),
        Token::U16(1),
        Token::TupleEnd,
    ],
)]
#[case(
    date!(+9999-365).compact(),
    &[
        Token::Tuple { len: 2 },
        Token::I32(9999),
        Token::U16(365),
        Token::TupleEnd,
    ],
)]
#[case(date!(-9999-001).readable(), &[Token::BorrowedStr("-9999-01-01")])]
#[case(date!(+9999-365).readable(), &[Token::BorrowedStr("9999-12-31")])]
#[case(
    datetime!(-9999-001 0:00).compact(),
    &[
        Token::Tuple { len: 6 },
        Token::I32(-9999),
        Token::U16(1),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(0),
        Token::TupleEnd,
    ],
)]
#[case(
    datetime!(+9999-365 23:58:59.123_456_789).compact(),
    &[
        Token::Tuple { len: 6 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(23),
        Token::U8(58),
        Token::U8(59),
        Token::U32(123_456_789),
        Token::TupleEnd,
    ],
)]
#[case(datetime!(-9999-001 0:00).readable(), &[Token::BorrowedStr("-9999-01-01 00:00:00.0")])]
#[case(
    datetime!(+9999-365 23:58:59.123_456_789).readable(),
    &[Token::BorrowedStr("9999-12-31 23:58:59.123456789")],
)]
#[case(
    datetime!(-9999-001 0:00 UTC).to_offset(offset!(+23:58:59)).compact(),
    &[
        Token::Tuple { len: 9 },
        Token::I32(-9999),
        Token::U16(1),
        Token::U8(23),
        Token::U8(58),
        Token::U8(59),
        Token::U32(0),
        Token::I8(23),
        Token::I8(58),
        Token::I8(59),
        Token::TupleEnd,
    ],
)]
#[case(
    datetime!(+9999-365 23:58:59.123_456_789 UTC).to_offset(offset!(-23:58:59)).compact(),
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(123_456_789),
        Token::I8(-23),
        Token::I8(-58),
        Token::I8(-59),
        Token::TupleEnd,
    ],
)]
#[case(
    datetime!(-9999-001 0:00 UTC).to_offset(offset!(+23:58:59)).readable(),
    &[Token::BorrowedStr("-9999-01-01 23:58:59.0 +23:58:59")],
)]
#[case(
    datetime!(+9999-365 23:58:59.123_456_789 UTC).to_offset(offset!(-23:58:59)).readable(),
    &[Token::BorrowedStr("9999-12-31 00:00:00.123456789 -23:58:59")],
)]
#[case(
    offset!(-23:58:59).compact(),
    &[
        Token::Tuple { len: 3 },
        Token::I8(-23),
        Token::I8(-58),
        Token::I8(-59),
        Token::TupleEnd,
    ],
)]
#[case(
    offset!(+23:58:59).compact(),
    &[
        Token::Tuple { len: 3 },
        Token::I8(23),
        Token::I8(58),
        Token::I8(59),
        Token::TupleEnd,
    ],
)]
#[case(offset!(-23:58:59).readable(), &[Token::BorrowedStr("-23:58:59")])]
#[case(offset!(+23:58:59).readable(), &[Token::BorrowedStr("+23:58:59")])]
#[case(
    SignedDuration::MIN.compact(),
    &[
        Token::Tuple { len: 2 },
        Token::I64(i64::MIN),
        Token::I32(-999_999_999),
        Token::TupleEnd,
    ],
)]
#[case(
    SignedDuration::MAX.compact(),
    &[
        Token::Tuple { len: 2 },
        Token::I64(i64::MAX),
        Token::I32(999_999_999),
        Token::TupleEnd,
    ],
)]
#[case(SignedDuration::MIN.readable(), &[Token::BorrowedStr("-9223372036854775808.999999999")])]
#[case(SignedDuration::MAX.readable(), &[Token::BorrowedStr("9223372036854775807.999999999")])]
#[case(SignedDuration::ZERO.readable(), &[Token::BorrowedStr("0.000000000")])]
#[case(SignedDuration::nanoseconds(123).readable(), &[Token::BorrowedStr("0.000000123")])]
#[case(SignedDuration::nanoseconds(-123).readable(), &[Token::BorrowedStr("-0.000000123")])]
#[case(Monday.compact(), &[Token::U8(1)])]
#[case(Tuesday.compact(), &[Token::U8(2)])]
#[case(Wednesday.compact(), &[Token::U8(3)])]
#[case(Thursday.compact(), &[Token::U8(4)])]
#[case(Friday.compact(), &[Token::U8(5)])]
#[case(Saturday.compact(), &[Token::U8(6)])]
#[case(Sunday.compact(), &[Token::U8(7)])]
#[case(Monday.readable(), &[Token::BorrowedStr("Monday")])]
#[case(Tuesday.readable(), &[Token::BorrowedStr("Tuesday")])]
#[case(Wednesday.readable(), &[Token::BorrowedStr("Wednesday")])]
#[case(Thursday.readable(), &[Token::BorrowedStr("Thursday")])]
#[case(Friday.readable(), &[Token::BorrowedStr("Friday")])]
#[case(Saturday.readable(), &[Token::BorrowedStr("Saturday")])]
#[case(Sunday.readable(), &[Token::BorrowedStr("Sunday")])]
#[case(January.compact(), &[Token::U8(1)])]
#[case(February.compact(), &[Token::U8(2)])]
#[case(March.compact(), &[Token::U8(3)])]
#[case(April.compact(), &[Token::U8(4)])]
#[case(May.compact(), &[Token::U8(5)])]
#[case(June.compact(), &[Token::U8(6)])]
#[case(July.compact(), &[Token::U8(7)])]
#[case(August.compact(), &[Token::U8(8)])]
#[case(September.compact(), &[Token::U8(9)])]
#[case(October.compact(), &[Token::U8(10)])]
#[case(November.compact(), &[Token::U8(11)])]
#[case(December.compact(), &[Token::U8(12)])]
#[case(January.readable(), &[Token::BorrowedStr("January")])]
#[case(February.readable(), &[Token::BorrowedStr("February")])]
#[case(March.readable(), &[Token::BorrowedStr("March")])]
#[case(April.readable(), &[Token::BorrowedStr("April")])]
#[case(May.readable(), &[Token::BorrowedStr("May")])]
#[case(June.readable(), &[Token::BorrowedStr("June")])]
#[case(July.readable(), &[Token::BorrowedStr("July")])]
#[case(August.readable(), &[Token::BorrowedStr("August")])]
#[case(September.readable(), &[Token::BorrowedStr("September")])]
#[case(October.readable(), &[Token::BorrowedStr("October")])]
#[case(November.readable(), &[Token::BorrowedStr("November")])]
#[case(December.readable(), &[Token::BorrowedStr("December")])]
fn success<T>(#[case] value: T, #[case] tokens: &[Token])
where
    T: Debug + PartialEq + Serialize + for<'de> Deserialize<'de>,
{
    assert_tokens(&value, tokens);
}

#[rstest]
#[case(
    PhantomData::<Compact<Time>>,
    &[
        Token::Tuple { len: 4 },
        Token::U8(24),
        Token::U8(0),
        Token::U8(0),
        Token::U32(0),
        Token::TupleEnd,
    ],
    "invalid hour, expected an in-range value",
)]
#[case(
    PhantomData::<Readable<Time>>,
    &[Token::BorrowedStr("24:00:00.0")],
    "the 'hour' component could not be parsed",
)]
#[case(
    PhantomData::<Readable<Time>>,
    &[Token::BorrowedStr("23-00:00.0")],
    "a character literal was not valid",
)]
#[case(
    PhantomData::<Readable<Time>>,
    &[Token::BorrowedStr("0:00:00.0")],
    "the 'hour' component could not be parsed",
)]
#[case(
    PhantomData::<Readable<Time>>,
    &[Token::BorrowedStr("00:00:00.0x")],
    "unexpected trailing characters; the end of input was expected",
)]
#[case(
    PhantomData::<Readable<Time>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `Time`",
)]
#[case(
    PhantomData::<Compact<Time>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `Time`",
)]
#[case(
    PhantomData::<Readable<Date>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `Date`",
)]
#[case(
    PhantomData::<Compact<Date>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `Date`",
)]
#[case(
    PhantomData::<Readable<PlainDateTime>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `PlainDateTime`",
)]
#[case(
    PhantomData::<Compact<PlainDateTime>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `PlainDateTime`",
)]
#[case(
    PhantomData::<Compact<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(2021),
        Token::U16(366),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(0),
        Token::TupleEnd,
    ],
    "invalid ordinal, expected an in-range value",
)]
#[case(
    PhantomData::<Compact<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(2021),
        Token::U16(1),
        Token::U8(24),
        Token::U8(0),
        Token::U8(0),
        Token::U32(0),
        Token::TupleEnd,
    ],
    "invalid hour, expected an in-range value",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected an `OffsetDateTime`",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected an `OffsetDateTime`",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(2021),
        Token::U16(366),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(0),
        Token::I8(0),
        Token::I8(0),
        Token::I8(0),
        Token::TupleEnd,
    ],
    "invalid ordinal, expected an in-range value",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(2021),
        Token::U16(1),
        Token::U8(24),
        Token::U8(0),
        Token::U8(0),
        Token::U32(0),
        Token::I8(0),
        Token::I8(0),
        Token::I8(0),
        Token::TupleEnd,
    ],
    "invalid hour, expected an in-range value",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(2021),
        Token::U16(1),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(0),
        Token::I8(26),
        Token::I8(0),
        Token::I8(0),
        Token::TupleEnd,
    ],
    "invalid offset hour, expected an in-range value",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(2021),
        Token::U16(365),
        Token::U8(23),
        Token::U8(59),
        Token::U8(60),
        Token::U32(0),
        Token::I8(0),
        Token::I8(0),
        Token::I8(0),
        Token::TupleEnd,
    ],
    "invalid second, expected an in-range value",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[Token::BorrowedStr("2021-12-31 23:59:60.0 +00:00:00")],
    "second was not in range",
)]
#[case(
    PhantomData::<Readable<UtcOffset>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `UtcOffset`",
)]
#[case(
    PhantomData::<Compact<UtcOffset>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `UtcOffset`",
)]
#[case(
    PhantomData::<Compact<UtcOffset>>,
    &[
        Token::Tuple { len: 3 },
        Token::I8(26),
        Token::I8(0),
        Token::I8(0),
        Token::TupleEnd,
    ],
    "invalid offset hour, expected an in-range value",
)]
#[case(
    PhantomData::<Compact<UtcOffset>>,
    &[
        Token::Tuple { len: 3 },
        Token::I8(5),
        Token::Str("not a number"),
        Token::TupleEnd,
    ],
    "invalid type: string \"not a number\", expected i8",
)]
#[case(
    PhantomData::<Readable<SignedDuration>>,
    &[Token::BorrowedStr("x")],
    r#"invalid value: string "x", expected a decimal point"#,
)]
#[case(
    PhantomData::<Readable<SignedDuration>>,
    &[Token::BorrowedStr("x.0")],
    r#"invalid value: string "x", expected seconds"#,
)]
#[case(
    PhantomData::<Readable<SignedDuration>>,
    &[Token::BorrowedStr("0.x")],
    r#"invalid value: string "x", expected nanoseconds"#,
)]
#[case(
    PhantomData::<Readable<SignedDuration>>,
    &[Token::BorrowedStr("0.123456789x")],
    r#"invalid value: string "123456789x", expected nanoseconds"#,
)]
#[case(
    PhantomData::<Readable<SignedDuration>>,
    &[Token::BorrowedStr("0.")],
    r#"invalid value: string "", expected nanoseconds"#,
)]
#[case(
    PhantomData::<Readable<SignedDuration>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `SignedDuration`",
)]
#[case(
    PhantomData::<Compact<SignedDuration>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `SignedDuration`",
)]
#[case(
    PhantomData::<Compact<SignedDuration>>,
    &[
        Token::Tuple { len: 2 },
        Token::I64(i64::MAX),
        Token::I32(i32::MAX),
        Token::TupleEnd,
    ],
    "invalid value: integer, expected an integer in the range -999999999..=999999999",
)]
#[case(
    PhantomData::<Compact<Weekday>>,
    &[Token::U8(0)],
    "invalid value: integer `0`, expected a value in the range 1..=7",
)]
#[case(
    PhantomData::<Readable<Weekday>>,
    &[Token::BorrowedStr("NotADay")],
    r#"invalid value: string "NotADay", expected a `Weekday`"#,
)]
#[case(
    PhantomData::<Readable<Weekday>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `Weekday`",
)]
#[case(
    PhantomData::<Compact<Weekday>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `Weekday`",
)]
#[case(
    PhantomData::<Compact<Month>>,
    &[Token::U8(0)],
    "invalid value: integer `0`, expected a value in the range 1..=12",
)]
#[case(
    PhantomData::<Readable<Month>>,
    &[Token::BorrowedStr("NotAMonth")],
    r#"invalid value: string "NotAMonth", expected a `Month`"#,
)]
#[case(
    PhantomData::<Readable<Month>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `Month`",
)]
#[case(
    PhantomData::<Compact<Month>>,
    &[Token::Bool(false)],
    "invalid type: boolean `false`, expected a `Month`",
)]
fn deserialize_error<T>(
    #[case] _type: PhantomData<T>,
    #[case] tokens: &[Token],
    #[case] expected: &str,
) where
    T: for<'de> Deserialize<'de>,
{
    assert_de_tokens_error::<T>(tokens, expected);
}

#[rstest]
#[case(
    PhantomData::<Compact<Time>>,
    &[Token::Tuple { len: 4 }, Token::TupleEnd],
    "expected hour",
)]
#[case(
    PhantomData::<Compact<Time>>,
    &[Token::Tuple { len: 4 }, Token::U8(0), Token::TupleEnd],
    "expected minute",
)]
#[case(
    PhantomData::<Compact<Time>>,
    &[
        Token::Tuple { len: 4 },
        Token::U8(0),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected second",
)]
#[case(
    PhantomData::<Compact<Time>>,
    &[
        Token::Tuple { len: 4 },
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected nanosecond",
)]
#[case(
    PhantomData::<Readable<Time>>,
    &[Token::Tuple { len: 4 }, Token::TupleEnd],
    "expected hour",
)]
#[case(
    PhantomData::<Readable<Time>>,
    &[Token::Tuple { len: 4 }, Token::U8(0), Token::TupleEnd],
    "expected minute",
)]
#[case(
    PhantomData::<Readable<Time>>,
    &[
        Token::Tuple { len: 4 },
        Token::U8(0),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected second",
)]
#[case(
    PhantomData::<Readable<Time>>,
    &[
        Token::Tuple { len: 4 },
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected nanosecond",
)]
#[case(
    PhantomData::<Compact<Date>>,
    &[Token::Tuple { len: 2 }, Token::TupleEnd],
    "expected year",
)]
#[case(
    PhantomData::<Compact<Date>>,
    &[Token::Tuple { len: 2 }, Token::I32(9999), Token::TupleEnd],
    "expected day of year",
)]
#[case(
    PhantomData::<Readable<Date>>,
    &[Token::Tuple { len: 2 }, Token::TupleEnd],
    "expected year",
)]
#[case(
    PhantomData::<Readable<Date>>,
    &[Token::Tuple { len: 2 }, Token::I32(9999), Token::TupleEnd],
    "expected day of year",
)]
#[case(
    PhantomData::<Compact<PlainDateTime>>,
    &[Token::Tuple { len: 6 }, Token::TupleEnd],
    "expected year",
)]
#[case(
    PhantomData::<Compact<PlainDateTime>>,
    &[Token::Tuple { len: 6 }, Token::I32(9999), Token::TupleEnd],
    "expected day of year",
)]
#[case(
    PhantomData::<Compact<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(9999),
        Token::U16(365),
        Token::TupleEnd,
    ],
    "expected hour",
)]
#[case(
    PhantomData::<Compact<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(23),
        Token::TupleEnd,
    ],
    "expected minute",
)]
#[case(
    PhantomData::<Compact<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(23),
        Token::U8(58),
        Token::TupleEnd,
    ],
    "expected second",
)]
#[case(
    PhantomData::<Compact<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(23),
        Token::U8(58),
        Token::U8(59),
        Token::TupleEnd,
    ],
    "expected nanosecond",
)]
#[case(
    PhantomData::<Readable<PlainDateTime>>,
    &[Token::Tuple { len: 6 }, Token::TupleEnd],
    "expected year",
)]
#[case(
    PhantomData::<Readable<PlainDateTime>>,
    &[Token::Tuple { len: 6 }, Token::I32(9999), Token::TupleEnd],
    "expected day of year",
)]
#[case(
    PhantomData::<Readable<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(9999),
        Token::U16(365),
        Token::TupleEnd,
    ],
    "expected hour",
)]
#[case(
    PhantomData::<Readable<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(23),
        Token::TupleEnd,
    ],
    "expected minute",
)]
#[case(
    PhantomData::<Readable<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(23),
        Token::U8(58),
        Token::TupleEnd,
    ],
    "expected second",
)]
#[case(
    PhantomData::<Readable<PlainDateTime>>,
    &[
        Token::Tuple { len: 6 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(23),
        Token::U8(58),
        Token::U8(59),
        Token::TupleEnd,
    ],
    "expected nanosecond",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[Token::Tuple { len: 9 }, Token::TupleEnd],
    "expected year",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[Token::Tuple { len: 9 }, Token::I32(9999), Token::TupleEnd],
    "expected day of year",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::TupleEnd,
    ],
    "expected hour",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected minute",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected second",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected nanosecond",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(123_456_789),
        Token::TupleEnd,
    ],
    "expected offset hours",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(123_456_789),
        Token::I8(-23),
        Token::TupleEnd,
    ],
    "expected offset minutes",
)]
#[case(
    PhantomData::<Compact<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(123_456_789),
        Token::I8(-23),
        Token::I8(-58),
        Token::TupleEnd,
    ],
    "expected offset seconds",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[Token::Tuple { len: 9 }, Token::TupleEnd],
    "expected year",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[Token::Tuple { len: 9 }, Token::I32(9999), Token::TupleEnd],
    "expected day of year",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::TupleEnd,
    ],
    "expected hour",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected minute",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected second",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::TupleEnd,
    ],
    "expected nanosecond",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(123_456_789),
        Token::TupleEnd,
    ],
    "expected offset hours",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(123_456_789),
        Token::I8(-23),
        Token::TupleEnd,
    ],
    "expected offset minutes",
)]
#[case(
    PhantomData::<Readable<OffsetDateTime>>,
    &[
        Token::Tuple { len: 9 },
        Token::I32(9999),
        Token::U16(365),
        Token::U8(0),
        Token::U8(0),
        Token::U8(0),
        Token::U32(123_456_789),
        Token::I8(-23),
        Token::I8(-58),
        Token::TupleEnd,
    ],
    "expected offset seconds",
)]
#[case(
    PhantomData::<Compact<UtcOffset>>,
    &[Token::Tuple { len: 0 }, Token::TupleEnd],
    "expected offset hours",
)]
#[case(
    PhantomData::<Readable<UtcOffset>>,
    &[Token::Tuple { len: 0 }, Token::TupleEnd],
    "expected offset hours",
)]
#[case(
    PhantomData::<Compact<SignedDuration>>,
    &[Token::Tuple { len: 2 }, Token::TupleEnd],
    "expected seconds",
)]
#[case(
    PhantomData::<Compact<SignedDuration>>,
    &[
        Token::Tuple { len: 2 },
        Token::I64(i64::MAX),
        Token::TupleEnd,
    ],
    "expected nanoseconds",
)]
#[case(
    PhantomData::<Readable<SignedDuration>>,
    &[Token::Tuple { len: 2 }, Token::TupleEnd],
    "expected seconds",
)]
#[case(
    PhantomData::<Readable<SignedDuration>>,
    &[
        Token::Tuple { len: 2 },
        Token::I64(i64::MAX),
        Token::TupleEnd,
    ],
    "expected nanoseconds",
)]
fn partial_deserialize<T>(
    #[case] _type: PhantomData<T>,
    #[case] tokens: &[Token],
    #[case] error_message: &str,
) where
    T: for<'de> Deserialize<'de>,
{
    assert_de_tokens_error::<T>(tokens, error_message);
}

#[rstest]
#[case(
    offset!(+23).compact(),
    &[Token::Tuple { len: 1 }, Token::I8(23), Token::TupleEnd],
)]
#[case(
    offset!(+23).readable(),
    &[Token::Tuple { len: 1 }, Token::I8(23), Token::TupleEnd],
)]
#[case(
    offset!(+23:58).compact(),
    &[Token::Tuple { len: 2 }, Token::I8(23), Token::I8(58), Token::TupleEnd],
)]
#[case(
    offset!(+23:58).readable(),
    &[Token::Tuple { len: 2 }, Token::I8(23), Token::I8(58), Token::TupleEnd],
)]
fn deserialize_success<T>(#[case] value: T, #[case] tokens: &[Token])
where
    T: Debug + PartialEq + for<'de> Deserialize<'de>,
{
    assert_de_tokens::<T>(&value, tokens);
}
