//! Iterator types.

mod date_iter;
mod month_iter;
mod weekday_iter;

pub use self::date_iter::DateIter;
pub use self::month_iter::MonthIter;
pub use self::weekday_iter::WeekdayIter;

/// An iterator that yields the elements of an underlying iterator in reverse order.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Rev<I> {
    iter: I,
}
