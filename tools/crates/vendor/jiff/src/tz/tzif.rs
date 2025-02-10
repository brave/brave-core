/*!
This module provides support for TZif binary files from the [Time Zone
Database].

These binary files are the ones commonly found in Unix distributions in the
`/usr/share/zoneinfo` directory.

[Time Zone Database]: https://www.iana.org/time-zones
*/

use core::ops::Range;

use alloc::{string::String, vec, vec::Vec};

use crate::{
    civil::DateTime,
    error::{err, Error, ErrorContext},
    timestamp::Timestamp,
    tz::{
        posix::{IanaTz, ReasonablePosixTimeZone},
        AmbiguousOffset, Dst, Offset, TimeZoneTransition,
    },
    util::{
        crc32,
        escape::{Byte, Bytes},
        t::UnixSeconds,
    },
};

/// A time zone based on IANA TZif formatted data.
///
/// TZif is a binary format described by RFC 8536. Its typical structure is to
/// define a single time zone per file in the `/usr/share/zoneinfo` directory
/// on Unix systems. The name of a time zone is its file path with the
/// `/usr/share/zoneinfo/` prefix stripped from it.
///
/// This type doesn't provide any facilities for dealing with files on disk
/// or the `/usr/share/zoneinfo` directory. This type is just for parsing the
/// contents of TZif formatted data in memory, and turning it into a data type
/// that can be used as a time zone.
#[derive(Debug)]
pub(crate) struct Tzif {
    name: Option<String>,
    /// An ASCII byte corresponding to the version number. So, 0x50 is '2'.
    ///
    /// This is unused. It's only used in `test` compilation for emitting
    /// diagnostic data about TZif files. If we really need to use this, we
    /// should probably just convert it to an actual integer.
    #[allow(dead_code)]
    version: u8,
    checksum: u32,
    transitions: Vec<Transition>,
    types: Vec<LocalTimeType>,
    designations: String,
    leap_seconds: Vec<LeapSecond>,
    posix_tz: Option<ReasonablePosixTimeZone>,
}

impl Tzif {
    /// Parses the given data as a TZif formatted file.
    ///
    /// The name given is attached to the `Tzif` value returned, but is
    /// otherwise not significant.
    ///
    /// If the given data is not recognized to be valid TZif, then an error is
    /// returned.
    ///
    /// In general, callers may assume that it is safe to pass arbitrary or
    /// even untrusted data to this function and count on it not panicking
    /// or using resources that aren't limited to a small constant factor of
    /// the size of the data itself. That is, callers can reliably limit the
    /// resources used by limiting the size of the data given to this parse
    /// function.
    pub(crate) fn parse(
        name: Option<String>,
        bytes: &[u8],
    ) -> Result<Tzif, Error> {
        let original = bytes;
        let name = name.into();
        let (header32, rest) = Header::parse(4, bytes)
            .map_err(|e| e.context("failed to parse 32-bit header"))?;
        let (mut tzif, rest) = if header32.version == 0 {
            Tzif::parse32(name, header32, rest)?
        } else {
            Tzif::parse64(name, header32, rest)?
        };
        // Compute the checksum using the entire contents of the TZif data.
        let tzif_raw_len = (rest.as_ptr() as usize)
            .checked_sub(original.as_ptr() as usize)
            .unwrap();
        let tzif_raw_bytes = &original[..tzif_raw_len];
        tzif.checksum = crc32::sum(tzif_raw_bytes);
        Ok(tzif)
    }

    /// Returns the name given to this TZif data in its constructor.
    pub(crate) fn name(&self) -> Option<&str> {
        self.name.as_deref()
    }

    /// Returns the appropriate time zone offset to use for the given
    /// timestamp.
    ///
    /// This also includes whether the offset returned should be considered to
    /// be DST or not, along with the time zone abbreviation (e.g., EST for
    /// standard time in New York, and EDT for DST in New York).
    pub(crate) fn to_offset(
        &self,
        timestamp: Timestamp,
    ) -> (Offset, Dst, &str) {
        // This is guaranteed because we always push at least one transition.
        // This isn't guaranteed by TZif since it might have 0 transitions,
        // but we always add a "dummy" first transition with our minimum
        // `Timestamp` value. TZif doesn't do this because there is no
        // universal minimum timestamp. (`i64::MIN` is a candidate, but that's
        // likely to cause overflow in readers that don't do error checking.)
        //
        // The result of the dummy transition is that the code below is simpler
        // with fewer special cases.
        assert!(!self.transitions.is_empty(), "transitions is non-empty");
        let index = if timestamp > self.transitions.last().unwrap().timestamp {
            self.transitions.len() - 1
        } else {
            let search = self
                .transitions
                .binary_search_by_key(&timestamp, |t| t.timestamp);
            match search {
                // Since the first transition is always Timestamp::MIN, it's
                // impossible for any timestamp to sort before it.
                Err(0) => {
                    unreachable!("impossible to come before Timestamp::MIN")
                }
                Ok(i) => i,
                // i points to the position immediately after the matching
                // timestamp. And since we know that i>0 because of the i==0
                // check above, we can safely subtract 1.
                Err(i) => i.checked_sub(1).expect("i is non-zero"),
            }
        };
        // Our index is always in bounds. The only way it couldn't be is if
        // binary search returns an Err(len) for a time greater than the
        // maximum transition. But we account for that above by converting
        // Err(len) to Err(len-1).
        assert!(index < self.transitions.len());
        // RFC 8536 says: "Local time for timestamps on or after the last
        // transition is specified by the TZ string in the footer (Section 3.3)
        // if present and nonempty; otherwise, it is unspecified."
        //
        // Subtracting 1 is OK because we know self.transitions is not empty.
        let t = if index < self.transitions.len() - 1 {
            // This is the typical case in "fat" TZif files: we found a
            // matching transition.
            &self.transitions[index]
        } else {
            match self.posix_tz.as_ref() {
                // This is the typical case in "slim" TZif files, where the
                // last transition is, as I understand it, the transition at
                // which a consistent rule started that a POSIX TZ string can
                // fully describe. For example, (as of 2024-03-27) the last
                // transition in the "fat" America/New_York TZif file is
                // in 2037, where as in the "slim" version it is 2007.
                //
                // This is likely why some things break with the "slim"
                // version: they don't support POSIX TZ strings (or don't
                // support them correctly).
                Some(tz) => return tz.to_offset(timestamp),
                // This case is technically unspecified, but I think the
                // typical thing to do is to just use the last transition.
                // I'm not 100% sure on this one.
                None => &self.transitions[index],
            }
        };
        let typ = self.local_time_type(t);
        self.local_time_type_to_offset(typ)
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
        // This implementation very nearly mirrors `to_offset` above in the
        // beginning: we do a binary search to find transition applicable for
        // the given datetime. Except, we do it on wall clock times instead
        // of timestamps. And in particular, each transition begins with a
        // possibly ambiguous range of wall clock times corresponding to either
        // a "gap" or "fold" in time.
        assert!(!self.transitions.is_empty(), "transitions is non-empty");
        let search =
            self.transitions.binary_search_by_key(&dt, |t| t.wall.start());
        let this_index = match search {
            Err(0) => unreachable!("impossible to come before DateTime::MIN"),
            Ok(i) => i,
            Err(i) => i.checked_sub(1).expect("i is non-zero"),
        };
        assert!(this_index < self.transitions.len());

        let this = &self.transitions[this_index];
        let this_offset = self.local_time_type(this).offset;
        // This is a little tricky, but we need to check for ambiguous civil
        // datetimes before possibly using the POSIX TZ string. Namely, a
        // datetime could be ambiguous with respect to the last transition,
        // and we should handle that according to the gap/fold determined for
        // that transition. We cover this case in tests in tz/mod.rs for the
        // Pacific/Honolulu time zone, whose last transition begins with a gap.
        match this.wall {
            TransitionWall::Gap { end, .. } if dt < end => {
                // A gap/fold can only appear when there exists a previous
                // transition.
                let prev_index = this_index.checked_sub(1).unwrap();
                let prev = &self.transitions[prev_index];
                let prev_offset = self.local_time_type(prev).offset;
                return AmbiguousOffset::Gap {
                    before: prev_offset,
                    after: this_offset,
                };
            }
            TransitionWall::Fold { end, .. } if dt < end => {
                // A gap/fold can only appear when there exists a previous
                // transition.
                let prev_index = this_index.checked_sub(1).unwrap();
                let prev = &self.transitions[prev_index];
                let prev_offset = self.local_time_type(prev).offset;
                return AmbiguousOffset::Fold {
                    before: prev_offset,
                    after: this_offset,
                };
            }
            _ => {}
        }
        // The datetime given is not ambiguous with respect to any of the
        // transitions in the TZif data. But, if we matched at or after the
        // last transition, then we need to use the POSIX TZ string (which
        // could still return an ambiguous offset).
        if this_index == self.transitions.len() - 1 {
            if let Some(tz) = self.posix_tz.as_ref() {
                return tz.to_ambiguous_kind(dt);
            }
            // This case is unspecified according to RFC 8536. It means that
            // the given datetime exceeds all transitions *and* there is no
            // POSIX TZ string. So this can happen in V1 files for example.
            // But those should hopefully be essentially non-existent nowadays
            // (2024-03). In any case, we just fall through to using the last
            // transition, which does seem likely to be wrong ~half the time
            // in time zones with DST. But there really isn't much else we can
            // do I think.
        }
        AmbiguousOffset::Unambiguous { offset: this_offset }
    }

