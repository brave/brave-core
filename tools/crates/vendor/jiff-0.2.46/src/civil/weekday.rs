use crate::error::Error;

/// A representation for the day of the week.
///
/// The default representation follows ISO 8601. That is, the week starts with
/// Monday and numbering starts at `1`. However, the various constructors and
/// accessors support using other schemes in wide use:
///
/// * [`Weekday::from_monday_zero_offset`] builds a weekday from
/// a scheme that starts the week on Monday at offset `0`, while
/// [`Weekday::to_monday_zero_offset`] converts to it.
/// * [`Weekday::from_monday_one_offset`] builds a weekday from a scheme
/// that starts the week on Monday at offset `1` (the default representation),
/// while [`Weekday::to_monday_one_offset`] converts to it.
/// * [`Weekday::from_sunday_zero_offset`] builds a weekday from
/// a scheme that starts the week on Sunday at offset `0`, while
/// [`Weekday::to_sunday_zero_offset`] converts to it.
/// * [`Weekday::from_sunday_one_offset`] builds a weekday from
/// a scheme that starts the week on Sunday at offset `1`, while
/// [`Weekday::to_sunday_one_offset`] converts to it.
///
/// # Arithmetic
///
/// This type provides [`Weekday::wrapping_add`] and [`Weekday::wrapping_sub`]
/// for performing wrapping arithmetic on weekdays. These are also available
/// via operator overloading:
///
/// ```
/// use jiff::civil::Weekday;
///
/// assert_eq!(Weekday::Monday + 1, Weekday::Tuesday);
/// assert_eq!(Weekday::Sunday - 1, Weekday::Saturday);
/// ```
///
/// # Comparisons
///
/// This type provides `Eq` and `PartialEq` trait implementations for easy
/// comparison:
///
/// ```
/// use jiff::civil::Weekday;
///
/// assert_eq!(Weekday::Wednesday, Weekday::Wednesday + 7);
/// assert_ne!(Weekday::Thursday, Weekday::Friday);
/// ```
///
/// But this type specifically does not provide `Ord` or `PartialOrd` trait
/// implementations. Such an implementation would require deciding whether
/// Sunday is less than Monday or greater than Monday. The former case
/// corresponds to a week scheme where Sunday is the first day in the week,
/// where as the latter corresponds to a scheme where Monday is the first day.
/// Since both schemes are in widespread use, it would be inappropriate to bake
/// in an assumption of one or the other. Instead, one can convert a weekday
/// into the desired offset scheme, and then compare the offsets:
///
/// ```
/// use jiff::civil::Weekday;
///
/// let (sun, mon) = (Weekday::Sunday, Weekday::Monday);
/// assert!(sun.to_sunday_zero_offset() < mon.to_sunday_zero_offset());
/// assert!(sun.to_monday_zero_offset() > mon.to_monday_zero_offset());
/// ```
///
/// # Example
///
/// This example shows the result of converting to and from different schemes:
///
/// ```
/// use jiff::civil::Weekday;
///
/// // The different representations of Monday.
/// assert_eq!(Weekday::Monday.to_monday_zero_offset(), 0);
/// assert_eq!(Weekday::Monday.to_monday_one_offset(), 1);
/// assert_eq!(Weekday::Monday.to_sunday_zero_offset(), 1);
/// assert_eq!(Weekday::Monday.to_sunday_one_offset(), 2);
///
/// // The different representations of Sunday.
/// assert_eq!(Weekday::Sunday.to_monday_zero_offset(), 6);
/// assert_eq!(Weekday::Sunday.to_monday_one_offset(), 7);
/// assert_eq!(Weekday::Sunday.to_sunday_zero_offset(), 0);
/// assert_eq!(Weekday::Sunday.to_sunday_one_offset(), 1);
/// ```
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
    /// Convert an offset to a structured `Weekday`.
    ///
    /// The offset should be from a scheme where the first day of the week
    /// is Monday and starts numbering at `0`.
    ///
    /// # Errors
    ///
    /// This returns an error when the given offset is not in the range
    /// `0..=6`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// let weekday = Weekday::from_monday_zero_offset(3)?;
    /// assert_eq!(weekday, Weekday::Thursday);
    ///
    /// assert!(Weekday::from_monday_zero_offset(7).is_err());
    /// assert!(Weekday::from_monday_zero_offset(-1).is_err());
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub fn from_monday_zero_offset(offset: i8) -> Result<Weekday, Error> {
        jcore::civil::Weekday::from_monday_zero_offset(offset)
            .map_err(Error::jcore_range)
            .map(Weekday::from_jcore)
    }

    /// Convert an offset to a structured `Weekday`.
    ///
    /// The offset should be from a scheme where the first day of the week
    /// is Monday and starts numbering at `1`.
    ///
    /// # Errors
    ///
    /// This returns an error when the given offset is not in the range
    /// `1..=7`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// let weekday = Weekday::from_monday_one_offset(4)?;
    /// assert_eq!(weekday, Weekday::Thursday);
    ///
    /// assert!(Weekday::from_monday_one_offset(8).is_err());
    /// assert!(Weekday::from_monday_one_offset(0).is_err());
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub fn from_monday_one_offset(offset: i8) -> Result<Weekday, Error> {
        jcore::civil::Weekday::from_monday_one_offset(offset)
            .map_err(Error::jcore_range)
            .map(Weekday::from_jcore)
    }

    /// Convert an offset to a structured `Weekday`.
    ///
    /// The offset should be from a scheme where the first day of the week
    /// is Sunday and starts numbering at `0`.
    ///
    /// # Errors
    ///
    /// This returns an error when the given offset is not in the range
    /// `0..=6`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// let weekday = Weekday::from_sunday_zero_offset(0)?;
    /// assert_eq!(weekday, Weekday::Sunday);
    /// let weekday = Weekday::from_sunday_zero_offset(4)?;
    /// assert_eq!(weekday, Weekday::Thursday);
    ///
    /// assert!(Weekday::from_sunday_zero_offset(7).is_err());
    /// assert!(Weekday::from_sunday_zero_offset(-1).is_err());
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub fn from_sunday_zero_offset(offset: i8) -> Result<Weekday, Error> {
        jcore::civil::Weekday::from_sunday_zero_offset(offset)
            .map_err(Error::jcore_range)
            .map(Weekday::from_jcore)
    }

    /// Convert an offset to a structured `Weekday`.
    ///
    /// The offset should be from a scheme where the first day of the week
    /// is Sunday and starts numbering at `1`.
    ///
    /// # Errors
    ///
    /// This returns an error when the given offset is not in the range
    /// `1..=7`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// let weekday = Weekday::from_sunday_one_offset(5)?;
    /// assert_eq!(weekday, Weekday::Thursday);
    ///
    /// assert!(Weekday::from_sunday_one_offset(8).is_err());
    /// assert!(Weekday::from_sunday_one_offset(0).is_err());
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub fn from_sunday_one_offset(offset: i8) -> Result<Weekday, Error> {
        jcore::civil::Weekday::from_sunday_one_offset(offset)
            .map_err(Error::jcore_range)
            .map(Weekday::from_jcore)
    }

    /// Returns this weekday as an offset.
    ///
    /// The offset returned is computed based on a week that starts with Monday
    /// and begins numbering at `0`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Thursday.to_monday_zero_offset(), 3);
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub const fn to_monday_zero_offset(self) -> i8 {
        self.to_jcore().to_monday_zero_offset()
    }

    /// Returns this weekday as an offset.
    ///
    /// The offset returned is computed based on a week that starts with Monday
    /// and begins numbering at `1`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Thursday.to_monday_one_offset(), 4);
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub const fn to_monday_one_offset(self) -> i8 {
        self.to_jcore().to_monday_one_offset()
    }

    /// Returns this weekday as an offset.
    ///
    /// The offset returned is computed based on a week that starts with Sunday
    /// and begins numbering at `0`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Thursday.to_sunday_zero_offset(), 4);
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub const fn to_sunday_zero_offset(self) -> i8 {
        self.to_jcore().to_sunday_zero_offset()
    }

    /// Returns this weekday as an offset.
    ///
    /// The offset returned is computed based on a week that starts with Sunday
    /// and begins numbering at `1`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Thursday.to_sunday_one_offset(), 5);
    ///
    /// # Ok::<(), Box<dyn std::error::Error>>(())
    /// ```
    #[inline]
    pub const fn to_sunday_one_offset(self) -> i8 {
        self.to_jcore().to_sunday_one_offset()
    }

    /// Returns the next weekday, wrapping around at the end of week to the
    /// beginning of the week.
    ///
    /// This is a convenience routing for calling [`Weekday::wrapping_add`]
    /// with a value of `1`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Wednesday.next(), Weekday::Thursday);
    /// assert_eq!(Weekday::Sunday.next(), Weekday::Monday);
    /// assert_eq!(Weekday::Saturday.next(), Weekday::Sunday);
    /// ```
    #[inline]
    pub fn next(self) -> Weekday {
        self.wrapping_add(1)
    }

    /// Returns the previous weekday, wrapping around at the beginning of week
    /// to the end of the week.
    ///
    /// This is a convenience routing for calling [`Weekday::wrapping_sub`]
    /// with a value of `1`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Wednesday.previous(), Weekday::Tuesday);
    /// assert_eq!(Weekday::Sunday.previous(), Weekday::Saturday);
    /// assert_eq!(Weekday::Saturday.previous(), Weekday::Friday);
    /// ```
    #[inline]
    pub fn previous(self) -> Weekday {
        self.wrapping_sub(1)
    }

    /// Returns the number of days from `other` to this weekday.
    ///
    /// Adding the returned number of days to `other` is guaranteed to sum to
    /// this weekday. The number of days returned is guaranteed to be in the
    /// range `0..=6`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Friday.since(Weekday::Tuesday), 3);
    /// assert_eq!(Weekday::Tuesday.since(Weekday::Tuesday), 0);
    /// assert_eq!(Weekday::Monday.since(Weekday::Sunday), 1);
    /// assert_eq!(Weekday::Sunday.since(Weekday::Monday), 6);
    /// ```
    #[inline]
    pub const fn since(self, other: Weekday) -> i8 {
        self.to_jcore().since(other.to_jcore())
    }

    /// Returns the number of days until `other` from this weekday.
    ///
    /// Adding the returned number of days to this weekday is guaranteed to sum
    /// to `other` weekday. The number of days returned is guaranteed to be in
    /// the range `0..=6`.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Friday.until(Weekday::Tuesday), 4);
    /// assert_eq!(Weekday::Tuesday.until(Weekday::Tuesday), 0);
    /// assert_eq!(Weekday::Monday.until(Weekday::Sunday), 6);
    /// assert_eq!(Weekday::Sunday.until(Weekday::Monday), 1);
    /// ```
    #[inline]
    pub fn until(self, other: Weekday) -> i8 {
        self.to_jcore().until(other.to_jcore())
    }

    /// Add the given number of days to this weekday, using wrapping arithmetic,
    /// and return the resulting weekday.
    ///
    /// Adding a multiple of `7` (including `0`) is guaranteed to produce the
    /// same weekday as this one.
    ///
    /// Note that this routine is also available via the `+` operator.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Sunday.wrapping_add(1), Weekday::Monday);
    /// assert_eq!(Weekday::Sunday.wrapping_add(2), Weekday::Tuesday);
    /// assert_eq!(Weekday::Saturday.wrapping_add(1), Weekday::Sunday);
    /// assert_eq!(Weekday::Saturday.wrapping_add(7), Weekday::Saturday);
    /// assert_eq!(Weekday::Sunday.wrapping_add(-1), Weekday::Saturday);
    /// ```
    ///
    /// Wrapping arithmetic is also performed by the `+` operator:
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Sunday + 1, Weekday::Monday);
    /// assert_eq!(Weekday::Sunday + 2, Weekday::Tuesday);
    /// assert_eq!(Weekday::Saturday + 1, Weekday::Sunday);
    /// assert_eq!(Weekday::Saturday + 7, Weekday::Saturday);
    /// assert_eq!(Weekday::Sunday + -1, Weekday::Saturday);
    /// // The weekday can also be on the right hand side of addition:
    /// assert_eq!(1 + Weekday::Sunday, Weekday::Monday);
    /// ```
    #[inline]
    pub fn wrapping_add<D: Into<i64>>(self, days: D) -> Weekday {
        Weekday::from_jcore(self.to_jcore().wrapping_add(days.into()))
    }

    /// Subtract the given number of days from this weekday, using wrapping
    /// arithmetic, and return the resulting weekday.
    ///
    /// Subtracting a multiple of `7` (including `0`) is guaranteed to produce
    /// the same weekday as this one.
    ///
    /// Note that this routine is also available via the `-` operator.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Sunday.wrapping_sub(1), Weekday::Saturday);
    /// assert_eq!(Weekday::Sunday.wrapping_sub(2), Weekday::Friday);
    /// assert_eq!(Weekday::Saturday.wrapping_sub(1), Weekday::Friday);
    /// assert_eq!(Weekday::Saturday.wrapping_sub(7), Weekday::Saturday);
    /// assert_eq!(Weekday::Sunday.wrapping_sub(-1), Weekday::Monday);
    /// ```
    ///
    /// Wrapping arithmetic is also performed by the `-` operator:
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// assert_eq!(Weekday::Sunday - 1, Weekday::Saturday);
    /// assert_eq!(Weekday::Sunday - 2, Weekday::Friday);
    /// assert_eq!(Weekday::Saturday - 1, Weekday::Friday);
    /// assert_eq!(Weekday::Saturday - 7, Weekday::Saturday);
    /// assert_eq!(Weekday::Sunday - -1, Weekday::Monday);
    /// ```
    ///
    /// Unlike addition, since subtraction is not commutative and negating a
    /// weekday has no semantic meaning, the weekday cannot be on the right
    /// hand side of the `-` operator.
    #[inline]
    pub fn wrapping_sub<D: Into<i64>>(self, days: D) -> Weekday {
        Weekday::from_jcore(self.to_jcore().wrapping_sub(days.into()))
    }

    /// Starting with this weekday, this returns an unending iterator that
    /// cycles forward through the days of the week.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// let mut it = Weekday::Tuesday.cycle_forward();
    /// assert_eq!(it.next(), Some(Weekday::Tuesday));
    /// assert_eq!(it.next(), Some(Weekday::Wednesday));
    /// assert_eq!(it.next(), Some(Weekday::Thursday));
    /// assert_eq!(it.next(), Some(Weekday::Friday));
    /// assert_eq!(it.next(), Some(Weekday::Saturday));
    /// assert_eq!(it.next(), Some(Weekday::Sunday));
    /// assert_eq!(it.next(), Some(Weekday::Monday));
    /// assert_eq!(it.next(), Some(Weekday::Tuesday));
    /// ```
    #[inline]
    pub const fn cycle_forward(self) -> WeekdaysForward {
        WeekdaysForward { it: self.to_jcore().cycle_forward() }
    }

    /// Starting with this weekday, this returns an unending iterator that
    /// cycles backward through the days of the week.
    ///
    /// # Example
    ///
    /// ```
    /// use jiff::civil::Weekday;
    ///
    /// let mut it = Weekday::Tuesday.cycle_reverse();
    /// assert_eq!(it.next(), Some(Weekday::Tuesday));
    /// assert_eq!(it.next(), Some(Weekday::Monday));
    /// assert_eq!(it.next(), Some(Weekday::Sunday));
    /// assert_eq!(it.next(), Some(Weekday::Saturday));
    /// assert_eq!(it.next(), Some(Weekday::Friday));
    /// assert_eq!(it.next(), Some(Weekday::Thursday));
    /// assert_eq!(it.next(), Some(Weekday::Wednesday));
    /// assert_eq!(it.next(), Some(Weekday::Tuesday));
    /// ```
    #[inline]
    pub const fn cycle_reverse(self) -> WeekdaysReverse {
        WeekdaysReverse { it: self.to_jcore().cycle_reverse() }
    }
}

