use crate::{
    bounds::{self as b, RangeError},
    civil::DateTime,
    constants as c,
    macros::{rbail, rtry, unwrapr},
    tz::Offset,
};

/// An instant in time represented as the number of nanoseconds since the Unix
/// epoch.
///
/// A timestamp is always in the Unix timescale with a UTC offset of zero.
#[derive(Clone, Copy, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct Timestamp {
    second: i64,
    nanosecond: i32,
}

impl Timestamp {
    /// The minimum allow Unix timestamp.
    pub const MIN: Timestamp = Timestamp {
        second: b::UnixEpochSeconds::MIN,
        nanosecond: b::SubsecNanosecond::MIN,
    };

    /// The maximum allow Unix timestamp.
    pub const MAX: Timestamp = Timestamp {
        second: b::UnixEpochSeconds::MAX,
        nanosecond: b::SubsecNanosecond::MAX,
    };

    /// The Unix epoch represented as a timestamp.
    pub const UNIX_EPOCH: Timestamp = Timestamp { second: 0, nanosecond: 0 };

    /// Create a new timestamp from the given number of seconds and its
    /// sub-second component.
    ///
    /// This returns an error if `nanos` is not in the range specified by
    /// [`SignedSubsecNanosecond`](b::SignedSubsecNanosecond). An error
    /// is also returned when `secs` is not in the range specified by
    /// [`UnixEpochSeconds`](b::UnixEpochSeconds).
    #[inline]
    pub const fn new(secs: i64, nanos: i32) -> Result<Timestamp, RangeError> {
        let mut secs = rtry!(b::UnixEpochSeconds::checkc(secs));
        let mut nanos = rtry!(b::SignedSubsecNanosecond::checkc(nanos as i64));
        if secs == b::UnixEpochSeconds::MIN && nanos < 0 {
            rbail!(b::UnixEpochSeconds::error());
        }
        // At this point, we're done if either unit is zero or if they have the
        // same sign.
        if nanos == 0 || secs == 0 || secs.signum() == (nanos.signum() as i64)
        {
            return Ok(Timestamp::new_unchecked(secs, nanos));
        }
        // Otherwise, the only work we have to do is to balance negative nanos
        // into positive seconds, or positive nanos into negative seconds.
        if secs < 0 {
            debug_assert!(nanos > 0);
            // Never wraps because adding +1 to a negative i64 never overflows.
            //
            // MSRV(1.79): Consider using `unchecked_add` here.
            secs += 1;
            // Never wraps because subtracting +1_000_000_000 from a positive
            // i32 never overflows.
            //
            // MSRV(1.79): Consider using `unchecked_sub` here.
            nanos -= c::NANOS_PER_SEC_32;
        } else {
            debug_assert!(secs > 0);
            debug_assert!(nanos < 0);
            // Never wraps because subtracting +1 from a positive i64 never
            // overflows.
            //
            // MSRV(1.79): Consider using `unchecked_add` here.
            secs -= 1;
            // Never wraps because adding +1_000_000_000 to a negative i32
            // never overflows.
            //
            // MSRV(1.79): Consider using `unchecked_add` here.
            nanos += c::NANOS_PER_SEC_32;
        }
        Ok(Timestamp::new_unchecked(secs, nanos))
    }

    /// Creates a new `Timestamp` value in a `const` context.
    ///
    /// This is identical to [`Timestamp::new`], except that it panics when
    /// `Timestamp::new` would return an error. This can be more convenient in
    /// a `const` context where unwrapping a `Result` is not ergonomic.
    #[inline]
    pub const fn constant(second: i64, nanosecond: i32) -> Timestamp {
        unwrapr!(Timestamp::new(second, nanosecond), "invalid timestamp")
    }

    /// Creates a new `Timestamp` without bounds checks.
    ///
    /// Note that this should not be made public *and* safe.
    #[inline]
    pub(crate) const fn new_unchecked(secs: i64, nanos: i32) -> Timestamp {
        debug_assert!(b::UnixEpochSeconds::checkc(secs).is_ok());
        debug_assert!(b::SignedSubsecNanosecond::checkc(nanos as i64).is_ok());
        debug_assert!(secs != b::UnixEpochSeconds::MIN || nanos >= 0);
        debug_assert!(
            nanos == 0
                || secs == 0
                || secs.signum() == (nanos.signum() as i64)
        );
        Timestamp { second: secs, nanosecond: nanos }
    }

    /// Constructs a timestamp from seconds since the Unix epoch.
    ///
    /// This is preferred to [`Timestamp::new`] when it is known that the
    /// sub-second component is always `0`. In particular, this generates
    /// less code and is likely to be faster.
    ///
    /// An error is returned when `second` is not in the range specified by
    /// [`UnixEpochSeconds`](b::UnixEpochSeconds).
    #[inline]
    pub const fn from_second(second: i64) -> Result<Timestamp, RangeError> {
        let second = rtry!(b::UnixEpochSeconds::checkc(second));
        Ok(Timestamp::new_unchecked(second, 0))
    }

    /// Constructs a timestamp from milliseconds since the Unix epoch.
    ///
    /// An error is returned when `millisecond` is not in the range specified by
    /// [`UnixEpochMilliseconds`](b::UnixEpochMilliseconds).
    #[inline]
    pub const fn from_millisecond(
        millisecond: i64,
    ) -> Result<Timestamp, RangeError> {
        let millisecond = rtry!(b::UnixEpochMilliseconds::checkc(millisecond));
        // OK because MILLIS_PER_SEC!={-1,0}.
        let secs = millisecond / c::MILLIS_PER_SEC;
        // OK because MILLIS_PER_SEC!={-1,0} and because
        // millis % MILLIS_PER_SEC can be at most 999, and 999 * 1_000_000
        // never overflows i32.
        let nanos =
            (millisecond % c::MILLIS_PER_SEC) as i32 * c::NANOS_PER_MILLI_32;
        // OK because we've already verified that `millisecond` is in range.
        Ok(Timestamp::new_unchecked(secs, nanos))
    }

