/*!
Implements [TZif] time zone parsing and transition handling.

The TZif format is used by the [Time Zone Database].

The parser in this module is designed to handle untrusted input. That is, there
is no input that should cause it to panic or allocate memory in a way that is
not proportional to the size of the TZif data.

These binary files are the ones commonly found in Unix distributions in the
`/usr/share/zoneinfo` directory.

[Time Zone Database]: https://www.iana.org/time-zones
[TZif]: https://datatracker.ietf.org/doc/rfc9636/
*/

use crate::{
    bounds::RangeError,
    civil,
    macros::unwrapr,
    tz::{self, posix, Abbreviation, Dst, Offset},
    util::MaybeStaticSlice,
};

#[cfg(feature = "alloc")]
mod parser;
mod query;

#[cfg(feature = "alloc")]
pub use self::parser::ParseError;

/// A representation of a possibly named time zone backed by [TZif] data.
///
/// It is useful to represent a named time zone as distinct from the time zone
/// itself because a name is not inherently part of the TZif data. Indeed, there
/// are a couple reasons why it's important to separate the name:
///
/// * It's possible that no meaningful name exists. For example, a
/// `/etc/localtime` file with no obvious mapping to a name. (e.g., No symlink
/// and no discoverable Time Zone Database.)
/// * Some IANA time zone identifiers correspond to different regions, but
/// share the same TZif data.
///
/// [TZif]: https://datatracker.ietf.org/doc/rfc9636/
#[derive(Clone, Debug, PartialEq)]
pub struct MaybeNamedTimeZone {
    /// The name of this TZif time zone. e.g., `America/New_York`.
    pub name: Option<tz::TimeZoneId>,
    /// The time zone itself.
    pub tz: TimeZone,
}

impl MaybeNamedTimeZone {
    /// Returns the underlying time zone name as a string.
    #[inline]
    pub fn name(&self) -> Option<&str> {
        self.name.as_deref()
    }

    /// Returns a reference to the underlying time zone definition.
    #[inline]
    pub fn tz(&self) -> &TimeZone {
        &self.tz
    }
}

/// A representation of an unnamed time zone backed by [TZif] data.
///
/// Two time zones are considered equivalent when their CRC32 sums are
/// equivalent.
///
/// [TZif]: https://datatracker.ietf.org/doc/rfc9636/
#[derive(Clone, Debug)]
// This ensures the alignment of this type is always *at least* 8 bytes. This
// is required for the pointer tagging inside of `TimeZone` to be sound. At
// time of writing (2024-02-24), this explicit `repr` isn't required on 64-bit
// systems since the type definition is such that it will have an alignment of
// at least 8 bytes anyway. But this *is* required for 32-bit systems, where
// the type definition at present only has an alignment of 4 bytes.
#[repr(align(8))]
pub struct TimeZone {
    /// An ASCII byte corresponding to the version number. So, 0x50 is '2'.
    pub version: u8,
    /// A CRC32 checksum of the underlying TZif data.
    ///
    /// This, along with the time zone's IANA identifier, is used to provide a
    /// "best effort" but also cheap notion of strict equality between two time
    /// zones.
    pub checksum: u32,
    /// The time zone abbreviations referenced by local time types.
    pub designations: MaybeStaticSlice<Abbreviation>,
    /// A POSIX time zone used for determining time zone transitions after the
    /// last transition in some TZif data.
    ///
    /// This is technically optional, but is usually present.
    pub posix_tz: Option<posix::TimeZone>,
    /// The local time types in this TZif data.
    ///
    /// Each local time type represents a distinct combination of offset,
    /// abbreviation and whether the region is in daylight saving time or not.
    pub types: MaybeStaticSlice<LocalTimeType>,
    /// The concrete transitions that make up this time zone.
    pub transitions: Transitions,
}

impl TimeZone {
    /// Converts this unnamed time zone into a time zone with the given name.
    #[inline]
    pub fn into_named(self, name: tz::TimeZoneId) -> MaybeNamedTimeZone {
        self.into_maybe_named(Some(name))
    }

