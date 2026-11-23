use crate::{
    bounds::{self as b, RangeError},
    macros::{rbail, unwrapr},
};

/// A representation for the day of the week.
#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
#[cfg_attr(feature = "arbitrary", derive(arbitrary::Arbitrary))]
#[repr(u8)]
#[allow(missing_docs)]
pub enum Weekday {
    Monday = 1,
    Tuesday = 2,
    Wednesday = 3,
    Thursday = 4,
    Friday = 5,
    Saturday = 6,
    Sunday = 7,
}

impl Weekday {
    /// Convert a 0-offset to a `Weekday`. Monday corresponds to offset `0` and
    /// Sunday corresponds to offset `6`.
    #[inline]
    pub const fn from_monday_zero_offset(
        offset: i8,
    ) -> Result<Weekday, RangeError> {
        Ok(match offset {
            0 => Weekday::Monday,
            1 => Weekday::Tuesday,
            2 => Weekday::Wednesday,
            3 => Weekday::Thursday,
            4 => Weekday::Friday,
            5 => Weekday::Saturday,
            6 => Weekday::Sunday,
            _ => rbail!(b::WeekdayMondayZero::error()),
        })
    }

    /// Convert a 1-offset to a `Weekday`. Monday corresponds to offset `1` and
    /// Sunday corresponds to offset `7`.
    #[inline]
    pub const fn from_monday_one_offset(
        offset: i8,
    ) -> Result<Weekday, RangeError> {
        Ok(match offset {
            1 => Weekday::Monday,
            2 => Weekday::Tuesday,
            3 => Weekday::Wednesday,
            4 => Weekday::Thursday,
            5 => Weekday::Friday,
            6 => Weekday::Saturday,
            7 => Weekday::Sunday,
            _ => rbail!(b::WeekdayMondayOne::error()),
        })
    }

    /// Convert a 0-offset to a `Weekday`. Sunday corresponds to offset `0` and
    /// Saturday corresponds to offset `6`.
    #[inline]
    pub const fn from_sunday_zero_offset(
        offset: i8,
    ) -> Result<Weekday, RangeError> {
        Ok(match offset {
            0 => Weekday::Sunday,
            1 => Weekday::Monday,
            2 => Weekday::Tuesday,
            3 => Weekday::Wednesday,
            4 => Weekday::Thursday,
            5 => Weekday::Friday,
            6 => Weekday::Saturday,
            _ => rbail!(b::WeekdaySundayZero::error()),
        })
    }

    /// Convert a 1-offset to a `Weekday`. Sunday corresponds to offset `1` and
    /// Saturday corresponds to offset `7`.
    #[inline]
    pub const fn from_sunday_one_offset(
        offset: i8,
    ) -> Result<Weekday, RangeError> {
        Ok(match offset {
            1 => Weekday::Sunday,
            2 => Weekday::Monday,
            3 => Weekday::Tuesday,
            4 => Weekday::Wednesday,
            5 => Weekday::Thursday,
            6 => Weekday::Friday,
            7 => Weekday::Saturday,
            _ => rbail!(b::WeekdaySundayOne::error()),
        })
    }

    /// Returns the weekday as a 0-offset. Monday corresponds to offset `0`
    /// and Sunday corresponds to offset `6`.
    #[inline]
    pub const fn to_monday_zero_offset(self) -> i8 {
        self.to_monday_one_offset() - 1
    }

    /// Returns the weekday as a 1-offset. Monday corresponds to offset `1`
    /// and Sunday corresponds to offset `7`.
    #[inline]
    pub const fn to_monday_one_offset(self) -> i8 {
        self as i8
    }

    /// Returns the weekday as a 0-offset. Sunday corresponds to offset `0`
    /// and Saturday corresponds to offset `6`.
    #[inline]
    pub const fn to_sunday_zero_offset(self) -> i8 {
        let offset = self.to_monday_one_offset();
        if offset == 7 { 0 } else { offset }
    }

    /// Returns the weekday as a 1-offset. Sunday corresponds to offset `1`
    /// and Saturday corresponds to offset `7`.
    #[inline]
    pub const fn to_sunday_one_offset(self) -> i8 {
        self.to_sunday_zero_offset() + 1
    }

