/*!
Defines boundary types and their corresponding error values.

The main trait in this module is [`Bounds`]. It is implemented by each of
several boundary types describing a range of contiguous values used by Jiff.
For example, [`Year`] corresponds to the range of support calendar year values.

Each range type has concrete methods defined on it that simply forward to its
corresponding method on `Bounds`. For example, [`Year::check`] forwards to the
default implementation of [`Bounds::check`]. These concrete wrappers exist to
make calling the routines more ergonomic. For example, they don't require that
the `Bounds` trait be in scope. Additionally, a [`Year::checkc`] concrete
method is provide for use in `const` contexts. This is not available on the
`Bounds` trait because generics are not yet supported (as of 2026-02-13) in
a `const` context.
*/

use crate::constants as c;

/// This macro writes out the boiler plate to define a boundary type.
///
/// Specifically, it implements the `Bounds` trait and provides a few
/// concrete methods. The concrete methods are mostly wrappers around
/// the generic trait methods. They are provided so that callers don't
/// have to import the `Bounds` trait to use them.
macro_rules! define_bounds {
    ($(
        $(#[$attr:meta])*
        (
            // The name of the boundary type.
            $name:ident,
            // The underlying primitive type. This is usually, but not always,
            // the smallest signed primitive integer type that can represent
            // both the minimum and maximum boundary values.
            $ty:ident,
            // A short human readable description that appears in error
            // messages when the boundaries of this type are violated.
            $what:expr,
            // The minimum value.
            $min:expr,
            // The maximum value.
            $max:expr $(,)?
        )
    ),* $(,)?) => {
        $(
            $(#[$attr])*
            #[allow(missing_debug_implementations)]
            #[allow(missing_docs)]
            #[derive(Eq, PartialEq)]
            pub struct $name(());

            impl Bounds for $name {
                const WHAT: &'static str = $what;
                const MIN: Self::Primitive = $min;
                const MAX: Self::Primitive = $max;
                type Primitive = $ty;
                type Error = BoundsError;

                #[cold]
                #[inline(never)]
                fn error() -> BoundsError {
                    Self::error()
                }
            }

            #[allow(dead_code)]
            #[allow(missing_docs)]
            impl $name {
                pub const MIN: $ty = <$name as Bounds>::MIN;
                pub const MAX: $ty = <$name as Bounds>::MAX;
                pub const LEN: i128 = Self::MAX as i128 - Self::MIN as i128 + 1;

                #[cold]
                pub const fn error() -> BoundsError {
                    BoundsError {
                        kind: BoundsErrorKind::$name(RawBoundsError::new()),
                    }
                }

                #[inline(always)]
                pub fn check(n: impl Into<i64>) -> Result<$ty, BoundsError> {
                    <$name as Bounds>::check(n)
                }

                #[inline(always)]
                pub const fn checkc(n: i64) -> Result<$ty, BoundsError> {
                    match self::const_check::$ty(n) {
                        Ok(n) => Ok(n),
                        Err(err) => Err(BoundsError {
                            kind: BoundsErrorKind::$name(err),
                        }),
                    }
                }

                #[inline(always)]
                pub const fn checked_add(n1: $ty, n2: $ty) -> Result<$ty, BoundsError> {
                    match self::const_checked_add::$ty(n1, n2) {
                        Ok(n) => Ok(n),
                        Err(err) => Err(BoundsError {
                            kind: BoundsErrorKind::$name(err),
                        }),
                    }
                }

                #[inline(always)]
                pub fn checked_mul(n1: $ty, n2: $ty) -> Result<$ty, BoundsError> {
                    <$name as Bounds>::checked_mul(n1, n2)
                }

                #[cfg(test)]
                pub(crate) fn arbitrary(g: &mut quickcheck::Gen) -> $ty {
                    use quickcheck::Arbitrary;

                    let mut n: $ty = <$ty>::arbitrary(g);
                    n = n.wrapping_rem_euclid(Self::LEN as $ty);
                    n += Self::MIN;
                    n
                }
            }
        )*

        /// An error that indicates a value is out of its intended range.
        #[derive(Clone, Copy, Debug, Eq, PartialEq)]
        #[cfg_attr(feature = "defmt", derive(defmt::Format))]
        enum BoundsErrorKind {
            $($name(RawBoundsError<$name>),)*
        }

        impl core::fmt::Display for BoundsErrorKind {
            fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
                match *self {
                    $(BoundsErrorKind::$name(ref err) => err.fmt(f),)*
                }
            }
        }
    }
}

define_bounds! {
    /// The supported range of nanoseconds in a single civil day.
    (
        CivilDayNanosecond,
        i64,
        "nanoseconds (in one civil day)",
        0,
        c::NANOS_PER_CIVIL_DAY - 1,
    ),
    /// The supported range of seconds in a single civil day.
    (
        CivilDaySecond,
        i32,
        "seconds (in one civil day)",
        0,
        c::SECS_PER_CIVIL_DAY_32 - 1,
    ),
    /// The supported range of "day of month."
    ///
    /// While the maximum value is 31, the actual maximum depends on the
    /// specific month. The smallest possible maximum for any month is `28`
    /// for February.
    (Day, i8, "day", 1, 31),
    /// The supported range of "day of year."
    ///
    /// This includes Feb 29 in leap years. For non-leap years, `366` is
    /// invalid. The value `1` always corresponds to Jan 1.
    (DayOfYear, i16, "day-of-year", 1, 366),
    /// The supported range of "day of year," but never including Feb 29.
    ///
    /// This means that the valid range for all years is `1..=365`. And there
    /// is no way to refer to Feb 29 in this scheme.
    (DayOfYearNoLeap, i16, "day-of-year (skipping Feb 29)", 1, 365),
    /// The range of seconds for the possible differences between any two pairs
    /// of `(timestamp, offset)`.
    ///
    /// All of our span types (except for years and months, since they have
    /// variable length even in civil datetimes) are defined in terms of this
    /// constant. The way it's defined is a little odd, so let's break it down.
    ///
    /// Firstly, a span of seconds should be able to represent at least the
    /// complete span supported by `Timestamp`. Thus, it's based off of
    /// `UnixSeconds::LEN`. That is, a span should be able to represent the
    /// value `UnixSeconds::MAX - UnixSeconds::MIN`.
    ///
    /// Secondly, a span should also be able to account for any amount of
    /// possible time that a time zone offset might add or subtract to an
    /// `Timestamp`. This also means it can account for any difference between
    /// two `civil::DateTime` values.
    ///
    /// Thirdly, we would like our span to be divisible by
    /// `SECONDS_PER_CIVIL_DAY`. This isn't strictly required, but it makes
    /// defining boundaries a little smoother. If it weren't divisible, then the
    /// lower bounds on some types would need to be adjusted by one.
    ///
    /// Note that neither the existence of this constant nor defining our
    /// spans based on it impacts the correctness of doing arithmetic on zoned
    /// instants. Arithmetic on zoned instants still uses "civil" spans, but the
    /// length of time for some units (like a day) might vary. The arithmetic
    /// for zoned instants accounts for this explicitly. But it still must obey
    /// the limits set here.
    (
        DeltaSeconds,
        i64,
        "seconds",
        -Self::MAX,
        next_multiple_of(
            UnixEpochSeconds::LEN as i64
                + OffsetTotalSeconds::MAX as i64
                + c::SECS_PER_CIVIL_DAY,
            c::SECS_PER_CIVIL_DAY,
        ),
    ),
    /// The range of hours supported.
    ///
    /// Jiff always uses the "24 hour clock" where midnight is hour `0` and
    /// 11pm is `23`. The alternative spelling of midnight, `24:00`, is not
    /// supported.
    (Hour, i8, "hour", 0, 23),
    /// The supported range of week numbers in the ISO 8601 week calendar.
    ///
    /// ISO 8601 leap years have exactly 53 weeks. Non-leap years have 52
    /// weeks.
    (ISOWeek, i8, "iso-week", 1, 53),
    /// The supported range of year numbers in the ISO 8601 week calendar.
    ///
    /// This matches the supported range of Gregorian year numbers.
    (ISOYear, i16, "iso-year", Year::MIN, Year::MAX),
    /// The supported range of microsecond values.
    (Microsecond, i16, "microsecond", 0, 999),
    /// The supported range of millisecond values.
    (Millisecond, i16, "millisecond", 0, 999),
    /// The supported range of minute values.
    (Minute, i8, "minute", 0, 59),
    /// The supported range of month values.
    (Month, i8, "month", 1, 12),
    /// The supported range of nanosecond values.
    (Nanosecond, i16, "nanosecond", 0, 999),
    /// The supported range of values for getting the "nth weekday" from a
    /// date.
    ///
    /// Note that `0` is also invalid, but this is not expressed by
    /// this single contiguous range.
    (
        NthWeekday,
        i32,
        "nth weekday",
        -Self::MAX,
        (DeltaSeconds::MAX / c::SECS_PER_CIVIL_WEEK) as i32,
    ),
    /// The supported range of values for getting the "nth weekday of a month"
    /// from a particular date.
    ///
    /// Note that `0` is also invalid, but this is not expressed by
    /// this single contiguous range. Note also that `5` or `-5` can be invalid
    /// if the provided month does not contain a 5th instance of the weekday
    /// provided by the caller.
    (NthWeekdayOfMonth, i8, "nth weekday of month", -5, 5),
    /// The number of hours allowed in a time zone offset.
    ///
    /// This number was somewhat arbitrarily chosen. In part because it's
    /// bigger than any current offset in actual use by a wide margin, and in
    /// part because POSIX `TZ` strings require the ability to store offsets in
    /// the range `-24:59:59..=25:59:59`. Note though that we make the range a
    /// little bigger with `-25:59:59..=25:59:59` so that negating an offset
    /// always produces a valid offset.
    ///
    /// Note that RFC 8536 actually allows offsets to be much bigger, namely,
    /// in the range `(-2^31, 2^31)`, where both ends are _exclusive_ (`-2^31`
    /// is explicitly disallowed, and `2^31` overflows a signed 32-bit
    /// integer). But RFC 8536 does say that it *should* be in the range
    /// `[-89999, 93599]`, which matches POSIX. In order to keep our offset
    /// small, we stick roughly to what POSIX requires.
    ///
    /// Note that we support a slightly bigger range of offsets than Temporal.
    /// Temporal seems to support only up to 23 hours, but we go up to 25
    /// hours. This is done to support POSIX time zone strings, which also
    /// require 25 hours (plus the maximal minute/second components).
    (OffsetHours, i8, "time zone offset hours", -25, 25),
    /// The supported range of "minute" component of a time zone offset.
    (OffsetMinutes, i8, "time zone offset minutes", -59, 59),
    /// The supported range of "second" component of a time zone offset.
    (OffsetSeconds, i8, "time zone offset seconds", -59, 59),
    /// The supported range of time zone offsets, expressed in units of
    /// seconds.
    (
        OffsetTotalSeconds,
        i32,
        "time zone offset total seconds",
        -Self::MAX,
        (OffsetHours::MAX as i32 * c::SECS_PER_HOUR_32)
        + (OffsetMinutes::MAX as i32 * c::MINS_PER_HOUR_32)
        + OffsetSeconds::MAX as i32,
    ),
    /// The supported range of second values.
    ///
    /// Note that Jiff does not support leap seconds. So a value of `60`
    /// is never valid. (Except when parsing. In which case, Jiff will
    /// automatically clamp the value `60` to `59`.)
    (Second, i8, "second", 0, 59),
    /// The supported range of fractional seconds, expressed in units of
    /// nanoseconds.
    (SubsecNanosecond, i32, "subsecond nanosecond", 0, c::NANOS_PER_SEC_32 - 1),
    /// The supported range of a timestamp's subsecond component.
    ///
    /// This is different from `SubsecNanosecond`, which applies to civil time
    /// within a single day, in that it can be negative. For example, the
    /// timestamp `-0s 123ms` refers to 123 milliseconds before the Unix epoch.
    (
        SignedSubsecNanosecond,
        i32,
        "subsecond nanosecond",
        -SubsecNanosecond::MAX,
        SubsecNanosecond::MAX,
    ),
    /// An error that occurs when doing arithmetic on a timestamp overflows
    /// the `i32` representation of a nanosecond.
    (TimestampArithmeticNanosecond, i32, "nanosecond", i32::MIN, i32::MAX),
    /// The supported range of days from the Unix epoch for the Gregorian
    /// calendar.
    ///
    /// The range supported is based on the range of Unix timestamps that we
    /// support.
    (
        UnixEpochDays,
        i32,
        "Unix epoch days",
        (UnixEpochSeconds::MIN + OffsetTotalSeconds::MIN as i64).div_euclid(c::SECS_PER_CIVIL_DAY) as i32,
        (UnixEpochSeconds::MAX + OffsetTotalSeconds::MAX as i64).div_euclid(c::SECS_PER_CIVIL_DAY) as i32,
    ),
    (
        UnixEpochMilliseconds,
        i64,
        "Unix timestamp milliseconds",
        UnixEpochSeconds::MIN * c::MILLIS_PER_SEC,
        UnixEpochSeconds::MAX * c::MILLIS_PER_SEC,
    ),
    (
        UnixEpochMicroseconds,
        i64,
        "Unix timestamp microseconds",
        UnixEpochMilliseconds::MIN * c::MICROS_PER_MILLI,
        UnixEpochMilliseconds::MAX * c::MICROS_PER_MILLI,
    ),
    /// The supported range of seconds for the difference between any two
    /// values of `UnixEpochDays`.
    ///
    /// This range should correspond to the first second of `Year::MIN`
    /// (with a minimal offset) up through (and including) the last second
    /// of `Year::MAX` (with a maximal offset). Actually computing that is
    /// non-trivial, however, it can be computed easily enough using Unix
    /// programs like `date`:
    ///
    /// ```text
    /// $ TZ=0 date -d 'Mon Jan  1 12:00:00 AM  -9999' +'%s'
    /// date: invalid date ‘Mon Jan  1 12:00:00 AM  -9999’
    /// $ TZ=0 date -d 'Fri Dec 31 23:59:59  9999' +'%s'
    /// 253402300799
    /// ```
    ///
    /// Well, almost easily enough. `date` apparently doesn't support negative
    /// years. But it does support negative timestamps:
    ///
    /// ```text
    /// $ TZ=0 date -d '@-377705116800'
    /// Mon Jan  1 12:00:00 AM  -9999
    /// $ TZ=0 date -d '@253402300799'
    /// Fri Dec 31 11:59:59 PM  9999
    /// ```
    ///
    /// With that said, we actually end up restricting the range a bit more
    /// than what's above. Namely, what's above is what we support for civil
    /// datetimes. Because of time zones, we need to choose whether all
    /// `Timestamp` values can be infallibly converted to `civil::DateTime`
    /// values, or whether all `civil::DateTime` values can be infallibly
    /// converted to `Timestamp` values. Jiff choses the former because getting
    /// a civil datetime is important for formatting. If Jiff didn't choose the
    /// former, there would be some timestamps that could not be formatted.
    /// Thus, we make room by shrinking the range of allowed instants by
    /// precisely the maximum supported time zone offset.
    (
        UnixEpochSeconds,
        i64,
        "Unix timestamp seconds",
        -377705116800 - OffsetTotalSeconds::MIN as i64,
        253402300799 - OffsetTotalSeconds::MAX as i64,
    ),
    /// The supported range of 0-offset week day values when weeks start on
    /// Monday.
    (WeekdayMondayZero, i8, "weekday (Monday 0-indexed)", 0, 6),
    /// The supported range of 1-offset week day values when weeks start on
    /// Monday.
    (WeekdayMondayOne, i8, "weekday (Monday 1-indexed)", 1, 7),
    /// The supported range of 0-offset week day values when weeks start on
    /// Sunday.
    (WeekdaySundayZero, i8, "weekday (Sunday 0-indexed)", 0, 6),
    /// The supported range of 1-offset week day values when weeks start on
    /// Sunday.
    (WeekdaySundayOne, i8, "weekday (Sunday 1-indexed)", 1, 7),
    /// The range of years supported.
    (Year, i16, "year", -9999, 9999),
    /// The range of years supported for the Common Era (CE).
    (YearCE, i16, "CE year", 1, Year::MAX),
    /// The range of years supported for Before Common Era (BCE).
    (YearBCE, i16, "BCE year", 1, Year::MAX + 1),
}

/// A trait for making `x as int_type` usable in a generic context.
///
/// All of these methods require callers to ensure the cast is correct.
/// However, when `debug_assertions` is enabled, the casts will result in
/// a panic if they are incorrect.
///
/// Because of the extra checks when `debug_assertions` is enabled, Jiff tries
/// to use these routines wherever possible in lieu of `as`. The primary
/// downside of using this trait is that it doesn't work in a `const` context
/// because Rust does not yet support generics in `const`.
#[allow(missing_docs)]
pub trait Primitive:
    Clone
    + Copy
    + Eq
    + PartialEq
    + PartialOrd
    + Ord
    + core::fmt::Debug
    + core::fmt::Display
{
    fn as_i8(self) -> i8;
    fn as_i16(self) -> i16;
    fn as_i32(self) -> i32;
    fn as_i64(self) -> i64;

    fn from_i8(n: i8) -> Self;
    fn from_i16(n: i16) -> Self;
    fn from_i32(n: i32) -> Self;
    fn from_i64(n: i64) -> Self;

    fn checked_add(self, n: Self) -> Option<Self>;
    fn checked_mul(self, n: Self) -> Option<Self>;
}

macro_rules! impl_primitive {
    ($($intty:ty),*) => {
        $(
            impl Primitive for $intty {
                fn as_i8(self) -> i8 {
                    #[cfg(debug_assertions)]
                    {
                        i8::try_from(self).unwrap()
                    }
                    #[cfg(not(debug_assertions))]
                    {
                        self as i8
                    }
                }

                fn as_i16(self) -> i16 {
                    #[cfg(debug_assertions)]
                    {
                        i16::try_from(self).unwrap()
                    }
                    #[cfg(not(debug_assertions))]
                    {
                        self as i16
                    }
                }

                fn as_i32(self) -> i32 {
                    #[cfg(debug_assertions)]
                    {
                        i32::try_from(self).unwrap()
                    }
                    #[cfg(not(debug_assertions))]
                    {
                        self as i32
                    }
                }

                fn as_i64(self) -> i64 {
                    #[cfg(debug_assertions)]
                    {
                        i64::try_from(self).unwrap()
                    }
                    #[cfg(not(debug_assertions))]
                    {
                        self as i64
                    }
                }

                fn from_i8(n: i8) -> Self {
                    #[cfg(debug_assertions)]
                    {
                        Self::try_from(n).unwrap()
                    }
                    #[cfg(not(debug_assertions))]
                    {
                        n as Self
                    }
                }

                fn from_i16(n: i16) -> Self {
                    #[cfg(debug_assertions)]
                    {
                        Self::try_from(n).unwrap()
                    }
                    #[cfg(not(debug_assertions))]
                    {
                        n as Self
                    }
                }

                fn from_i32(n: i32) -> Self {
                    #[cfg(debug_assertions)]
                    {
                        Self::try_from(n).unwrap()
                    }
                    #[cfg(not(debug_assertions))]
                    {
                        n as Self
                    }
                }

                fn from_i64(n: i64) -> Self {
                    #[cfg(debug_assertions)]
                    {
                        Self::try_from(n).unwrap()
                    }
                    #[cfg(not(debug_assertions))]
                    {
                        n as Self
                    }
                }

                fn checked_add(self, n: $intty) -> Option<$intty> {
                    <$intty>::checked_add(self, n)
                }

                fn checked_mul(self, n: $intty) -> Option<$intty> {
                    <$intty>::checked_mul(self, n)
                }
            }
        )*
    }
}

impl_primitive!(i8, i16, i32, i64);

/// An interface for defining boundaries on integer values.
///
/// An implementation of this trait defines a single contiguous range used
/// inside of Jiff. For example, the allowed range of years is `-9999..=9999`.
///
/// Each implementation defines its primitive representation, which is usally
/// the smallest signed integer type that can hold the minimum and maximum
/// values. This trait provides `check`, `checked_add` and `checked_mul`
/// methods that callers likely do not need to override.
///
/// Other than the associated types and constants, the only required method
/// that callers must implement is `error()`.
///
/// # History & Design
///
/// It took a lot of design iteration to arrive at this trait.
/// [Jiff originally started with an Ada-inspired ranged integer
/// abstraction][jiff-range-history]. At first, it worked really well and
/// seemed to do a decent job at uncovering bugs related to values going
/// outside of their intended range. Or worse, actual integer overflow. This
/// particular abstraction had three key properties:
///
/// 1. Each ranged integer type corresponded to a single contiguous range
/// (e.g., `Year` was `-9999..=9999`), and the range values were defined by
/// const type parameters.
/// 2. The value inside of a ranged integer was permitted to "drift" outside
/// of its defined range (but not outside of its primitive representation).
/// A panic would only manifest when one tried to look inside the ranged
/// integer and access its primitive representation (e.g., `Date::year`).
/// 3. Ranged integers kept track of their minimum and maximum possible values,
/// *at runtime*, but only when `debug_assertions` were enabled. So when you
/// did `x + y`, the result wouldn't just be the actual addition, but also
/// addition performed on the corresponding min and max values on each of `x`
/// and `y`.
///
/// When all three of these properties combined, you got an excellent bug
/// finding tool without necessarily needing to write tests covering all of the
/// edge cases. For example, if you did `x + y` and the result only went out
/// of range when `x` and `y` were some extreme values, you'd get an immediate
/// panic once you accessed the value even when using non-extreme values.
/// That's because the extreme values are computed dynamically at runtime.
///
/// Unfortunately, as Jiff grew bigger, ranged integers became more and more
/// annoying. They have two fatal flaws as conceived above:
///
/// 1. If you "escape" outside of a ranged integer at any point, you lose the
/// tracked min and max values. And thus you lose advantage of ranged integers
/// in the first place.
/// 2. Because of (1), it was exceptionally difficult to do any sort of clever
/// representation-based optimizations. Since a ranged integer had its own
/// representation, trying to do, e.g., bit-packing was effectively impossible.
///
/// There were some other downsides, although I didn't perceive them as fatal
/// on their own (but they certaintly contributed to my decision to abdandon
/// them):
///
/// * Since these were custom types and they were generic, literally none of
/// them worked in a `const` context.
/// * Whenever there was conditional control flow involving ranged integers,
/// it was necessary to do strange contortions to get their internal min/max
/// values to line up correctly.
/// * As time pressed on, the panics surfaced by ranged integers were more and
/// more likely to be as a result of "holding it wrong" and not because of any
/// actual bugs in the arithmetic.
///
/// With all of that said, a datetime library has to be quite paranoid about
/// the range of values it permits. And error messages really benefit from
/// being able to encode information about the allowed range so that users know
/// what is and isn't illegal. Hence, I settled on this lighter weight design
/// that still encodes ranges as a static property of the program, but as
/// associated types on a trait. And then that trait provides a few very
/// primitive operations (like checking if an integer is in bounds) that
/// produce an appropriate error message on failure.
///
/// Note also that this trait is specifically not implemented for `i128`.
/// It's only implemented for `i8`, `i16`, `i32` and `i64`. This is because
/// the `check` function accepts an `i64` as the type that can contain all
/// possible values for _any_ range type. And then that value is checked before
/// potentially being converted to a smaller primitive representation. If this
/// trait supported `i128`, then one would want a `check` function where
/// parameters get converted to `i128` and then their ranges are checked. I
/// did not want this because of the extra costs associated with `i128`.
///
/// Moreover, Jiff is using `i128` in a vanishingly small number of locations.
/// I don't think I'll ever be able to eliminate it entirely, but the number
/// of locations is small enough that they are special cased and don't need to
/// fit into this infrastructure.
///
/// Finally, a key property of this design is that error values carry no
/// associated data. Instead, they are instantiated as implementations of
/// this trait, and that implementation provides all of the information
/// necessary to craft a half-way decent error message.
///
/// [jiff-range-history]: https://github.com/BurntSushi/jiff/issues/11
pub trait Bounds: Sized {
    /// A short human readable description of the values represented by these
    /// bounds. This is used in error messages.
    const WHAT: &'static str;

    /// The minimum boundary value.
    const MIN: Self::Primitive;

    /// The maximum boundary value.
    const MAX: Self::Primitive;

    /// The primitive integer representation for this boundary type.
    ///
    /// This is generally the smallest primitive integer type that fits the
    /// minimum and maximum allowed values.
    type Primitive: Primitive;

    /// The error type returned when a value is considered out of range for
    /// this particular implementation.
    ///
    /// The intended usage is for this type to be an enum of single-field
    /// variants. The field is meant to be an instantiation of `RawBoundsError`
    /// with the type parameter set to `Self`.
    ///
    /// See [`BoundsError`] for an example.
    type Error;

    /// Create an error when a value is outside the bounds for this type.
    fn error() -> Self::Error;

    /// Converts the 64-bit integer provided into the primitive representation
    /// of these bounds.
    ///
    /// # Errors
    ///
    /// This returns an error if the given integer does not fit in the bounds
    /// prescribed by this trait implementation.
    #[inline(always)]
    fn check(n: impl Into<i64>) -> Result<Self::Primitive, Self::Error> {
        let n = n.into();
        if !(Self::MIN.as_i64() <= n && n <= Self::MAX.as_i64()) {
            return Err(Self::error());
        }
        Ok(Self::Primitive::from_i64(n))
    }

    /// Checks whether the given integer, in the same primitive representation
    /// as this boundary type, is in bounds.
    ///
    /// # Errors
    ///
    /// This returns an error if the given integer does not fit in the bounds
    /// prescribed by this trait implementation.
    #[inline(always)]
    fn check_self(n: Self::Primitive) -> Result<Self::Primitive, Self::Error> {
        if !(Self::MIN <= n && n <= Self::MAX) {
            return Err(Self::error());
        }
        Ok(n)
    }

    /// Performs checked addition using this boundary type's primitive
    /// representation.
    ///
    /// # Errors
    ///
    /// If the result exceeds the boundaries of the primitive type or of the
    /// declared range for this type, then an error is returned.
    #[inline(always)]
    fn checked_add(
        n1: Self::Primitive,
        n2: Self::Primitive,
    ) -> Result<Self::Primitive, Self::Error> {
        Self::check_self(n1.checked_add(n2).ok_or_else(Self::error)?)
    }

    /// Performs checked multiplication using this boundary type's primitive
    /// representation.
    ///
    /// # Errors
    ///
    /// If the result exceeds the boundaries of the primitive type or of the
    /// declared range for this type, then an error is returned.
    #[inline(always)]
    fn checked_mul(
        n1: Self::Primitive,
        n2: Self::Primitive,
    ) -> Result<Self::Primitive, Self::Error> {
        Self::check_self(n1.checked_mul(n2).ok_or_else(Self::error)?)
    }
}

/// An error type that encodes information from an implementation of
/// [`Bounds`].
///
/// The information encoded is used to write an error message in response to a
/// value being out of bounds.
#[derive(Eq, PartialEq)]
pub struct RawBoundsError<B>(core::marker::PhantomData<B>);

impl<B> RawBoundsError<B> {
    /// Create a new raw boundary error.
    ///
    /// It is intended for `B` to implement the `Bounds` trait. But this
    /// constructor technically does not require it. However, the `Debug`
    /// and `Display` impls on this type do require it.
    #[inline]
    pub const fn new() -> RawBoundsError<B> {
        RawBoundsError(core::marker::PhantomData)
    }
}

impl<B> Copy for RawBoundsError<B> {}

impl<B> Clone for RawBoundsError<B> {
    #[inline]
    fn clone(&self) -> RawBoundsError<B> {
        RawBoundsError::new()
    }
}

impl<B, P> core::fmt::Debug for RawBoundsError<B>
where
    B: Bounds<Primitive = P>,
    P: core::fmt::Debug,
{
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        f.debug_struct("RawBoundsError")
            .field("what", &B::WHAT)
            .field("min", &B::MIN)
            .field("max", &B::MAX)
            .finish()
    }
}

