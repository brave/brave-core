/*!
Provides a parser for [POSIX's `TZ` environment variable][posix-env].

The `TZ` environment variable is most commonly used to set a time zone. For
example, `TZ=America/New_York`. But it can also be used to tersely define DST
transitions. Moreover, the format is not just used as an environment variable,
but is also included at the end of TZif files (version 2 or greater). The IANA
Time Zone Database project also [documents the `TZ` variable][iana-env] with
a little more commentary.

Note that we (along with pretty much everyone else) don't strictly follow
POSIX here. Namely, `TZ=America/New_York` isn't a POSIX compatible usage,
and I believe it technically should be `TZ=:America/New_York`. Nevertheless,
apparently some group of people (IANA folks?) decided `TZ=America/New_York`
should be fine. From the [IANA `theory.html` documentation][iana-env]:

> It was recognized that allowing the TZ environment variable to take on values
> such as 'America/New_York' might cause "old" programs (that expect TZ to have
> a certain form) to operate incorrectly; consideration was given to using
> some other environment variable (for example, TIMEZONE) to hold the string
> used to generate the TZif file's name. In the end, however, it was decided
> to continue using TZ: it is widely used for time zone purposes; separately
> maintaining both TZ and TIMEZONE seemed a nuisance; and systems where "new"
> forms of TZ might cause problems can simply use legacy TZ values such as
> "EST5EDT" which can be used by "new" programs as well as by "old" programs
> that assume pre-POSIX TZ values.

Indeed, even [musl subscribes to this behavior][musl-env]. So that's what we do
here too.

Note that a POSIX time zone like `EST5` corresponds to the UTC offset `-05:00`,
and `GMT-4` corresponds to the UTC offset `+04:00`. Yes, it's backwards. How
fun.

# IANA v3+ Support

While this module and many of its types are directly associated with POSIX,
this module also plays a supporting role for `TZ` strings in the IANA TZif
binary format for versions 2 and greater. Specifically, for versions 3 and
greater, some minor extensions are supported here via `IanaTz::parse`. But
using `PosixTz::parse` is limited to parsing what is specified by POSIX.
Nevertheless, we generally use `IanaTz::parse` everywhere, even when parsing
the `TZ` environment variable. The reason for this is that it seems to be what
other programs do in practice (for example, GNU date).

# `no-std` and `no-alloc` support

This module works just fine in `no_std` mode. It also generally works fine
without `alloc` too, modulo some APIs for parsing from an environment variable
(which need `std` anyway). The main problem is that the type defined here takes
up a lot of space (100+ bytes). A good chunk of that comes from representing
time zone abbreviations inline. In theory, only 6-10 bytes are needed for
simple cases like `TZ=EST5EDT,M3.2.0,M11.1.0`, but we make room for 30 byte
length abbreviations (times two). Plus, there's a much of room made for the
rule representation.

When you then stuff this inside a `TimeZone` which cannot use heap allocation
to force an indirection, you wind up with a very chunky `TimeZone`. And this in
turn makes `Zoned` itself quite chunky.

So while there isn't actually any particular reason why a
`ReasonablePosixTimeZone` cannot be used in core-only environments, we don't
include it in Jiff for now because it seems like bad juju to make `TimeZone`
so big. So if you do need POSIX time zone support in core-only environments,
please open an issue.

My guess is that `Zoned` is itself kind of doomed in core-only environments.
It's just too hard to bundle an entire time zone with every instant without
using the heap to amortize copies of the time zone definition. I've been
thinking about adding an `Unzoned` type that is just like `Zoned`, but requires
the caller to pass in a `&TimeZone` for every API call. Less convenient for
sure, but you get a more flexible type.

[posix-env]: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html#tag_08_03
[iana-env]: https://data.iana.org/time-zones/tzdb-2024a/theory.html#functions
[musl-env]: https://wiki.musl-libc.org/environment-variables
*/

use core::cell::Cell;

use crate::{
    civil::{Date, DateTime, Time, Weekday},
    error::{err, Error, ErrorContext},
    timestamp::Timestamp,
    tz::{AmbiguousOffset, Dst, Offset, TimeZoneTransition},
    util::{
        array_str::Abbreviation,
        escape::{Byte, Bytes},
        parse,
        rangeint::{ri16, ri8, RFrom, RInto},
        t::{self, Minute, Month, Second, Sign, SpanZoneOffset, Year, C},
    },
    SignedDuration,
};

/// POSIX says the hour must be in the range `0..=24`, but that the default
/// hour for DST is one hour more than standard time. Therefore, the actual
/// allowed range is `0..=25`. (Although we require `0..=24` during parsing.)
type PosixHour = ri8<0, 25>;
type IanaHour = ri16<0, 167>;
type PosixJulianDayNoLeap = ri16<1, 365>;
type PosixJulianDayWithLeap = ri16<0, 365>;
type PosixWeek = ri8<1, 5>;

/// The result of parsing the POSIX `TZ` environment variable.
///
/// A `TZ` variable can either be a time zone string with an optional DST
/// transition rule, or it can begin with a `:` followed by an arbitrary set of
/// bytes that is implementation defined.
///
/// In practice, the content following a `:` is treated as an IANA time zone
/// name. Moreover, even if the `TZ` string doesn't start with a `:` but
/// corresponds to a IANA time zone name, then it is interpreted as such.
/// (See the module docs.) However, this type only encapsulates the choices
/// strictly provided by POSIX: either a time zone string with an optional DST
/// transition rule, or an implementation defined string with a `:` prefix. If,
/// for example, `TZ="America/New_York"`, then that case isn't encapsulated by
/// this type. Callers needing that functionality will need to handle the error
/// returned by parsing this type and layer their own semantics on top.
#[cfg(feature = "tz-system")]
#[derive(Debug, Eq, PartialEq)]
pub(crate) enum PosixTz {
    /// A valid POSIX time zone with an optional DST transition rule.
    Rule(PosixTimeZone),
    /// An implementation defined string. This occurs when the `TZ` value
    /// starts with a `:`. The string returned here does not include the `:`.
    Implementation(alloc::boxed::Box<str>),
}

#[cfg(feature = "tz-system")]
impl PosixTz {
    /// Parse a POSIX `TZ` environment variable string from the given bytes.
    pub(crate) fn parse(bytes: impl AsRef<[u8]>) -> Result<PosixTz, Error> {
        let bytes = bytes.as_ref();
        if bytes.get(0) == Some(&b':') {
            let Ok(string) = core::str::from_utf8(&bytes[1..]) else {
                return Err(err!(
                    "POSIX time zone string with a ':' prefix contains \
                     invalid UTF-8: {:?}",
                    Bytes(&bytes[1..]),
                ));
            };
            Ok(PosixTz::Implementation(string.into()))
        } else {
            PosixTimeZone::parse(bytes).map(PosixTz::Rule)
        }
    }

    /// Parse a POSIX `TZ` environment variable string from the given `OsStr`.
    pub(crate) fn parse_os_str(
        osstr: impl AsRef<std::ffi::OsStr>,
    ) -> Result<PosixTz, Error> {
        PosixTz::parse(parse::os_str_bytes(osstr.as_ref())?)
    }
}

#[cfg(feature = "tz-system")]
impl core::fmt::Display for PosixTz {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match *self {
            PosixTz::Rule(ref tz) => write!(f, "{tz}"),
            PosixTz::Implementation(ref imp) => write!(f, ":{imp}"),
        }
    }
}

/// The result of parsing a V2 or V3+ `TZ` string from IANA `tzfile` data.
///
/// A V2 `TZ` string is precisely identical to a POSIX `TZ` environment
/// variable string. A V3 `TZ` string however supports signed DST transition
/// times, and hours in the range `0..=167`.
///
/// We also specifically require that IANA `TZ` strings are "reasonable." That
/// is, if DST exists, then it must have a corresponding rule.
#[derive(Debug, Eq, PartialEq)]
pub(crate) struct IanaTz(ReasonablePosixTimeZone);

impl IanaTz {
    /// Parse a IANA tzfile v3+ `TZ` string from the given bytes.
    pub(crate) fn parse_v3plus(
        bytes: impl AsRef<[u8]>,
    ) -> Result<IanaTz, Error> {
        let bytes = bytes.as_ref();
        let posix_tz = PosixTimeZone::parse(bytes).map_err(|e| {
            e.context(err!("invalid POSIX TZ string {:?}", Bytes(bytes)))
        })?;
        let Ok(reasonable) = posix_tz.reasonable() else {
            return Err(err!(
                "TZ string {:?} in v3+ tzfile has DST but no transition rules",
                Bytes(bytes),
            ));
        };
        Ok(IanaTz(reasonable))
    }

    /// Like `parse_v3plus`, but parses a POSIX TZ string from a prefix of the
    /// given input. And remaining input is returned.
    pub(crate) fn parse_v3plus_prefix<'b, B: AsRef<[u8]> + ?Sized + 'b>(
        bytes: &'b B,
    ) -> Result<(IanaTz, &'b [u8]), Error> {
        let bytes = bytes.as_ref();
        let (posix_tz, remaining) = PosixTimeZone::parse_prefix(bytes)
            .map_err(|e| {
                e.context(err!("invalid POSIX TZ string {:?}", Bytes(bytes)))
            })?;
        let Ok(reasonable) = posix_tz.reasonable() else {
            return Err(err!(
                "TZ string {:?} in v3+ tzfile has DST but no transition rules",
                Bytes(bytes),
            ));
        };
        Ok((IanaTz(reasonable), remaining))
    }

    /// Return ownership of the underlying "reasonable" POSIX time zone value.
    ///
    /// If this was parsed as an IANA v3+ `TZ` string, then the DST transition
    /// rules can extend beyond `00:00:00..=24:59:59`, and instead are in the
    /// range `-167:59:59..=167:59:59`.
    pub(crate) fn into_tz(self) -> ReasonablePosixTimeZone {
        self.0
    }
}

impl core::fmt::Display for IanaTz {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{}", self.0)
    }
}

/// A "reasonable" POSIX time zone.
///
/// This is the same as a regular POSIX time zone, but requires that if a DST
/// time zone abbreviation is present, then a transition rule must also be
/// present for it. In other words, this considers a `TZ` string of `EST5EDT`
/// as unreasonable because it doesn't say *when* the DST transitions should
/// occur.
///
/// Generally speaking, we only deal with reasonable POSIX time zones. And
/// we expect `TZ` strings parsed from IANA v2+ formatted `tzfile`s to also
/// be reasonable or parsing fails. This also seems to be consistent with the
/// [GNU C Library]'s treatment of the `TZ` variable: it only documents support
/// for reasonable POSIX time zone strings.
///
/// [GNU C Library]: https://www.gnu.org/software/libc/manual/2.25/html_node/TZ-Variable.html
#[derive(Clone, Debug, Eq, PartialEq)]
pub(crate) struct ReasonablePosixTimeZone {
    std_abbrev: Abbreviation,
    std_offset: PosixOffset,
    dst: Option<ReasonablePosixDst>,
}

impl ReasonablePosixTimeZone {
    /// Returns the appropriate time zone offset to use for the given
    /// timestamp.
    ///
    /// This also includes whether the offset returned should be considered
    /// to be "DST" or not, along with the time zone abbreviation (e.g., EST
    /// for standard time in New York, and EDT for DST in New York).
    pub(crate) fn to_offset(
        &self,
        timestamp: Timestamp,
    ) -> (Offset, Dst, &str) {
        if self.dst.is_none() {
            return (self.std_offset(), Dst::No, self.std_abbrev.as_str());
        }

        let dt = Offset::UTC.to_datetime(timestamp);
        self.dst_info_utc(dt.date().year_ranged())
            .filter(|dst_info| dst_info.in_dst(dt))
            .map(|dst_info| {
                (dst_info.offset, Dst::Yes, dst_info.dst.abbrev.as_str())
            })
            .unwrap_or_else(|| {
                (self.std_offset(), Dst::No, self.std_abbrev.as_str())
            })
    }

    /// Returns a possibly ambiguous timestamp for the given civil datetime.
    ///
    /// The given datetime should correspond to the "wall" clock time of what
    /// humans use to tell time for this time zone.
    ///
    /// Note that "ambiguous timestamp" is represented by the possible
    /// selection of offsets that could be applied to the given datetime. In
    /// general, it is only ambiguous around transitions to-and-from DST. The
    /// ambiguity can arise as a "fold" (when a particular wall clock time is
    /// repeated) or as a "gap" (when a particular wall clock time is skipped
    /// entirely).
    pub(crate) fn to_ambiguous_kind(&self, dt: DateTime) -> AmbiguousOffset {
        let year = dt.date().year_ranged();
        let std_offset = self.std_offset();
        let Some(dst_info) = self.dst_info_wall(year) else {
            return AmbiguousOffset::Unambiguous { offset: std_offset };
        };
        let diff = dst_info.offset - std_offset;
        // When the difference between DST and standard is positive, that means
        // STD->DST results in a gap while DST->STD results in a fold. However,
        // when the difference is negative, that means STD->DST results in a
        // fold while DST->STD results in a gap. The former is by far the most
        // common. The latter is a bit weird, but real cases do exist. For
        // example, Dublin has DST in winter (UTC+01) and STD in the summer
        // (UTC+00).
        //
        // When the difference is zero, then we have a weird POSIX time zone
        // where a DST transition rule was specified, but was set to explicitly
        // be the same as STD. In this case, there can be no ambiguity. (The
        // zero case is strictly redundant. Both the diff < 0 and diff > 0
        // cases handle the zero case correctly. But we write it out for
        // clarity.)
        if diff.get_seconds_ranged() == 0 {
            debug_assert_eq!(std_offset, dst_info.offset);
            AmbiguousOffset::Unambiguous { offset: std_offset }
        } else if diff.is_negative() {
            // For DST transitions that always move behind one hour, ambiguous
            // timestamps only occur when the given civil datetime falls in the
            // standard time range.
            if dst_info.in_dst(dt) {
                AmbiguousOffset::Unambiguous { offset: dst_info.offset }
            } else {
                let fold_start = dst_info.start.saturating_add(diff);
                let gap_end = dst_info.end.saturating_sub(diff);
                if fold_start <= dt && dt < dst_info.start {
                    AmbiguousOffset::Fold {
                        before: std_offset,
                        after: dst_info.offset,
                    }
                } else if dst_info.end <= dt && dt < gap_end {
                    AmbiguousOffset::Gap {
                        before: dst_info.offset,
                        after: std_offset,
                    }
                } else {
                    AmbiguousOffset::Unambiguous { offset: std_offset }
                }
            }
        } else {
            // For DST transitions that always move ahead one hour, ambiguous
            // timestamps only occur when the given civil datetime falls in the
            // DST range.
            if !dst_info.in_dst(dt) {
                AmbiguousOffset::Unambiguous { offset: std_offset }
            } else {
                // PERF: I wonder if it makes sense to pre-compute these?
                // Probably not, because we have to do it based on year of
                // datetime given. But if we ever add a "caching" layer for
                // POSIX time zones, then it might be worth adding these to it.
                let gap_end = dst_info.start.saturating_add(diff);
                let fold_start = dst_info.end.saturating_sub(diff);
                if dst_info.start <= dt && dt < gap_end {
                    AmbiguousOffset::Gap {
                        before: std_offset,
                        after: dst_info.offset,
                    }
                } else if fold_start <= dt && dt < dst_info.end {
                    AmbiguousOffset::Fold {
                        before: dst_info.offset,
                        after: std_offset,
                    }
                } else {
                    AmbiguousOffset::Unambiguous { offset: dst_info.offset }
                }
            }
        }
    }

