/*!
Primitives for civil dates and times.
*/
pub use self::{
    date::{Date, ISOWeekDate, UnixEpochDay},
    datetime::DateTime,
    time::{Time, TimeNanosecond, TimeSecond},
    weekday::{Weekday, WeekdaysForward, WeekdaysReverse},
};
use crate::macros::unwrapr;

mod date;
mod datetime;
mod time;
mod weekday;

/// Creates a new `DateTime` value in a `const` context.
///
/// This is a convenience free function for [`DateTime::new`] that panics when
/// the given datetime is not valid. It is intended to provide a terse syntax
/// for constructing `DateTime` values from parameters that are known to be
/// valid.
///
/// # Panics
///
/// This routine panics when [`DateTime::new`] would return an error. That
/// is, when the given components do not correspond to a valid datetime.
/// Namely, all of the following must be true:
///
/// * The year must be in the range `-9999..=9999`.
/// * The month must be in the range `1..=12`.
/// * The day must be at least `1` and must be at most the number of days
/// in the corresponding month. So for example, `2024-02-29` is valid but
/// `2023-02-29` is not.
/// * `0 <= hour <= 23`
/// * `0 <= minute <= 59`
/// * `0 <= second <= 59`
/// * `0 <= subsec_nanosecond <= 999,999,999`
///
/// Similarly, when used in a const context, invalid parameters will prevent
/// your Rust program from compiling.
///
/// # Example
///
/// ```
/// use jiff_core::civil::datetime;
///
/// let dt = datetime(2024, 2, 29, 21, 30, 5, 123_456_789);
/// assert_eq!(dt.date().year(), 2024);
/// assert_eq!(dt.date().month(), 2);
/// assert_eq!(dt.date().day(), 29);
/// assert_eq!(dt.time().hour(), 21);
/// assert_eq!(dt.time().minute(), 30);
/// assert_eq!(dt.time().second(), 5);
/// assert_eq!(dt.time().millisecond(), 123);
/// assert_eq!(dt.time().microsecond(), 456);
/// assert_eq!(dt.time().nanosecond(), 789);
/// ```
#[inline]
pub const fn datetime(
    year: i16,
    month: i8,
    day: i8,
    hour: i8,
    minute: i8,
    second: i8,
    subsec_nanosecond: i32,
) -> DateTime {
    unwrapr!(
        DateTime::new(
            year,
            month,
            day,
            hour,
            minute,
            second,
            subsec_nanosecond,
        ),
        "invalid datetime"
    )
}

/// Creates a new `Date` value in a `const` context.
///
/// This is a convenience free function for [`Date::new`] that panics when
/// the given date is not valid. It is intended to provide a terse syntax for
/// constructing `Date` values from parameters that are known to be valid.
///
/// # Panics
///
/// This routine panics when [`Date::new`] would return an error. That is,
/// when the given year-month-day does not correspond to a valid date.
/// Namely, all of the following must be true:
///
/// * The year must be in the range `-9999..=9999`.
/// * The month must be in the range `1..=12`.
/// * The day must be at least `1` and must be at most the number of days
/// in the corresponding month. So for example, `2024-02-29` is valid but
/// `2023-02-29` is not.
///
/// Similarly, when used in a const context, invalid parameters will prevent
/// your Rust program from compiling.
///
/// # Example
///
/// ```
/// use jiff_core::civil::date;
///
/// let d = date(2024, 2, 29);
/// assert_eq!(d.year(), 2024);
/// assert_eq!(d.month(), 2);
/// assert_eq!(d.day(), 29);
/// ```
#[inline]
pub const fn date(year: i16, month: i8, day: i8) -> Date {
    unwrapr!(Date::new(year, month, day), "invalid date")
}

/// Creates a new `Time` value in a `const` context.
///
/// This is a convenience free function for [`Time::new`] that panics when
/// the given time is not valid. It is intended to provide a terse syntax for
/// constructing `Time` values from parameters that are known to be valid.
///
/// # Panics
///
/// This panics if the given values do not correspond to a valid `Time`.
/// All of the following conditions must be true:
///
/// * `0 <= hour <= 23`
/// * `0 <= minute <= 59`
/// * `0 <= second <= 59`
/// * `0 <= subsec_nanosecond <= 999,999,999`
///
/// Similarly, when used in a const context, invalid parameters will
/// prevent your Rust program from compiling.
///
/// # Example
///
/// This shows an example of a valid time in a `const` context:
///
/// ```
/// use jiff_core::civil::time;
///
/// let t = time(21, 30, 5, 123_456_789);
/// assert_eq!(t.hour(), 21);
/// assert_eq!(t.minute(), 30);
/// assert_eq!(t.second(), 5);
/// assert_eq!(t.millisecond(), 123);
/// assert_eq!(t.microsecond(), 456);
/// assert_eq!(t.nanosecond(), 789);
/// assert_eq!(t.subsec_nanosecond(), 123_456_789);
/// ```
#[inline]
pub const fn time(
    hour: i8,
    minute: i8,
    second: i8,
    subsec_nanosecond: i32,
) -> Time {
    unwrapr!(
        Time::new(hour, minute, second, subsec_nanosecond),
        "invalid time"
    )
}