    /// Converts this unnamed time zone into a time zone with the given
    /// optional name.
    #[inline]
    pub fn into_maybe_named(
        self,
        name: Option<tz::TimeZoneId>,
    ) -> MaybeNamedTimeZone {
        MaybeNamedTimeZone { name, tz: self }
    }
}

impl PartialEq for TimeZone {
    fn eq(&self, rhs: &TimeZone) -> bool {
        self.checksum == rhs.checksum
    }
}

/// A "local time type" from TZif data.
///
/// This is referenced by time zone transitions. It may be used by one or
/// more time zone transitions. This contains information about whether
/// the transition moves into DST, its time zone abbreviation, and most
/// importantly, the offset.
#[derive(Clone, Copy, Debug)]
pub struct LocalTimeType {
    /// The offset from UTC.
    pub offset: Offset,
    /// Whether the region is considered to be in daylight saving time or not.
    pub dst: Dst,
    /// An index into `TimeZone::designations` corresponding to the time zone
    /// abbreviation for this local time type.
    pub designation: u8,
    /// It's unclear to the author of Jiff what this is or what it's supposed
    /// to be used for.
    pub indicator: Indicator,
}

impl LocalTimeType {
    fn designation(&self) -> usize {
        usize::from(self.designation)
    }
}

/// The possible indicator values for standard/wall and UT/local.
///
/// Their purpose, as of 2026-07-06, is unknown to Jiff's author. But they are
/// represented here for completeness.
// Note that UT+Wall is not allowed.
//
// I honestly have no earthly clue what they mean. I've read the section about
// them in RFC 8536 several times and I can't make sense of it. I've even
// looked at data files that have these set and still can't make sense of
// them. I've even looked at what other datetime libraries do with these, and
// they all seem to just ignore them. Like, WTF. I've spent the last couple
// months of my life steeped in time, and I just cannot figure this out. Am I
// just dumb?
//
// Anyway, we parse them, but otherwise ignore them because that's what all
// the cool kids do.
//
// The default is `LocalWall`, which also occurs when no indicators are
// present.
//
// I tried again and still don't get it. Here's a dump for `Pacific/Honolulu`:
//
// ```text
// $ ./scripts/jiff-debug tzif /usr/share/zoneinfo/Pacific/Honolulu
// TIME ZONE NAME
//   /usr/share/zoneinfo/Pacific/Honolulu
// LOCAL TIME TYPES
//   000: offset=-10:31:26, is_dst=false, designation=LMT, indicator=local/wall
//   001: offset=-10:30, is_dst=false, designation=HST, indicator=local/wall
//   002: offset=-09:30, is_dst=true, designation=HDT, indicator=local/wall
//   003: offset=-09:30, is_dst=true, designation=HWT, indicator=local/wall
//   004: offset=-09:30, is_dst=true, designation=HPT, indicator=ut/std
//   005: offset=-10, is_dst=false, designation=HST, indicator=local/wall
// TRANSITIONS
//   0000: -9999-01-02T01:59:59 :: -377705023201 :: type=0, -10:31:26, is_dst=false, LMT, local/wall
//   0001: 1896-01-13T22:31:26 :: -2334101314 :: type=1, -10:30, is_dst=false, HST, local/wall
//   0002: 1933-04-30T12:30:00 :: -1157283000 :: type=2, -09:30, is_dst=true, HDT, local/wall
//   0003: 1933-05-21T21:30:00 :: -1155436200 :: type=1, -10:30, is_dst=false, HST, local/wall
//   0004: 1942-02-09T12:30:00 :: -880198200 :: type=3, -09:30, is_dst=true, HWT, local/wall
//   0005: 1945-08-14T23:00:00 :: -769395600 :: type=4, -09:30, is_dst=true, HPT, ut/std
//   0006: 1945-09-30T11:30:00 :: -765376200 :: type=1, -10:30, is_dst=false, HST, local/wall
//   0007: 1947-06-08T12:30:00 :: -712150200 :: type=5, -10, is_dst=false, HST, local/wall
// POSIX TIME ZONE STRING
//   HST10
// ```
//
// See how type 004 has a ut/std indicator? What the fuck does that mean?
// All transitions are defined in terms of UTC. I confirmed this with `zdump`:
//
// ```text
// $ zdump -v Pacific/Honolulu | rg 1945
// Pacific/Honolulu  Tue Aug 14 22:59:59 1945 UT = Tue Aug 14 13:29:59 1945 HWT isdst=1 gmtoff=-34200
// Pacific/Honolulu  Tue Aug 14 23:00:00 1945 UT = Tue Aug 14 13:30:00 1945 HPT isdst=1 gmtoff=-34200
// Pacific/Honolulu  Sun Sep 30 11:29:59 1945 UT = Sun Sep 30 01:59:59 1945 HPT isdst=1 gmtoff=-34200
// Pacific/Honolulu  Sun Sep 30 11:30:00 1945 UT = Sun Sep 30 01:00:00 1945 HST isdst=0 gmtoff=-37800
// ```
//
// The times match up. All of them. The indicators don't seem to make a
// difference. I'm clearly missing something.
#[allow(missing_docs)]
#[derive(Clone, Copy, Debug)]
pub enum Indicator {
    LocalWall,
    LocalStandard,
    UTStandard,
}

