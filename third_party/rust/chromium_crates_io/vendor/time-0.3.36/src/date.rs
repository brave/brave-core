//! The [`Date`] struct and its associated `impl`s.

#[cfg(feature = "formatting")]
use alloc::string::String;
use core::num::NonZeroI32;
use core::ops::{Add, Sub};
use core::time::Duration as StdDuration;
use core::{cmp, fmt};
#[cfg(feature = "formatting")]
use std::io;

use deranged::RangedI32;
use num_conv::prelude::*;
use powerfmt::ext::FormatterExt;
use powerfmt::smart_display::{self, FormatterOptions, Metadata, SmartDisplay};

use crate::convert::*;
use crate::ext::DigitCount;
#[cfg(feature = "formatting")]
use crate::formatting::Formattable;
use crate::internal_macros::{
    cascade, const_try, const_try_opt, div_floor, ensure_ranged, expect_opt, impl_add_assign,
    impl_sub_assign,
};
#[cfg(feature = "parsing")]
use crate::parsing::Parsable;
use crate::util::{days_in_year, days_in_year_month, is_leap_year, weeks_in_year};
use crate::{error, Duration, Month, PrimitiveDateTime, Time, Weekday};

type Year = RangedI32<MIN_YEAR, MAX_YEAR>;

/// The minimum valid year.
pub(crate) const MIN_YEAR: i32 = if cfg!(feature = "large-dates") {
    -999_999
} else {
    -9999
};
/// The maximum valid year.
pub(crate) const MAX_YEAR: i32 = if cfg!(feature = "large-dates") {
    999_999
} else {
    9999
};

/// Date in the proleptic Gregorian calendar.
///
/// By default, years between ±9999 inclusive are representable. This can be expanded to ±999,999
/// inclusive by enabling the `large-dates` crate feature. Doing so has performance implications
/// and introduces some ambiguities when parsing.
#[derive(Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct Date {
    /// Bitpacked field containing both the year and ordinal.
    // |     xx     | xxxxxxxxxxxxxxxxxxxxx | xxxxxxxxx |
    // |   2 bits   |        21 bits        |  9 bits   |
    // | unassigned |         year          |  ordinal  |
    // The year is 15 bits when `large-dates` is not enabled.
    value: NonZeroI32,
}

impl Date {
    /// The minimum valid `Date`.
    ///
    /// The value of this may vary depending on the feature flags enabled.
    // Safety: `ordinal` is not zero.
    #[allow(clippy::undocumented_unsafe_blocks)]
    pub const MIN: Self = unsafe { Self::__from_ordinal_date_unchecked(MIN_YEAR, 1) };

    /// The maximum valid `Date`.
    ///
    /// The value of this may vary depending on the feature flags enabled.
    // Safety: `ordinal` is not zero.
    #[allow(clippy::undocumented_unsafe_blocks)]
    pub const MAX: Self =
        unsafe { Self::__from_ordinal_date_unchecked(MAX_YEAR, days_in_year(MAX_YEAR)) };

    // region: constructors
    /// Construct a `Date` from the year and ordinal values, the validity of which must be
    /// guaranteed by the caller.
    ///
    /// # Safety
    ///
    /// `ordinal` must not be zero. `year` should be in the range `MIN_YEAR..=MAX_YEAR`, but this
    /// is not a safety invariant.
    #[doc(hidden)]
    pub const unsafe fn __from_ordinal_date_unchecked(year: i32, ordinal: u16) -> Self {
        debug_assert!(year >= MIN_YEAR);
        debug_assert!(year <= MAX_YEAR);
        debug_assert!(ordinal != 0);
        debug_assert!(ordinal <= days_in_year(year));

        Self {
            // Safety: The caller must guarantee that `ordinal` is not zero.
            value: unsafe { NonZeroI32::new_unchecked((year << 9) | ordinal as i32) },
        }
    }

    /// Attempt to create a `Date` from the year, month, and day.
    ///
    /// ```rust
    /// # use time::{Date, Month};
    /// assert!(Date::from_calendar_date(2019, Month::January, 1).is_ok());
    /// assert!(Date::from_calendar_date(2019, Month::December, 31).is_ok());
    /// ```
    ///
    /// ```rust
    /// # use time::{Date, Month};
    /// assert!(Date::from_calendar_date(2019, Month::February, 29).is_err()); // 2019 isn't a leap year.
    /// ```
    pub const fn from_calendar_date(
        year: i32,
        month: Month,
        day: u8,
    ) -> Result<Self, error::ComponentRange> {
        /// Cumulative days through the beginning of a month in both common and leap years.
        const DAYS_CUMULATIVE_COMMON_LEAP: [[u16; 12]; 2] = [
            [0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334],
            [0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335],
        ];

        ensure_ranged!(Year: year);
        match day {
            1..=28 => {}
            29..=31 if day <= days_in_year_month(year, month) => {}
            _ => {
                return Err(error::ComponentRange {
                    name: "day",
                    minimum: 1,
                    maximum: days_in_year_month(year, month) as _,
                    value: day as _,
                    conditional_range: true,
                });
            }
        }

        // Safety: `ordinal` is not zero.
        Ok(unsafe {
            Self::__from_ordinal_date_unchecked(
                year,
                DAYS_CUMULATIVE_COMMON_LEAP[is_leap_year(year) as usize][month as usize - 1]
                    + day as u16,
            )
        })
    }

    /// Attempt to create a `Date` from the year and ordinal day number.
    ///
    /// ```rust
    /// # use time::Date;
    /// assert!(Date::from_ordinal_date(2019, 1).is_ok());
    /// assert!(Date::from_ordinal_date(2019, 365).is_ok());
    /// ```
    ///
    /// ```rust
    /// # use time::Date;
    /// assert!(Date::from_ordinal_date(2019, 366).is_err()); // 2019 isn't a leap year.
    /// ```
    pub const fn from_ordinal_date(year: i32, ordinal: u16) -> Result<Self, error::ComponentRange> {
        ensure_ranged!(Year: year);
        match ordinal {
            1..=365 => {}
            366 if is_leap_year(year) => {}
            _ => {
                return Err(error::ComponentRange {
                    name: "ordinal",
                    minimum: 1,
                    maximum: days_in_year(year) as _,
                    value: ordinal as _,
                    conditional_range: true,
                });
            }
        }

        // Safety: `ordinal` is not zero.
        Ok(unsafe { Self::__from_ordinal_date_unchecked(year, ordinal) })
    }

