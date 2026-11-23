use crate::{
    bounds::{self as b, RangeError},
    civil::{self, DateTime, Weekday},
    macros::{ctry, rbail, rtry, unwrapr},
};

/// A Gregorian civil date.
///
/// A Gregorian civil date can be infallibly converted to and from
/// [`UnixEpochDay`] values.
///
/// Note that since this supports a year `0`, this technically implements the
/// ISO 8601 proleptic calendar and not the Gregorian calendar. Notably, the
/// Gregorian calendar does not have a year `0`. Therefore, a `Date` with a
/// year `0` is equivalent to the Gregorian `1 BCE` year.
#[derive(Clone, Copy, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct Date {
    year: i16,
    month: i8,
    day: i8,
}

impl Date {
    /// The minimum allowed Gregorian date.
    ///
    /// This is guaranteed to be equivalent to [`UnixEpochDay::MIN`] when
    /// converted to a Unix epoch day.
    pub const MIN: Date = {
        let min = UnixEpochDay::MIN.to_date();
        assert!(min.year() == -9999);
        assert!(min.month() == 1);
        assert!(min.day() == 1);
        min
    };

    /// The maximum allowed Gregorian date.
    ///
    /// This is guaranteed to be equivalent to [`UnixEpochDay::MAX`] when
    /// converted to a Unix epoch day.
    pub const MAX: Date = {
        let max = UnixEpochDay::MAX.to_date();
        assert!(max.year() == 9999);
        assert!(max.month() == 12);
        assert!(max.day() == 31);
        max
    };

    // I'm hopeful we won't need a public unchecked constructor. But if
    // we do, it cannot be marked safe. Otherwise callers cannot soundly rely
    // on the values of, e.g., `Date::day()` being in range for memory safety.
    /*
    /// Returns a new `Date` without doing bounds checks on the values given.
    ///
    /// # Safety
    ///
    /// Callers must ensure that the year, month and day provided are a valid
    /// Gregorian date **and** that `year` falls into the range specified by
    /// [`Year`](b::Year).
    ///
    /// While memory safety may not be violated immediately from using an
    /// invalid value, downstream callers may rely on the validity of routines
    /// like `Date::month` for memory safety. If this routine was safe,
    /// then such reliance would be unsound.
    #[inline]
    pub const unsafe fn new_unchecked(year: i16, month: i8, day: i8) -> Date {
        Date { year, month, day }
    }
    */

    /// Creates a new date in the Gregorian calendar.
    ///
    /// If the date is invalid or any of the values are out of their supported
    /// ranges, then an error is returned.
    ///
    /// Note that, technically, the date returned is in the ISO 8601 proleptic
    /// calendar. The only difference between it and the Gregorian calendar
    /// is that ISO 8601 has a year `0`. ISO 8601's year `0` corresponds to
    /// `-1 BCE` in the Gregorian calendar.
    #[inline]
    pub const fn new(
        year: i16,
        month: i8,
        day: i8,
    ) -> Result<Date, RangeError> {
        let year = rtry!(b::Year::checkc(year as i64));
        let month = rtry!(b::Month::checkc(month as i64));
        if day < 1 {
            rbail!(b::Day::error());
        } else if day > 28 && day > civil::days_in_month(year, month) {
            rbail!(b::SpecialBoundsError::DateInvalidDay { year, month });
        }
        Ok(Date { year, month, day })
    }

    /// Like `Date::new`, but constrains the day value to the last day of
    /// `month`.
    ///
    /// This still returns an error when `day < 1` or when `year` or `month`
    /// are invalid.
    #[inline]
    pub const fn new_constrain(
        year: i16,
        month: i8,
        day: i8,
    ) -> Result<Date, RangeError> {
        let year = rtry!(b::Year::checkc(year as i64));
        let month = rtry!(b::Month::checkc(month as i64));
        let day = if day < 1 {
            rbail!(b::Day::error());
        } else if day > 28 {
            let days_in_month = civil::days_in_month(year, month);
            if day <= days_in_month { day } else { days_in_month }
        } else {
            day
        };
        Ok(Date { year, month, day })
    }

    /// Returns the date corresponding to the day of the given year. The day
    /// of the year should be a value in `1..=366`, with `366` only being valid
    /// if `year` is a leap year.
    ///
    /// Returns an error if `year` is not in the range specified by
    /// [`Year`](b::Year), or if `day` is invalid for the given year.
    #[inline]
    pub const fn from_day_of_year(
        year: i16,
        day: i16,
    ) -> Result<Date, RangeError> {
        let year = rtry!(b::Year::checkc(year as i64));
        let day = rtry!(b::DayOfYear::checkc(day as i64));
        let start = Date { year, month: 1, day: 1 }.to_unix_epoch_day();
        let end = match start.checked_add(day as i32 - 1) {
            Ok(end) => end.to_date(),
            // This can only happen when `year=9999` and `day=366`.
            Err(_) => {
                rbail!(b::SpecialBoundsError::DateInvalidDayOfYear { year })
            }
        };
        // If we overflowed into the next year, then `day` is too big.
        if year != end.year {
            // Can only happen given day=366 and this is a leap year.
            debug_assert!(day == 366);
            debug_assert!(!civil::is_leap_year(year));
            rbail!(b::SpecialBoundsError::DateInvalidDayOfYear { year })
        }
        Ok(end)
    }

    /// Returns the date corresponding to the day of the given year. The day
    /// of the year must be a value in `1..=365`, with February 29 being
    /// completely ignored. That is, it is guaranteed that February 29 will
    /// never be returned by this function. It is impossible.
    ///
    /// Returns an error if `year` is not in the range specified by
    /// [`Year`](b::Year), or if `day` is outside the range `1..=365`.
    #[inline]
    pub const fn from_day_of_year_no_leap(
        year: i16,
        day: i16,
    ) -> Result<Date, RangeError> {
        let year = rtry!(b::Year::checkc(year as i64));
        let mut day = rtry!(b::DayOfYearNoLeap::checkc(day as i64));
        if day >= 60 && civil::is_leap_year(year) {
            day += 1;
        }
        // The boundary check above guarantees this always succeeds.
        Ok(unwrapr!(Date::from_day_of_year(year, day), "valid day of year"))
    }

    /// Returns the year component of this date.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`Year`](crate::bounds::Year).
    #[inline]
    pub const fn year(self) -> i16 {
        self.year
    }

    /// Returns the month component of this date.
    ///
    /// The value returned is guaranteed to be in the range `1..=12`.
    #[inline]
    pub const fn month(self) -> i8 {
        self.month
    }

    /// Returns the day component of this date.
    ///
    /// The value returned is guaranteed to be in the range
    /// `1..=jiff_core::civil::days_in_month(date.year(), date.month())`.
    #[inline]
    pub const fn day(self) -> i8 {
        self.day
    }

    /// Returns the weekday corresponding to this date.
    #[inline]
    pub const fn weekday(self) -> Weekday {
        self.to_unix_epoch_day().weekday()
    }

    /// Returns the ordinal day of the year that this date resides in.
    ///
    /// For leap years, this always returns a value in the range `1..=366`.
    /// Otherwise, the value is in the range `1..=365`.
    #[inline]
    pub const fn day_of_year(self) -> i16 {
        const DAYS_BY_MONTH_NO_LEAP: [i16; 14] =
            [0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365];
        const DAYS_BY_MONTH_LEAP: [i16; 14] =
            [0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366];
        const TABLES: [[i16; 14]; 2] =
            [DAYS_BY_MONTH_NO_LEAP, DAYS_BY_MONTH_LEAP];
        TABLES[self.in_leap_year() as usize][self.month() as usize]
            + (self.day() as i16)
    }

    /// Returns the ordinal day of the year that this date resides in, but
    /// ignores leap years.
    ///
    /// That is, the range of possible values returned by this routine is
    /// `1..=365`, even if this date resides in a leap year. If this date is
    /// February 29, then this routine returns `None`.
    ///
    /// The value `365` always corresponds to the last day in the year,
    /// December 31, even for leap years.
    #[inline]
    pub const fn day_of_year_no_leap(self) -> Option<i16> {
        let mut days = self.day_of_year();
        if self.in_leap_year() {
            // day=60 is Feb 29
            if days == 60 {
                return None;
            } else if days > 60 {
                days -= 1;
            }
        }
        Some(days)
    }

    /// Returns the first date of the month for this date.
    #[inline]
    pub const fn first_of_month(self) -> Date {
        Date { day: 1, ..self }
    }

    /// Returns the last date of the month for this date.
    #[inline]
    pub const fn last_of_month(self) -> Date {
        Date { day: self.days_in_month(), ..self }
    }

    /// Returns the total number of days in the the month in which this date
    /// resides.
    ///
    /// This is guaranteed to always return one of the following values,
    /// depending on the year and the month: 28, 29, 30 or 31.
    #[inline]
    pub const fn days_in_month(self) -> i8 {
        civil::days_in_month(self.year(), self.month())
    }

    /// Returns the first date of the year that this date resides in.
    #[inline]
    pub const fn first_of_year(self) -> Date {
        Date { month: 1, day: 1, ..self }
    }

    /// Returns the last date of the year that this date resides in.
    #[inline]
    pub const fn last_of_year(self) -> Date {
        Date { month: 12, day: 31, ..self }
    }

    /// Returns the number of days in the year for this date.
    ///
    /// It is guaranteed that the value returned is either `365` or `366`.
    #[inline]
    pub const fn days_in_year(self) -> i16 {
        if self.in_leap_year() { 366 } else { 365 }
    }

    /// Returns true when this date is in a leap year.
    #[inline]
    pub const fn in_leap_year(self) -> bool {
        civil::is_leap_year(self.year())
    }

