/*!
A module for constants and various base utilities.

This module is a work-in-progress that may lead to helping us move off of
ranged integers. I'm not quite sure where this will go.
*/

use jcore::{
    bounds::{self, const_check, Bounds, RawBoundsError},
    constants as c,
};

use crate::Error;

/// Writes the definition of a single boundary type.
///
/// When only the name and type are given, then this copies the `WHAT`,
/// `MIN` and `MAX` definitions from `jcore`. This avoids copying the specific
/// range values and maintains a single point of truth.
macro_rules! define_one_boundary_type {
    (
        // The name of the boundary type.
        $name:ident,
        // The underlying primitive type. This is usually, but not always,
        // the smallest signed primitive integer type that can represent both
        // the minimum and maximum boundary values.
        $ty:ident,
        // A short human readable description that appears in error messages
        // when the boundaries of this type are violated.
        $what:expr,
        // The minimum value.
        $min:expr,
        // The maximum value.
        $max:expr $(,)?
    ) => {
        pub(crate) struct $name(());

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
        impl $name {
            pub(crate) const MIN: $ty = <$name as Bounds>::MIN;
            pub(crate) const MAX: $ty = <$name as Bounds>::MAX;
            const LEN: i128 = Self::MAX as i128 - Self::MIN as i128 + 1;

            #[cold]
            pub(crate) const fn error() -> BoundsError {
                BoundsError::$name(RawBoundsError::new())
            }

            #[cfg_attr(feature = "perf-inline", inline(always))]
            pub(crate) fn check(
                n: impl Into<i64>,
            ) -> Result<$ty, BoundsError> {
                <$name as Bounds>::check(n)
            }

            #[cfg_attr(feature = "perf-inline", inline(always))]
            pub(crate) const fn checkc(n: i64) -> Result<$ty, BoundsError> {
                match const_check::$ty(n) {
                    Ok(n) => Ok(n),
                    Err(err) => Err(BoundsError::$name(err)),
                }
            }

            #[cfg_attr(feature = "perf-inline", inline(always))]
            pub(crate) fn checked_add(
                n1: $ty,
                n2: $ty,
            ) -> Result<$ty, BoundsError> {
                <$name as Bounds>::checked_add(n1, n2)
            }

            #[cfg_attr(feature = "perf-inline", inline(always))]
            pub(crate) fn checked_mul(
                n1: $ty,
                n2: $ty,
            ) -> Result<$ty, BoundsError> {
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
    };
    // A shortcut for defining a boundary type in `jiff` that is just a mirror
    // of a boundary type defined in `jiff-core`. We do this instead of using
    // the boundary type from `jiff-core` directly because we can't have a
    // `From` impl on a `jiff-core` error type for `jiff::Error`. (Because
    // that would make `jiff-core` a public dependency of `jiff`. Which means
    // any semver incompatible release of `jiff-core` would require a semver
    // incompatible release of `jiff`. Since `jiff-core` evolves more quickly
    // than `jiff`, this would be bad juju.)
    ($name:ident, $ty:ident $(,)?) => {
        define_one_boundary_type!(
            $name,
            $ty,
            jcore::bounds::$name::WHAT,
            jcore::bounds::$name::MIN,
            jcore::bounds::$name::MAX,
        );
    };
}

/// This macro writes out the boiler plate to define a boundary type.
///
/// Specifically, it implements the `Bounds` trait and provides a few
/// concrete methods. The concrete methods are mostly wrappers around
/// the generic trait methods. They are provided so that callers don't
/// have to import the `Bounds` trait to use them.
macro_rules! define_bounds {
    ($((
        $name:ident,
        $ty:ident
        $($tt:tt)*
    )),* $(,)?) => {
        $(
            define_one_boundary_type!($name, $ty $($tt)*);
        )*

        /// An error that indicates a value is out of its intended range.
        #[derive(Clone, Debug)]
        #[cfg_attr(feature = "defmt", derive(defmt::Format))]
        pub(crate) enum BoundsError {
            $($name(RawBoundsError<$name>),)*
        }

        impl core::fmt::Display for BoundsError {
            fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
                match *self {
                    $(BoundsError::$name(ref err) => err.fmt(f),)*
                }
            }
        }
    }
}

// This is where we define all of Jiff's ranges.
//
// Well, that's not quite true. Many of them are actually defined in
// `jiff-core`. For those, we still have an entry here that still produces
// a boundary type, but it reuses the minimum and maximum values from
// `jiff-core`. This is so `jiff` completely owns its own boundary types
// (to avoid a public dep on `jiff-core`) while simultaneously preserving a
// single point of truth for every range type.
define_bounds! {
    (Century, i8, "century", 0, 99),
    (CivilDayNanosecond, i64),
    (CivilDaySecond, i32),
    (Day, i8),
    (DayOfYear, i16),
    (Hour, i8),
    (Hour12, i8, "hour (12 hour clock)", 1, 12),
    (ISOWeek, i8),
    (ISOYear, i16),
    // This matches Temporal's range.
    // See: https://github.com/tc39/proposal-temporal/issues/2458#issuecomment-1380742911
    (Increment, i64, "rounding increment", 1, 1_000_000_000),
    (Increment32, i32, "rounding increment", 1, 1_000_000_000),
    // This is only used in parsing. A value of `60` gets clamped to `59`.
    (LeapSecond, i8, "second", 0, 60),
    (Microsecond, i16),
    (Millisecond, i16),
    (Minute, i8),
    (Month, i8),
    (Nanosecond, i16),
    (NthWeekday, i32),
    (OffsetHours, i8),
    (OffsetMinutes, i8),
    (OffsetSeconds, i8),
    (OffsetTotalSeconds, i32),
    (Second, i8),
    (SignedDurationSeconds, i64, "signed duration seconds", i64::MIN, i64::MAX),
    (SpanYears, i16, "years", -Self::MAX, (Year::LEN - 1) as i16),
    (SpanMonths, i32, "months", -Self::MAX, SpanYears::MAX as i32 * 12),
    (SpanWeeks, i32, "weeks", -Self::MAX, SpanDays::MAX / c::DAYS_PER_CIVIL_WEEK_32),
    (SpanDays, i32, "days", -Self::MAX, SpanHours::MAX / c::HOURS_PER_CIVIL_DAY_32),
    (SpanHours, i32, "hours", -Self::MAX, (SpanMinutes::MAX / c::MINS_PER_HOUR) as i32),
    (SpanMinutes, i64, "minutes", -Self::MAX, SpanSeconds::MAX / c::SECS_PER_MIN),
    (
        SpanSeconds,
        i64,
        "seconds",
        bounds::DeltaSeconds::MIN,
        bounds::DeltaSeconds::MAX,
    ),
    (
        SpanMilliseconds,
        i64,
        "milliseconds",
        -Self::MAX,
        SpanSeconds::MAX * c::MILLIS_PER_SEC,
    ),
    (
        SpanMicroseconds,
        i64,
        "microseconds",
        -Self::MAX,
        SpanMilliseconds::MAX * c::MICROS_PER_MILLI,
    ),
    (SpanMultiple, i64, "span multiple", i64::MIN  + 1, i64::MAX),
    // A range of the allowed number of nanoseconds.
    //
    // For this, we cannot cover the full span of supported time instants since
    // `UnixSeconds::MAX * NANOSECONDS_PER_SECOND` cannot fit into 64-bits. We
    // could use a `i128`, but it doesn't seem worth bloating up `Span`.
    //
    // Also note that our min is equal to -max, so that the total number of
    // values in this range is one less than the number of distinct `i64`
    // values. We do that so that the absolute value is always defined.
    (SpanNanoseconds, i64, "nanoseconds", i64::MIN + 1, i64::MAX),
    (SubsecNanosecond, i32),
    (SignedSubsecNanosecond, i32),
    (UnixEpochDays, i32),
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
    (UnixEpochSeconds, i64),
    (WeekNum, i8, "week-number", 0, 53),
    (WeekdayMondayZero, i8),
    (WeekdayMondayOne, i8),
    (WeekdaySundayZero, i8),
    (WeekdaySundayOne, i8),
    // The range of years supported by Jiff.
    //
    // This is ultimately where some of the other ranges (like `UnixSeconds`)
    // were determined from. That is, the range of years is the primary point
    // at which the space of supported time instants is derived from. If one
    // wanted to expand this range, you'd need to change it here and then
    // compute the corresponding min/max values for `UnixSeconds`. (Among other
    // things... Increasing the supported Jiff range is far more complicated
    // than just changing some ranges here.)
    (Year, i16),
    (YearCE, i16),
    (YearBCE, i16),
    (YearTwoDigit, i16, "year (2 digits)", 0, 99),
    (
        ZonedDayNanoseconds,
        i64,
        "nanoseconds (in one zoned datetime day)",
        ZonedDaySeconds::MIN as i64 * c::NANOS_PER_SEC,
        ZonedDaySeconds::MAX as i64 * c::NANOS_PER_SEC,
    ),
    // The number of seconds permitted in a single day.
    //
    // This is mostly just a "sensible" cap on what is possible. We allow one
    // day to span up to 7 civil days.
    //
    // It must also be at least 1 second long.
    (
        ZonedDaySeconds,
        i32,
        "seconds (in one zoned datetime day)",
        1,
        7 * c::SECS_PER_CIVIL_DAY_32,
    ),
}

impl From<BoundsError> for Error {
    fn from(err: BoundsError) -> Error {
        Error::bounds(err)
    }
}

impl crate::error::IntoError for BoundsError {
    fn into_error(self) -> Error {
        self.into()
    }
}

/// Like `BoundsError`, but maintained manually.
///
/// This is useful for range errors outside of the framework above.
#[derive(Clone, Debug)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub(crate) enum SpecialBoundsError {
    SignedDurationFloatOutOfRangeF32,
    SignedDurationFloatOutOfRangeF64,
    SignedToUnsignedDuration,
}