    /// Returns the timestamp of the most recent time zone transition prior
    /// to the timestamp given. If one doesn't exist, `None` is returned.
    pub(crate) fn previous_transition(
        &self,
        timestamp: Timestamp,
    ) -> Option<TimeZoneTransition> {
        let dt = Offset::UTC.to_datetime(timestamp);
        let dst_info = self.dst_info_utc(dt.date().year_ranged())?;
        let (earlier, later) = dst_info.ordered();
        let (prev, dst_info) = if dt > later {
            (later, dst_info)
        } else if dt > earlier {
            (earlier, dst_info)
        } else {
            let prev_year = dt.date().year_ranged().checked_sub(C(1))?;
            let dst_info = self.dst_info_utc(prev_year)?;
            let (_, later) = dst_info.ordered();
            (later, dst_info)
        };

        let timestamp = Offset::UTC.to_timestamp(prev).ok()?;
        let dt = Offset::UTC.to_datetime(timestamp);
        let (offset, abbrev, dst) = if dst_info.in_dst(dt) {
            (dst_info.offset, dst_info.dst.abbrev.as_str(), Dst::Yes)
        } else {
            (self.std_offset(), self.std_abbrev.as_str(), Dst::No)
        };
        Some(TimeZoneTransition { timestamp, offset, abbrev, dst })
    }

    /// Returns the timestamp of the soonest time zone transition after the
    /// timestamp given. If one doesn't exist, `None` is returned.
    pub(crate) fn next_transition(
        &self,
        timestamp: Timestamp,
    ) -> Option<TimeZoneTransition> {
        let dt = Offset::UTC.to_datetime(timestamp);
        let dst_info = self.dst_info_utc(dt.date().year_ranged())?;
        let (earlier, later) = dst_info.ordered();
        let (next, dst_info) = if dt < earlier {
            (earlier, dst_info)
        } else if dt < later {
            (later, dst_info)
        } else {
            let next_year = dt.date().year_ranged().checked_add(C(1))?;
            let dst_info = self.dst_info_utc(next_year)?;
            let (earlier, _) = dst_info.ordered();
            (earlier, dst_info)
        };

        let timestamp = Offset::UTC.to_timestamp(next).ok()?;
        let dt = Offset::UTC.to_datetime(timestamp);
        let (offset, abbrev, dst) = if dst_info.in_dst(dt) {
            (dst_info.offset, dst_info.dst.abbrev.as_str(), Dst::Yes)
        } else {
            (self.std_offset(), self.std_abbrev.as_str(), Dst::No)
        };
        Some(TimeZoneTransition { timestamp, offset, abbrev, dst })
    }

    /// Returns the offset for standard time in this POSIX time zone.
    fn std_offset(&self) -> Offset {
        self.std_offset.to_offset()
    }

    /// Returns the range in which DST occurs.
    ///
    /// The civil datetimes returned are in UTC. This is useful for determining
    /// whether a timestamp is in DST or not.
    fn dst_info_utc(&self, year: impl RInto<Year>) -> Option<DstInfo<'_>> {
        let year = year.rinto();
        let dst = self.dst.as_ref()?;
        let std_offset = self.std_offset.to_offset();
        let dst_offset = dst.posix_offset(&self.std_offset).to_offset();
        // DST time starts with respect to standard time, so offset it by the
        // standard offset.
        let start = dst.rule.start.to_datetime(year, std_offset);
        // DST time ends with respect to DST time, so offset it by the DST
        // offset.
        let end = dst.rule.end.to_datetime(year, dst_offset);
        Some(DstInfo { dst, offset: dst_offset, start, end })
    }

    /// Returns the range in which DST occurs.
    ///
    /// The civil datetimes returned are in "wall clock time." That is, they
    /// represent the transitions as they are seen from humans reading a clock
    /// within the geographic location of that time zone.
    fn dst_info_wall(&self, year: impl RInto<Year>) -> Option<DstInfo<'_>> {
        let year = year.rinto();
        let dst = self.dst.as_ref()?;
        let dst_offset = dst.posix_offset(&self.std_offset).to_offset();
        // POSIX time zones express their DST transitions in terms of wall
        // clock time. Since this method specifically is returning wall
        // clock times, we don't want to offset our datetimes at all.
        let start = dst.rule.start.to_datetime(year, Offset::ZERO);
        let end = dst.rule.end.to_datetime(year, Offset::ZERO);
        Some(DstInfo { dst, offset: dst_offset, start, end })
    }

    /// Returns the DST transition rule. This panics if this time zone doesn't
    /// have DST.
    #[cfg(test)]
    fn rule(&self) -> Rule {
        self.dst.as_ref().unwrap().rule
    }
}

impl core::fmt::Display for ReasonablePosixTimeZone {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(
            f,
            "{}{}",
            AbbreviationDisplay(self.std_abbrev),
            self.std_offset
        )?;
        if let Some(ref dst) = self.dst {
            write!(f, "{dst}")?;
        }
        Ok(())
    }
}

/// The daylight saving time (DST) info for a POSIX time zone in a particular
/// year.
#[derive(Debug, Eq, PartialEq)]
struct DstInfo<'a> {
    /// The DST transition rule that generated this info.
    dst: &'a ReasonablePosixDst,
    /// The DST offset.
    ///
    /// This is the same as `ReasonablePosixDst::offset`, but accounts for
    /// its default value (when it isn't given, 1 hour ahead of standard time)
    /// and is converted to a Jiff data type that we can use in arithmetic.
    offset: Offset,
    /// The start time (inclusive) that DST begins.
    ///
    /// Note that this may be greater than `end`. This tends to happen in the
    /// southern hemisphere.
    ///
    /// Note also that this may be in UTC or in wall clock civil time.
    /// It depends on whether `ReasonablePosixTimeZone::dst_info_utc` or
    /// `ReasonablePosixTimeZone::dst_info_wall` was used.
    start: DateTime,
    /// The end time (exclusive) that DST ends.
    ///
    /// Note that this may be less than `start`. This tends to happen in the
    /// southern hemisphere.
    ///
    /// Note also that this may be in UTC or in wall clock civil time.
    /// It depends on whether `ReasonablePosixTimeZone::dst_info_utc` or
    /// `ReasonablePosixTimeZone::dst_info_wall` was used.
    end: DateTime,
}

impl<'a> DstInfo<'a> {
    /// Returns true if and only if the given civil datetime ought to be
    /// considered in DST.
    fn in_dst(&self, utc_dt: DateTime) -> bool {
        if self.start <= self.end {
            self.start <= utc_dt && utc_dt < self.end
        } else {
            !(self.end <= utc_dt && utc_dt < self.start)
        }
    }

    /// Returns the earlier and later times for this DST info.
    fn ordered(&self) -> (DateTime, DateTime) {
        if self.start <= self.end {
            (self.start, self.end)
        } else {
            (self.end, self.start)
        }
    }
}

/// A "reasonable" DST transition rule.
///
/// Unlike what POSIX specifies, this requires a rule.
#[derive(Clone, Debug, Eq, PartialEq)]
struct ReasonablePosixDst {
    abbrev: Abbreviation,
    offset: Option<PosixOffset>,
    /// This is the principal change. A "reasonable" DST must include a rule.
    rule: Rule,
}

impl ReasonablePosixDst {
    /// Returns the offset for this DST zone.
    ///
    /// If one wasn't explicity given, then this returns an offset equivalent
    /// to one hour east of the given standard time offset.
    fn posix_offset(&self, std_offset: &PosixOffset) -> PosixOffset {
        if let Some(ref offset) = self.offset {
            return *offset;
        }
        // When no offset is specified, we default to an offset one hour
        // east for DST. We subtract one because POSIX time offsets are
        // backwards.
        PosixOffset {
            hour: std_offset.hour - (std_offset.sign() * C(1)),
            ..*std_offset
        }
    }
}

impl core::fmt::Display for ReasonablePosixDst {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{}", AbbreviationDisplay(self.abbrev))?;
        if let Some(offset) = self.offset {
            write!(f, "{offset}")?;
        }
        write!(f, ",{}", self.rule)?;
        Ok(())
    }
}

/// A POSIX time zone.
#[derive(Debug, Eq, PartialEq)]
pub(crate) struct PosixTimeZone {
    std_abbrev: Abbreviation,
    std_offset: PosixOffset,
    dst: Option<PosixDst>,
}

impl PosixTimeZone {
    /// Parse a POSIX `TZ` environment variable, assuming it's a rule and not
    /// an implementation defined value, from the given bytes.
    fn parse(bytes: impl AsRef<[u8]>) -> Result<PosixTimeZone, Error> {
        // We enable the IANA v3+ extensions here. (Namely, that the time
        // specification hour value has the range `-167..=167` instead of
        // `0..=24`.) Requiring strict POSIX rules doesn't seem necessary
        // since the extension is a strict superset. Plus, GNU tooling
        // seems to accept the extension.
        let parser =
            Parser { ianav3plus: true, ..Parser::new(bytes.as_ref()) };
        parser.parse()
    }

    /// Like parse, but parses a prefix of the input given and returns whatever
    /// is remaining.
    fn parse_prefix<'b, B: AsRef<[u8]> + ?Sized + 'b>(
        bytes: &'b B,
    ) -> Result<(PosixTimeZone, &'b [u8]), Error> {
        let parser =
            Parser { ianav3plus: true, ..Parser::new(bytes.as_ref()) };
        parser.parse_prefix()
    }

    /// Transforms this POSIX time zone into a "reasonable" time zone.
    ///
    /// If this isn't a reasonable time zone, then the original time zone is
    /// returned unchanged.
    ///
    /// A POSIX time zone is reasonable when, if it has DST, then it must also
    /// have a rule declaring when DST starts and ends. A POSIX time zone
    /// without DST at all is always reasonable.
    pub(crate) fn reasonable(
        mut self,
    ) -> Result<ReasonablePosixTimeZone, PosixTimeZone> {
        if let Some(mut dst) = self.dst.take() {
            if let Some(rule) = dst.rule.take() {
                Ok(ReasonablePosixTimeZone {
                    std_abbrev: self.std_abbrev,
                    std_offset: self.std_offset,
                    dst: Some(ReasonablePosixDst {
                        abbrev: dst.abbrev,
                        offset: dst.offset,
                        rule,
                    }),
                })
            } else {
                // This is the main problematic case: the time zone declares
                // that DST exists, but gives us no information about when
                // it starts and ends.
                Err(PosixTimeZone { dst: Some(dst), ..self })
            }
        } else {
            // This is still reasonable even though there's no rule because
            // there's no DST at all. So no rule is required for this time
            // zone to be reasonable.
            Ok(ReasonablePosixTimeZone {
                std_abbrev: self.std_abbrev,
                std_offset: self.std_offset,
                dst: None,
            })
        }
    }
}

impl core::fmt::Display for PosixTimeZone {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(
            f,
            "{}{}",
            AbbreviationDisplay(self.std_abbrev),
            self.std_offset
        )?;
        if let Some(ref dst) = self.dst {
            write!(f, "{dst}")?;
        }
        Ok(())
    }
}

/// The daylight-saving-time abbreviation, offset and rule for this time zone.
#[derive(Debug, Eq, PartialEq)]
struct PosixDst {
    abbrev: Abbreviation,
    offset: Option<PosixOffset>,
    rule: Option<Rule>,
}

impl core::fmt::Display for PosixDst {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{}", AbbreviationDisplay(self.abbrev))?;
        if let Some(offset) = self.offset {
            write!(f, "{offset}")?;
        }
        if let Some(rule) = self.rule {
            write!(f, ",{rule}")?;
        }
        Ok(())
    }
}

/// The offset from UTC for standard-time or daylight-saving-time for this
/// time zone.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct PosixOffset {
    sign: Option<Sign>,
    hour: PosixHour,
    minute: Option<Minute>,
    second: Option<Second>,
}

impl PosixOffset {
    /// Converts this offset to the standard time zone offset used in this
    /// crate. This flips the underlying sign to follow `jiff::tz::Offset`'s
    /// convention (which is the convention that pretty much everyon, except
    /// of course POSIX, uses).
    ///
    /// The sign is flipped because POSIX time zones use negative offsets for
    /// zones east of the prime meridian. But the much more common convention
    /// nowadays is to use negative offsets for zones west of the prime
    /// meridian. In other words, a POSIX time zone like `EST5` corresponds to
    /// an offset of `-05:00:00`.
    fn to_offset(&self) -> Offset {
        let sign = SpanZoneOffset::rfrom(-self.sign());
        let hour = SpanZoneOffset::rfrom(self.hour);
        let minute =
            SpanZoneOffset::rfrom(self.minute.unwrap_or(C(0).rinto()));
        let second =
            SpanZoneOffset::rfrom(self.second.unwrap_or(C(0).rinto()));
        let seconds = (hour * t::SECONDS_PER_HOUR)
            + (minute * t::SECONDS_PER_MINUTE)
            + second;
        Offset::from_seconds_ranged(sign * seconds)
    }

    /// Returns the sign for this offset, defaulting to positive if one wasn't
    /// explicitly given.
    ///
    /// Note that the sign for a POSIX offset is backwards. EST5, for example,
    /// corresponds to UTC-05.
    fn sign(&self) -> Sign {
        self.sign.unwrap_or(Sign::N::<1>())
    }
}