    /// Returns the day before this date.
    ///
    /// Returns an error when this is the minimal date.
    #[inline]
    pub const fn yesterday(self) -> Result<Date, RangeError> {
        if self.day() == 1 {
            if self.month() == 1 {
                let year = ctry!(self.prev_year());
                return Ok(Date { year, month: 12, day: 31 });
            }
            let month = self.month() - 1;
            let day = civil::days_in_month(self.year(), month);
            return Ok(Date { month, day, ..self });
        }
        Ok(Date { day: self.day() - 1, ..self })
    }

    /// Returns the day after this date.
    ///
    /// Returns an error when this is the maximal date.
    #[inline]
    pub const fn tomorrow(self) -> Result<Date, RangeError> {
        if self.day() >= 28 && self.day() == self.days_in_month() {
            if self.month() == 12 {
                let year = ctry!(self.next_year());
                return Ok(Date { year, month: 1, day: 1 });
            }
            let month = self.month() + 1;
            return Ok(Date { month, day: 1, ..self });
        }
        Ok(Date { day: self.day() + 1, ..self })
    }

    /// Returns the `nth` weekday of the month represented by this date.
    ///
    /// `nth` must be non-zero and otherwise in the range `-5..=5`. If it
    /// isn't, an error is returned.
    ///
    /// This also returns an error if `abs(nth)==5` and there is no "5th"
    /// weekday of this month.
    #[inline]
    pub const fn nth_weekday_of_month(
        &self,
        nth: i8,
        weekday: Weekday,
    ) -> Result<Date, RangeError> {
        let nth = rtry!(b::NthWeekdayOfMonth::checkc(nth as i64));
        if nth == 0 {
            rbail!(b::NthWeekdayOfMonth::error());
        } else if nth > 0 {
            let first = self.first_of_month();
            let first_weekday = first.weekday();
            let diff = weekday.since(first_weekday);
            let day = diff + 1 + (nth - 1) * 7;
            Date::new(self.year(), self.month(), day)
        } else {
            let last = self.last_of_month();
            let last_weekday = last.weekday();
            let diff = last_weekday.since(weekday);
            let day = last.day() - diff - (nth.abs() - 1) * 7;
            Date::new(self.year(), self.month(), day)
        }
    }

    /// Returns the "nth" weekday from this date, not including itself.
    ///
    /// The `nth` parameter can be positive or negative. A positive value
    /// computes the "nth" weekday starting at the day after this date and
    /// going forwards in time. A negative value computes the "nth" weekday
    /// starting at the day before this date and going backwards in time.
    ///
    /// For example, if this date's weekday is a Sunday and the first Sunday is
    /// asked for (that is, `date.nth_weekday(1, Weekday::Sunday)`), then the
    /// result is a week from this date corresponding to the following Sunday.
    #[inline]
    pub const fn nth_weekday(
        self,
        nth: i32,
        weekday: Weekday,
    ) -> Result<Date, RangeError> {
        self.to_unix_epoch_day().nth_weekday(nth, weekday)
    }

    /// Returns the year one year before this date.
    ///
    /// Returns an error if this would result in a year outside the range
    /// specified by [`Year`](b::Year).
    #[inline]
    pub(crate) const fn prev_year(self) -> Result<i16, RangeError> {
        Ok(rtry!(b::Year::checked_add(self.year(), -1)))
    }

    /// Returns the year one year from this date.
    #[inline]
    pub(crate) const fn next_year(self) -> Result<i16, RangeError> {
        Ok(rtry!(b::Year::checked_add(self.year(), 1)))
    }

    /// Adds the given number of days to this date.
    ///
    /// Returns an error if the result is outside the valid range of a `Date`.
    #[inline]
    pub const fn checked_add(self, days: i32) -> Result<Date, RangeError> {
        match days {
            0 => Ok(self),
            -1 => self.yesterday(),
            1 => self.tomorrow(),
            n => Ok(ctry!(self.to_unix_epoch_day().checked_add(n)).to_date()),
        }
    }

    /// Subtracts the given number of days to this date.
    ///
    /// Returns an error if the result is outside the valid range of a `Date`.
    #[inline]
    pub const fn checked_sub(self, days: i32) -> Result<Date, RangeError> {
        let Some(days) = days.checked_neg() else {
            rbail!(b::UnixEpochDays::error());
        };
        self.checked_add(days)
    }

    /// Returns the number of days from this date until `other`.
    ///
    /// This routine never overflows because of the enforced range on
    /// `Date` values.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::civil::date;
    ///
    /// let date1 = date(2023, 7, 1);
    /// let date2 = date(2024, 7, 1);
    /// assert_eq!(366, date1.until(date2));
    ///
    /// // The value will be negative when the dates are flipped:
    /// assert_eq!(-366, date2.until(date1));
    /// ```
    ///
    /// # Example: overflow isn't possible
    ///
    /// Even when using the minimum or maximum values:
    ///
    /// ```
    /// use jiff_core::civil::Date;
    ///
    /// assert_eq!(7_304_483, Date::MIN.until(Date::MAX));
    /// assert_eq!(-7_304_483, Date::MAX.until(Date::MIN));
    /// ```
    #[inline]
    pub const fn until(self, other: Date) -> i32 {
        -self.since(other)
    }

    /// Returns the number of days from this date since `other`.
    ///
    /// This routine never overflows because of the enforced range on
    /// `Date` values.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::civil::date;
    ///
    /// let date1 = date(2023, 7, 1);
    /// let date2 = date(2024, 7, 1);
    /// assert_eq!(366, date2.since(date1));
    ///
    /// // The value will be negative when the dates are flipped:
    /// assert_eq!(-366, date1.since(date2));
    /// ```
    ///
    /// # Example: overflow isn't possible
    ///
    /// Even when using the minimum or maximum values:
    ///
    /// ```
    /// use jiff_core::civil::Date;
    ///
    /// assert_eq!(7_304_483, Date::MAX.since(Date::MIN));
    /// ```
    #[inline]
    pub const fn since(self, other: Date) -> i32 {
        // Try to avoid conversions to Unix epoch days in some cases.
        if self.year() == other.year() {
            if self.month() == other.month() {
                (self.day() - other.day()) as i32
            } else {
                (self.day_of_year() - other.day_of_year()) as i32
            }
        } else {
            self.to_unix_epoch_day().since(other.to_unix_epoch_day())
        }
    }

    /// Converts a Gregorian date to a Unix epoch day.
    #[inline]
    #[allow(non_upper_case_globals, non_snake_case)] // to mimic source
    pub const fn to_unix_epoch_day(self) -> UnixEpochDay {
        // This is Neri-Schneider. There's no branching or divisions.
        //
        // Ref: https://github.com/cassioneri/eaf/blob/684d3cc32d14eee371d0abe4f683d6d6a49ed5c1/algorithms/neri_schneider.hpp#L83
        const s: u32 = 82;
        const K: u32 = 719468 + 146097 * s;
        const L: u32 = 400 * s;

        let year = self.year as u32;
        let month = self.month as u32;
        let day = self.day as u32;

        let J = month <= 2;
        let Y = year.wrapping_add(L).wrapping_sub(J as u32);
        let M = if J { month + 12 } else { month };
        let D = day - 1;
        let C = Y / 100;

        let y_star = 1461 * Y / 4 - C + C / 4;
        let m_star = (979 * M - 2919) / 32;
        let N = y_star + m_star + D;

        let N_U = N.wrapping_sub(K);
        let epoch_day = N_U as i32;
        UnixEpochDay { day: epoch_day }
    }

    /// Converts this Gregorian date to an ISO 8601 week date.
    #[inline]
    pub const fn to_iso_week_date(self) -> ISOWeekDate {
        let epoch_day = self.to_unix_epoch_day();
        let mut year = self.year();
        let mut epoch_day_year_start = iso_week_start_from_year(year);
        if epoch_day.day() < epoch_day_year_start.day() {
            // If our date comes before the first day of the ISO 8601 year,
            // then it must be the case that our date falls at the end of the
            // previous ISO 8601 year. This can happen for Gregorian dates
            // Jan 1, 2 or 3.
            //
            // And this subtraction is OK because `year - 1` is only a problem
            // when `year=-9999`. But we can't be here when `year=-9999` since
            // that would imply an `epoch_day` that occurs before the first day
            // of this date's Gregorian year. That would in turn imply a date
            // before `-9999-01-01`, which is impossible.
            year -= 1;
            epoch_day_year_start = iso_week_start_from_year(year);
        } else if self.month() == 12 && self.day() >= 29 && year < b::Year::MAX
        {
            // Otherwise, it's possible for dates at the end of the Gregorian
            // calendar year to actually have an ISO 8601 week year following
            // the Gregorian calendar year. This can occur for only Dec 29, 30
            // or 31. For `year=9999`, we don't need to do this check because
            // in that specific instance, the last day of `9999` corresponds to
            // `9999-W52-5`. So it's not possible for the ISO 8601 week year to
            // be `10000`.
            let epoch_day_next_year_week_start =
                iso_week_start_from_year(year + 1);
            if epoch_day.day() >= epoch_day_next_year_week_start.day() {
                epoch_day_year_start = epoch_day_next_year_week_start;
                year += 1;
            }
        }

        // OK because the biggest difference between epoch days here can be
        // 370 (for a long year), and dividing that by 7 and adding 1 always
        // fits into an `i8`.
        let week =
            (((epoch_day.day() - epoch_day_year_start.day()) / 7) + 1) as i8;
        let weekday = epoch_day.weekday();

        ISOWeekDate { year, week, weekday }
    }

