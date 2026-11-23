use std::fmt::Debug;
use std::marker::PhantomData;

use rstest::rstest;
use serde::{Deserialize, Serialize};
use serde_test2::Configure;
use time::Month::*;
use time::Weekday::*;
use time::macros::{date, datetime, time};
use time::{Date, Month, OffsetDateTime, PlainDateTime, SignedDuration, Time, Weekday};

enum Format {
    Compact,
    Readable,
}

use Format::*;

#[rstest]
#[case(Monday.compact(), "1")]
#[case(Monday.readable(), r#""Monday""#)]
#[case(March.compact(), "3")]
#[case(March.readable(), r#""March""#)]
#[case(time!(12:40:20).compact(), "[12,40,20,0]")]
#[case(time!(12:40:20).readable(), r#""12:40:20.0""#)]
#[case(datetime!(2022-05-20 12:40:20).compact(), "[2022,140,12,40,20,0]")]
#[case(datetime!(2022-05-20 12:40:20).readable(), r#""2022-05-20 12:40:20.0""#)]
#[case(datetime!(2022-05-20 12:40:20 UTC).compact(), "[2022,140,12,40,20,0,0,0,0]")]
#[case(datetime!(2022-05-20 12:40:20 UTC).readable(), r#""2022-05-20 12:40:20.0 +00:00:00""#)]
#[case(SignedDuration::new(50, 0).compact(), "[50,0]")]
#[case(SignedDuration::new(50, 0).readable(), r#""50.000000000""#)]
#[case(date!(2022-04-05).compact(), "[2022,95]")]
#[case(date!(2022-04-05).readable(), r#""2022-04-05""#)]
fn serialize<T>(#[case] value: T, #[case] expected: &str)
where
    T: Serialize,
{
    let mut buf: Vec<u8> = Vec::new();
    let mut ser = serde_json::Serializer::new(&mut buf);
    value.serialize(&mut ser).expect("serialization failed");
    assert_eq!(String::from_utf8(buf).as_deref(), Ok(expected));
}

#[rstest]
#[case(PhantomData::<Weekday>, Compact, "1", Monday)]
#[case(PhantomData::<Weekday>, Readable, "1", Monday)]
#[case(PhantomData::<Weekday>, Readable, r#""Monday""#, Monday)]
#[case(PhantomData::<Month>, Compact, "3", March)]
#[case(PhantomData::<Month>, Readable, "3", March)]
#[case(PhantomData::<Month>, Readable, r#""March""#, March)]
#[case(PhantomData::<Time>, Compact, "[12,40,20,0]", time!(12:40:20))]
#[case(PhantomData::<Time>, Readable, "[12,40,20,0]", time!(12:40:20))]
#[case(PhantomData::<Time>, Readable, r#""12:40:20.0""#, time!(12:40:20))]
#[case(
    PhantomData::<PlainDateTime>,
    Compact,
    "[2022,140,12,40,20,0]",
    datetime!(2022-05-20 12:40:20),
)]
#[case(
    PhantomData::<PlainDateTime>,
    Readable,
    "[2022,140,12,40,20,0]",
    datetime!(2022-05-20 12:40:20),
)]
#[case(
    PhantomData::<PlainDateTime>,
    Readable,
    r#""2022-05-20 12:40:20.0""#,
    datetime!(2022-05-20 12:40:20),
)]
#[case(
    PhantomData::<OffsetDateTime>,
    Compact,
    "[2022,140,12,40,20,0,0,0,0]",
    datetime!(2022-05-20 12:40:20 UTC),
)]
#[case(
    PhantomData::<OffsetDateTime>,
    Readable,
    "[2022,140,12,40,20,0,0,0,0]",
    datetime!(2022-05-20 12:40:20 UTC),
)]
#[case(
    PhantomData::<OffsetDateTime>,
    Readable,
    r#""2022-05-20 12:40:20.0 +00:00:00""#,
    datetime!(2022-05-20 12:40:20 UTC),
)]
#[case(PhantomData::<SignedDuration>, Compact, "[50,0]", SignedDuration::new(50, 0))]
#[case(PhantomData::<SignedDuration>, Readable, "[50,0]", SignedDuration::new(50, 0))]
#[case(PhantomData::<SignedDuration>, Readable, r#""50.000000000""#, SignedDuration::new(50, 0))]
#[case(
    PhantomData::<SignedDuration>,
    Readable,
    r#""0.1""#,
    SignedDuration::new(0, 100_000_000),
)]
#[case(
    PhantomData::<SignedDuration>,
    Readable,
    r#""0.123""#,
    SignedDuration::new(0, 123_000_000),
)]
#[case(
    PhantomData::<SignedDuration>,
    Readable,
    r#""-0.1""#,
    SignedDuration::new(0, -100_000_000),
)]
#[case(PhantomData::<Date>, Compact, "[2022,95]", date!(2022-04-05))]
#[case(PhantomData::<Date>, Readable, "[2022,95]", date!(2022-04-05))]
#[case(PhantomData::<Date>, Readable, r#""2022-04-05""#, date!(2022-04-05))]
fn deserialize<T>(
    #[case] _type: PhantomData<T>,
    #[case] format: Format,
    #[case] value: &str,
    #[case] expected: T,
) where
    T: Debug + PartialEq + for<'de> Deserialize<'de>,
{
    let mut de = serde_json::Deserializer::from_str(value);
    let value = match format {
        Compact => T::deserialize((&mut de).compact()),
        Readable => T::deserialize((&mut de).readable()),
    }
    .expect("deserialization failed");
    assert_eq!(value, expected);
}