impl core::fmt::Display for PosixOffset {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        if let Some(sign) = self.sign {
            if sign < 0 {
                write!(f, "-")?;
            } else {
                write!(f, "+")?;
            }
        }
        write!(f, "{}", self.hour)?;
        if let Some(minute) = self.minute {
            write!(f, ":{minute:02}")?;
            if let Some(second) = self.second {
                write!(f, ":{second:02}")?;
            }
        }
        Ok(())
    }
}

/// The rule for when a DST transition starts and ends.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct Rule {
    start: PosixDateTimeSpec,
    end: PosixDateTimeSpec,
}

impl core::fmt::Display for Rule {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{},{}", self.start, self.end)
    }
}

/// A specification for the day and an optional time at which a DST
/// transition occurs.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct PosixDateTimeSpec {
    date: PosixDateSpec,
    time: Option<PosixTimeSpec>,
}

impl PosixDateTimeSpec {
    /// Turns this POSIX datetime spec into a civil datetime in the year given
    /// with the given offset. The datetimes returned are offset by the given
    /// offset. For wall clock time, an offset of `0` should be given. For
    /// UTC time, the offset (standard or DST) corresponding to this time
    /// spec should be given.
    ///
    /// The datetime returned is guaranteed to have a year component equal
    /// to the year given. This guarantee is upheld even when the datetime
    /// specification (combined with the offset) would extend past the end of
    /// the year (or before the start of the year). In this case, the maximal
    /// (or minimal) datetime for the given year is returned.
    fn to_datetime(&self, year: impl RInto<Year>, offset: Offset) -> DateTime {
        let year = year.rinto();
        let mkmin = || {
            Date::new_ranged(year, C(1), C(1)).unwrap().to_datetime(Time::MIN)
        };
        let mkmax = || {
            Date::new_ranged(year, C(12), C(31))
                .unwrap()
                .to_datetime(Time::MAX)
        };

        let Some(date) = self.date.to_civil_date(year) else { return mkmax() };
        let mut dt = date.to_datetime(Time::MIN);
        let dur_transition = self.time().to_duration();
        let dur_offset = SignedDuration::from(offset);
        dt = dt.checked_add(dur_transition).unwrap_or_else(|_| {
            if dur_transition.is_negative() {
                mkmin()
            } else {
                mkmax()
            }
        });
        dt = dt.checked_sub(dur_offset).unwrap_or_else(|_| {
            if dur_transition.is_negative() {
                mkmax()
            } else {
                mkmin()
            }
        });
        if dt.date().year() < year {
            mkmin()
        } else if dt.date().year() > year {
            mkmax()
        } else {
            dt
        }
    }

    /// Returns the time for this spec, falling back to the default of 2:00:00
    /// as specified by POSIX.
    fn time(self) -> PosixTimeSpec {
        const DEFAULT: PosixTimeSpec = PosixTimeSpec {
            sign: None,
            hour: IanaHour::N::<2>(),
            minute: None,
            second: None,
        };
        self.time.unwrap_or(DEFAULT)
    }
}

impl core::fmt::Display for PosixDateTimeSpec {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(f, "{}", self.date)?;
        if let Some(time) = self.time {
            write!(f, "/{time}")?;
        }
        Ok(())
    }
}

/// A specification for the day at which a DST transition occurs.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum PosixDateSpec {
    /// POSIX says:
    ///
    /// > The Julian day n (`1 <= n <= 365`). Leap days shall not be counted.
    /// > That is, in all years-including leap years-February 28 is day 59
    /// > and March 1 is day 60. It is impossible to refer explicitly to the
    /// > occasional February 29.
    JulianOne(PosixJulianDayNoLeap),
    /// POSIX says:
    ///
    /// > The zero-based Julian day (`0 <= n <= 365`). Leap days shall be
    /// > counted, and it is possible to refer to February 29.
    JulianZero(PosixJulianDayWithLeap),
    /// The nth weekday of a particular month.
    WeekdayOfMonth(WeekdayOfMonth),
}

impl PosixDateSpec {
    /// Convert this date specification to a civil date in the year given.
    ///
    /// If this date specification couldn't be turned into a date in the year
    /// given, then `None` is returned. This happens when `366` is given as
    /// a day, but the year given is not a leap year. In this case, callers may
    /// want to assume a datetime that is maximal for the year given.
    fn to_civil_date(&self, year: impl RInto<Year>) -> Option<Date> {
        match *self {
            PosixDateSpec::JulianOne(day) => {
                let first = Date::new_ranged(year, C(1), C(1)).unwrap();
                // Parsing validates that our day is 1-365 which will always
                // succeed for all possible year values. That is, every valid
                // year has a December 31.
                Some(
                    first
                        .with()
                        .day_of_year_no_leap(day.get())
                        .build()
                        .expect("Julian 'J day' should be in bounds"),
                )
            }
            PosixDateSpec::JulianZero(day) => {
                let first = Date::new_ranged(year, C(1), C(1)).unwrap();
                // OK because our value for `day` is validated to be `0..=365`,
                // and since it is an `i16`, it is always valid to add 1.
                let off1 = day.get().checked_add(1).unwrap();
                // While `off1` is guaranteed to be in `1..=366`, it is
                // possible that `366` is invalid. In this case, we throw
                // our hands up, and ask the caller to make a decision for
                // how to deal with it. Why does POSIX go out of its way to
                // specifically not specify behavior in error cases?
                first.with().day_of_year(off1).build().ok()
            }
            PosixDateSpec::WeekdayOfMonth(wom) => {
                let first = Date::new_ranged(year, wom.month, C(1)).unwrap();
                // This is maybe non-obvious, but this will always succeed
                // because it can only fail when the week number is one of {-5,
                // 0, 5}. Since we've validated that 'wom.week' is in 1..=5, we
                // know it can't be 0. Moreover, `wom.week()` never returns `5`
                // since `5` actually means "last weekday of month." That is,
                // `wom.week()` is guaranteed to return -1 or 1..=4.
                //
                // Also, I looked at how other libraries deal with this case,
                // and almost all of them just do a bunch of inline hairy
                // arithmetic here. I suppose I could be reduced to such
                // things if perf called for it, but we have a nice civil date
                // abstraction. So use it, god damn it.
                let week = wom.week();
                debug_assert!(week == -1 || (1..=4).contains(&week));
                Some(
                    first
                        .nth_weekday_of_month(week, wom.weekday)
                        .expect("nth weekday always exists"),
                )
            }
        }
    }
}

impl core::fmt::Display for PosixDateSpec {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match *self {
            PosixDateSpec::JulianOne(n) => write!(f, "J{n}"),
            PosixDateSpec::JulianZero(n) => write!(f, "{n}"),
            PosixDateSpec::WeekdayOfMonth(wk) => write!(f, "{wk}"),
        }
    }
}

/// A specification for the day of the month at which a DST transition occurs.
/// POSIX says:
///
/// > The `d`'th day (`0 <= d <= 6`) of week `n` of month `m` of the year (`1
/// > <= n <= 5`, `1 <= m <= 12`, where week `5` means "the last `d` day in
/// > month `m`" which may occur in either the fourth or the fifth week). Week
/// > `1` is the first week in which the `d`'th day occurs. Day zero is Sunday.
///
/// The interesting thing to note here (or my interpretation anyway), is that
/// a week of `4` means the "4th weekday in a month" where as a week of `5`
/// means the "last weekday in a month, even if it's the 4th weekday."
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct WeekdayOfMonth {
    month: Month,
    week: PosixWeek,
    weekday: Weekday,
}

impl WeekdayOfMonth {
    /// Returns the week number.
    ///
    /// This converts a week number of `5` to `-1`, which more sensible
    /// represents the "last week of the month."
    fn week(&self) -> i8 {
        if self.week == 5 {
            -1
        } else {
            self.week.get()
        }
    }
}

impl core::fmt::Display for WeekdayOfMonth {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        write!(
            f,
            "M{month}.{week}.{weekday}",
            month = self.month,
            week = self.week,
            weekday = self.weekday.to_sunday_zero_offset(),
        )
    }
}

/// A specification for "time" in a POSIX time zone, with optional minute and
/// second components.
///
/// Note that this is more of a duration than a "time." While POSIX limits the
/// hour range to `0..=24` (and therefore looks _almost_ like a time), the
/// IANA tzfile v3+ format expands the range to `-167..=167`.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
struct PosixTimeSpec {
    sign: Option<Sign>,
    /// The hour component of this time specification. When IANA V3+ parsing
    /// is enabled, then this can be in any value in the range `0..=167`. But
    /// otherwise, it is limited to `0..=24`.
    hour: IanaHour,
    minute: Option<Minute>,
    second: Option<Second>,
}

impl PosixTimeSpec {
    /// Returns this time specification as a duration of time.
    fn to_duration(&self) -> SignedDuration {
        let sign = i64::from(self.sign());
        let hour = sign * i64::from(self.hour);
        let minute = sign * i64::from(self.minute.unwrap_or(C(0).rinto()));
        let second = sign * i64::from(self.second.unwrap_or(C(0).rinto()));
        SignedDuration::from_secs(second + (60 * minute) + (60 * 60 * hour))
    }

    /// Returns the sign for this time sepc, defaulting to positive if one
    /// wasn't explicitly given.
    fn sign(&self) -> Sign {
        self.sign.unwrap_or(Sign::N::<1>())
    }
}

impl core::fmt::Display for PosixTimeSpec {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        if let Some(sign) = self.sign {
            if sign < 0 {
                write!(f, "-")?;
            } else {
                write!(f, "+")?;
            }
        }
        write!(f, "{}", self.hour)?;
        if let Some(minute) = self.minute {
            write!(f, ":{minute:02}")?;
            if let Some(second) = self.second {
                write!(f, ":{second:02}")?;
            }
        }
        Ok(())
    }
}

#[derive(Debug)]
struct Parser<'s> {
    /// The `TZ` string that we're parsing.
    tz: &'s [u8],
    /// The parser's current position in `tz`.
    pos: Cell<usize>,
    /// Whether to use IANA rules, i.e., when parsing a TZ string in a TZif
    /// file of version 3 or greater. From `tzfile(5)`:
    ///
    /// > First, the hours part of its transition times may be signed and range
    /// > from `-167` through `167` instead of the POSIX-required unsigned
    /// > values from `0` through `24`. Second, DST is in effect all year if
    /// > it starts January 1 at 00:00 and ends December 31 at 24:00 plus the
    /// > difference between daylight saving and standard time.
    ///
    /// At time of writing, I don't think I understand the significance of
    /// the second part above. (RFC 8536 elaborates that it is meant to be an
    /// explicit clarification of something that POSIX itself implies.) But the
    /// first part is clear: it permits the hours to be a bigger range.
    ianav3plus: bool,
}