    /// A convenience function for constructing a [`DateTime`] from this date
    /// at the time given by its components.
    ///
    /// # Panics
    ///
    /// This panics if the provided values do not correspond to a valid `Time`.
    /// All of the following conditions must be true:
    ///
    /// * `0 <= hour <= 23`
    /// * `0 <= minute <= 59`
    /// * `0 <= second <= 59`
    /// * `0 <= subsec_nanosecond <= 999,999,999`
    ///
    /// Similarly, when used in a const context, invalid parameters will
    /// prevent your Rust program from compiling.
    #[inline]
    pub const fn at(
        self,
        hour: i8,
        minute: i8,
        second: i8,
        subsec_nanosecond: i32,
    ) -> DateTime {
        DateTime::from_parts(
            self,
            civil::time(hour, minute, second, subsec_nanosecond),
        )
    }
}

impl core::fmt::Debug for Date {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        if self.year() < 0 {
            write!(f, "-{:06}", self.year().unsigned_abs())?;
        } else {
            write!(f, "{:04}", self.year())?;
        }
        write!(f, "-{:02}-{:02}", self.month(), self.day())
    }
}

/// Returns the number of days between two `Date` values.
///
/// This routine never overflows because of the enforced range on
/// `Date` values.
///
/// # Example
///
/// ```
/// use jiff_core::civil::date;
///
/// let date1 = date(2023, 7, 1);
/// let date2 = date(2024, 7, 1);
/// assert_eq!(366, date2 - date1);
///
/// // The value will be negative when the dates are flipped:
/// assert_eq!(-366, date1 - date2);
/// ```
///
/// # Example: overflow isn't possible
///
/// Even when using the minimum or maximum values:
///
/// ```
/// use jiff_core::civil::Date;
///
/// assert_eq!(7_304_483, Date::MAX - Date::MIN);
/// ```
impl core::ops::Sub for Date {
    type Output = i32;

    #[inline]
    fn sub(self, rhs: Date) -> i32 {
        self.since(rhs)
    }
}

#[cfg(feature = "arbitrary")]
impl<'a> arbitrary::Arbitrary<'a> for Date {
    fn arbitrary(
        u: &mut arbitrary::Unstructured<'a>,
    ) -> arbitrary::Result<Date> {
        let year = u.int_in_range(b::Year::MIN..=b::Year::MAX)?;
        let month = u.int_in_range(b::Month::MIN..=b::Month::MAX)?;
        let day = u.int_in_range(b::Day::MIN..=b::Day::MAX)?;
        Ok(Date::new_constrain(year, month, day).unwrap())
    }

    fn size_hint(depth: usize) -> (usize, Option<usize>) {
        arbitrary::size_hint::and_all(&[
            <i16 as arbitrary::Arbitrary>::size_hint(depth),
            <i8 as arbitrary::Arbitrary>::size_hint(depth),
            <i8 as arbitrary::Arbitrary>::size_hint(depth),
        ])
    }
}

#[cfg(test)]
impl quickcheck::Arbitrary for Date {
    fn arbitrary(g: &mut quickcheck::Gen) -> Date {
        let year = b::Year::arbitrary(g);
        let month = b::Month::arbitrary(g);
        let day = b::Day::arbitrary(g);
        Date::new_constrain(year, month, day).unwrap()
    }

    fn shrink(&self) -> alloc::boxed::Box<dyn Iterator<Item = Date>> {
        alloc::boxed::Box::new(
            (self.year(), self.month(), self.day()).shrink().filter_map(
                |(year, month, day)| {
                    Date::new_constrain(year, month, day).ok()
                },
            ),
        )
    }
}

/// A civil date represented by a number of days since the Unix epoch
/// (1970-01-01).
///
/// The date can be positive (occurs after 1970-01-01) or negative (occurs
/// before 1970-01-01). A zero value corresponds to 1970-01-01.
///
/// Unix epoch days can be infallibly converted to and from [`Date`] values.
#[derive(Clone, Copy, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct UnixEpochDay {
    day: i32,
}

impl UnixEpochDay {
    /// The minimum allowed Unix epoch day.
    ///
    /// This is guaranteed to be equivalent to [`Date::MIN`] when
    /// converted to a Gregorian date.
    pub const MIN: UnixEpochDay = UnixEpochDay { day: b::UnixEpochDays::MIN };

    /// The maximum allowed Unix epoch day.
    ///
    /// This is guaranteed to be equivalent to [`Date::MAX`] when
    /// converted to a Gregorian date.
    pub const MAX: UnixEpochDay = UnixEpochDay { day: b::UnixEpochDays::MAX };

    /// Creates a new Unix epoch day.
    ///
    /// The day given must correspond to a number of days (positive or
    /// negative) since the Unix epoch (1970-01-01). A value of `0` corresponds
    /// to `1970-01-01`.
    #[inline]
    pub const fn new(day: i32) -> Result<UnixEpochDay, RangeError> {
        let day = rtry!(b::UnixEpochDays::checkc(day as i64));
        Ok(UnixEpochDay { day })
    }

    /// Returns the underlying day value.
    ///
    /// This is guaranteed to be in the [`UnixEpochDays`](b::UnixEpochDays)
    /// range.
    #[inline]
    pub const fn day(self) -> i32 {
        self.day
    }

    /// Returns the day of the week for this epoch day.
    #[inline]
    pub const fn weekday(&self) -> Weekday {
        // Fast technique to obtain weekday 1..7 (Mon..Sun)
        // directly via an add-mul-shift. Relies on the fact
        // that the Unix epoch was a Thursday and that 7 is a
        // Mersenne number.
        //
        // Accurate over a limited range (-89478489 to 89478489)
        // which is -243014-03-21 (Tue) to 246953-10-13 (Sat).
        // This exceeds Jiff's range.
        //
        // Ref: https://www.benjoffe.com/fast-day-of-week
        let result = Weekday::from_monday_one_offset({
            const M: u32 = {
                // MSRV(1.73): Just use `n.div_ceil` instead.
                const fn div_ceil(lhs: u64, rhs: u64) -> u64 {
                    let d = lhs / rhs;
                    let r = lhs % rhs;
                    if r > 0 { d + 1 } else { d }
                }

                let n = div_ceil(1u64 << 32, 7) as u32;
                assert!(n == 613_566_757);
                n
            };
            const Z: u32 = 0x90000000; // Magic add: see link above.
            let rd: u32 = self.day() as u32;
            (rd.wrapping_mul(M).wrapping_add(Z) >> 29) as i8
        });
        unwrapr!(result, "weekday must be in range 1..=7")
    }

    /// Returns the "nth" weekday from this date, not including itself.
    ///
    /// The `nth` parameter can be positive or negative. A positive value
    /// computes the "nth" weekday starting at the day after this date and
    /// going forwards in time. A negative value computes the "nth" weekday
    /// starting at the day before this date and going backwards in time.
    ///
    /// For example, if this date's weekday is a Sunday and the first Sunday is
    /// asked for (that is, `date.nth_weekday(1, Weekday::Sunday)`), then the
    /// result is a week from this date corresponding to the following Sunday.
    #[inline]
    pub const fn nth_weekday(
        self,
        nth: i32,
        weekday: Weekday,
    ) -> Result<Date, RangeError> {
        // ref: http://howardhinnant.github.io/date_algorithms.html#next_weekday

        let nth = rtry!(b::NthWeekday::checkc(nth as i64));
        if nth == 0 {
            rbail!(b::NthWeekday::error());
        } else if nth > 0 {
            let weekday_diff = weekday.since(self.weekday().next()) as i32;
            let diff = (nth - 1) * 7 + weekday_diff;
            let end = ctry!(self.checked_add(diff + 1));
            Ok(end.to_date())
        } else {
            let weekday_diff = self.weekday().previous().since(weekday) as i32;
            // OK because of the range on `NthWeekday`.
            let nth = nth.abs();
            // OK because of the range on `NthWeekday`.
            let diff = -((nth - 1) * 7 + weekday_diff);
            let end = ctry!(self.checked_add(diff - 1));
            Ok(end.to_date())
        }
    }

    /// Add the given number of days to this Unix epoch day.
    ///
    /// If this would overflow an `i32` or result in an out-of-bounds Unix
    /// epoch day, then this returns an error.
    #[inline]
    pub const fn checked_add(
        self,
        days: i32,
    ) -> Result<UnixEpochDay, RangeError> {
        let day = rtry!(b::UnixEpochDays::checked_add(self.day(), days));
        Ok(UnixEpochDay { day })
    }

    /// Subtracts the given number of days from this Unix epoch day.
    ///
    /// If this would overflow an `i32` or result in an out-of-bounds Unix
    /// epoch day, then this returns an error.
    #[inline]
    pub const fn checked_sub(
        self,
        days: i32,
    ) -> Result<UnixEpochDay, RangeError> {
        let Some(days) = days.checked_neg() else {
            rbail!(b::UnixEpochDays::error());
        };
        self.checked_add(days)
    }

    /// Returns the number of days from this date until `other`.
    ///
    /// This routine never overflows because of the enforced range on
    /// `UnixEpochDay` values.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::civil::date;
    ///
    /// let date1 = date(2023, 7, 1).to_unix_epoch_day();
    /// let date2 = date(2024, 7, 1).to_unix_epoch_day();
    /// assert_eq!(366, date1.until(date2));
    ///
    /// // The value will be negative when the dates are flipped:
    /// assert_eq!(-366, date2.until(date1));
    /// ```
    ///
    /// # Example: overflow isn't possible
    ///
    /// Even when using the minimum or maximum values:
    ///
    /// ```
    /// use jiff_core::civil::UnixEpochDay;
    ///
    /// assert_eq!(7_304_483, UnixEpochDay::MIN.until(UnixEpochDay::MAX));
    /// assert_eq!(-7_304_483, UnixEpochDay::MAX.until(UnixEpochDay::MIN));
    /// ```
    #[inline]
    pub const fn until(self, other: UnixEpochDay) -> i32 {
        -self.since(other)
    }