    /// Attempt to create a `Date` from the ISO year, week, and weekday.
    ///
    /// ```rust
    /// # use time::{Date, Weekday::*};
    /// assert!(Date::from_iso_week_date(2019, 1, Monday).is_ok());
    /// assert!(Date::from_iso_week_date(2019, 1, Tuesday).is_ok());
    /// assert!(Date::from_iso_week_date(2020, 53, Friday).is_ok());
    /// ```
    ///
    /// ```rust
    /// # use time::{Date, Weekday::*};
    /// assert!(Date::from_iso_week_date(2019, 53, Monday).is_err()); // 2019 doesn't have 53 weeks.
    /// ```
    pub const fn from_iso_week_date(
        year: i32,
        week: u8,
        weekday: Weekday,
    ) -> Result<Self, error::ComponentRange> {
        ensure_ranged!(Year: year);
        match week {
            1..=52 => {}
            53 if week <= weeks_in_year(year) => {}
            _ => {
                return Err(error::ComponentRange {
                    name: "week",
                    minimum: 1,
                    maximum: weeks_in_year(year) as _,
                    value: week as _,
                    conditional_range: true,
                });
            }
        }

        let adj_year = year - 1;
        let raw = 365 * adj_year + div_floor!(adj_year, 4) - div_floor!(adj_year, 100)
            + div_floor!(adj_year, 400);
        let jan_4 = match (raw % 7) as i8 {
            -6 | 1 => 8,
            -5 | 2 => 9,
            -4 | 3 => 10,
            -3 | 4 => 4,
            -2 | 5 => 5,
            -1 | 6 => 6,
            _ => 7,
        };
        let ordinal = week as i16 * 7 + weekday.number_from_monday() as i16 - jan_4;

        Ok(if ordinal <= 0 {
            // Safety: `ordinal` is not zero.
            unsafe {
                Self::__from_ordinal_date_unchecked(
                    year - 1,
                    (ordinal as u16).wrapping_add(days_in_year(year - 1)),
                )
            }
        } else if ordinal > days_in_year(year) as i16 {
            // Safety: `ordinal` is not zero.
            unsafe {
                Self::__from_ordinal_date_unchecked(year + 1, ordinal as u16 - days_in_year(year))
            }
        } else {
            // Safety: `ordinal` is not zero.
            unsafe { Self::__from_ordinal_date_unchecked(year, ordinal as _) }
        })
    }

    /// Create a `Date` from the Julian day.
    ///
    /// The algorithm to perform this conversion is derived from one provided by Peter Baum; it is
    /// freely available [here](https://www.researchgate.net/publication/316558298_Date_Algorithms).
    ///
    /// ```rust
    /// # use time::Date;
    /// # use time_macros::date;
    /// assert_eq!(Date::from_julian_day(0), Ok(date!(-4713 - 11 - 24)));
    /// assert_eq!(Date::from_julian_day(2_451_545), Ok(date!(2000 - 01 - 01)));
    /// assert_eq!(Date::from_julian_day(2_458_485), Ok(date!(2019 - 01 - 01)));
    /// assert_eq!(Date::from_julian_day(2_458_849), Ok(date!(2019 - 12 - 31)));
    /// ```
    #[doc(alias = "from_julian_date")]
    pub const fn from_julian_day(julian_day: i32) -> Result<Self, error::ComponentRange> {
        type JulianDay = RangedI32<{ Date::MIN.to_julian_day() }, { Date::MAX.to_julian_day() }>;
        ensure_ranged!(JulianDay: julian_day);
        Ok(Self::from_julian_day_unchecked(julian_day))
    }

    /// Create a `Date` from the Julian day.
    ///
    /// This does not check the validity of the provided Julian day, and as such may result in an
    /// internally invalid value.
    #[doc(alias = "from_julian_date_unchecked")]
    pub(crate) const fn from_julian_day_unchecked(julian_day: i32) -> Self {
        debug_assert!(julian_day >= Self::MIN.to_julian_day());
        debug_assert!(julian_day <= Self::MAX.to_julian_day());

        // To avoid a potential overflow, the value may need to be widened for some arithmetic.

        let z = julian_day - 1_721_119;
        let (mut year, mut ordinal) = if julian_day < -19_752_948 || julian_day > 23_195_514 {
            let g = 100 * z as i64 - 25;
            let a = (g / 3_652_425) as i32;
            let b = a - a / 4;
            let year = div_floor!(100 * b as i64 + g, 36525) as i32;
            let ordinal = (b + z - div_floor!(36525 * year as i64, 100) as i32) as _;
            (year, ordinal)
        } else {
            let g = 100 * z - 25;
            let a = g / 3_652_425;
            let b = a - a / 4;
            let year = div_floor!(100 * b + g, 36525);
            let ordinal = (b + z - div_floor!(36525 * year, 100)) as _;
            (year, ordinal)
        };

        if is_leap_year(year) {
            ordinal += 60;
            cascade!(ordinal in 1..367 => year);
        } else {
            ordinal += 59;
            cascade!(ordinal in 1..366 => year);
        }

        // Safety: `ordinal` is not zero.
        unsafe { Self::__from_ordinal_date_unchecked(year, ordinal) }
    }
    // endregion constructors

    // region: getters
    /// Get the year of the date.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).year(), 2019);
    /// assert_eq!(date!(2019 - 12 - 31).year(), 2019);
    /// assert_eq!(date!(2020 - 01 - 01).year(), 2020);
    /// ```
    pub const fn year(self) -> i32 {
        self.value.get() >> 9
    }