    /// Constructs a timestamp from microseconds since the Unix epoch.
    ///
    /// An error is returned when `microsecond` is not in the range specified
    /// by [`UnixEpochMicroseconds`](b::UnixEpochMicroseconds).
    #[inline]
    pub const fn from_microsecond(
        microsecond: i64,
    ) -> Result<Timestamp, RangeError> {
        let microsecond = rtry!(b::UnixEpochMicroseconds::checkc(microsecond));
        // OK because MILLIS_PER_SEC!={-1,0}.
        let secs = microsecond / c::MICROS_PER_SEC;
        // OK because MILLIS_PER_SEC!={-1,0} and because
        // millis % MILLIS_PER_SEC can be at most 999, and 999 * 1_000_000
        // never overflows i32.
        let nanos =
            (microsecond % c::MICROS_PER_SEC) as i32 * c::NANOS_PER_MICRO_32;
        // OK because we've already verified that `millisecond` is in range.
        Ok(Timestamp::new_unchecked(secs, nanos))
    }

    /// Constructs a timestamp from nanoseconds since the Unix epoch.
    ///
    /// An error is returned when `nanosecond` refers to a timestamp outside
    /// of the range [`Timestamp::MIN`] to [`Timestamp::MAX`].
    #[inline]
    pub const fn from_nanosecond(
        nanosecond: i128,
    ) -> Result<Timestamp, RangeError> {
        const NANOS_PER_SEC: i128 = c::NANOS_PER_SEC as i128;
        // OK because NANOS_PER_SEC!={-1,0}.
        let secs = nanosecond / NANOS_PER_SEC;
        // RUST: Use `i64::try_from` when available in `const`.
        if !(i64::MIN as i128 <= secs && secs <= i64::MAX as i128) {
            rbail!(b::SpecialBoundsError::UnixEpochNanoseconds);
        }
        let secs64 = rtry!(b::UnixEpochSeconds::checkc(secs as i64));
        // OK because NANOS_PER_SEC!={-1,0}.
        let nanosecond = (nanosecond % NANOS_PER_SEC) as i32;
        if secs64 == b::UnixEpochSeconds::MIN && nanosecond < 0 {
            rbail!(b::UnixEpochSeconds::error());
        }
        Ok(Timestamp::new_unchecked(secs64, nanosecond))
    }

    /// Returns this timestamp as a number of seconds since the Unix epoch.
    ///
    /// This only returns the number of whole seconds. That is, if there are
    /// any fractional seconds in this timestamp, then they are truncated.
    #[inline]
    pub const fn as_second(self) -> i64 {
        self.second
    }

    /// Returns this timestamp as a number of milliseconds since the Unix
    /// epoch.
    ///
    /// This only returns the number of whole milliseconds. That is, if there
    /// are any fractional milliseconds in this timestamp, then they are
    /// truncated.
    #[inline]
    pub const fn as_millisecond(self) -> i64 {
        // OK because the range of `Timestamp` guarantees that its
        // representation as milliseconds fits into an i64.
        let millis = self.as_second() * c::MILLIS_PER_SEC;
        // OK because subsec_millis maxes out at 999, and adding that to
        // b::UnixSeconds::MAX*1_000 will never overflow an i64.
        millis + (self.subsec_millisecond() as i64)
    }

    /// Returns this timestamp as a number of microseconds since the Unix
    /// epoch.
    ///
    /// This only returns the number of whole microseconds. That is, if there
    /// are any fractional microseconds in this timestamp, then they are
    /// truncated.
    #[inline]
    pub const fn as_microsecond(self) -> i64 {
        // OK because the range of `Timestamp` guarantees that its
        // representation as microseconds fits into an i64.
        let micros = self.as_second() * c::MICROS_PER_SEC;
        // OK because subsec_micros maxes out at 999_999, and adding that to
        // b::UnixSeconds::MAX*1_000_000 will never overflow an i64.
        micros + (self.subsec_microsecond() as i64)
    }

    /// Returns this timestamp as a number of nanoseconds since the Unix
    /// epoch.
    #[inline]
    pub const fn as_nanosecond(self) -> i128 {
        // OK because 1_000_000_000 times any i64 will never overflow i128.
        let nanos = (self.second as i128) * (c::NANOS_PER_SEC as i128);
        // OK because nanosecond maxes out at 999_999_999, and adding that to
        // i64::MAX*1_000_000_000 will never overflow a i128.
        nanos + (self.nanosecond as i128)
    }

    /// Returns the fractional second component of this timestamp in units of
    /// milliseconds.
    ///
    /// The value returned is negative when the timestamp is negative. It is
    /// guaranteed that the range of the value returned is in the inclusive
    /// range `-999..=999`.
    #[inline]
    pub const fn subsec_millisecond(&self) -> i32 {
        // OK because NANOS_PER_MILLI!={-1,0}.
        self.nanosecond / c::NANOS_PER_MILLI_32
    }

    /// Returns the fractional second component of this timestamp in units of
    /// microseconds.
    ///
    /// The value returned is negative when the timestamp is negative. It is
    /// guaranteed that the range of the value returned is in the inclusive
    /// range `-999_999..=999_999`.
    #[inline]
    pub const fn subsec_microsecond(&self) -> i32 {
        // OK because NANOS_PER_MICRO!={-1,0}.
        self.nanosecond / c::NANOS_PER_MICRO_32
    }

