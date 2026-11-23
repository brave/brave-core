use core::cmp::Ordering;
use core::ops::Sub;

#[allow(unused_imports)] // MSRV of 1.87
use num_conv::prelude::*;

use crate::{OffsetDateTime, SignedDuration, Timestamp};

impl Sub<OffsetDateTime> for Timestamp {
    type Output = SignedDuration;

    #[inline]
    fn sub(self, rhs: OffsetDateTime) -> Self::Output {
        SignedDuration::new(
            self.as_seconds() - rhs.unix_timestamp(),
            self.nanosecond().cast_signed() - rhs.nanosecond().cast_signed(),
        )
    }
}

impl Sub<Timestamp> for OffsetDateTime {
    type Output = SignedDuration;

    #[inline]
    fn sub(self, rhs: Timestamp) -> Self::Output {
        SignedDuration::new(
            self.unix_timestamp() - rhs.as_seconds(),
            self.nanosecond().cast_signed() - rhs.nanosecond().cast_signed(),
        )
    }
}

impl PartialEq<OffsetDateTime> for Timestamp {
    #[expect(clippy::suspicious_operation_groupings, reason = "false positive")]
    #[inline]
    fn eq(&self, other: &OffsetDateTime) -> bool {
        self.as_seconds() == other.unix_timestamp() && self.nanosecond() == other.nanosecond()
    }
}

impl PartialEq<Timestamp> for OffsetDateTime {
    #[inline]
    fn eq(&self, other: &Timestamp) -> bool {
        other == self
    }
}

impl PartialOrd<OffsetDateTime> for Timestamp {
    #[inline]
    fn partial_cmp(&self, other: &OffsetDateTime) -> Option<Ordering> {
        (self.as_seconds(), self.nanosecond())
            .partial_cmp(&(other.unix_timestamp(), other.nanosecond()))
    }
}

impl PartialOrd<Timestamp> for OffsetDateTime {
    #[inline]
    fn partial_cmp(&self, other: &Timestamp) -> Option<Ordering> {
        other.partial_cmp(self).map(Ordering::reverse)
    }
}

impl From<OffsetDateTime> for Timestamp {
    /// # Panics
    ///
    /// This may panic if an overflow occurs.
    #[inline]
    #[track_caller]
    fn from(datetime: OffsetDateTime) -> Self {
        // Safety: Nanoseconds have the same range.
        unsafe {
            Self::from_seconds(datetime.unix_timestamp())
                .expect("local datetime out of valid range for `Timestamp`")
                .replace_nanosecond(datetime.nanosecond())
                .unwrap_unchecked()
        }
    }
}

impl From<Timestamp> for OffsetDateTime {
    #[inline]
    fn from(timestamp: Timestamp) -> Self {
        // Safety: The valid range of `Timestamp` is less than that of `OffsetDateTime` (due to edge
        // cases of `OffsetDateTime`). Nanoseconds have the same range.
        unsafe {
            Self::from_unix_timestamp(timestamp.as_seconds())
                .unwrap_unchecked()
                .replace_nanosecond(timestamp.nanosecond())
                .unwrap_unchecked()
        }
    }
}
