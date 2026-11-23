/*!
Building blocks for supporting time zones.
*/

use crate::{util::SmallStr, Timestamp};

mod offset;
pub mod posix;
pub mod tzif;

pub use self::offset::{
    AmbiguousError, AmbiguousOffset, AmbiguousTimestamp, Offset,
};

/// A limit on how much stack space we're willing to use for time zone
/// abbreviations.
///
/// POSIX says this:
///
/// > Indicate no less than three, nor more than {TZNAME_MAX}, bytes that are
/// > the designation for the standard (std) or the alternative (dst -such as
/// > Daylight Savings Time) timezone.
///
/// But it doesn't seem worth the trouble to query `TZNAME_MAX`. Interestingly,
/// IANA says:
///
/// > are 3 or more characters specifying the standard and daylight saving time
/// > (DST) zone abbreviations
///
/// Which implies that IANA thinks there is no limit. But that seems unwise.
/// Moreover, in practice, it seems like the `date` utility supports fairly
/// long abbreviations. On my mac (so, BSD `date` as I understand it):
///
/// ```text
/// $ TZ=ZZZ5YYYYYYYYYYYYYYYYYYYYY date
/// Sun Mar 17 20:05:58 YYYYYYYYYYYYYYYYYYYYY 2024
/// ```
///
/// And on my Linux machine (so, GNU `date`):
///
/// ```text
/// $ TZ=ZZZ5YYYYYYYYYYYYYYYYYYYYY date
/// Sun Mar 17 08:05:36 PM YYYYYYYYYYYYYYYYYYYYY 2024
/// ```
///
/// I don't know exactly what limit these programs use, but 30 seems good
/// enough?
///
/// Previously, I had been using 255 and stuffing the string in a `Box<str>`.
/// But as part of work on [#168], I was looking to remove allocation from as
/// many places as possible. And this was one candidate. But making room on the
/// stack for 255 byte abbreviations seemed gratuitous. So I picked something
/// smaller. If we come across an abbreviation bigger than this max, then we'll
/// error.
///
/// In environments with dynamic memory allocation, this maximum is just the
/// maximum number of bytes we're willing to spend on array-backed storage of
/// a time zone abbreviation. If we hit anything bigger, we'll use the heap.
///
/// In core-only environments, we use a bigger limit as mentioned above.
/// Anything bigger than this will result in a parse error.
///
/// [#168]: https://github.com/BurntSushi/jiff/issues/168
const TIME_ZONE_ABBREVIATION_MAX: usize = {
    #[cfg(feature = "alloc")]
    {
        // 6 + 1 byte for the length gives us a nice 7 bytes total for the
        // array.
        6
    }
    #[cfg(not(feature = "alloc"))]
    {
        REASONABLE_ABBREVIATION_MAX
    }
};

/// When a time zone abbreviation is bigger than this, we give up and error.
const REASONABLE_ABBREVIATION_MAX: usize = {
    #[cfg(feature = "alloc")]
    {
        // Let this expand to a pretty unreasonable amount.
        // We could make this even higher, but we should have some
        // kind of limit.
        255
    }
    #[cfg(not(feature = "alloc"))]
    {
        // We make this the same as the array capacity maximum in environments
        // with dynamic memory allocation as a conservative choice.
        //
        // This seems short, but at time of writing, the maximum possible
        // abbreviation in the tzdb is 5 bytes.
        //
        // Actually, we make this bigger so that an abbreviation can fit a
        // full offset to second resolution. e.g., `+10:30:25`.
        9
    }
};

/// A limit on how much stack space we're willing to use for time zone
/// identifiers.
///
/// As of 2026-07-03, 32 is the length of the longest IANA time zone
/// identifier. Specifically, `America/Argentina/ComodRivadavia`. Anything
/// bigger than this will error in core-only environments and spill
/// over to the heap in all other environments. For environments with
/// a heap, we set a slightly smaller array to make the total size a
/// bit smaller (4 words on x86-64 instead of 5). This just means that
/// `America/Argentina/ComodRivadavia` will spill to the heap, but nothing else
/// should.
const TIME_ZONE_ID_MAX: usize = {
    #[cfg(feature = "alloc")]
    {
        30
    }
    #[cfg(not(feature = "alloc"))]
    {
        32
    }
};

/// A type that defines the storage for an IANA time zone identifier.
pub type TimeZoneId = SmallStr<TIME_ZONE_ID_MAX>;

/// A type that defines the storage for a time zone abbreviation.
///
/// For an abbreviation whose length is less than or equal to a certain
/// implementation defined number, it will be stored inline inside an array.
/// All other cases spill out into the heap. (Unless callers do not have
/// dynamic memory allocation, in which case, whatever tried to store the
/// time zone abbreviation will return an error. For example, parsing a POSIX
/// time zone transition rule will fail in that case.)
pub type Abbreviation = SmallStr<TIME_ZONE_ABBREVIATION_MAX>;

