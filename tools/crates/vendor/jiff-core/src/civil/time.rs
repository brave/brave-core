use crate::{
    bounds::{self as b, RangeError},
    civil::{self, DateTime},
    constants as c,
    macros::{rbail, rtry, unwrapr},
};

/// The civil time of day.
///
/// This time's representation uses nanosecond precision. The full range of
/// clock values are `00:00:00.000000000` to `23:59:59.999999999` inclusive.
#[derive(Clone, Copy, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct Time {
    hour: i8,
    minute: i8,
    second: i8,
    subsec_nanosecond: i32,
}

impl Time {
    /// The minimum allowed civil time.
    ///
    /// This corresponds to midnight.
    pub const MIN: Time =
        Time { hour: 0, minute: 0, second: 0, subsec_nanosecond: 0 };

    /// The maximum allowed civil time.
    ///
    /// This corresponds to the last nanosecond in a civil day.
    pub const MAX: Time = Time {
        hour: 23,
        minute: 59,
        second: 59,
        subsec_nanosecond: 999_999_999,
    };

    /// Creates a new civil time from its constituent components.
    ///
    /// If any of the values are out of their supported ranges, then an error
    /// is returned.
    #[inline]
    pub const fn new(
        hour: i8,
        minute: i8,
        second: i8,
        subsec_nanosecond: i32,
    ) -> Result<Time, RangeError> {
        let hour = rtry!(b::Hour::checkc(hour as i64));
        let minute = rtry!(b::Minute::checkc(minute as i64));
        let second = rtry!(b::Second::checkc(second as i64));
        let subsec_nanosecond =
            rtry!(b::SubsecNanosecond::checkc(subsec_nanosecond as i64));
        Ok(Time { hour, minute, second, subsec_nanosecond })
    }

    /// Returns the hour component of this civil time.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`Hour`](crate::bounds::Hour).
    #[inline]
    pub const fn hour(self) -> i8 {
        self.hour
    }

    /// Returns the minute component of this civil time.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`Minute`](crate::bounds::Minute).
    #[inline]
    pub const fn minute(self) -> i8 {
        self.minute
    }

    /// Returns the second component of this civil time.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`Second`](crate::bounds::Second).
    #[inline]
    pub const fn second(self) -> i8 {
        self.second
    }

    /// Returns the "millisecond" component of this time.
    ///
    /// The value returned is guaranteed to be in the range `0..=999`.
    #[inline]
    pub const fn millisecond(self) -> i16 {
        (self.subsec_nanosecond() as u32 / c::NANOS_PER_MILLI_32 as u32) as i16
    }

    /// Returns the "microsecond" component of this time.
    ///
    /// The value returned is guaranteed to be in the range `0..=999`.
    #[inline]
    pub const fn microsecond(self) -> i16 {
        ((self.subsec_nanosecond() as u32 / c::NANOS_PER_MICRO_32 as u32)
            % c::MICROS_PER_MILLI_32 as u32) as i16
    }

    /// Returns the "nanosecond" component of this time.
    ///
    /// The value returned is guaranteed to be in the range `0..=999`.
    #[inline]
    pub const fn nanosecond(self) -> i16 {
        (self.subsec_nanosecond() as u32 % c::NANOS_PER_MICRO_32 as u32) as i16
    }

    /// Returns the fractional second (to nanosecond precision) component of
    /// this civil time.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`SubsecNanosecond`](crate::bounds::SubsecNanosecond).
    #[inline]
    pub const fn subsec_nanosecond(self) -> i32 {
        self.subsec_nanosecond
    }

    /// Returns this time with its subsecond component replaced with the three
    /// given subsecond components.
    ///
    /// If any of the given components are out of range (`0..=999`), then an
    /// error is returned.
    #[inline]
    pub const fn with_subsec_parts(
        self,
        millisecond: i16,
        microsecond: i16,
        nanosecond: i16,
    ) -> Result<Time, RangeError> {
        let millisecond = rtry!(b::Millisecond::checkc(millisecond as i64));
        let microsecond = rtry!(b::Microsecond::checkc(microsecond as i64));
        let nanosecond = rtry!(b::Nanosecond::checkc(nanosecond as i64));
        let subsec_nanosecond = (millisecond as i32 * c::NANOS_PER_MILLI_32)
            + (microsecond as i32 * c::NANOS_PER_MICRO_32)
            + (nanosecond as i32);
        Ok(Time { subsec_nanosecond, ..self })
    }