impl<'s> Parser<'s> {
    fn new<B: ?Sized + AsRef<[u8]>>(tz: &'s B) -> Parser<'s> {
        Parser { tz: tz.as_ref(), pos: Cell::new(0), ianav3plus: false }
    }

    /// Parses a POSIX time zone from the current position of the parser and
    /// ensures that the entire TZ string corresponds to a single valid POSIX
    /// time zone.
    fn parse(&self) -> Result<PosixTimeZone, Error> {
        let (time_zone, remaining) = self.parse_prefix()?;
        if !remaining.is_empty() {
            return Err(err!(
                "expected entire TZ string to be a valid POSIX \
                 time zone, but found '{}' after what would otherwise \
                 be a valid POSIX TZ string",
                Bytes(remaining),
            ));
        }
        Ok(time_zone)
    }

    /// Parses a POSIX time zone from the current position of the parser and
    /// returns the remaining input.
    fn parse_prefix(&self) -> Result<(PosixTimeZone, &'s [u8]), Error> {
        let time_zone = self.parse_posix_time_zone()?;
        Ok((time_zone, self.remaining()))
    }

    /// Parse a POSIX time zone from the current position of the parser.
    ///
    /// Upon success, the parser will be positioned immediately following the
    /// TZ string.
    fn parse_posix_time_zone(&self) -> Result<PosixTimeZone, Error> {
        let std_abbrev = self
            .parse_abbreviation()
            .map_err(|e| e.context("failed to parse standard abbreviation"))?;
        let std_offset = self
            .parse_posix_offset()
            .map_err(|e| e.context("failed to parse standard offset"))?;
        let mut dst = None;
        if !self.is_done()
            && (self.byte().is_ascii_alphabetic() || self.byte() == b'<')
        {
            dst = Some(self.parse_posix_dst()?);
        }
        Ok(PosixTimeZone { std_abbrev, std_offset, dst })
    }

    /// Parse a DST zone with an optional explicit transition rule.
    ///
    /// This assumes the parser is positioned at the first byte of the DST
    /// abbreviation.
    ///
    /// Upon success, the parser will be positioned immediately after the end
    /// of the DST transition rule (which might just be the abbreviation, but
    /// might also include explicit start/end datetime specifications).
    fn parse_posix_dst(&self) -> Result<PosixDst, Error> {
        let abbrev = self
            .parse_abbreviation()
            .map_err(|e| e.context("failed to parse DST abbreviation"))?;
        let mut dst = PosixDst { abbrev, offset: None, rule: None };
        if self.is_done() {
            return Ok(dst);
        }
        if self.byte() != b',' {
            dst.offset = Some(
                self.parse_posix_offset()
                    .map_err(|e| e.context("failed to parse DST offset"))?,
            );
            if self.is_done() {
                return Ok(dst);
            }
        }
        if self.byte() != b',' {
            return Err(err!(
                "after parsing DST offset in POSIX time zone string, \
                 found '{}' but expected a ','",
                Byte(self.byte()),
            ));
        }
        if !self.bump() {
            return Err(err!(
                "after parsing DST offset in POSIX time zone string, \
                 found end of string after a trailing ','",
            ));
        }
        dst.rule = Some(self.parse_rule()?);
        Ok(dst)
    }

    /// Parse a time zone abbreviation.
    ///
    /// This assumes the parser is positioned at the first byte of the
    /// abbreviation. This is either the first character in the abbreviation,
    /// or the opening quote of a quoted abbreviation.
    ///
    /// Upon success, the parser will be positioned immediately following the
    /// abbreviation name.
    fn parse_abbreviation(&self) -> Result<Abbreviation, Error> {
        if self.byte() == b'<' {
            if !self.bump() {
                return Err(err!(
                    "found opening '<' quote for abbreviation in \
                     POSIX time zone string, and expected a name \
                     following it, but found the end of string instead"
                ));
            }
            self.parse_quoted_abbreviation()
        } else {
            self.parse_unquoted_abbreviation()
        }
    }

    /// Parses an unquoted time zone abbreviation.
    ///
    /// This assumes the parser is position at the first byte in the
    /// abbreviation.
    ///
    /// Upon success, the parser will be positioned immediately after the
    /// last byte in the abbreviation.
    fn parse_unquoted_abbreviation(&self) -> Result<Abbreviation, Error> {
        let start = self.pos();
        for i in 0.. {
            if !self.byte().is_ascii_alphabetic() {
                break;
            }
            if i >= Abbreviation::capacity() {
                return Err(err!(
                    "expected abbreviation with at most {} bytes, \
                     but found a longer abbreviation beginning with '{}'",
                    Abbreviation::capacity(),
                    Bytes(&self.tz[start..i]),
                ));
            }
            if !self.bump() {
                break;
            }
        }
        let end = self.pos();
        let abbrev =
            core::str::from_utf8(&self.tz[start..end]).map_err(|_| {
                // NOTE: I believe this error is technically impossible since
                // the loop above restricts letters in an abbreviation to
                // ASCII. So everything from `start` to `end` is ASCII and
                // thus should be UTF-8. But it doesn't cost us anything to
                // report an error here in case the code above evolves somehow.
                err!(
                    "found abbreviation '{}', but it is not valid UTF-8",
                    Bytes(&self.tz[start..end]),
                )
            })?;
        if abbrev.len() < 3 {
            return Err(err!(
                "expected abbreviation with 3 or more bytes, but found \
                 abbreviation {:?} with {} bytes",
                abbrev,
                abbrev.len(),
            ));
        }
        // OK because we verified above that the abbreviation
        // does not exceed `Abbreviation::capacity`.
        Ok(Abbreviation::new(abbrev).unwrap())
    }

    /// Parses a quoted time zone abbreviation.
    ///
    /// This assumes the parser is positioned immediately after the opening
    /// `<` quote. That is, at the first byte in the abbreviation.
    ///
    /// Upon success, the parser will be positioned immediately after the
    /// closing `>` quote.
    fn parse_quoted_abbreviation(&self) -> Result<Abbreviation, Error> {
        let start = self.pos();
        for i in 0.. {
            if !self.byte().is_ascii_alphanumeric()
                && self.byte() != b'+'
                && self.byte() != b'-'
            {
                break;
            }
            if i >= Abbreviation::capacity() {
                return Err(err!(
                    "expected abbreviation with at most {} bytes, \
                     but found a longer abbreviation beginning with '{}'",
                    Abbreviation::capacity(),
                    Bytes(&self.tz[start..i]),
                ));
            }
            if !self.bump() {
                break;
            }
        }
        let end = self.pos();
        let abbrev =
            core::str::from_utf8(&self.tz[start..end]).map_err(|_| {
                // NOTE: I believe this error is technically impossible since
                // the loop above restricts letters in an abbreviation to
                // ASCII. So everything from `start` to `end` is ASCII and
                // thus should be UTF-8. But it doesn't cost us anything to
                // report an error here in case the code above evolves somehow.
                err!(
                    "found abbreviation '{}', but it is not valid UTF-8",
                    Bytes(&self.tz[start..end]),
                )
            })?;
        if self.is_done() {
            return Err(err!(
                "found non-empty quoted abbreviation {abbrev:?}, but \
                 did not find expected end-of-quoted abbreviation \
                 '>' character",
            ));
        }
        if self.byte() != b'>' {
            return Err(err!(
                "found non-empty quoted abbreviation {abbrev:?}, but \
                 found '{}' instead of end-of-quoted abbreviation '>' \
                 character",
                Byte(self.byte()),
            ));
        }
        self.bump();
        if abbrev.len() < 3 {
            return Err(err!(
                "expected abbreviation with 3 or more bytes, but found \
                 abbreviation {abbrev:?} with {} bytes",
                abbrev.len(),
            ));
        }
        // OK because we verified above that the abbreviation
        // does not exceed `Abbreviation::capacity()`.
        Ok(Abbreviation::new(abbrev).unwrap())
    }

    /// Parse a POSIX time offset.
    ///
    /// This assumes the parser is positioned at the first byte of the offset.
    /// This can either be a digit (for a positive offset) or the sign of the
    /// offset (which must be either `-` or `+`).
    ///
    /// Upon success, the parser will be positioned immediately after the end
    /// of the offset.
    fn parse_posix_offset(&self) -> Result<PosixOffset, Error> {
        let sign = self.parse_optional_sign().map_err(|e| {
            e.context(
                "failed to parse sign for time offset \
                 in POSIX time zone string",
            )
        })?;
        let hour = self.parse_hour_posix()?;
        let offset = PosixOffset { sign, hour, minute: None, second: None };
        if self.maybe_byte() != Some(b':') {
            return Ok(offset);
        }
        if !self.bump() {
            return Err(err!(
                "incomplete time in POSIX timezone (missing minutes)",
            ));
        }
        let minute = Some(self.parse_minute()?);
        if self.maybe_byte() != Some(b':') {
            return Ok(PosixOffset { sign, hour, minute, second: None });
        }
        if !self.bump() {
            return Err(err!(
                "incomplete time in POSIX timezone (missing seconds)",
            ));
        }
        let second = Some(self.parse_second()?);
        Ok(PosixOffset { sign, hour, minute, second })
    }

    /// Parses a POSIX DST transition rule.
    ///
    /// This assumes the parser is positioned at the first byte in the rule.
    /// That is, it comes immediately after the DST abbreviation or its
    /// optional offset.
    ///
    /// Upon success, the parser will be positioned immediately after the
    /// DST transition rule. In typical cases, this corresponds to the end of
    /// the TZ string.
    fn parse_rule(&self) -> Result<Rule, Error> {
        let start = self.parse_posix_datetime_spec().map_err(|e| {
            e.context("failed to parse start of DST transition rule")
        })?;
        if self.maybe_byte() != Some(b',') || !self.bump() {
            return Err(err!(
                "expected end of DST rule after parsing the start \
                 of the DST rule"
            ));
        }
        let end = self.parse_posix_datetime_spec().map_err(|e| {
            e.context("failed to parse end of DST transition rule")
        })?;
        Ok(Rule { start, end })
    }

    /// Parses a POSIX datetime specification.
    ///
    /// This assumes the parser is position at the first byte where a datetime
    /// specification is expected to occur.
    ///
    /// Upon success, the parser will be positioned after the datetime
    /// specification. This will either be immediately after the date, or if
    /// it's present, the time part of the specification.
    fn parse_posix_datetime_spec(&self) -> Result<PosixDateTimeSpec, Error> {
        let date = self.parse_posix_date_spec()?;
        let mut spec = PosixDateTimeSpec { date, time: None };
        if self.maybe_byte() != Some(b'/') {
            return Ok(spec);
        }
        if !self.bump() {
            return Err(err!(
                "expected time specification after '/' following a date
                 specification in a POSIX time zone DST transition rule",
            ));
        }
        spec.time = Some(self.parse_posix_time_spec()?);
        Ok(spec)
    }

    /// Parses a POSIX date specification.
    ///
    /// This assumes the parser is positioned at the first byte of the date
    /// specification. This can be `J` (for one based Julian day without leap
    /// days), `M` (for "weekday of month") or a digit starting the zero based
    /// Julian day with leap days. This routine will validate that the position
    /// points to one of these possible values. That is, the caller doesn't
    /// need to parse the `M` or the `J` or the leading digit. The caller
    /// should just call this routine when it *expect* a date specification to
    /// follow.
    ///
    /// Upon success, the parser will be positioned immediately after the date
    /// specification.
    fn parse_posix_date_spec(&self) -> Result<PosixDateSpec, Error> {
        match self.byte() {
            b'J' => {
                if !self.bump() {
                    return Err(err!(
                        "expected one-based Julian day after 'J' in date \
                         specification of a POSIX time zone DST transition \
                         rule, but got the end of the string instead"
                    ));
                }
                Ok(PosixDateSpec::JulianOne(
                    self.parse_posix_julian_day_no_leap()?,
                ))
            }
            b'0'..=b'9' => Ok(PosixDateSpec::JulianZero(
                self.parse_posix_julian_day_with_leap()?,
            )),
            b'M' => {
                if !self.bump() {
                    return Err(err!(
                        "expected month-week-weekday after 'M' in date \
                         specification of a POSIX time zone DST transition \
                         rule, but got the end of the string instead"
                    ));
                }
                Ok(PosixDateSpec::WeekdayOfMonth(
                    self.parse_weekday_of_month()?,
                ))
            }
            _ => Err(err!(
                "expected 'J', a digit or 'M' at the beginning of a date \
                 specification of a POSIX time zone DST transition rule, \
                 but got '{}' instead",
                Byte(self.byte()),
            )),
        }
    }

    /// Parses a POSIX Julian day that does not include leap days
    /// (`1 <= n <= 365`).
    ///
    /// This assumes the parser is positioned just after the `J` and at the
    /// first digit of the Julian day. Upon success, the parser will be
    /// positioned immediately following the day number.
    fn parse_posix_julian_day_no_leap(
        &self,
    ) -> Result<PosixJulianDayNoLeap, Error> {
        let number = self
            .parse_number_with_upto_n_digits(3)
            .map_err(|e| e.context("invalid one based Julian day"))?;
        let day = PosixJulianDayNoLeap::new(number).ok_or_else(|| {
            err!("invalid one based Julian day (must be in range 1..=365")
        })?;
        Ok(day)
    }

    /// Parses a POSIX Julian day that includes leap days (`0 <= n <= 365`).
    ///
    /// This assumes the parser is positioned at the first digit of the Julian
    /// day. Upon success, the parser will be positioned immediately following
    /// the day number.
    fn parse_posix_julian_day_with_leap(
        &self,
    ) -> Result<PosixJulianDayWithLeap, Error> {
        let number = self
            .parse_number_with_upto_n_digits(3)
            .map_err(|e| e.context("invalid zero based Julian day"))?;
        let day = PosixJulianDayWithLeap::new(number).ok_or_else(|| {
            err!("invalid zero based Julian day (must be in range 0..=365")
        })?;
        Ok(day)
    }

    /// Parses a POSIX "weekday of month" specification.
    ///
    /// This assumes the parser is positioned just after the `M` byte and
    /// at the first digit of the month. Upon success, the parser will be
    /// positioned immediately following the "weekday of the month" that was
    /// parsed.
    fn parse_weekday_of_month(&self) -> Result<WeekdayOfMonth, Error> {
        let month = self.parse_month()?;
        if self.maybe_byte() != Some(b'.') {
            return Err(err!(
                "expected '.' after month '{month}' in POSIX time zone rule"
            ));
        }
        if !self.bump() {
            return Err(err!(
                "expected week after month '{month}' in POSIX time zone rule"
            ));
        }
        let week = self.parse_week()?;
        if self.maybe_byte() != Some(b'.') {
            return Err(err!(
                "expected '.' after week '{week}' in POSIX time zone rule"
            ));
        }
        if !self.bump() {
            return Err(err!(
                "expected day-of-week after week '{week}' in \
                 POSIX time zone rule"
            ));
        }
        let weekday = self.parse_weekday()?;
        Ok(WeekdayOfMonth { month, week, weekday })
    }

    /// This parses a POSIX time specification in the format
    /// `[+/-]hh?[:mm[:ss]]`.
    ///
    /// This assumes the parser is positioned at the first `h` (or the sign,
    /// if present). Upon success, the parser will be positioned immediately
    /// following the end of the time specification.
    fn parse_posix_time_spec(&self) -> Result<PosixTimeSpec, Error> {
        let (sign, hour) = if self.ianav3plus {
            let sign = self.parse_optional_sign().map_err(|e| {
                e.context(
                    "failed to parse sign for transition time \
                     in POSIX time zone string",
                )
            })?;
            let hour = self.parse_hour_ianav3plus()?;
            (sign, hour)
        } else {
            (None, self.parse_hour_posix()?.rinto())
        };
        let spec = PosixTimeSpec { sign, hour, minute: None, second: None };
        if self.maybe_byte() != Some(b':') {
            return Ok(spec);
        }
        if !self.bump() {
            return Err(err!(
                "incomplete transition time in \
                 POSIX time zone string (missing minutes)",
            ));
        }
        let minute = Some(self.parse_minute()?);
        if self.maybe_byte() != Some(b':') {
            return Ok(PosixTimeSpec { sign, hour, minute, second: None });
        }
        if !self.bump() {
            return Err(err!(
                "incomplete transition time in \
                 POSIX time zone string (missing seconds)",
            ));
        }
        let second = Some(self.parse_second()?);
        Ok(PosixTimeSpec { sign, hour, minute, second })
    }

    /// Parses a month.
    ///
    /// This is expected to be positioned at the first digit. Upon success,
    /// the parser will be positioned after the month (which may contain two
    /// digits).
    fn parse_month(&self) -> Result<Month, Error> {
        let number = self.parse_number_with_upto_n_digits(2)?;
        let month = Month::new(number).ok_or_else(|| {
            err!("month in POSIX time zone must be in range 1..=12")
        })?;
        Ok(month)
    }

    /// Parses a week-of-month number.
    ///
    /// This is expected to be positioned at the first digit. Upon success,
    /// the parser will be positioned after the week digit.
    fn parse_week(&self) -> Result<PosixWeek, Error> {
        let number = self.parse_number_with_exactly_n_digits(1)?;
        let week = PosixWeek::new(number).ok_or_else(|| {
            err!("week in POSIX time zone must be in range 1..=5")
        })?;
        Ok(week)
    }

    /// Parses a week-of-month number.
    ///
    /// This is expected to be positioned at the first digit. Upon success,
    /// the parser will be positioned after the week digit.
    fn parse_weekday(&self) -> Result<Weekday, Error> {
        let number = self.parse_number_with_exactly_n_digits(1)?;
        let number8 = i8::try_from(number).map_err(|_| {
            err!(
                "weekday '{number}' in POSIX time zone \
                 does not fit into 8-bit integer"
            )
        })?;
        let weekday =
            Weekday::from_sunday_zero_offset(number8).map_err(|_| {
                err!(
                    "weekday in POSIX time zone must be in range 0..=6 \
                     (with 0 corresponding to Sunday), but got {number8}",
                )
            })?;
        Ok(weekday)
    }

    /// Parses an hour from a POSIX time specification with the IANA v3+
    /// extension. That is, the hour may be in the range `0..=167`. (Callers
    /// should parse an optional sign preceding the hour digits when IANA V3+
    /// parsing is enabled.)
    ///
    /// The hour is allowed to be a single digit (unlike minutes or seconds).
    ///
    /// This assumes the parser is positioned at the position where the first
    /// hour digit should occur. Upon success, the parser will be positioned
    /// immediately after the last hour digit.
    fn parse_hour_ianav3plus(&self) -> Result<IanaHour, Error> {
        // Callers should only be using this method when IANA v3+ parsing is
        // enabled.
        assert!(self.ianav3plus);
        let number = self
            .parse_number_with_upto_n_digits(3)
            .map_err(|e| e.context("invalid hour digits"))?;
        let hour = IanaHour::new(number).ok_or_else(|| {
            err!(
                "hour in POSIX (IANA v3+ style) \
                     time zone must be in range -167..=167"
            )
        })?;
        Ok(hour)
    }

    /// Parses an hour from a POSIX time specification, with the allowed range
    /// being `0..=24`.
    ///
    /// The hour is allowed to be a single digit (unlike minutes or seconds).
    ///
    /// This assumes the parser is positioned at the position where the first
    /// hour digit should occur. Upon success, the parser will be positioned
    /// immediately after the last hour digit.
    fn parse_hour_posix(&self) -> Result<PosixHour, Error> {
        type PosixHour24 = ri8<0, 24>;

        let number = self
            .parse_number_with_upto_n_digits(2)
            .map_err(|e| e.context("invalid hour digits"))?;
        let hour = PosixHour24::new(number).ok_or_else(|| {
            err!("hour in POSIX time zone must be in range 0..=24")
        })?;
        Ok(hour.rinto())
    }

    /// Parses a minute from a POSIX time specification.
    ///
    /// The minute must be exactly two digits.
    ///
    /// This assumes the parser is positioned at the position where the first
    /// minute digit should occur. Upon success, the parser will be positioned
    /// immediately after the second minute digit.
    fn parse_minute(&self) -> Result<Minute, Error> {
        let number = self
            .parse_number_with_exactly_n_digits(2)
            .map_err(|e| e.context("invalid minute digits"))?;
        let minute = Minute::new(number).ok_or_else(|| {
            err!("minute in POSIX time zone must be in range 0..=59")
        })?;
        Ok(minute)
    }

    /// Parses a second from a POSIX time specification.
    ///
    /// The second must be exactly two digits.
    ///
    /// This assumes the parser is positioned at the position where the first
    /// second digit should occur. Upon success, the parser will be positioned
    /// immediately after the second second digit.
    fn parse_second(&self) -> Result<Second, Error> {
        let number = self
            .parse_number_with_exactly_n_digits(2)
            .map_err(|e| e.context("invalid second digits"))?;
        let second = Second::new(number).ok_or_else(|| {
            err!("second in POSIX time zone must be in range 0..=59")
        })?;
        Ok(second)
    }

    /// Parses a signed 64-bit integer expressed in exactly `n` digits.
    ///
    /// If `n` digits could not be found (or if the `TZ` string ends before
    /// `n` digits could be found), then this returns an error.
    ///
    /// This assumes that `n >= 1` and that the parser is positioned at the
    /// first digit. Upon success, the parser is positioned immediately after
    /// the `n`th digit.
    fn parse_number_with_exactly_n_digits(
        &self,
        n: usize,
    ) -> Result<i64, Error> {
        assert!(n >= 1, "numbers must have at least 1 digit");
        let start = self.pos();
        for i in 0..n {
            if self.is_done() {
                return Err(err!("expected {n} digits, but found {i}"));
            }
            if !self.byte().is_ascii_digit() {
                return Err(err!("invalid digit '{}'", Byte(self.byte())));
            }
            self.bump();
        }
        let end = self.pos();
        parse::i64(&self.tz[start..end])
    }

    /// Parses a signed 64-bit integer expressed with up to `n` digits and at
    /// least 1 digit.
    ///
    /// This assumes that `n >= 1` and that the parser is positioned at the
    /// first digit. Upon success, the parser is position immediately after the
    /// last digit (which can be at most `n`).
    fn parse_number_with_upto_n_digits(&self, n: usize) -> Result<i64, Error> {
        assert!(n >= 1, "numbers must have at least 1 digit");
        let start = self.pos();
        for _ in 0..n {
            if self.is_done() || !self.byte().is_ascii_digit() {
                break;
            }
            self.bump();
        }
        let end = self.pos();
        parse::i64(&self.tz[start..end])
    }

    /// Parses an optional sign.
    ///
    /// This assumes the parser is positioned at the position where a positive
    /// or negative sign is permitted. If one exists, then it is consumed and
    /// returned. Moreover, if one exists, then this guarantees that it is not
    /// the last byte in the input. That is, upon success, it is valid to call
    /// `self.byte()`.
    fn parse_optional_sign(&self) -> Result<Option<Sign>, Error> {
        if self.is_done() {
            return Ok(None);
        }
        Ok(match self.byte() {
            b'-' => {
                if !self.bump() {
                    return Err(err!(
                        "expected digit after '-' sign, but got end of input",
                    ));
                }
                Some(Sign::N::<-1>())
            }
            b'+' => {
                if !self.bump() {
                    return Err(err!(
                        "expected digit after '+' sign, but got end of input",
                    ));
                }
                Some(Sign::N::<1>())
            }
            _ => None,
        })
    }
}

/// Helper routines for parsing a POSIX `TZ` string.
impl<'s> Parser<'s> {
    /// Bump the parser to the next byte.
    ///
    /// If the end of the input has been reached, then `false` is returned.
    fn bump(&self) -> bool {
        if self.is_done() {
            return false;
        }
        self.pos.set(
            self.pos().checked_add(1).expect("pos cannot overflow usize"),
        );
        !self.is_done()
    }