    /// Add the given number of days to this weekday, using wrapping arithmetic,
    /// and return the resulting weekday.
    ///
    /// Adding a multiple of `7` (including `0`) is guaranteed to produce the
    /// same weekday as this one.
    #[inline]
    pub const fn wrapping_add(self, days: i64) -> Weekday {
        let start = self.to_monday_zero_offset() as i64;
        // We are careful to `rem_euclid` on `rhs` before doing
        // wrapping arithmetic, otherwise the result is not
        // correct. Namely, it would assume that, e.g., since
        // `i64::MAX.rem_euclid(7)` is 0, then the next value would be
        // `rem_euclid(7) == 1`. But `i64::MIN.rem_euclid(7)` is 6.
        let end = (start.wrapping_add(days.rem_euclid(7)) % 7) as i8;
        // Always valid because of the mod 7 above.
        unwrapr!(
            Weekday::from_monday_zero_offset(end),
            "weekday is always 0..=6",
        )
    }

    /// Subtract the given number of days from this weekday, using wrapping
    /// arithmetic, and return the resulting weekday.
    ///
    /// Subtracting a multiple of `7` (including `0`) is guaranteed to produce
    /// the same weekday as this one.
    #[inline]
    pub const fn wrapping_sub(self, days: i64) -> Weekday {
        // i64::MIN.rem_euclid(7) == 6
        let days = match days.checked_neg() {
            Some(days) => days,
            None => -6,
        };
        self.wrapping_add(days)
    }

    /// Returns the next weekday, wrapping around at the end of week to the
    /// beginning of the week.
    ///
    /// This is a convenience routing for calling [`Weekday::wrapping_add`]
    /// with a value of `1`.
    #[inline]
    pub const fn next(self) -> Weekday {
        self.wrapping_add(1)
    }

    /// Returns the previous weekday, wrapping around at the beginning of week
    /// to the end of the week.
    ///
    /// This is a convenience routing for calling [`Weekday::wrapping_sub`]
    /// with a value of `1`.
    #[inline]
    pub const fn previous(self) -> Weekday {
        self.wrapping_sub(1)
    }

    /// Returns the number of days from `other` to this weekday.
    ///
    /// Adding the returned number of days to `other` is guaranteed to sum to
    /// this weekday. The number of days returned is guaranteed to be in the
    /// range `0..=6`.
    #[inline]
    pub const fn since(self, other: Weekday) -> i8 {
        (self.to_monday_zero_offset() - other.to_monday_zero_offset())
            .rem_euclid(7)
    }

    /// Returns the number of days until `other` from this weekday.
    ///
    /// Adding the returned number of days to this weekday is guaranteed to sum
    /// to `other` weekday. The number of days returned is guaranteed to be in
    /// the range `0..=6`.
    #[inline]
    pub const fn until(self, other: Weekday) -> i8 {
        other.since(self)
    }

    /// Starting with this weekday, this returns an unending iterator that
    /// cycles forward through the days of the week.
    #[inline]
    pub const fn cycle_forward(self) -> WeekdaysForward {
        WeekdaysForward { next: self }
    }

    /// Starting with this weekday, this returns an unending iterator that
    /// cycles backward through the days of the week.
    #[inline]
    pub const fn cycle_reverse(self) -> WeekdaysReverse {
        WeekdaysReverse { next: self }
    }
}

/// An unending iterator of the days of the week.
///
/// This iterator is created by calling [`Weekday::cycle_forward`].
#[derive(Clone, Debug)]
pub struct WeekdaysForward {
    next: Weekday,
}

impl Iterator for WeekdaysForward {
    type Item = Weekday;

    #[inline]
    fn next(&mut self) -> Option<Weekday> {
        let next = self.next;
        self.next = self.next.wrapping_add(1);
        Some(next)
    }
}

impl core::iter::FusedIterator for WeekdaysForward {}

/// An unending iterator of the days of the week in reverse.
///
/// This iterator is created by calling [`Weekday::cycle_reverse`].
#[derive(Clone, Debug)]
pub struct WeekdaysReverse {
    next: Weekday,
}

impl Iterator for WeekdaysReverse {
    type Item = Weekday;