    /// Returns the fractional second component of this timestamp in units of
    /// nanoseconds.
    ///
    /// The value returned is negative when the timestamp is negative. It is
    /// guaranteed that the range of the value returned is in the inclusive
    /// range `-999,999,999..=999,999,999`.
    #[inline]
    pub const fn subsec_nanosecond(&self) -> i32 {
        self.nanosecond
    }

    /// Returns a number that represents the sign of this timestamp.
    ///
    /// * When [`Timestamp::is_zero`] is true, this returns `0`.
    /// * When [`Timestamp::is_positive`] is true, this returns `1`.
    /// * When [`Timestamp::is_negative`] is true, this returns `-1`.
    ///
    /// The above cases are mutually exclusive.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::Timestamp;
    ///
    /// assert_eq!(0, Timestamp::UNIX_EPOCH.signum());
    ///
    /// let ts = Timestamp::new(5, -999_999_999).unwrap();
    /// assert_eq!(ts.signum(), 1);
    /// // The mixed signs were normalized away!
    /// assert_eq!(ts.as_second(), 4);
    /// assert_eq!(ts.subsec_nanosecond(), 1);
    ///
    /// // The same applies for negative timestamps.
    /// let ts = Timestamp::new(-5, 999_999_999).unwrap();
    /// assert_eq!(ts.signum(), -1);
    /// assert_eq!(ts.as_second(), -4);
    /// assert_eq!(ts.subsec_nanosecond(), -1);
    /// ```
    #[inline]
    pub const fn signum(self) -> i8 {
        if self.is_zero() {
            0
        } else if self.is_positive() {
            1
        } else {
            debug_assert!(self.is_negative());
            -1
        }
    }

    /// Returns true if and only if this timestamp corresponds to the instant
    /// in time known as the Unix epoch.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::Timestamp;
    ///
    /// assert!(Timestamp::UNIX_EPOCH.is_zero());
    /// ```
    #[inline]
    pub const fn is_zero(self) -> bool {
        self.second == 0 && self.nanosecond == 0
    }

    /// Returns true when this timestamp is positive. That is, after the Unix
    /// epoch.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::Timestamp;
    ///
    /// let ts = Timestamp::new(0, 1).unwrap();
    /// assert!(ts.is_positive());
    /// ```
    #[inline]
    pub const fn is_positive(&self) -> bool {
        self.second.is_positive() || self.nanosecond.is_positive()
    }

    /// Returns true when this timestamp is negative. That is, before the Unix
    /// epoch.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::Timestamp;
    ///
    /// let ts = Timestamp::new(0, -1).unwrap();
    /// assert!(ts.is_negative());
    /// ```
    #[inline]
    pub const fn is_negative(&self) -> bool {
        self.second.is_negative() || self.nanosecond.is_negative()
    }

    /// Converts a Unix timestamp with an offset to a Gregorian datetime.
    ///
    /// The offset should correspond to the number of seconds required to
    /// add to this timestamp to get the local time.
    #[inline]
    pub const fn to_datetime(&self, offset: Offset) -> DateTime {
        offset.to_datetime(*self)
    }

    /// Add the given number of seconds and nanoseconds to this timestamp.
    ///
    /// If this would result in a timestamp outside of its boundaries, then
    /// this returns an error.
    ///
    /// # Examples
    ///
    /// ```
    /// use jiff_core::Timestamp;
    ///
    /// let mkts = |sec, nano| Timestamp::new(sec, nano).unwrap();
    /// let ts = mkts(123, 0);
    ///
    /// assert_eq!(ts.checked_add(1, 0), Ok(mkts(124, 0)));
    /// assert_eq!(ts.checked_add(1, 1), Ok(mkts(124, 1)));
    /// assert_eq!(ts.checked_add(1, -1), Ok(mkts(123, 999_999_999)));
    /// assert_eq!(ts.checked_add(0, 1), Ok(mkts(123, 1)));
    /// assert_eq!(ts.checked_add(0, -1), Ok(mkts(122, 999_999_999)));
    /// assert_eq!(ts.checked_add(-1, 0), Ok(mkts(122, 0)));
    /// assert_eq!(ts.checked_add(-1, 1), Ok(mkts(122, 1)));
    /// assert_eq!(ts.checked_add(-1, -1), Ok(mkts(121, 999_999_999)));
    ///
    /// assert_eq!(ts.checked_add(0, i32::MIN), Ok(mkts(121, -147_483_648)));
    /// assert_eq!(ts.checked_add(1, i32::MIN), Ok(mkts(122, -147_483_648)));
    /// assert_eq!(ts.checked_add(-1, i32::MIN), Ok(mkts(120, -147_483_648)));
    /// assert_eq!(ts.checked_add(0, i32::MAX), Ok(mkts(125, 147_483_647)));
    /// assert_eq!(ts.checked_add(1, i32::MAX), Ok(mkts(126, 147_483_647)));
    /// assert_eq!(ts.checked_add(-1, i32::MAX), Ok(mkts(124, 147_483_647)));
    ///
    /// assert!(ts.checked_add(i64::MAX, 0).is_err());
    /// assert!(ts.checked_add(i64::MIN, 0).is_err());
    ///
    /// let ts = Timestamp::UNIX_EPOCH;
    /// let max = Timestamp::MAX.as_second();
    /// assert!(ts.checked_add(max, 0).is_ok());
    /// assert!(ts.checked_add(max + 1, 0).is_err());
    /// assert!(ts.checked_add(max, 999_999_999).is_ok());
    /// assert!(ts.checked_add(max, 1_000_000_000).is_err());
    /// ```
    #[inline]
    pub const fn checked_add(
        self,
        mut seconds: i64,
        mut nanos: i32,
    ) -> Result<Timestamp, RangeError> {
        // When |nanos| exceeds 1 second, we balance the excess up to seconds.
        if !(-c::NANOS_PER_SEC_32 < nanos && nanos < c::NANOS_PER_SEC_32) {
            // Never wraps or panics because NANOS_PER_SEC!={0,-1}.
            let addsecs = nanos / c::NANOS_PER_SEC_32;
            seconds = match seconds.checked_add(addsecs as i64) {
                Some(secs) => secs,
                None => rbail!(b::UnixEpochSeconds::error()),
            };
            // Never wraps or panics because NANOS_PER_SEC!={0,-1}.
            nanos = nanos % c::NANOS_PER_SEC_32;
        }

        self.checked_add_sensible(seconds, nanos)
    }