impl Weekday {
    #[inline]
    pub(crate) const fn from_jcore(weekday: jcore::civil::Weekday) -> Weekday {
        match weekday {
            jcore::civil::Weekday::Monday => Weekday::Monday,
            jcore::civil::Weekday::Tuesday => Weekday::Tuesday,
            jcore::civil::Weekday::Wednesday => Weekday::Wednesday,
            jcore::civil::Weekday::Thursday => Weekday::Thursday,
            jcore::civil::Weekday::Friday => Weekday::Friday,
            jcore::civil::Weekday::Saturday => Weekday::Saturday,
            jcore::civil::Weekday::Sunday => Weekday::Sunday,
        }
    }

    #[inline]
    pub(crate) const fn to_jcore(self) -> jcore::civil::Weekday {
        match self {
            Weekday::Monday => jcore::civil::Weekday::Monday,
            Weekday::Tuesday => jcore::civil::Weekday::Tuesday,
            Weekday::Wednesday => jcore::civil::Weekday::Wednesday,
            Weekday::Thursday => jcore::civil::Weekday::Thursday,
            Weekday::Friday => jcore::civil::Weekday::Friday,
            Weekday::Saturday => jcore::civil::Weekday::Saturday,
            Weekday::Sunday => jcore::civil::Weekday::Sunday,
        }
    }
}