    /// Returns the number of days from this date since `other`.
    ///
    /// This routine never overflows because of the enforced range on
    /// `UnixEpochDay` values.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::civil::date;
    ///
    /// let date1 = date(2023, 7, 1).to_unix_epoch_day();
    /// let date2 = date(2024, 7, 1).to_unix_epoch_day();
    /// assert_eq!(366, date2.since(date1));
    ///
    /// // The value will be negative when the dates are flipped:
    /// assert_eq!(-366, date1.since(date2));
    /// ```
    ///
    /// # Example: overflow isn't possible
    ///
    /// Even when using the minimum or maximum values:
    ///
    /// ```
    /// use jiff_core::civil::UnixEpochDay;
    ///
    /// assert_eq!(7_304_483, UnixEpochDay::MAX.since(UnixEpochDay::MIN));
    /// ```
    #[inline]
    pub const fn since(self, other: UnixEpochDay) -> i32 {
        self.day() - other.day()
    }

    /// Converts this Unix epoch day to a Gregorian date.
    #[inline]
    #[allow(non_upper_case_globals, non_snake_case)] // to mimic source
    pub const fn to_date(self) -> Date {
        // This is Neri-Schneider. There's no branching or divisions.
        //
        // Ref: <https://github.com/cassioneri/eaf/blob/684d3cc32d14eee371d0abe4f683d6d6a49ed5c1/algorithms/neri_schneider.hpp#L40C3-L40C34>
        const s: u32 = 82;
        const K: u32 = 719468 + 146097 * s;
        const L: u32 = 400 * s;

        let N_U = self.day as u32;
        let N = N_U.wrapping_add(K);

        let N_1 = 4 * N + 3;
        let C = N_1 / 146097;
        let N_C = (N_1 % 146097) / 4;

        let N_2 = 4 * N_C + 3;
        let P_2 = 2939745 * (N_2 as u64);
        let Z = (P_2 / 4294967296) as u32;
        let N_Y = (P_2 % 4294967296) as u32 / 2939745 / 4;
        let Y = 100 * C + Z;

        let N_3 = 2141 * N_Y + 197913;
        let M = N_3 / 65536;
        let D = (N_3 % 65536) / 2141;

        let J = N_Y >= 306;
        let year = Y.wrapping_sub(L).wrapping_add(J as u32) as i16;
        let month = (if J { M - 12 } else { M }) as i8;
        let day = (D + 1) as i8;
        Date { year, month, day }
    }
}

impl core::fmt::Debug for UnixEpochDay {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        f.debug_tuple("UnixEpochDay").field(&self.day()).finish()
    }
}

/// Returns the number of days between two `UnixEpochDay` values.
///
/// This routine never overflows because of the enforced range on
/// `UnixEpochDay` values.
///
/// # Example
///
/// ```
/// use jiff_core::civil::date;
///
/// let date1 = date(2023, 7, 1).to_unix_epoch_day();
/// let date2 = date(2024, 7, 1).to_unix_epoch_day();
/// assert_eq!(366, date2 - date1);
///
/// // The value will be negative when the dates are flipped:
/// assert_eq!(-366, date1 - date2);
/// ```
///
/// # Example: overflow isn't possible
///
/// Even when using the minimum or maximum values:
///
/// ```
/// use jiff_core::civil::UnixEpochDay;
///
/// assert_eq!(7_304_483, UnixEpochDay::MAX - UnixEpochDay::MIN);
/// ```
impl core::ops::Sub for UnixEpochDay {
    type Output = i32;

    #[inline]
    fn sub(self, rhs: UnixEpochDay) -> i32 {
        self.since(rhs)
    }
}

/// An ISO 8601 civil week date.
///
/// An ISO  8601 civil week date can be infallibly converted to and from
/// Gregorian [`Date`] values.
#[derive(Clone, Copy, Eq, Hash, PartialEq)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct ISOWeekDate {
    year: i16,
    week: i8,
    weekday: Weekday,
}

impl ISOWeekDate {
    /// The minimum representable ISO week date.
    pub const MIN: ISOWeekDate = ISOWeekDate {
        year: b::ISOYear::MIN,
        week: b::ISOWeek::MIN,
        weekday: Weekday::Monday,
    };

    /// The maximum representable ISO week date.
    pub const MAX: ISOWeekDate = ISOWeekDate {
        year: b::ISOYear::MAX,
        // Technical max is 52, but 9999 is not a leap year.
        week: 52,
        weekday: Weekday::Friday,
    };

    /// The zero ISO 8601 week date. It is the first day of the zeroth year.
    pub const ZERO: ISOWeekDate =
        ISOWeekDate { year: 0, week: 1, weekday: Weekday::Monday };

    /// Create a new ISO week date from its constituent parts.
    ///
    /// If the week date is invalid or any of the values are out of their
    /// supported ranges, then an error is returned.
    #[inline]
    pub const fn new(
        year: i16,
        week: i8,
        weekday: Weekday,
    ) -> Result<ISOWeekDate, RangeError> {
        let year = rtry!(b::ISOYear::checkc(year as i64));
        let week = rtry!(b::ISOWeek::checkc(week as i64));

        // All combinations of years, weeks and weekdays allowed by our
        // range types are valid ISO week dates with one exception: a week
        // number of 53 is only valid for "long" years. Or years with an ISO
        // leap week. It turns out this only happens when the last day of the
        // year is a Thursday.
        //
        // Note that if the ranges in this crate are changed, this could be
        // a little trickier if the range of ISOYear is different from Year.
        debug_assert!(b::Year::MIN == b::ISOYear::MIN);
        debug_assert!(b::Year::MAX == b::ISOYear::MAX);
        if week == 53 && !civil::is_long_iso_week_year(year) {
            rbail!(b::ISOWeek::error());
        }
        // And also, the maximum Date constrains what we can utter with
        // ISOWeekDate so that we can preserve infallible conversions between
        // them. So since 9999-12-31 maps to 9999 W52 Friday, it follows that
        // Saturday and Sunday are not allowed when the year is at the maximum
        // value. So reject them.
        //
        // We don't need to worry about the minimum because the minimum date
        // (-9999-01-01) corresponds also to the minimum possible combination
        // of an ISO week date's fields: -9999 W01 Monday. Nice.
        if year == b::ISOYear::MAX
            && week == 52
            && weekday.to_monday_zero_offset()
                > Weekday::Friday.to_monday_zero_offset()
        {
            rbail!(b::WeekdayMondayOne::error());
        }
        Ok(ISOWeekDate { year, week, weekday })
    }

    /// Like `ISOWeekDate::new`, but constrains out-of-bounds week and weekday
    /// values to their closest valid equivalent.
    ///
    /// For example, given `9999 W52 Saturday`, this will return
    /// `9999 W52 Friday`.
    ///
    /// This still returns an error when `week < 1` or when `year` is invalid.
    #[inline]
    pub const fn new_constrain(
        year: i16,
        mut week: i8,
        mut weekday: Weekday,
    ) -> Result<ISOWeekDate, RangeError> {
        let year = rtry!(b::ISOYear::checkc(year as i64));
        if week < 1 {
            rbail!(b::ISOWeek::error());
        }
        let weeks_in_year =
            if civil::is_long_iso_week_year(year) { 53 } else { 52 };
        week = if week <= weeks_in_year { week } else { weeks_in_year };
        if year == b::ISOYear::MAX
            && week == 52
            && weekday.to_monday_zero_offset()
                > Weekday::Friday.to_monday_zero_offset()
        {
            weekday = Weekday::Friday;
        }
        Ok(ISOWeekDate { year, week, weekday })
    }

    /// Returns the year component of this ISO 8601 week date.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`ISOYear`](crate::bounds::ISOYear).
    #[inline]
    pub const fn year(self) -> i16 {
        self.year
    }

    /// Returns the week number component of this ISO 8601 week date.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`ISOWeek`](crate::bounds::ISOWeek).
    #[inline]
    pub const fn week(self) -> i8 {
        self.week
    }

    /// Returns the weekday component of this ISO 8601 week date.
    #[inline]
    pub const fn weekday(self) -> Weekday {
        self.weekday
    }

    /// Returns the ISO 8601 week date corresponding to the first day in the
    /// week of this week date. The date returned is guaranteed to have a
    /// weekday of [`Weekday::Monday`].
    ///
    /// # Errors
    ///
    /// Since `-9999-01-01` falls on a Monday, it follows that the minimum
    /// supported Gregorian date is exactly equivalent to the minimum supported
    /// ISO 8601 week date. This means that this routine can never actually
    /// fail, but only insomuch as the minimums line up. For that reason, and
    /// for consistency with [`ISOWeekDate::last_of_week`], the API is
    /// fallible.
    #[inline]
    pub const fn first_of_week(self) -> Result<ISOWeekDate, RangeError> {
        // I believe this can never return an error because `Monday` is in
        // bounds for all possible year-and-week combinations. This is *only*
        // because -9999-01-01 corresponds to -9999-W01-Monday. Which is kinda
        // lucky. And I guess if we ever change the ranges, this could become
        // fallible.
        Ok(ISOWeekDate { weekday: Weekday::Monday, ..self })
    }

    /// Returns the ISO 8601 week date corresponding to the last day in the
    /// week of this week date. The date returned is guaranteed to have a
    /// weekday of [`Weekday::Sunday`].
    ///
    /// # Errors
    ///
    /// This can return an error if the last day of the week exceeds Jiff's
    /// maximum Gregorian date of `9999-12-31`. It turns out this can happen
    /// since `9999-12-31` falls on a Friday.
    #[inline]
    pub const fn last_of_week(self) -> Result<ISOWeekDate, RangeError> {
        ISOWeekDate::new(self.year(), self.week(), Weekday::Sunday)
    }

