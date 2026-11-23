use crate::{
    civil::DateTime,
    tz::{
        posix, Abbreviation, AmbiguousOffset, AmbiguousTimestamp, Offset,
        OffsetInfo, Transition,
    },
    Timestamp,
};

use super::{
    DateTime as TzifDateTime, LocalTimeType, TimeZone,
    Timestamp as TzifTimestamp, TransitionInfo as TzifTransitionInfo,
    TransitionKind,
};

impl TimeZone {
    /// Returns the appropriate time zone offset to use for the given
    /// timestamp.
    pub fn to_offset(&self, timestamp: Timestamp) -> Offset {
        match self.to_local_time_type(timestamp) {
            Ok(typ) => typ.offset,
            Err(tz) => tz.to_offset(timestamp),
        }
    }

    /// Returns the appropriate time zone offset to use for the given
    /// timestamp.
    ///
    /// This also includes whether the offset returned should be considered to
    /// be DST or not, along with the time zone abbreviation (e.g., EST for
    /// standard time in New York, and EDT for DST in New York).
    pub fn to_offset_info(&self, timestamp: Timestamp) -> OffsetInfo {
        let typ = match self.to_local_time_type(timestamp) {
            Ok(typ) => typ,
            Err(tz) => return tz.to_offset_info(timestamp),
        };
        // This clone will generally just be a memcpy. It only does a heap
        // alloc when the designation is unusually long. (Which should be never
        // for standard tzdata.)
        let abbreviation = self.designation(typ).clone();
        OffsetInfo { offset: typ.offset, abbreviation, dst: typ.dst }
    }