    /// Returns true if the next call to `bump` would return false.
    fn is_done(&self) -> bool {
        self.pos() == self.tz.len()
    }

    /// Return the byte at the current position of the parser.
    ///
    /// This panics if the parser is positioned at the end of the TZ string.
    fn byte(&self) -> u8 {
        self.tz[self.pos()]
    }

    /// Return the byte at the current position of the parser. If the TZ string
    /// has been exhausted, then this returns `None`.
    fn maybe_byte(&self) -> Option<u8> {
        self.tz.get(self.pos()).copied()
    }

    /// Return the current byte offset of the parser.
    ///
    /// The offset starts at `0` from the beginning of the TZ string.
    fn pos(&self) -> usize {
        self.pos.get()
    }

    /// Returns the remaining bytes of the TZ string.
    ///
    /// This includes `self.byte()`. It may be empty.
    fn remaining(&self) -> &'s [u8] {
        &self.tz[self.pos()..]
    }
}

/// A helper type for formatting a time zone abbreviation.
///
/// Basically, this will write the `<` and `>` quotes if necessary, and
/// otherwise write out the abbreviation in its unquoted form.
#[derive(Debug)]
struct AbbreviationDisplay(Abbreviation);

impl core::fmt::Display for AbbreviationDisplay {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        let s = self.0.as_str();
        if s.chars().any(|ch| ch == '+' || ch == '-') {
            write!(f, "<{s}>")
        } else {
            write!(f, "{s}")
        }
    }
}

// Note that most of the tests below are for the parsing. For the actual time
// zone transition logic, that's unit tested in tz/mod.rs.
#[cfg(test)]
mod tests {
    use std::string::ToString;

    use crate::civil::date;

    use super::*;

    fn reasonable_posix_time_zone(
        input: impl AsRef<[u8]>,
    ) -> ReasonablePosixTimeZone {
        let input = core::str::from_utf8(input.as_ref()).unwrap();
        let tz = IanaTz::parse_v3plus(input).unwrap().into_tz();
        // While we're here, assert that converting the TZ back
        // to a string matches what we got. This isn't guaranteed
        // in all cases, but good enough for what we test I think.
        assert_eq!(tz.to_string(), input);
        tz
    }

    /// DEBUG COMMAND
    ///
    /// Takes environment variable `JIFF_DEBUG_POSIX_TZ` as input, and prints
    /// the Rust (extended) debug representation of it after parsing it as a
    /// POSIX TZ string.
    #[cfg(feature = "std")]
    #[test]
    fn debug_posix_tz() -> anyhow::Result<()> {
        const ENV: &str = "JIFF_DEBUG_POSIX_TZ";
        let Some(val) = std::env::var_os(ENV) else { return Ok(()) };
        let val = val
            .to_str()
            .ok_or_else(|| err!("{ENV} contains invalid UTF-8"))?;
        let tz = Parser::new(val).parse()?;
        std::eprintln!("{tz:#?}");
        Ok(())
    }

    /// DEBUG COMMAND
    ///
    /// Takes environment variable `JIFF_DEBUG_IANA_TZ` as input, and prints
    /// the Rust (extended) debug representation of it after parsing it as a
    /// POSIX TZ string with IANA tzfile v3+ extensions.
    #[cfg(feature = "std")]
    #[test]
    fn debug_iana_tz() -> anyhow::Result<()> {
        const ENV: &str = "JIFF_DEBUG_IANA_TZ";
        let Some(val) = std::env::var_os(ENV) else { return Ok(()) };
        let val = val
            .to_str()
            .ok_or_else(|| err!("{ENV} contains invalid UTF-8"))?;
        let tz = Parser { ianav3plus: true, ..Parser::new(val) }.parse()?;
        std::eprintln!("{tz:#?}");
        Ok(())
    }

    #[test]
    fn reasonable_to_dst_civil_datetime_utc_range() {
        let tz = reasonable_posix_time_zone("WART4WARST,J1/-3,J365/20");
        let dst_info = DstInfo {
            // We test this in other places. It's too annoying to write this
            // out here, and I didn't adopt snapshot testing until I had
            // written out these tests by hand. ¯\_(ツ)_/¯
            dst: tz.dst.as_ref().unwrap(),
            offset: crate::tz::offset(-3),
            start: date(2024, 1, 1).at(1, 0, 0, 0),
            end: date(2024, 12, 31).at(23, 0, 0, 0),
        };
        assert_eq!(tz.dst_info_utc(C(2024)), Some(dst_info));

        let tz = reasonable_posix_time_zone("WART4WARST,J1/-4,J365/21");
        let dst_info = DstInfo {
            dst: tz.dst.as_ref().unwrap(),
            offset: crate::tz::offset(-3),
            start: date(2024, 1, 1).at(0, 0, 0, 0),
            end: date(2024, 12, 31).at(23, 59, 59, 999_999_999),
        };
        assert_eq!(tz.dst_info_utc(C(2024)), Some(dst_info));

        let tz = reasonable_posix_time_zone("EST5EDT,M3.2.0,M11.1.0");
        let dst_info = DstInfo {
            dst: tz.dst.as_ref().unwrap(),
            offset: crate::tz::offset(-4),
            start: date(2024, 3, 10).at(7, 0, 0, 0),
            end: date(2024, 11, 3).at(6, 0, 0, 0),
        };
        assert_eq!(tz.dst_info_utc(C(2024)), Some(dst_info));
    }