/// The set of transitions in TZif data, laid out in column orientation.
///
/// The column orientation is used to make TZ lookups faster. Specifically,
/// for finding an offset for a timestamp, we do a binary search on
/// `timestamps`. For finding an offset for a local datetime, we do a binary
/// search on `civil_starts`. By making these two distinct sequences with
/// nothing else in them, we make them as small as possible and thus improve
/// cache locality.
///
/// All sequences in this type are in correspondence with one another. They
/// are all guaranteed to have the same length.
#[derive(Clone, Debug)]
pub struct Transitions {
    /// The timestamp at which this transition begins.
    pub timestamps: MaybeStaticSlice<Timestamp>,
    /// The wall clock time for when a transition begins.
    pub civil_starts: MaybeStaticSlice<DateTime>,
    /// The wall clock time for when a transition ends.
    ///
    /// This is equivalent to the corresponding entry in `civil_starts` when
    /// the corresponding transition is neither a gap nor a fold. A transition
    /// that isn't a gap or a fold keeps the offset the same but may change
    /// something else, like the abbreviation or whether it's considered
    /// daylight saving time.
    pub civil_ends: MaybeStaticSlice<DateTime>,
    /// Any other relevant data about a transition, such as its local type
    /// index and the transition kind.
    pub infos: MaybeStaticSlice<TransitionInfo>,
}

/// TZif transition info beyond the timestamp and civil datetime.
///
/// For example, this contains a transition's "local type index," which in
/// turn gives access to the offset (among other metadata) for that transition.
#[derive(Clone, Copy, Debug)]
pub struct TransitionInfo {
    /// The index into the sequence of local time type records. This is what
    /// provides the correct offset (from UTC) that is active beginning at
    /// this transition.
    pub type_index: u8,
    /// The boundary condition for quickly determining if a given wall clock
    /// time is ambiguous (i.e., falls in a gap or a fold).
    pub kind: TransitionKind,
}