    /// Returns the timestamp of the most recent time zone transition prior
    /// to the timestamp given. If one doesn't exist, `None` is returned.
    pub(crate) fn previous_transition(
        &self,
        ts: Timestamp,
    ) -> Option<TimeZoneTransition> {
        assert!(!self.transitions.is_empty(), "transitions is non-empty");
        let search =
            self.transitions.binary_search_by_key(&ts, |t| t.timestamp);
        let index = match search {
            Ok(i) | Err(i) => i.checked_sub(1)?,
        };
        let trans = if index == 0 {
            // The first transition is a dummy that we insert, so if we land on
            // it here, treat it as if it doesn't exist.
            return None;
        } else if index == self.transitions.len() - 1 {
            if let Some(ref posix_tz) = self.posix_tz {
                // Since the POSIX TZ must be consistent with the last
                // transition, it must be the case that tzif_last <=
                // posix_prev_trans in all cases. So the transition according
                // to the POSIX TZ is always correct here.
                //
                // What if this returns `None` though? I'm not sure in which
                // cases that could matter, and I think it might be a violation
                // of the TZif format if it does.
                return posix_tz.previous_transition(ts);
            }
            &self.transitions[index]
        } else {
            &self.transitions[index]
        };
        let typ = &self.types[usize::from(trans.type_index)];
        Some(TimeZoneTransition {
            timestamp: trans.timestamp,
            offset: typ.offset,
            abbrev: self.designation(typ),
            dst: typ.is_dst,
        })
    }

    /// Returns the timestamp of the soonest time zone transition after the
    /// timestamp given. If one doesn't exist, `None` is returned.
    pub(crate) fn next_transition(
        &self,
        ts: Timestamp,
    ) -> Option<TimeZoneTransition> {
        assert!(!self.transitions.is_empty(), "transitions is non-empty");
        let search =
            self.transitions.binary_search_by_key(&ts, |t| t.timestamp);
        let index = match search {
            Ok(i) => i.checked_add(1)?,
            Err(i) => i,
        };
        let trans = if index == 0 {
            // The first transition is a dummy that we insert, so if we land on
            // it here, treat it as if it doesn't exist.
            return None;
        } else if index >= self.transitions.len() - 1 {
            if let Some(ref posix_tz) = self.posix_tz {
                // Since the POSIX TZ must be consistent with the last
                // transition, it must be the case that next.timestamp <=
                // posix_next_tans in all cases. So the transition according to
                // the POSIX TZ is always correct here.
                //
                // What if this returns `None` though? I'm not sure in which
                // cases that could matter, and I think it might be a violation
                // of the TZif format if it does.
                return posix_tz.next_transition(ts);
            }
            self.transitions.last().expect("last transition")
        } else {
            &self.transitions[index]
        };
        let typ = &self.types[usize::from(trans.type_index)];
        Some(TimeZoneTransition {
            timestamp: trans.timestamp,
            offset: typ.offset,
            abbrev: self.designation(typ),
            dst: typ.is_dst,
        })
    }

    fn local_time_type_to_offset(
        &self,
        typ: &LocalTimeType,
    ) -> (Offset, Dst, &str) {
        (typ.offset, typ.is_dst, self.designation(typ))
    }

    fn designation(&self, typ: &LocalTimeType) -> &str {
        // OK because we verify that the designation range on every local
        // time type is a valid range into `self.designations`.
        &self.designations[typ.designation()]
    }

    fn local_time_type(&self, transition: &Transition) -> &LocalTimeType {
        // OK because we require that `type_index` always points to a valid
        // local time type.
        &self.types[usize::from(transition.type_index)]
    }

    fn first_transition(&self) -> &Transition {
        // OK because we know we have at least one transition. This isn't
        // true generally of the TZif format, since it does actually permit 0
        // transitions. But as part of parsing, we always add a "dummy" first
        // transition corresponding to the minimum possible Jiff timestamp.
        // This makes some logic for transition lookups a little simpler by
        // reducing special cases.
        self.transitions.first().unwrap()
    }