/// Returns true if and only if the given year is a leap year.
///
/// A leap year is a year with 366 days. Non-leap years have 365 days.
#[inline]
pub const fn is_leap_year(year: i16) -> bool {
    // From: https://github.com/BurntSushi/jiff/pull/23
    let d = if year % 25 != 0 { 4 } else { 16 };
    (year % d) == 0
}

/// Return the number of days in the given year.
///
/// This is guaranteed to either return `365` or `366`.
#[inline]
pub const fn days_in_year(year: i16) -> i16 {
    if is_leap_year(year) {
        366
    } else {
        365
    }
}

/// Return the number of days in the given month.
///
/// This is guaranteed to return a value in the range `1..=31`.
#[inline]
pub const fn days_in_month(year: i16, month: i8) -> i8 {
    // From: https://github.com/BurntSushi/jiff/pull/23
    if month == 2 {
        if is_leap_year(year) {
            29
        } else {
            28
        }
    } else {
        30 | (month ^ month >> 3)
    }
}

/// Returns true if the given ISO 8601 week date year is a "long" year or not.
///
/// A "long" year is a year with 53 weeks. Otherwise, it's a "short" year
/// with 52 weeks.
#[inline]
pub const fn is_long_iso_week_year(year: i16) -> bool {
    // Inspired by: https://en.wikipedia.org/wiki/ISO_week_date#Weeks_per_year
    let last =
        unwrapr!(Date::new(year, 12, 31), "last day of year is always valid");
    let weekday = last.weekday();
    matches!(weekday, Weekday::Thursday)
        || (last.in_leap_year() && matches!(weekday, Weekday::Friday))
}

/// Returns the total number of weeks in the year of this ISO 8601 week
/// date.
///
/// It is guaranteed that the value returned is either 52 or 53. The
/// latter case occurs precisely when [`ISOWeekDate::in_long_year`]
/// returns `true`.
#[inline]
pub const fn weeks_in_iso_week_year(year: i16) -> i8 {
    if is_long_iso_week_year(year) {
        53
    } else {
        52
    }
}

#[cfg(test)]
mod tests {

    use super::*;

    static LEAPS: &[i16] = &[-400, -104, -4, 0, 1904, 2000, 2004, 2024];
    static NOT_LEAPS: &[i16] = &[-401, -100, -1, 1900, 1999, 2001, 2002, 2003];

    #[test]
    fn year_leap() {
        for &y in LEAPS {
            assert!(is_leap_year(y), "{y} should be a leap year");
        }
        for &y in NOT_LEAPS {
            assert!(!is_leap_year(y), "{y} should NOT be a leap year");
        }
    }

    #[test]
    fn year_days() {
        for &y in LEAPS {
            assert_eq!(days_in_year(y), 366, "{y} should be a leap year");
        }
        for &y in NOT_LEAPS {
            assert_eq!(days_in_year(y), 365, "{y} should NOT be a leap year");
        }
    }

    #[test]
    fn month_days() {
        assert_eq!(days_in_month(2001, 1), 31);
        assert_eq!(days_in_month(2001, 2), 28);
        assert_eq!(days_in_month(2001, 3), 31);
        assert_eq!(days_in_month(2001, 4), 30);
        assert_eq!(days_in_month(2001, 5), 31);
        assert_eq!(days_in_month(2001, 6), 30);
        assert_eq!(days_in_month(2001, 7), 31);
        assert_eq!(days_in_month(2001, 8), 31);
        assert_eq!(days_in_month(2001, 9), 30);
        assert_eq!(days_in_month(2001, 10), 31);
        assert_eq!(days_in_month(2001, 11), 30);
        assert_eq!(days_in_month(2001, 12), 31);

        for &y in LEAPS {
            assert_eq!(days_in_month(y, 2), 29, "{y} should be a leap year");
        }
        for &y in NOT_LEAPS {
            assert_eq!(
                days_in_month(y, 2),
                28,
                "{y} should NOT be a leap year"
            );
        }
    }
}