    /// Get the month.
    ///
    /// ```rust
    /// # use time::Month;
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).month(), Month::January);
    /// assert_eq!(date!(2019 - 12 - 31).month(), Month::December);
    /// ```
    pub const fn month(self) -> Month {
        self.month_day().0
    }

    /// Get the day of the month.
    ///
    /// The returned value will always be in the range `1..=31`.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).day(), 1);
    /// assert_eq!(date!(2019 - 12 - 31).day(), 31);
    /// ```
    pub const fn day(self) -> u8 {
        self.month_day().1
    }

    /// Get the month and day. This is more efficient than fetching the components individually.
    // For whatever reason, rustc has difficulty optimizing this function. It's significantly faster
    // to write the statements out by hand.
    pub(crate) const fn month_day(self) -> (Month, u8) {
        /// The number of days up to and including the given month. Common years
        /// are first, followed by leap years.
        const CUMULATIVE_DAYS_IN_MONTH_COMMON_LEAP: [[u16; 11]; 2] = [
            [31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334],
            [31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335],
        ];

        let days = CUMULATIVE_DAYS_IN_MONTH_COMMON_LEAP[is_leap_year(self.year()) as usize];
        let ordinal = self.ordinal();

        if ordinal > days[10] {
            (Month::December, (ordinal - days[10]) as _)
        } else if ordinal > days[9] {
            (Month::November, (ordinal - days[9]) as _)
        } else if ordinal > days[8] {
            (Month::October, (ordinal - days[8]) as _)
        } else if ordinal > days[7] {
            (Month::September, (ordinal - days[7]) as _)
        } else if ordinal > days[6] {
            (Month::August, (ordinal - days[6]) as _)
        } else if ordinal > days[5] {
            (Month::July, (ordinal - days[5]) as _)
        } else if ordinal > days[4] {
            (Month::June, (ordinal - days[4]) as _)
        } else if ordinal > days[3] {
            (Month::May, (ordinal - days[3]) as _)
        } else if ordinal > days[2] {
            (Month::April, (ordinal - days[2]) as _)
        } else if ordinal > days[1] {
            (Month::March, (ordinal - days[1]) as _)
        } else if ordinal > days[0] {
            (Month::February, (ordinal - days[0]) as _)
        } else {
            (Month::January, ordinal as _)
        }
    }

    /// Get the day of the year.
    ///
    /// The returned value will always be in the range `1..=366` (`1..=365` for common years).
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).ordinal(), 1);
    /// assert_eq!(date!(2019 - 12 - 31).ordinal(), 365);
    /// ```
    pub const fn ordinal(self) -> u16 {
        (self.value.get() & 0x1FF) as _
    }

    /// Get the ISO 8601 year and week number.
    pub(crate) const fn iso_year_week(self) -> (i32, u8) {
        let (year, ordinal) = self.to_ordinal_date();

        match ((ordinal + 10 - self.weekday().number_from_monday() as u16) / 7) as _ {
            0 => (year - 1, weeks_in_year(year - 1)),
            53 if weeks_in_year(year) == 52 => (year + 1, 1),
            week => (year, week),
        }
    }

    /// Get the ISO week number.
    ///
    /// The returned value will always be in the range `1..=53`.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).iso_week(), 1);
    /// assert_eq!(date!(2019 - 10 - 04).iso_week(), 40);
    /// assert_eq!(date!(2020 - 01 - 01).iso_week(), 1);
    /// assert_eq!(date!(2020 - 12 - 31).iso_week(), 53);
    /// assert_eq!(date!(2021 - 01 - 01).iso_week(), 53);
    /// ```
    pub const fn iso_week(self) -> u8 {
        self.iso_year_week().1
    }

    /// Get the week number where week 1 begins on the first Sunday.
    ///
    /// The returned value will always be in the range `0..=53`.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).sunday_based_week(), 0);
    /// assert_eq!(date!(2020 - 01 - 01).sunday_based_week(), 0);
    /// assert_eq!(date!(2020 - 12 - 31).sunday_based_week(), 52);
    /// assert_eq!(date!(2021 - 01 - 01).sunday_based_week(), 0);
    /// ```
    pub const fn sunday_based_week(self) -> u8 {
        ((self.ordinal() as i16 - self.weekday().number_days_from_sunday() as i16 + 6) / 7) as _
    }

    /// Get the week number where week 1 begins on the first Monday.
    ///
    /// The returned value will always be in the range `0..=53`.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).monday_based_week(), 0);
    /// assert_eq!(date!(2020 - 01 - 01).monday_based_week(), 0);
    /// assert_eq!(date!(2020 - 12 - 31).monday_based_week(), 52);
    /// assert_eq!(date!(2021 - 01 - 01).monday_based_week(), 0);
    /// ```
    pub const fn monday_based_week(self) -> u8 {
        ((self.ordinal() as i16 - self.weekday().number_days_from_monday() as i16 + 6) / 7) as _
    }

    /// Get the year, month, and day.
    ///
    /// ```rust
    /// # use time::Month;
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2019 - 01 - 01).to_calendar_date(),
    ///     (2019, Month::January, 1)
    /// );
    /// ```
    pub const fn to_calendar_date(self) -> (i32, Month, u8) {
        let (month, day) = self.month_day();
        (self.year(), month, day)
    }

    /// Get the year and ordinal day number.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).to_ordinal_date(), (2019, 1));
    /// ```
    pub const fn to_ordinal_date(self) -> (i32, u16) {
        (self.year(), self.ordinal())
    }

    /// Get the ISO 8601 year, week number, and weekday.
    ///
    /// ```rust
    /// # use time::Weekday::*;
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).to_iso_week_date(), (2019, 1, Tuesday));
    /// assert_eq!(date!(2019 - 10 - 04).to_iso_week_date(), (2019, 40, Friday));
    /// assert_eq!(
    ///     date!(2020 - 01 - 01).to_iso_week_date(),
    ///     (2020, 1, Wednesday)
    /// );
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).to_iso_week_date(),
    ///     (2020, 53, Thursday)
    /// );
    /// assert_eq!(date!(2021 - 01 - 01).to_iso_week_date(), (2020, 53, Friday));
    /// ```
    pub const fn to_iso_week_date(self) -> (i32, u8, Weekday) {
        let (year, ordinal) = self.to_ordinal_date();
        let weekday = self.weekday();

        match ((ordinal + 10 - self.weekday().number_from_monday() as u16) / 7) as _ {
            0 => (year - 1, weeks_in_year(year - 1), weekday),
            53 if weeks_in_year(year) == 52 => (year + 1, 1, weekday),
            week => (year, week, weekday),
        }
    }

    /// Get the weekday.
    ///
    /// ```rust
    /// # use time::Weekday::*;
    /// # use time_macros::date;
    /// assert_eq!(date!(2019 - 01 - 01).weekday(), Tuesday);
    /// assert_eq!(date!(2019 - 02 - 01).weekday(), Friday);
    /// assert_eq!(date!(2019 - 03 - 01).weekday(), Friday);
    /// assert_eq!(date!(2019 - 04 - 01).weekday(), Monday);
    /// assert_eq!(date!(2019 - 05 - 01).weekday(), Wednesday);
    /// assert_eq!(date!(2019 - 06 - 01).weekday(), Saturday);
    /// assert_eq!(date!(2019 - 07 - 01).weekday(), Monday);
    /// assert_eq!(date!(2019 - 08 - 01).weekday(), Thursday);
    /// assert_eq!(date!(2019 - 09 - 01).weekday(), Sunday);
    /// assert_eq!(date!(2019 - 10 - 01).weekday(), Tuesday);
    /// assert_eq!(date!(2019 - 11 - 01).weekday(), Friday);
    /// assert_eq!(date!(2019 - 12 - 01).weekday(), Sunday);
    /// ```
    pub const fn weekday(self) -> Weekday {
        match self.to_julian_day() % 7 {
            -6 | 1 => Weekday::Tuesday,
            -5 | 2 => Weekday::Wednesday,
            -4 | 3 => Weekday::Thursday,
            -3 | 4 => Weekday::Friday,
            -2 | 5 => Weekday::Saturday,
            -1 | 6 => Weekday::Sunday,
            val => {
                debug_assert!(val == 0);
                Weekday::Monday
            }
        }
    }

    /// Get the next calendar date.
    ///
    /// ```rust
    /// # use time::Date;
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2019 - 01 - 01).next_day(),
    ///     Some(date!(2019 - 01 - 02))
    /// );
    /// assert_eq!(
    ///     date!(2019 - 01 - 31).next_day(),
    ///     Some(date!(2019 - 02 - 01))
    /// );
    /// assert_eq!(
    ///     date!(2019 - 12 - 31).next_day(),
    ///     Some(date!(2020 - 01 - 01))
    /// );
    /// assert_eq!(Date::MAX.next_day(), None);
    /// ```
    pub const fn next_day(self) -> Option<Self> {
        if self.ordinal() == 366 || (self.ordinal() == 365 && !is_leap_year(self.year())) {
            if self.value.get() == Self::MAX.value.get() {
                None
            } else {
                // Safety: `ordinal` is not zero.
                unsafe { Some(Self::__from_ordinal_date_unchecked(self.year() + 1, 1)) }
            }
        } else {
            Some(Self {
                // Safety: `ordinal` is not zero.
                value: unsafe { NonZeroI32::new_unchecked(self.value.get() + 1) },
            })
        }
    }

    /// Get the previous calendar date.
    ///
    /// ```rust
    /// # use time::Date;
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2019 - 01 - 02).previous_day(),
    ///     Some(date!(2019 - 01 - 01))
    /// );
    /// assert_eq!(
    ///     date!(2019 - 02 - 01).previous_day(),
    ///     Some(date!(2019 - 01 - 31))
    /// );
    /// assert_eq!(
    ///     date!(2020 - 01 - 01).previous_day(),
    ///     Some(date!(2019 - 12 - 31))
    /// );
    /// assert_eq!(Date::MIN.previous_day(), None);
    /// ```
    pub const fn previous_day(self) -> Option<Self> {
        if self.ordinal() != 1 {
            Some(Self {
                // Safety: `ordinal` is not zero.
                value: unsafe { NonZeroI32::new_unchecked(self.value.get() - 1) },
            })
        } else if self.value.get() == Self::MIN.value.get() {
            None
        } else {
            // Safety: `ordinal` is not zero.
            Some(unsafe {
                Self::__from_ordinal_date_unchecked(self.year() - 1, days_in_year(self.year() - 1))
            })
        }
    }

    /// Calculates the first occurrence of a weekday that is strictly later than a given `Date`.
    ///
    /// # Panics
    /// Panics if an overflow occurred.
    ///
    /// # Examples
    /// ```
    /// # use time::Weekday;
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2023 - 06 - 28).next_occurrence(Weekday::Monday),
    ///     date!(2023 - 07 - 03)
    /// );
    /// assert_eq!(
    ///     date!(2023 - 06 - 19).next_occurrence(Weekday::Monday),
    ///     date!(2023 - 06 - 26)
    /// );
    /// ```
    pub const fn next_occurrence(self, weekday: Weekday) -> Self {
        expect_opt!(
            self.checked_next_occurrence(weekday),
            "overflow calculating the next occurrence of a weekday"
        )
    }

    /// Calculates the first occurrence of a weekday that is strictly earlier than a given `Date`.
    ///
    /// # Panics
    /// Panics if an overflow occurred.
    ///
    /// # Examples
    /// ```
    /// # use time::Weekday;
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2023 - 06 - 28).prev_occurrence(Weekday::Monday),
    ///     date!(2023 - 06 - 26)
    /// );
    /// assert_eq!(
    ///     date!(2023 - 06 - 19).prev_occurrence(Weekday::Monday),
    ///     date!(2023 - 06 - 12)
    /// );
    /// ```
    pub const fn prev_occurrence(self, weekday: Weekday) -> Self {
        expect_opt!(
            self.checked_prev_occurrence(weekday),
            "overflow calculating the previous occurrence of a weekday"
        )
    }

    /// Calculates the `n`th occurrence of a weekday that is strictly later than a given `Date`.
    ///
    /// # Panics
    /// Panics if an overflow occurred or if `n == 0`.
    ///
    /// # Examples
    /// ```
    /// # use time::Weekday;
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2023 - 06 - 25).nth_next_occurrence(Weekday::Monday, 5),
    ///     date!(2023 - 07 - 24)
    /// );
    /// assert_eq!(
    ///     date!(2023 - 06 - 26).nth_next_occurrence(Weekday::Monday, 5),
    ///     date!(2023 - 07 - 31)
    /// );
    /// ```
    pub const fn nth_next_occurrence(self, weekday: Weekday, n: u8) -> Self {
        expect_opt!(
            self.checked_nth_next_occurrence(weekday, n),
            "overflow calculating the next occurrence of a weekday"
        )
    }

    /// Calculates the `n`th occurrence of a weekday that is strictly earlier than a given `Date`.
    ///
    /// # Panics
    /// Panics if an overflow occurred or if `n == 0`.
    ///
    /// # Examples
    /// ```
    /// # use time::Weekday;
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2023 - 06 - 27).nth_prev_occurrence(Weekday::Monday, 3),
    ///     date!(2023 - 06 - 12)
    /// );
    /// assert_eq!(
    ///     date!(2023 - 06 - 26).nth_prev_occurrence(Weekday::Monday, 3),
    ///     date!(2023 - 06 - 05)
    /// );
    /// ```
    pub const fn nth_prev_occurrence(self, weekday: Weekday, n: u8) -> Self {
        expect_opt!(
            self.checked_nth_prev_occurrence(weekday, n),
            "overflow calculating the previous occurrence of a weekday"
        )
    }

    /// Get the Julian day for the date.
    ///
    /// The algorithm to perform this conversion is derived from one provided by Peter Baum; it is
    /// freely available [here](https://www.researchgate.net/publication/316558298_Date_Algorithms).
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(date!(-4713 - 11 - 24).to_julian_day(), 0);
    /// assert_eq!(date!(2000 - 01 - 01).to_julian_day(), 2_451_545);
    /// assert_eq!(date!(2019 - 01 - 01).to_julian_day(), 2_458_485);
    /// assert_eq!(date!(2019 - 12 - 31).to_julian_day(), 2_458_849);
    /// ```
    pub const fn to_julian_day(self) -> i32 {
        let year = self.year() - 1;
        let ordinal = self.ordinal() as i32;

        ordinal + 365 * year + div_floor!(year, 4) - div_floor!(year, 100)
            + div_floor!(year, 400)
            + 1_721_425
    }
    // endregion getters

    // region: checked arithmetic
    /// Computes `self + duration`, returning `None` if an overflow occurred.
    ///
    /// ```rust
    /// # use time::{Date, ext::NumericalDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MAX.checked_add(1.days()), None);
    /// assert_eq!(Date::MIN.checked_add((-2).days()), None);
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_add(2.days()),
    ///     Some(date!(2021 - 01 - 02))
    /// );
    /// ```
    ///
    /// # Note
    ///
    /// This function only takes whole days into account.
    ///
    /// ```rust
    /// # use time::{Date, ext::NumericalDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MAX.checked_add(23.hours()), Some(Date::MAX));
    /// assert_eq!(Date::MIN.checked_add((-23).hours()), Some(Date::MIN));
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_add(23.hours()),
    ///     Some(date!(2020 - 12 - 31))
    /// );
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_add(47.hours()),
    ///     Some(date!(2021 - 01 - 01))
    /// );
    /// ```
    pub const fn checked_add(self, duration: Duration) -> Option<Self> {
        let whole_days = duration.whole_days();
        if whole_days < i32::MIN as i64 || whole_days > i32::MAX as i64 {
            return None;
        }

        let julian_day = const_try_opt!(self.to_julian_day().checked_add(whole_days as _));
        if let Ok(date) = Self::from_julian_day(julian_day) {
            Some(date)
        } else {
            None
        }
    }

    /// Computes `self + duration`, returning `None` if an overflow occurred.
    ///
    /// ```rust
    /// # use time::{Date, ext::NumericalStdDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MAX.checked_add_std(1.std_days()), None);
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_add_std(2.std_days()),
    ///     Some(date!(2021 - 01 - 02))
    /// );
    /// ```
    ///
    /// # Note
    ///
    /// This function only takes whole days into account.
    ///
    /// ```rust
    /// # use time::{Date, ext::NumericalStdDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MAX.checked_add_std(23.std_hours()), Some(Date::MAX));
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_add_std(23.std_hours()),
    ///     Some(date!(2020 - 12 - 31))
    /// );
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_add_std(47.std_hours()),
    ///     Some(date!(2021 - 01 - 01))
    /// );
    /// ```
    pub const fn checked_add_std(self, duration: StdDuration) -> Option<Self> {
        let whole_days = duration.as_secs() / Second::per(Day) as u64;
        if whole_days > i32::MAX as u64 {
            return None;
        }

        let julian_day = const_try_opt!(self.to_julian_day().checked_add(whole_days as _));
        if let Ok(date) = Self::from_julian_day(julian_day) {
            Some(date)
        } else {
            None
        }
    }

    /// Computes `self - duration`, returning `None` if an overflow occurred.
    ///
    /// ```
    /// # use time::{Date, ext::NumericalDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MAX.checked_sub((-2).days()), None);
    /// assert_eq!(Date::MIN.checked_sub(1.days()), None);
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_sub(2.days()),
    ///     Some(date!(2020 - 12 - 29))
    /// );
    /// ```
    ///
    /// # Note
    ///
    /// This function only takes whole days into account.
    ///
    /// ```
    /// # use time::{Date, ext::NumericalDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MAX.checked_sub((-23).hours()), Some(Date::MAX));
    /// assert_eq!(Date::MIN.checked_sub(23.hours()), Some(Date::MIN));
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_sub(23.hours()),
    ///     Some(date!(2020 - 12 - 31))
    /// );
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_sub(47.hours()),
    ///     Some(date!(2020 - 12 - 30))
    /// );
    /// ```
    pub const fn checked_sub(self, duration: Duration) -> Option<Self> {
        let whole_days = duration.whole_days();
        if whole_days < i32::MIN as i64 || whole_days > i32::MAX as i64 {
            return None;
        }

        let julian_day = const_try_opt!(self.to_julian_day().checked_sub(whole_days as _));
        if let Ok(date) = Self::from_julian_day(julian_day) {
            Some(date)
        } else {
            None
        }
    }

    /// Computes `self - duration`, returning `None` if an overflow occurred.
    ///
    /// ```
    /// # use time::{Date, ext::NumericalStdDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MIN.checked_sub_std(1.std_days()), None);
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_sub_std(2.std_days()),
    ///     Some(date!(2020 - 12 - 29))
    /// );
    /// ```
    ///
    /// # Note
    ///
    /// This function only takes whole days into account.
    ///
    /// ```
    /// # use time::{Date, ext::NumericalStdDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MIN.checked_sub_std(23.std_hours()), Some(Date::MIN));
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_sub_std(23.std_hours()),
    ///     Some(date!(2020 - 12 - 31))
    /// );
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).checked_sub_std(47.std_hours()),
    ///     Some(date!(2020 - 12 - 30))
    /// );
    /// ```
    pub const fn checked_sub_std(self, duration: StdDuration) -> Option<Self> {
        let whole_days = duration.as_secs() / Second::per(Day) as u64;
        if whole_days > i32::MAX as u64 {
            return None;
        }

        let julian_day = const_try_opt!(self.to_julian_day().checked_sub(whole_days as _));
        if let Ok(date) = Self::from_julian_day(julian_day) {
            Some(date)
        } else {
            None
        }
    }

    /// Calculates the first occurrence of a weekday that is strictly later than a given `Date`.
    /// Returns `None` if an overflow occurred.
    pub(crate) const fn checked_next_occurrence(self, weekday: Weekday) -> Option<Self> {
        let day_diff = match weekday as i8 - self.weekday() as i8 {
            1 | -6 => 1,
            2 | -5 => 2,
            3 | -4 => 3,
            4 | -3 => 4,
            5 | -2 => 5,
            6 | -1 => 6,
            val => {
                debug_assert!(val == 0);
                7
            }
        };

        self.checked_add(Duration::days(day_diff))
    }

    /// Calculates the first occurrence of a weekday that is strictly earlier than a given `Date`.
    /// Returns `None` if an overflow occurred.
    pub(crate) const fn checked_prev_occurrence(self, weekday: Weekday) -> Option<Self> {
        let day_diff = match weekday as i8 - self.weekday() as i8 {
            1 | -6 => 6,
            2 | -5 => 5,
            3 | -4 => 4,
            4 | -3 => 3,
            5 | -2 => 2,
            6 | -1 => 1,
            val => {
                debug_assert!(val == 0);
                7
            }
        };

        self.checked_sub(Duration::days(day_diff))
    }

    /// Calculates the `n`th occurrence of a weekday that is strictly later than a given `Date`.
    /// Returns `None` if an overflow occurred or if `n == 0`.
    pub(crate) const fn checked_nth_next_occurrence(self, weekday: Weekday, n: u8) -> Option<Self> {
        if n == 0 {
            return None;
        }

        const_try_opt!(self.checked_next_occurrence(weekday))
            .checked_add(Duration::weeks(n as i64 - 1))
    }

    /// Calculates the `n`th occurrence of a weekday that is strictly earlier than a given `Date`.
    /// Returns `None` if an overflow occurred or if `n == 0`.
    pub(crate) const fn checked_nth_prev_occurrence(self, weekday: Weekday, n: u8) -> Option<Self> {
        if n == 0 {
            return None;
        }

        const_try_opt!(self.checked_prev_occurrence(weekday))
            .checked_sub(Duration::weeks(n as i64 - 1))
    }
    // endregion: checked arithmetic

    // region: saturating arithmetic
    /// Computes `self + duration`, saturating value on overflow.
    ///
    /// ```rust
    /// # use time::{Date, ext::NumericalDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MAX.saturating_add(1.days()), Date::MAX);
    /// assert_eq!(Date::MIN.saturating_add((-2).days()), Date::MIN);
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).saturating_add(2.days()),
    ///     date!(2021 - 01 - 02)
    /// );
    /// ```
    ///
    /// # Note
    ///
    /// This function only takes whole days into account.
    ///
    /// ```rust
    /// # use time::ext::NumericalDuration;
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).saturating_add(23.hours()),
    ///     date!(2020 - 12 - 31)
    /// );
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).saturating_add(47.hours()),
    ///     date!(2021 - 01 - 01)
    /// );
    /// ```
    pub const fn saturating_add(self, duration: Duration) -> Self {
        if let Some(datetime) = self.checked_add(duration) {
            datetime
        } else if duration.is_negative() {
            Self::MIN
        } else {
            debug_assert!(duration.is_positive());
            Self::MAX
        }
    }

    /// Computes `self - duration`, saturating value on overflow.
    ///
    /// ```
    /// # use time::{Date, ext::NumericalDuration};
    /// # use time_macros::date;
    /// assert_eq!(Date::MAX.saturating_sub((-2).days()), Date::MAX);
    /// assert_eq!(Date::MIN.saturating_sub(1.days()), Date::MIN);
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).saturating_sub(2.days()),
    ///     date!(2020 - 12 - 29)
    /// );
    /// ```
    ///
    /// # Note
    ///
    /// This function only takes whole days into account.
    ///
    /// ```
    /// # use time::ext::NumericalDuration;
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).saturating_sub(23.hours()),
    ///     date!(2020 - 12 - 31)
    /// );
    /// assert_eq!(
    ///     date!(2020 - 12 - 31).saturating_sub(47.hours()),
    ///     date!(2020 - 12 - 30)
    /// );
    /// ```
    pub const fn saturating_sub(self, duration: Duration) -> Self {
        if let Some(datetime) = self.checked_sub(duration) {
            datetime
        } else if duration.is_negative() {
            Self::MAX
        } else {
            debug_assert!(duration.is_positive());
            Self::MIN
        }
    }
    // region: saturating arithmetic

    // region: replacement
    /// Replace the year. The month and day will be unchanged.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2022 - 02 - 18).replace_year(2019),
    ///     Ok(date!(2019 - 02 - 18))
    /// );
    /// assert!(date!(2022 - 02 - 18).replace_year(-1_000_000_000).is_err()); // -1_000_000_000 isn't a valid year
    /// assert!(date!(2022 - 02 - 18).replace_year(1_000_000_000).is_err()); // 1_000_000_000 isn't a valid year
    /// ```
    #[must_use = "This method does not mutate the original `Date`."]
    pub const fn replace_year(self, year: i32) -> Result<Self, error::ComponentRange> {
        ensure_ranged!(Year: year);

        let ordinal = self.ordinal();

        // Dates in January and February are unaffected by leap years.
        if ordinal <= 59 {
            // Safety: `ordinal` is not zero.
            return Ok(unsafe { Self::__from_ordinal_date_unchecked(year, ordinal) });
        }

        match (is_leap_year(self.year()), is_leap_year(year)) {
            (false, false) | (true, true) => {
                // Safety: `ordinal` is not zero.
                Ok(unsafe { Self::__from_ordinal_date_unchecked(year, ordinal) })
            }
            // February 29 does not exist in common years.
            (true, false) if ordinal == 60 => Err(error::ComponentRange {
                name: "day",
                value: 29,
                minimum: 1,
                maximum: 28,
                conditional_range: true,
            }),
            // We're going from a common year to a leap year. Shift dates in March and later by
            // one day.
            // Safety: `ordinal` is not zero.
            (false, true) => Ok(unsafe { Self::__from_ordinal_date_unchecked(year, ordinal + 1) }),
            // We're going from a leap year to a common year. Shift dates in January and
            // February by one day.
            // Safety: `ordinal` is not zero.
            (true, false) => Ok(unsafe { Self::__from_ordinal_date_unchecked(year, ordinal - 1) }),
        }
    }

    /// Replace the month of the year.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// # use time::Month;
    /// assert_eq!(
    ///     date!(2022 - 02 - 18).replace_month(Month::January),
    ///     Ok(date!(2022 - 01 - 18))
    /// );
    /// assert!(
    ///     date!(2022 - 01 - 30)
    ///         .replace_month(Month::February)
    ///         .is_err()
    /// ); // 30 isn't a valid day in February
    /// ```
    #[must_use = "This method does not mutate the original `Date`."]
    pub const fn replace_month(self, month: Month) -> Result<Self, error::ComponentRange> {
        let (year, _, day) = self.to_calendar_date();
        Self::from_calendar_date(year, month, day)
    }

    /// Replace the day of the month.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(
    ///     date!(2022 - 02 - 18).replace_day(1),
    ///     Ok(date!(2022 - 02 - 01))
    /// );
    /// assert!(date!(2022 - 02 - 18).replace_day(0).is_err()); // 0 isn't a valid day
    /// assert!(date!(2022 - 02 - 18).replace_day(30).is_err()); // 30 isn't a valid day in February
    /// ```
    #[must_use = "This method does not mutate the original `Date`."]
    pub const fn replace_day(self, day: u8) -> Result<Self, error::ComponentRange> {
        match day {
            1..=28 => {}
            29..=31 if day <= days_in_year_month(self.year(), self.month()) => {}
            _ => {
                return Err(error::ComponentRange {
                    name: "day",
                    minimum: 1,
                    maximum: days_in_year_month(self.year(), self.month()) as _,
                    value: day as _,
                    conditional_range: true,
                });
            }
        }

        // Safety: `ordinal` is not zero.
        Ok(unsafe {
            Self::__from_ordinal_date_unchecked(
                self.year(),
                (self.ordinal() as i16 - self.day() as i16 + day as i16) as _,
            )
        })
    }

    /// Replace the day of the year.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert_eq!(date!(2022 - 049).replace_ordinal(1), Ok(date!(2022 - 001)));
    /// assert!(date!(2022 - 049).replace_ordinal(0).is_err()); // 0 isn't a valid ordinal
    /// assert!(date!(2022 - 049).replace_ordinal(366).is_err()); // 2022 isn't a leap year
    /// ````
    #[must_use = "This method does not mutate the original `Date`."]
    pub const fn replace_ordinal(self, ordinal: u16) -> Result<Self, error::ComponentRange> {
        match ordinal {
            1..=365 => {}
            366 if is_leap_year(self.year()) => {}
            _ => {
                return Err(error::ComponentRange {
                    name: "ordinal",
                    minimum: 1,
                    maximum: days_in_year(self.year()) as _,
                    value: ordinal as _,
                    conditional_range: true,
                });
            }
        }

        // Safety: `ordinal` is in range.
        Ok(unsafe { Self::__from_ordinal_date_unchecked(self.year(), ordinal) })
    }
    // endregion replacement
}