    fn parse32<'b>(
        name: Option<String>,
        header32: Header,
        bytes: &'b [u8],
    ) -> Result<(Tzif, &'b [u8]), Error> {
        let mut tzif = Tzif {
            name,
            version: header32.version,
            // filled in later
            checksum: 0,
            transitions: vec![],
            types: vec![],
            designations: String::new(),
            leap_seconds: vec![],
            posix_tz: None,
        };
        let rest = tzif.parse_transitions(&header32, bytes)?;
        let rest = tzif.parse_transition_types(&header32, rest)?;
        let rest = tzif.parse_local_time_types(&header32, rest)?;
        let rest = tzif.parse_time_zone_designations(&header32, rest)?;
        let rest = tzif.parse_leap_seconds(&header32, rest)?;
        let rest = tzif.parse_indicators(&header32, rest)?;
        tzif.set_wall_datetimes();
        Ok((tzif, rest))
    }

    fn parse64<'b>(
        name: Option<String>,
        header32: Header,
        bytes: &'b [u8],
    ) -> Result<(Tzif, &'b [u8]), Error> {
        let (_, rest) = try_split_at(
            "V1 TZif data block",
            bytes,
            header32.data_block_len()?,
        )?;
        let (header64, rest) = Header::parse(8, rest)
            .map_err(|e| e.context("failed to parse 64-bit header"))?;
        let mut tzif = Tzif {
            name,
            version: header64.version,
            // filled in later
            checksum: 0,
            transitions: vec![],
            types: vec![],
            designations: String::new(),
            leap_seconds: vec![],
            posix_tz: None,
        };
        let rest = tzif.parse_transitions(&header64, rest)?;
        let rest = tzif.parse_transition_types(&header64, rest)?;
        let rest = tzif.parse_local_time_types(&header64, rest)?;
        let rest = tzif.parse_time_zone_designations(&header64, rest)?;
        let rest = tzif.parse_leap_seconds(&header64, rest)?;
        let rest = tzif.parse_indicators(&header64, rest)?;
        let rest = tzif.parse_footer(&header64, rest)?;
        // Validates that the POSIX TZ string we parsed (if one exists) is
        // consistent with the last transition in this time zone. This is
        // required by RFC 8536.
        //
        // RFC 8536 says, "If the string is nonempty and one or more
        // transitions appear in the version 2+ data, the string MUST be
        // consistent with the last version 2+ transition."
        //
        // We need to be a little careful, since we always have at least one
        // transition (accounting for the dummy `Timestamp::MIN` transition).
        // So if we only have 1 transition and a POSIX TZ string, then we
        // should not validate it since it's equivalent to the case of 0
        // transitions and a POSIX TZ string.
        if tzif.transitions.len() > 1 {
            if let Some(ref tz) = tzif.posix_tz {
                let last = tzif.transitions.last().expect("last transition");
                let typ = tzif.local_time_type(last);
                let (offset, dst, abbrev) = tz.to_offset(last.timestamp);
                if offset != typ.offset {
                    return Err(err!(
                        "expected last transition to have DST offset \
                         of {}, but got {offset} according to POSIX TZ \
                         string {}",
                        typ.offset,
                        tz,
                    ));
                }
                if dst != typ.is_dst {
                    return Err(err!(
                        "expected last transition to have is_dst={}, \
                         but got is_dst={} according to POSIX TZ \
                         string {}",
                        typ.is_dst.is_dst(),
                        dst.is_dst(),
                        tz,
                    ));
                }
                if abbrev != tzif.designation(&typ) {
                    return Err(err!(
                        "expected last transition to have \
                         designation={abbrev}, \
                         but got designation={} according to POSIX TZ \
                         string {}",
                        tzif.designation(&typ),
                        tz,
                    ));
                }
            }
        }
        tzif.set_wall_datetimes();
        // N.B. We don't check that the TZif data is fully valid. It
        // is possible for it to contain superfluous information. For
        // example, a non-zero local time type that is never referenced
        // by a transition.
        Ok((tzif, rest))
    }

    fn parse_transitions<'b>(
        &mut self,
        header: &Header,
        bytes: &'b [u8],
    ) -> Result<&'b [u8], Error> {
        let (bytes, rest) = try_split_at(
            "transition times data block",
            bytes,
            header.transition_times_len()?,
        )?;
        let mut it = bytes.chunks_exact(header.time_size);
        // RFC 8536 says: "If there are no transitions, local time for all
        // timestamps is specified by the TZ string in the footer if present
        // and nonempty; otherwise, it is specified by time type 0."
        //
        // RFC 8536 also says: "Local time for timestamps before the first
        // transition is specified by the first time type (time type
        // 0)."
        //
        // So if there are no transitions, pushing this dummy one will result
        // in the desired behavior even when it's the only transition.
        // Similarly, since this is the minimum timestamp value, it will
        // trigger for any times before the first transition found in the TZif
        // data.
        self.transitions.push(Transition {
            timestamp: Timestamp::MIN,
            wall: TransitionWall::Unambiguous { start: DateTime::MIN },
            type_index: 0,
        });
        while let Some(chunk) = it.next() {
            let seconds = if header.is_32bit() {
                i64::from(from_be_bytes_i32(chunk))
            } else {
                from_be_bytes_i64(chunk)
            };
            let timestamp =
                Timestamp::from_second(seconds).unwrap_or_else(|_| {
                    // We really shouldn't error here just because the Unix
                    // timestamp is outside what Jiff supports. Since what Jiff
                    // supports is _somewhat_ arbitrary. But Jiff's supported
                    // range is good enough for all realistic purposes, so we
                    // just clamp an out-of-range Unix timestamp to the Jiff
                    // min or max value.
                    //
                    // This can't result in the sorting order being wrong, but
                    // it can result in a transition that is duplicative with
                    // the dummy transition we inserted above. This should be
                    // fine.
                    let clamped = seconds
                        .clamp(UnixSeconds::MIN_REPR, UnixSeconds::MAX_REPR);
                    warn!(
                        "found Unix timestamp {seconds} that is outside \
                         Jiff's supported range, clamping to {clamped}",
                    );
                    // Guaranteed to succeed since we clamped `seconds` such
                    // that it is in the supported range of `Timestamp`.
                    Timestamp::from_second(clamped).unwrap()
                });
            self.transitions.push(Transition {
                timestamp,
                // We can't compute the wall clock times until we know the
                // actual offset for the transition prior to this one. We don't
                // know that until we parse the local time types.
                wall: TransitionWall::Unambiguous {
                    start: DateTime::default(),
                },
                // We can't fill in the type index either. We fill this in
                // later when we parse the transition types.
                type_index: 0,
            });
        }
        assert!(it.remainder().is_empty());
        Ok(rest)
    }

    fn parse_transition_types<'b>(
        &mut self,
        header: &Header,
        bytes: &'b [u8],
    ) -> Result<&'b [u8], Error> {
        let (bytes, rest) = try_split_at(
            "transition types data block",
            bytes,
            header.transition_types_len()?,
        )?;
        // We start our transition indices at 1 because we always insert a
        // dummy first transition corresponding to `Timestamp::MIN`. Its type
        // index is always 0, so there's no need to change it here.
        for (transition_index, &type_index) in (1..).zip(bytes) {
            if usize::from(type_index) >= header.tzh_typecnt {
                return Err(err!(
                    "found transition type index {type_index},
                     but there are only {} local time types",
                    header.tzh_typecnt,
                ));
            }
            self.transitions[transition_index].type_index = type_index;
        }
        Ok(rest)
    }

    fn parse_local_time_types<'b>(
        &mut self,
        header: &Header,
        bytes: &'b [u8],
    ) -> Result<&'b [u8], Error> {
        let (bytes, rest) = try_split_at(
            "local time types data block",
            bytes,
            header.local_time_types_len()?,
        )?;
        let mut it = bytes.chunks_exact(6);
        while let Some(chunk) = it.next() {
            let offset_seconds = from_be_bytes_i32(&chunk[..4]);
            let offset =
                Offset::from_seconds(offset_seconds).map_err(|e| {
                    err!(
                        "found local time type with out-of-bounds offset: {e}"
                    )
                })?;
            let is_dst = Dst::from(chunk[4] == 1);
            let designation = chunk[5]..chunk[5];
            self.types.push(LocalTimeType {
                offset,
                is_dst,
                designation,
                indicator: Indicator::LocalWall,
            });
        }
        assert!(it.remainder().is_empty());
        Ok(rest)
    }

    fn parse_time_zone_designations<'b>(
        &mut self,
        header: &Header,
        bytes: &'b [u8],
    ) -> Result<&'b [u8], Error> {
        let (bytes, rest) = try_split_at(
            "time zone designations data block",
            bytes,
            header.time_zone_designations_len()?,
        )?;
        self.designations =
            String::from_utf8(bytes.to_vec()).map_err(|_| {
                err!(
                    "time zone designations are not valid UTF-8: {:?}",
                    Bytes(bytes),
                )
            })?;
        // Holy hell, this is brutal. The boundary conditions are crazy.
        for (i, typ) in self.types.iter_mut().enumerate() {
            let start = usize::from(typ.designation.start);
            let Some(suffix) = self.designations.get(start..) else {
                return Err(err!(
                    "local time type {i} has designation index of {start}, \
                     but cannot be more than {}",
                    self.designations.len(),
                ));
            };
            let Some(len) = suffix.find('\x00') else {
                return Err(err!(
                    "local time type {i} has designation index of {start}, \
                     but could not find NUL terminator after it in \
                     designations: {:?}",
                    self.designations,
                ));
            };
            let Some(end) = start.checked_add(len) else {
                return Err(err!(
                    "local time type {i} has designation index of {start}, \
                     but its length {len} is too big",
                ));
            };
            typ.designation.end = u8::try_from(end).map_err(|_| {
                err!(
                    "local time type {i} has designation range of \
                     {start}..{end}, but end is too big",
                )
            })?;
        }
        Ok(rest)
    }

    fn parse_leap_seconds<'b>(
        &mut self,
        header: &Header,
        bytes: &'b [u8],
    ) -> Result<&'b [u8], Error> {
        let (bytes, rest) = try_split_at(
            "leap seconds data block",
            bytes,
            header.leap_second_len()?,
        )?;
        let chunk_len = header
            .time_size
            .checked_add(4)
            .expect("time_size plus 4 fits in usize");
        let mut it = bytes.chunks_exact(chunk_len);
        while let Some(chunk) = it.next() {
            let (occur_bytes, corr_bytes) = chunk.split_at(header.time_size);
            let occur_seconds = if header.is_32bit() {
                i64::from(from_be_bytes_i32(occur_bytes))
            } else {
                from_be_bytes_i64(occur_bytes)
            };
            let occurrence =
                Timestamp::from_second(occur_seconds).map_err(|e| {
                    err!(
                        "leap second occurrence {occur_seconds} \
                         is out of range: {e}"
                    )
                })?;
            let correction = from_be_bytes_i32(corr_bytes);
            self.leap_seconds.push(LeapSecond { occurrence, correction });
        }
        assert!(it.remainder().is_empty());
        Ok(rest)
    }

    fn parse_indicators<'b>(
        &mut self,
        header: &Header,
        bytes: &'b [u8],
    ) -> Result<&'b [u8], Error> {
        let (std_wall_bytes, rest) = try_split_at(
            "standard/wall indicators data block",
            bytes,
            header.standard_wall_len()?,
        )?;
        let (ut_local_bytes, rest) = try_split_at(
            "UT/local indicators data block",
            rest,
            header.ut_local_len()?,
        )?;
        if std_wall_bytes.is_empty() && !ut_local_bytes.is_empty() {
            // This is a weird case, but technically possible only if all
            // UT/local indicators are 0. If any are 1, then it's an error,
            // because it would require the corresponding std/wall indicator
            // to be 1 too. Which it can't be, because there aren't any. So
            // we just check that they're all zeros.
            for (i, &byte) in ut_local_bytes.iter().enumerate() {
                if byte != 0 {
                    return Err(err!(
                        "found UT/local indicator '{byte}' for local time \
                         type {i}, but it must be 0 since all std/wall \
                         indicators are 0",
                    ));
                }
            }
        } else if !std_wall_bytes.is_empty() && ut_local_bytes.is_empty() {
            for (i, &byte) in std_wall_bytes.iter().enumerate() {
                // Indexing is OK because Header guarantees that the number of
                // indicators is 0 or equal to the number of types.
                self.types[i].indicator = if byte == 0 {
                    Indicator::LocalWall
                } else if byte == 1 {
                    Indicator::LocalStandard
                } else {
                    return Err(err!(
                        "found invalid std/wall indicator '{byte}' for \
                         local time type {i}, it must be 0 or 1",
                    ));
                };
            }
        } else if !std_wall_bytes.is_empty() && !ut_local_bytes.is_empty() {
            assert_eq!(std_wall_bytes.len(), ut_local_bytes.len());
            let it = std_wall_bytes.iter().zip(ut_local_bytes);
            for (i, (&stdwall, &utlocal)) in it.enumerate() {
                // Indexing is OK because Header guarantees that the number of
                // indicators is 0 or equal to the number of types.
                self.types[i].indicator = match (stdwall, utlocal) {
                    (0, 0) => Indicator::LocalWall,
                    (1, 0) => Indicator::LocalStandard,
                    (1, 1) => Indicator::UTStandard,
                    (0, 1) => {
                        return Err(err!(
                            "found illegal ut-wall combination for \
                         local time type {i}, only local-wall, local-standard \
                         and ut-standard are allowed",
                        ))
                    }
                    _ => {
                        return Err(err!(
                            "found illegal std/wall or ut/local value for \
                         local time type {i}, each must be 0 or 1",
                        ))
                    }
                };
            }
        } else {
            // If they're both empty then we don't need to do anything. Every
            // local time type record already has the correct default for this
            // case set.
            debug_assert!(std_wall_bytes.is_empty());
            debug_assert!(ut_local_bytes.is_empty());
        }
        Ok(rest)
    }

    fn parse_footer<'b>(
        &mut self,
        _header: &Header,
        bytes: &'b [u8],
    ) -> Result<&'b [u8], Error> {
        if bytes.is_empty() {
            return Err(err!(
                "invalid V2+ TZif footer, expected \\n, \
                 but found unexpected end of data",
            ));
        }
        if bytes[0] != b'\n' {
            return Err(err!(
                "invalid V2+ TZif footer, expected {:?}, but found {:?}",
                Byte(b'\n'),
                Byte(bytes[0]),
            ));
        }
        let bytes = &bytes[1..];
        // Only scan up to 1KB for a NUL terminator in case we somehow got
        // passed a huge block of bytes.
        let toscan = &bytes[..bytes.len().min(1024)];
        let Some(nlat) = toscan.iter().position(|&b| b == b'\n') else {
            return Err(err!(
                "invalid V2 TZif footer, could not find {:?} \
                 terminator in: {:?}",
                Byte(b'\n'),
                Bytes(toscan),
            ));
        };
        let (bytes, rest) = bytes.split_at(nlat);
        if !bytes.is_empty() {
            // We could in theory limit TZ strings to their strict POSIX
            // definition here for TZif V2, but I don't think there is any
            // harm in allowing the extensions in V2 formatted TZif data. Note
            // that the GNU tooling allow it via the `TZ` environment variable
            // even though POSIX doesn't specify it. This all seems okay to me
            // because the V3+ extension is a strict superset of functionality.
            let iana_tz = IanaTz::parse_v3plus(bytes)?;
            self.posix_tz = Some(iana_tz.into_tz());
        }
        Ok(&rest[1..])
    }

    /// This sets the wall clock times for each transition.
    ///
    /// The wall clock time corresponds to time on the clock that the
    /// transition begins. That is, it is the time offset by the previous
    /// transition's offset.
    ///
    /// This also computes whether there is a gap or fold or neither between
    /// each transition. This is used to resolve ambiguous timestamps when
    /// given a civil datetime.
    fn set_wall_datetimes(&mut self) {
        let mut prev = self.local_time_type(self.first_transition()).offset;
        // We iterate over indices instead of `transitions.iter_mut()` because
        // of the borrow checker breaking composition.
        for i in 0..self.transitions.len() {
            let this = self.local_time_type(&self.transitions[i]).offset;
            let t = &mut self.transitions[i];
            t.wall = if prev == this {
                // Equivalent offsets means there can never be any ambiguity.
                let start = prev.to_datetime(t.timestamp);
                TransitionWall::Unambiguous { start }
            } else if prev < this {
                // When the offset of the previous transition is less, that
                // means there is some non-zero amount of time that is
                // "skipped" when moving to the next transition. Thus, we have
                // a gap. The start of the gap is the offset which gets us the
                // earliest time, i.e., the smaller of the two offsets.
                let start = prev.to_datetime(t.timestamp);
                let end = this.to_datetime(t.timestamp);
                TransitionWall::Gap { start, end }
            } else {
                // When the offset of the previous transition is greater, that
                // means there is some non-zero amount of time that will be
                // replayed on a wall clock in this time zone. Thus, we have
                // a fold. The start of the gold is the offset which gets us
                // the earliest time, i.e., the smaller of the two offsets.
                assert!(prev > this);
                let start = this.to_datetime(t.timestamp);
                let end = prev.to_datetime(t.timestamp);
                TransitionWall::Fold { start, end }
            };
            prev = this;
        }
    }
}

