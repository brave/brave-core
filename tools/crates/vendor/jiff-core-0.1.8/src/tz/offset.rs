use crate::{
    bounds::{self as b, RangeError},
    civil::{self, DateTime},
    constants as c,
    macros::{rtry, unwrapr},
    Timestamp,
};

/// A fixed offset, in seconds, from UTC.
#[derive(Clone, Copy, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub struct Offset {
    seconds: i32,
}

impl Offset {
    /// The minimum possible offset from UTC.
    pub const MIN: Offset = Offset { seconds: b::OffsetTotalSeconds::MIN };

    /// The maximum possible offset from UTC.
    pub const MAX: Offset = Offset { seconds: b::OffsetTotalSeconds::MAX };

    /// The UTC offset.
    pub const UTC: Offset = Offset { seconds: 0 };

    /// The zero offset.
    pub const ZERO: Offset = Offset { seconds: 0 };

    /// Creates a new time zone offset in a `const` context from a given number
    /// of hours.
    #[inline]
    pub const fn constant(hours: i8) -> Offset {
        unwrapr!(Offset::from_hours(hours), "invalid time zone offset hours")
    }

    /// Creates a new time zone offset in a `const` context from a given number
    /// of seconds.
    #[inline]
    pub const fn constant_seconds(seconds: i32) -> Offset {
        unwrapr!(
            Offset::from_seconds(seconds),
            "invalid time zone offset seconds",
        )
    }

    /// Creates a new time zone offset from a given number of hours.
    ///
    /// Negative offsets correspond to time zones west of the prime meridian,
    /// while positive offsets correspond to time zones east of the prime
    /// meridian. Equivalently, in all cases, `civil-time - offset = UTC`.
    #[inline]
    pub const fn from_hours(hours: i8) -> Result<Offset, RangeError> {
        Offset::from_seconds(hours as i32 * c::SECS_PER_HOUR_32)
    }

    /// Returns a new time zone offset from UTC given its representation in
    /// seconds.
    ///
    /// An error is also returned when `seconds` is not in the range specified
    /// by [`OffsetTotalSeconds`](b::OffsetTotalSeconds).
    #[inline]
    pub const fn from_seconds(seconds: i32) -> Result<Offset, RangeError> {
        let seconds = rtry!(b::OffsetTotalSeconds::checkc(seconds as i64));
        Ok(Offset { seconds })
    }

    /// Returns the seconds value corresponding to this time zone offset.
    #[inline]
    pub const fn seconds(self) -> i32 {
        self.seconds
    }

    /// Returns the negation of this offset.
    ///
    /// A negative offset will become positive and vice versa. This is a no-op
    /// if the offset is zero.
    ///
    /// This never panics.
    #[inline]
    pub const fn negate(self) -> Offset {
        // OK because of the boundaries we enforce. `seconds` can never be
        // `i32::MIN`.
        Offset { seconds: -self.seconds() }
    }

    /// Returns the "sign number" or "signum" of this offset.
    ///
    /// The number returned is `-1` when this offset is negative,
    /// `0` when this offset is zero and `1` when this span is positive.
    #[inline]
    pub const fn signum(self) -> i8 {
        self.seconds().signum() as i8
    }

    /// Returns true if and only if this offset is positive.
    ///
    /// This returns false when the offset is zero or negative.
    #[inline]
    pub const fn is_positive(self) -> bool {
        self.seconds() > 0
    }

    /// Returns true if and only if this offset is less than zero.
    ///
    /// This returns false when the offset is zero or positive.
    #[inline]
    pub const fn is_negative(self) -> bool {
        self.seconds() < 0
    }

    /// Returns true if and only if this offset is zero.
    ///
    /// Or equivalently, when this offset corresponds to [`Offset::UTC`].
    #[inline]
    pub const fn is_zero(self) -> bool {
        self.seconds() == 0
    }