    /// Subtracts the given number of seconds and nanoseconds from this
    /// timestamp.
    ///
    /// # Examples
    ///
    /// ```
    /// use jiff_core::Timestamp;
    ///
    /// let mkts = |sec, nano| Timestamp::new(sec, nano).unwrap();
    /// let ts = mkts(123, 0);
    ///
    /// assert_eq!(ts.checked_sub(1, 0), Ok(mkts(122, 0)));
    /// assert_eq!(ts.checked_sub(1, 1), Ok(mkts(121, 999_999_999)));
    /// assert_eq!(ts.checked_sub(1, -1), Ok(mkts(122, 1)));
    /// assert_eq!(ts.checked_sub(0, 1), Ok(mkts(122, 999_999_999)));
    /// assert_eq!(ts.checked_sub(0, -1), Ok(mkts(123, 1)));
    /// assert_eq!(ts.checked_sub(-1, 0), Ok(mkts(124, 0)));
    /// assert_eq!(ts.checked_sub(-1, 1), Ok(mkts(123, 999_999_999)));
    /// assert_eq!(ts.checked_sub(-1, -1), Ok(mkts(124, 1)));
    ///
    /// assert_eq!(ts.checked_sub(0, i32::MIN), Ok(mkts(125, 147_483_648)));
    /// assert_eq!(ts.checked_sub(1, i32::MIN), Ok(mkts(124, 147_483_648)));
    /// assert_eq!(ts.checked_sub(-1, i32::MIN), Ok(mkts(126, 147_483_648)));
    /// assert_eq!(ts.checked_sub(0, i32::MAX), Ok(mkts(121, -147_483_647)));
    /// assert_eq!(ts.checked_sub(1, i32::MAX), Ok(mkts(120, -147_483_647)));
    /// assert_eq!(ts.checked_sub(-1, i32::MAX), Ok(mkts(122, -147_483_647)));
    ///
    /// assert!(ts.checked_sub(i64::MAX, 0).is_err());
    /// assert!(ts.checked_sub(i64::MIN, 0).is_err());
    ///
    /// let ts = Timestamp::UNIX_EPOCH;
    /// let min = Timestamp::MIN.as_second();
    /// assert!(ts.checked_sub(-min, 0).is_ok());
    /// assert!(ts.checked_sub(-(min - 1), 0).is_err());
    /// assert!(ts.checked_sub(-min, -999_999_999).is_ok());
    /// assert!(ts.checked_sub(-min, 1).is_err());
    /// assert!(ts.checked_sub(-min, 999_999_999).is_err());
    /// assert!(ts.checked_sub(-min, 1_000_000_000).is_err());
    /// ```
    #[inline]
    pub const fn checked_sub(
        self,
        seconds: i64,
        mut nanos: i32,
    ) -> Result<Timestamp, RangeError> {
        let Some(mut seconds) = seconds.checked_neg() else {
            rbail!(b::UnixEpochSeconds::error())
        };

        // When |nanos| exceeds 1 second, we balance the excess up to seconds.
        if !(-c::NANOS_PER_SEC_32 < nanos && nanos < c::NANOS_PER_SEC_32) {
            // Never wraps or panics because NANOS_PER_SEC!={0,-1}.
            let addsecs = nanos / c::NANOS_PER_SEC_32;
            seconds = match seconds.checked_sub(addsecs as i64) {
                Some(secs) => secs,
                None => rbail!(b::UnixEpochSeconds::error()),
            };
            // Never wraps or panics because NANOS_PER_SEC!={0,-1}.
            nanos = nanos % c::NANOS_PER_SEC_32;
        }
        // Negating `nanos` here is OK because the above guarantees that it's
        // in the inclusive range `[-999_999_999, 999_999_999]`.
        self.checked_add(seconds, -nanos)
    }