impl Eq for Tzif {}

impl PartialEq for Tzif {
    fn eq(&self, rhs: &Tzif) -> bool {
        self.name == rhs.name && self.checksum == rhs.checksum
    }
}

/// A transition to a different offset.
#[derive(Clone, Debug, Eq, PartialEq)]
struct Transition {
    /// The UNIX leap time at which the transition starts. The transition
    /// continues up to and _not_ including the next transition.
    timestamp: Timestamp,
    /// The wall clock time for when this transition begins. This includes
    /// boundary conditions for quickly determining if a given wall clock time
    /// is ambiguous (i.e., falls in a gap or a fold).
    wall: TransitionWall,
    /// The index into the sequence of local time type records. This is what
    /// provides the correct offset (from UTC) that is active beginning at
    /// this transition.
    type_index: u8,
}

/// The wall clock time for when a transition begins.
///
/// This explicitly represents ambiguous wall clock times that occur at the
/// boundaries of transitions.
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
#[derive(Clone, Debug, Eq, PartialEq)]
enum TransitionWall {
    /// This transition cannot possibly lead to an unambiguous offset because
    /// its offset is equivalent to the offset of the previous transition.
    Unambiguous {
        /// The civil datetime corresponding to the beginning of this
        /// transition, inclusive.
        start: DateTime,
    },
    /// This occurs when this transition's offset is strictly greater than the
    /// previous transition's offset. This effectively results in a "gap" of
    /// time equal to the difference in the offsets between the two
    /// transitions.
    Gap {
        /// The start of a gap (inclusive) in wall clock time.
        start: DateTime,
        /// The end of the gap (exclusive) in wall clock time.
        end: DateTime,
    },
    /// This occurs when this transition's offset is strictly less than the
    /// previous transition's offset. This results in a "fold" of time where
    /// the two transitions have an overlap where it is ambiguous which one
    /// applies given a wall clock time. In effect, a span of time equal to the
    /// difference in the offsets is repeated.
    Fold {
        /// The start of the fold (inclusive) in wall clock time.
        start: DateTime,
        /// The end of the fold (exclusive) in wall clock time.
        end: DateTime,
    },
}