    /// Adds the given number of seconds to this offset.
    ///
    /// If the resulting offset would be outside the an offset's boundaries,
    /// an error is returned.
    #[inline]
    pub const fn checked_add(
        self,
        seconds: i32,
    ) -> Result<Offset, RangeError> {
        let seconds =
            rtry!(b::OffsetTotalSeconds::checked_add(self.seconds(), seconds));
        Ok(Offset { seconds })
    }

    /// Subtracts the given number of seconds from this offset.
    ///
    /// If the resulting offset would be outside the an offset's boundaries,
    /// an error is returned.
    #[inline]
    pub const fn checked_sub(
        self,
        seconds: i32,
    ) -> Result<Offset, RangeError> {
        let seconds = rtry!(b::OffsetTotalSeconds::checkc(
            self.seconds() as i64 - seconds as i64,
        ));
        Ok(Offset { seconds })
    }

    /// Returns the number of seconds from this offset to `other`.
    #[inline]
    pub const fn until(self, other: Offset) -> i32 {
        other.seconds() - self.seconds()
    }

    /// Returns the number of seconds since this offset from `other`.
    #[inline]
    pub const fn since(self, other: Offset) -> i32 {
        self.seconds() - other.seconds()
    }

    /// Converts a Unix timestamp with an offset to a Gregorian datetime.
    ///
    /// The offset should correspond to the number of seconds required to
    /// add to this timestamp to get the local time.
    #[inline]
    pub const fn to_datetime(self, timestamp: Timestamp) -> civil::DateTime {
        let offset = self;
        let second = timestamp.as_second();
        let mut nanosecond = timestamp.subsec_nanosecond();

        // Shift second comfortably into the postive domain
        // so that division and remainder can use unsigned math
        // which is much faster.
        // 30 * 400 years: 12,000 yr range > [-9,999..1970]
        // (146097 being the number of days per 400 years).
        const DAY_SHIFT: i32 = 30 * 146097;
        const SEC_SHIFT: i64 = (DAY_SHIFT as i64) * 86_400;

        let mut pos_sec =
            (second + (offset.seconds() as i64) + SEC_SHIFT) as u64;

        let is_neg_nano: bool = nanosecond < 0;
        pos_sec -= is_neg_nano as u64;
        nanosecond += if is_neg_nano { 1_000_000_000 } else { 0 };

        let mut epoch_day = (pos_sec / 86_400) as i32;
        let second = (pos_sec % 86_400) as i32;

        epoch_day -= DAY_SHIFT;

        // We should check whether having unchecked APIs
        // would be beneficial here. In particular, the
        // math above, coupled with the ranges allowed on
        // `Timestamp` and `Offset` (by design) guarantee
        // that our resulting datetime will always be in
        // range.
        let date = unwrapr!(
            civil::UnixEpochDay::new(epoch_day),
            "always valid Unix epoch day",
        )
        .to_date();
        let time = unwrapr!(
            unwrapr!(
                civil::TimeSecond::new(second),
                "always valid civil second time"
            )
            .to_time()
            .with_subsec_nanosecond(nanosecond),
            "always valid civil subsecond"
        );
        civil::DateTime::from_parts(date, time)
    }

    /// Converts the given civil datetime to a timestamp using this offset.
    ///
    /// # Errors
    ///
    /// This returns an error if this would have returned a timestamp outside
    /// of its minimum and maximum values.
    #[inline]
    pub const fn to_timestamp(
        self,
        dt: civil::DateTime,
    ) -> Result<Timestamp, RangeError> {
        let offset = self;
        let epoch_day = dt.date().to_unix_epoch_day().day();
        let mut second = (epoch_day as i64) * c::SECS_PER_CIVIL_DAY
            + (dt.time().to_second().second() as i64);
        let mut nanosecond = dt.time().subsec_nanosecond();
        second -= offset.seconds() as i64;
        if second < 0 && nanosecond != 0 {
            second += 1;
            nanosecond -= c::NANOS_PER_SEC_32;
        }
        let second = rtry!(b::UnixEpochSeconds::checkc(second));
        Ok(Timestamp::new_unchecked(second, nanosecond))
    }
}