impl<B, P> core::fmt::Display for RawBoundsError<B>
where
    B: Bounds<Primitive = P>,
    P: core::fmt::Display,
{
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(
            f,
            "parameter '{what}' is not in the required range of {min}..={max}",
            what = B::WHAT,
            min = B::MIN,
            max = B::MAX,
        )
    }
}

#[cfg(feature = "defmt")]
impl<B, P> defmt::Format for RawBoundsError<B>
where
    B: Bounds<Primitive = P>,
    P: defmt::Format,
{
    fn format(&self, f: defmt::Formatter) {
        defmt::write!(
            f,
            "RawBoundsError {{ what: {=str}, min: {}, max: {} }}",
            B::WHAT,
            B::MIN,
            B::MAX
        );
    }
}

/// The error type used by the trait implementations of `Bounds` in this crate.
///
/// Callers can think of this as a single unifying error value that combines
/// all implementations of `Bounds` in this crate. Internally, it's an enum
/// with each variant corresponding to a single implementation of `Bounds`.
/// Each variant has a field containing a zero-sized `RawBoundsError<B>`,
/// where `B` is the type implementing `Bounds`.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct BoundsError {
    kind: BoundsErrorKind,
}

impl BoundsError {
    pub(crate) const fn into_range_error(self) -> RangeError {
        RangeError { kind: RangeErrorKind::Bounds(self) }
    }
}

