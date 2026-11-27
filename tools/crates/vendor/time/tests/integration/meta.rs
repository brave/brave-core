// Prefer runtime checks if possible, as otherwise tests can't be run at all if something is
// changed.

use std::borrow::Borrow;
use std::error::Error as StdError;
use std::fmt::{Debug, Display};
use std::hash::Hash;
use std::iter::Sum;
use std::marker::PhantomData;
use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Neg, Sub, SubAssign};
use std::panic::{RefUnwindSafe, UnwindSafe};
use std::time::{Duration as StdDuration, Instant as StdInstant, SystemTime};

use quickcheck::Arbitrary;
use rand08::distributions::{Distribution as DistributionRand08, Standard as StandardRand08};
use rand09::distr::{Distribution as DistributionRand09, StandardUniform as StandardUniformRand09};
use rstest::rstest;
use serde::{Deserialize, Serialize};
#[expect(deprecated)]
use time::Instant;
use time::format_description::well_known::iso8601;
use time::format_description::{BorrowedFormatItem, Component, modifier, well_known};
use time::formatting::Formattable;
use time::parsing::{Parsable, Parsed};
use time::{
    Date, Error, Month, OffsetDateTime, PlainDateTime, SignedDuration, Time, UtcDateTime,
    UtcOffset, Weekday, error, ext,
};

#[rstest]
#[case(PhantomData::<Date>, 4)]
#[case(PhantomData::<SignedDuration>, 8)]
#[case(PhantomData::<OffsetDateTime>, 4)]
#[case(PhantomData::<PlainDateTime>, 4)]
#[case(PhantomData::<UtcDateTime>, 4)]
#[case(PhantomData::<Time>, 4)]
#[case(PhantomData::<UtcOffset>, 1)]
#[case(PhantomData::<error::ComponentRange>, 8)]
#[case(PhantomData::<error::ConversionRange>, 1)]
#[case(PhantomData::<error::DifferentVariant>, 1)]
#[case(PhantomData::<error::IndeterminateOffset>, 1)]
#[case(PhantomData::<modifier::Day>, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::Hour>, 1)]
#[case(PhantomData::<modifier::Minute>, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::Month>, 1)]
#[case(PhantomData::<modifier::OffsetHour>, 1)]
#[case(PhantomData::<modifier::OffsetMinute>, 1)]
#[case(PhantomData::<modifier::OffsetSecond>, 1)]
#[case(PhantomData::<modifier::Ordinal>, 1)]
#[case(PhantomData::<modifier::Period>, 1)]
#[case(PhantomData::<modifier::Second>, 1)]
#[case(PhantomData::<modifier::Subsecond>, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::WeekNumber>, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::Weekday>, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::Year>, 1)]
#[case(PhantomData::<well_known::Rfc2822>, 1)]
#[case(PhantomData::<well_known::Rfc3339>, 1)]
#[case(PhantomData::<well_known::Iso8601<{ iso8601::Config::DEFAULT.encode() }>>, 1)]
#[case(PhantomData::<iso8601::Config>, 1)]
#[case(PhantomData::<iso8601::DateKind>, 1)]
#[case(PhantomData::<iso8601::FormattedComponents>, 1)]
#[case(PhantomData::<iso8601::OffsetPrecision>, 1)]
#[case(PhantomData::<iso8601::TimePrecision>, 1)]
#[case(PhantomData::<Parsed>, align_of::<u128>())]
#[case(PhantomData::<Month>, 1)]
#[case(PhantomData::<Weekday>, 1)]
#[case(PhantomData::<Error>, 8)]
#[case(PhantomData::<error::Format>, 8)]
#[case(PhantomData::<error::InvalidFormatDescription>, 8)]
#[case(PhantomData::<error::Parse>, 8)]
#[case(PhantomData::<error::ParseFromDescription>, 8)]
#[case(PhantomData::<error::TryFromParsed>, 8)]
#[case(PhantomData::<Component>, 2)]
#[case(PhantomData::<BorrowedFormatItem<'_>>, 8)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::MonthRepr>, 1)]
#[case(PhantomData::<modifier::Padding>, 1)]
#[case(PhantomData::<modifier::SubsecondDigits>, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::WeekNumberRepr>, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::WeekdayRepr>, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::YearRepr>, 1)]
fn alignment<T>(#[case] _type: PhantomData<T>, #[case] expected: usize) {
    assert_eq!(
        align_of::<T>(),
        expected,
        "alignment of `{}` was {expected}",
        std::any::type_name::<T>()
    );
}