/// The kind of a transition.
///
/// This is used when trying to determine the offset for a local datetime. It
/// indicates how the corresponding civil datetimes in `civil_starts` and
/// `civil_ends` should be interpreted. That is, there are three possible
/// cases:
///
/// 1. The offset of this transition is equivalent to the offset of the
/// previous transition. That means there are no ambiguous civil datetimes
/// between the transitions. This can occur, e.g., when the time zone
/// abbreviation changes.
/// 2. The offset of the transition is greater than the offset of the previous
/// transition. That means there is a "gap" in local time between the
/// transitions. This typically corresponds to entering daylight saving time.
/// It is usually, but not always, 1 hour.
/// 3. The offset of the transition is less than the offset of the previous
/// transition. That means there is a "fold" in local time where time is
/// repeated. This typically corresponds to leaving daylight saving time. It
/// is usually, but not always, 1 hour.
///
/// # More explanation
///
/// This, when combined with `civil_starts` and `civil_ends` in
/// `Transitions`, explicitly represents ambiguous wall clock times that
/// occur at the boundaries of transitions.
///
/// The start of the wall clock time is always the earlier possible wall clock
/// time that could occur with this transition's corresponding offset. For a
/// gap, it's the previous transition's offset. For a fold, it's the current
/// transition's offset.
///
/// For example, DST for `America/New_York` began on `2024-03-10T07:00:00+00`.
/// The offset prior to this instant in time is `-05`, corresponding
/// to standard time (EST). Thus, in wall clock time, DST began at
/// `2024-03-10T02:00:00`. And since this is a DST transition that jumps ahead
/// an hour, the start of DST also corresponds to the start of a gap. That is,
/// the times `02:00:00` through `02:59:59` never appear on a clock for this
/// hour. The question is thus: which offset should we apply to `02:00:00`?
/// We could apply the offset from the earlier transition `-05` and get
/// `2024-03-10T01:00:00-05` (that's `2024-03-10T06:00:00+00`), or we could
/// apply the offset from the later transition `-04` and get
/// `2024-03-10T03:00:00-04` (that's `2024-03-10T07:00:00+00`).
///
/// So in the above, we would have a `Gap` variant where `start` (inclusive) is
/// `2024-03-10T02:00:00` and `end` (exclusive) is `2024-03-10T03:00:00`.
///
/// The fold case is the same idea, but where the same time is repeated.
/// For example, in `America/New_York`, standard time began on
/// `2024-11-03T06:00:00+00`. The offset prior to this instant in time
/// is `-04`, corresponding to DST (EDT). Thus, in wall clock time, DST
/// ended at `2024-11-03T02:00:00`. However, since this is a fold, the
/// actual set of ambiguous times begins at `2024-11-03T01:00:00` and
/// ends at `2024-11-03T01:59:59.999999999`. That is, the wall clock time
/// `2024-11-03T02:00:00` is unambiguous.
///
/// So in the fold case above, we would have a `Fold` variant where
/// `start` (inclusive) is `2024-11-03T01:00:00` and `end` (exclusive) is
/// `2024-11-03T02:00:00`.
///
/// Since this gets bundled in with the sorted sequence of transitions, we'll
/// use the "start" time in all three cases as our target of binary search.
/// Once we land on a transition, we'll know our given wall clock time is
/// greater than or equal to its start wall clock time. At that point, to
/// determine if there is ambiguity, we merely need to determine if the given
/// wall clock time is less than the corresponding `end` time. If it is, then
/// it falls in a gap or fold. Otherwise, it's unambiguous.
///
/// Note that we could compute these datetime values while searching for the
/// correct transition, but there's a fair bit of math involved in going
/// between timestamps (which is what TZif gives us) and calendar datetimes
/// (which is what we're given as input). It is also necessary that we offset
/// the timestamp given in TZif at some point, since it is in UTC and the
/// datetime given is in wall clock time. So I decided it would be worth
/// pre-computing what we need in terms of what the input is. This way, we
/// don't need to do any conversions, or indeed, any arithmetic at all, for
/// time zone lookups. We *could* store these as transitions, but then the
/// input datetime would need to be converted to a timestamp before searching
/// the transitions.
#[derive(Clone, Copy, Debug)]
pub enum TransitionKind {
    /// This transition cannot possibly lead to an unambiguous offset because
    /// its offset is equivalent to the offset of the previous transition.
    ///
    /// Has an entry in `civil_starts`, but corresponding entry in `civil_ends`
    /// is always zeroes (i.e., meaningless).
    Unambiguous,
    /// This occurs when this transition's offset is strictly greater than the
    /// previous transition's offset. This effectively results in a "gap" of
    /// time equal to the difference in the offsets between the two
    /// transitions.
    ///
    /// Has an entry in `civil_starts` for when the gap starts (inclusive) in
    /// local time. Also has an entry in `civil_ends` for when the fold ends
    /// (exclusive) in local time.
    Gap,
    /// This occurs when this transition's offset is strictly less than the
    /// previous transition's offset. This results in a "fold" of time where
    /// the two transitions have an overlap where it is ambiguous which one
    /// applies given a wall clock time. In effect, a span of time equal to the
    /// difference in the offsets is repeated.
    ///
    /// Has an entry in `civil_starts` for when the fold starts (inclusive) in
    /// local time. Also has an entry in `civil_ends` for when the fold ends
    /// (exclusive) in local time.
    Fold,
}