impl TransitionWall {
    fn start(&self) -> DateTime {
        match *self {
            TransitionWall::Unambiguous { start } => start,
            TransitionWall::Gap { start, .. } => start,
            TransitionWall::Fold { start, .. } => start,
        }
    }
}

/// A single local time type.
///
/// Basically, this is what transition times map to. Once you have a local time
/// type, then you know the offset, whether it's in DST and the corresponding
/// abbreviation. (There is also an "indicator," but I have no clue what it
/// means. See the `Indicator` type for a rant.)
#[derive(Clone, Debug, Eq, PartialEq)]
struct LocalTimeType {
    offset: Offset,
    is_dst: Dst,
    designation: Range<u8>,
    indicator: Indicator,
}

impl LocalTimeType {
    fn designation(&self) -> Range<usize> {
        usize::from(self.designation.start)..usize::from(self.designation.end)
    }
}

/// This enum corresponds to the possible indicator values for standard/wall
/// and UT/local.
///
/// Note that UT+Wall is not allowed.
///
/// I honestly have no earthly clue what they mean. I've read the section about
/// them in RFC 8536 several times and I can't make sense of it. I've even
/// looked at data files that have these set and still can't make sense of
/// them. I've even looked at what other datetime libraries do with these, and
/// they all seem to just ignore them. Like, WTF. I've spent the last couple
/// months of my life steeped in time, and I just cannot figure this out. Am I
/// just dumb?
///
/// Anyway, we parse them, but otherwise ignore them because that's what all
/// the cool kids do.
///
/// The default is `LocalWall`, which also occurs when no indicators are
/// present.
///
/// I tried again and still don't get it. Here's a dump for `Pacific/Honolulu`:
///
/// ```text
/// $ ./scripts/jiff-debug tzif /usr/share/zoneinfo/Pacific/Honolulu
/// TIME ZONE NAME
///   /usr/share/zoneinfo/Pacific/Honolulu
/// LOCAL TIME TYPES
///   000: offset=-10:31:26, is_dst=false, designation=LMT, indicator=local/wall
///   001: offset=-10:30, is_dst=false, designation=HST, indicator=local/wall
///   002: offset=-09:30, is_dst=true, designation=HDT, indicator=local/wall
///   003: offset=-09:30, is_dst=true, designation=HWT, indicator=local/wall
///   004: offset=-09:30, is_dst=true, designation=HPT, indicator=ut/std
///   005: offset=-10, is_dst=false, designation=HST, indicator=local/wall
/// TRANSITIONS
///   0000: -9999-01-02T01:59:59 :: -377705023201 :: type=0, -10:31:26, is_dst=false, LMT, local/wall
///   0001: 1896-01-13T22:31:26 :: -2334101314 :: type=1, -10:30, is_dst=false, HST, local/wall
///   0002: 1933-04-30T12:30:00 :: -1157283000 :: type=2, -09:30, is_dst=true, HDT, local/wall
///   0003: 1933-05-21T21:30:00 :: -1155436200 :: type=1, -10:30, is_dst=false, HST, local/wall
///   0004: 1942-02-09T12:30:00 :: -880198200 :: type=3, -09:30, is_dst=true, HWT, local/wall
///   0005: 1945-08-14T23:00:00 :: -769395600 :: type=4, -09:30, is_dst=true, HPT, ut/std
///   0006: 1945-09-30T11:30:00 :: -765376200 :: type=1, -10:30, is_dst=false, HST, local/wall
///   0007: 1947-06-08T12:30:00 :: -712150200 :: type=5, -10, is_dst=false, HST, local/wall
/// POSIX TIME ZONE STRING
///   HST10
/// ```
///
/// See how type 004 has a ut/std indicator? What the fuck does that mean?
/// All transitions are defined in terms of UTC. I confirmed this with `zdump`:
///
/// ```text
/// $ zdump -v Pacific/Honolulu | rg 1945
/// Pacific/Honolulu  Tue Aug 14 22:59:59 1945 UT = Tue Aug 14 13:29:59 1945 HWT isdst=1 gmtoff=-34200
/// Pacific/Honolulu  Tue Aug 14 23:00:00 1945 UT = Tue Aug 14 13:30:00 1945 HPT isdst=1 gmtoff=-34200
/// Pacific/Honolulu  Sun Sep 30 11:29:59 1945 UT = Sun Sep 30 01:59:59 1945 HPT isdst=1 gmtoff=-34200
/// Pacific/Honolulu  Sun Sep 30 11:30:00 1945 UT = Sun Sep 30 01:00:00 1945 HST isdst=0 gmtoff=-37800
/// ```
///
/// The times match up. All of them. The indicators don't seem to make a
/// difference. I'm clearly missing something.
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum Indicator {
    LocalWall,
    LocalStandard,
    UTStandard,
}