impl core::fmt::Display for BoundsError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        self.kind.fmt(f)
    }
}

/// An error type that combines `BoundsError` with other custom error variants.
///
/// For example, a range error on the days of the month when constructing a
/// date would ideally include the corresponding year and month. Otherwise the
/// error message is likely to be much less useful than it could be.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct RangeError {
    kind: RangeErrorKind,
}

impl From<BoundsError> for RangeError {
    fn from(err: BoundsError) -> RangeError {
        err.into_range_error()
    }
}

impl From<SpecialBoundsError> for RangeError {
    fn from(err: SpecialBoundsError) -> RangeError {
        err.into_range_error()
    }
}

impl core::fmt::Display for RangeError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match self.kind {
            RangeErrorKind::Bounds(ref err) => err.fmt(f),
            RangeErrorKind::Special(ref err) => err.fmt(f),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for RangeError {}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
enum RangeErrorKind {
    Bounds(BoundsError),
    Special(SpecialBoundsError),
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub(crate) enum SpecialBoundsError {
    DateInvalidDay { year: i16, month: i8 },
    DateInvalidDayOfYear { year: i16 },
    UnixEpochNanoseconds,
}

impl SpecialBoundsError {
    pub(crate) const fn into_range_error(self) -> RangeError {
        RangeError { kind: RangeErrorKind::Special(self) }
    }
}

impl core::fmt::Display for SpecialBoundsError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        use self::SpecialBoundsError::*;