impl Offset {
    #[inline]
    fn part_hours(self) -> i8 {
        (self.seconds() / c::SECS_PER_HOUR_32) as i8
    }

    #[inline]
    fn part_minutes(self) -> i8 {
        ((self.seconds() / c::SECS_PER_MIN_32) % c::MINS_PER_HOUR_32) as i8
    }

    #[inline]
    fn part_seconds(self) -> i8 {
        (self.seconds() % c::SECS_PER_MIN_32) as i8
    }
}

/// Negate this offset.
///
/// A positive offset becomes negative and vice versa. This is a no-op for the
/// zero offset.
///
/// This never panics.
impl core::ops::Neg for Offset {
    type Output = Offset;

    #[inline]
    fn neg(self) -> Offset {
        self.negate()
    }
}

/// Adds a number of seconds to an `Offset`.
///
/// # Panics
///
/// When adding would result in a value outside the boundaries of a
/// `Offset`.
impl core::ops::Add<i32> for Offset {
    type Output = Offset;

    fn add(self, seconds: i32) -> Offset {
        self.checked_add(seconds).unwrap()
    }
}

/// Adds a number of seconds into an `Offset`.
///
/// # Panics
///
/// When adding would result in a value outside the boundaries of a
/// `Offset`.
impl core::ops::AddAssign<i32> for Offset {
    #[inline]
    fn add_assign(&mut self, rhs: i32) {
        *self = *self + rhs;
    }
}

/// Subtracts a number of seconds from an `Offset`.
///
/// # Panics
///
/// When adding would result in a value outside the boundaries of a
/// `Offset`.
impl core::ops::Sub<i32> for Offset {
    type Output = Offset;

    fn sub(self, seconds: i32) -> Offset {
        self.checked_sub(seconds).unwrap()
    }
}

/// Subtracts a number of seconds from an `Offset` in place.
///
/// # Panics
///
/// When adding would result in a value outside the boundaries of a
/// `Offset`.
impl core::ops::SubAssign<i32> for Offset {
    #[inline]
    fn sub_assign(&mut self, rhs: i32) {
        *self = *self - rhs;
    }
}

impl core::fmt::Debug for Offset {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        let sign = if self.is_negative() { "-" } else { "" };
        write!(
            f,
            "{sign}{:02}:{:02}:{:02}",
            self.part_hours().unsigned_abs(),
            self.part_minutes().unsigned_abs(),
            self.part_seconds().unsigned_abs(),
        )
    }
}

impl core::fmt::Display for Offset {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        let sign = if self.is_negative() { "-" } else { "+" };
        let hours = self.part_hours().unsigned_abs();
        let minutes = self.part_minutes().unsigned_abs();
        let seconds = self.part_seconds().unsigned_abs();
        if hours == 0 && minutes == 0 && seconds == 0 {
            f.write_str("+00")
        } else if hours != 0 && minutes == 0 && seconds == 0 {
            write!(f, "{sign}{hours:02}")
        } else if minutes != 0 && seconds == 0 {
            write!(f, "{sign}{hours:02}:{minutes:02}")
        } else {
            write!(f, "{sign}{hours:02}:{minutes:02}:{seconds:02}")
        }
    }
}

#[cfg(feature = "defmt")]
impl defmt::Format for Offset {
    fn format(&self, f: defmt::Formatter) {
        let sign = if self.is_negative() { "-" } else { "" };
        defmt::write!(
            f,
            "{=str}{=u8:02}:{=u8:02}:{=u8:02}",
            sign,
            self.part_hours().unsigned_abs(),
            self.part_minutes().unsigned_abs(),
            self.part_seconds().unsigned_abs(),
        )
    }
}

