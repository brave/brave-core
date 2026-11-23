//! An infinite iterator over [`Month`]s.

use core::iter::FusedIterator;

use super::Rev;
use crate::Month::{self, *};

pub(super) const ALL_MONTHS: [Month; 12] = [
    January, February, March, April, May, June, July, August, September, October, November,
    December,
];

/// An infinite iterator over [`Month`]s.
///
/// This struct is created by [`Month::iter_from`] or [`MonthIter::new`].
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MonthIter {
    current: Month,
}

impl MonthIter {
    /// Create a new `MonthIter` starting at `start`.
    ///
    /// ```rust
    /// # use time::{Month, iter::MonthIter};
    /// let mut iter = MonthIter::new(Month::January);
    /// assert_eq!(iter.next(), Some(Month::January));
    /// assert_eq!(iter.next(), Some(Month::February));
    /// assert_eq!(iter.next(), Some(Month::March));
    /// ```
    #[inline]
    pub const fn new(start: Month) -> Self {
        Self { current: start }
    }

    /// Make the iterator go in reverse order.
    ///
    /// ```rust
    /// # use time::{Month, iter::MonthIter};
    /// let mut iter = MonthIter::new(Month::January).rev();
    /// assert_eq!(iter.next(), Some(Month::January));
    /// assert_eq!(iter.next(), Some(Month::December));
    /// assert_eq!(iter.next(), Some(Month::November));
    /// ```
    #[inline]
    pub const fn rev(self) -> Rev<Self> {
        Rev { iter: self }
    }
}

impl Iterator for MonthIter {
    type Item = Month;

    #[inline]
    fn next(&mut self) -> Option<Self::Item> {
        let current = self.current;
        self.current = current.next();
        Some(current)
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        (usize::MAX, None)
    }

    #[inline]
    fn nth(&mut self, n: usize) -> Option<Self::Item> {
        let result = self.current.nth_next((n % 12) as u8);
        self.current = result.next();
        Some(result)
    }

    fn count(self) -> usize {
        panic!("`MonthIter` is infinite and cannot be counted")
    }

    fn last(self) -> Option<Self::Item> {
        panic!("`MonthIter` is infinite and has no last element")
    }

    #[inline]
    fn all<F>(&mut self, f: F) -> bool
    where
        F: FnMut(Self::Item) -> bool,
    {
        ALL_MONTHS.into_iter().all(f)
    }

    #[inline]
    fn any<F>(&mut self, f: F) -> bool
    where
        F: FnMut(Self::Item) -> bool,
    {
        ALL_MONTHS.into_iter().any(f)
    }

    #[inline]
    fn max(self) -> Option<Self::Item> {
        Some(December)
    }

    #[inline]
    fn min(self) -> Option<Self::Item> {
        Some(January)
    }

    #[inline]
    fn is_sorted(self) -> bool {
        // The iterator will always have December followed by January, so it is not sorted.
        false
    }
}

impl Iterator for Rev<MonthIter> {
    type Item = Month;

    #[inline]
    fn next(&mut self) -> Option<Self::Item> {
        let current = self.iter.current;
        self.iter.current = current.previous();
        Some(current)
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        (usize::MAX, None)
    }

    #[inline]
    fn nth(&mut self, n: usize) -> Option<Self::Item> {
        let result = self.iter.current.nth_prev((n % 12) as u8);
        self.iter.current = result.previous();
        Some(result)
    }

    fn count(self) -> usize {
        panic!("`Rev<MonthIter>` is infinite and cannot be counted")
    }

    fn last(self) -> Option<Self::Item> {
        panic!("`Rev<MonthIter>` is infinite and has no last element")
    }

    #[inline]
    fn all<F>(&mut self, f: F) -> bool
    where
        F: FnMut(Self::Item) -> bool,
    {
        ALL_MONTHS.into_iter().all(f)
    }

    #[inline]
    fn any<F>(&mut self, f: F) -> bool
    where
        F: FnMut(Self::Item) -> bool,
    {
        ALL_MONTHS.into_iter().any(f)
    }

    #[inline]
    fn max(self) -> Option<Self::Item> {
        Some(December)
    }

    #[inline]
    fn min(self) -> Option<Self::Item> {
        Some(January)
    }

    #[inline]
    fn is_sorted(self) -> bool {
        false
    }
}

impl FusedIterator for MonthIter {}
impl FusedIterator for Rev<MonthIter> {}
