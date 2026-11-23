use core::cmp::Ordering;
use core::ops::Sub;

use crate::{SignedDuration, Timestamp, UtcDateTime};

impl Sub<UtcDateTime> for Timestamp {
    type Output = SignedDuration;

    #[inline]
    fn sub(self, rhs: UtcDateTime) -> Self::Output {
        SignedDuration::new(
            self.as_seconds() - rhs.unix_timestamp(),
            self.nanosecond().cast_signed() - rhs.nanosecond().cast_signed(),
        )
    }
}

impl Sub<Timestamp> for UtcDateTime {
    type Output = SignedDuration;

    #[inline]
    fn sub(self, rhs: Timestamp) -> Self::Output {
        SignedDuration::new(
            self.unix_timestamp() - rhs.as_seconds(),
            self.nanosecond().cast_signed() - rhs.nanosecond().cast_signed(),
        )
    }
}

impl PartialEq<UtcDateTime> for Timestamp {
    #[expect(clippy::suspicious_operation_groupings, reason = "false positive")]
    #[inline]
    fn eq(&self, other: &UtcDateTime) -> bool {
        self.as_seconds() == other.unix_timestamp() && self.nanosecond() == other.nanosecond()
    }
}

impl PartialEq<Timestamp> for UtcDateTime {
    #[inline]
    fn eq(&self, other: &Timestamp) -> bool {
        other == self
    }
}

impl PartialOrd<UtcDateTime> for Timestamp {
    #[inline]
    fn partial_cmp(&self, other: &UtcDateTime) -> Option<Ordering> {
        (self.as_seconds(), self.nanosecond())
            .partial_cmp(&(other.unix_timestamp(), other.nanosecond()))
    }
}

impl PartialOrd<Timestamp> for UtcDateTime {
    #[inline]
    fn partial_cmp(&self, other: &Timestamp) -> Option<Ordering> {
        other.partial_cmp(self).map(Ordering::reverse)
    }
}

impl From<UtcDateTime> for Timestamp {
    #[inline]
    fn from(datetime: UtcDateTime) -> Self {
        // Safety: The valid range of `Timestamp` and `UtcDateTime` are the same. Nanoseconds also
        // have the same range.
        unsafe {
            Self::from_seconds(datetime.unix_timestamp())
                .unwrap_unchecked()
                .replace_nanosecond(datetime.nanosecond())
                .unwrap_unchecked()
        }
    }
}

impl From<Timestamp> for UtcDateTime {
    #[inline]
    fn from(timestamp: Timestamp) -> Self {
        // Safety: The valid range of `Timestamp` and `UtcDateTime` are the same. Nanoseconds also
        // have the same range.
        unsafe {
            Self::from_unix_timestamp(timestamp.as_seconds())
                .unwrap_unchecked()
                .replace_nanosecond(timestamp.nanosecond())
                .unwrap_unchecked()
        }
    }
}
