//! An iterator over a range of [`Date`]s, yielding each day from start to end inclusive.

use core::iter::FusedIterator;

use crate::Date;

/// An iterator over a range of [`Date`]s, yielding each day from start to end inclusive.
///
/// This struct is created by [`Date::iter_to`] or [`DateIter::new`].
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DateIter {
    front: Date,
    back: Option<Date>,
}

impl DateIter {
    /// Create a new `DateIter` from `start` to `end` inclusive.
    ///
    /// If `start > end`, the iterator will be empty.
    ///
    /// ```rust
    /// # use time::iter::DateIter;
    /// # use time_macros::date;
    /// let mut iter = DateIter::new(date!(2019-01-01), date!(2019-01-03));
    /// assert_eq!(iter.next(), Some(date!(2019-01-01)));
    /// assert_eq!(iter.next(), Some(date!(2019-01-02)));
    /// assert_eq!(iter.next(), Some(date!(2019-01-03)));
    /// assert_eq!(iter.next(), None);
    /// ```
    #[inline]
    pub const fn new(start: Date, end: Date) -> Self {
        Self {
            front: start,
            back: if start.as_i32() <= end.as_i32() {
                Some(end)
            } else {
                None
            },
        }
    }

    /// Returns `true` if the iterator is empty, `false` otherwise.
    #[inline]
    const fn is_exhausted(&self) -> bool {
        self.back.is_none()
    }

    /// Make the iterator exhausted.
    #[inline]
    const fn make_exhausted(&mut self) {
        self.back = None;
    }

    /// Return the number of days from `front` to `back`. Uses ordinal arithmetic as a fast path
    /// when both dates fall in the same year, avoiding the more expensive Julian day computation.
    #[inline]
    const fn days_between(front: Date, back: Date) -> i32 {
        if front.year() == back.year() {
            back.ordinal() as i32 - front.ordinal() as i32
        } else {
            back.to_julian_day() - front.to_julian_day()
        }
    }
}

impl Iterator for DateIter {
    type Item = Date;

    #[inline]
    fn next(&mut self) -> Option<Self::Item> {
        let current = self.front;
        let back = self.back?;

        if current < back {
            // Safety: `current < back`, so `current` has a successor.
            self.front = unsafe { current.next_day().unwrap_unchecked() };
        } else {
            self.make_exhausted();
        }

        Some(current)
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        let len = self.len();
        (len, Some(len))
    }

    #[inline]
    fn count(self) -> usize {
        self.len()
    }

    #[inline]
    fn last(self) -> Option<Self::Item> {
        self.back
    }

    #[inline]
    fn nth(&mut self, n: usize) -> Option<Self::Item> {
        let back = self.back?;
        let front = self.front;

        let same_year = front.year() == back.year();
        let remaining = if same_year {
            (back.ordinal() - front.ordinal()) as usize
        } else {
            (back.to_julian_day() - front.to_julian_day()) as usize
        };

        if n > remaining {
            self.make_exhausted();
            return None;
        }

        // Fast path for when the result is in the same year, avoiding the more expensive Julian day
        // computation.
        let result = if same_year
            || front.ordinal() as usize + n <= if front.is_in_leap_year() { 366 } else { 365 }
        {
            // Safety: We know that we're staying in the same year and that the resulting ordinal is
            // valid.
            unsafe { front.add_days_unchecked(n as i32) }
        } else {
            // Safety: `n <= remaining = back_jd - front_jd`, so `front_jd + n <= back_jd`.
            unsafe { Date::from_julian_day_unchecked(front.to_julian_day() + n as i32) }
        };

        if n == remaining {
            self.make_exhausted();
        } else {
            // Safety: `result < back`, so `result` has a successor.
            self.front = unsafe { result.next_day().unwrap_unchecked() };
        }

        Some(result)
    }

    #[inline]
    fn max(self) -> Option<Self::Item> {
        self.back
    }

    #[inline]
    fn min(self) -> Option<Self::Item> {
        (!self.is_exhausted()).then_some(self.front)
    }

    #[inline]
    fn is_sorted(self) -> bool {
        true
    }
}

impl DoubleEndedIterator for DateIter {
    #[inline]
    fn next_back(&mut self) -> Option<Self::Item> {
        let back = self.back?;

        if self.front < back {
            // Safety: `front < back`, so `back` has a predecessor.
            self.back = Some(unsafe { back.previous_day().unwrap_unchecked() });
        } else {
            self.make_exhausted();
        }

        Some(back)
    }

    #[inline]
    fn nth_back(&mut self, n: usize) -> Option<Self::Item> {
        let back = self.back?;
        let front = self.front;

        let same_year = front.year() == back.year();
        let remaining = if same_year {
            (back.ordinal() - front.ordinal()) as usize
        } else {
            (back.to_julian_day() - front.to_julian_day()) as usize
        };

        if n > remaining {
            self.make_exhausted();
            return None;
        }

        let result = if same_year || back.ordinal() as usize > n {
            // Safety: `back.ordinal() > n`, so subtracting stays within `back`'s year.
            unsafe { back.add_days_unchecked(-(n as i32)) }
        } else {
            // Safety: `n <= remaining = back_jd - front_jd`, so `back_jd - n >= front_jd`.
            unsafe { Date::from_julian_day_unchecked(back.to_julian_day() - n as i32) }
        };

        if n == remaining {
            self.make_exhausted();
        } else {
            // Safety: `result > front`, so `result` has a predecessor.
            self.back = Some(unsafe { result.previous_day().unwrap_unchecked() });
        }

        Some(result)
    }
}

impl ExactSizeIterator for DateIter {
    #[inline]
    fn len(&self) -> usize {
        let Some(back) = self.back else {
            return 0;
        };

        Self::days_between(self.front, back) as usize + 1
    }
}

impl FusedIterator for DateIter {}