    #[test]
    fn reasonable() {
        assert!(PosixTimeZone::parse("EST5").unwrap().reasonable().is_ok());
        assert!(PosixTimeZone::parse("EST5EDT")
            .unwrap()
            .reasonable()
            .is_err());
        assert!(PosixTimeZone::parse("EST5EDT,J1,J365")
            .unwrap()
            .reasonable()
            .is_ok());

        let tz = reasonable_posix_time_zone("EST24EDT,J1,J365");
        assert_eq!(
            tz,
            ReasonablePosixTimeZone {
                std_abbrev: "EST".into(),
                std_offset: PosixOffset {
                    sign: None,
                    hour: C(24).rinto(),
                    minute: None,
                    second: None,
                },
                dst: Some(ReasonablePosixDst {
                    abbrev: "EDT".into(),
                    offset: None,
                    rule: Rule {
                        start: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(1).rinto()),
                            time: None,
                        },
                        end: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(365).rinto()),
                            time: None,
                        },
                    },
                }),
            },
        );

        let tz = reasonable_posix_time_zone("EST-24EDT,J1,J365");
        assert_eq!(
            tz,
            ReasonablePosixTimeZone {
                std_abbrev: "EST".into(),
                std_offset: PosixOffset {
                    sign: Some(C(-1).rinto()),
                    hour: C(24).rinto(),
                    minute: None,
                    second: None,
                },
                dst: Some(ReasonablePosixDst {
                    abbrev: "EDT".into(),
                    offset: None,
                    rule: Rule {
                        start: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(1).rinto()),
                            time: None,
                        },
                        end: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(365).rinto()),
                            time: None,
                        },
                    },
                }),
            },
        );
    }

    #[test]
    fn posix_date_time_spec_to_datetime() {
        // For this test, we just keep the offset to zero to simplify things
        // a bit. We get coverage for non-zero offsets in higher level tests.
        let to_datetime = |spec: &PosixDateTimeSpec, year: i16| {
            let year = Year::new(year).unwrap();
            spec.to_datetime(year, crate::tz::offset(0))
        };

        let tz = reasonable_posix_time_zone("EST5EDT,J1,J365/5:12:34");
        assert_eq!(
            to_datetime(&tz.rule().start, 2023),
            date(2023, 1, 1).at(2, 0, 0, 0),
        );
        assert_eq!(
            to_datetime(&tz.rule().end, 2023),
            date(2023, 12, 31).at(5, 12, 34, 0),
        );

        let tz = reasonable_posix_time_zone("EST+5EDT,M3.2.0/2,M11.1.0/2");
        assert_eq!(
            to_datetime(&tz.rule().start, 2024),
            date(2024, 3, 10).at(2, 0, 0, 0),
        );
        assert_eq!(
            to_datetime(&tz.rule().end, 2024),
            date(2024, 11, 3).at(2, 0, 0, 0),
        );

        let tz = reasonable_posix_time_zone("EST+5EDT,M1.1.1,M12.5.2");
        assert_eq!(
            to_datetime(&tz.rule().start, 2024),
            date(2024, 1, 1).at(2, 0, 0, 0),
        );
        assert_eq!(
            to_datetime(&tz.rule().end, 2024),
            date(2024, 12, 31).at(2, 0, 0, 0),
        );

        let tz = reasonable_posix_time_zone("EST5EDT,0/0,J365/25");
        assert_eq!(
            to_datetime(&tz.rule().start, 2024),
            date(2024, 1, 1).at(0, 0, 0, 0),
        );
        assert_eq!(
            to_datetime(&tz.rule().end, 2024),
            date(2024, 12, 31).at(23, 59, 59, 999_999_999),
        );

        let tz = reasonable_posix_time_zone("XXX3EDT4,0/0,J365/23");
        assert_eq!(
            to_datetime(&tz.rule().start, 2024),
            date(2024, 1, 1).at(0, 0, 0, 0),
        );
        assert_eq!(
            to_datetime(&tz.rule().end, 2024),
            date(2024, 12, 31).at(23, 0, 0, 0),
        );

        let tz = reasonable_posix_time_zone("XXX3EDT4,0/0,365");
        assert_eq!(
            to_datetime(&tz.rule().end, 2023),
            date(2023, 12, 31).at(23, 59, 59, 999_999_999),
        );
        assert_eq!(
            to_datetime(&tz.rule().end, 2024),
            date(2024, 12, 31).at(2, 0, 0, 0),
        );

        let tz = reasonable_posix_time_zone(
            "XXX3EDT4,J1/-167:59:59,J365/167:59:59",
        );
        assert_eq!(
            to_datetime(&tz.rule().start, 2024),
            date(2024, 1, 1).at(0, 0, 0, 0),
        );
        assert_eq!(
            to_datetime(&tz.rule().end, 2024),
            date(2024, 12, 31).at(23, 59, 59, 999_999_999),
        );
    }

    #[test]
    fn posix_date_time_spec_time() {
        let tz = reasonable_posix_time_zone("EST5EDT,J1,J365/5:12:34");
        assert_eq!(
            tz.rule().start.time(),
            PosixTimeSpec {
                sign: None,
                hour: C(2).rinto(),
                minute: None,
                second: None,
            },
        );
        assert_eq!(
            tz.rule().end.time(),
            PosixTimeSpec {
                sign: None,
                hour: C(5).rinto(),
                minute: Some(C(12).rinto()),
                second: Some(C(34).rinto()),
            },
        );
    }

    #[test]
    fn posix_date_spec_to_date() {
        let tz = reasonable_posix_time_zone("EST+5EDT,M3.2.0/2,M11.1.0/2");
        let start = tz.rule().start.date.to_civil_date(C(2023));
        assert_eq!(start, Some(date(2023, 3, 12)));
        let end = tz.rule().end.date.to_civil_date(C(2023));
        assert_eq!(end, Some(date(2023, 11, 5)));
        let start = tz.rule().start.date.to_civil_date(C(2024));
        assert_eq!(start, Some(date(2024, 3, 10)));
        let end = tz.rule().end.date.to_civil_date(C(2024));
        assert_eq!(end, Some(date(2024, 11, 3)));

        let tz = reasonable_posix_time_zone("EST+5EDT,J60,J365");
        let start = tz.rule().start.date.to_civil_date(C(2023));
        assert_eq!(start, Some(date(2023, 3, 1)));
        let end = tz.rule().end.date.to_civil_date(C(2023));
        assert_eq!(end, Some(date(2023, 12, 31)));
        let start = tz.rule().start.date.to_civil_date(C(2024));
        assert_eq!(start, Some(date(2024, 3, 1)));
        let end = tz.rule().end.date.to_civil_date(C(2024));
        assert_eq!(end, Some(date(2024, 12, 31)));

        let tz = reasonable_posix_time_zone("EST+5EDT,59,365");
        let start = tz.rule().start.date.to_civil_date(C(2023));
        assert_eq!(start, Some(date(2023, 3, 1)));
        let end = tz.rule().end.date.to_civil_date(C(2023));
        assert_eq!(end, None);
        let start = tz.rule().start.date.to_civil_date(C(2024));
        assert_eq!(start, Some(date(2024, 2, 29)));
        let end = tz.rule().end.date.to_civil_date(C(2024));
        assert_eq!(end, Some(date(2024, 12, 31)));

        let tz = reasonable_posix_time_zone("EST+5EDT,M1.1.1,M12.5.2");
        let start = tz.rule().start.date.to_civil_date(C(2024));
        assert_eq!(start, Some(date(2024, 1, 1)));
        let end = tz.rule().end.date.to_civil_date(C(2024));
        assert_eq!(end, Some(date(2024, 12, 31)));
    }

    #[test]
    fn posix_time_spec_to_civil_time() {
        let tz = reasonable_posix_time_zone("EST5EDT,J1,J365/5:12:34");
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.start.time().to_duration(),
            SignedDuration::from_hours(2),
        );
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.end.time().to_duration(),
            SignedDuration::new(5 * 60 * 60 + 12 * 60 + 34, 0),
        );

        let tz =
            reasonable_posix_time_zone("EST5EDT,J1/23:59:59,J365/24:00:00");
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.start.time().to_duration(),
            SignedDuration::new(23 * 60 * 60 + 59 * 60 + 59, 0),
        );
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.end.time().to_duration(),
            SignedDuration::from_hours(24),
        );

        let tz = reasonable_posix_time_zone("EST5EDT,J1/-1,J365/167:00:00");
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.start.time().to_duration(),
            SignedDuration::from_hours(-1),
        );
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.end.time().to_duration(),
            SignedDuration::from_hours(167),
        );
    }

    #[test]
    fn posix_time_spec_to_span() {
        let tz = reasonable_posix_time_zone("EST5EDT,J1,J365/5:12:34");
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.start.time().to_duration(),
            SignedDuration::from_hours(2),
        );
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.end.time().to_duration(),
            SignedDuration::from_secs((5 * 60 * 60) + (12 * 60) + 34),
        );

        let tz =
            reasonable_posix_time_zone("EST5EDT,J1/23:59:59,J365/24:00:00");
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.start.time().to_duration(),
            SignedDuration::from_secs((23 * 60 * 60) + (59 * 60) + 59),
        );
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.end.time().to_duration(),
            SignedDuration::from_hours(24),
        );

        let tz = reasonable_posix_time_zone("EST5EDT,J1/-1,J365/167:00:00");
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.start.time().to_duration(),
            SignedDuration::from_hours(-1),
        );
        assert_eq!(
            tz.dst.as_ref().unwrap().rule.end.time().to_duration(),
            SignedDuration::from_hours(167),
        );
    }

    #[cfg(feature = "tz-system")]
    #[test]
    fn parse_posix_tz() {
        let tz = PosixTz::parse("EST5EDT").unwrap();
        assert_eq!(
            tz,
            PosixTz::Rule(PosixTimeZone {
                std_abbrev: "EST".into(),
                std_offset: PosixOffset {
                    sign: None,
                    hour: C(5).rinto(),
                    minute: None,
                    second: None,
                },
                dst: Some(PosixDst {
                    abbrev: "EDT".into(),
                    offset: None,
                    rule: None,
                }),
            },)
        );

        let tz = PosixTz::parse(":EST5EDT").unwrap();
        assert_eq!(tz, PosixTz::Implementation("EST5EDT".into()));

        // We require implementation strings to be UTF-8, because we're
        // sensible.
        assert!(PosixTz::parse(b":EST5\xFFEDT").is_err());
    }

    #[test]
    fn parse_iana() {
        // Ref: https://github.com/chronotope/chrono/issues/1153
        let p = IanaTz::parse_v3plus("CRAZY5SHORT,M12.5.0/50,0/2").unwrap();
        assert_eq!(
            p,
            IanaTz(ReasonablePosixTimeZone {
                std_abbrev: "CRAZY".into(),
                std_offset: PosixOffset {
                    sign: None,
                    hour: C(5).rinto(),
                    minute: None,
                    second: None,
                },
                dst: Some(ReasonablePosixDst {
                    abbrev: "SHORT".into(),
                    offset: None,
                    rule: Rule {
                        start: PosixDateTimeSpec {
                            date: PosixDateSpec::WeekdayOfMonth(
                                WeekdayOfMonth {
                                    month: C(12).rinto(),
                                    week: C(5).rinto(),
                                    weekday: Weekday::Sunday,
                                },
                            ),
                            time: Some(PosixTimeSpec {
                                sign: None,
                                hour: C(50).rinto(),
                                minute: None,
                                second: None,
                            },),
                        },
                        end: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianZero(C(0).rinto()),
                            time: Some(PosixTimeSpec {
                                sign: None,
                                hour: C(2).rinto(),
                                minute: None,
                                second: None,
                            },),
                        },
                    },
                }),
            }),
        );

        let p = Parser::new("America/New_York");
        assert!(p.parse().is_err());

        let p = Parser::new(":America/New_York");
        assert!(p.parse().is_err());
    }

    #[test]
    fn parse() {
        let p = Parser::new("NZST-12NZDT,J60,J300");
        assert_eq!(
            p.parse().unwrap(),
            PosixTimeZone {
                std_abbrev: "NZST".into(),
                std_offset: PosixOffset {
                    sign: Some(Sign::N::<-1>()),
                    hour: C(12).rinto(),
                    minute: None,
                    second: None,
                },
                dst: Some(PosixDst {
                    abbrev: "NZDT".into(),
                    offset: None,
                    rule: Some(Rule {
                        start: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(60).rinto()),
                            time: None,
                        },
                        end: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(300).rinto()),
                            time: None,
                        },
                    }),
                }),
            },
        );

        let p = Parser::new("NZST-12NZDT,J60,J300WAT");
        assert!(p.parse().is_err());
    }

    #[test]
    fn parse_posix_time_zone() {
        let p = Parser::new("NZST-12NZDT,M9.5.0,M4.1.0/3");
        assert_eq!(
            p.parse_posix_time_zone().unwrap(),
            PosixTimeZone {
                std_abbrev: "NZST".into(),
                std_offset: PosixOffset {
                    sign: Some(Sign::N::<-1>()),
                    hour: C(12).rinto(),
                    minute: None,
                    second: None,
                },
                dst: Some(PosixDst {
                    abbrev: "NZDT".into(),
                    offset: None,
                    rule: Some(Rule {
                        start: PosixDateTimeSpec {
                            date: PosixDateSpec::WeekdayOfMonth(
                                WeekdayOfMonth {
                                    month: C(9).rinto(),
                                    week: C(5).rinto(),
                                    weekday: Weekday::Sunday,
                                }
                            ),
                            time: None,
                        },
                        end: PosixDateTimeSpec {
                            date: PosixDateSpec::WeekdayOfMonth(
                                WeekdayOfMonth {
                                    month: C(4).rinto(),
                                    week: C(1).rinto(),
                                    weekday: Weekday::Sunday,
                                }
                            ),
                            time: Some(PosixTimeSpec {
                                sign: None,
                                hour: C(3).rinto(),
                                minute: None,
                                second: None,
                            }),
                        },
                    })
                }),
            },
        );

        let p = Parser::new("NZST-12NZDT,M9.5.0,M4.1.0/3WAT");
        assert_eq!(
            p.parse_posix_time_zone().unwrap(),
            PosixTimeZone {
                std_abbrev: "NZST".into(),
                std_offset: PosixOffset {
                    sign: Some(Sign::N::<-1>()),
                    hour: C(12).rinto(),
                    minute: None,
                    second: None,
                },
                dst: Some(PosixDst {
                    abbrev: "NZDT".into(),
                    offset: None,
                    rule: Some(Rule {
                        start: PosixDateTimeSpec {
                            date: PosixDateSpec::WeekdayOfMonth(
                                WeekdayOfMonth {
                                    month: C(9).rinto(),
                                    week: C(5).rinto(),
                                    weekday: Weekday::Sunday,
                                }
                            ),
                            time: None,
                        },
                        end: PosixDateTimeSpec {
                            date: PosixDateSpec::WeekdayOfMonth(
                                WeekdayOfMonth {
                                    month: C(4).rinto(),
                                    week: C(1).rinto(),
                                    weekday: Weekday::Sunday,
                                }
                            ),
                            time: Some(PosixTimeSpec {
                                sign: None,
                                hour: C(3).rinto(),
                                minute: None,
                                second: None,
                            }),
                        },
                    })
                }),
            },
        );

        let p = Parser::new("NZST-12NZDT,J60,J300");
        assert_eq!(
            p.parse_posix_time_zone().unwrap(),
            PosixTimeZone {
                std_abbrev: "NZST".into(),
                std_offset: PosixOffset {
                    sign: Some(Sign::N::<-1>()),
                    hour: C(12).rinto(),
                    minute: None,
                    second: None,
                },
                dst: Some(PosixDst {
                    abbrev: "NZDT".into(),
                    offset: None,
                    rule: Some(Rule {
                        start: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(60).rinto()),
                            time: None,
                        },
                        end: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(300).rinto()),
                            time: None,
                        },
                    }),
                }),
            },
        );

        let p = Parser::new("NZST-12NZDT,J60,J300WAT");
        assert_eq!(
            p.parse_posix_time_zone().unwrap(),
            PosixTimeZone {
                std_abbrev: "NZST".into(),
                std_offset: PosixOffset {
                    sign: Some(Sign::N::<-1>()),
                    hour: C(12).rinto(),
                    minute: None,
                    second: None,
                },
                dst: Some(PosixDst {
                    abbrev: "NZDT".into(),
                    offset: None,
                    rule: Some(Rule {
                        start: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(60).rinto()),
                            time: None,
                        },
                        end: PosixDateTimeSpec {
                            date: PosixDateSpec::JulianOne(C(300).rinto()),
                            time: None,
                        },
                    }),
                }),
            },
        );
    }

    #[test]
    fn parse_posix_dst() {
        let p = Parser::new("NZDT,M9.5.0,M4.1.0/3");
        assert_eq!(
            p.parse_posix_dst().unwrap(),
            PosixDst {
                abbrev: "NZDT".into(),
                offset: None,
                rule: Some(Rule {
                    start: PosixDateTimeSpec {
                        date: PosixDateSpec::WeekdayOfMonth(WeekdayOfMonth {
                            month: C(9).rinto(),
                            week: C(5).rinto(),
                            weekday: Weekday::Sunday,
                        }),
                        time: None,
                    },
                    end: PosixDateTimeSpec {
                        date: PosixDateSpec::WeekdayOfMonth(WeekdayOfMonth {
                            month: C(4).rinto(),
                            week: C(1).rinto(),
                            weekday: Weekday::Sunday,
                        }),
                        time: Some(PosixTimeSpec {
                            sign: None,
                            hour: C(3).rinto(),
                            minute: None,
                            second: None,
                        }),
                    },
                }),
            },
        );

        let p = Parser::new("NZDT,J60,J300");
        assert_eq!(
            p.parse_posix_dst().unwrap(),
            PosixDst {
                abbrev: "NZDT".into(),
                offset: None,
                rule: Some(Rule {
                    start: PosixDateTimeSpec {
                        date: PosixDateSpec::JulianOne(C(60).rinto()),
                        time: None,
                    },
                    end: PosixDateTimeSpec {
                        date: PosixDateSpec::JulianOne(C(300).rinto()),
                        time: None,
                    },
                }),
            },
        );

        let p = Parser::new("NZDT-7,J60,J300");
        assert_eq!(
            p.parse_posix_dst().unwrap(),
            PosixDst {
                abbrev: "NZDT".into(),
                offset: Some(PosixOffset {
                    sign: Some(Sign::N::<-1>()),
                    hour: C(7).rinto(),
                    minute: None,
                    second: None,
                }),
                rule: Some(Rule {
                    start: PosixDateTimeSpec {
                        date: PosixDateSpec::JulianOne(C(60).rinto()),
                        time: None,
                    },
                    end: PosixDateTimeSpec {
                        date: PosixDateSpec::JulianOne(C(300).rinto()),
                        time: None,
                    },
                }),
            },
        );

        let p = Parser::new("NZDT+7,J60,J300");
        assert_eq!(
            p.parse_posix_dst().unwrap(),
            PosixDst {
                abbrev: "NZDT".into(),
                offset: Some(PosixOffset {
                    sign: Some(Sign::N::<1>()),
                    hour: C(7).rinto(),
                    minute: None,
                    second: None,
                }),
                rule: Some(Rule {
                    start: PosixDateTimeSpec {
                        date: PosixDateSpec::JulianOne(C(60).rinto()),
                        time: None,
                    },
                    end: PosixDateTimeSpec {
                        date: PosixDateSpec::JulianOne(C(300).rinto()),
                        time: None,
                    },
                }),
            },
        );

        let p = Parser::new("NZDT7,J60,J300");
        assert_eq!(
            p.parse_posix_dst().unwrap(),
            PosixDst {
                abbrev: "NZDT".into(),
                offset: Some(PosixOffset {
                    sign: None,
                    hour: C(7).rinto(),
                    minute: None,
                    second: None,
                }),
                rule: Some(Rule {
                    start: PosixDateTimeSpec {
                        date: PosixDateSpec::JulianOne(C(60).rinto()),
                        time: None,
                    },
                    end: PosixDateTimeSpec {
                        date: PosixDateSpec::JulianOne(C(300).rinto()),
                        time: None,
                    },
                }),
            },
        );

        let p = Parser::new("NZDT7,");
        assert!(p.parse_posix_dst().is_err());

        let p = Parser::new("NZDT7!");
        assert!(p.parse_posix_dst().is_err());
    }

    #[test]
    fn parse_abbreviation() {
        let p = Parser::new("ABC");
        assert_eq!(p.parse_abbreviation().unwrap(), "ABC");

        let p = Parser::new("<ABC>");
        assert_eq!(p.parse_abbreviation().unwrap(), "ABC");

        let p = Parser::new("<+09>");
        assert_eq!(p.parse_abbreviation().unwrap(), "+09");

        let p = Parser::new("+09");
        assert!(p.parse_abbreviation().is_err());
    }

    #[test]
    fn parse_unquoted_abbreviation() {
        let p = Parser::new("ABC");
        assert_eq!(p.parse_unquoted_abbreviation().unwrap(), "ABC");

        let p = Parser::new("ABCXYZ");
        assert_eq!(p.parse_unquoted_abbreviation().unwrap(), "ABCXYZ");

        let p = Parser::new("ABC123");
        assert_eq!(p.parse_unquoted_abbreviation().unwrap(), "ABC");

        let tz = "a".repeat(30);
        let p = Parser::new(&tz);
        assert_eq!(p.parse_unquoted_abbreviation().unwrap(), &*tz);

        let p = Parser::new("a");
        assert!(p.parse_unquoted_abbreviation().is_err());

        let p = Parser::new("ab");
        assert!(p.parse_unquoted_abbreviation().is_err());

        let p = Parser::new("ab1");
        assert!(p.parse_unquoted_abbreviation().is_err());

        let tz = "a".repeat(31);
        let p = Parser::new(&tz);
        assert!(p.parse_unquoted_abbreviation().is_err());

        let p = Parser::new(b"ab\xFFcd");
        assert!(p.parse_unquoted_abbreviation().is_err());
    }

    #[test]
    fn parse_quoted_abbreviation() {
        // The inputs look a little funny here, but that's because
        // 'parse_quoted_abbreviation' starts after the opening quote
        // has been parsed.

        let p = Parser::new("ABC>");
        assert_eq!(p.parse_quoted_abbreviation().unwrap(), "ABC");

        let p = Parser::new("ABCXYZ>");
        assert_eq!(p.parse_quoted_abbreviation().unwrap(), "ABCXYZ");

        let p = Parser::new("ABC>123");
        assert_eq!(p.parse_quoted_abbreviation().unwrap(), "ABC");

        let p = Parser::new("ABC123>");
        assert_eq!(p.parse_quoted_abbreviation().unwrap(), "ABC123");

        let p = Parser::new("ab1>");
        assert_eq!(p.parse_quoted_abbreviation().unwrap(), "ab1");

        let p = Parser::new("+09>");
        assert_eq!(p.parse_quoted_abbreviation().unwrap(), "+09");

        let p = Parser::new("-09>");
        assert_eq!(p.parse_quoted_abbreviation().unwrap(), "-09");

        let tz = alloc::format!("{}>", "a".repeat(30));
        let p = Parser::new(&tz);
        assert_eq!(
            p.parse_quoted_abbreviation().unwrap(),
            tz.trim_end_matches(">")
        );

        let p = Parser::new("a>");
        assert!(p.parse_quoted_abbreviation().is_err());

        let p = Parser::new("ab>");
        assert!(p.parse_quoted_abbreviation().is_err());

        let tz = alloc::format!("{}>", "a".repeat(31));
        let p = Parser::new(&tz);
        assert!(p.parse_quoted_abbreviation().is_err());

        let p = Parser::new(b"ab\xFFcd>");
        assert!(p.parse_quoted_abbreviation().is_err());

        let p = Parser::new("ABC");
        assert!(p.parse_quoted_abbreviation().is_err());

        let p = Parser::new("ABC!>");
        assert!(p.parse_quoted_abbreviation().is_err());
    }

    #[test]
    fn parse_posix_offset() {
        let p = Parser::new("5");
        assert_eq!(
            p.parse_posix_offset().unwrap(),
            PosixOffset {
                sign: None,
                hour: C(5).rinto(),
                minute: None,
                second: None,
            },
        );

        let p = Parser::new("+5");
        assert_eq!(
            p.parse_posix_offset().unwrap(),
            PosixOffset {
                sign: Some(Sign::N::<1>()),
                hour: C(5).rinto(),
                minute: None,
                second: None,
            },
        );

        let p = Parser::new("-5");
        assert_eq!(
            p.parse_posix_offset().unwrap(),
            PosixOffset {
                sign: Some(Sign::N::<-1>()),
                hour: C(5).rinto(),
                minute: None,
                second: None,
            },
        );

        let p = Parser::new("-12:34:56");
        assert_eq!(
            p.parse_posix_offset().unwrap(),
            PosixOffset {
                sign: Some(Sign::N::<-1>()),
                hour: C(12).rinto(),
                minute: Some(C(34).rinto()),
                second: Some(C(56).rinto()),
            },
        );

        let p = Parser::new("a");
        assert!(p.parse_posix_offset().is_err());

        let p = Parser::new("-");
        assert!(p.parse_posix_offset().is_err());

        let p = Parser::new("+");
        assert!(p.parse_posix_offset().is_err());

        let p = Parser::new("-a");
        assert!(p.parse_posix_offset().is_err());

        let p = Parser::new("+a");
        assert!(p.parse_posix_offset().is_err());

        let p = Parser::new("-25");
        assert!(p.parse_posix_offset().is_err());

        let p = Parser::new("+25");
        assert!(p.parse_posix_offset().is_err());

        // This checks that we don't accidentally permit IANA rules for
        // offset parsing. Namely, the IANA tzfile v3+ extension only applies
        // to transition times. But since POSIX says that the "time" for the
        // offset and transition is the same format, it would be an easy
        // implementation mistake to implement the more flexible rule for
        // IANA and have it accidentally also apply to the offset. So we check
        // that it doesn't here.
        let p = Parser { ianav3plus: true, ..Parser::new("25") };
        assert!(p.parse_posix_offset().is_err());
        let p = Parser { ianav3plus: true, ..Parser::new("+25") };
        assert!(p.parse_posix_offset().is_err());
        let p = Parser { ianav3plus: true, ..Parser::new("-25") };
        assert!(p.parse_posix_offset().is_err());
    }

    #[test]
    fn parse_rule() {
        let p = Parser::new("M9.5.0,M4.1.0/3");
        assert_eq!(
            p.parse_rule().unwrap(),
            Rule {
                start: PosixDateTimeSpec {
                    date: PosixDateSpec::WeekdayOfMonth(WeekdayOfMonth {
                        month: C(9).rinto(),
                        week: C(5).rinto(),
                        weekday: Weekday::Sunday,
                    }),
                    time: None,
                },
                end: PosixDateTimeSpec {
                    date: PosixDateSpec::WeekdayOfMonth(WeekdayOfMonth {
                        month: C(4).rinto(),
                        week: C(1).rinto(),
                        weekday: Weekday::Sunday,
                    }),
                    time: Some(PosixTimeSpec {
                        sign: None,
                        hour: C(3).rinto(),
                        minute: None,
                        second: None,
                    }),
                },
            },
        );

        let p = Parser::new("M9.5.0");
        assert!(p.parse_rule().is_err());

        let p = Parser::new(",M9.5.0,M4.1.0/3");
        assert!(p.parse_rule().is_err());

        let p = Parser::new("M9.5.0/");
        assert!(p.parse_rule().is_err());

        let p = Parser::new("M9.5.0,M4.1.0/");
        assert!(p.parse_rule().is_err());
    }

    #[test]
    fn parse_posix_datetime_spec() {
        let p = Parser::new("J1");
        assert_eq!(
            p.parse_posix_datetime_spec().unwrap(),
            PosixDateTimeSpec {
                date: PosixDateSpec::JulianOne(C(1).rinto()),
                time: None,
            },
        );

        let p = Parser::new("J1/3");
        assert_eq!(
            p.parse_posix_datetime_spec().unwrap(),
            PosixDateTimeSpec {
                date: PosixDateSpec::JulianOne(C(1).rinto()),
                time: Some(PosixTimeSpec {
                    sign: None,
                    hour: C(3).rinto(),
                    minute: None,
                    second: None,
                }),
            },
        );

        let p = Parser::new("M4.1.0/3");
        assert_eq!(
            p.parse_posix_datetime_spec().unwrap(),
            PosixDateTimeSpec {
                date: PosixDateSpec::WeekdayOfMonth(WeekdayOfMonth {
                    month: C(4).rinto(),
                    week: C(1).rinto(),
                    weekday: Weekday::Sunday,
                }),
                time: Some(PosixTimeSpec {
                    sign: None,
                    hour: C(3).rinto(),
                    minute: None,
                    second: None,
                }),
            },
        );

        let p = Parser::new("1/3:45:05");
        assert_eq!(
            p.parse_posix_datetime_spec().unwrap(),
            PosixDateTimeSpec {
                date: PosixDateSpec::JulianZero(C(1).rinto()),
                time: Some(PosixTimeSpec {
                    sign: None,
                    hour: C(3).rinto(),
                    minute: Some(C(45).rinto()),
                    second: Some(C(5).rinto()),
                }),
            },
        );

        let p = Parser::new("a");
        assert!(p.parse_posix_datetime_spec().is_err());

        let p = Parser::new("J1/");
        assert!(p.parse_posix_datetime_spec().is_err());

        let p = Parser::new("1/");
        assert!(p.parse_posix_datetime_spec().is_err());

        let p = Parser::new("M4.1.0/");
        assert!(p.parse_posix_datetime_spec().is_err());
    }

    #[test]
    fn parse_posix_date_spec() {
        let p = Parser::new("J1");
        assert_eq!(
            p.parse_posix_date_spec().unwrap(),
            PosixDateSpec::JulianOne(C(1).rinto())
        );
        let p = Parser::new("J365");
        assert_eq!(
            p.parse_posix_date_spec().unwrap(),
            PosixDateSpec::JulianOne(C(365).rinto())
        );

        let p = Parser::new("0");
        assert_eq!(
            p.parse_posix_date_spec().unwrap(),
            PosixDateSpec::JulianZero(C(0).rinto())
        );
        let p = Parser::new("1");
        assert_eq!(
            p.parse_posix_date_spec().unwrap(),
            PosixDateSpec::JulianZero(C(1).rinto())
        );
        let p = Parser::new("365");
        assert_eq!(
            p.parse_posix_date_spec().unwrap(),
            PosixDateSpec::JulianZero(C(365).rinto())
        );

        let p = Parser::new("M9.5.0");
        assert_eq!(
            p.parse_posix_date_spec().unwrap(),
            PosixDateSpec::WeekdayOfMonth(WeekdayOfMonth {
                month: C(9).rinto(),
                week: C(5).rinto(),
                weekday: Weekday::Sunday,
            }),
        );
        let p = Parser::new("M9.5.6");
        assert_eq!(
            p.parse_posix_date_spec().unwrap(),
            PosixDateSpec::WeekdayOfMonth(WeekdayOfMonth {
                month: C(9).rinto(),
                week: C(5).rinto(),
                weekday: Weekday::Saturday,
            }),
        );
        let p = Parser::new("M09.5.6");
        assert_eq!(
            p.parse_posix_date_spec().unwrap(),
            PosixDateSpec::WeekdayOfMonth(WeekdayOfMonth {
                month: C(9).rinto(),
                week: C(5).rinto(),
                weekday: Weekday::Saturday,
            }),
        );
        let p = Parser::new("M12.1.1");
        assert_eq!(
            p.parse_posix_date_spec().unwrap(),
            PosixDateSpec::WeekdayOfMonth(WeekdayOfMonth {
                month: C(12).rinto(),
                week: C(1).rinto(),
                weekday: Weekday::Monday,
            }),
        );

        let p = Parser::new("a");
        assert!(p.parse_posix_date_spec().is_err());

        let p = Parser::new("j");
        assert!(p.parse_posix_date_spec().is_err());

        let p = Parser::new("m");
        assert!(p.parse_posix_date_spec().is_err());

        let p = Parser::new("n");
        assert!(p.parse_posix_date_spec().is_err());

        let p = Parser::new("J366");
        assert!(p.parse_posix_date_spec().is_err());

        let p = Parser::new("366");
        assert!(p.parse_posix_date_spec().is_err());
    }

    #[test]
    fn parse_posix_julian_day_no_leap() {
        let p = Parser::new("1");
        assert_eq!(p.parse_posix_julian_day_no_leap().unwrap(), 1);

        let p = Parser::new("001");
        assert_eq!(p.parse_posix_julian_day_no_leap().unwrap(), 1);

        let p = Parser::new("365");
        assert_eq!(p.parse_posix_julian_day_no_leap().unwrap(), 365);

        let p = Parser::new("3655");
        assert_eq!(p.parse_posix_julian_day_no_leap().unwrap(), 365);

        let p = Parser::new("0");
        assert!(p.parse_posix_julian_day_no_leap().is_err());

        let p = Parser::new("366");
        assert!(p.parse_posix_julian_day_no_leap().is_err());
    }

    #[test]
    fn parse_posix_julian_day_with_leap() {
        let p = Parser::new("0");
        assert_eq!(p.parse_posix_julian_day_with_leap().unwrap(), 0);

        let p = Parser::new("1");
        assert_eq!(p.parse_posix_julian_day_with_leap().unwrap(), 1);

        let p = Parser::new("001");
        assert_eq!(p.parse_posix_julian_day_with_leap().unwrap(), 1);

        let p = Parser::new("365");
        assert_eq!(p.parse_posix_julian_day_with_leap().unwrap(), 365);

        let p = Parser::new("3655");
        assert_eq!(p.parse_posix_julian_day_with_leap().unwrap(), 365);

        let p = Parser::new("366");
        assert!(p.parse_posix_julian_day_with_leap().is_err());
    }

    #[test]
    fn parse_weekday_of_month() {
        let p = Parser::new("9.5.0");
        assert_eq!(
            p.parse_weekday_of_month().unwrap(),
            WeekdayOfMonth {
                month: C(9).rinto(),
                week: C(5).rinto(),
                weekday: Weekday::Sunday,
            },
        );

        let p = Parser::new("9.1.6");
        assert_eq!(
            p.parse_weekday_of_month().unwrap(),
            WeekdayOfMonth {
                month: C(9).rinto(),
                week: C(1).rinto(),
                weekday: Weekday::Saturday,
            },
        );

        let p = Parser::new("09.1.6");
        assert_eq!(
            p.parse_weekday_of_month().unwrap(),
            WeekdayOfMonth {
                month: C(9).rinto(),
                week: C(1).rinto(),
                weekday: Weekday::Saturday,
            },
        );

        let p = Parser::new("9");
        assert!(p.parse_weekday_of_month().is_err());

        let p = Parser::new("9.");
        assert!(p.parse_weekday_of_month().is_err());

        let p = Parser::new("9.5");
        assert!(p.parse_weekday_of_month().is_err());

        let p = Parser::new("9.5.");
        assert!(p.parse_weekday_of_month().is_err());

        let p = Parser::new("0.5.0");
        assert!(p.parse_weekday_of_month().is_err());

        let p = Parser::new("13.5.0");
        assert!(p.parse_weekday_of_month().is_err());

        let p = Parser::new("9.0.0");
        assert!(p.parse_weekday_of_month().is_err());

        let p = Parser::new("9.6.0");
        assert!(p.parse_weekday_of_month().is_err());

        let p = Parser::new("9.5.7");
        assert!(p.parse_weekday_of_month().is_err());
    }

    #[test]
    fn parse_posix_time_spec() {
        let p = Parser::new("5");
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: None,
                hour: C(5).rinto(),
                minute: None,
                second: None
            }
        );

        let p = Parser::new("22");
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: None,
                hour: C(22).rinto(),
                minute: None,
                second: None
            }
        );

        let p = Parser::new("02");
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: None,
                hour: C(2).rinto(),
                minute: None,
                second: None
            }
        );

        let p = Parser::new("5:45");
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: None,
                hour: C(5).rinto(),
                minute: Some(C(45).rinto()),
                second: None
            }
        );

        let p = Parser::new("5:45:12");
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: None,
                hour: C(5).rinto(),
                minute: Some(C(45).rinto()),
                second: Some(C(12).rinto()),
            }
        );

        let p = Parser::new("5:45:129");
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: None,
                hour: C(5).rinto(),
                minute: Some(C(45).rinto()),
                second: Some(C(12).rinto()),
            }
        );

        let p = Parser::new("5:45:12:");
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: None,
                hour: C(5).rinto(),
                minute: Some(C(45).rinto()),
                second: Some(C(12).rinto()),
            }
        );

        let p = Parser { ianav3plus: true, ..Parser::new("+5:45:12") };
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: Some(C(1).rinto()),
                hour: C(5).rinto(),
                minute: Some(C(45).rinto()),
                second: Some(C(12).rinto()),
            }
        );

        let p = Parser { ianav3plus: true, ..Parser::new("-5:45:12") };
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: Some(C(-1).rinto()),
                hour: C(5).rinto(),
                minute: Some(C(45).rinto()),
                second: Some(C(12).rinto()),
            }
        );

        let p = Parser { ianav3plus: true, ..Parser::new("-167:45:12") };
        assert_eq!(
            p.parse_posix_time_spec().unwrap(),
            PosixTimeSpec {
                sign: Some(C(-1).rinto()),
                hour: C(167).rinto(),
                minute: Some(C(45).rinto()),
                second: Some(C(12).rinto()),
            }
        );

        let p = Parser::new("25");
        assert!(p.parse_posix_time_spec().is_err());

        let p = Parser::new("12:2");
        assert!(p.parse_posix_time_spec().is_err());

        let p = Parser::new("12:");
        assert!(p.parse_posix_time_spec().is_err());

        let p = Parser::new("12:23:5");
        assert!(p.parse_posix_time_spec().is_err());

        let p = Parser::new("12:23:");
        assert!(p.parse_posix_time_spec().is_err());

        let p = Parser { ianav3plus: true, ..Parser::new("168") };
        assert!(p.parse_posix_time_spec().is_err());

        let p = Parser { ianav3plus: true, ..Parser::new("-168") };
        assert!(p.parse_posix_time_spec().is_err());

        let p = Parser { ianav3plus: true, ..Parser::new("+168") };
        assert!(p.parse_posix_time_spec().is_err());
    }

    #[test]
    fn parse_month() {
        let p = Parser::new("1");
        assert_eq!(p.parse_month().unwrap(), 1);

        // Should this be allowed? POSIX spec is unclear.
        // We allow it because our parse does stop at 2
        // digits, so this seems harmless. Namely, '001'
        // results in an error.
        let p = Parser::new("01");
        assert_eq!(p.parse_month().unwrap(), 1);

        let p = Parser::new("12");
        assert_eq!(p.parse_month().unwrap(), 12);

        let p = Parser::new("0");
        assert!(p.parse_month().is_err());

        let p = Parser::new("00");
        assert!(p.parse_month().is_err());

        let p = Parser::new("001");
        assert!(p.parse_month().is_err());

        let p = Parser::new("13");
        assert!(p.parse_month().is_err());
    }

    #[test]
    fn parse_week() {
        let p = Parser::new("1");
        assert_eq!(p.parse_week().unwrap(), 1);

        let p = Parser::new("5");
        assert_eq!(p.parse_week().unwrap(), 5);

        let p = Parser::new("55");
        assert_eq!(p.parse_week().unwrap(), 5);

        let p = Parser::new("0");
        assert!(p.parse_week().is_err());

        let p = Parser::new("6");
        assert!(p.parse_week().is_err());

        let p = Parser::new("00");
        assert!(p.parse_week().is_err());

        let p = Parser::new("01");
        assert!(p.parse_week().is_err());

        let p = Parser::new("05");
        assert!(p.parse_week().is_err());
    }

    #[test]
    fn parse_weekday() {
        let p = Parser::new("0");
        assert_eq!(p.parse_weekday().unwrap(), Weekday::Sunday);

        let p = Parser::new("1");
        assert_eq!(p.parse_weekday().unwrap(), Weekday::Monday);

        let p = Parser::new("6");
        assert_eq!(p.parse_weekday().unwrap(), Weekday::Saturday);

        let p = Parser::new("00");
        assert_eq!(p.parse_weekday().unwrap(), Weekday::Sunday);

        let p = Parser::new("06");
        assert_eq!(p.parse_weekday().unwrap(), Weekday::Sunday);

        let p = Parser::new("60");
        assert_eq!(p.parse_weekday().unwrap(), Weekday::Saturday);

        let p = Parser::new("7");
        assert!(p.parse_weekday().is_err());
    }

    #[test]
    fn parse_hour_posix() {
        let p = Parser::new("5");
        assert_eq!(p.parse_hour_posix().unwrap(), 5);

        let p = Parser::new("0");
        assert_eq!(p.parse_hour_posix().unwrap(), 0);

        let p = Parser::new("00");
        assert_eq!(p.parse_hour_posix().unwrap(), 0);

        let p = Parser::new("24");
        assert_eq!(p.parse_hour_posix().unwrap(), 24);

        let p = Parser::new("100");
        assert_eq!(p.parse_hour_posix().unwrap(), 10);

        let p = Parser::new("25");
        assert!(p.parse_hour_posix().is_err());

        let p = Parser::new("99");
        assert!(p.parse_hour_posix().is_err());
    }

    #[test]
    fn parse_hour_ianav3plus() {
        let new = |input| Parser { ianav3plus: true, ..Parser::new(input) };

        let p = new("5");
        assert_eq!(p.parse_hour_ianav3plus().unwrap(), 5);

        let p = new("0");
        assert_eq!(p.parse_hour_ianav3plus().unwrap(), 0);

        let p = new("00");
        assert_eq!(p.parse_hour_ianav3plus().unwrap(), 0);

        let p = new("000");
        assert_eq!(p.parse_hour_ianav3plus().unwrap(), 0);

        let p = new("24");
        assert_eq!(p.parse_hour_ianav3plus().unwrap(), 24);

        let p = new("100");
        assert_eq!(p.parse_hour_ianav3plus().unwrap(), 100);

        let p = new("1000");
        assert_eq!(p.parse_hour_ianav3plus().unwrap(), 100);

        let p = new("167");
        assert_eq!(p.parse_hour_ianav3plus().unwrap(), 167);

        let p = new("168");
        assert!(p.parse_hour_ianav3plus().is_err());

        let p = new("999");
        assert!(p.parse_hour_ianav3plus().is_err());
    }

    #[test]
    fn parse_minute() {
        let p = Parser::new("00");
        assert_eq!(p.parse_minute().unwrap(), 0);

        let p = Parser::new("24");
        assert_eq!(p.parse_minute().unwrap(), 24);

        let p = Parser::new("59");
        assert_eq!(p.parse_minute().unwrap(), 59);

        let p = Parser::new("599");
        assert_eq!(p.parse_minute().unwrap(), 59);

        let p = Parser::new("0");
        assert!(p.parse_minute().is_err());

        let p = Parser::new("1");
        assert!(p.parse_minute().is_err());

        let p = Parser::new("9");
        assert!(p.parse_minute().is_err());

        let p = Parser::new("60");
        assert!(p.parse_minute().is_err());
    }

    #[test]
    fn parse_second() {
        let p = Parser::new("00");
        assert_eq!(p.parse_second().unwrap(), 0);

        let p = Parser::new("24");
        assert_eq!(p.parse_second().unwrap(), 24);

        let p = Parser::new("59");
        assert_eq!(p.parse_second().unwrap(), 59);

        let p = Parser::new("599");
        assert_eq!(p.parse_second().unwrap(), 59);

        let p = Parser::new("0");
        assert!(p.parse_second().is_err());

        let p = Parser::new("1");
        assert!(p.parse_second().is_err());

        let p = Parser::new("9");
        assert!(p.parse_second().is_err());

        let p = Parser::new("60");
        assert!(p.parse_second().is_err());
    }

    #[test]
    fn parse_number_with_exactly_n_digits() {
        let p = Parser::new("1");
        assert_eq!(p.parse_number_with_exactly_n_digits(1).unwrap(), 1);

        let p = Parser::new("12");
        assert_eq!(p.parse_number_with_exactly_n_digits(2).unwrap(), 12);

        let p = Parser::new("123");
        assert_eq!(p.parse_number_with_exactly_n_digits(2).unwrap(), 12);

        let p = Parser::new("");
        assert!(p.parse_number_with_exactly_n_digits(1).is_err());

        let p = Parser::new("1");
        assert!(p.parse_number_with_exactly_n_digits(2).is_err());

        let p = Parser::new("12");
        assert!(p.parse_number_with_exactly_n_digits(3).is_err());
    }

    #[test]
    fn parse_number_with_upto_n_digits() {
        let p = Parser::new("1");
        assert_eq!(p.parse_number_with_upto_n_digits(1).unwrap(), 1);

        let p = Parser::new("1");
        assert_eq!(p.parse_number_with_upto_n_digits(2).unwrap(), 1);

        let p = Parser::new("12");
        assert_eq!(p.parse_number_with_upto_n_digits(2).unwrap(), 12);

        let p = Parser::new("12");
        assert_eq!(p.parse_number_with_upto_n_digits(3).unwrap(), 12);

        let p = Parser::new("123");
        assert_eq!(p.parse_number_with_upto_n_digits(2).unwrap(), 12);

        let p = Parser::new("");
        assert!(p.parse_number_with_upto_n_digits(1).is_err());

        let p = Parser::new("a");
        assert!(p.parse_number_with_upto_n_digits(1).is_err());
    }
}