        match *self {
            DateInvalidDay { year, month } => write!(
                f,
                "parameter 'day' for `{year:04}-{month:02}` is invalid, \
                 must be in range `1..={max_day}`",
                max_day = crate::civil::days_in_month(year, month),
            ),
            DateInvalidDayOfYear { year } => write!(
                f,
                "number of days for `{year:04}` is invalid, \
                 must be in range `1..={max_day}`",
                max_day = crate::civil::days_in_year(year),
            ),
            UnixEpochNanoseconds => write!(
                f,
                "parameter 'Unix timestamp nanoseconds' \
                 is not in the required range of {min}..={max}",
                min = UnixEpochMicroseconds::MIN as i128
                    * (c::NANOS_PER_MICRO as i128),
                max = UnixEpochMicroseconds::MAX as i128
                    * (c::NANOS_PER_MICRO as i128),
            ),
        }
    }
}

/// Provides routines usable in a `const` context for checking boundary values.
///
/// These routines return a `RawBoundsError` and thus work with any type that
/// implements the `Bounds` trait.
#[allow(missing_docs)]
pub mod const_check {
    use super::{Bounds, RawBoundsError};

    #[inline(always)]
    pub const fn i8<B>(n: i64) -> Result<i8, RawBoundsError<B>>
    where
        B: Bounds<Primitive = i8>,
    {
        if !((B::MIN as i64) <= n && n <= (B::MAX as i64)) {
            return Err(RawBoundsError::new());
        }
        Ok(n as i8)
    }