/// The representation for a timestamp used by the TZif implementation.
///
/// We don't use [`Timestamp`](crate::Timestamp) from the root of this crate
/// because TZif data doesn't require nanosecond resolution. Instead, this
/// representation uses only second resolution. This makes for a more compact
/// sequence of transitions, which means more data fits into cache and thus
/// faster binary search.
#[derive(Clone, Copy, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub struct Timestamp {
    second: i64,
}

impl Timestamp {
    /// The minimum timestamp value.
    pub const MIN: Timestamp = Timestamp::new(crate::Timestamp::MIN);

    /// The maximum timestamp value.
    pub const MAX: Timestamp = Timestamp::new(crate::Timestamp::MAX);

    /// The zero value for a timestamp, which also corresponds to the Unix
    /// epoch (`1970-01-01T00:00:00Z`).
    pub const UNIX_EPOCH: Timestamp =
        Timestamp::new(crate::Timestamp::UNIX_EPOCH);

    /// Creates a new TZif timestamp from jiff-core's standard timestamp type.
    ///
    /// Any fractional second is rounded down to the preceding whole second.
    /// This ensures that a timestamp just before a transition stays before
    /// it, even when the timestamp is negative.
    pub const fn new(ts: crate::Timestamp) -> Timestamp {
        let mut second = ts.as_second();
        if ts.subsec_nanosecond() < 0 {
            // This cannot underflow: the minimum timestamp has no negative
            // fractional component.
            second -= 1;
        }
        Timestamp { second }
    }

    /// Creates a new `Timestamp` from a Unix timestamp integer value.
    ///
    /// This returns an error if the value is not in the legal bounds for this
    /// type.
    pub const fn from_second(second: i64) -> Result<Timestamp, RangeError> {
        match crate::Timestamp::from_second(second) {
            Ok(ts) => Ok(Timestamp::new(ts)),
            Err(err) => Err(err),
        }
    }

    /// Returns the second value of this timestamp.
    ///
    /// It is the number of seconds since the Unix epoch
    /// (`1970-01-01T00:00:00Z`). Timestamps prior to the Unix epoch are
    /// negative. Timestamps are the Unix epoch are `0`.
    pub const fn as_second(self) -> i64 {
        self.second
    }

    /// Adds the number of seconds to this timestamp, saturating at the minimum
    /// or maximum legal value.
    const fn saturating_add(self, seconds: i64) -> Timestamp {
        let second = self.as_second() + seconds;
        if second > Timestamp::MAX.as_second() {
            Timestamp::MAX
        } else if second < Timestamp::MIN.as_second() {
            Timestamp::MIN
        } else {
            Timestamp { second }
        }
    }

    /// Converts this timestamp to a civil datetime for the offset given.
    pub const fn to_datetime(self, offset: Offset) -> DateTime {
        DateTime::new(self.to_standard_timestamp().to_datetime(offset))
    }

    /// Converts this timestamp back to the "standard" timestamp.
    ///
    /// Note that the timestamp returned here always has its nanosecond
    /// component set to `0`.
    pub const fn to_standard_timestamp(self) -> crate::Timestamp {
        // OK because we don't provide a way to construct, mutate or
        // change a `Timestamp` that drifts from the valid values of a
        // `crate::Timestamp` (for its second component).
        unwrapr!(
            crate::Timestamp::from_second(self.as_second()),
            "always in bounds"
        )
    }
}