impl core::fmt::Display for SpecialBoundsError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        use self::SpecialBoundsError::*;

        match *self {
            SignedToUnsignedDuration => f.write_str(
                "negative signed durations cannot be converted \
                 to an unsigned duration",
            ),
            SignedDurationFloatOutOfRangeF32 => {
                write!(
                    f,
                    "parameter 'floating point seconds' is not in \
                     the required range of {min}..={max}",
                    min = i64::MIN as f32,
                    max = i64::MAX as f32,
                )
            }
            SignedDurationFloatOutOfRangeF64 => {
                write!(
                    f,
                    "parameter 'floating point seconds' is not in \
                     the required range of {min}..={max}",
                    min = i64::MIN as f64,
                    max = i64::MAX as f64,
                )
            }
        }
    }
}

impl From<SpecialBoundsError> for Error {
    fn from(err: SpecialBoundsError) -> Error {
        Error::special_bounds(err)
    }
}

impl crate::error::IntoError for SpecialBoundsError {
    fn into_error(self) -> Error {
        self.into()
    }
}

#[cfg(test)]
mod tests {
    use alloc::string::ToString;

    use super::*;

    #[test]
    fn size_of_bounds_error() {
        // It's okay if this grows, but I wrote this test initially
        // to confirm that a `BoundsError` is very small. (Since it's
        // just a bunch of variants of ZSTs.)
        assert_eq!(1, core::mem::size_of::<BoundsError>());
    }

    #[test]
    fn basic_error_functionality() {
        let err = Year::check(10_000).unwrap_err();
        assert_eq!(
            err.to_string(),
            "parameter 'year' is not in the required range of -9999..=9999",
        );
    }
}