    /// Returns this time with its subsecond component replaced with the
    /// nanosecond component given.
    ///
    /// If the number of nanoseconds is out of range (`0..=999_999_999`), then
    /// an error is returned.
    #[inline]
    pub const fn with_subsec_nanosecond(
        self,
        subsec_nanosecond: i32,
    ) -> Result<Time, RangeError> {
        let subsec_nanosecond =
            rtry!(b::SubsecNanosecond::checkc(subsec_nanosecond as i64));
        Ok(Time { subsec_nanosecond, ..self })
    }

    /// Converts this civil time to a second value corresponding to the number
    /// of seconds that has elapsed since midnight until this time. If this
    /// time is midnight, then the second value returned is `0`.
    ///
    /// Note that this drops any subsecond component on this civil time.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`CivilDaySecond`](crate::bounds::CivilDaySecond).
    #[inline]
    pub const fn to_second(self) -> TimeSecond {
        let mut second: i32 = 0;
        second += (self.hour() as i32) * 3600;
        second += (self.minute() as i32) * 60;
        second += self.second() as i32;
        TimeSecond { second }
    }

    /// Converts this civil time to a nanosecond value corresponding to the
    /// number of nanoseconds that has elapsed since midnight until this time.
    /// If this time is midnight, then the nanosecond value returned is `0`.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`CivilDayNanosecond`](crate::bounds::CivilDayNanosecond).
    #[inline]
    pub const fn to_nanosecond(self) -> TimeNanosecond {
        let mut nanosecond: i64 = 0;
        nanosecond += (self.hour() as i64) * 3_600_000_000_000;
        nanosecond += (self.minute() as i64) * 60_000_000_000;
        nanosecond += (self.second() as i64) * 1_000_000_000;
        nanosecond += self.subsec_nanosecond() as i64;
        TimeNanosecond { nanosecond }
    }

    /// A convenience function for constructing a [`DateTime`] from this time
    /// on the date given by its components.
    ///
    /// # Panics
    ///
    /// This routine panics when [`Date::new`](crate::civil::Date) with
    /// the given inputs would return an error. That is, when the given
    /// year-month-day does not correspond to a valid date. Namely, all of the
    /// following must be true:
    ///
    /// * The year must be in the range `-9999..=9999`.
    /// * The month must be in the range `1..=12`.
    /// * The day must be at least `1` and must be at most the number of days
    /// in the corresponding month. So for example, `2024-02-29` is valid but
    /// `2023-02-29` is not.
    ///
    /// Similarly, when used in a const context, invalid parameters will
    /// prevent your Rust program from compiling.
    #[inline]
    pub const fn on(self, year: i16, month: i8, day: i8) -> DateTime {
        DateTime::from_parts(civil::date(year, month, day), self)
    }
}

impl core::fmt::Debug for Time {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(
            f,
            "{:02}:{:02}:{:02}",
            self.hour(),
            self.minute(),
            self.second()
        )?;
        let mut subsec = self.subsec_nanosecond();
        if subsec == 0 {
            return Ok(());
        }

        // This is really annoying. But we don't have Jiff's formatting
        // facilities to handle this for us. We should also support precision
        // settings from `Formatter`.
        let mut buf = [b'0'; 9];
        for i in (0..9).rev() {
            buf[i] += (subsec % 10) as u8;
            subsec /= 10;
        }
        let mut end = 9;
        while end > 0 && buf[usize::from(end) - 1] == b'0' {
            end -= 1;
        }
        // OK because `buf` only ever contains ASCII digits.
        let fractional_digits = core::str::from_utf8(&buf[..end]).unwrap();
        write!(f, ".{fractional_digits}")
    }
}