    #[inline(always)]
    pub const fn i16<B>(n: i64) -> Result<i16, RawBoundsError<B>>
    where
        B: Bounds<Primitive = i16>,
    {
        if !((B::MIN as i64) <= n && n <= (B::MAX as i64)) {
            return Err(RawBoundsError::new());
        }
        Ok(n as i16)
    }

    #[inline(always)]
    pub const fn i32<B>(n: i64) -> Result<i32, RawBoundsError<B>>
    where
        B: Bounds<Primitive = i32>,
    {
        if !((B::MIN as i64) <= n && n <= (B::MAX as i64)) {
            return Err(RawBoundsError::new());
        }
        Ok(n as i32)
    }

    #[inline(always)]
    pub const fn i64<B>(n: i64) -> Result<i64, RawBoundsError<B>>
    where
        B: Bounds<Primitive = i64>,
    {
        if !(B::MIN <= n && n <= B::MAX) {
            return Err(RawBoundsError::new());
        }
        Ok(n)
    }
}

/// Provides routines usable in a `const` context for checked arithmetic.
///
/// These routines return a `RawBoundsError` and thus work with any type that
/// implements the `Bounds` trait.
#[allow(missing_docs)]
pub mod const_checked_add {
    use super::{Bounds, RawBoundsError};

