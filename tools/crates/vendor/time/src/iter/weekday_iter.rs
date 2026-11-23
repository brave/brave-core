//! An infinite iterator over [`Weekday`]s.

use core::iter::FusedIterator;

use super::Rev;
use crate::Weekday::{self, *};

const ALL_WEEKDAYS: [Weekday; 7] = [
    Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday,
];

/// An infinite iterator over [`Weekday`]s.
///
/// This struct is created by [`Weekday::iter_from`] or [`WeekdayIter::new`].
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct WeekdayIter {
    current: Weekday,
}

impl WeekdayIter {
    /// Create a new `WeekdayIter` starting at `start`.
    ///
    /// ```rust
    /// # use time::{Weekday, iter::WeekdayIter};
    /// let mut iter = WeekdayIter::new(Weekday::Monday);
    /// assert_eq!(iter.next(), Some(Weekday::Monday));
    /// assert_eq!(iter.next(), Some(Weekday::Tuesday));
    /// assert_eq!(iter.next(), Some(Weekday::Wednesday));
    /// ```
    #[inline]
    pub const fn new(start: Weekday) -> Self {
        Self { current: start }
    }

    /// Make the iterator go in reverse order.
    ///
    /// ```rust
    /// # use time::{Weekday, iter::WeekdayIter};
    /// let mut iter = WeekdayIter::new(Weekday::Monday).rev();
    /// assert_eq!(iter.next(), Some(Weekday::Monday));
    /// assert_eq!(iter.next(), Some(Weekday::Sunday));
    /// assert_eq!(iter.next(), Some(Weekday::Saturday));
    /// ```
    #[inline]
    pub const fn rev(self) -> Rev<Self> {
        Rev { iter: self }
    }
}

impl Iterator for WeekdayIter {
    type Item = Weekday;

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
        let result = self.current.nth_next((n % 7) as u8);
        self.current = result.next();
        Some(result)
    }

    fn count(self) -> usize {
        panic!("`WeekdayIter` is infinite and cannot be counted")
    }

    fn last(self) -> Option<Self::Item> {
        panic!("`WeekdayIter` is infinite and has no last element")
    }

    #[inline]
    fn all<F>(&mut self, f: F) -> bool
    where
        F: FnMut(Self::Item) -> bool,
    {
        ALL_WEEKDAYS.into_iter().all(f)
    }

    #[inline]
    fn any<F>(&mut self, f: F) -> bool
    where
        F: FnMut(Self::Item) -> bool,
    {
        ALL_WEEKDAYS.into_iter().any(f)
    }
}

impl Iterator for Rev<WeekdayIter> {
    type Item = Weekday;

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
        let result = self.iter.current.nth_prev((n % 7) as u8);
        self.iter.current = result.previous();
        Some(result)
    }

    fn count(self) -> usize {
        panic!("`Rev<WeekdayIter>` is infinite and cannot be counted")
    }

    fn last(self) -> Option<Self::Item> {
        panic!("`Rev<WeekdayIter>` is infinite and has no last element")
    }

    #[inline]
    fn all<F>(&mut self, f: F) -> bool
    where
        F: FnMut(Self::Item) -> bool,
    {
        ALL_WEEKDAYS.into_iter().all(f)
    }

    #[inline]
    fn any<F>(&mut self, f: F) -> bool
    where
        F: FnMut(Self::Item) -> bool,
    {
        ALL_WEEKDAYS.into_iter().any(f)
    }
}

impl FusedIterator for WeekdayIter {}
impl FusedIterator for Rev<WeekdayIter> {}