    /// Returns the ISO 8601 week date corresponding to the first day in the
    /// year of this week date. The date returned is guaranteed to have a
    /// weekday of [`Weekday::Monday`].
    ///
    /// # Errors
    ///
    /// Since `-9999-01-01` falls on a Monday, it follows that the minimum
    /// support Gregorian date is exactly equivalent to the minimum supported
    /// ISO 8601 week date. This means that this routine can never actually
    /// fail, but only insomuch as the minimums line up. For that reason, and
    /// for consistency with [`ISOWeekDate::last_of_year`], the API is
    /// fallible.
    #[inline]
    pub const fn first_of_year(self) -> Result<ISOWeekDate, RangeError> {
        // I believe this can never return an error because `Monday` is in
        // bounds for all possible year-and-week combinations. This is *only*
        // because -9999-01-01 corresponds to -9999-W01-Monday. Which is kinda
        // lucky. And I guess if we ever change the ranges, this could become
        // fallible.
        Ok(ISOWeekDate { week: 1, weekday: Weekday::Monday, ..self })
    }

    /// Returns the ISO 8601 week date corresponding to the last day in the
    /// year of this week date. The date returned is guaranteed to have a
    /// weekday of [`Weekday::Sunday`].
    ///
    /// # Errors
    ///
    /// This can return an error if the last day of the year exceeds Jiff's
    /// maximum Gregorian date of `9999-12-31`. It turns out this can happen
    /// since `9999-12-31` falls on a Friday.
    #[inline]
    pub const fn last_of_year(self) -> Result<ISOWeekDate, RangeError> {
        ISOWeekDate::new(self.year(), self.weeks_in_year(), Weekday::Sunday)
    }

    /// Returns the total number of days in the year of this ISO 8601 week
    /// date.
    ///
    /// It is guaranteed that the value returned is either 364 or 371. The
    /// latter case occurs precisely when [`ISOWeekDate::in_long_year`]
    /// returns `true`.
    #[inline]
    pub const fn days_in_year(self) -> i16 {
        if self.in_long_year() { 371 } else { 364 }
    }

    /// Returns the total number of weeks in the year of this ISO 8601 week
    /// date.
    ///
    /// It is guaranteed that the value returned is either 52 or 53. The
    /// latter case occurs precisely when [`ISOWeekDate::in_long_year`]
    /// returns `true`.
    #[inline]
    pub const fn weeks_in_year(self) -> i8 {
        civil::weeks_in_iso_week_year(self.year())
    }

    /// Returns true if and only if the year of this week date is a "long"
    /// year.
    ///
    /// A long year is one that contains precisely 53 weeks. All other years
    /// contain precisely 52 weeks.
    #[inline]
    pub const fn in_long_year(self) -> bool {
        civil::is_long_iso_week_year(self.year())
    }

    /// Returns the ISO 8601 date immediately following this one.
    ///
    /// # Errors
    ///
    /// This returns an error when this date is the maximum value.
    #[inline]
    pub const fn tomorrow(self) -> Result<ISOWeekDate, RangeError> {
        // The maximum week date is `9999-W52-5`, which is a Friday. It doesn't
        // end on a Sunday, so adjusting the logic below to check for it is
        // a bit weird. So we just check here.
        if self.year() == ISOWeekDate::MAX.year()
            && self.week() == ISOWeekDate::MAX.week()
            && matches!(self.weekday(), Weekday::Friday)
        {
            rbail!(b::ISOYear::error());
        }
        // I suppose we could probably implement this in a more efficient
        // manner by avoiding the roundtrip through Gregorian dates.
        // self.to_date().tomorrow().map(|d| d.to_iso_week_date())
        if matches!(self.weekday(), Weekday::Sunday) {
            if self.week() >= 52 && self.week() == self.weeks_in_year() {
                let year = self.year() + 1;
                return Ok(ISOWeekDate {
                    year,
                    week: 1,
                    weekday: Weekday::Monday,
                });
            }
            let week = self.week() + 1;
            return Ok(ISOWeekDate { week, weekday: Weekday::Monday, ..self });
        }
        Ok(ISOWeekDate { weekday: self.weekday().next(), ..self })
    }

    /// Returns the ISO 8601 week date immediately preceding this one.
    ///
    /// # Errors
    ///
    /// This returns an error when this date is the minimum value.
    #[inline]
    pub fn yesterday(self) -> Result<ISOWeekDate, RangeError> {
        if matches!(self.weekday(), Weekday::Monday) {
            if self.week() == 1 {
                let year = rtry!(b::ISOYear::checked_add(self.year(), -1));
                let week = civil::weeks_in_iso_week_year(year);
                return Ok(ISOWeekDate {
                    year,
                    week,
                    weekday: Weekday::Sunday,
                });
            }
            let week = self.week() - 1;
            return Ok(ISOWeekDate { week, weekday: Weekday::Sunday, ..self });
        }
        Ok(ISOWeekDate { weekday: self.weekday().previous(), ..self })
    }

    /// Converts this ISO 8601 week date to a Unix epoch day.
    #[inline]
    pub const fn to_unix_epoch_day(self) -> UnixEpochDay {
        let epoch_day_year = iso_week_start_from_year(self.year());
        let week = self.week() as i32;
        let weekday = self.weekday().to_monday_zero_offset() as i32;
        unwrapr!(
            epoch_day_year.checked_add(((week - 1) * 7) + weekday),
            "all valid ISO 8601 dates convert to a valid Unix epoch day",
        )
    }

    /// Converts this ISO 8601 week date to a Gregorian date.
    #[inline]
    pub const fn to_date(self) -> Date {
        self.to_unix_epoch_day().to_date()
    }
}

impl core::fmt::Debug for ISOWeekDate {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(
            f,
            "{:04}-W{:02}-{}",
            self.year,
            self.week,
            self.weekday.to_monday_one_offset()
        )
    }
}

impl Ord for ISOWeekDate {
    #[inline]
    fn cmp(&self, other: &ISOWeekDate) -> core::cmp::Ordering {
        (self.year(), self.week(), self.weekday().to_monday_one_offset()).cmp(
            &(
                other.year(),
                other.week(),
                other.weekday().to_monday_one_offset(),
            ),
        )
    }
}

impl PartialOrd for ISOWeekDate {
    #[inline]
    fn partial_cmp(&self, other: &ISOWeekDate) -> Option<core::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

/// Returns the Unix epoch day corresponding to the first day in the ISO 8601
/// week year given.
///
/// Callers must ensure that `year` is in the range specified by
/// [`Year`](b::Year).
///
/// Ref: http://howardhinnant.github.io/date_algorithms.html
const fn iso_week_start_from_year(year: i16) -> UnixEpochDay {
    debug_assert!(b::Year::checkc(year as i64).is_ok());
    // A week's year always corresponds to the Gregorian year in which the
    // Thursday of that week falls. Therefore, Jan 4 is *always* in the first
    // week of any ISO week year.
    let epoch_day_in_first_week =
        Date { year, month: 1, day: 4 }.to_unix_epoch_day();
    // The start of the first week is a Monday, so find the number of days
    // since Monday from a date that we know is in the first ISO week of
    // `year`.
    let diff_from_monday =
        epoch_day_in_first_week.weekday().since(Weekday::Monday);
    // OK because `diff_from_monday` is never bigger than 6 and is always
    // positive. Therefore, the only case where this could plausibly fail
    // is when `year=-9999`. But in that specific case, -9999-01-01 is on a
    // Monday, so the diff is guaranteed to give us the minimal Unix epoch
    // day value.
    unwrapr!(
        epoch_day_in_first_week.checked_sub(diff_from_monday as i32),
        "valid Unix epoch day"
    )
}

#[cfg(feature = "arbitrary")]
impl<'a> arbitrary::Arbitrary<'a> for ISOWeekDate {
    fn arbitrary(
        u: &mut arbitrary::Unstructured<'a>,
    ) -> arbitrary::Result<ISOWeekDate> {
        let year = u.int_in_range(b::ISOYear::MIN..=b::ISOYear::MAX)?;
        let week = u.int_in_range(b::ISOWeek::MIN..=b::ISOWeek::MAX)?;
        let weekday = Weekday::arbitrary(u)?;
        Ok(ISOWeekDate::new_constrain(year, week, weekday).unwrap())
    }

    fn size_hint(depth: usize) -> (usize, Option<usize>) {
        arbitrary::size_hint::and_all(&[
            <i16 as arbitrary::Arbitrary>::size_hint(depth),
            <i8 as arbitrary::Arbitrary>::size_hint(depth),
            <Weekday as arbitrary::Arbitrary>::size_hint(depth),
        ])
    }
}

#[cfg(test)]
impl quickcheck::Arbitrary for ISOWeekDate {
    fn arbitrary(g: &mut quickcheck::Gen) -> ISOWeekDate {
        let year = b::ISOYear::arbitrary(g);
        let week = b::ISOWeek::arbitrary(g);
        let weekday = Weekday::arbitrary(g);
        ISOWeekDate::new_constrain(year, week, weekday).unwrap()
    }