    #[inline(always)]
    pub const fn i8<B>(n1: i8, n2: i8) -> Result<i8, RawBoundsError<B>>
    where
        B: Bounds<Primitive = i8>,
    {
        let sum = match n1.checked_add(n2) {
            Some(sum) => sum,
            None => return Err(RawBoundsError::new()),
        };
        super::const_check::i8(sum as i64)
    }

    #[inline(always)]
    pub const fn i16<B>(n1: i16, n2: i16) -> Result<i16, RawBoundsError<B>>
    where
        B: Bounds<Primitive = i16>,
    {
        let sum = match n1.checked_add(n2) {
            Some(sum) => sum,
            None => return Err(RawBoundsError::new()),
        };
        super::const_check::i16(sum as i64)
    }

    #[inline(always)]
    pub const fn i32<B>(n1: i32, n2: i32) -> Result<i32, RawBoundsError<B>>
    where
        B: Bounds<Primitive = i32>,
    {
        let sum = match n1.checked_add(n2) {
            Some(sum) => sum,
            None => return Err(RawBoundsError::new()),
        };
        super::const_check::i32(sum as i64)
    }

    #[inline(always)]
    pub const fn i64<B>(n1: i64, n2: i64) -> Result<i64, RawBoundsError<B>>
    where
        B: Bounds<Primitive = i64>,
    {
        let sum = match n1.checked_add(n2) {
            Some(sum) => sum,
            None => return Err(RawBoundsError::new()),
        };
        super::const_check::i64(sum)
    }
}