// region: attach time
/// Methods to add a [`Time`] component, resulting in a [`PrimitiveDateTime`].
impl Date {
    /// Create a [`PrimitiveDateTime`] using the existing date. The [`Time`] component will be set
    /// to midnight.
    ///
    /// ```rust
    /// # use time_macros::{date, datetime};
    /// assert_eq!(date!(1970-01-01).midnight(), datetime!(1970-01-01 0:00));
    /// ```
    pub const fn midnight(self) -> PrimitiveDateTime {
        PrimitiveDateTime::new(self, Time::MIDNIGHT)
    }

    /// Create a [`PrimitiveDateTime`] using the existing date and the provided [`Time`].
    ///
    /// ```rust
    /// # use time_macros::{date, datetime, time};
    /// assert_eq!(
    ///     date!(1970-01-01).with_time(time!(0:00)),
    ///     datetime!(1970-01-01 0:00),
    /// );
    /// ```
    pub const fn with_time(self, time: Time) -> PrimitiveDateTime {
        PrimitiveDateTime::new(self, time)
    }

    /// Attempt to create a [`PrimitiveDateTime`] using the existing date and the provided time.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert!(date!(1970 - 01 - 01).with_hms(0, 0, 0).is_ok());
    /// assert!(date!(1970 - 01 - 01).with_hms(24, 0, 0).is_err());
    /// ```
    pub const fn with_hms(
        self,
        hour: u8,
        minute: u8,
        second: u8,
    ) -> Result<PrimitiveDateTime, error::ComponentRange> {
        Ok(PrimitiveDateTime::new(
            self,
            const_try!(Time::from_hms(hour, minute, second)),
        ))
    }

