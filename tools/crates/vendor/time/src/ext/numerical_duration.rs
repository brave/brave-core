use crate::SignedDuration;
use crate::unit::*;

/// Sealed trait to prevent downstream implementations.
mod sealed {
    /// A trait that cannot be implemented by downstream users.
    pub trait Sealed {}
    impl Sealed for i64 {}
    impl Sealed for f64 {}
}

/// Create [`SignedDuration`]s from numeric literals.
///
/// # Examples
///
/// Basic construction of [`SignedDuration`]s.
///
/// ```rust
/// # use time::{SignedDuration, ext::NumericalDuration};
/// assert_eq!(5.nanoseconds(), SignedDuration::nanoseconds(5));
/// assert_eq!(5.microseconds(), SignedDuration::microseconds(5));
/// assert_eq!(5.milliseconds(), SignedDuration::milliseconds(5));
/// assert_eq!(5.seconds(), SignedDuration::seconds(5));
/// assert_eq!(5.minutes(), SignedDuration::minutes(5));
/// assert_eq!(5.hours(), SignedDuration::hours(5));
/// assert_eq!(5.days(), SignedDuration::days(5));
/// assert_eq!(5.weeks(), SignedDuration::weeks(5));
/// ```
///
/// Signed integers work as well!
///
/// ```rust
/// # use time::{SignedDuration, ext::NumericalDuration};
/// assert_eq!((-5).nanoseconds(), SignedDuration::nanoseconds(-5));
/// assert_eq!((-5).microseconds(), SignedDuration::microseconds(-5));
/// assert_eq!((-5).milliseconds(), SignedDuration::milliseconds(-5));
/// assert_eq!((-5).seconds(), SignedDuration::seconds(-5));
/// assert_eq!((-5).minutes(), SignedDuration::minutes(-5));
/// assert_eq!((-5).hours(), SignedDuration::hours(-5));
/// assert_eq!((-5).days(), SignedDuration::days(-5));
/// assert_eq!((-5).weeks(), SignedDuration::weeks(-5));
/// ```
///
/// Just like any other [`SignedDuration`], they can be added, subtracted, etc.
///
/// ```rust
/// # use time::ext::NumericalDuration;
/// assert_eq!(2.seconds() + 500.milliseconds(), 2_500.milliseconds());
/// assert_eq!(2.seconds() - 500.milliseconds(), 1_500.milliseconds());
/// ```
///
/// When called on floating point values, any remainder of the floating point value will be
/// truncated. Keep in mind that floating point numbers are inherently imprecise and have
/// limited capacity.
#[diagnostic::on_unimplemented(note = "this extension trait is intended to be used with numeric \
                                       literals, such as `5.seconds()`")]
pub trait NumericalDuration: sealed::Sealed {
    /// Create a [`SignedDuration`] from the number of nanoseconds.
    fn nanoseconds(self) -> SignedDuration;
    /// Create a [`SignedDuration`] from the number of microseconds.
    fn microseconds(self) -> SignedDuration;
    /// Create a [`SignedDuration`] from the number of milliseconds.
    fn milliseconds(self) -> SignedDuration;
    /// Create a [`SignedDuration`] from the number of seconds.
    fn seconds(self) -> SignedDuration;
    /// Create a [`SignedDuration`] from the number of minutes.
    fn minutes(self) -> SignedDuration;
    /// Create a [`SignedDuration`] from the number of hours.
    fn hours(self) -> SignedDuration;
    /// Create a [`SignedDuration`] from the number of days.
    fn days(self) -> SignedDuration;
    /// Create a [`SignedDuration`] from the number of weeks.
    fn weeks(self) -> SignedDuration;
}

impl NumericalDuration for i64 {
    #[inline]
    fn nanoseconds(self) -> SignedDuration {
        SignedDuration::nanoseconds(self)
    }

    #[inline]
    fn microseconds(self) -> SignedDuration {
        SignedDuration::microseconds(self)
    }

    #[inline]
    fn milliseconds(self) -> SignedDuration {
        SignedDuration::milliseconds(self)
    }

    #[inline]
    fn seconds(self) -> SignedDuration {
        SignedDuration::seconds(self)
    }

    #[inline]
    #[track_caller]
    fn minutes(self) -> SignedDuration {
        SignedDuration::minutes(self)
    }

    #[inline]
    #[track_caller]
    fn hours(self) -> SignedDuration {
        SignedDuration::hours(self)
    }

    #[inline]
    #[track_caller]
    fn days(self) -> SignedDuration {
        SignedDuration::days(self)
    }

    #[inline]
    #[track_caller]
    fn weeks(self) -> SignedDuration {
        SignedDuration::weeks(self)
    }
}

impl NumericalDuration for f64 {
    #[inline]
    fn nanoseconds(self) -> SignedDuration {
        SignedDuration::nanoseconds(self as i64)
    }

    #[inline]
    fn microseconds(self) -> SignedDuration {
        SignedDuration::nanoseconds((self * Nanosecond::per_t::<Self>(Microsecond)) as i64)
    }

    #[inline]
    fn milliseconds(self) -> SignedDuration {
        SignedDuration::nanoseconds((self * Nanosecond::per_t::<Self>(Millisecond)) as i64)
    }

    #[inline]
    fn seconds(self) -> SignedDuration {
        SignedDuration::nanoseconds((self * Nanosecond::per_t::<Self>(Second)) as i64)
    }

    #[inline]
    #[track_caller]
    fn minutes(self) -> SignedDuration {
        SignedDuration::nanoseconds((self * Nanosecond::per_t::<Self>(Minute)) as i64)
    }

    #[inline]
    #[track_caller]
    fn hours(self) -> SignedDuration {
        SignedDuration::nanoseconds((self * Nanosecond::per_t::<Self>(Hour)) as i64)
    }

    #[inline]
    #[track_caller]
    fn days(self) -> SignedDuration {
        SignedDuration::nanoseconds((self * Nanosecond::per_t::<Self>(Day)) as i64)
    }

    #[inline]
    #[track_caller]
    fn weeks(self) -> SignedDuration {
        SignedDuration::nanoseconds((self * Nanosecond::per_t::<Self>(Week)) as i64)
    }
}