impl core::ops::Add<i8> for Weekday {
    type Output = Weekday;

    #[inline]
    fn add(self, rhs: i8) -> Weekday {
        self.wrapping_add(rhs)
    }
}

impl core::ops::Add<i16> for Weekday {
    type Output = Weekday;

    #[inline]
    fn add(self, rhs: i16) -> Weekday {
        self.wrapping_add(rhs)
    }
}

impl core::ops::Add<i32> for Weekday {
    type Output = Weekday;

    #[inline]
    fn add(self, rhs: i32) -> Weekday {
        self.wrapping_add(rhs)
    }
}

impl core::ops::Add<i64> for Weekday {
    type Output = Weekday;

    #[inline]
    fn add(self, rhs: i64) -> Weekday {
        self.wrapping_add(rhs)
    }
}

// Since addition is commutative, we don't care if users write `n + weekday`
// or `weekday + n`.

impl core::ops::Add<Weekday> for i8 {
    type Output = Weekday;

    #[inline]
    fn add(self, rhs: Weekday) -> Weekday {
        rhs.wrapping_add(self)
    }
}

impl core::ops::Add<Weekday> for i16 {
    type Output = Weekday;

    #[inline]
    fn add(self, rhs: Weekday) -> Weekday {
        rhs.wrapping_add(self)
    }
}