/// A representation of a numeric sign.
///
/// Its `Display` impl emits the ASCII minus sign, `-`, when this is negative.
/// It emits the empty string in all other cases.
#[derive(
    Clone, Copy, Debug, Default, Eq, Hash, PartialEq, PartialOrd, Ord,
)]
#[repr(i8)]
#[allow(missing_docs)]
pub enum Sign {
    #[default]
    Zero = 0,
    Positive = 1,
    Negative = -1,
}

impl Sign {
    /// Returns true when the sign is `Sign::Zero`.
    #[inline]
    pub const fn is_zero(self) -> bool {
        matches!(self, Sign::Zero)
    }

    /// Returns true when the sign is `Sign::Positive`.
    #[inline]
    pub const fn is_positive(self) -> bool {
        matches!(self, Sign::Positive)
    }

    /// Returns true when the sign is `Sign::Negative`.
    #[inline]
    pub const fn is_negative(self) -> bool {
        matches!(self, Sign::Negative)
    }

    /// Returns the sign as an `i8`.
    ///
    /// This is guaranteed to be `-1`, `0` or `1`.
    #[inline]
    pub const fn signum(self) -> i8 {
        self.as_i8()
    }

    /// Returns the sign as an `i8`.
    ///
    /// This is guaranteed to be `-1`, `0` or `1`.
    #[inline]
    pub const fn as_i8(self) -> i8 {
        self as i8
    }

    /// Returns the sign as an `i16`.
    ///
    /// This is guaranteed to be `-1`, `0` or `1`.
    #[inline]
    pub const fn as_i16(self) -> i16 {
        self as i16
    }

    /// Returns the sign as an `i32`.
    ///
    /// This is guaranteed to be `-1`, `0` or `1`.
    #[inline]
    pub const fn as_i32(self) -> i32 {
        self as i32
    }

    /// Returns the sign as an `i64`.
    ///
    /// This is guaranteed to be `-1`, `0` or `1`.
    #[inline]
    pub const fn as_i64(self) -> i64 {
        self as i64
    }

    /// Returns the sign as an `i128`.
    ///
    /// This is guaranteed to be `-1`, `0` or `1`.
    #[inline]
    pub const fn as_i128(self) -> i128 {
        self as i128
    }

    /// Returns a `Sign` as a result of comparing two values.
    ///
    /// * When `t1 < t2`, returns `Sign::Negative`.
    /// * When `t1 == t2`, returns `Sign::Zero`.
    /// * When `t1 > t2`, returns `Sign::Positive`.
    #[inline]
    pub fn from_ordinals<T: Ord>(t1: T, t2: T) -> Sign {
        use core::cmp::Ordering::*;
        match t1.cmp(&t2) {
            Less => Sign::Negative,
            Equal => Sign::Zero,
            Greater => Sign::Positive,
        }
    }
}