    /// Implementation of `checked_add` that assumes `|nanos| < 1 second`.
    #[inline]
    const fn checked_add_sensible(
        self,
        seconds: i64,
        nanos: i32,
    ) -> Result<Timestamp, RangeError> {
        debug_assert!(
            -c::NANOS_PER_SEC_32 < nanos && nanos < c::NANOS_PER_SEC_32
        );

        // Delay the timestamp bounds check until after balancing nanoseconds,
        // which can bring an out-of-range second back into range.
        let Some(mut second) = self.as_second().checked_add(seconds) else {
            rbail!(b::UnixEpochSeconds::error());
        };
        // OK because we know both are in the inclusive range
        // [-999_999_999, 999_999_999] per above math.
        let mut nanosecond = self.nanosecond + nanos;
        // When the nanosecond component is zero, we can ignore it and just
        // return seconds as-is.
        if nanosecond == 0 {
            return Timestamp::from_second(second);
        }

        if nanosecond >= c::NANOS_PER_SEC_32 {
            nanosecond -= c::NANOS_PER_SEC_32;
            second = match second.checked_add(1) {
                Some(secs) => secs,
                None => rbail!(b::UnixEpochSeconds::error()),
            };
        } else if nanosecond <= -c::NANOS_PER_SEC_32 {
            nanosecond += c::NANOS_PER_SEC_32;
            second = match second.checked_sub(1) {
                Some(secs) => secs,
                None => rbail!(b::UnixEpochSeconds::error()),
            };
        }
        if second != 0
            && nanosecond != 0
            && second.signum() != (nanosecond.signum() as i64)
        {
            if second < 0 {
                debug_assert!(nanosecond > 0);
                // OK because second<0.
                second += 1;
                // OK because nanosecond>0.
                nanosecond -= c::NANOS_PER_SEC_32;
            } else {
                debug_assert!(second > 0);
                debug_assert!(nanosecond < 0);
                // OK because second>0.
                second -= 1;
                // OK because nanosecond<0.
                nanosecond += c::NANOS_PER_SEC_32;
            }
        }
        let second = rtry!(b::UnixEpochSeconds::checkc(second));
        if second == b::UnixEpochSeconds::MIN && nanosecond < 0 {
            rbail!(b::UnixEpochSeconds::error());
        }
        Ok(Timestamp::new_unchecked(second, nanosecond))
    }

    /// Add the given number of seconds to this timestamp.
    ///
    /// If this would result in a timestamp outside of its boundaries, then
    /// this returns an error.
    ///
    /// The result is normalized so that its non-zero seconds and nanoseconds
    /// have the same sign. Crossing the Unix epoch may therefore change the
    /// nanosecond component.
    ///
    /// # Examples
    ///
    /// ```
    /// use jiff_core::Timestamp;
    ///
    /// let mkts = |sec, nano| Timestamp::new(sec, nano).unwrap();
    ///
    /// let ts = mkts(123, 0);
    /// assert_eq!(ts.checked_add_seconds(0), Ok(mkts(123, 0)));
    /// assert_eq!(ts.checked_add_seconds(1), Ok(mkts(124, 0)));
    /// assert_eq!(ts.checked_add_seconds(-1), Ok(mkts(122, 0)));
    ///
    /// let ts = mkts(123, 999_999_999);
    /// assert_eq!(ts.checked_add_seconds(0), Ok(mkts(123, 999_999_999)));
    /// assert_eq!(ts.checked_add_seconds(1), Ok(mkts(124, 999_999_999)));
    /// assert_eq!(ts.checked_add_seconds(-1), Ok(mkts(122, 999_999_999)));
    ///
    /// let ts = mkts(-1, -500_000_000);
    /// assert_eq!(ts.checked_add_seconds(2), Ok(mkts(0, 500_000_000)));
    /// let ts = mkts(1, 500_000_000);
    /// assert_eq!(ts.checked_add_seconds(-2), Ok(mkts(0, -500_000_000)));
    ///
    /// assert!(ts.checked_add_seconds(i64::MIN).is_err());
    /// assert!(ts.checked_add_seconds(i64::MAX).is_err());
    /// ```
    #[inline]
    pub const fn checked_add_seconds(
        self,
        seconds: i64,
    ) -> Result<Timestamp, RangeError> {
        self.checked_add(seconds, 0)
    }

    /// Subtracts the given number of seconds from this timestamp.
    #[inline]
    pub const fn checked_sub_seconds(
        self,
        seconds: i64,
    ) -> Result<Timestamp, RangeError> {
        let Some(seconds) = seconds.checked_neg() else {
            rbail!(b::UnixEpochSeconds::error())
        };
        self.checked_add_seconds(seconds)
    }
}

impl core::fmt::Debug for Timestamp {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        let dt = self.to_datetime(Offset::UTC);
        core::fmt::Debug::fmt(&dt, f)?;
        f.write_str("Z")
    }
}

impl Default for Timestamp {
    #[inline]
    fn default() -> Timestamp {
        Timestamp::UNIX_EPOCH
    }
}

/// Adds a number of seconds to a `Timestamp`.
///
/// # Panics
///
/// When adding would result in a value outside the boundaries of a
/// `Timestamp`.
///
/// # Example
///
/// ```
/// use jiff_core::Timestamp;
///
/// let ts = Timestamp::new(123, 999_999_999).unwrap();;
/// assert_eq!(ts + 400, Timestamp::new(523, 999_999_999).unwrap());
/// ```
impl core::ops::Add<i64> for Timestamp {
    type Output = Timestamp;

    fn add(self, seconds: i64) -> Timestamp {
        self.checked_add_seconds(seconds).unwrap()
    }
}

/// Adds a number of seconds and nanoseconds to a `Timestamp`.
///
/// # Panics
///
/// When adding would result in a value outside the boundaries of a
/// `Timestamp`.
///
/// # Example
///
/// ```
/// use jiff_core::Timestamp;
///
/// let ts = Timestamp::new(123, 999_999_999).unwrap();;
/// assert_eq!(ts + (400, 1), Timestamp::new(524, 0).unwrap());
/// ```
impl core::ops::Add<(i64, i32)> for Timestamp {
    type Output = Timestamp;

    fn add(self, (seconds, nanoseconds): (i64, i32)) -> Timestamp {
        self.checked_add(seconds, nanoseconds).unwrap()
    }
}