    #[inline]
    fn next(&mut self) -> Option<Weekday> {
        let next = self.next;
        self.next = self.next.wrapping_sub(1);
        Some(next)
    }
}

impl core::iter::FusedIterator for WeekdaysReverse {}

#[cfg(test)]
impl quickcheck::Arbitrary for Weekday {
    fn arbitrary(g: &mut quickcheck::Gen) -> Weekday {
        let offset = b::WeekdayMondayZero::arbitrary(g);
        Weekday::from_monday_zero_offset(offset).unwrap()
    }

    fn shrink(&self) -> alloc::boxed::Box<dyn Iterator<Item = Weekday>> {
        alloc::boxed::Box::new(
            self.to_monday_zero_offset()
                .shrink()
                .filter_map(|n| Weekday::from_monday_zero_offset(n).ok()),
        )
    }
}

#[cfg(test)]
mod tests {
    use alloc::vec::Vec;

    use super::*;

    static WEEKDAYS: &[Weekday] = &[
        Weekday::Monday,
        Weekday::Tuesday,
        Weekday::Wednesday,
        Weekday::Thursday,
        Weekday::Friday,
        Weekday::Saturday,
        Weekday::Sunday,
    ];

    #[test]
    fn weekday_from_monday_zero() {
        use self::Weekday::*;

        assert_eq!(Weekday::from_monday_zero_offset(0), Ok(Monday));
        assert_eq!(Weekday::from_monday_zero_offset(1), Ok(Tuesday));
        assert_eq!(Weekday::from_monday_zero_offset(2), Ok(Wednesday));
        assert_eq!(Weekday::from_monday_zero_offset(3), Ok(Thursday));
        assert_eq!(Weekday::from_monday_zero_offset(4), Ok(Friday));
        assert_eq!(Weekday::from_monday_zero_offset(5), Ok(Saturday));
        assert_eq!(Weekday::from_monday_zero_offset(6), Ok(Sunday));
    }

    #[test]
    fn weekday_from_monday_one() {
        use self::Weekday::*;

        assert_eq!(Weekday::from_monday_one_offset(1), Ok(Monday));
        assert_eq!(Weekday::from_monday_one_offset(2), Ok(Tuesday));
        assert_eq!(Weekday::from_monday_one_offset(3), Ok(Wednesday));
        assert_eq!(Weekday::from_monday_one_offset(4), Ok(Thursday));
        assert_eq!(Weekday::from_monday_one_offset(5), Ok(Friday));
        assert_eq!(Weekday::from_monday_one_offset(6), Ok(Saturday));
        assert_eq!(Weekday::from_monday_one_offset(7), Ok(Sunday));
    }

    #[test]
    fn weekday_from_sunday_zero() {
        use self::Weekday::*;

        assert_eq!(Weekday::from_sunday_zero_offset(1), Ok(Monday));
        assert_eq!(Weekday::from_sunday_zero_offset(2), Ok(Tuesday));
        assert_eq!(Weekday::from_sunday_zero_offset(3), Ok(Wednesday));
        assert_eq!(Weekday::from_sunday_zero_offset(4), Ok(Thursday));
        assert_eq!(Weekday::from_sunday_zero_offset(5), Ok(Friday));
        assert_eq!(Weekday::from_sunday_zero_offset(6), Ok(Saturday));
        assert_eq!(Weekday::from_sunday_zero_offset(0), Ok(Sunday));
    }

    #[test]
    fn weekday_from_sunday_one() {
        use self::Weekday::*;

        assert_eq!(Weekday::from_sunday_one_offset(2), Ok(Monday));
        assert_eq!(Weekday::from_sunday_one_offset(3), Ok(Tuesday));
        assert_eq!(Weekday::from_sunday_one_offset(4), Ok(Wednesday));
        assert_eq!(Weekday::from_sunday_one_offset(5), Ok(Thursday));
        assert_eq!(Weekday::from_sunday_one_offset(6), Ok(Friday));
        assert_eq!(Weekday::from_sunday_one_offset(7), Ok(Saturday));
        assert_eq!(Weekday::from_sunday_one_offset(1), Ok(Sunday));
    }