    /// Attempt to create a [`PrimitiveDateTime`] using the existing date and the provided time.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert!(date!(1970 - 01 - 01).with_hms_milli(0, 0, 0, 0).is_ok());
    /// assert!(date!(1970 - 01 - 01).with_hms_milli(24, 0, 0, 0).is_err());
    /// ```
    pub const fn with_hms_milli(
        self,
        hour: u8,
        minute: u8,
        second: u8,
        millisecond: u16,
    ) -> Result<PrimitiveDateTime, error::ComponentRange> {
        Ok(PrimitiveDateTime::new(
            self,
            const_try!(Time::from_hms_milli(hour, minute, second, millisecond)),
        ))
    }

    /// Attempt to create a [`PrimitiveDateTime`] using the existing date and the provided time.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert!(date!(1970 - 01 - 01).with_hms_micro(0, 0, 0, 0).is_ok());
    /// assert!(date!(1970 - 01 - 01).with_hms_micro(24, 0, 0, 0).is_err());
    /// ```
    pub const fn with_hms_micro(
        self,
        hour: u8,
        minute: u8,
        second: u8,
        microsecond: u32,
    ) -> Result<PrimitiveDateTime, error::ComponentRange> {
        Ok(PrimitiveDateTime::new(
            self,
            const_try!(Time::from_hms_micro(hour, minute, second, microsecond)),
        ))
    }