/// Adds a number of seconds to a `Timestamp`.
///
/// # Panics
///
/// When adding would result in a value outside the boundaries of a
/// `Timestamp`.
impl core::ops::AddAssign<i64> for Timestamp {
    #[inline]
    fn add_assign(&mut self, rhs: i64) {
        *self = *self + rhs;
    }
}

/// Adds a number of seconds and nanoseconds to a `Timestamp`.
///
/// # Panics
///
/// When adding would result in a value outside the boundaries of a
/// `Timestamp`.
impl core::ops::AddAssign<(i64, i32)> for Timestamp {
    #[inline]
    fn add_assign(&mut self, rhs: (i64, i32)) {
        *self = *self + rhs;
    }
}

/// Subtracts a number of seconds from a `Timestamp`.
///
/// # Panics
///
/// When adding would result in a value outside the boundaries of a
/// `Timestamp`.
///
/// # Example
///
/// ```
/// use jiff_core::Timestamp;
///
/// let ts = Timestamp::new(523, 999_999_999).unwrap();;
/// assert_eq!(ts - 400, Timestamp::new(123, 999_999_999).unwrap());
/// ```
impl core::ops::Sub<i64> for Timestamp {
    type Output = Timestamp;

    fn sub(self, seconds: i64) -> Timestamp {
        self.checked_sub_seconds(seconds).unwrap()
    }
}

/// Subtracts a number of seconds and nanoseconds from a `Timestamp`.
///
/// # Panics
///
/// When subtracting would result in a value outside the boundaries of a
/// `Timestamp`.
///
/// # Example
///
/// ```
/// use jiff_core::Timestamp;
///
/// let ts = Timestamp::new(523, 999_999_999).unwrap();;
/// assert_eq!(ts - (400, 1), Timestamp::new(123, 999_999_998).unwrap());
/// ```
impl core::ops::Sub<(i64, i32)> for Timestamp {
    type Output = Timestamp;

    fn sub(self, (seconds, nanoseconds): (i64, i32)) -> Timestamp {
        self.checked_sub(seconds, nanoseconds).unwrap()
    }
}

/// Subtracts a number of seconds from a `Timestamp`.
///
/// # Panics
///
/// When subtracting would result in a value outside the boundaries of a
/// `Timestamp`.
impl core::ops::SubAssign<i64> for Timestamp {
    #[inline]
    fn sub_assign(&mut self, rhs: i64) {
        *self = *self - rhs;
    }
}

/// Subtracts a number of seconds and nanoseconds from a `Timestamp`.
///
/// # Panics
///
/// When subtracting would result in a value outside the boundaries of a
/// `Timestamp`.
impl core::ops::SubAssign<(i64, i32)> for Timestamp {
    #[inline]
    fn sub_assign(&mut self, rhs: (i64, i32)) {
        *self = *self - rhs;
    }
}

#[cfg(feature = "arbitrary")]
impl<'a> arbitrary::Arbitrary<'a> for Timestamp {
    fn arbitrary(
        u: &mut arbitrary::Unstructured<'a>,
    ) -> arbitrary::Result<Timestamp> {
        let secs = u.int_in_range(
            b::UnixEpochSeconds::MIN..=b::UnixEpochSeconds::MAX,
        )?;
        let mut nanos = u.int_in_range(
            b::SignedSubsecNanosecond::MIN..=b::SignedSubsecNanosecond::MAX,
        )?;
        // nanoseconds must be zero for the minimum second value,
        // so just clamp it to 0.
        if secs == b::UnixEpochSeconds::MIN && nanos < 0 {
            nanos = 0;
        }
        Ok(Timestamp::new(secs, nanos).unwrap_or(Timestamp::UNIX_EPOCH))
    }

    fn size_hint(depth: usize) -> (usize, Option<usize>) {
        arbitrary::size_hint::and(
            <i64 as arbitrary::Arbitrary>::size_hint(depth),
            <i32 as arbitrary::Arbitrary>::size_hint(depth),
        )
    }
}

#[cfg(test)]
impl quickcheck::Arbitrary for Timestamp {
    fn arbitrary(g: &mut quickcheck::Gen) -> Timestamp {
        let secs = b::UnixEpochSeconds::arbitrary(g);
        let mut nanos = b::SignedSubsecNanosecond::arbitrary(g);
        // nanoseconds must be zero for the minimum second value,
        // so just clamp it to 0.
        if secs == b::UnixEpochSeconds::MIN && nanos < 0 {
            nanos = 0;
        }
        Timestamp::new(secs, nanos).unwrap_or(Timestamp::UNIX_EPOCH)
    }