#[cfg(feature = "arbitrary")]
impl<'a> arbitrary::Arbitrary<'a> for Offset {
    fn arbitrary(
        u: &mut arbitrary::Unstructured<'a>,
    ) -> arbitrary::Result<Offset> {
        let secs = u.int_in_range(
            b::OffsetTotalSeconds::MIN..=b::OffsetTotalSeconds::MAX,
        )?;
        Ok(Offset::from_seconds(secs).unwrap_or(Offset::UTC))
    }

    fn size_hint(depth: usize) -> (usize, Option<usize>) {
        <i32 as arbitrary::Arbitrary>::size_hint(depth)
    }
}

#[cfg(test)]
impl quickcheck::Arbitrary for Offset {
    fn arbitrary(g: &mut quickcheck::Gen) -> Offset {
        let secs = b::OffsetTotalSeconds::arbitrary(g);
        Offset::from_seconds(secs).unwrap_or(Offset::UTC)
    }

    fn shrink(&self) -> alloc::boxed::Box<dyn Iterator<Item = Self>> {
        let secs = self.seconds();
        alloc::boxed::Box::new(secs.shrink().filter_map(|secs| {
            let secs = b::OffsetTotalSeconds::check(secs).ok()?;
            Offset::from_seconds(secs).ok()
        }))
    }
}

/// A possibly ambiguous [`Offset`].
///
/// One of three possibilities encoded by this type occurs when converting a
/// civil datetime into a specific instant in time. In rare cases, the civil
/// datetime can fall into a gap or a fold, in which case, one of two offsets
/// could be applicable. Or, perhaps, neither. Callers must decide how best to
/// handle these cases.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub enum AmbiguousOffset {
    /// The offset for a particular civil datetime and time zone is
    /// unambiguous.
    ///
    /// This is the overwhelmingly common case. In general, the only time this
    /// case does not occur is when there is a transition to a different time
    /// zone (rare) or to/from daylight saving time (occurs for 1 hour twice
    /// in year in many geographic locations).
    Unambiguous {
        /// The offset from UTC for the corresponding civil datetime given. The
        /// offset is determined via the relevant time zone data, and in this
        /// case, there is only one possible offset that could be applied to
        /// the given civil datetime.
        offset: Offset,
    },
    /// The offset for a particular civil datetime and time zone is ambiguous
    /// because there is a gap.
    ///
    /// This most commonly occurs when a civil datetime corresponds to an hour
    /// that was "skipped" in a jump to DST (daylight saving time).
    Gap {
        /// The offset corresponding to the time before a gap.
        ///
        /// For example, given a time zone of `America/Los_Angeles`, the offset
        /// for time immediately preceding `2020-03-08T02:00:00` is `-08`.
        before: Offset,
        /// The offset corresponding to the later time in a gap.
        ///
        /// For example, given a time zone of `America/Los_Angeles`, the offset
        /// for time immediately following `2020-03-08T02:59:59` is `-07`.
        after: Offset,
    },
    /// The offset for a particular civil datetime and time zone is ambiguous
    /// because there is a fold.
    ///
    /// This most commonly occurs when a civil datetime corresponds to an hour
    /// that was "repeated" in a jump to standard time from DST (daylight
    /// saving time).
    Fold {
        /// The offset corresponding to the earlier time in a fold.
        ///
        /// For example, given a time zone of `America/Los_Angeles`, the offset
        /// for time on the first `2020-11-01T01:00:00` is `-07`.
        before: Offset,
        /// The offset corresponding to the earlier time in a fold.
        ///
        /// For example, given a time zone of `America/Los_Angeles`, the offset
        /// for time on the second `2020-11-01T01:00:00` is `-08`.
        after: Offset,
    },
}

impl AmbiguousOffset {
    #[inline]
    pub(crate) const fn into_ambiguous_timestamp(
        self,
        dt: DateTime,
    ) -> AmbiguousTimestamp {
        AmbiguousTimestamp { dt, offset: self }
    }
}