impl core::ops::Add<Weekday> for i32 {
    type Output = Weekday;

    #[inline]
    fn add(self, rhs: Weekday) -> Weekday {
        rhs.wrapping_add(self)
    }
}

impl core::ops::Add<Weekday> for i64 {
    type Output = Weekday;

    #[inline]
    fn add(self, rhs: Weekday) -> Weekday {
        rhs.wrapping_add(self)
    }
}

impl core::ops::AddAssign<i8> for Weekday {
    #[inline]
    fn add_assign(&mut self, rhs: i8) {
        *self = *self + rhs;
    }
}

impl core::ops::AddAssign<i16> for Weekday {
    #[inline]
    fn add_assign(&mut self, rhs: i16) {
        *self = *self + rhs;
    }
}

impl core::ops::AddAssign<i32> for Weekday {
    #[inline]
    fn add_assign(&mut self, rhs: i32) {
        *self = *self + rhs;
    }
}

impl core::ops::AddAssign<i64> for Weekday {
    #[inline]
    fn add_assign(&mut self, rhs: i64) {
        *self = *self + rhs;
    }
}

// Subtraction isn't commutative, so we only define it when the right hand
// side is an integer. Otherwise we'd need a concept of what it means to
// "negate" a weekday, which doesn't really make sense?