impl core::fmt::Debug for Timestamp {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Debug::fmt(&self.to_standard_timestamp(), f)
    }
}

/// The representation for a civil datetime used by the TZif implementation.
///
/// We don't use [`civil::DateTime`] here because we specifically
/// do not need to represent fractional seconds. This lets us easily represent
/// what we need in 8 bytes.
///
/// Moreover, we pack the fields into a single `i64` to make comparisons
/// extremely cheap. This is especially useful since we do a binary search on
/// civil datetimes when determining the instant that corresponds to a civil
/// datetime.
#[derive(Clone, Copy, Eq, Hash, PartialEq, PartialOrd, Ord)]
pub struct DateTime {
    bits: i64,
}

impl DateTime {
    /// The minimum civil datetime value.
    pub const MIN: DateTime = DateTime::new(civil::DateTime::from_parts(
        civil::Date::MIN,
        civil::Time::MIN,
    ));

    /// The maximum civil datetime value.
    pub const MAX: DateTime = DateTime::new(civil::DateTime::from_parts(
        civil::Date::MAX,
        civil::Time::MAX,
    ));

    /// Creates a new TZif civil datetime from jiff-core's standard civil
    /// datetime type.
    ///
    /// Note that this completely ignores any fractional second component on
    /// the provided datetime.
    pub const fn new(dt: civil::DateTime) -> DateTime {
        let (d, t) = (dt.date(), dt.time());
        let mut bits = 0;
        bits |= (d.year() as u64) << 48;
        bits |= (d.month() as u64) << 40;
        bits |= (d.day() as u64) << 32;
        bits |= (t.hour() as u64) << 24;
        bits |= (t.minute() as u64) << 16;
        bits |= (t.second() as u64) << 8;
        // The least significant 8 bits remain 0.
        DateTime { bits: bits as i64 }
    }

    /// Returns the year component of this civil datetime.
    pub const fn year(self) -> i16 {
        (self.bits as u64 >> 48) as u16 as i16
    }

    /// Returns the month component of this civil datetime.
    pub const fn month(self) -> i8 {
        (self.bits as u64 >> 40) as u8 as i8
    }

    /// Returns the day component of this civil datetime.
    pub const fn day(self) -> i8 {
        (self.bits as u64 >> 32) as u8 as i8
    }

    /// Returns the hour component of this civil datetime.
    pub const fn hour(self) -> i8 {
        (self.bits as u64 >> 24) as u8 as i8
    }

    /// Returns the minute component of this civil datetime.
    pub const fn minute(self) -> i8 {
        (self.bits as u64 >> 16) as u8 as i8
    }

    /// Returns the second component of this civil datetime.
    pub const fn second(self) -> i8 {
        (self.bits as u64 >> 8) as u8 as i8
    }
}

/// Creates a new bit packed datetime from jiff-core's standard datetime type.
///
/// Note that this completely ignores any fractional second component on the
/// provided datetime.
impl From<civil::DateTime> for DateTime {
    fn from(dt: civil::DateTime) -> DateTime {
        DateTime::new(dt)
    }
}

impl core::fmt::Debug for DateTime {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        if !f.alternate() {
            f.debug_struct("DateTime").field("bits", &self.bits).finish()
        } else {
            f.debug_tuple("DateTime")
                .field(&format_args!(
                    "{:04}-{:02}-{:02}T{:02}:{:02}:{:02}",
                    self.year(),
                    self.month(),
                    self.day(),
                    self.hour(),
                    self.minute(),
                    self.second(),
                ))
                .finish()
        }
    }
}

/// Returns true if the data might be in TZif format.
///
/// It is possible that this returns true even if the given data is not in TZif
/// format. However, it is impossible for this to return false when the given
/// data is TZif. That is, a false positive is allowed but a false negative is
/// not.
pub fn is_possibly_tzif(data: &[u8]) -> bool {
    data.starts_with(b"TZif")
}

// If you're looking for tests, they can be found in Jiff's `tz::timezone`
// and `tz::tzif` modules.