/// A possibly ambiguous [`Timestamp`].
///
/// While this is called an ambiguous _timestamp_, the thing that is
/// actually ambiguous is the offset. That is, an ambiguous timestamp is
/// actually a pair of a [`civil::DateTime`](crate::civil::DateTime) and an
/// [`AmbiguousOffset`].
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct AmbiguousTimestamp {
    dt: DateTime,
    offset: AmbiguousOffset,
}

impl AmbiguousTimestamp {
    /// Returns the civil datetime that was used to create this ambiguous
    /// timestamp.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::{civil::date, tz::posix};
    ///
    /// let tz = posix::TimeZone::parse("EST5EDT,M3.2.0,M11.1.0").unwrap();
    /// let dt = date(2024, 7, 10).at(17, 15, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert_eq!(ts.datetime(), dt);
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub const fn datetime(&self) -> DateTime {
        self.dt
    }

    /// Returns the possibly ambiguous offset that is the ultimate source of
    /// ambiguity.
    ///
    /// Most civil datetimes are not ambiguous, and thus, the offset will not
    /// be ambiguous either. In this case, the offset returned will be the
    /// [`AmbiguousOffset::Unambiguous`] variant.
    ///
    /// But, not all civil datetimes are unambiguous. There are exactly two
    /// cases where a civil datetime can be ambiguous: when a civil datetime
    /// does not exist (a gap) or when a civil datetime is repeated (a fold).
    /// In both such cases, the _offset_ is the thing that is ambiguous as
    /// there are two possible choices for the offset in both cases: the offset
    /// before the transition (whether it's a gap or a fold) or the offset
    /// after the transition.
    ///
    /// This type captures the fact that computing an offset from a civil
    /// datetime in a particular time zone is in one of three possible states:
    ///
    /// 1. It is unambiguous.
    /// 2. It is ambiguous because there is a gap in time.
    /// 3. It is ambiguous because there is a fold in time.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::{civil::date, tz::{self, posix, AmbiguousOffset}};
    ///
    /// let tz = posix::TimeZone::parse("EST5EDT,M3.2.0,M11.1.0").unwrap();
    ///
    /// // Not ambiguous.
    /// let dt = date(2024, 7, 15).at(17, 30, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert_eq!(ts.offset(), AmbiguousOffset::Unambiguous {
    ///     offset: tz::offset(-4),
    /// });
    ///
    /// // Ambiguous because of a gap.
    /// let dt = date(2024, 3, 10).at(2, 30, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert_eq!(ts.offset(), AmbiguousOffset::Gap {
    ///     before: tz::offset(-5),
    ///     after: tz::offset(-4),
    /// });
    ///
    /// // Ambiguous because of a fold.
    /// let dt = date(2024, 11, 3).at(1, 30, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert_eq!(ts.offset(), AmbiguousOffset::Fold {
    ///     before: tz::offset(-4),
    ///     after: tz::offset(-5),
    /// });
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub const fn offset(&self) -> AmbiguousOffset {
        self.offset
    }

    /// Returns true if and only if this possibly ambiguous timestamp is
    /// actually ambiguous.
    ///
    /// This occurs precisely in cases when the offset is _not_
    /// [`AmbiguousOffset::Unambiguous`].
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::{civil::date, tz::posix};
    ///
    /// let tz = posix::TimeZone::parse("EST5EDT,M3.2.0,M11.1.0").unwrap();
    ///
    /// // Not ambiguous.
    /// let dt = date(2024, 7, 15).at(17, 30, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert!(!ts.is_ambiguous());
    ///
    /// // Ambiguous because of a gap.
    /// let dt = date(2024, 3, 10).at(2, 30, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert!(ts.is_ambiguous());
    ///
    /// // Ambiguous because of a fold.
    /// let dt = date(2024, 11, 3).at(1, 30, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert!(ts.is_ambiguous());
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub const fn is_ambiguous(&self) -> bool {
        !matches!(self.offset(), AmbiguousOffset::Unambiguous { .. })
    }