impl core::ops::Sub<i8> for Weekday {
    type Output = Weekday;

    #[inline]
    fn sub(self, rhs: i8) -> Weekday {
        self.wrapping_sub(rhs)
    }
}

impl core::ops::Sub<i16> for Weekday {
    type Output = Weekday;

    #[inline]
    fn sub(self, rhs: i16) -> Weekday {
        self.wrapping_sub(rhs)
    }
}

impl core::ops::Sub<i32> for Weekday {
    type Output = Weekday;

    #[inline]
    fn sub(self, rhs: i32) -> Weekday {
        self.wrapping_sub(rhs)
    }
}

impl core::ops::Sub<i64> for Weekday {
    type Output = Weekday;

    #[inline]
    fn sub(self, rhs: i64) -> Weekday {
        self.wrapping_sub(rhs)
    }
}

impl core::ops::SubAssign<i8> for Weekday {
    #[inline]
    fn sub_assign(&mut self, rhs: i8) {
        *self = *self - rhs;
    }
}

impl core::ops::SubAssign<i16> for Weekday {
    #[inline]
    fn sub_assign(&mut self, rhs: i16) {
        *self = *self - rhs;
    }
}

impl core::ops::SubAssign<i32> for Weekday {
    #[inline]
    fn sub_assign(&mut self, rhs: i32) {
        *self = *self - rhs;
    }
}

impl core::ops::SubAssign<i64> for Weekday {
    #[inline]
    fn sub_assign(&mut self, rhs: i64) {
        *self = *self - rhs;
    }
}

#[cfg(test)]
impl quickcheck::Arbitrary for Weekday {
    fn arbitrary(g: &mut quickcheck::Gen) -> Weekday {
        let offset = crate::util::b::WeekdayMondayZero::arbitrary(g);
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

/// An unending iterator of the days of the week.
///
/// This iterator is created by calling [`Weekday::cycle_forward`].
#[derive(Clone, Debug)]
pub struct WeekdaysForward {
    it: jcore::civil::WeekdaysForward,
}

impl Iterator for WeekdaysForward {
    type Item = Weekday;

    #[inline]
    fn next(&mut self) -> Option<Weekday> {
        self.it.next().map(Weekday::from_jcore)
    }
}

impl core::iter::FusedIterator for WeekdaysForward {}

/// An unending iterator of the days of the week in reverse.
///
/// This iterator is created by calling [`Weekday::cycle_reverse`].
#[derive(Clone, Debug)]
pub struct WeekdaysReverse {
    it: jcore::civil::WeekdaysReverse,
}

impl Iterator for WeekdaysReverse {
    type Item = Weekday;

    #[inline]
    fn next(&mut self) -> Option<Weekday> {
        self.it.next().map(Weekday::from_jcore)
    }
}

impl core::iter::FusedIterator for WeekdaysReverse {}