#[cfg(feature = "arbitrary")]
impl<'a> arbitrary::Arbitrary<'a> for Time {
    fn arbitrary(
        u: &mut arbitrary::Unstructured<'a>,
    ) -> arbitrary::Result<Time> {
        let hour = u.int_in_range(b::Hour::MIN..=b::Hour::MAX)?;
        let minute = u.int_in_range(b::Minute::MIN..=b::Minute::MAX)?;
        let second = u.int_in_range(b::Second::MIN..=b::Second::MAX)?;
        let subsec_nanosecond = u.int_in_range(
            b::SubsecNanosecond::MIN..=b::SubsecNanosecond::MAX,
        )?;
        Ok(Time::new(hour, minute, second, subsec_nanosecond).unwrap())
    }

    fn size_hint(depth: usize) -> (usize, Option<usize>) {
        arbitrary::size_hint::and_all(&[
            <i8 as arbitrary::Arbitrary>::size_hint(depth),
            <i8 as arbitrary::Arbitrary>::size_hint(depth),
            <i8 as arbitrary::Arbitrary>::size_hint(depth),
            <i32 as arbitrary::Arbitrary>::size_hint(depth),
        ])
    }
}

#[cfg(test)]
impl quickcheck::Arbitrary for Time {
    fn arbitrary(g: &mut quickcheck::Gen) -> Time {
        let hour = b::Hour::arbitrary(g);
        let minute = b::Minute::arbitrary(g);
        let second = b::Second::arbitrary(g);
        let subsec_nanosecond = b::SubsecNanosecond::arbitrary(g);
        Time::new(hour, minute, second, subsec_nanosecond).unwrap()
    }

    fn shrink(&self) -> alloc::boxed::Box<dyn Iterator<Item = Time>> {
        alloc::boxed::Box::new(
            (
                self.hour(),
                self.minute(),
                self.second(),
                self.subsec_nanosecond(),
            )
                .shrink()
                .filter_map(
                    |(hour, minute, second, subsec_nanosecond)| {
                        Time::new(hour, minute, second, subsec_nanosecond).ok()
                    },
                ),
        )
    }
}

/// Represents a single point in a civil day, to second precision.
#[derive(Clone, Copy, Debug, Eq, PartialEq, PartialOrd, Ord)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct TimeSecond {
    second: i32,
}

impl TimeSecond {
    /// Creates a new civil time from a given second value.
    ///
    /// The value must correspond to the number of seconds elapsed since
    /// the start of a civil day. It cannot exceed the length of a civil day
    /// (in seconds).
    #[inline]
    pub const fn new(second: i32) -> Result<TimeSecond, RangeError> {
        let second = rtry!(b::CivilDaySecond::checkc(second as i64));
        Ok(TimeSecond { second })
    }

    /// Creates a new civil time from a given second value.
    ///
    /// This panics when `second` exceeds the maximum number of seconds in a
    /// single civil day.
    #[inline]
    pub const fn constant(second: i32) -> TimeSecond {
        unwrapr!(TimeSecond::new(second), "invalid civil day second")
    }

    /// Returns the second value.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`CivilDaySecond`](crate::bounds::CivilDaySecond).
    #[inline]
    pub const fn second(self) -> i32 {
        self.second
    }

    /// Adds the given number of seconds to this civil time and returns the
    /// resulting civil time with any overflowing amount in units of civil
    /// days.
    ///
    /// This returns an error when integer overflow occurs. For example, when
    /// `seconds` is `i32::MAX` and this civil time is any time other than
    /// midnight.
    ///
    /// Note that the number of days returned may exceed the range supported
    /// by [`UnixEpochDay`](crate::civil::UnixEpochDay). Moreover, the number
    /// of days returned may be negative. This occurs only when `seconds` is
    /// negative enough to result in the time wrapping around at `0`.
    #[inline]
    pub const fn overflowing_add(
        self,
        seconds: i32,
    ) -> Result<(TimeSecond, i32), RangeError> {
        let Some(sum) = self.second().checked_add(seconds) else {
            rbail!(b::CivilDaySecond::error());
        };
        let days = sum.div_euclid(c::SECS_PER_CIVIL_DAY_32);
        let rem = sum.rem_euclid(c::SECS_PER_CIVIL_DAY_32);
        Ok((TimeSecond { second: rem }, days))
    }