#[rstest]
#[case(PhantomData::<Date>, 4, 4)]
#[case(PhantomData::<SignedDuration>, 16, 16)]
#[case(PhantomData::<OffsetDateTime>, 16, 16)]
#[case(PhantomData::<PlainDateTime>, 12, 12)]
#[case(PhantomData::<UtcDateTime>, 12, 12)]
#[case(PhantomData::<Time>, 8, 8)]
#[case(PhantomData::<UtcOffset>, 3, 4)]
#[case(PhantomData::<error::ComponentRange>, 24, 24)]
#[case(PhantomData::<error::ConversionRange>, 0, 1)]
#[case(PhantomData::<error::DifferentVariant>, 0, 1)]
#[case(PhantomData::<error::IndeterminateOffset>, 0, 1)]
#[case(PhantomData::<modifier::Day>, 1, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::Hour>, 2, 2)]
#[case(PhantomData::<modifier::Minute>, 1, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::Month>, 3, 3)]
#[case(PhantomData::<modifier::OffsetHour>, 2, 2)]
#[case(PhantomData::<modifier::OffsetMinute>, 1, 1)]
#[case(PhantomData::<modifier::OffsetSecond>, 1, 1)]
#[case(PhantomData::<modifier::Ordinal>, 1, 1)]
#[case(PhantomData::<modifier::Period>, 2, 2)]
#[case(PhantomData::<modifier::Second>, 1, 1)]
#[case(PhantomData::<modifier::Subsecond>, 1, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::WeekNumber>, 2, 2)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::Weekday>, 3, 3)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::Year>, 5, 5)]
#[case(PhantomData::<well_known::Rfc2822>, 0, 1)]
#[case(PhantomData::<well_known::Rfc3339>, 0, 1)]
#[case(PhantomData::<well_known::Iso8601<{ iso8601::Config::DEFAULT.encode() }>>, 0, 1)]
#[case(PhantomData::<iso8601::Config>, 7, 7)]
#[case(PhantomData::<iso8601::DateKind>, 1, 1)]
#[case(PhantomData::<iso8601::FormattedComponents>, 1, 1)]
#[case(PhantomData::<iso8601::OffsetPrecision>, 1, 1)]
#[case(PhantomData::<iso8601::TimePrecision>, 2, 2)]
#[case(PhantomData::<Parsed>, 64, 64)]
#[case(PhantomData::<Month>, 1, 1)]
#[case(PhantomData::<Weekday>, 1, 1)]
#[case(PhantomData::<Error>, 48, 48)]
#[case(PhantomData::<error::Format>, 24, 24)]
#[case(PhantomData::<error::InvalidFormatDescription>, 48, 48)]
#[case(PhantomData::<error::Parse>, 32, 32)]
#[case(PhantomData::<error::ParseFromDescription>, 24, 24)]
#[case(PhantomData::<error::TryFromParsed>, 24, 24)]
#[case(PhantomData::<Component>, 6, 6)]
#[case(PhantomData::<BorrowedFormatItem<'_>>, 24, 24)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::MonthRepr>, 1, 1)]
#[case(PhantomData::<modifier::Padding>, 1, 1)]
#[case(PhantomData::<modifier::SubsecondDigits>, 1, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::WeekNumberRepr>, 1, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::WeekdayRepr>, 1, 1)]
#[expect(deprecated)]
#[case(PhantomData::<modifier::YearRepr>, 1, 1)]
fn size<T>(
    #[case] _type: PhantomData<T>,
    #[case] expected_size: usize,
    #[case] expected_opt_size: usize,
) {
    assert!(
        size_of::<T>() <= expected_size,
        "size of `{}` used to be {expected_size}, but is now {}",
        std::any::type_name::<T>(),
        size_of::<T>(),
    );
    assert!(
        size_of::<Option<T>>() <= expected_opt_size,
        "size of `Option<{}>` used to be {expected_opt_size}, but is now {}",
        std::any::type_name::<T>(),
        size_of::<Option<T>>(),
    );
}

macro_rules! assert_obj_safe {
    ($($xs:path),+ $(,)?) => {
        $(const _: Option<&dyn $xs> = None;)+
    };
}

assert_obj_safe!(ext::NumericalDuration);
assert_obj_safe!(ext::NumericalStdDuration);
// `Parsable` is not object safe.
// `Formattable` is not object safe.