    /// Attempt to create a [`PrimitiveDateTime`] using the existing date and the provided time.
    ///
    /// ```rust
    /// # use time_macros::date;
    /// assert!(date!(1970 - 01 - 01).with_hms_nano(0, 0, 0, 0).is_ok());
    /// assert!(date!(1970 - 01 - 01).with_hms_nano(24, 0, 0, 0).is_err());
    /// ```
    pub const fn with_hms_nano(
        self,
        hour: u8,
        minute: u8,
        second: u8,
        nanosecond: u32,
    ) -> Result<PrimitiveDateTime, error::ComponentRange> {
        Ok(PrimitiveDateTime::new(
            self,
            const_try!(Time::from_hms_nano(hour, minute, second, nanosecond)),
        ))
    }
}
// endregion attach time

// region: formatting & parsing
#[cfg(feature = "formatting")]
impl Date {
    /// Format the `Date` using the provided [format description](crate::format_description).
    pub fn format_into(
        self,
        output: &mut impl io::Write,
        format: &(impl Formattable + ?Sized),
    ) -> Result<usize, error::Format> {
        format.format_into(output, Some(self), None, None)
    }

    /// Format the `Date` using the provided [format description](crate::format_description).
    ///
    /// ```rust
    /// # use time::{format_description};
    /// # use time_macros::date;
    /// let format = format_description::parse("[year]-[month]-[day]")?;
    /// assert_eq!(date!(2020 - 01 - 02).format(&format)?, "2020-01-02");
    /// # Ok::<_, time::Error>(())
    /// ```
    pub fn format(self, format: &(impl Formattable + ?Sized)) -> Result<String, error::Format> {
        format.format(Some(self), None, None)
    }
}