    fn shrink(&self) -> alloc::boxed::Box<dyn Iterator<Item = ISOWeekDate>> {
        alloc::boxed::Box::new(
            (self.year(), self.week(), self.weekday()).shrink().filter_map(
                |(year, week, weekday)| {
                    ISOWeekDate::new_constrain(year, week, weekday).ok()
                },
            ),
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn date(year: i16, month: i8, day: i8) -> Date {
        Date::new(year, month, day).unwrap()
    }

    fn week_date(year: i16, week: i8, weekday: Weekday) -> ISOWeekDate {
        ISOWeekDate::new(year, week, weekday).unwrap()
    }

    #[test]
    fn iso_week_date_new_constrain() {
        assert_eq!(
            ISOWeekDate::new_constrain(2024, 60, Weekday::Monday).unwrap(),
            week_date(2024, 52, Weekday::Monday),
        );
    }

    #[test]
    fn date_min() {
        assert_eq!(Date::MIN, date(-9999, 1, 1));
    }

    #[test]
    fn date_max() {
        assert_eq!(Date::MAX, date(9999, 12, 31));
    }

    #[test]
    fn unix_epoch_to_date_and_back_again_min_to_max() {
        for i in 0.. {
            let Ok(epoch_day) = UnixEpochDay::MIN.checked_add(i) else {
                break;
            };
            let date = epoch_day.to_date();
            let got = date.to_unix_epoch_day();
            assert_eq!(epoch_day, got);
        }
    }

    #[test]
    fn unix_epoch_to_date_and_back_again_max_to_min() {
        for i in 0.. {
            let Ok(epoch_day) = UnixEpochDay::MAX.checked_sub(i) else {
                break;
            };
            let date = epoch_day.to_date();
            let got = date.to_unix_epoch_day();
            assert_eq!(epoch_day, got);
        }
    }

    #[test]
    fn date_to_unix_epoch_and_back_again() {
        for year in b::Year::MIN..=b::Year::MAX {
            for month in b::Month::MIN..=b::Month::MAX {
                for day in b::Day::MIN..=civil::days_in_month(year, month) {
                    let d = date(year, month, day);
                    let epoch_day = d.to_unix_epoch_day();
                    let got = epoch_day.to_date();
                    assert_eq!(d, got);
                }
            }
        }
    }

    #[test]
    fn date_to_week_date_and_back_again() {
        for year in b::Year::MIN..=b::Year::MAX {
            for month in b::Month::MIN..=b::Month::MAX {
                for day in b::Day::MIN..=civil::days_in_month(year, month) {
                    let d = date(year, month, day);
                    let wd = d.to_iso_week_date();
                    let got = wd.to_date();
                    assert_eq!(d, got);
                }
            }
        }
    }

    #[test]
    fn date_to_day_of_year_and_back_again() {
        for year in b::Year::MIN..=b::Year::MAX {
            for month in b::Month::MIN..=b::Month::MAX {
                for day in b::Day::MIN..=civil::days_in_month(year, month) {
                    let d = date(year, month, day);
                    let doy = d.day_of_year();
                    let got = Date::from_day_of_year(year, doy);
                    assert_eq!(Ok(d), got);
                }
            }
        }
    }

    #[test]
    fn day_of_year_in_non_leap_year() {
        let year = 2026;
        let mut doy = 1;
        for month in b::Month::MIN..=b::Month::MAX {
            for day in b::Day::MIN..=civil::days_in_month(year, month) {
                let d = Date::from_day_of_year(year, doy).unwrap();
                assert_eq!(d, date(year, month, day));
                assert_eq!(d.day_of_year(), doy);
                doy += 1;
            }
        }
    }

    #[test]
    fn day_of_year_in_leap_year() {
        let year = 2024;
        let mut doy = 1;
        for month in b::Month::MIN..=b::Month::MAX {
            for day in b::Day::MIN..=civil::days_in_month(year, month) {
                let d = Date::from_day_of_year(year, doy).unwrap();
                assert_eq!(d, date(year, month, day));
                assert_eq!(d.day_of_year(), doy);
                doy += 1;
            }
        }
    }

    #[test]
    fn day_of_year_no_leap_in_non_leap_year() {
        let year = 2026;
        let mut doy = 1;
        for month in b::Month::MIN..=b::Month::MAX {
            for day in b::Day::MIN..=civil::days_in_month(year, month) {
                let d = Date::from_day_of_year_no_leap(year, doy).unwrap();
                assert_eq!(d, date(year, month, day));
                assert_eq!(d.day_of_year_no_leap(), Some(doy));
                doy += 1;
            }
        }
    }

    #[test]
    fn day_of_year_no_leap_in_leap_year() {
        let year = 2024;
        let mut doy = 1;
        for month in b::Month::MIN..=b::Month::MAX {
            for day in b::Day::MIN..=civil::days_in_month(year, month) {
                if month == 2 && day == 29 {
                    continue;
                }
                let d = Date::from_day_of_year_no_leap(year, doy).unwrap();
                assert_eq!(d, date(year, month, day));
                assert_eq!(d.day_of_year_no_leap(), Some(doy));
                doy += 1;
            }
        }
    }

    #[test]
    fn date_to_day_of_year_no_leap_and_back_again() {
        for year in b::Year::MIN..=b::Year::MAX {
            for month in b::Month::MIN..=b::Month::MAX {
                for day in b::Day::MIN..=civil::days_in_month(year, month) {
                    if month == 2 && day == 29 {
                        continue;
                    }
                    let d = date(year, month, day);
                    let doy = d.day_of_year_no_leap().unwrap();
                    let got = Date::from_day_of_year_no_leap(year, doy);
                    assert_eq!(Ok(d), got);
                }
            }
        }
    }

    #[test]
    fn unix_epoch_day_weekday() {
        let mk = |day| date(2026, 2, day).weekday();
        assert_eq!(mk(14), Weekday::Saturday);
        assert_eq!(mk(15), Weekday::Sunday);
        assert_eq!(mk(16), Weekday::Monday);
        assert_eq!(mk(17), Weekday::Tuesday);
        assert_eq!(mk(18), Weekday::Wednesday);
        assert_eq!(mk(19), Weekday::Thursday);
        assert_eq!(mk(20), Weekday::Friday);
        assert_eq!(mk(21), Weekday::Saturday);
    }

    #[test]
    fn first_of_last_of() {
        for year in b::Year::MIN..=b::Year::MAX {
            for month in b::Month::MIN..=b::Month::MAX {
                for day in b::Day::MIN..=civil::days_in_month(year, month) {
                    let d = date(year, month, day);

                    assert_eq!(d.first_of_month(), date(year, month, 1));
                    assert_eq!(
                        d.last_of_month(),
                        date(year, month, d.days_in_month())
                    );

                    assert_eq!(d.first_of_year(), date(year, 1, 1));
                    assert_eq!(d.last_of_year(), date(year, 12, 31));
                }
            }
        }
    }

    #[test]
    fn days_in() {
        for year in b::Year::MIN..=b::Year::MAX {
            for month in b::Month::MIN..=b::Month::MAX {
                for day in b::Day::MIN..=civil::days_in_month(year, month) {
                    let d = date(year, month, day);

                    assert!([365, 366].contains(&d.days_in_year()));
                    assert!([28, 29, 30, 31].contains(&d.days_in_month()));
                }
            }
        }
    }

    #[test]
    fn nth_weekday_of_month_various() {
        let d1 = date(2017, 3, 1);
        let wday = Weekday::Friday;
        assert_eq!(d1.nth_weekday_of_month(2, wday), Ok(date(2017, 3, 10)));

        let d1 = date(2024, 3, 1);
        let wday = Weekday::Thursday;
        assert_eq!(d1.nth_weekday_of_month(-1, wday), Ok(date(2024, 3, 28)));

        let d1 = date(2024, 3, 25);
        let wday = Weekday::Monday;
        assert!(d1.nth_weekday_of_month(5, wday).is_err());
        assert!(d1.nth_weekday_of_month(-5, wday).is_err());

        let d1 = date(1998, 1, 1);
        let wday = Weekday::Saturday;
        assert_eq!(d1.nth_weekday_of_month(5, wday), Ok(date(1998, 1, 31)));
    }

    #[test]
    fn nth_weekday_of_month_errors() {
        let d = date(2017, 3, 1);
        let wday = Weekday::Tuesday;

        assert!(d.nth_weekday_of_month(0, wday).is_err());
        assert!(d.nth_weekday_of_month(5, wday).is_err());
        assert!(d.nth_weekday_of_month(-5, wday).is_err());
        assert!(d.nth_weekday_of_month(6, wday).is_err());
        assert!(d.nth_weekday_of_month(-6, wday).is_err());
        assert!(d.nth_weekday_of_month(i8::MIN, wday).is_err());
        assert!(d.nth_weekday_of_month(i8::MAX, wday).is_err());

        assert_eq!(
            d.nth_weekday_of_month(5, Weekday::Friday),
            Ok(date(2017, 3, 31))
        );
        assert_eq!(
            d.nth_weekday_of_month(-5, Weekday::Friday),
            Ok(date(2017, 3, 3))
        );
    }

    #[test]
    fn nth_weekday_of_month_near_minimum_date() {
        let d = date(-9999, 1, 1);

        assert_eq!(
            d.nth_weekday_of_month(1, Weekday::Monday),
            Ok(date(-9999, 1, 1))
        );
        assert_eq!(
            d.nth_weekday_of_month(2, Weekday::Monday),
            Ok(date(-9999, 1, 8))
        );
        assert_eq!(
            d.nth_weekday_of_month(3, Weekday::Monday),
            Ok(date(-9999, 1, 15))
        );
        assert_eq!(
            d.nth_weekday_of_month(4, Weekday::Monday),
            Ok(date(-9999, 1, 22))
        );
        assert_eq!(
            d.nth_weekday_of_month(5, Weekday::Monday),
            Ok(date(-9999, 1, 29))
        );

        assert_eq!(
            d.nth_weekday_of_month(-5, Weekday::Monday),
            Ok(date(-9999, 1, 1))
        );
        assert_eq!(
            d.nth_weekday_of_month(-4, Weekday::Monday),
            Ok(date(-9999, 1, 8))
        );
        assert_eq!(
            d.nth_weekday_of_month(-3, Weekday::Monday),
            Ok(date(-9999, 1, 15))
        );
        assert_eq!(
            d.nth_weekday_of_month(-2, Weekday::Monday),
            Ok(date(-9999, 1, 22))
        );
        assert_eq!(
            d.nth_weekday_of_month(-1, Weekday::Monday),
            Ok(date(-9999, 1, 29))
        );
    }

    #[test]
    fn nth_weekday_of_month_near_maximum_date() {
        let d = date(9999, 12, 1);

        assert_eq!(
            d.nth_weekday_of_month(1, Weekday::Friday),
            Ok(date(9999, 12, 3))
        );
        assert_eq!(
            d.nth_weekday_of_month(2, Weekday::Friday),
            Ok(date(9999, 12, 10))
        );
        assert_eq!(
            d.nth_weekday_of_month(3, Weekday::Friday),
            Ok(date(9999, 12, 17))
        );
        assert_eq!(
            d.nth_weekday_of_month(4, Weekday::Friday),
            Ok(date(9999, 12, 24))
        );
        assert_eq!(
            d.nth_weekday_of_month(5, Weekday::Friday),
            Ok(date(9999, 12, 31))
        );

        assert_eq!(
            d.nth_weekday_of_month(-5, Weekday::Friday),
            Ok(date(9999, 12, 3))
        );
        assert_eq!(
            d.nth_weekday_of_month(-4, Weekday::Friday),
            Ok(date(9999, 12, 10))
        );
        assert_eq!(
            d.nth_weekday_of_month(-3, Weekday::Friday),
            Ok(date(9999, 12, 17))
        );
        assert_eq!(
            d.nth_weekday_of_month(-2, Weekday::Friday),
            Ok(date(9999, 12, 24))
        );
        assert_eq!(
            d.nth_weekday_of_month(-1, Weekday::Friday),
            Ok(date(9999, 12, 31))
        );
    }

    #[test]
    fn nth_weekday_of_month_every_month_has_four_weekdays() {
        for year in b::Year::MIN..=b::Year::MAX {
            for month in b::Month::MIN..=b::Month::MAX {
                let d = date(year, month, 1);
                for weekday in Weekday::Sunday.cycle_forward().take(7) {
                    for nth in [-4, -3, -2, -1, 1, 2, 3, 4] {
                        assert!(d.nth_weekday_of_month(nth, weekday).is_ok());
                        // never valid
                        assert!(d.nth_weekday_of_month(0, weekday).is_err());
                        assert!(d.nth_weekday_of_month(6, weekday).is_err());
                        assert!(d.nth_weekday_of_month(-6, weekday).is_err());
                    }
                }
            }
        }
    }

    #[test]
    fn nth_weekday_various() {
        let d = date(2024, 3, 10);

        assert_eq!(d.nth_weekday(1, Weekday::Monday), Ok(date(2024, 3, 11)));
        assert_eq!(d.nth_weekday(1, Weekday::Sunday), Ok(date(2024, 3, 17)));
        assert_eq!(d.nth_weekday(2, Weekday::Thursday), Ok(date(2024, 3, 21)));

        assert_eq!(d.nth_weekday(-1, Weekday::Monday), Ok(date(2024, 3, 4)));
        assert_eq!(d.nth_weekday(-1, Weekday::Sunday), Ok(date(2024, 3, 3)));
        assert_eq!(
            d.nth_weekday(-2, Weekday::Thursday),
            Ok(date(2024, 2, 29))
        );

        let d = date(9999, 12, 24);
        assert_eq!(d.nth_weekday(1, Weekday::Friday), Ok(date(9999, 12, 31)));
        let d = date(9999, 12, 30);
        assert_eq!(d.nth_weekday(1, Weekday::Friday), Ok(date(9999, 12, 31)));
        let d = date(9999, 12, 31);
        assert_eq!(d.nth_weekday(-1, Weekday::Friday), Ok(date(9999, 12, 24)));
        assert!(d.nth_weekday(1, Weekday::Friday).is_err());

        let d = date(-9999, 1, 8);
        assert_eq!(d.nth_weekday(-1, Weekday::Monday), Ok(date(-9999, 1, 1)));
        let d = date(-9999, 1, 2);
        assert_eq!(d.nth_weekday(-1, Weekday::Monday), Ok(date(-9999, 1, 1)));
        let d = date(-9999, 1, 1);
        assert_eq!(d.nth_weekday(1, Weekday::Monday), Ok(date(-9999, 1, 8)));
        assert!(d.nth_weekday(-1, Weekday::Monday).is_err());
    }

    #[test]
    fn nth_weekday_errors() {
        let d = date(2024, 3, 10);
        assert!(d.nth_weekday(0, Weekday::Monday).is_err());
    }

    #[test]
    fn nth_weekday_extreme() {
        let weeks = 1_043_497;

        let d1 = date(-9999, 1, 1);
        let d2 = d1.nth_weekday(weeks, Weekday::Monday).unwrap();
        assert_eq!(d2, date(9999, 12, 27));
        assert!(d1.nth_weekday(weeks + 1, Weekday::Monday).is_err());
        assert!(d1.nth_weekday(i32::MIN, Weekday::Monday).is_err());
        assert!(d1.nth_weekday(i32::MAX, Weekday::Monday).is_err());

        let d1 = date(9999, 12, 31);
        let d2 = d1.nth_weekday(-weeks, Weekday::Friday).unwrap();
        assert_eq!(d2, date(-9999, 1, 5));
        assert!(d1.nth_weekday(weeks - 1, Weekday::Friday).is_err());
        assert!(d1.nth_weekday(i32::MIN, Weekday::Friday).is_err());
        assert!(d1.nth_weekday(i32::MAX, Weekday::Friday).is_err());
    }

    #[test]
    fn yesterday() {
        assert_eq!(date(2024, 7, 3).yesterday(), Ok(date(2024, 7, 2)));
        assert_eq!(date(2024, 7, 1).yesterday(), Ok(date(2024, 6, 30)));
        assert_eq!(date(2024, 6, 1).yesterday(), Ok(date(2024, 5, 31)));
        assert_eq!(date(2024, 3, 1).yesterday(), Ok(date(2024, 2, 29)));
        assert_eq!(date(2023, 3, 1).yesterday(), Ok(date(2023, 2, 28)));
        assert_eq!(date(2023, 1, 1).yesterday(), Ok(date(2022, 12, 31)));
        assert_eq!(date(-9999, 1, 2).yesterday(), Ok(date(-9999, 1, 1)));
        assert_eq!(date(9999, 12, 31).yesterday(), Ok(date(9999, 12, 30)));

        assert!(date(-9999, 1, 1).yesterday().is_err());
    }

    #[test]
    fn tomorrow() {
        assert_eq!(date(2024, 7, 3).tomorrow(), Ok(date(2024, 7, 4)));
        assert_eq!(date(2024, 6, 30).tomorrow(), Ok(date(2024, 7, 1)));
        assert_eq!(date(2024, 5, 30).tomorrow(), Ok(date(2024, 5, 31)));
        assert_eq!(date(2024, 5, 31).tomorrow(), Ok(date(2024, 6, 1)));
        assert_eq!(date(2024, 2, 28).tomorrow(), Ok(date(2024, 2, 29)));
        assert_eq!(date(2024, 2, 29).tomorrow(), Ok(date(2024, 3, 1)));
        assert_eq!(date(2023, 2, 28).tomorrow(), Ok(date(2023, 3, 1)));
        assert_eq!(date(2023, 12, 31).tomorrow(), Ok(date(2024, 1, 1)));
        assert_eq!(date(-9999, 1, 1).tomorrow(), Ok(date(-9999, 1, 2)));
        assert_eq!(date(9999, 12, 30).tomorrow(), Ok(date(9999, 12, 31)));

        assert!(date(9999, 12, 31).tomorrow().is_err());
    }

    #[test]
    fn iso_week_date_tomorrow() {
        assert_eq!(
            week_date(2024, 27, Weekday::Wednesday).tomorrow(),
            Ok(week_date(2024, 27, Weekday::Thursday)),
        );
        assert_eq!(
            week_date(2024, 27, Weekday::Sunday).tomorrow(),
            Ok(week_date(2024, 28, Weekday::Monday)),
        );
        assert_eq!(
            week_date(2024, 52, Weekday::Sunday).tomorrow(),
            Ok(week_date(2025, 1, Weekday::Monday)),
        );
        assert_eq!(
            week_date(2025, 1, Weekday::Monday).tomorrow(),
            Ok(week_date(2025, 1, Weekday::Tuesday)),
        );
        assert_eq!(
            week_date(2025, 1, Weekday::Tuesday).tomorrow(),
            Ok(week_date(2025, 1, Weekday::Wednesday)),
        );
        assert_eq!(
            week_date(2026, 52, Weekday::Sunday).tomorrow(),
            Ok(week_date(2026, 53, Weekday::Monday)),
        );
        assert_eq!(
            week_date(2026, 53, Weekday::Sunday).tomorrow(),
            Ok(week_date(2027, 1, Weekday::Monday)),
        );
        assert_eq!(
            week_date(-9999, 1, Weekday::Monday).tomorrow(),
            Ok(week_date(-9999, 1, Weekday::Tuesday)),
        );
        assert_eq!(
            week_date(9999, 52, Weekday::Thursday).tomorrow(),
            Ok(week_date(9999, 52, Weekday::Friday)),
        );

        assert!(week_date(9999, 52, Weekday::Friday).tomorrow().is_err());
    }

    #[test]
    fn iso_week_date_yesterday() {
        assert_eq!(
            week_date(2024, 27, Weekday::Thursday).yesterday(),
            Ok(week_date(2024, 27, Weekday::Wednesday)),
        );
        assert_eq!(
            week_date(2024, 28, Weekday::Monday).yesterday(),
            Ok(week_date(2024, 27, Weekday::Sunday)),
        );
        assert_eq!(
            week_date(2025, 1, Weekday::Monday).yesterday(),
            Ok(week_date(2024, 52, Weekday::Sunday)),
        );
        assert_eq!(
            week_date(2025, 1, Weekday::Tuesday).yesterday(),
            Ok(week_date(2025, 1, Weekday::Monday)),
        );
        assert_eq!(
            week_date(2025, 1, Weekday::Wednesday).yesterday(),
            Ok(week_date(2025, 1, Weekday::Tuesday)),
        );
        assert_eq!(
            week_date(2026, 53, Weekday::Monday).yesterday(),
            Ok(week_date(2026, 52, Weekday::Sunday)),
        );
        assert_eq!(
            week_date(2027, 1, Weekday::Monday).yesterday(),
            Ok(week_date(2026, 53, Weekday::Sunday)),
        );
        assert_eq!(
            week_date(9999, 12, Weekday::Friday).yesterday(),
            Ok(week_date(9999, 12, Weekday::Thursday)),
        );
        assert_eq!(
            week_date(-9999, 1, Weekday::Tuesday).yesterday(),
            Ok(week_date(-9999, 1, Weekday::Monday)),
        );

        assert!(week_date(-9999, 1, Weekday::Monday).yesterday().is_err());
    }

    #[test]
    fn add() {
        assert_eq!(date(2024, 7, 3).checked_add(-1), Ok(date(2024, 7, 2)));
        assert_eq!(date(2024, 7, 1).checked_add(-1), Ok(date(2024, 6, 30)));
        assert_eq!(date(2024, 6, 1).checked_add(-1), Ok(date(2024, 5, 31)));
        assert_eq!(date(2024, 3, 1).checked_add(-1), Ok(date(2024, 2, 29)));
        assert_eq!(date(2023, 3, 1).checked_add(-1), Ok(date(2023, 2, 28)));
        assert_eq!(date(2023, 1, 1).checked_add(-1), Ok(date(2022, 12, 31)));
        assert_eq!(date(-9999, 1, 2).checked_add(-1), Ok(date(-9999, 1, 1)));
        assert_eq!(date(9999, 12, 31).checked_add(-1), Ok(date(9999, 12, 30)));

        assert_eq!(date(2024, 7, 3).checked_add(1), Ok(date(2024, 7, 4)));
        assert_eq!(date(2024, 6, 30).checked_add(1), Ok(date(2024, 7, 1)));
        assert_eq!(date(2024, 5, 30).checked_add(1), Ok(date(2024, 5, 31)));
        assert_eq!(date(2024, 5, 31).checked_add(1), Ok(date(2024, 6, 1)));
        assert_eq!(date(2024, 2, 28).checked_add(1), Ok(date(2024, 2, 29)));
        assert_eq!(date(2024, 2, 29).checked_add(1), Ok(date(2024, 3, 1)));
        assert_eq!(date(2023, 2, 28).checked_add(1), Ok(date(2023, 3, 1)));
        assert_eq!(date(2023, 12, 31).checked_add(1), Ok(date(2024, 1, 1)));
        assert_eq!(date(-9999, 1, 1).checked_add(1), Ok(date(-9999, 1, 2)));
        assert_eq!(date(9999, 12, 30).checked_add(1), Ok(date(9999, 12, 31)));

        assert_eq!(date(2024, 7, 3).checked_add(2), Ok(date(2024, 7, 5)));
        assert_eq!(date(2024, 6, 29).checked_add(2), Ok(date(2024, 7, 1)));
        assert_eq!(date(2024, 5, 29).checked_add(2), Ok(date(2024, 5, 31)));
        assert_eq!(date(2024, 5, 30).checked_add(2), Ok(date(2024, 6, 1)));
        assert_eq!(date(2024, 2, 27).checked_add(2), Ok(date(2024, 2, 29)));
        assert_eq!(date(2024, 2, 28).checked_add(2), Ok(date(2024, 3, 1)));
        assert_eq!(date(2023, 2, 27).checked_add(2), Ok(date(2023, 3, 1)));
        assert_eq!(date(2023, 12, 30).checked_add(2), Ok(date(2024, 1, 1)));
        assert_eq!(date(-9999, 1, 1).checked_add(2), Ok(date(-9999, 1, 3)));
        assert_eq!(date(9999, 12, 29).checked_add(2), Ok(date(9999, 12, 31)));

        let max_days = (b::UnixEpochDays::LEN - 1) as i32;

        assert_eq!(
            date(-9999, 1, 1).checked_add(max_days),
            Ok(date(9999, 12, 31))
        );
        assert_eq!(
            date(9999, 12, 31).checked_add(-max_days),
            Ok(date(-9999, 1, 1))
        );

        assert!(date(-9999, 1, 1).checked_add(-1).is_err());
        assert!(date(9999, 12, 31).checked_add(1).is_err());
        assert!(date(-9999, 1, 1).checked_add(max_days + 1).is_err());
        assert!(date(9999, 12, 31).checked_add(-(max_days + 1)).is_err());
    }

    #[test]
    fn sub() {
        assert_eq!(date(-9999, 1, 1).checked_sub(-1), Ok(date(-9999, 1, 2)));
        assert_eq!(date(-9999, 1, 1).checked_sub(-2), Ok(date(-9999, 1, 3)));

        assert!(date(-9999, 1, 1).checked_sub(1).is_err());
        assert!(date(-9999, 1, 1).checked_sub(i32::MIN).is_err());
        assert!(date(9999, 12, 31).checked_sub(i32::MAX).is_err());
    }

    #[test]
    fn prev_year() {
        assert_eq!(date(2024, 2, 29).prev_year(), Ok(2023));
        assert_eq!(date(2024, 1, 1).prev_year(), Ok(2023));
        assert_eq!(date(2023, 12, 31).prev_year(), Ok(2022));
        assert_eq!(date(9999, 12, 31).prev_year(), Ok(9998));

        assert!(date(-9999, 12, 31).prev_year().is_err());
        assert!(date(-9999, 1, 1).prev_year().is_err());
    }

    #[test]
    fn next_year() {
        assert_eq!(date(2024, 2, 29).next_year(), Ok(2025));
        assert_eq!(date(2024, 1, 1).next_year(), Ok(2025));
        assert_eq!(date(2023, 12, 31).next_year(), Ok(2024));
        assert_eq!(date(-9999, 12, 31).next_year(), Ok(-9998));

        assert!(date(9999, 12, 31).next_year().is_err());
        assert!(date(9999, 1, 1).next_year().is_err());
    }

    #[test]
    fn to_iso_week_date_various() {
        assert_eq!(
            date(1995, 1, 1).to_iso_week_date(),
            week_date(1994, 52, Weekday::Sunday),
        );
        assert_eq!(
            date(1996, 12, 31).to_iso_week_date(),
            week_date(1997, 1, Weekday::Tuesday),
        );
        assert_eq!(
            date(2019, 12, 30).to_iso_week_date(),
            week_date(2020, 1, Weekday::Monday),
        );
        assert_eq!(
            date(2031, 12, 29).to_iso_week_date(),
            week_date(2032, 1, Weekday::Monday),
        );
        assert_eq!(
            date(2024, 3, 9).to_iso_week_date(),
            week_date(2024, 10, Weekday::Saturday),
        );
        assert_eq!(
            Date::MIN.to_iso_week_date(),
            week_date(-9999, 1, Weekday::Monday),
        );
        assert_eq!(
            Date::MAX.to_iso_week_date(),
            week_date(9999, 52, Weekday::Friday),
        );
    }

    quickcheck::quickcheck! {
        fn prop_tomorrow_yesterday_is_identity(d: Date) -> quickcheck::TestResult {
            let Ok(yesterday) = d.yesterday() else {
                return quickcheck::TestResult::discard()
            };
            quickcheck::TestResult::from_bool(yesterday.tomorrow() == Ok(d))
        }

        fn prop_yesterday_tomorrow_is_identity(d: Date) -> quickcheck::TestResult {
            let Ok(tomorrow) = d.tomorrow() else {
                return quickcheck::TestResult::discard()
            };
            quickcheck::TestResult::from_bool(tomorrow.yesterday() == Ok(d))
        }

        fn prop_add_equals_sub(d1: Date, days: i32) -> quickcheck::TestResult {
            let Ok(d2) = d1.checked_add(days) else {
                return quickcheck::TestResult::discard();
            };
            quickcheck::TestResult::from_bool(d2.checked_sub(days) == Ok(d1))
        }

        fn prop_all_long_years_have_53rd_week(year: i16) -> quickcheck::TestResult {
            if b::ISOYear::check(year).is_err() {
                return quickcheck::TestResult::discard();
            }
            quickcheck::TestResult::from_bool(
                !civil::is_long_iso_week_year(year)
                 || ISOWeekDate::new(year, 53, Weekday::Sunday).is_ok()
            )
        }

        fn prop_prev_day_is_less(wd: ISOWeekDate) -> quickcheck::TestResult {
            let Ok(prev_date) = wd.to_date().yesterday() else {
                return quickcheck::TestResult::discard();
            };
            quickcheck::TestResult::from_bool(
                prev_date.to_iso_week_date() < wd,
            )
        }

        fn prop_next_day_is_greater(wd: ISOWeekDate) -> quickcheck::TestResult {
            let Ok(next_date) = wd.to_date().tomorrow() else {
                return quickcheck::TestResult::discard();
            };
            quickcheck::TestResult::from_bool(
                wd < next_date.to_iso_week_date(),
            )
        }

        fn prop_iso_tomorrow_yesterday_is_identity(
            wd: ISOWeekDate
        ) -> quickcheck::TestResult {
            let Ok(yesterday) = wd.yesterday() else {
                return quickcheck::TestResult::discard()
            };
            quickcheck::TestResult::from_bool(yesterday.tomorrow() == Ok(wd))
        }

        fn prop_iso_yesterday_tomorrow_is_identity(
            wd: ISOWeekDate
        ) -> quickcheck::TestResult {
            let Ok(tomorrow) = wd.tomorrow() else {
                return quickcheck::TestResult::discard()
            };
            quickcheck::TestResult::from_bool(tomorrow.yesterday() == Ok(wd))
        }
    }
}