    #[test]
    fn weekday_to_monday_zero() {
        for &weekday in WEEKDAYS {
            assert_eq!(
                Weekday::from_monday_zero_offset(
                    weekday.to_monday_zero_offset()
                ),
                Ok(weekday)
            );
        }
    }

    #[test]
    fn weekday_to_monday_one() {
        for &weekday in WEEKDAYS {
            assert_eq!(
                Weekday::from_monday_one_offset(
                    weekday.to_monday_one_offset()
                ),
                Ok(weekday)
            );
        }
    }

    #[test]
    fn weekday_to_sunday_zero() {
        for &weekday in WEEKDAYS {
            assert_eq!(
                Weekday::from_sunday_zero_offset(
                    weekday.to_sunday_zero_offset()
                ),
                Ok(weekday)
            );
        }
    }

    #[test]
    fn weekday_to_sunday_one() {
        for &weekday in WEEKDAYS {
            assert_eq!(
                Weekday::from_sunday_one_offset(
                    weekday.to_sunday_one_offset()
                ),
                Ok(weekday)
            );
        }
    }

    #[test]
    fn weekday_wrapping_add() {
        use self::Weekday::*;

        assert_eq!(Sunday.wrapping_add(0), Sunday);
        assert_eq!(Sunday.wrapping_add(1), Monday);
        assert_eq!(Sunday.wrapping_add(2), Tuesday);
        assert_eq!(Sunday.wrapping_add(3), Wednesday);
        assert_eq!(Sunday.wrapping_add(4), Thursday);
        assert_eq!(Sunday.wrapping_add(5), Friday);
        assert_eq!(Sunday.wrapping_add(6), Saturday);
        assert_eq!(Sunday.wrapping_add(7), Sunday);

        assert_eq!(Wednesday.wrapping_add(0), Wednesday);
        assert_eq!(Wednesday.wrapping_add(1), Thursday);
        assert_eq!(Wednesday.wrapping_add(2), Friday);
        assert_eq!(Wednesday.wrapping_add(3), Saturday);
        assert_eq!(Wednesday.wrapping_add(4), Sunday);
        assert_eq!(Wednesday.wrapping_add(5), Monday);
        assert_eq!(Wednesday.wrapping_add(6), Tuesday);
        assert_eq!(Wednesday.wrapping_add(7), Wednesday);

        assert_eq!(Sunday.wrapping_add(-1), Saturday);
        assert_eq!(Sunday.wrapping_add(-2), Friday);
        assert_eq!(Sunday.wrapping_add(-3), Thursday);
        assert_eq!(Sunday.wrapping_add(-4), Wednesday);
        assert_eq!(Sunday.wrapping_add(-5), Tuesday);
        assert_eq!(Sunday.wrapping_add(-6), Monday);
        assert_eq!(Sunday.wrapping_add(-7), Sunday);

        assert_eq!(Wednesday.wrapping_add(-1), Tuesday);
        assert_eq!(Wednesday.wrapping_add(-2), Monday);
        assert_eq!(Wednesday.wrapping_add(-3), Sunday);
        assert_eq!(Wednesday.wrapping_add(-4), Saturday);
        assert_eq!(Wednesday.wrapping_add(-5), Friday);
        assert_eq!(Wednesday.wrapping_add(-6), Thursday);
        assert_eq!(Wednesday.wrapping_add(-7), Wednesday);

        // This caught a bug where our wrapping arithmetic in
        // `Weekday::wrapping_add` was incorrect when overflow occurred.
        assert_eq!(Tuesday.wrapping_add(9223372036854775807i64), Tuesday);
    }