#[cfg(feature = "parsing")]
impl Date {
    /// Parse a `Date` from the input using the provided [format
    /// description](crate::format_description).
    ///
    /// ```rust
    /// # use time::Date;
    /// # use time_macros::{date, format_description};
    /// let format = format_description!("[year]-[month]-[day]");
    /// assert_eq!(Date::parse("2020-01-02", &format)?, date!(2020 - 01 - 02));
    /// # Ok::<_, time::Error>(())
    /// ```
    pub fn parse(
        input: &str,
        description: &(impl Parsable + ?Sized),
    ) -> Result<Self, error::Parse> {
        description.parse_date(input.as_bytes())
    }
}

mod private {
    #[non_exhaustive]
    #[derive(Debug, Clone, Copy)]
    pub struct DateMetadata {
        /// The width of the year component, including the sign.
        pub(super) year_width: u8,
        /// Whether the sign should be displayed.
        pub(super) display_sign: bool,
        pub(super) year: i32,
        pub(super) month: u8,
        pub(super) day: u8,
    }
}
use private::DateMetadata;

impl SmartDisplay for Date {
    type Metadata = DateMetadata;

    fn metadata(&self, _: FormatterOptions) -> Metadata<Self> {
        let (year, month, day) = self.to_calendar_date();

        // There is a minimum of four digits for any year.
        let mut year_width = cmp::max(year.unsigned_abs().num_digits(), 4);
        let display_sign = if !(0..10_000).contains(&year) {
            // An extra character is required for the sign.
            year_width += 1;
            true
        } else {
            false
        };

        let formatted_width = year_width.extend::<usize>()
            + smart_display::padded_width_of!(
                "-",
                u8::from(month) => width(2),
                "-",
                day => width(2),
            );

        Metadata::new(
            formatted_width,
            self,
            DateMetadata {
                year_width,
                display_sign,
                year,
                month: u8::from(month),
                day,
            },
        )
    }