    fn shrink(&self) -> alloc::boxed::Box<dyn Iterator<Item = Self>> {
        let secs = self.as_second();
        let nanos = self.subsec_nanosecond();
        alloc::boxed::Box::new((secs, nanos).shrink().filter_map(
            |(secs, nanos)| {
                let secs = b::UnixEpochSeconds::check(secs).ok()?;
                let nanos = b::SignedSubsecNanosecond::check(nanos).ok()?;
                if secs == b::UnixEpochSeconds::MIN && nanos > 0 {
                    None
                } else {
                    Timestamp::new(secs, nanos).ok()
                }
            },
        ))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[track_caller]
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

    #[track_caller]
    fn stamp(second: i64, subsec: i32) -> Timestamp {
        Timestamp::new(second, subsec).unwrap()
    }

    #[track_caller]
    fn offset(second: i32) -> Offset {
        Offset::from_seconds(second).unwrap()
    }

    // Regression test for: https://github.com/BurntSushi/jiff/pull/695
    #[test]
    fn sub_assign() {
        for (rhs, expected) in [(400, 600), (-400, 1400), (0, 1000)] {
            let mut ts = stamp(1000, 0);
            ts -= rhs;
            assert_eq!(ts, stamp(expected, 0));
        }

        for (rhs, expected) in [
            ((400, 1), stamp(599, 999_999_999)),
            ((-400, -1), stamp(1400, 1)),
            ((0, 0), stamp(1000, 0)),
        ] {
            let mut ts = stamp(1000, 0);
            ts -= rhs;
            assert_eq!(ts, expected);
        }
    }

    #[test]
    fn new_ok() {
        let ts = stamp(0, 0);
        assert_eq!(ts, Timestamp::UNIX_EPOCH);

        let ts = stamp(0, 123_000_000);
        assert_eq!(ts.as_second(), 0);
        assert_eq!(ts.subsec_nanosecond(), 123_000_000);

        let ts = stamp(0, -123_000_000);
        assert_eq!(ts.as_second(), 0);
        assert_eq!(ts.subsec_nanosecond(), -123_000_000);

        let ts = stamp(1, 0);
        assert_eq!(ts.as_second(), 1);
        assert_eq!(ts.subsec_nanosecond(), 0);

        let ts = stamp(-1, 0);
        assert_eq!(ts.as_second(), -1);
        assert_eq!(ts.subsec_nanosecond(), 0);

        let ts = stamp(1, 123_000_000);
        assert_eq!(ts.as_second(), 1);
        assert_eq!(ts.subsec_nanosecond(), 123_000_000);

        let ts = stamp(-1, -123_000_000);
        assert_eq!(ts.as_second(), -1);
        assert_eq!(ts.subsec_nanosecond(), -123_000_000);

        let ts = stamp(1, -123_000_000);
        assert_eq!(ts.as_second(), 0);
        assert_eq!(ts.subsec_nanosecond(), 877_000_000);

        let ts = stamp(-1, 123_000_000);
        assert_eq!(ts.as_second(), 0);
        assert_eq!(ts.subsec_nanosecond(), -877_000_000);

        let ts = stamp(-377705023201, 0);
        assert_eq!(ts, Timestamp::MIN);

        let ts = stamp(253402207200, 999_999_999);
        assert_eq!(ts, Timestamp::MAX);
    }

    #[test]
    fn new_err() {
        assert!(Timestamp::new(0, 1_000_000_000).is_err());
        assert!(Timestamp::new(0, -1_000_000_000).is_err());
        assert!(Timestamp::new(1, 1_000_000_000).is_err());
        assert!(Timestamp::new(1, -1_000_000_000).is_err());
        assert!(Timestamp::new(-1, 1_000_000_000).is_err());
        assert!(Timestamp::new(-1, -1_000_000_000).is_err());
        assert!(Timestamp::new(0, i32::MAX).is_err());
        assert!(Timestamp::new(0, i32::MIN).is_err());
        assert!(Timestamp::new(-377705023201, -1).is_err());
        assert!(Timestamp::new(253402207201, 0).is_err());
    }

    // Regression test for #645: `Timestamp::from_nanosecond` panicked
    // instead of returning an error when the given nanosecond count fell
    // just outside of `Timestamp::MIN..=Timestamp::MAX`, even though its
    // signature (and docs) promise a `Result`.
    #[test]
    fn from_nanosecond_err() {
        let min = Timestamp::MIN.as_nanosecond();
        assert_eq!(Timestamp::from_nanosecond(min).unwrap(), Timestamp::MIN);
        assert!(Timestamp::from_nanosecond(min - 1).is_err());

        let max = Timestamp::MAX.as_nanosecond();
        assert_eq!(Timestamp::from_nanosecond(max).unwrap(), Timestamp::MAX);
        assert!(Timestamp::from_nanosecond(max + 1).is_err());

        assert!(Timestamp::from_nanosecond(i128::MIN).is_err());
        assert!(Timestamp::from_nanosecond(i128::MAX).is_err());
    }

    // Regression tests for #696.
    #[test]
    fn checked_arithmetic_below_min() {
        assert!(Timestamp::MIN.checked_add(0, -1).is_err());
        assert!(Timestamp::MIN.checked_sub(0, 1).is_err());
    }

    #[test]
    fn checked_arithmetic_overflow() {
        for ts in [Timestamp::MIN, Timestamp::UNIX_EPOCH, Timestamp::MAX] {
            for seconds in [i64::MIN, i64::MIN + 1, i64::MAX - 1, i64::MAX] {
                for nanos in [i32::MIN, -1, 0, 1, i32::MAX] {
                    assert!(ts.checked_add(seconds, nanos).is_err());
                    assert!(ts.checked_sub(seconds, nanos).is_err());
                }
            }
        }
    }

    #[test]
    fn checked_arithmetic_at_max() {
        let ts = stamp(1, 0);
        let seconds = Timestamp::MAX.as_second();
        assert_eq!(ts.checked_add(seconds, -1), Ok(Timestamp::MAX));
        assert_eq!(ts.checked_sub(-seconds, 1), Ok(Timestamp::MAX));
        assert_eq!(ts + (seconds, -1), Timestamp::MAX);
        assert_eq!(ts - (-seconds, 1), Timestamp::MAX);
    }

    // Regression tests for #703.
    #[test]
    fn checked_seconds_across_epoch() {
        for (ts, seconds, expected) in [
            (stamp(-1, -500_000_000), 2, stamp(0, 500_000_000)),
            (stamp(1, 500_000_000), -2, stamp(0, -500_000_000)),
            (stamp(-1, -1), 3, stamp(1, 999_999_999)),
            (stamp(1, 1), -3, stamp(-1, -999_999_999)),
            (stamp(0, -1), 1, stamp(0, 999_999_999)),
            (stamp(0, 1), -1, stamp(0, -999_999_999)),
        ] {
            assert_eq!(ts.checked_add_seconds(seconds), Ok(expected));
            assert_eq!(ts.checked_sub_seconds(-seconds), Ok(expected));
            assert_eq!(ts + seconds, expected);
            assert_eq!(ts - -seconds, expected);
        }
    }

    #[test]
    fn checked_arithmetic_boundaries() {
        let min = Timestamp::MIN.as_second();
        let max = Timestamp::MAX.as_second();
        let timestamps = [
            Timestamp::MIN,
            stamp(min + 1, -999_999_999),
            stamp(-1, -999_999_999),
            stamp(-1, 0),
            stamp(0, -1),
            Timestamp::UNIX_EPOCH,
            stamp(0, 1),
            stamp(1, 0),
            stamp(1, 999_999_999),
            stamp(max, 999_999_998),
            Timestamp::MAX,
        ];
        let seconds = [
            i64::MIN,
            min - max - 1,
            min - max,
            min - 1,
            min,
            min + 1,
            -1,
            0,
            1,
            max - 1,
            max,
            max + 1,
            max - min,
            max - min + 1,
            i64::MAX,
        ];
        let nanos = [
            i32::MIN,
            -1_000_000_000,
            -999_999_999,
            -1,
            0,
            1,
            999_999_999,
            1_000_000_000,
            i32::MAX,
        ];
        for ts in timestamps {
            for seconds in seconds {
                let delta = (seconds as i128) * (c::NANOS_PER_SEC as i128);
                assert_eq!(
                    ts.checked_add_seconds(seconds).ok(),
                    Timestamp::from_nanosecond(ts.as_nanosecond() + delta)
                        .ok(),
                    "{ts:?} + {seconds}",
                );
                assert_eq!(
                    ts.checked_sub_seconds(seconds).ok(),
                    Timestamp::from_nanosecond(ts.as_nanosecond() - delta)
                        .ok(),
                    "{ts:?} - {seconds}",
                );
                for nanos in nanos {
                    let delta = delta + (nanos as i128);
                    let sum = ts.as_nanosecond() + delta;
                    let difference = ts.as_nanosecond() - delta;
                    assert_eq!(
                        ts.checked_add(seconds, nanos).ok(),
                        Timestamp::from_nanosecond(sum).ok(),
                        "{ts:?} + ({seconds}, {nanos})",
                    );
                    assert_eq!(
                        ts.checked_sub(seconds, nanos).ok(),
                        Timestamp::from_nanosecond(difference).ok(),
                        "{ts:?} - ({seconds}, {nanos})",
                    );
                }
            }
        }
    }

    #[test]
    fn to_datetime_no_subsec() {
        let dt = datetime(1970, 1, 1, 0, 0, 0, 0);
        assert_eq!(stamp(0, 0).to_datetime(offset(0)), dt);
        assert_eq!(stamp(-3600, 0).to_datetime(offset(3600)), dt);
        assert_eq!(stamp(3600, 0).to_datetime(offset(-3600)), dt);

        let dt = datetime(1969, 12, 31, 23, 30, 0, 0);
        assert_eq!(stamp(-1800, 0).to_datetime(offset(0)), dt);
        assert_eq!(stamp(-5400, 0).to_datetime(offset(3600)), dt);
        assert_eq!(stamp(1800, 0).to_datetime(offset(-3600)), dt);

        let dt = datetime(1970, 1, 1, 0, 30, 0, 0);
        assert_eq!(stamp(1800, 0).to_datetime(offset(0)), dt);
        assert_eq!(stamp(-1800, 0).to_datetime(offset(3600)), dt);
        assert_eq!(stamp(5400, 0).to_datetime(offset(-3600)), dt);
    }

    #[test]
    fn to_datetime_with_subsec() {
        let dt = datetime(1970, 1, 1, 0, 0, 0, 123);
        assert_eq!(stamp(0, 123).to_datetime(offset(0)), dt);
        assert_eq!(stamp(-3599, -999_999_877).to_datetime(offset(3600)), dt);
        assert_eq!(stamp(3600, 123).to_datetime(offset(-3600)), dt);

        let dt = datetime(1969, 12, 31, 23, 30, 0, 123);
        assert_eq!(stamp(-1799, -999_999_877).to_datetime(offset(0)), dt);
        assert_eq!(stamp(-5399, -999_999_877).to_datetime(offset(3600)), dt);
        assert_eq!(stamp(1800, 123).to_datetime(offset(-3600)), dt);

        let dt = datetime(1970, 1, 1, 0, 30, 0, 123);
        assert_eq!(stamp(1800, 123).to_datetime(offset(0)), dt);
        assert_eq!(stamp(-1799, -999_999_877).to_datetime(offset(3600)), dt);
        assert_eq!(stamp(5400, 123).to_datetime(offset(-3600)), dt);
    }

    #[test]
    fn to_datetime_limits() {
        assert_eq!(Timestamp::MIN.to_datetime(Offset::MIN), DateTime::MIN);
        assert_eq!(Timestamp::MAX.to_datetime(Offset::MAX), DateTime::MAX);
    }

    quickcheck::quickcheck! {
        fn prop_timestamp_datetime_roundtrip(
            ts: Timestamp,
            offset: Offset
        ) -> bool {
            let dt = ts.to_datetime(offset);
            let got = dt.to_timestamp(offset).unwrap();
            got == ts
        }
    }
}