    /// Converts this second representation of a civil time into the
    /// components of a civil time.
    ///
    /// The subsecond component on the `Time` returned is always `0`.
    #[inline]
    pub const fn to_time(&self) -> Time {
        // Uses a non-obvious sequence to allow component extraction
        // to have overlapping execution.
        //
        // Calculation of `minute` and `second` use an optimised
        // "power-complement-padding" technique instead of standard mod.
        // Relies on the identity: 60 + 2^2 = 2^6.
        //
        // Ref: https://www.benjoffe.com/fast-time-of-day

        // cast to unsigned for faster division:
        let secs: u32 = self.second as u32;
        let hour: u32 = (secs / 3600) as u32;
        let tmin: u32 = (secs / 60) as u32;

        Time {
            hour: hour as i8,
            minute: ((tmin + 4 * hour) % 64) as i8,
            second: ((secs + 4 * tmin) % 64) as i8,
            subsec_nanosecond: 0,
        }
    }
}

/// Represents a single point in a civil day, to nanosecond precision.
#[derive(Clone, Copy, Debug, Eq, PartialEq, PartialOrd, Ord)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub struct TimeNanosecond {
    nanosecond: i64,
}

impl TimeNanosecond {
    /// Creates a new civil time from a given nanosecond value.
    ///
    /// The value must correspond to the number of nanoseconds elapsed since
    /// the start of a civil day. It cannot exceed the length of a civil day
    /// (in nanoseconds).
    #[inline]
    pub const fn new(nanosecond: i64) -> Result<TimeNanosecond, RangeError> {
        let nanosecond =
            rtry!(b::CivilDayNanosecond::checkc(nanosecond as i64));
        Ok(TimeNanosecond { nanosecond })
    }

    /// Returns the nanosecond value.
    ///
    /// The value returned is guaranteed to be in the range specified by
    /// [`CivilDayNanosecond`](crate::bounds::CivilDayNanosecond).
    #[inline]
    pub const fn nanosecond(self) -> i64 {
        self.nanosecond
    }

    /// Adds the given number of nanoseconds to this civil time and returns the
    /// resulting civil time with any overflowing amount in units of civil
    /// days.
    ///
    /// This returns an error when integer overflow occurs. For example, when
    /// `seconds` is `i64::MAX` and this civil time is any time other than
    /// midnight.
    ///
    /// Note that the number of days returned may exceed the range supported by
    /// [`UnixEpochDay`](crate::civil::UnixEpochDay). Moreover, the number of
    /// days returned may be negative. This occurs only when `nanoseconds` is
    /// negative enough to result in the time wrapping around at `0`.
    #[inline]
    pub const fn overflowing_add(
        self,
        nanoseconds: i64,
    ) -> Result<(TimeNanosecond, i64), RangeError> {
        let Some(sum) = self.nanosecond().checked_add(nanoseconds) else {
            rbail!(b::CivilDayNanosecond::error());
        };
        let days = sum.div_euclid(c::NANOS_PER_CIVIL_DAY);
        let rem = sum.rem_euclid(c::NANOS_PER_CIVIL_DAY);
        Ok((TimeNanosecond { nanosecond: rem }, days))
    }