    fn fmt_with_metadata(
        &self,
        f: &mut fmt::Formatter<'_>,
        metadata: Metadata<Self>,
    ) -> fmt::Result {
        let DateMetadata {
            year_width,
            display_sign,
            year,
            month,
            day,
        } = *metadata;
        let year_width = year_width.extend();

        if display_sign {
            f.pad_with_width(
                metadata.unpadded_width(),
                format_args!("{year:+0year_width$}-{month:02}-{day:02}"),
            )
        } else {
            f.pad_with_width(
                metadata.unpadded_width(),
                format_args!("{year:0year_width$}-{month:02}-{day:02}"),
            )
        }
    }
}

impl fmt::Display for Date {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        SmartDisplay::fmt(self, f)
    }
}

impl fmt::Debug for Date {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> Result<(), fmt::Error> {
        fmt::Display::fmt(self, f)
    }
}
// endregion formatting & parsing

// region: trait impls
impl Add<Duration> for Date {
    type Output = Self;

    /// # Panics
    ///
    /// This may panic if an overflow occurs.
    fn add(self, duration: Duration) -> Self::Output {
        self.checked_add(duration)
            .expect("overflow adding duration to date")
    }
}

impl Add<StdDuration> for Date {
    type Output = Self;

    /// # Panics
    ///
    /// This may panic if an overflow occurs.
    fn add(self, duration: StdDuration) -> Self::Output {
        self.checked_add_std(duration)
            .expect("overflow adding duration to date")
    }
}

impl_add_assign!(Date: Duration, StdDuration);

impl Sub<Duration> for Date {
    type Output = Self;

    /// # Panics
    ///
    /// This may panic if an overflow occurs.
    fn sub(self, duration: Duration) -> Self::Output {
        self.checked_sub(duration)
            .expect("overflow subtracting duration from date")
    }
}

impl Sub<StdDuration> for Date {
    type Output = Self;

    /// # Panics
    ///
    /// This may panic if an overflow occurs.
    fn sub(self, duration: StdDuration) -> Self::Output {
        self.checked_sub_std(duration)
            .expect("overflow subtracting duration from date")
    }
}

impl_sub_assign!(Date: Duration, StdDuration);

impl Sub for Date {
    type Output = Duration;

    fn sub(self, other: Self) -> Self::Output {
        Duration::days((self.to_julian_day() - other.to_julian_day()).extend())
    }
}
// endregion trait impls