macro_rules! assert_impl {
    ($(#[$meta:meta])* $($(@$lifetimes:lifetime),+ ;)? $type:ty: $($trait:path),+ $(,)?) => {
        $(#[$meta])*
        const _: fn() = || {
            fn assert_impl_all<$($($lifetimes,)+)? T: ?Sized $(+ $trait)+>() {}
            assert_impl_all::<$type>();
        };
    };
}

assert_impl! { @'a; Date:
    Add<SignedDuration, Output = Date>,
    Add<StdDuration, Output = Date>,
    AddAssign<SignedDuration>,
    AddAssign<StdDuration>,
    Arbitrary,
    Clone,
    Debug,
    Deserialize<'a>,
    Display,
    Hash,
    Ord,
    PartialEq<Date>,
    PartialOrd<Date>,
    Serialize,
    Sub<Date, Output = SignedDuration>,
    Sub<SignedDuration, Output = Date>,
    Sub<StdDuration, Output = Date>,
    SubAssign<SignedDuration>,
    SubAssign<StdDuration>,
    TryFrom<Parsed, Error = error::TryFromParsed>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; SignedDuration:
    Add<SignedDuration, Output = SignedDuration>,
    Add<StdDuration, Output = SignedDuration>,
    AddAssign<SignedDuration>,
    AddAssign<StdDuration>,
    Arbitrary,
    Clone,
    Debug,
    Default,
    Deserialize<'a>,
    Div<SignedDuration, Output = f64>,
    Div<StdDuration, Output = f64>,
    Div<f32, Output = SignedDuration>,
    Div<f64, Output = SignedDuration>,
    Div<i16, Output = SignedDuration>,
    Div<i32, Output = SignedDuration>,
    Div<i8, Output = SignedDuration>,
    Div<u16, Output = SignedDuration>,
    Div<u32, Output = SignedDuration>,
    Div<u8, Output = SignedDuration>,
    DivAssign<f32>,
    DivAssign<f64>,
    DivAssign<i16>,
    DivAssign<i32>,
    DivAssign<i8>,
    DivAssign<u16>,
    DivAssign<u32>,
    DivAssign<u8>,
    Hash,
    Mul<f32, Output = SignedDuration>,
    Mul<f64, Output = SignedDuration>,
    Mul<i16, Output = SignedDuration>,
    Mul<i32, Output = SignedDuration>,
    Mul<i8, Output = SignedDuration>,
    Mul<u16, Output = SignedDuration>,
    Mul<u32, Output = SignedDuration>,
    Mul<u8, Output = SignedDuration>,
    MulAssign<f32>,
    MulAssign<f64>,
    MulAssign<i16>,
    MulAssign<i32>,
    MulAssign<i8>,
    MulAssign<u16>,
    MulAssign<u32>,
    MulAssign<u8>,
    Neg<Output = SignedDuration>,
    Ord,
    PartialEq<SignedDuration>,
    PartialEq<StdDuration>,
    PartialOrd<SignedDuration>,
    PartialOrd<StdDuration>,
    Serialize,
    Sub<SignedDuration, Output = SignedDuration>,
    Sub<StdDuration, Output = SignedDuration>,
    SubAssign<SignedDuration>,
    SubAssign<StdDuration>,
    Sum<&'a SignedDuration>,
    Sum<SignedDuration>,
    TryFrom<StdDuration, Error = error::ConversionRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { #[expect(deprecated)] Instant:
    Add<SignedDuration, Output = Instant>,
    Add<StdDuration, Output = Instant>,
    AddAssign<SignedDuration>,
    AddAssign<StdDuration>,
    AsRef<StdInstant>,
    Borrow<StdInstant>,
    Clone,
    Debug,
    From<StdInstant>,
    Hash,
    Ord,
    PartialEq<Instant>,
    PartialEq<StdInstant>,
    PartialOrd<Instant>,
    PartialOrd<StdInstant>,
    Sub<SignedDuration, Output = Instant>,
    Sub<StdDuration, Output = Instant>,
    Sub<Instant, Output = SignedDuration>,
    Sub<StdInstant, Output = SignedDuration>,
    SubAssign<SignedDuration>,
    SubAssign<StdDuration>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; OffsetDateTime:
    Add<SignedDuration, Output = OffsetDateTime>,
    Add<StdDuration, Output = OffsetDateTime>,
    AddAssign<SignedDuration>,
    AddAssign<StdDuration>,
    Arbitrary,
    Clone,
    Debug,
    Deserialize<'a>,
    Display,
    From<SystemTime>,
    Hash,
    Ord,
    PartialEq<OffsetDateTime>,
    PartialEq<SystemTime>,
    PartialOrd<OffsetDateTime>,
    PartialOrd<SystemTime>,
    Serialize,
    Sub<OffsetDateTime, Output = SignedDuration>,
    Sub<SystemTime, Output = SignedDuration>,
    Sub<SignedDuration, Output = OffsetDateTime>,
    Sub<StdDuration, Output = OffsetDateTime>,
    SubAssign<SignedDuration>,
    SubAssign<StdDuration>,
    TryFrom<Parsed, Error = error::TryFromParsed>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; PlainDateTime:
    Add<SignedDuration, Output = PlainDateTime>,
    Add<StdDuration, Output = PlainDateTime>,
    AddAssign<SignedDuration>,
    AddAssign<StdDuration>,
    Arbitrary,
    Clone,
    Debug,
    Deserialize<'a>,
    Display,
    Hash,
    Ord,
    PartialEq<PlainDateTime>,
    PartialOrd<PlainDateTime>,
    Serialize,
    Sub<SignedDuration, Output = PlainDateTime>,
    Sub<StdDuration, Output = PlainDateTime>,
    Sub<PlainDateTime>,
    SubAssign<SignedDuration>,
    SubAssign<StdDuration>,
    TryFrom<Parsed, Error = error::TryFromParsed>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; UtcDateTime:
    Add<SignedDuration, Output = UtcDateTime>,
    Add<StdDuration, Output = UtcDateTime>,
    AddAssign<SignedDuration>,
    AddAssign<StdDuration>,
    Arbitrary,
    Clone,
    Debug,
    Deserialize<'a>,
    Display,
    Hash,
    Ord,
    PartialEq<UtcDateTime>,
    PartialEq<OffsetDateTime>,
    PartialEq<SystemTime>,
    PartialOrd<UtcDateTime>,
    PartialOrd<OffsetDateTime>,
    PartialOrd<SystemTime>,
    Serialize,
    Sub<SignedDuration, Output = UtcDateTime>,
    Sub<StdDuration, Output = UtcDateTime>,
    Sub<UtcDateTime>,
    Sub<OffsetDateTime>,
    SubAssign<SignedDuration>,
    SubAssign<StdDuration>,
    TryFrom<Parsed, Error = error::TryFromParsed>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; Time:
    Add<SignedDuration, Output = Time>,
    Add<StdDuration, Output = Time>,
    AddAssign<SignedDuration>,
    AddAssign<StdDuration>,
    Arbitrary,
    Clone,
    Debug,
    Deserialize<'a>,
    Display,
    Hash,
    Ord,
    PartialEq<Time>,
    PartialOrd<Time>,
    Serialize,
    Sub<SignedDuration, Output = Time>,
    Sub<StdDuration, Output = Time>,
    Sub<Time, Output = SignedDuration>,
    SubAssign<SignedDuration>,
    SubAssign<StdDuration>,
    TryFrom<Parsed, Error = error::TryFromParsed>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; UtcOffset:
    Arbitrary,
    Clone,
    Debug,
    Deserialize<'a>,
    Display,
    Hash,
    Neg,
    Ord,
    PartialEq<UtcOffset>,
    PartialOrd<UtcOffset>,
    Serialize,
    TryFrom<Parsed, Error = error::TryFromParsed>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { error::ComponentRange:
    Clone,
    Debug,
    Display,
    StdError,
    serde::de::Expected,
    Hash,
    PartialEq<error::ComponentRange>,
    TryFrom<Error, Error = error::DifferentVariant>,
    TryFrom<error::TryFromParsed, Error = error::DifferentVariant>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { error::ConversionRange:
    Clone,
    Debug,
    Display,
    StdError,
    PartialEq<error::ConversionRange>,
    TryFrom<Error, Error = error::DifferentVariant>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { error::DifferentVariant:
    Clone,
    Debug,
    Display,
    StdError,
    PartialEq<error::DifferentVariant>,
    TryFrom<Error, Error = error::DifferentVariant>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { error::IndeterminateOffset:
    Clone,
    Debug,
    Display,
    StdError,
    PartialEq<error::IndeterminateOffset>,
    TryFrom<Error, Error = error::DifferentVariant>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::Day:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Day>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::Hour12:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Hour12>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::Hour24:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Hour24>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { #[expect(deprecated)] modifier::Hour:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Hour>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::Minute:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Minute>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::MonthNumerical:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::MonthNumerical>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::MonthShort:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::MonthShort>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::MonthLong:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::MonthLong>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { #[expect(deprecated)] modifier::Month:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Month>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::OffsetHour:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::OffsetHour>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::OffsetMinute:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::OffsetMinute>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::OffsetSecond:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::OffsetSecond>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::Ordinal:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Ordinal>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::Period:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Period>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::Second:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Second>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::Subsecond:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Subsecond>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::WeekNumberIso:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekNumberIso>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::WeekNumberSunday:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekNumberSunday>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::WeekNumberMonday:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekNumberMonday>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { #[expect(deprecated)] modifier::WeekNumber:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekNumber>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::WeekdayShort:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekdayShort>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::WeekdayLong:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekdayLong>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::WeekdaySunday:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekdaySunday>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::WeekdayMonday:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekdayMonday>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { #[expect(deprecated)] modifier::Weekday:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Weekday>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::CalendarYearFullExtendedRange:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::CalendarYearFullExtendedRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::CalendarYearFullStandardRange:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::CalendarYearFullStandardRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::IsoYearFullExtendedRange:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::IsoYearFullExtendedRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::IsoYearFullStandardRange:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::IsoYearFullStandardRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::CalendarYearCenturyExtendedRange:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::CalendarYearCenturyExtendedRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::CalendarYearCenturyStandardRange:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::CalendarYearCenturyStandardRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::IsoYearCenturyExtendedRange:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::IsoYearCenturyExtendedRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::IsoYearCenturyStandardRange:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::IsoYearCenturyStandardRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::CalendarYearLastTwo:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::CalendarYearLastTwo>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::IsoYearLastTwo:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::IsoYearLastTwo>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { #[expect(deprecated)] modifier::Year:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Year>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { well_known::Rfc2822:
    Clone,
    Debug,
    PartialEq<well_known::Rfc2822>,
    Copy,
    Eq,
    Formattable,
    Parsable,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { well_known::Rfc3339:
    Clone,
    Debug,
    PartialEq<well_known::Rfc3339>,
    Copy,
    Eq,
    Formattable,
    Parsable,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { well_known::Iso8601::<{ iso8601::Config::DEFAULT.encode() }>:
    Clone,
    Debug,
    PartialEq<well_known::Iso8601<{ iso8601::Config::DEFAULT.encode() }>>,
    Copy,
    Eq,
    Formattable,
    Parsable,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { iso8601::Config:
    Debug,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { iso8601::DateKind:
    Clone,
    Debug,
    PartialEq<iso8601::DateKind>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { iso8601::FormattedComponents:
    Clone,
    Debug,
    PartialEq<iso8601::FormattedComponents>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { iso8601::OffsetPrecision:
    Clone,
    Debug,
    PartialEq<iso8601::OffsetPrecision>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { iso8601::TimePrecision:
    Clone,
    Debug,
    PartialEq<iso8601::TimePrecision>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { Parsed:
    Clone,
    Debug,
    Copy,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; Month:
    Arbitrary,
    Clone,
    Debug,
    Deserialize<'a>,
    Display,
    Hash,
    PartialEq<Month>,
    TryFrom<u8, Error = error::ComponentRange>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; Weekday:
    Arbitrary,
    Clone,
    Debug,
    Deserialize<'a>,
    Display,
    Hash,
    PartialEq<Weekday>,
    Serialize,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { Error:
    Debug,
    Display,
    StdError,
    From<error::ComponentRange>,
    From<error::ConversionRange>,
    From<error::DifferentVariant>,
    From<error::Format>,
    From<error::IndeterminateOffset>,
    From<error::InvalidFormatDescription>,
    From<error::Parse>,
    From<error::ParseFromDescription>,
    From<error::TryFromParsed>,
    Send,
    Sync,
    Unpin,
}
assert_impl! { error::Format:
    Debug,
    Display,
    StdError,
    From<std::io::Error>,
    TryFrom<Error, Error = error::DifferentVariant>,
    Send,
    Sync,
    Unpin,
}
assert_impl! { error::InvalidFormatDescription:
    Clone,
    Debug,
    Display,
    StdError,
    PartialEq<error::InvalidFormatDescription>,
    TryFrom<Error, Error = error::DifferentVariant>,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { error::Parse:
    Clone,
    Debug,
    Display,
    StdError,
    From<error::ParseFromDescription>,
    From<error::TryFromParsed>,
    PartialEq<error::Parse>,
    TryFrom<Error, Error = error::DifferentVariant>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { error::ParseFromDescription:
    Clone,
    Debug,
    Display,
    StdError,
    PartialEq<error::ParseFromDescription>,
    TryFrom<Error, Error = error::DifferentVariant>,
    TryFrom<error::Parse, Error = error::DifferentVariant>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { error::TryFromParsed:
    Clone,
    Debug,
    Display,
    StdError,
    From<error::ComponentRange>,
    PartialEq<error::TryFromParsed>,
    TryFrom<Error, Error = error::DifferentVariant>,
    TryFrom<error::Parse, Error = error::DifferentVariant>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; Component:
    Clone,
    Debug,
    PartialEq<Component>,
    PartialEq<BorrowedFormatItem<'a>>,
    TryFrom<BorrowedFormatItem<'a>, Error = error::DifferentVariant>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; BorrowedFormatItem<'_>:
    Clone,
    Debug,
    From<&'a [BorrowedFormatItem<'a>]>,
    From<Component>,
    PartialEq<&'a [BorrowedFormatItem<'a>]>,
    PartialEq<Component>,
    PartialEq<BorrowedFormatItem<'a>>,
    Eq,
    Formattable,
    Parsable,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { @'a; &[BorrowedFormatItem<'_>]:
    PartialEq<BorrowedFormatItem<'a>>,
    TryFrom<BorrowedFormatItem<'a>, Error = error::DifferentVariant>,
}
assert_impl! { #[expect(deprecated)] modifier::MonthRepr:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::MonthRepr>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::Padding:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::Padding>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { modifier::SubsecondDigits:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::SubsecondDigits>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { #[expect(deprecated)] modifier::WeekNumberRepr:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekNumberRepr>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { #[expect(deprecated)] modifier::WeekdayRepr:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::WeekdayRepr>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { #[expect(deprecated)] modifier::YearRepr:
    Clone,
    Debug,
    Default,
    PartialEq<modifier::YearRepr>,
    Copy,
    Eq,
    RefUnwindSafe,
    Send,
    Sync,
    Unpin,
    UnwindSafe,
}
assert_impl! { StandardRand08:
    DistributionRand08<Date>,
    DistributionRand08<SignedDuration>,
    DistributionRand08<OffsetDateTime>,
    DistributionRand08<UtcDateTime>,
    DistributionRand08<PlainDateTime>,
    DistributionRand08<Time>,
    DistributionRand08<UtcOffset>,
    DistributionRand08<Month>,
    DistributionRand08<Weekday>,
}
assert_impl! { StandardUniformRand09:
    DistributionRand09<Date>,
    DistributionRand09<SignedDuration>,
    DistributionRand09<OffsetDateTime>,
    DistributionRand09<UtcDateTime>,
    DistributionRand09<PlainDateTime>,
    DistributionRand09<Time>,
    DistributionRand09<UtcOffset>,
    DistributionRand09<Month>,
    DistributionRand09<Weekday>,
}
assert_impl! { StdDuration:
    Add<SignedDuration, Output = SignedDuration>,
    AddAssign<SignedDuration>,
    Div<SignedDuration, Output = f64>,
    PartialEq<SignedDuration>,
    PartialOrd<SignedDuration>,
    Sub<SignedDuration, Output = SignedDuration>,
    SubAssign<SignedDuration>,
    TryFrom<SignedDuration>,
}
assert_impl! { #[expect(deprecated)] StdInstant:
    Add<SignedDuration, Output = StdInstant>,
    AddAssign<SignedDuration>,
    Sub<SignedDuration, Output = StdInstant>,
    SubAssign<SignedDuration>,
    PartialEq<Instant>,
    PartialOrd<Instant>,
    From<Instant>,
    Sub<Instant>,
}
assert_impl! { SystemTime:
    Add<SignedDuration, Output = SystemTime>,
    AddAssign<SignedDuration>,
    Sub<SignedDuration, Output = SystemTime>,
    SubAssign<SignedDuration>,
    From<OffsetDateTime>,
    PartialEq<OffsetDateTime>,
    PartialOrd<OffsetDateTime>,
    Sub<OffsetDateTime>,
}
assert_impl! { i8:
    Mul<SignedDuration>,
}
assert_impl! { i16:
    Mul<SignedDuration>,
}
assert_impl! { i32:
    Mul<SignedDuration>,
}
assert_impl! { u8:
    Mul<SignedDuration>,
    From<Month>,
}
assert_impl! { u16:
    Mul<SignedDuration>,
}
assert_impl! { u32:
    Mul<SignedDuration>,
}
assert_impl! { f32:
    Mul<SignedDuration>,
}
assert_impl! { f64:
    Mul<SignedDuration>,
}