impl core::fmt::Display for Indicator {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        match *self {
            Indicator::LocalWall => write!(f, "local/wall"),
            Indicator::LocalStandard => write!(f, "local/std"),
            Indicator::UTStandard => write!(f, "ut/std"),
        }
    }
}

/// A leap second "correction" record.
#[derive(Clone, Debug, Eq, PartialEq)]
struct LeapSecond {
    /// The Unix leap time at which the leap second occurred.
    occurrence: Timestamp,
    /// The leap second offset. Usually +1 or -1.
    correction: i32,
}

/// The header for a TZif formatted file.
///
/// V2+ TZif format have two headers: one for V1 data, and then a second
/// following the V1 data block that describes another data block which uses
/// 64-bit timestamps. The two headers both have the same format and both
/// use 32-bit big-endian encoded integers.
#[derive(Debug)]
struct Header {
    /// The size of the timestamps encoded in the data block.
    ///
    /// This is guaranteed to be either 4 (for V1) or 8 (for the 64-bit header
    /// block in V2+).
    time_size: usize,
    /// The file format version.
    ///
    /// Note that this is either a NUL byte (for version 1), or an ASCII byte
    /// corresponding to the version number. That is, `0x32` for `2`, `0x33`
    /// for `3` or `0x34` for `4`. Note also that just because zoneinfo might
    /// have been recently generated does not mean it uses the latest format
    /// version. It seems like newer versions are only compiled by `zic` when
    /// they are needed. For example, `America/New_York` on my system (as of
    /// `2024-03-25`) has version `0x32`, but `Asia/Jerusalem` has version
    /// `0x33`.
    version: u8,
    /// Number of UT/local indicators stored in the file.
    ///
    /// This is checked to be either equal to `0` or equal to `tzh_typecnt`.
    tzh_ttisutcnt: usize,
    /// The number of standard/wall indicators stored in the file.
    ///
    /// This is checked to be either equal to `0` or equal to `tzh_typecnt`.
    tzh_ttisstdcnt: usize,
    /// The number of leap seconds for which data entries are stored in the
    /// file.
    tzh_leapcnt: usize,
    /// The number of transition times for which data entries are stored in
    /// the file.
    tzh_timecnt: usize,
    /// The number of local time types for which data entries are stored in the
    /// file.
    ///
    /// This is checked to be at least `1`.
    tzh_typecnt: usize,
    /// The number of bytes of time zone abbreviation strings stored in the
    /// file.
    ///
    /// This is checked to be at least `1`.
    tzh_charcnt: usize,
}