impl core::fmt::Display for Sign {
    #[inline]
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        if self.is_negative() {
            f.write_str("-")
        } else {
            Ok(())
        }
    }
}

impl core::ops::Neg for Sign {
    type Output = Sign;

    #[inline]
    fn neg(self) -> Sign {
        match self {
            Sign::Positive => Sign::Negative,
            Sign::Zero => Sign::Zero,
            Sign::Negative => Sign::Positive,
        }
    }
}

impl From<i8> for Sign {
    #[inline]
    fn from(n: i8) -> Sign {
        Sign::from(i64::from(n))
    }
}

impl From<i16> for Sign {
    #[inline]
    fn from(n: i16) -> Sign {
        Sign::from(i64::from(n))
    }
}

impl From<i32> for Sign {
    #[inline]
    fn from(n: i32) -> Sign {
        Sign::from(i64::from(n))
    }
}

impl From<i64> for Sign {
    #[inline]
    fn from(n: i64) -> Sign {
        if n == 0 {
            Sign::Zero
        } else if n > 0 {
            Sign::Positive
        } else {
            Sign::Negative
        }
    }
}

impl From<i128> for Sign {
    #[inline]
    fn from(n: i128) -> Sign {
        if n == 0 {
            Sign::Zero
        } else if n > 0 {
            Sign::Positive
        } else {
            Sign::Negative
        }
    }
}

/// This considers `NaN` values as having a zero sign.
///
/// In general, Jiff having a `NaN` value in memory is a bug.
impl From<f64> for Sign {
    #[inline]
    fn from(n: f64) -> Sign {
        use core::num::FpCategory::*;

        // This is a little odd, but we want +/- 0 to
        // always have a sign of zero, so as to be consistent
        // with how we deal with signed integers.
        //
        // As for NaN... It should generally be a bug if
        // Jiff ever materializes a NaN. Notably, I do not
        // believe there are any APIs in which a float is
        // given from the caller. Jiff only ever uses them
        // internally or returns them. So if we get a NaN,
        // it's on us. If we do, just assign it a zero sign?
        if matches!(n.classify(), Nan | Zero) {
            Sign::Zero
        } else if n.is_sign_positive() {
            Sign::Positive
        } else {
            Sign::Negative
        }
    }
}

impl core::ops::Mul<Sign> for Sign {
    type Output = Sign;

    #[inline]
    fn mul(self, rhs: Sign) -> Sign {
        match (self, rhs) {
            (Sign::Zero, _) | (_, Sign::Zero) => Sign::Zero,
            (Sign::Positive, Sign::Positive) => Sign::Positive,
            (Sign::Negative, Sign::Negative) => Sign::Positive,
            (Sign::Positive, Sign::Negative) => Sign::Negative,
            (Sign::Negative, Sign::Positive) => Sign::Negative,
        }
    }
}

impl core::ops::Mul<i8> for Sign {
    type Output = i8;

    #[inline]
    fn mul(self, n: i8) -> i8 {
        self.as_i8() * n
    }
}

impl core::ops::Mul<Sign> for i8 {
    type Output = i8;

    #[inline]
    fn mul(self, n: Sign) -> i8 {
        self * n.as_i8()
    }
}

impl core::ops::Mul<i16> for Sign {
    type Output = i16;

    #[inline]
    fn mul(self, n: i16) -> i16 {
        self.as_i16() * n
    }
}

impl core::ops::Mul<Sign> for i16 {
    type Output = i16;

    #[inline]
    fn mul(self, n: Sign) -> i16 {
        self * n.as_i16()
    }
}

impl core::ops::Mul<i32> for Sign {
    type Output = i32;

    #[inline]
    fn mul(self, n: i32) -> i32 {
        self.as_i32() * n
    }
}

impl core::ops::Mul<Sign> for i32 {
    type Output = i32;

    #[inline]
    fn mul(self, n: Sign) -> i32 {
        self * n.as_i32()
    }
}

impl core::ops::Mul<i64> for Sign {
    type Output = i64;

    #[inline]
    fn mul(self, n: i64) -> i64 {
        self.as_i64() * n
    }
}

impl core::ops::Mul<Sign> for i64 {
    type Output = i64;

    #[inline]
    fn mul(self, n: Sign) -> i64 {
        self * n.as_i64()
    }
}

impl core::ops::Mul<i128> for Sign {
    type Output = i128;

    #[inline]
    fn mul(self, n: i128) -> i128 {
        self.as_i128() * n
    }
}

impl core::ops::Mul<Sign> for i128 {
    type Output = i128;

    #[inline]
    fn mul(self, n: Sign) -> i128 {
        self * n.as_i128()
    }
}

/// Computes the next multiple of `rhs` that is greater than or equal to `lhs`.
///
/// Taken from:
/// https://github.com/rust-lang/rust/blob/eff958c59e8c07ba0515e164b825c9001b242294/library/core/src/num/int_macros.rs
const fn next_multiple_of(lhs: i64, rhs: i64) -> i64 {
    // This would otherwise fail when calculating `r` when self == T::MIN.
    if rhs == -1 {
        return lhs;
    }

    let r = lhs % rhs;
    let m = if (r > 0 && rhs < 0) || (r < 0 && rhs > 0) { r + rhs } else { r };
    if m == 0 {
        lhs
    } else {
        lhs + (rhs - m)
    }
}