    /// Disambiguates this timestamp according to the "compatible" strategy.
    ///
    /// If this timestamp is unambiguous, then this is a no-op.
    ///
    /// The "compatible" strategy selects the offset corresponding to the civil
    /// time after a gap, and the offset corresponding to the civil time before
    /// a fold. This is what is specified in [RFC 5545].
    ///
    /// [RFC 5545]: https://datatracker.ietf.org/doc/html/rfc5545
    ///
    /// # Errors
    ///
    /// This returns an error when the combination of the civil datetime
    /// and offset would lead to a `Timestamp` outside of the
    /// [`Timestamp::MIN`] and [`Timestamp::MAX`] limits. This only occurs
    /// when the civil datetime is "close" to its own [`DateTime::MIN`]
    /// and [`DateTime::MAX`] limits.
    #[inline]
    pub const fn compatible(self) -> Result<Timestamp, RangeError> {
        let offset = match self.offset() {
            AmbiguousOffset::Unambiguous { offset } => offset,
            AmbiguousOffset::Gap { before, .. } => before,
            AmbiguousOffset::Fold { before, .. } => before,
        };
        offset.to_timestamp(self.dt)
    }

    /// Disambiguates this timestamp according to the "earlier" strategy.
    ///
    /// If this timestamp is unambiguous, then this is a no-op.
    ///
    /// The "earlier" strategy selects the offset corresponding to the civil
    /// time before a gap, and the offset corresponding to the civil time
    /// before a fold.
    ///
    /// # Errors
    ///
    /// This returns an error when the combination of the civil datetime
    /// and offset would lead to a `Timestamp` outside of the
    /// [`Timestamp::MIN`] and [`Timestamp::MAX`] limits. This only occurs
    /// when the civil datetime is "close" to its own [`DateTime::MIN`]
    /// and [`DateTime::MAX`] limits.
    #[inline]
    pub const fn earlier(self) -> Result<Timestamp, RangeError> {
        let offset = match self.offset() {
            AmbiguousOffset::Unambiguous { offset } => offset,
            AmbiguousOffset::Gap { after, .. } => after,
            AmbiguousOffset::Fold { before, .. } => before,
        };
        offset.to_timestamp(self.dt)
    }

    /// Disambiguates this timestamp according to the "later" strategy.
    ///
    /// If this timestamp is unambiguous, then this is a no-op.
    ///
    /// The "later" strategy selects the offset corresponding to the civil
    /// time after a gap, and the offset corresponding to the civil time
    /// after a fold.
    ///
    /// # Errors
    ///
    /// This returns an error when the combination of the civil datetime
    /// and offset would lead to a `Timestamp` outside of the
    /// [`Timestamp::MIN`] and [`Timestamp::MAX`] limits. This only occurs
    /// when the civil datetime is "close" to its own [`DateTime::MIN`]
    /// and [`DateTime::MAX`] limits.
    #[inline]
    pub const fn later(self) -> Result<Timestamp, RangeError> {
        let offset = match self.offset() {
            AmbiguousOffset::Unambiguous { offset } => offset,
            AmbiguousOffset::Gap { before, .. } => before,
            AmbiguousOffset::Fold { after, .. } => after,
        };
        offset.to_timestamp(self.dt)
    }