    /// Converts this nanosecond representation of a civil time into the
    /// components of a civil time.
    ///
    /// The subsecond component is preserved in the returned `Time`.
    #[inline]
    pub const fn to_time(&self) -> Time {
        // Nanosecond is enforced as non-negative elsewhere
        // as such, we can cast to unsigned for faster division:
        let nanos: u64 = self.nanosecond as u64;

        let mut time: Time =
            TimeSecond { second: (nanos / 1_000_000_000) as i32 }.to_time();
        time.subsec_nanosecond = (nanos % 1_000_000_000) as i32;
        time
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn time(hour: i8, minute: i8, second: i8) -> Time {
        timesub(hour, minute, second, 0)
    }

    fn timesub(hour: i8, minute: i8, second: i8, subsec: i32) -> Time {
        Time::new(hour, minute, second, subsec).unwrap()
    }

    fn timesec(second: i32) -> TimeSecond {
        TimeSecond::new(second).unwrap()
    }

    fn timenano(nanosecond: i64) -> TimeNanosecond {
        TimeNanosecond::new(nanosecond).unwrap()
    }

    #[test]
    fn time_to_second_various() {
        let t = time(0, 0, 0);
        assert_eq!(t.to_second().second(), 0);

        let t = timesub(0, 0, 0, 1);
        assert_eq!(t.to_second().second(), 0);

        let t = timesub(0, 0, 0, 999_999_999);
        assert_eq!(t.to_second().second(), 0);

        let t = time(0, 0, 1);
        assert_eq!(t.to_second().second(), 1);

        let t = time(0, 1, 1);
        assert_eq!(t.to_second().second(), 60 + 1);

        let t = time(1, 1, 1);
        assert_eq!(t.to_second().second(), 3600 + 60 + 1);

        let t = time(23, 59, 59);
        assert_eq!(t.to_second().second(), 86_399);
    }

    #[test]
    fn second_to_time_various() {
        let ts = timesec(0);
        assert_eq!(ts.to_time(), time(0, 0, 0));

        let ts = timesec(1);
        assert_eq!(ts.to_time(), time(0, 0, 1));

        let ts = timesec(60 + 1);
        assert_eq!(ts.to_time(), time(0, 1, 1));

        let ts = timesec(3600 + 60 + 1);
        assert_eq!(ts.to_time(), time(1, 1, 1));

        let ts = timesec(86_399);
        assert_eq!(ts.to_time(), time(23, 59, 59));
    }

    #[test]
    fn time_to_nanosecond_various() {
        let t = timesub(0, 0, 0, 0);
        assert_eq!(t.to_nanosecond().nanosecond(), 0);

        let t = timesub(0, 0, 0, 1);
        assert_eq!(t.to_nanosecond().nanosecond(), 1);

        let t = timesub(0, 0, 0, 999_999_999);
        assert_eq!(t.to_nanosecond().nanosecond(), 999_999_999);

        let t = timesub(0, 0, 1, 1);
        assert_eq!(t.to_nanosecond().nanosecond(), 1_000_000_000 + 1);

        let t = timesub(0, 1, 1, 1);
        assert_eq!(
            t.to_nanosecond().nanosecond(),
            (60 + 1) * 1_000_000_000 + 1
        );

        let t = timesub(1, 1, 1, 1);
        assert_eq!(
            t.to_nanosecond().nanosecond(),
            (3600 + 60 + 1) * 1_000_000_000 + 1
        );

        let t = timesub(23, 59, 59, 1);
        assert_eq!(t.to_nanosecond().nanosecond(), 86_399 * 1_000_000_000 + 1);

        let t = timesub(23, 59, 59, 999_999_999);
        assert_eq!(
            t.to_nanosecond().nanosecond(),
            86_399 * 1_000_000_000 + 999_999_999
        );
    }

    #[test]
    fn nanosecond_to_time_various() {
        let ts = timenano(0);
        assert_eq!(ts.to_time(), timesub(0, 0, 0, 0));

        let ts = timenano(1);
        assert_eq!(ts.to_time(), timesub(0, 0, 0, 1));

        let ts = timenano(1_000_000_000 + 1);
        assert_eq!(ts.to_time(), timesub(0, 0, 1, 1));

        let ts = timenano(61 * 1_000_000_000 + 1);
        assert_eq!(ts.to_time(), timesub(0, 1, 1, 1));

        let ts = timenano((3600 + 60 + 1) * 1_000_000_000 + 1);
        assert_eq!(ts.to_time(), timesub(1, 1, 1, 1));

        let ts = timenano(86_399 * 1_000_000_000);
        assert_eq!(ts.to_time(), timesub(23, 59, 59, 0));

        let ts = timenano(86_399 * 1_000_000_000 + 1);
        assert_eq!(ts.to_time(), timesub(23, 59, 59, 1));

        let ts = timenano(86_399 * 1_000_000_000 + 999_999_999);
        assert_eq!(ts.to_time(), timesub(23, 59, 59, 999_999_999));
    }

    #[test]
    fn second_overflowing_add() {
        let ts = timesec(0);
        assert_eq!(ts.overflowing_add(86_399), Ok((timesec(86_399), 0)));
        assert_eq!(ts.overflowing_add(86_400), Ok((timesec(0), 1)));
        assert_eq!(ts.overflowing_add(86_401), Ok((timesec(1), 1)));
        assert_eq!(ts.overflowing_add(i32::MAX), Ok((timesec(11_647), 24855)));

        assert_eq!(ts.overflowing_add(-1), Ok((timesec(86_399), -1)));
        assert_eq!(ts.overflowing_add(-86_399), Ok((timesec(1), -1)));
        assert_eq!(ts.overflowing_add(-86_400), Ok((timesec(0), -1)));
        assert_eq!(ts.overflowing_add(-86_401), Ok((timesec(86_399), -2)));
        assert_eq!(
            ts.overflowing_add(i32::MIN),
            Ok((timesec(74_752), -24856))
        );

        let ts = timesec(86_399);
        assert_eq!(
            ts.overflowing_add(i32::MIN),
            Ok((timesec(74_751), -24855))
        );

        let ts = timesec(1);
        assert!(ts.overflowing_add(i32::MAX).is_err());
    }

    #[test]
    fn nanosecond_overflowing_add() {
        let ts = timenano(0);
        assert_eq!(
            ts.overflowing_add(86_399_000_000_000),
            Ok((timenano(86_399_000_000_000), 0))
        );
        assert_eq!(
            ts.overflowing_add(86_400_000_000_000),
            Ok((timenano(0), 1))
        );
        assert_eq!(
            ts.overflowing_add(86_401_000_000_000),
            Ok((timenano(1_000_000_000), 1))
        );
        assert_eq!(
            ts.overflowing_add(i64::MAX),
            Ok((timenano(85_636_854_775_807), 106_751))
        );

        assert_eq!(
            ts.overflowing_add(-1),
            Ok((timenano(86_399_999_999_999), -1))
        );
        assert_eq!(
            ts.overflowing_add(-1_000_000_000),
            Ok((timenano(86_399_000_000_000), -1))
        );
        assert_eq!(
            ts.overflowing_add(-86_399_000_000_000),
            Ok((timenano(1_000_000_000), -1))
        );
        assert_eq!(
            ts.overflowing_add(-86_400_000_000_000),
            Ok((timenano(0), -1))
        );
        assert_eq!(
            ts.overflowing_add(-86_401_000_000_000),
            Ok((timenano(86_399_000_000_000), -2))
        );
        assert_eq!(
            ts.overflowing_add(i64::MIN),
            Ok((timenano(763_145_224_192), -106_752))
        );

        let ts = timenano(86_399_000_000_000);
        assert_eq!(
            ts.overflowing_add(i64::MIN),
            Ok((timenano(762_145_224_192), -106_751))
        );

        let ts = timenano(1_000_000_000);
        assert!(ts.overflowing_add(i64::MAX).is_err());
        let ts = timenano(1);
        assert!(ts.overflowing_add(i64::MAX).is_err());
    }

    quickcheck::quickcheck! {
        fn prop_time_to_second_roundtrip(t: Time) -> bool {
            let t = Time::new(t.hour(), t.minute(), t.second(), 0).unwrap();
            t == t.to_second().to_time()
        }

        fn prop_time_to_nanosecond_roundtrip(t: Time) -> bool {
            t == t.to_nanosecond().to_time()
        }
    }
}
