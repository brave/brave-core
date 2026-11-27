use std::time::SystemTime;

use crate::SignedDuration;

/// Sealed trait to prevent downstream implementations.
mod sealed {
    /// A trait that cannot be implemented by downstream users.
    pub trait Sealed: Sized {}
    impl Sealed for std::time::SystemTime {}
}

/// An extension trait for [`std::time::SystemTime`] that adds methods for
/// [`time::SignedDuration`](SignedDuration)s.
pub trait SystemTimeExt: sealed::Sealed {
    /// Adds the given [`SignedDuration`] to the [`SystemTime`], returning `None` is the result
    /// cannot be represented by the underlying data structure.
    fn checked_add_signed(&self, duration: SignedDuration) -> Option<Self>;

    /// Subtracts the given [`SignedDuration`] from the [`SystemTime`], returning `None` is the
    /// result cannot be represented by the underlying data structure.
    fn checked_sub_signed(&self, duration: SignedDuration) -> Option<Self>;

    /// Returns the amount of time elapsed from another [`SystemTime`] to this one. This will be
    /// negative if `earlier` is later than `self.`
    ///
    /// If the duration cannot be stored by [`SignedDuration`], the value will be saturated to
    /// [`SignedDuration::MIN`] or [`SignedDuration::MAX`] as appropriate.
    ///
    /// # Example
    ///
    /// ```rust
    /// # use std::time::SystemTime;
    /// # use time::ext::{NumericalDuration, SystemTimeExt};
    /// let epoch = SystemTime::UNIX_EPOCH;
    /// let other = epoch + 1.seconds();
    /// assert_eq!(other.signed_duration_since(epoch), 1.seconds());
    /// assert_eq!(epoch.signed_duration_since(other), (-1).seconds());
    /// ```
    fn signed_duration_since(&self, earlier: Self) -> SignedDuration;
}

impl SystemTimeExt for SystemTime {
    #[inline]
    fn checked_add_signed(&self, duration: SignedDuration) -> Option<Self> {
        if duration.is_positive() {
            self.checked_add(duration.unsigned_abs())
        } else if duration.is_negative() {
            self.checked_sub(duration.unsigned_abs())
        } else {
            Some(*self)
        }
    }

    #[inline]
    fn checked_sub_signed(&self, duration: SignedDuration) -> Option<Self> {
        if duration.is_positive() {
            self.checked_sub(duration.unsigned_abs())
        } else if duration.is_negative() {
            self.checked_add(duration.unsigned_abs())
        } else {
            Some(*self)
        }
    }

    #[inline]
    fn signed_duration_since(&self, earlier: Self) -> SignedDuration {
        match self.duration_since(earlier) {
            Ok(duration) => duration.try_into().unwrap_or(SignedDuration::MAX),
            Err(err) => {
                let seconds = match i64::try_from(err.duration().as_secs()) {
                    Ok(seconds) => -seconds,
                    Err(_) => return SignedDuration::MIN,
                };
                let nanoseconds = -err.duration().subsec_nanos().cast_signed();

                // Safety: `nanoseconds` is guaranteed to be between -999_999_999 and 0
                // inclusive.
                unsafe { SignedDuration::new_unchecked(seconds, nanoseconds) }
            }
        }
    }
}
