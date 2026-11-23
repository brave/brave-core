use crate::{
    bounds::RangeError,
    civil::{Date, Time},
    macros::{ctry, unwrapr},
    tz::Offset,
    Timestamp,
};

/// A civil time of a day on a particular Gregorian date.
#[derive(Clone, Copy, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct DateTime {
    date: Date,
    time: Time,
}

impl DateTime {
    /// The minimum allowed Gregorian date and clock time.
    pub const MIN: DateTime = DateTime { date: Date::MIN, time: Time::MIN };

    /// The maximum allowed Gregorian date and clock time.
    pub const MAX: DateTime = DateTime { date: Date::MAX, time: Time::MAX };

    /// Creates a new civil datetime from its constituent components.
    ///
    /// If any of the values are out of their supported ranges, then an
    /// error is returned. Additionally, if `year`, `month` and `day` do not
    /// correspond to a valid Gregorian date, then an error is returned.
    #[inline]
    pub const fn new(
        year: i16,
        month: i8,
        day: i8,
        hour: i8,
        minute: i8,
        second: i8,
        subsec_nanosecond: i32,
    ) -> Result<DateTime, RangeError> {
        let date = ctry!(Date::new(year, month, day));
        let time = ctry!(Time::new(hour, minute, second, subsec_nanosecond));
        Ok(DateTime::from_parts(date, time))
    }

    /// Creates a new `DateTime` from its [`Date`] and [`Time`] components.
    #[inline]
    pub const fn from_parts(date: Date, time: Time) -> DateTime {
        DateTime { date, time }
    }

    /// Returns the Gregorian date component of this datetime.
    #[inline]
    pub const fn date(&self) -> Date {
        self.date
    }

    /// Returns the civil time component of this datetime.
    #[inline]
    pub const fn time(&self) -> Time {
        self.time
    }

    /// Adds the given number of seconds to this civil datetime.
    ///
    /// This returns an error when the resulting datetime would exceed either
    /// [`DateTime::MIN`] or [`DateTime::MAX`].
    #[inline]
    pub const fn checked_add_seconds(
        &self,
        seconds: i32,
    ) -> Result<DateTime, RangeError> {
        let (second, added_days) =
            ctry!(self.time().to_second().overflowing_add(seconds));
        let date = ctry!(self.date().checked_add(added_days));
        let time = unwrapr!(
            second
                .to_time()
                .with_subsec_nanosecond(self.time().subsec_nanosecond()),
            "subsec we started from hasn't change and must be valid",
        );
        Ok(DateTime::from_parts(date, time))
    }

    /// Like `DateTime::checked_add_seconds`, but arithmetic saturates to
    /// either [`DateTime::MIN`] (when `seconds < 0`) or [`DateTime::MAX`]
    /// (when `seconds > 0`).
    #[inline]
    pub const fn saturating_add_seconds(&self, seconds: i32) -> DateTime {
        match self.checked_add_seconds(seconds) {
            Ok(dt) => dt,
            Err(_) => {
                if seconds < 0 {
                    DateTime::MIN
                } else {
                    DateTime::MAX
                }
            }
        }
    }

    /// Converts this datetime, along with its offset from UTC, to a
    /// corresponding Unix timestamp.
    ///
    /// Note that unlike the reverse operation, [`Timestamp::to_datetime`],
    /// this is fallible. This is by design. Namely, by making this routine
    /// fallible at the boundaries, it permits the reverse operation to be
    /// infallible. That is, all instants can be converted to a civil datetime
    /// (appropriate for formatting), but not all civil datetimes combined with
    /// all UTC offsets can be converted to an instant in time.
    ///
    /// This errors when the timestamp returned would be outside the range
    /// given by [`Timestamp::MIN`] and [`Timestamp::MAX`].
    #[inline]
    pub const fn to_timestamp(
        &self,
        offset: Offset,
    ) -> Result<Timestamp, RangeError> {
        offset.to_timestamp(*self)
    }
}