impl Header {
    /// Parse the header record from the given bytes.
    ///
    /// Upon success, return the header and all bytes after the header.
    ///
    /// The given `time_size` must be 4 or 8, corresponding to either the
    /// V1 header block or the V2+ header block, respectively.
    fn parse(
        time_size: usize,
        bytes: &[u8],
    ) -> Result<(Header, &[u8]), Error> {
        assert!(time_size == 4 || time_size == 8, "time size must be 4 or 8");
        if bytes.len() < 44 {
            return Err(err!("invalid header: too short"));
        }
        let (magic, rest) = bytes.split_at(4);
        if magic != b"TZif" {
            return Err(err!("invalid header: magic bytes mismatch"));
        }
        let (version, rest) = rest.split_at(1);
        let (_reserved, rest) = rest.split_at(15);

        let (tzh_ttisutcnt_bytes, rest) = rest.split_at(4);
        let (tzh_ttisstdcnt_bytes, rest) = rest.split_at(4);
        let (tzh_leapcnt_bytes, rest) = rest.split_at(4);
        let (tzh_timecnt_bytes, rest) = rest.split_at(4);
        let (tzh_typecnt_bytes, rest) = rest.split_at(4);
        let (tzh_charcnt_bytes, rest) = rest.split_at(4);

        let tzh_ttisutcnt = from_be_bytes_u32_to_usize(tzh_ttisutcnt_bytes)
            .map_err(|e| e.context("failed to parse tzh_ttisutcnt"))?;
        let tzh_ttisstdcnt = from_be_bytes_u32_to_usize(tzh_ttisstdcnt_bytes)
            .map_err(|e| e.context("failed to parse tzh_ttisstdcnt"))?;
        let tzh_leapcnt = from_be_bytes_u32_to_usize(tzh_leapcnt_bytes)
            .map_err(|e| e.context("failed to parse tzh_leapcnt"))?;
        let tzh_timecnt = from_be_bytes_u32_to_usize(tzh_timecnt_bytes)
            .map_err(|e| e.context("failed to parse tzh_timecnt"))?;
        let tzh_typecnt = from_be_bytes_u32_to_usize(tzh_typecnt_bytes)
            .map_err(|e| e.context("failed to parse tzh_typecnt"))?;
        let tzh_charcnt = from_be_bytes_u32_to_usize(tzh_charcnt_bytes)
            .map_err(|e| e.context("failed to parse tzh_charcnt"))?;

        if tzh_ttisutcnt != 0 && tzh_ttisutcnt != tzh_typecnt {
            return Err(err!(
                "expected tzh_ttisutcnt={tzh_ttisutcnt} to be zero \
                 or equal to tzh_typecnt={tzh_typecnt}",
            ));
        }
        if tzh_ttisstdcnt != 0 && tzh_ttisstdcnt != tzh_typecnt {
            return Err(err!(
                "expected tzh_ttisstdcnt={tzh_ttisstdcnt} to be zero \
                 or equal to tzh_typecnt={tzh_typecnt}",
            ));
        }
        if tzh_typecnt < 1 {
            return Err(err!(
                "expected tzh_typecnt={tzh_typecnt} to be at least 1",
            ));
        }
        if tzh_charcnt < 1 {
            return Err(err!(
                "expected tzh_charcnt={tzh_charcnt} to be at least 1",
            ));
        }

        let header = Header {
            time_size,
            version: version[0],
            tzh_ttisutcnt,
            tzh_ttisstdcnt,
            tzh_leapcnt,
            tzh_timecnt,
            tzh_typecnt,
            tzh_charcnt,
        };
        Ok((header, rest))
    }

    /// Returns true if this header is for a 32-bit data block.
    ///
    /// When false, it is guaranteed that this header is for a 64-bit data
    /// block.
    fn is_32bit(&self) -> bool {
        self.time_size == 4
    }

    /// Returns the size of the data block, in bytes, for this header.
    ///
    /// This returns an error if the arithmetic required to compute the
    /// length would overflow.
    ///
    /// This is useful for, e.g., skipping over the 32-bit V1 data block in
    /// V2+ TZif formatted files.
    fn data_block_len(&self) -> Result<usize, Error> {
        let a = self.transition_times_len()?;
        let b = self.transition_types_len()?;
        let c = self.local_time_types_len()?;
        let d = self.time_zone_designations_len()?;
        let e = self.leap_second_len()?;
        let f = self.standard_wall_len()?;
        let g = self.ut_local_len()?;
        a.checked_add(b)
            .and_then(|z| z.checked_add(c))
            .and_then(|z| z.checked_add(d))
            .and_then(|z| z.checked_add(e))
            .and_then(|z| z.checked_add(f))
            .and_then(|z| z.checked_add(g))
            .ok_or_else(|| {
                err!(
                    "length of data block in V{} tzfile is too big",
                    self.version
                )
            })
    }

    fn transition_times_len(&self) -> Result<usize, Error> {
        self.tzh_timecnt.checked_mul(self.time_size).ok_or_else(|| {
            err!("tzh_timecnt value {} is too big", self.tzh_timecnt)
        })
    }

    fn transition_types_len(&self) -> Result<usize, Error> {
        Ok(self.tzh_timecnt)
    }

    fn local_time_types_len(&self) -> Result<usize, Error> {
        self.tzh_typecnt.checked_mul(6).ok_or_else(|| {
            err!("tzh_typecnt value {} is too big", self.tzh_typecnt)
        })
    }

    fn time_zone_designations_len(&self) -> Result<usize, Error> {
        Ok(self.tzh_charcnt)
    }

    fn leap_second_len(&self) -> Result<usize, Error> {
        let record_len = self
            .time_size
            .checked_add(4)
            .expect("4-or-8 plus 4 always fits in usize");
        self.tzh_leapcnt.checked_mul(record_len).ok_or_else(|| {
            err!("tzh_leapcnt value {} is too big", self.tzh_leapcnt)
        })
    }

    fn standard_wall_len(&self) -> Result<usize, Error> {
        Ok(self.tzh_ttisstdcnt)
    }

    fn ut_local_len(&self) -> Result<usize, Error> {
        Ok(self.tzh_ttisutcnt)
    }
}

/// Does a quick check that returns true if the data might be in TZif format.
///
/// It is possible that this returns true even if the given data is not in TZif
/// format. However, it is impossible for this to return false when the given
/// data is TZif. That is, a false positive is allowed but a false negative is
/// not.
#[cfg(feature = "tzdb-zoneinfo")]
pub(crate) fn is_possibly_tzif(data: &[u8]) -> bool {
    data.starts_with(b"TZif")
}

/// Interprets the given slice as an unsigned 32-bit big endian integer,
/// attempts to convert it to a `usize` and returns it.
///
/// # Panics
///
/// When `bytes.len() != 4`.
///
/// # Errors
///
/// This errors if the `u32` parsed from the given bytes cannot fit in a
/// `usize`.
fn from_be_bytes_u32_to_usize(bytes: &[u8]) -> Result<usize, Error> {
    let n = from_be_bytes_u32(bytes);
    usize::try_from(n).map_err(|_| {
        err!(
            "failed to parse integer {n} (too big, max allowed is {}",
            usize::MAX
        )
    })
}

/// Interprets the given slice as an unsigned 32-bit big endian integer and
/// returns it.
///
/// # Panics
///
/// When `bytes.len() != 4`.
fn from_be_bytes_u32(bytes: &[u8]) -> u32 {
    u32::from_be_bytes(bytes.try_into().unwrap())
}

/// Interprets the given slice as a signed 32-bit big endian integer and
/// returns it.
///
/// # Panics
///
/// When `bytes.len() != 4`.
fn from_be_bytes_i32(bytes: &[u8]) -> i32 {
    i32::from_be_bytes(bytes.try_into().unwrap())
}

/// Interprets the given slice as a signed 64-bit big endian integer and
/// returns it.
///
/// # Panics
///
/// When `bytes.len() != 8`.
fn from_be_bytes_i64(bytes: &[u8]) -> i64 {
    i64::from_be_bytes(bytes.try_into().unwrap())
}

/// Splits the given slice of bytes at the index given.
///
/// If the index is out of range (greater than `bytes.len()`) then an error is
/// returned. The error message will include the `what` string given, which is
/// meant to describe the thing being split.
fn try_split_at<'b>(
    what: &'static str,
    bytes: &'b [u8],
    at: usize,
) -> Result<(&'b [u8], &'b [u8]), Error> {
    if at > bytes.len() {
        Err(err!(
            "expected at least {at} bytes for {what}, \
             but found only {} bytes",
            bytes.len(),
        ))
    } else {
        Ok(bytes.split_at(at))
    }
}