    /// Disambiguates this timestamp according to the "reject" strategy.
    ///
    /// If this timestamp is unambiguous, then this is a no-op.
    ///
    /// The "reject" strategy always returns an error when the timestamp
    /// is ambiguous.
    ///
    /// # Errors
    ///
    /// This returns an error when the combination of the civil datetime
    /// and offset would lead to a `Timestamp` outside of the
    /// [`Timestamp::MIN`] and [`Timestamp::MAX`] limits. This only occurs
    /// when the civil datetime is "close" to its own [`DateTime::MIN`]
    /// and [`DateTime::MAX`] limits.
    ///
    /// This also returns an error when the timestamp is ambiguous.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff_core::{civil::date, tz::{posix, Offset}};
    ///
    /// let tz = posix::TimeZone::parse("EST5EDT,M3.2.0,M11.1.0").unwrap();
    ///
    /// // Not ambiguous.
    /// let dt = date(2024, 7, 15).at(17, 30, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert_eq!(
    ///     ts.later().unwrap().to_datetime(Offset::UTC),
    ///     date(2024, 7, 15).at(21, 30, 0, 0),
    /// );
    ///
    /// // Ambiguous because of a gap.
    /// let dt = date(2024, 3, 10).at(2, 30, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert!(ts.unambiguous().is_err());
    ///
    /// // Ambiguous because of a fold.
    /// let dt = date(2024, 11, 3).at(1, 30, 0, 0);
    /// let ts = tz.to_ambiguous_timestamp(dt);
    /// assert!(ts.unambiguous().is_err());
    /// ```
    #[inline]
    pub const fn unambiguous(self) -> Result<Timestamp, AmbiguousError> {
        let offset = match self.offset() {
            AmbiguousOffset::Unambiguous { offset } => offset,
            AmbiguousOffset::Gap { before, after } => {
                return Err(AmbiguousError {
                    kind: AmbiguousErrorKind::BecauseGap { before, after },
                });
            }
            AmbiguousOffset::Fold { before, after } => {
                return Err(AmbiguousError {
                    kind: AmbiguousErrorKind::BecauseFold { before, after },
                });
            }
        };
        match offset.to_timestamp(self.dt) {
            Ok(timestamp) => Ok(timestamp),
            Err(range_error) => Err(AmbiguousError {
                kind: AmbiguousErrorKind::Range(range_error),
            }),
        }
    }
}

/// An error that occurs when an unmabiguous civil datetime is demanded.
///
/// This surfaces via the [`AmbiguousTimestamp::unambiguous`] API.
#[derive(Clone, Debug)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct AmbiguousError {
    kind: AmbiguousErrorKind,
}

#[derive(Clone, Debug)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
enum AmbiguousErrorKind {
    Range(RangeError),
    BecauseFold { before: Offset, after: Offset },
    BecauseGap { before: Offset, after: Offset },
}

impl core::fmt::Display for AmbiguousError {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        use self::AmbiguousErrorKind::*;

        match self.kind {
            Range(ref err) => core::fmt::Display::fmt(err, f),
            BecauseFold { before, after } => write!(
                f,
                "datetime is ambiguous since it falls into a \
                 fold between offsets {before} and {after}",
            ),
            BecauseGap { before, after } => write!(
                f,
                "datetime is ambiguous since it falls into a \
                 gap between offsets {before} and {after}",
            ),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for AmbiguousError {}

#[cfg(test)]
mod tests {
    use super::Offset;

    #[test]
    fn checked_subtracts_positive_and_negative_seconds() {
        assert_eq!(
            Offset::UTC.checked_sub(1).unwrap(),
            Offset::constant_seconds(-1),
        );
        assert_eq!(
            Offset::UTC.checked_sub(-1).unwrap(),
            Offset::constant_seconds(1),
        );
    }

    #[test]
    fn checked_sub_rejects_overflow() {
        assert!(Offset::MIN.checked_sub(1).is_err());
        assert!(Offset::MAX.checked_sub(-1).is_err());
        assert!(Offset::UTC.checked_sub(i32::MIN).is_err());
    }

    #[test]
    fn subtraction_operators_subtract_seconds() {
        assert_eq!(Offset::UTC - 1, Offset::constant_seconds(-1));

        let mut offset = Offset::UTC;
        offset -= 1;
        assert_eq!(offset, Offset::constant_seconds(-1));
    }
}