    #[test]
    fn weekday_wrapping_sub() {
        use self::Weekday::*;

        assert_eq!(Sunday.wrapping_sub(0), Sunday);
        assert_eq!(Sunday.wrapping_sub(1), Saturday);
        assert_eq!(Sunday.wrapping_sub(2), Friday);
        assert_eq!(Sunday.wrapping_sub(3), Thursday);
        assert_eq!(Sunday.wrapping_sub(4), Wednesday);
        assert_eq!(Sunday.wrapping_sub(5), Tuesday);
        assert_eq!(Sunday.wrapping_sub(6), Monday);
        assert_eq!(Sunday.wrapping_sub(7), Sunday);

        assert_eq!(Wednesday.wrapping_sub(0), Wednesday);
        assert_eq!(Wednesday.wrapping_sub(1), Tuesday);
        assert_eq!(Wednesday.wrapping_sub(2), Monday);
        assert_eq!(Wednesday.wrapping_sub(3), Sunday);
        assert_eq!(Wednesday.wrapping_sub(4), Saturday);
        assert_eq!(Wednesday.wrapping_sub(5), Friday);
        assert_eq!(Wednesday.wrapping_sub(6), Thursday);
        assert_eq!(Wednesday.wrapping_sub(7), Wednesday);

        assert_eq!(Sunday.wrapping_sub(-1), Monday);
        assert_eq!(Sunday.wrapping_sub(-2), Tuesday);
        assert_eq!(Sunday.wrapping_sub(-3), Wednesday);
        assert_eq!(Sunday.wrapping_sub(-4), Thursday);
        assert_eq!(Sunday.wrapping_sub(-5), Friday);
        assert_eq!(Sunday.wrapping_sub(-6), Saturday);
        assert_eq!(Sunday.wrapping_sub(-7), Sunday);

        assert_eq!(Wednesday.wrapping_sub(-1), Thursday);
        assert_eq!(Wednesday.wrapping_sub(-2), Friday);
        assert_eq!(Wednesday.wrapping_sub(-3), Saturday);
        assert_eq!(Wednesday.wrapping_sub(-4), Sunday);
        assert_eq!(Wednesday.wrapping_sub(-5), Monday);
        assert_eq!(Wednesday.wrapping_sub(-6), Tuesday);
        assert_eq!(Wednesday.wrapping_sub(-7), Wednesday);

        // This found a bug where we were negating the integer
        // given and assuming it wouldn't panic.
        assert_eq!(Monday.wrapping_sub(-9223372036854775805i64), Saturday);
        assert_eq!(Monday.wrapping_sub(-9223372036854775806i64), Sunday);
        assert_eq!(Monday.wrapping_sub(-9223372036854775807i64), Monday);
        assert_eq!(Monday.wrapping_sub(-9223372036854775808i64), Tuesday);
    }

    #[test]
    fn weekday_since() {
        for &wd1 in WEEKDAYS {
            for (distance, wd2) in wd1.cycle_forward().enumerate().take(7) {
                assert_eq!(
                    usize::try_from(wd2.since(wd1)).unwrap(),
                    distance,
                    "{wd2:?} since {wd1:?} should be {distance}",
                );
            }
        }
    }

    #[test]
    fn weekday_until() {
        for &wd1 in WEEKDAYS {
            for (distance, wd2) in wd1.cycle_forward().enumerate().take(7) {
                assert_eq!(
                    usize::try_from(wd1.until(wd2)).unwrap(),
                    distance,
                    "{wd1:?} until {wd2:?} should be {distance}",
                );
            }
        }
    }

    #[test]
    fn weekday_cycle_forward() {
        assert_eq!(
            WEEKDAYS,
            Weekday::Monday.cycle_forward().take(7).collect::<Vec<_>>(),
        );
    }

    #[test]
    fn weekday_cycle_reverse() {
        let mut got =
            Weekday::Sunday.cycle_reverse().take(7).collect::<Vec<_>>();
        got.reverse();
        assert_eq!(WEEKDAYS, got);
    }

    quickcheck::quickcheck! {
        fn prop_weekday_add_sub(wd: Weekday, n: i64) -> bool {
            wd.wrapping_add(n).wrapping_sub(n) == wd
        }

        fn prop_weekday_since_until(wd1: Weekday, wd2: Weekday) -> bool {
            wd1.until(wd2) == wd2.since(wd1)
        }

        fn prop_since_add_equals_self(wd1: Weekday, wd2: Weekday) -> bool {
            let days = wd1.since(wd2);
            wd2.wrapping_add(days.into()) == wd1
        }

        fn prop_until_add_equals_other(wd1: Weekday, wd2: Weekday) -> bool {
            let days = wd1.until(wd2);
            wd1.wrapping_add(days.into()) == wd2
        }
    }
}