impl core::fmt::Debug for DateTime {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{:?}T{:?}", self.date(), self.time())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn datetime(
        year: i16,
        month: i8,
        day: i8,
        hour: i8,
        minute: i8,
        second: i8,
        subsec_nanosecond: i32,
    ) -> DateTime {
        DateTime::new(
            year,
            month,
            day,
            hour,
            minute,
            second,
            subsec_nanosecond,
        )
        .unwrap()
    }

    fn stamp(second: i64, subsec: i32) -> Timestamp {
        Timestamp::new(second, subsec).unwrap()
    }

    fn offset(second: i32) -> Offset {
        Offset::from_seconds(second).unwrap()
    }

    #[test]
    fn checked_add_seconds() {
        let dt = datetime(2026, 2, 25, 0, 0, 0, 0);
        assert_eq!(
            dt.checked_add_seconds(1),
            Ok(datetime(2026, 2, 25, 0, 0, 1, 0))
        );
        assert_eq!(
            dt.checked_add_seconds(86_399),
            Ok(datetime(2026, 2, 25, 23, 59, 59, 0))
        );
        assert_eq!(
            dt.checked_add_seconds(86_400),
            Ok(datetime(2026, 2, 26, 0, 0, 0, 0))
        );
        assert_eq!(
            dt.checked_add_seconds(-1),
            Ok(datetime(2026, 2, 24, 23, 59, 59, 0))
        );
        assert_eq!(
            dt.checked_add_seconds(-86_399),
            Ok(datetime(2026, 2, 24, 0, 0, 1, 0))
        );
        assert_eq!(
            dt.checked_add_seconds(-86_400),
            Ok(datetime(2026, 2, 24, 0, 0, 0, 0))
        );

        let dt = datetime(2026, 2, 25, 0, 0, 0, 1);
        assert_eq!(
            dt.checked_add_seconds(1),
            Ok(datetime(2026, 2, 25, 0, 0, 1, 1))
        );
        assert_eq!(
            dt.checked_add_seconds(86_399),
            Ok(datetime(2026, 2, 25, 23, 59, 59, 1))
        );
        assert_eq!(
            dt.checked_add_seconds(86_400),
            Ok(datetime(2026, 2, 26, 0, 0, 0, 1))
        );
        assert_eq!(
            dt.checked_add_seconds(-1),
            Ok(datetime(2026, 2, 24, 23, 59, 59, 1))
        );
        assert_eq!(
            dt.checked_add_seconds(-86_399),
            Ok(datetime(2026, 2, 24, 0, 0, 1, 1))
        );
        assert_eq!(
            dt.checked_add_seconds(-86_400),
            Ok(datetime(2026, 2, 24, 0, 0, 0, 1))
        );
    }

    #[test]
    fn to_timestamp_no_subsec() {
        let dt = datetime(1970, 1, 1, 0, 0, 0, 0);
        assert_eq!(dt.to_timestamp(offset(0)), Ok(stamp(0, 0)));
        assert_eq!(dt.to_timestamp(offset(3600)), Ok(stamp(-3600, 0)));
        assert_eq!(dt.to_timestamp(offset(-3600)), Ok(stamp(3600, 0)));

        let dt = datetime(1969, 12, 31, 23, 30, 0, 0);
        assert_eq!(dt.to_timestamp(offset(0)), Ok(stamp(-1800, 0)));
        assert_eq!(dt.to_timestamp(offset(3600)), Ok(stamp(-5400, 0)));
        assert_eq!(dt.to_timestamp(offset(-3600)), Ok(stamp(1800, 0)));

        let dt = datetime(1970, 1, 1, 0, 30, 0, 0);
        assert_eq!(dt.to_timestamp(offset(0)), Ok(stamp(1800, 0)));
        assert_eq!(dt.to_timestamp(offset(3600)), Ok(stamp(-1800, 0)));
        assert_eq!(dt.to_timestamp(offset(-3600)), Ok(stamp(5400, 0)));
    }

    #[test]
    fn to_timestamp_with_subsec() {
        let dt = datetime(1970, 1, 1, 0, 0, 0, 123);
        assert_eq!(dt.to_timestamp(offset(0)), Ok(stamp(0, 123)));
        assert_eq!(
            dt.to_timestamp(offset(3600)),
            Ok(stamp(-3599, -999_999_877))
        );
        assert_eq!(dt.to_timestamp(offset(-3600)), Ok(stamp(3600, 123)));

        let dt = datetime(1969, 12, 31, 23, 30, 0, 123);
        assert_eq!(dt.to_timestamp(offset(0)), Ok(stamp(-1799, -999_999_877)));
        assert_eq!(
            dt.to_timestamp(offset(3600)),
            Ok(stamp(-5399, -999_999_877))
        );
        assert_eq!(dt.to_timestamp(offset(-3600)), Ok(stamp(1800, 123)));

        let dt = datetime(1970, 1, 1, 0, 30, 0, 123);
        assert_eq!(dt.to_timestamp(offset(0)), Ok(stamp(1800, 123)));
        assert_eq!(
            dt.to_timestamp(offset(3600)),
            Ok(stamp(-1799, -999_999_877))
        );
        assert_eq!(dt.to_timestamp(offset(-3600)), Ok(stamp(5400, 123)));
    }

    #[test]
    fn to_timestamp_err() {
        let dt = datetime(-9999, 1, 1, 0, 0, 0, 0);
        assert_eq!(dt.to_timestamp(Offset::MIN), Ok(Timestamp::MIN));
        assert!(dt.to_timestamp(offset(Offset::MIN.seconds() + 1)).is_err());

        let dt = datetime(9999, 12, 31, 23, 59, 59, 999_999_999);
        assert_eq!(dt.to_timestamp(Offset::MAX), Ok(Timestamp::MAX));
        assert!(dt.to_timestamp(offset(Offset::MAX.seconds() - 1)).is_err());

        let dt = datetime(9999, 12, 31, 23, 59, 59, 0);
        assert!(dt.to_timestamp(offset(Offset::MAX.seconds() - 1)).is_err());
    }
}
