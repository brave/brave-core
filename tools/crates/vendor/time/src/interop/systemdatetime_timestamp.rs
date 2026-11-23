use core::cmp::Ordering;
use core::ops::Sub;
use std::time::SystemTime;

#[allow(unused_imports)] // MSRV of 1.87
use num_conv::prelude::*;

use crate::ext::SystemTimeExt;
use crate::unit::*;
use crate::{SignedDuration, Timestamp};

impl Sub<SystemTime> for Timestamp {
    type Output = SignedDuration;

    #[inline]
    fn sub(self, rhs: SystemTime) -> Self::Output {
        let lhs_duration = self - Self::UNIX_EPOCH;
        match rhs.duration_since(SystemTime::UNIX_EPOCH) {
            Ok(duration) => lhs_duration - duration,
            Err(err) => lhs_duration + err.duration(),
        }
    }
}

impl Sub<Timestamp> for SystemTime {
    type Output = SignedDuration;

    #[inline]
    fn sub(self, rhs: Timestamp) -> Self::Output {
        let rhs_duration = rhs - Timestamp::UNIX_EPOCH;

        match self.duration_since(Self::UNIX_EPOCH) {
            Ok(lhs) => lhs - rhs_duration,
            Err(err) => -(err.duration() + rhs_duration),
        }
    }
}

impl PartialEq<SystemTime> for Timestamp {
    #[inline]
    fn eq(&self, other: &SystemTime) -> bool {
        *self - Self::UNIX_EPOCH == other.signed_duration_since(SystemTime::UNIX_EPOCH)
    }
}

impl PartialEq<Timestamp> for SystemTime {
    #[inline]
    fn eq(&self, other: &Timestamp) -> bool {
        other == self
    }
}

impl PartialOrd<SystemTime> for Timestamp {
    #[inline]
    fn partial_cmp(&self, other: &SystemTime) -> Option<Ordering> {
        (*self - Self::UNIX_EPOCH).partial_cmp(&other.signed_duration_since(SystemTime::UNIX_EPOCH))
    }
}

impl PartialOrd<Timestamp> for SystemTime {
    #[inline]
    fn partial_cmp(&self, other: &Timestamp) -> Option<Ordering> {
        other.partial_cmp(self).map(Ordering::reverse)
    }
}

impl From<SystemTime> for Timestamp {
    /// # Panics
    ///
    /// This may panic if an overflow occurs.
    #[inline]
    #[track_caller]
    fn from(datetime: SystemTime) -> Self {
        let duration = datetime.signed_duration_since(SystemTime::UNIX_EPOCH);
        let mut seconds = duration.whole_seconds();
        let mut nanoseconds = duration.subsec_nanoseconds();

        if nanoseconds < 0 {
            seconds -= 1;
            nanoseconds += Nanosecond::per_t::<i32>(Second);
        }

        Self::new(seconds, nanoseconds.cast_unsigned())
            .expect("SystemTime value is out of valid range")
    }
}

impl From<Timestamp> for SystemTime {
    /// # Panics
    ///
    /// This may panic if an overflow occurs.
    #[inline]
    #[track_caller]
    fn from(timestamp: Timestamp) -> Self {
        Self::UNIX_EPOCH + (timestamp - Timestamp::UNIX_EPOCH)
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use crate::ext::NumericalDuration;

    #[test]
    fn systime_to_timestamp() -> crate::Result<()> {
        let ts = Timestamp::from(SystemTime::UNIX_EPOCH + 5.seconds());
        assert_eq!(ts, Timestamp::from_seconds(5)?);

        let ts = Timestamp::from(SystemTime::UNIX_EPOCH - 5.seconds());
        assert_eq!(ts, Timestamp::from_seconds(-5)?);

        let ts = Timestamp::from(SystemTime::UNIX_EPOCH + 5.seconds() + 1.milliseconds());
        assert_eq!(ts, Timestamp::from_milliseconds(5_001)?);

        let ts = Timestamp::from(SystemTime::UNIX_EPOCH - 5.seconds() - 1.milliseconds());
        assert_eq!(ts, Timestamp::from_milliseconds(-5_001)?);

        Ok(())
    }
}