    /// Returns the local time type for the timestamp given.
    ///
    /// If one could not be found, then this implies that the caller should
    /// use the POSIX time zone returned in the error variant.
    fn to_local_time_type(
        &self,
        timestamp: Timestamp,
    ) -> Result<&LocalTimeType, &posix::TimeZone> {
        let timestamp = TzifTimestamp::new(timestamp);
        // This is guaranteed because we always push at least one transition.
        // This isn't guaranteed by TZif since it might have 0 transitions,
        // but we always add a "dummy" first transition with our minimum
        // `Timestamp` value. TZif doesn't do this because there is no
        // universal minimum timestamp. (`i64::MIN` is a candidate, but that's
        // likely to cause overflow in readers that don't do error checking.)
        //
        // The result of the dummy transition is that the code below is simpler
        // with fewer special cases.
        let timestamps = self.timestamps();
        let last = *timestamps.last().expect("non-empty transitions");
        let index = if timestamp > last {
            timestamps.len() - 1
        } else {
            let search = self.timestamps().binary_search(&timestamp);
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
        debug_assert!(index < timestamps.len());
        // RFC 8536 says: "Local time for timestamps on or after the last
        // transition is specified by the TZ string in the footer (Section 3.3)
        // if present and nonempty; otherwise, it is unspecified."
        //
        // Subtracting 1 is OK because we know self.transitions is not empty.
        let index = if index < timestamps.len() - 1 {
            // This is the typical case in "fat" TZif files: we found a
            // matching transition.
            index
        } else {
            match self.posix_tz() {
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
                Some(tz) => return Err(tz),
                // This case is technically unspecified, but I think the
                // typical thing to do is to just use the last transition.
                // I'm not 100% sure on this one.
                None => index,
            }
        };
        Ok(self.local_time_type(index))
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
    pub fn to_ambiguous_timestamp(&self, dt: DateTime) -> AmbiguousTimestamp {
        // This implementation very nearly mirrors `to_local_time_type`
        // above in the beginning: we do a binary search to find transition
        // applicable for the given datetime. Except, we do it on wall clock
        // times instead of timestamps. And in particular, each transition
        // begins with a possibly ambiguous range of wall clock times
        // corresponding to either a "gap" or "fold" in time.
        let dtt = TzifDateTime::new(dt);
        let (starts, ends) = (self.civil_starts(), self.civil_ends());
        assert!(!starts.is_empty(), "transitions is non-empty");
        let this_index = match starts.binary_search(&dtt) {
            Err(0) => unreachable!("impossible to come before DateTime::MIN"),
            Ok(i) => i,
            Err(i) => i.checked_sub(1).expect("i is non-zero"),
        };
        debug_assert!(this_index < starts.len());

        let this_offset = self.local_time_type(this_index).offset;
        // This is a little tricky, but we need to check for ambiguous civil
        // datetimes before possibly using the POSIX TZ string. Namely, a
        // datetime could be ambiguous with respect to the last transition,
        // and we should handle that according to the gap/fold determined for
        // that transition. We cover this case in tests in tz/mod.rs for the
        // Pacific/Honolulu time zone, whose last transition begins with a gap.
        match self.transition_kind(this_index) {
            TransitionKind::Gap if dtt < ends[this_index] => {
                // A gap/fold can only appear when there exists a previous
                // transition.
                let prev_index = this_index.checked_sub(1).unwrap();
                let prev_offset = self.local_time_type(prev_index).offset;
                return AmbiguousOffset::Gap {
                    before: prev_offset,
                    after: this_offset,
                }
                .into_ambiguous_timestamp(dt);
            }
            TransitionKind::Fold if dtt < ends[this_index] => {
                // A gap/fold can only appear when there exists a previous
                // transition.
                let prev_index = this_index.checked_sub(1).unwrap();
                let prev_offset = self.local_time_type(prev_index).offset;
                return AmbiguousOffset::Fold {
                    before: prev_offset,
                    after: this_offset,
                }
                .into_ambiguous_timestamp(dt);
            }
            _ => {}
        }
        // The datetime given is not ambiguous with respect to any of the
        // transitions in the TZif data. But, if we matched at or after the
        // last transition, then we need to use the POSIX TZ string (which
        // could still return an ambiguous offset).
        if this_index == starts.len() - 1 {
            if let Some(tz) = self.posix_tz() {
                return tz.to_ambiguous_timestamp(dt);
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
            .into_ambiguous_timestamp(dt)
    }

    /// Returns the timestamp of the most recent time zone transition prior
    /// to the timestamp given. If one doesn't exist, `None` is returned.
    pub fn previous_transition<'t>(
        &'t self,
        ts: Timestamp,
    ) -> Option<Transition> {
        assert!(!self.timestamps().is_empty(), "transitions is non-empty");
        let mut timestamp = TzifTimestamp::new(ts);
        if ts.subsec_nanosecond() != 0 {
            timestamp = timestamp.saturating_add(1);
        }
        let search = self.timestamps().binary_search(&timestamp);
        let index = match search {
            Ok(i) | Err(i) => i.checked_sub(1)?,
        };
        let index = if index == 0 {
            // The first transition is a dummy that we insert, so if we land on
            // it here, treat it as if it doesn't exist.
            return None;
        } else if index == self.timestamps().len() - 1 {
            if let Some(ref posix_tz) = self.posix_tz() {
                // Since the POSIX TZ must be consistent with the last
                // transition, it must be the case that tzif_last <=
                // posix_prev_trans in all cases. So the transition according
                // to the POSIX TZ is always correct here.
                //
                // What if this returns `None` though? I'm not sure in which
                // cases that could matter, and I think it might be a violation
                // of the TZif format if it does.
                //
                // It can return `None`! In the case of a time zone that
                // has eliminated DST, it might have historical time zone
                // transitions but a POSIX time zone without DST. (For example,
                // `America/Sao_Paulo`.) And thus, this would return `None`.
                // So if it does, we pretend as if the POSIX time zone doesn't
                // exist.
                if let Some(trans) = posix_tz.previous_transition(ts) {
                    return Some(trans);
                }
            }
            index
        } else {
            index
        };
        let timestamp = self.timestamps()[index];
        let typ = self.local_time_type(index);
        let info = OffsetInfo {
            offset: typ.offset,
            abbreviation: self.designation(typ).clone(),
            dst: typ.dst,
        };
        Some(Transition { timestamp: timestamp.to_standard_timestamp(), info })
    }

    /// Returns the timestamp of the soonest time zone transition after the
    /// timestamp given. If one doesn't exist, `None` is returned.
    pub fn next_transition<'t>(&'t self, ts: Timestamp) -> Option<Transition> {
        assert!(!self.timestamps().is_empty(), "transitions is non-empty");
        let timestamp = TzifTimestamp::new(ts);
        let search = self.timestamps().binary_search(&timestamp);
        let index = match search {
            Ok(i) => i.checked_add(1)?,
            Err(i) => i,
        };
        let index = if index == 0 {
            // The first transition is a dummy that we insert, so if we land on
            // it here, treat it as if it doesn't exist.
            return None;
        } else if index >= self.timestamps().len() {
            if let Some(posix_tz) = self.posix_tz() {
                // Since the POSIX TZ must be consistent with the last
                // transition, it must be the case that next.timestamp <=
                // posix_next_tans in all cases. So the transition according to
                // the POSIX TZ is always correct here.
                //
                // What if this returns `None` though? I'm not sure in which
                // cases that could matter, and I think it might be a violation
                // of the TZif format if it does.
                //
                // In the "previous" case above, this could return `None` even
                // when there are historical time zone transitions in the case
                // of a time zone eliminating DST (e.g., `America/Sao_Paulo`).
                // But unlike the previous case, if we get `None` here, then
                // that is the real answer because there are no other known
                // future time zone transitions.
                //
                // 2025-05-05: OK, this could return `None` and this is fine.
                // It happens for time zones that had DST but then stopped
                // it at some point in the past. The POSIX time zone has no
                // DST and thus returns `None`. That's fine. But there was a
                // problem: we were using the POSIX time zone even when there
                // was a historical time zone transition after the timestamp
                // given. That was fixed by changing the condition when we get
                // here: it can only happen when the timestamp given comes at
                // or after all historical time zone transitions.
                return posix_tz.next_transition(ts);
            }
            self.timestamps().len() - 1
        } else {
            index
        };
        let timestamp = self.timestamps()[index];
        let typ = self.local_time_type(index);
        let info = OffsetInfo {
            offset: typ.offset,
            abbreviation: self.designation(typ).clone(),
            dst: typ.dst,
        };
        Some(Transition { timestamp: timestamp.to_standard_timestamp(), info })
    }

    fn local_time_type(&self, transition_index: usize) -> &LocalTimeType {
        // OK because we require that `type_index` always points to a valid
        // local time type.
        &self.types()[usize::from(self.infos()[transition_index].type_index)]
    }

    fn transition_kind(&self, transition_index: usize) -> TransitionKind {
        self.infos()[transition_index].kind
    }

    fn types(&self) -> &[LocalTimeType] {
        &self.types
    }

    fn timestamps(&self) -> &[TzifTimestamp] {
        &self.transitions.timestamps
    }

    fn civil_starts(&self) -> &[TzifDateTime] {
        &self.transitions.civil_starts
    }

    fn civil_ends(&self) -> &[TzifDateTime] {
        &self.transitions.civil_ends
    }

    fn infos(&self) -> &[TzifTransitionInfo] {
        &self.transitions.infos
    }

    fn designation(&self, typ: &LocalTimeType) -> &Abbreviation {
        // OK because every local time type is assigned a valid designation
        // index while parsing or constructing this time zone.
        &self.designations[typ.designation()]
    }

    fn posix_tz(&self) -> Option<&posix::TimeZone> {
        self.posix_tz.as_ref()
    }
}