/// A representation a single time zone transition.
#[derive(Clone, Debug)]
pub struct Transition {
    timestamp: Timestamp,
    info: OffsetInfo,
}

impl Transition {
    /// Returns the timestamp at which this transition began.
    pub fn timestamp(&self) -> Timestamp {
        self.timestamp
    }

    /// Returns the offset corresponding to this time zone transition. All
    /// instants at and following this transition's timestamp (and before the
    /// next transition's timestamp) need to apply this offset from UTC to get
    /// the civil or "local" time in the corresponding time zone.
    pub fn offset(&self) -> Offset {
        self.info.offset()
    }

    /// Returns the time zone abbreviation corresponding to this time
    /// zone transition.
    pub fn abbreviation(&self) -> &Abbreviation {
        &self.info.abbreviation()
    }

    /// Returns whether daylight saving time is enabled for this time zone
    /// transition.
    pub fn dst(&self) -> Dst {
        self.info.dst()
    }

    /// Consumes this transition and returns the underlying `OffsetInfo`.
    pub fn into_offset_info(self) -> OffsetInfo {
        self.info
    }
}

/// Information associated with an offset when doing a time zone transition
/// lookup.
///
/// Callers should generally only need the offset. This exposes additional
/// information such as the time zone abbreviation or whether a timestamp is in
/// daylight saving time or not.
#[derive(Clone, Debug, Eq, Hash, PartialEq)]
pub struct OffsetInfo {
    offset: Offset,
    abbreviation: Abbreviation,
    dst: Dst,
}

impl OffsetInfo {
    /// Returns the offset corresponding to this time zone transition. All
    /// instants at and following this transition's timestamp (and before the
    /// next transition's timestamp) need to apply this offset from UTC to get
    /// the civil or "local" time in the corresponding time zone.
    pub fn offset(&self) -> Offset {
        self.offset
    }

    /// Returns the time zone abbreviation corresponding to this time
    /// zone transition.
    pub fn abbreviation(&self) -> &Abbreviation {
        &self.abbreviation
    }

    /// Consumes this offset info and returns its abbreviation.
    pub fn into_abbreviation(self) -> Abbreviation {
        self.abbreviation
    }

    /// Returns whether daylight saving time is enabled for this time zone
    /// transition.
    pub fn dst(&self) -> Dst {
        self.dst
    }
}

/// An enum indicating whether a particular datetime is in daylight saving time
/// (DST) or not.
#[derive(Clone, Copy, Debug, Eq, Hash, PartialEq, PartialOrd, Ord)]
#[cfg_attr(feature = "defmt", derive(defmt::Format))]
pub enum Dst {
    /// DST is not in effect. In other words, standard time is in effect.
    No,
    /// DST is in effect.
    Yes,
}

impl Dst {
    /// Returns true when this value is equal to `Dst::Yes`.
    pub fn is_dst(self) -> bool {
        matches!(self, Dst::Yes)
    }

    /// Returns true when this value is equal to `Dst::No`.
    ///
    /// `std` in this context refers to "standard time." That is, it is the
    /// offset from UTC used when DST is not in effect.
    pub fn is_std(self) -> bool {
        matches!(self, Dst::No)
    }
}

impl From<bool> for Dst {
    fn from(is_dst: bool) -> Dst {
        if is_dst {
            Dst::Yes
        } else {
            Dst::No
        }
    }
}

/// Creates a new time zone offset in a `const` context from a given number
/// of hours.
///
/// Negative offsets correspond to time zones west of the prime meridian,
/// while positive offsets correspond to time zones east of the prime
/// meridian. Equivalently, in all cases, `civil-time - offset = UTC`.
///
/// The fallible non-const version of this constructor is
/// [`Offset::from_hours`].
///
/// This is a convenience free function for [`Offset::constant`]. It is
/// intended to provide a terse syntax for constructing `Offset` values from
/// a value that is known to be valid.
///
/// # Panics
///
/// This routine panics when the given number of hours is out of range.
/// Namely, `hours` must be in the range `-25..=25`.
///
/// Similarly, when used in a const context, an out of bounds hour will prevent
/// your Rust program from compiling.
///
/// # Example
///
/// ```
/// use jiff_core::tz::offset;
///
/// let o = offset(-5);
/// assert_eq!(o.seconds(), -18_000);
/// let o = offset(5);
/// assert_eq!(o.seconds(), 18_000);
/// ```
#[inline]
pub const fn offset(hours: i8) -> Offset {
    Offset::constant(hours)
}

#[cfg(test)]
mod tests {
    // Don't bother trying to test this on non-64 bit. It's too annoying to
    // keep this test updated.
    #[cfg(target_pointer_width = "64")]
    #[test]
    fn sizes() {
        use super::*;

        assert_eq!(24, core::mem::size_of::<Abbreviation>());

        #[cfg(feature = "alloc")]
        assert_eq!(32, core::mem::size_of::<TimeZoneId>());
        #[cfg(not(feature = "alloc"))]
        assert_eq!(40, core::mem::size_of::<TimeZoneId>());
    }
}