#[cfg(test)]
mod tests {
    use alloc::string::ToString;

    use crate::tz::testdata::TZIF_TEST_FILES;

    use super::*;

    /// This converts TZif data into a human readable format.
    ///
    /// This is useful for debugging (via `./scripts/jiff-debug tzif`), but we
    /// also use it for snapshot testing to make reading the test output at
    /// least *somewhat* comprehensible for humans. Otherwise, one needs to
    /// read and understand Unix timestamps. That ain't going to fly.
    ///
    /// For this to work, we make sure everything in a `Tzif` value is
    /// represented in some way in this output.
    fn tzif_to_human_readable(tzif: &Tzif) -> String {
        use std::io::Write;

        let mut out = tabwriter::TabWriter::new(vec![])
            .alignment(tabwriter::Alignment::Left);

        writeln!(out, "TIME ZONE NAME").unwrap();
        writeln!(out, "  {}", tzif.name().unwrap_or("UNNAMED")).unwrap();

        writeln!(out, "TIME ZONE VERSION").unwrap();
        writeln!(out, "  {}", char::try_from(tzif.version).unwrap()).unwrap();

        writeln!(out, "LOCAL TIME TYPES").unwrap();
        for (i, typ) in tzif.types.iter().enumerate() {
            writeln!(
                out,
                "  {i:03}:\toffset={off}\t\
                   designation={desig}\t{dst}\tindicator={ind}",
                off = typ.offset,
                desig = tzif.designation(&typ),
                dst = if typ.is_dst.is_dst() { "dst" } else { "" },
                ind = typ.indicator,
            )
            .unwrap();
        }
        if !tzif.transitions.is_empty() {
            writeln!(out, "TRANSITIONS").unwrap();
            for (i, t) in tzif.transitions.iter().enumerate() {
                let dt = Offset::UTC.to_datetime(t.timestamp);
                let typ = &tzif.types[usize::from(t.type_index)];
                let wall = alloc::format!("{:?}", t.wall.start());
                let ambiguous = match t.wall {
                    TransitionWall::Unambiguous { .. } => {
                        "unambiguous".to_string()
                    }
                    TransitionWall::Gap { end, .. } => {
                        alloc::format!(" gap-until({end:?})")
                    }
                    TransitionWall::Fold { end, .. } => {
                        alloc::format!("fold-until({end:?})")
                    }
                };

                writeln!(
                    out,
                    "  {i:04}:\t{dt:?}Z\tunix={ts}\twall={wall}\t\
                       {ambiguous}\t\
                       type={type_index}\t{off}\t\
                       {desig}\t{dst}",
                    ts = t.timestamp.as_second(),
                    type_index = t.type_index,
                    off = typ.offset,
                    desig = tzif.designation(typ),
                    dst = if typ.is_dst.is_dst() { "dst" } else { "" },
                )
                .unwrap();
            }
        }
        if !tzif.leap_seconds.is_empty() {
            writeln!(out, "LEAP SECONDS").unwrap();
            for ls in tzif.leap_seconds.iter() {
                let dt = Offset::UTC.to_datetime(ls.occurrence);
                let c = ls.correction;
                writeln!(out, "  {dt:?}\tcorrection={c}").unwrap();
            }
        }
        if let Some(ref posix_tz) = tzif.posix_tz {
            writeln!(out, "POSIX TIME ZONE STRING").unwrap();
            writeln!(out, "  {}", posix_tz).unwrap();
        }
        String::from_utf8(out.into_inner().unwrap()).unwrap()
    }

    /// DEBUG COMMAND
    ///
    /// Takes environment variable `JIFF_DEBUG_TZIF_PATH` as input, and treats
    /// the value as a TZif file path. This test will open the file, parse it
    /// as a TZif and then dump debug data about the file in a human readable
    /// plain text format.
    #[cfg(feature = "std")]
    #[test]
    fn debug_tzif() -> anyhow::Result<()> {
        use anyhow::Context;

        let _ = crate::logging::Logger::init();

        const ENV: &str = "JIFF_DEBUG_TZIF_PATH";
        let Some(val) = std::env::var_os(ENV) else { return Ok(()) };
        let Ok(val) = val.into_string() else {
            anyhow::bail!("{ENV} has invalid UTF-8")
        };
        let bytes =
            std::fs::read(&val).with_context(|| alloc::format!("{val:?}"))?;
        let tzif = Tzif::parse(Some(val.to_string()), &bytes)?;
        std::eprint!("{}", tzif_to_human_readable(&tzif));
        Ok(())
    }

    #[test]
    fn tzif_parse_v2plus() {
        for tzif_test in TZIF_TEST_FILES {
            insta::assert_snapshot!(
                alloc::format!("{}_v2+", tzif_test.name),
                tzif_to_human_readable(&tzif_test.parse())
            );
        }
    }

    #[test]
    fn tzif_parse_v1() {
        for tzif_test in TZIF_TEST_FILES {
            insta::assert_snapshot!(
                alloc::format!("{}_v1", tzif_test.name),
                tzif_to_human_readable(&tzif_test.parse_v1())
            );
        }
    }

    /// This tests walks the /usr/share/zoneinfo directory (if it exists) and
    /// tries to parse every TZif formatted file it can find. We don't really
    /// do much with it other than to ensure we don't panic or return an error.
    /// That is, we check that we can parse each file, but not that we do so
    /// correctly.
    #[cfg(feature = "tzdb-zoneinfo")]
    #[cfg(target_os = "linux")]
    #[test]
    fn zoneinfo() {
        const TZDIR: &str = "/usr/share/zoneinfo";

        for result in walkdir::WalkDir::new(TZDIR) {
            // Just skip if we got an error traversing the directory tree.
            // These aren't related to our parsing, so it's some other problem
            // (like the directory not existing).
            let Ok(dent) = result else { continue };
            // This test can take some time in debug mode, so skip parsing
            // some of the less frequently used TZif files.
            let Some(name) = dent.path().to_str() else { continue };
            if name.contains("right/") || name.contains("posix/") {
                continue;
            }
            // Again, skip if we can't read. Not my monkeys, not my circus.
            let Ok(bytes) = std::fs::read(dent.path()) else { continue };
            if !is_possibly_tzif(&bytes) {
                continue;
            }
            let tzname = dent
                .path()
                .strip_prefix(TZDIR)
                .unwrap_or_else(|_| {
                    panic!("all paths in TZDIR have {TZDIR:?} prefix")
                })
                .to_str()
                .expect("all paths to be valid UTF-8")
                .to_string();
            // OK at this point, we're pretty sure `bytes` should be a TZif
            // binary file. So try to parse it and fail the test if it fails.
            if let Err(err) = Tzif::parse(Some(tzname), &bytes) {
                panic!("failed to parse TZif file {:?}: {err}", dent.path());
            }
        }
    }
}
