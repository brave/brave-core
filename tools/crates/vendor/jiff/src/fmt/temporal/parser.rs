use crate::{
    civil::{Date, DateTime, Time},
    error::{err, Error, ErrorContext},
    fmt::{
        offset::{self, ParsedOffset},
        rfc9557::{self, ParsedAnnotations},
        temporal::Pieces,
        util::{parse_temporal_fraction, DurationUnits},
        Parsed,
    },
    span::Span,
    tz::{
        AmbiguousZoned, Disambiguation, Offset, OffsetConflict, TimeZone,
        TimeZoneDatabase,
    },
    util::{
        c::Sign,
        escape, parse,
        t::{self, C},
    },
    SignedDuration, Timestamp, Unit, Zoned,
};

/// The datetime components parsed from a string.
#[derive(Debug)]
pub(super) struct ParsedDateTime<'i> {
    /// The original input that the datetime was parsed from.
    input: escape::Bytes<'i>,
    /// A required civil date.
    date: ParsedDate<'i>,
    /// An optional civil time.
    time: Option<ParsedTime<'i>>,
    /// An optional UTC offset.
    offset: Option<ParsedOffset>,
    /// An optional RFC 9557 annotations parsed.
    ///
    /// An empty `ParsedAnnotations` is valid and possible, so this bakes
    /// optionality into the type and doesn't need to be an `Option` itself.
    annotations: ParsedAnnotations<'i>,
}

impl<'i> ParsedDateTime<'i> {
    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn to_pieces(&self) -> Result<Pieces<'i>, Error> {
        let mut pieces = Pieces::from(self.date.date);
        if let Some(ref time) = self.time {
            pieces = pieces.with_time(time.time);
        }
        if let Some(ref offset) = self.offset {
            pieces = pieces.with_offset(offset.to_pieces_offset()?);
        }
        if let Some(ann) = self.annotations.to_time_zone_annotation()? {
            pieces = pieces.with_time_zone_annotation(ann);
        }
        Ok(pieces)
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn to_zoned(
        &self,
        db: &TimeZoneDatabase,
        offset_conflict: OffsetConflict,
        disambiguation: Disambiguation,
    ) -> Result<Zoned, Error> {
        self.to_ambiguous_zoned(db, offset_conflict)?
            .disambiguate(disambiguation)
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn to_ambiguous_zoned(
        &self,
        db: &TimeZoneDatabase,
        offset_conflict: OffsetConflict,
    ) -> Result<AmbiguousZoned, Error> {
        let time = self.time.as_ref().map_or(Time::midnight(), |p| p.time);
        let dt = DateTime::from_parts(self.date.date, time);

        // We always require a time zone when parsing a zoned instant.
        let tz_annotation =
            self.annotations.to_time_zone_annotation()?.ok_or_else(|| {
                err!(
                    "failed to find time zone in square brackets \
                     in {:?}, which is required for parsing a zoned instant",
                    self.input,
                )
            })?;
        let tz = tz_annotation.to_time_zone_with(db)?;

        // If there's no offset, then our only choice, regardless of conflict
        // resolution preference, is to use the time zone. That is, there is no
        // possible conflict.
        let Some(ref parsed_offset) = self.offset else {
            return Ok(tz.into_ambiguous_zoned(dt));
        };
        if parsed_offset.is_zulu() {
            // When `Z` is used, that means the offset to local time is not
            // known. In this case, there really can't be a conflict because
            // there is an explicit acknowledgment that the offset could be
            // anything. So we just always accept `Z` as if it were `UTC` and
            // respect that. If we didn't have this special check, we'd fall
            // below and the `Z` would just be treated as `+00:00`, which would
            // likely result in `OffsetConflict::Reject` raising an error.
            // (Unless the actual correct offset at the time is `+00:00` for
            // the time zone parsed.)
            return OffsetConflict::AlwaysOffset
                .resolve(dt, Offset::UTC, tz)
                .with_context(|| {
                    err!("parsing {input:?} failed", input = self.input)
                });
        }
        let offset = parsed_offset.to_offset()?;
        let is_equal = |parsed: Offset, candidate: Offset| {
            // If they're equal down to the second, then no amount of rounding
            // or whatever should change that.
            if parsed == candidate {
                return true;
            }
            // If the candidate offset we're considering is a whole minute,
            // then we never need rounding.
            //
            // Alternatively, if the parsed offset has an explicit sub-minute
            // component (even if it's zero), we should use exact equality.
            // (The error message for this case when "reject" offset
            // conflict resolution is used is not the best. But this case
            // is stupidly rare, so I'm not sure it's worth the effort to
            // improve the error message. I'd be open to a simple patch
            // though.)
            if candidate.part_seconds_ranged() == C(0)
                || parsed_offset.has_subminute()
            {
                return parsed == candidate;
            }
            let Ok(candidate) = candidate.round(Unit::Minute) else {
                // This is a degenerate case and this is the only sensible
                // thing to do.
                return parsed == candidate;
            };
            parsed == candidate
        };
        offset_conflict.resolve_with(dt, offset, tz, is_equal).with_context(
            || err!("parsing {input:?} failed", input = self.input),
        )
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn to_timestamp(&self) -> Result<Timestamp, Error> {
        let time = self.time.as_ref().map(|p| p.time).ok_or_else(|| {
            err!(
                "failed to find time component in {:?}, \
                 which is required for parsing a timestamp",
                self.input,
            )
        })?;
        let parsed_offset = self.offset.as_ref().ok_or_else(|| {
            err!(
                "failed to find offset component in {:?}, \
                 which is required for parsing a timestamp",
                self.input,
            )
        })?;
        let offset = parsed_offset.to_offset()?;
        let dt = DateTime::from_parts(self.date.date, time);
        let timestamp = offset.to_timestamp(dt).with_context(|| {
            err!(
                "failed to convert civil datetime to timestamp \
                 with offset {offset}",
            )
        })?;
        Ok(timestamp)
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn to_datetime(&self) -> Result<DateTime, Error> {
        if self.offset.as_ref().map_or(false, |o| o.is_zulu()) {
            return Err(err!(
                "cannot parse civil date from string with a Zulu \
                 offset, parse as a `Timestamp` and convert to a civil \
                 datetime instead",
            ));
        }
        Ok(DateTime::from_parts(self.date.date, self.time()))
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn to_date(&self) -> Result<Date, Error> {
        if self.offset.as_ref().map_or(false, |o| o.is_zulu()) {
            return Err(err!(
                "cannot parse civil date from string with a Zulu \
                 offset, parse as a `Timestamp` and convert to a civil \
                 date instead",
            ));
        }
        Ok(self.date.date)
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn time(&self) -> Time {
        self.time.as_ref().map(|p| p.time).unwrap_or(Time::midnight())
    }
}

impl<'i> core::fmt::Display for ParsedDateTime<'i> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Display::fmt(&self.input, f)
    }
}

/// The result of parsing a Gregorian calendar civil date.
#[derive(Debug)]
pub(super) struct ParsedDate<'i> {
    /// The original input that the date was parsed from.
    input: escape::Bytes<'i>,
    /// The actual parsed date.
    date: Date,
}

impl<'i> core::fmt::Display for ParsedDate<'i> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Display::fmt(&self.input, f)
    }
}

/// The result of parsing a 24-hour civil time.
#[derive(Debug)]
pub(super) struct ParsedTime<'i> {
    /// The original input that the time was parsed from.
    input: escape::Bytes<'i>,
    /// The actual parsed time.
    time: Time,
    /// Whether the time was parsed in extended format or not.
    extended: bool,
}

impl<'i> ParsedTime<'i> {
    pub(super) fn to_time(&self) -> Time {
        self.time
    }
}

impl<'i> core::fmt::Display for ParsedTime<'i> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Display::fmt(&self.input, f)
    }
}

#[derive(Debug)]
pub(super) struct ParsedTimeZone<'i> {
    /// The original input that the time zone was parsed from.
    input: escape::Bytes<'i>,
    /// The kind of time zone parsed.
    kind: ParsedTimeZoneKind<'i>,
}

impl<'i> core::fmt::Display for ParsedTimeZone<'i> {
    fn fmt(&self, f: &mut core::fmt::Formatter) -> core::fmt::Result {
        core::fmt::Display::fmt(&self.input, f)
    }
}

#[derive(Debug)]
pub(super) enum ParsedTimeZoneKind<'i> {
    Named(&'i str),
    Offset(ParsedOffset),
    #[cfg(feature = "alloc")]
    Posix(crate::tz::posix::PosixTimeZoneOwned),
}

impl<'i> ParsedTimeZone<'i> {
    pub(super) fn into_time_zone(
        self,
        db: &TimeZoneDatabase,
    ) -> Result<TimeZone, Error> {
        match self.kind {
            ParsedTimeZoneKind::Named(iana_name) => {
                let tz = db.get(iana_name).with_context(|| {
                    err!(
                        "parsed apparent IANA time zone identifier \
                         {iana_name} from {input}, but the tzdb lookup \
                         failed",
                        input = self.input,
                    )
                })?;
                Ok(tz)
            }
            ParsedTimeZoneKind::Offset(poff) => {
                let offset = poff.to_offset().with_context(|| {
                    err!(
                        "offset successfully parsed from {input}, \
                         but failed to convert to numeric `Offset`",
                        input = self.input,
                    )
                })?;
                Ok(TimeZone::fixed(offset))
            }
            #[cfg(feature = "alloc")]
            ParsedTimeZoneKind::Posix(posix_tz) => {
                Ok(TimeZone::from_posix_tz(posix_tz))
            }
        }
    }
}

/// A parser for Temporal datetimes.
#[derive(Debug)]
pub(super) struct DateTimeParser {
    /// There are currently no configuration options for this parser.
    _priv: (),
}

impl DateTimeParser {
    /// Create a new Temporal datetime parser with the default configuration.
    pub(super) const fn new() -> DateTimeParser {
        DateTimeParser { _priv: () }
    }

    // TemporalDateTimeString[Zoned] :::
    //   AnnotatedDateTime[?Zoned]
    //
    // AnnotatedDateTime[Zoned] :::
    //   [~Zoned] DateTime TimeZoneAnnotation[opt] Annotations[opt]
    //   [+Zoned] DateTime TimeZoneAnnotation Annotations[opt]
    //
    // DateTime :::
    //   Date
    //   Date DateTimeSeparator TimeSpec DateTimeUTCOffset[opt]
    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn parse_temporal_datetime<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, ParsedDateTime<'i>>, Error> {
        let mkslice = parse::slicer(input);
        let Parsed { value: date, input } = self.parse_date_spec(input)?;
        if input.is_empty() {
            let value = ParsedDateTime {
                input: escape::Bytes(mkslice(input)),
                date,
                time: None,
                offset: None,
                annotations: ParsedAnnotations::none(),
            };
            return Ok(Parsed { value, input });
        }
        let (time, offset, input) = if !matches!(input[0], b' ' | b'T' | b't')
        {
            (None, None, input)
        } else {
            let input = &input[1..];
            // If there's a separator, then we must parse a time and we are
            // *allowed* to parse an offset. But without a separator, we don't
            // support offsets. Just annotations (which are parsed below).
            let Parsed { value: time, input } = self.parse_time_spec(input)?;
            let Parsed { value: offset, input } = self.parse_offset(input)?;
            (Some(time), offset, input)
        };
        let Parsed { value: annotations, input } =
            self.parse_annotations(input)?;
        let value = ParsedDateTime {
            input: escape::Bytes(mkslice(input)),
            date,
            time,
            offset,
            annotations,
        };
        Ok(Parsed { value, input })
    }

    // TemporalTimeString :::
    //   AnnotatedTime
    //   AnnotatedDateTimeTimeRequired
    //
    // AnnotatedTime :::
    //   TimeDesignator TimeSpec
    //                  DateTimeUTCOffset[opt]
    //                  TimeZoneAnnotation[opt]
    //                  Annotations[opt]
    //   TimeSpecWithOptionalOffsetNotAmbiguous TimeZoneAnnotation[opt]
    //                                          Annotations[opt]
    //
    // TimeSpecWithOptionalOffsetNotAmbiguous :::
    //   TimeSpec DateTimeUTCOffsetopt (but not one of ValidMonthDay or DateSpecYearMonth)
    //
    // TimeDesignator ::: one of
    //   T t
    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn parse_temporal_time<'i>(
        &self,
        mut input: &'i [u8],
    ) -> Result<Parsed<'i, ParsedTime<'i>>, Error> {
        let mkslice = parse::slicer(input);

        if input.starts_with(b"T") || input.starts_with(b"t") {
            input = &input[1..];
            let Parsed { value: time, input } = self.parse_time_spec(input)?;
            let Parsed { value: offset, input } = self.parse_offset(input)?;
            if offset.map_or(false, |o| o.is_zulu()) {
                return Err(err!(
                    "cannot parse civil time from string with a Zulu \
                     offset, parse as a `Timestamp` and convert to a civil \
                     time instead",
                ));
            }
            let Parsed { input, .. } = self.parse_annotations(input)?;
            return Ok(Parsed { value: time, input });
        }
        // We now look for a full datetime and extract the time from that.
        // We do this before looking for a non-T time-only component because
        // otherwise things like `2024-06-01T01:02:03` end up having `2024-06`
        // parsed as a `HHMM-OFFSET` time, and then result in an "ambiguous"
        // error.
        //
        // This is largely a result of us trying to parse a time off of the
        // beginning of the input without assuming that the time must consume
        // the entire input.
        if let Ok(parsed) = self.parse_temporal_datetime(input) {
            let Parsed { value: dt, input } = parsed;
            if dt.offset.map_or(false, |o| o.is_zulu()) {
                return Err(err!(
                    "cannot parse plain time from full datetime string with a \
                     Zulu offset, parse as a `Timestamp` and convert to a \
                     plain time instead",
                ));
            }
            let Some(time) = dt.time else {
                return Err(err!(
                    "successfully parsed date from {parsed:?}, but \
                     no time component was found",
                    parsed = dt.input,
                ));
            };
            return Ok(Parsed { value: time, input });
        }

        // At this point, we look for something that is a time that doesn't
        // start with a `T`. We need to check that it isn't ambiguous with a
        // possible date.
        let Parsed { value: time, input } = self.parse_time_spec(input)?;
        let Parsed { value: offset, input } = self.parse_offset(input)?;
        if offset.map_or(false, |o| o.is_zulu()) {
            return Err(err!(
                "cannot parse plain time from string with a Zulu \
                 offset, parse as a `Timestamp` and convert to a plain \
                 time instead",
            ));
        }
        // The possible ambiguities occur with the time AND the
        // optional offset, so try to parse what we have so far as
        // either a "month-day" or a "year-month." If either succeeds,
        // then the time is ambiguous and we can report an error.
        //
        // ... but this can only happen when the time was parsed in
        // "basic" mode. i.e., without the `:` separators.
        if !time.extended {
            let possibly_ambiguous = mkslice(input);
            if self.parse_month_day(possibly_ambiguous).is_ok() {
                return Err(err!(
                    "parsed time from {parsed:?} is ambiguous \
                             with a month-day date",
                    parsed = escape::Bytes(possibly_ambiguous),
                ));
            }
            if self.parse_year_month(possibly_ambiguous).is_ok() {
                return Err(err!(
                    "parsed time from {parsed:?} is ambiguous \
                             with a year-month date",
                    parsed = escape::Bytes(possibly_ambiguous),
                ));
            }
        }
        // OK... carry on.
        let Parsed { input, .. } = self.parse_annotations(input)?;
        Ok(Parsed { value: time, input })
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn parse_time_zone<'i>(
        &self,
        mut input: &'i [u8],
    ) -> Result<Parsed<'i, ParsedTimeZone<'i>>, Error> {
        let Some(first) = input.first().copied() else {
            return Err(err!("an empty string is not a valid time zone"));
        };
        let original = escape::Bytes(input);
        if matches!(first, b'+' | b'-') {
            static P: offset::Parser = offset::Parser::new()
                .zulu(false)
                .subminute(true)
                .subsecond(false);
            let Parsed { value: offset, input } = P.parse(input)?;
            let kind = ParsedTimeZoneKind::Offset(offset);
            let value = ParsedTimeZone { input: original, kind };
            return Ok(Parsed { value, input });
        }

        // Creates a "named" parsed time zone, generally meant to
        // be an IANA time zone identifier. We do this in a couple
        // different cases below, hence the helper function.
        let mknamed = |consumed, remaining| {
            let Ok(tzid) = core::str::from_utf8(consumed) else {
                return Err(err!(
                    "found plausible IANA time zone identifier \
                     {input:?}, but it is not valid UTF-8",
                    input = escape::Bytes(consumed),
                ));
            };
            let kind = ParsedTimeZoneKind::Named(tzid);
            let value = ParsedTimeZone { input: original, kind };
            Ok(Parsed { value, input: remaining })
        };
        // This part get tricky. The common case is absolutely an IANA time
        // zone identifier. So we try to parse something that looks like an IANA
        // tz id.
        //
        // In theory, IANA tz ids can never be valid POSIX TZ strings, since
        // POSIX TZ strings minimally require an offset in them (e.g., `EST5`)
        // and IANA tz ids aren't supposed to contain numbers. But there are
        // some legacy IANA tz ids (`EST5EDT`) that do contain numbers.
        //
        // However, the legacy IANA tz ids, like `EST5EDT`, are pretty much
        // nonsense as POSIX TZ strings since there is no DST transition rule.
        // So in cases of nonsense tz ids, we assume they are IANA tz ids.
        let mkconsumed = parse::slicer(input);
        let mut saw_number = false;
        loop {
            let Some(byte) = input.first().copied() else { break };
            if byte.is_ascii_whitespace() {
                break;
            }
            saw_number = saw_number || byte.is_ascii_digit();
            input = &input[1..];
        }
        let consumed = mkconsumed(input);
        if !saw_number {
            return mknamed(consumed, input);
        }
        #[cfg(not(feature = "alloc"))]
        {
            Err(err!(
                "cannot parsed time zones other than fixed offsets \
                 without the `alloc` crate feature enabled",
            ))
        }
        #[cfg(feature = "alloc")]
        {
            use crate::tz::posix::PosixTimeZone;

            match PosixTimeZone::parse_prefix(consumed) {
                Ok((posix_tz, input)) => {
                    let kind = ParsedTimeZoneKind::Posix(posix_tz);
                    let value = ParsedTimeZone { input: original, kind };
                    Ok(Parsed { value, input })
                }
                // We get here for invalid POSIX tz strings, or even if
                // they are technically valid according to POSIX but not
                // "reasonable", i.e., `EST5EDT`. Which in that case would
                // end up doing an IANA tz lookup. (And it might hit because
                // `EST5EDT` is a legacy IANA tz id. Lol.)
                Err(_) => mknamed(consumed, input),
            }
        }
    }

    // Date :::
    //   DateYear - DateMonth - DateDay
    //   DateYear DateMonth DateDay
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_date_spec<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, ParsedDate<'i>>, Error> {
        let mkslice = parse::slicer(input);
        let original = escape::Bytes(input);

        // Parse year component.
        let Parsed { value: year, input } =
            self.parse_year(input).with_context(|| {
                err!("failed to parse year in date {original:?}")
            })?;
        let extended = input.starts_with(b"-");

        // Parse optional separator.
        let Parsed { input, .. } = self
            .parse_date_separator(input, extended)
            .context("failed to parse separator after year")?;

        // Parse month component.
        let Parsed { value: month, input } =
            self.parse_month(input).with_context(|| {
                err!("failed to parse month in date {original:?}")
            })?;

        // Parse optional separator.
        let Parsed { input, .. } = self
            .parse_date_separator(input, extended)
            .context("failed to parse separator after month")?;

        // Parse day component.
        let Parsed { value: day, input } =
            self.parse_day(input).with_context(|| {
                err!("failed to parse day in date {original:?}")
            })?;

        let date = Date::new_ranged(year, month, day).with_context(|| {
            err!("date parsed from {original:?} is not valid")
        })?;
        let value = ParsedDate { input: escape::Bytes(mkslice(input)), date };
        Ok(Parsed { value, input })
    }

    // TimeSpec :::
    //   TimeHour
    //   TimeHour : TimeMinute
    //   TimeHour TimeMinute
    //   TimeHour : TimeMinute : TimeSecond TimeFraction[opt]
    //   TimeHour TimeMinute TimeSecond TimeFraction[opt]
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_time_spec<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, ParsedTime<'i>>, Error> {
        let mkslice = parse::slicer(input);
        let original = escape::Bytes(input);

        // Parse hour component.
        let Parsed { value: hour, input } =
            self.parse_hour(input).with_context(|| {
                err!("failed to parse hour in time {original:?}")
            })?;
        let extended = input.starts_with(b":");

        // Parse optional minute component.
        let Parsed { value: has_minute, input } =
            self.parse_time_separator(input, extended);
        if !has_minute {
            let time = Time::new_ranged(
                hour,
                t::Minute::N::<0>(),
                t::Second::N::<0>(),
                t::SubsecNanosecond::N::<0>(),
            );
            let value = ParsedTime {
                input: escape::Bytes(mkslice(input)),
                time,
                extended,
            };
            return Ok(Parsed { value, input });
        }
        let Parsed { value: minute, input } =
            self.parse_minute(input).with_context(|| {
                err!("failed to parse minute in time {original:?}")
            })?;

        // Parse optional second component.
        let Parsed { value: has_second, input } =
            self.parse_time_separator(input, extended);
        if !has_second {
            let time = Time::new_ranged(
                hour,
                minute,
                t::Second::N::<0>(),
                t::SubsecNanosecond::N::<0>(),
            );
            let value = ParsedTime {
                input: escape::Bytes(mkslice(input)),
                time,
                extended,
            };
            return Ok(Parsed { value, input });
        }
        let Parsed { value: second, input } =
            self.parse_second(input).with_context(|| {
                err!("failed to parse second in time {original:?}")
            })?;

        // Parse an optional fractional component.
        let Parsed { value: nanosecond, input } =
            parse_temporal_fraction(input).with_context(|| {
                err!(
                    "failed to parse fractional nanoseconds \
                     in time {original:?}",
                )
            })?;

        let time = Time::new_ranged(
            hour,
            minute,
            second,
            // OK because `parse_temporal_fraction` guarantees
            // `0..=999_999_999`.
            nanosecond
                .map(|n| t::SubsecNanosecond::new(n).unwrap())
                .unwrap_or(t::SubsecNanosecond::N::<0>()),
        );
        let value = ParsedTime {
            input: escape::Bytes(mkslice(input)),
            time,
            extended,
        };
        Ok(Parsed { value, input })
    }

    // ValidMonthDay :::
    //   DateMonth -[opt] 0 NonZeroDigit
    //   DateMonth -[opt] 1 DecimalDigit
    //   DateMonth -[opt] 2 DecimalDigit
    //   DateMonth -[opt] 30 but not one of 0230 or 02-30
    //   DateMonthWithThirtyOneDays -opt 31
    //
    // DateMonthWithThirtyOneDays ::: one of
    //   01 03 05 07 08 10 12
    //
    // NOTE: Jiff doesn't have a "month-day" type, but we still have a parsing
    // function for it so that we can detect ambiguous time strings.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_month_day<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, ()>, Error> {
        let original = escape::Bytes(input);

        // Parse month component.
        let Parsed { value: month, mut input } =
            self.parse_month(input).with_context(|| {
                err!("failed to parse month in month-day {original:?}")
            })?;

        // Skip over optional separator.
        if input.starts_with(b"-") {
            input = &input[1..];
        }

        // Parse day component.
        let Parsed { value: day, input } =
            self.parse_day(input).with_context(|| {
                err!("failed to parse day in month-day {original:?}")
            })?;

        // Check that the month-day is valid. Since Temporal's month-day
        // permits 02-29, we use a leap year. The error message here is
        // probably confusing, but these errors should never be exposed to the
        // user.
        let year = t::Year::N::<2024>();
        let _ = Date::new_ranged(year, month, day).with_context(|| {
            err!("month-day parsed from {original:?} is not valid")
        })?;

        // We have a valid year-month. But we don't return it because we just
        // need to check validity.
        Ok(Parsed { value: (), input })
    }

    // DateSpecYearMonth :::
    //   DateYear -[opt] DateMonth
    //
    // NOTE: Jiff doesn't have a "year-month" type, but we still have a parsing
    // function for it so that we can detect ambiguous time strings.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_year_month<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, ()>, Error> {
        let original = escape::Bytes(input);

        // Parse year component.
        let Parsed { value: year, mut input } =
            self.parse_year(input).with_context(|| {
                err!("failed to parse year in date {original:?}")
            })?;

        // Skip over optional separator.
        if input.starts_with(b"-") {
            input = &input[1..];
        }

        // Parse month component.
        let Parsed { value: month, input } =
            self.parse_month(input).with_context(|| {
                err!("failed to parse month in month-day {original:?}")
            })?;

        // Check that the year-month is valid. We just use a day of 1, since
        // every month in every year must have a day 1.
        let day = t::Day::N::<1>();
        let _ = Date::new_ranged(year, month, day).with_context(|| {
            err!("year-month parsed from {original:?} is not valid")
        })?;

        // We have a valid year-month. But we don't return it because we just
        // need to check validity.
        Ok(Parsed { value: (), input })
    }

    // DateYear :::
    //   DecimalDigit DecimalDigit DecimalDigit DecimalDigit
    //   TemporalSign DecimalDigit DecimalDigit DecimalDigit DecimalDigit DecimalDigit DecimalDigit
    //
    // NOTE: I don't really like the fact that in order to write a negative
    // year, you need to use the six digit variant. Like, why not allow
    // `-0001`? I'm not sure why, so for Chesterton's fence reasons, I'm
    // sticking with the Temporal spec. But I may loosen this in the future. We
    // should be careful not to introduce any possible ambiguities, though, I
    // don't think there are any?
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_year<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, t::Year>, Error> {
        // TODO: We could probably decrease the codegen for this function,
        // or at least make it tighter, by putting the code for signed years
        // behind an unlineable function.

        let Parsed { value: sign, input } = self.parse_year_sign(input);
        if let Some(sign) = sign {
            let (year, input) = parse::split(input, 6).ok_or_else(|| {
                err!(
                    "expected six digit year (because of a leading sign), \
                     but found end of input",
                )
            })?;
            let year = parse::i64(year).with_context(|| {
                err!(
                    "failed to parse {year:?} as year (a six digit integer)",
                    year = escape::Bytes(year),
                )
            })?;
            let year =
                t::Year::try_new("year", year).context("year is not valid")?;
            if year == C(0) && sign.is_negative() {
                return Err(err!(
                    "year zero must be written without a sign or a \
                     positive sign, but not a negative sign",
                ));
            }
            Ok(Parsed { value: year * sign.as_ranged_integer(), input })
        } else {
            let (year, input) = parse::split(input, 4).ok_or_else(|| {
                err!(
                    "expected four digit year (or leading sign for \
                     six digit year), but found end of input",
                )
            })?;
            let year = parse::i64(year).with_context(|| {
                err!(
                    "failed to parse {year:?} as year (a four digit integer)",
                    year = escape::Bytes(year),
                )
            })?;
            let year =
                t::Year::try_new("year", year).context("year is not valid")?;
            Ok(Parsed { value: year, input })
        }
    }

    // DateMonth :::
    //   0 NonZeroDigit
    //   10
    //   11
    //   12
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_month<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, t::Month>, Error> {
        let (month, input) = parse::split(input, 2).ok_or_else(|| {
            err!("expected two digit month, but found end of input")
        })?;
        let month = parse::i64(month).with_context(|| {
            err!(
                "failed to parse {month:?} as month (a two digit integer)",
                month = escape::Bytes(month),
            )
        })?;
        let month =
            t::Month::try_new("month", month).context("month is not valid")?;
        Ok(Parsed { value: month, input })
    }

    // DateDay :::
    //   0 NonZeroDigit
    //   1 DecimalDigit
    //   2 DecimalDigit
    //   30
    //   31
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_day<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, t::Day>, Error> {
        let (day, input) = parse::split(input, 2).ok_or_else(|| {
            err!("expected two digit day, but found end of input")
        })?;
        let day = parse::i64(day).with_context(|| {
            err!(
                "failed to parse {day:?} as day (a two digit integer)",
                day = escape::Bytes(day),
            )
        })?;
        let day = t::Day::try_new("day", day).context("day is not valid")?;
        Ok(Parsed { value: day, input })
    }

    // TimeHour :::
    //   Hour
    //
    // Hour :::
    //   0 DecimalDigit
    //   1 DecimalDigit
    //   20
    //   21
    //   22
    //   23
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_hour<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, t::Hour>, Error> {
        let (hour, input) = parse::split(input, 2).ok_or_else(|| {
            err!("expected two digit hour, but found end of input")
        })?;
        let hour = parse::i64(hour).with_context(|| {
            err!(
                "failed to parse {hour:?} as hour (a two digit integer)",
                hour = escape::Bytes(hour),
            )
        })?;
        let hour =
            t::Hour::try_new("hour", hour).context("hour is not valid")?;
        Ok(Parsed { value: hour, input })
    }

    // TimeMinute :::
    //   MinuteSecond
    //
    // MinuteSecond :::
    //   0 DecimalDigit
    //   1 DecimalDigit
    //   2 DecimalDigit
    //   3 DecimalDigit
    //   4 DecimalDigit
    //   5 DecimalDigit
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_minute<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, t::Minute>, Error> {
        let (minute, input) = parse::split(input, 2).ok_or_else(|| {
            err!("expected two digit minute, but found end of input")
        })?;
        let minute = parse::i64(minute).with_context(|| {
            err!(
                "failed to parse {minute:?} as minute (a two digit integer)",
                minute = escape::Bytes(minute),
            )
        })?;
        let minute = t::Minute::try_new("minute", minute)
            .context("minute is not valid")?;
        Ok(Parsed { value: minute, input })
    }

    // TimeSecond :::
    //   MinuteSecond
    //   60
    //
    // MinuteSecond :::
    //   0 DecimalDigit
    //   1 DecimalDigit
    //   2 DecimalDigit
    //   3 DecimalDigit
    //   4 DecimalDigit
    //   5 DecimalDigit
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_second<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, t::Second>, Error> {
        let (second, input) = parse::split(input, 2).ok_or_else(|| {
            err!("expected two digit second, but found end of input",)
        })?;
        let mut second = parse::i64(second).with_context(|| {
            err!(
                "failed to parse {second:?} as second (a two digit integer)",
                second = escape::Bytes(second),
            )
        })?;
        // NOTE: I believe Temporal allows one to make this configurable. That
        // is, to reject it. But for now, we just always clamp a leap second.
        if second == 60 {
            second = 59;
        }
        let second = t::Second::try_new("second", second)
            .context("second is not valid")?;
        Ok(Parsed { value: second, input })
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_offset<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, Option<ParsedOffset>>, Error> {
        const P: offset::Parser =
            offset::Parser::new().zulu(true).subminute(true);
        P.parse_optional(input)
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_annotations<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, ParsedAnnotations<'i>>, Error> {
        const P: rfc9557::Parser = rfc9557::Parser::new();
        if input.is_empty() || input[0] != b'[' {
            let value = ParsedAnnotations::none();
            return Ok(Parsed { input, value });
        }
        P.parse(input)
    }

    /// Parses the separator that is expected to appear between
    /// date components.
    ///
    /// When in extended mode, a `-` is expected. When not in extended mode,
    /// no input is consumed and this routine never fails.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_date_separator<'i>(
        &self,
        mut input: &'i [u8],
        extended: bool,
    ) -> Result<Parsed<'i, ()>, Error> {
        if !extended {
            // If we see a '-' when not in extended mode, then we can report
            // a better error message than, e.g., "-3 isn't a valid day."
            if input.starts_with(b"-") {
                return Err(err!(
                    "expected no separator after month since none was \
                     found after the year, but found a '-' separator",
                ));
            }
            return Ok(Parsed { value: (), input });
        }
        if input.is_empty() {
            return Err(err!(
                "expected '-' separator, but found end of input"
            ));
        }
        if input[0] != b'-' {
            return Err(err!(
                "expected '-' separator, but found {found:?} instead",
                found = escape::Byte(input[0]),
            ));
        }
        input = &input[1..];
        Ok(Parsed { value: (), input })
    }

    /// Parses the separator that is expected to appear between time
    /// components. When `true` is returned, we expect to parse the next
    /// component. When `false` is returned, then no separator was found and
    /// there is no expectation of finding another component.
    ///
    /// When in extended mode, true is returned if and only if a separator is
    /// found.
    ///
    /// When in basic mode (not extended), then a subsequent component is only
    /// expected when `input` begins with two ASCII digits.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_time_separator<'i>(
        &self,
        mut input: &'i [u8],
        extended: bool,
    ) -> Parsed<'i, bool> {
        if !extended {
            let expected =
                input.len() >= 2 && input[..2].iter().all(u8::is_ascii_digit);
            return Parsed { value: expected, input };
        }
        let is_separator = input.get(0).map_or(false, |&b| b == b':');
        if is_separator {
            input = &input[1..];
        }
        Parsed { value: is_separator, input }
    }

    // TemporalSign :::
    //   ASCIISign
    //   <MINUS>
    //
    // ASCIISign ::: one of
    //   + -
    //
    // NOTE: We specifically only support ASCII signs. I think Temporal needs
    // to support `<MINUS>` because of other things in ECMA script that
    // require it?[1]
    //
    // [1]: https://github.com/tc39/proposal-temporal/issues/2843
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_year_sign<'i>(
        &self,
        mut input: &'i [u8],
    ) -> Parsed<'i, Option<Sign>> {
        let Some(sign) = input.get(0).copied() else {
            return Parsed { value: None, input };
        };
        let sign = if sign == b'+' {
            Sign::Positive
        } else if sign == b'-' {
            Sign::Negative
        } else {
            return Parsed { value: None, input };
        };
        input = &input[1..];
        Parsed { value: Some(sign), input }
    }
}

/// A parser for Temporal spans.
///
/// Note that in Temporal, a "span" is called a "duration."
#[derive(Debug)]
pub(super) struct SpanParser {
    /// There are currently no configuration options for this parser.
    _priv: (),
}

impl SpanParser {
    /// Create a new Temporal span parser with the default configuration.
    pub(super) const fn new() -> SpanParser {
        SpanParser { _priv: () }
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn parse_span<I: AsRef<[u8]>>(
        &self,
        input: I,
    ) -> Result<Span, Error> {
        #[inline(never)]
        fn imp(p: &SpanParser, input: &[u8]) -> Result<Span, Error> {
            let mut builder = DurationUnits::default();
            let parsed = p.parse_calendar_and_time(input, &mut builder)?;
            let parsed = parsed.and_then(|_| builder.to_span())?;
            parsed.into_full()
        }

        let input = input.as_ref();
        imp(self, input).with_context(|| {
            err!(
                "failed to parse {input:?} as an ISO 8601 duration string",
                input = escape::Bytes(input)
            )
        })
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn parse_signed_duration<I: AsRef<[u8]>>(
        &self,
        input: I,
    ) -> Result<SignedDuration, Error> {
        #[inline(never)]
        fn imp(p: &SpanParser, input: &[u8]) -> Result<SignedDuration, Error> {
            let mut builder = DurationUnits::default();
            let parsed = p.parse_time_only(input, &mut builder)?;
            let parsed = parsed.and_then(|_| builder.to_signed_duration())?;
            parsed.into_full()
        }

        let input = input.as_ref();
        imp(self, input).with_context(|| {
            err!(
                "failed to parse {input:?} as an ISO 8601 duration string",
                input = escape::Bytes(input)
            )
        })
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(super) fn parse_unsigned_duration<I: AsRef<[u8]>>(
        &self,
        input: I,
    ) -> Result<core::time::Duration, Error> {
        #[inline(never)]
        fn imp(
            p: &SpanParser,
            input: &[u8],
        ) -> Result<core::time::Duration, Error> {
            let mut builder = DurationUnits::default();
            let parsed = p.parse_time_only(input, &mut builder)?;
            let parsed =
                parsed.and_then(|_| builder.to_unsigned_duration())?;
            let d = parsed.value;
            parsed.into_full_with(format_args!("{d:?}"))
        }

        let input = input.as_ref();
        imp(self, input).with_context(|| {
            err!(
                "failed to parse {input:?} as an ISO 8601 duration string",
                input = escape::Bytes(input)
            )
        })
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_calendar_and_time<'i>(
        &self,
        input: &'i [u8],
        builder: &mut DurationUnits,
    ) -> Result<Parsed<'i, ()>, Error> {
        let original = escape::Bytes(input);
        let (sign, input) =
            if !input.first().map_or(false, |&b| matches!(b, b'+' | b'-')) {
                (Sign::Positive, input)
            } else {
                let Parsed { value: sign, input } = self.parse_sign(input);
                (sign, input)
            };
        let Parsed { input, .. } = self.parse_duration_designator(input)?;

        let Parsed { input, .. } = self.parse_date_units(input, builder)?;
        let Parsed { value: has_time, mut input } =
            self.parse_time_designator(input);
        if has_time {
            let parsed = self.parse_time_units(input, builder)?;
            input = parsed.input;

            if builder.get_min().map_or(true, |min| min > Unit::Hour) {
                return Err(err!(
                    "found a time designator (T or t) in an ISO 8601 \
                     duration string in {original:?}, but did not find \
                     any time units",
                ));
            }
        }
        builder.set_sign(sign);
        Ok(Parsed { value: (), input })
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_time_only<'i>(
        &self,
        input: &'i [u8],
        builder: &mut DurationUnits,
    ) -> Result<Parsed<'i, ()>, Error> {
        let original = escape::Bytes(input);
        let (sign, input) =
            if !input.first().map_or(false, |&b| matches!(b, b'+' | b'-')) {
                (Sign::Positive, input)
            } else {
                let Parsed { value: sign, input } = self.parse_sign(input);
                (sign, input)
            };
        let Parsed { input, .. } = self.parse_duration_designator(input)?;

        let Parsed { value: has_time, input } =
            self.parse_time_designator(input);
        if !has_time {
            return Err(err!(
                "parsing ISO 8601 duration into a `SignedDuration` requires \
                 that the duration contain a time component and no \
                 components of days or greater",
            ));
        }

        let Parsed { input, .. } = self.parse_time_units(input, builder)?;
        if builder.get_min().map_or(true, |min| min > Unit::Hour) {
            return Err(err!(
                "found a time designator (T or t) in an ISO 8601 \
                 duration string in {original:?}, but did not find \
                 any time units",
            ));
        }
        builder.set_sign(sign);
        Ok(Parsed { value: (), input })
    }

    /// Parses consecutive units from an ISO 8601 duration string into the
    /// `DurationUnits` given.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_date_units<'i>(
        &self,
        mut input: &'i [u8],
        builder: &mut DurationUnits,
    ) -> Result<Parsed<'i, ()>, Error> {
        loop {
            let parsed = self.parse_unit_value(input)?;
            input = parsed.input;
            let Some(value) = parsed.value else { break };

            let parsed = self.parse_unit_date_designator(input)?;
            input = parsed.input;
            let unit = parsed.value;

            builder.set_unit_value(unit, value)?;
        }
        Ok(Parsed { value: (), input })
    }

    /// Parses consecutive time units from an ISO 8601 duration string into the
    /// `DurationUnits` given.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_time_units<'i>(
        &self,
        mut input: &'i [u8],
        builder: &mut DurationUnits,
    ) -> Result<Parsed<'i, ()>, Error> {
        loop {
            let parsed = self.parse_unit_value(input)?;
            input = parsed.input;
            let Some(value) = parsed.value else { break };

            let parsed = parse_temporal_fraction(input)?;
            input = parsed.input;
            let fraction = parsed.value;

            let parsed = self.parse_unit_time_designator(input)?;
            input = parsed.input;
            let unit = parsed.value;

            builder.set_unit_value(unit, value)?;
            if let Some(fraction) = fraction {
                builder.set_fraction(fraction)?;
                // Once we see a fraction, we are done. We don't permit parsing
                // any more units. That is, a fraction can only occur on the
                // lowest unit of time.
                break;
            }
        }
        Ok(Parsed { value: (), input })
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_unit_value<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, Option<u64>>, Error> {
        let (value, input) = parse::u64_prefix(input)?;
        Ok(Parsed { value, input })
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_unit_date_designator<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, Unit>, Error> {
        if input.is_empty() {
            return Err(err!(
                "expected to find date unit designator suffix \
                 (Y, M, W or D), but found end of input",
            ));
        }
        let unit = match input[0] {
            b'Y' | b'y' => Unit::Year,
            b'M' | b'm' => Unit::Month,
            b'W' | b'w' => Unit::Week,
            b'D' | b'd' => Unit::Day,
            unknown => {
                return Err(err!(
                    "expected to find date unit designator suffix \
                     (Y, M, W or D), but found {found:?} instead",
                    found = escape::Byte(unknown),
                ));
            }
        };
        Ok(Parsed { value: unit, input: &input[1..] })
    }

    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_unit_time_designator<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, Unit>, Error> {
        if input.is_empty() {
            return Err(err!(
                "expected to find time unit designator suffix \
                 (H, M or S), but found end of input",
            ));
        }
        let unit = match input[0] {
            b'H' | b'h' => Unit::Hour,
            b'M' | b'm' => Unit::Minute,
            b'S' | b's' => Unit::Second,
            unknown => {
                return Err(err!(
                    "expected to find time unit designator suffix \
                     (H, M or S), but found {found:?} instead",
                    found = escape::Byte(unknown),
                ));
            }
        };
        Ok(Parsed { value: unit, input: &input[1..] })
    }

    // DurationDesignator ::: one of
    //   P p
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_duration_designator<'i>(
        &self,
        input: &'i [u8],
    ) -> Result<Parsed<'i, ()>, Error> {
        if input.is_empty() {
            return Err(err!(
                "expected to find duration beginning with 'P' or 'p', \
                 but found end of input",
            ));
        }
        if !matches!(input[0], b'P' | b'p') {
            return Err(err!(
                "expected 'P' or 'p' prefix to begin duration, \
                 but found {found:?} instead",
                found = escape::Byte(input[0]),
            ));
        }
        Ok(Parsed { value: (), input: &input[1..] })
    }

    // TimeDesignator ::: one of
    //   T t
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn parse_time_designator<'i>(&self, input: &'i [u8]) -> Parsed<'i, bool> {
        if input.is_empty() || !matches!(input[0], b'T' | b't') {
            return Parsed { value: false, input };
        }
        Parsed { value: true, input: &input[1..] }
    }

    // TemporalSign :::
    //   ASCIISign
    //   <MINUS>
    //
    // NOTE: Like with other things with signs, we don't support the Unicode
    // <MINUS> sign. Just ASCII.
    #[cold]
    #[inline(never)]
    fn parse_sign<'i>(&self, input: &'i [u8]) -> Parsed<'i, Sign> {
        let Some(sign) = input.get(0).copied() else {
            return Parsed { value: Sign::Positive, input };
        };
        let sign = if sign == b'+' {
            Sign::Positive
        } else if sign == b'-' {
            Sign::Negative
        } else {
            return Parsed { value: Sign::Positive, input };
        };
        Parsed { value: sign, input: &input[1..] }
    }
}

#[cfg(feature = "alloc")]
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ok_signed_duration() {
        let p = |input: &[u8]| {
            SpanParser::new().parse_signed_duration(input).unwrap()
        };

        insta::assert_debug_snapshot!(p(b"PT0s"), @"0s");
        insta::assert_debug_snapshot!(p(b"PT0.000000001s"), @"1ns");
        insta::assert_debug_snapshot!(p(b"PT1s"), @"1s");
        insta::assert_debug_snapshot!(p(b"PT59s"), @"59s");
        insta::assert_debug_snapshot!(p(b"PT60s"), @"60s");
        insta::assert_debug_snapshot!(p(b"PT1m"), @"60s");
        insta::assert_debug_snapshot!(p(b"PT1m0.000000001s"), @"60s 1ns");
        insta::assert_debug_snapshot!(p(b"PT1.25m"), @"75s");
        insta::assert_debug_snapshot!(p(b"PT1h"), @"3600s");
        insta::assert_debug_snapshot!(p(b"PT1h0.000000001s"), @"3600s 1ns");
        insta::assert_debug_snapshot!(p(b"PT1.25h"), @"4500s");

        insta::assert_debug_snapshot!(p(b"-PT2562047788015215h30m8.999999999s"), @"-9223372036854775808s 999999999ns");
        insta::assert_debug_snapshot!(p(b"PT2562047788015215h30m7.999999999s"), @"9223372036854775807s 999999999ns");

        insta::assert_debug_snapshot!(p(b"PT9223372036854775807S"), @"9223372036854775807s");
        insta::assert_debug_snapshot!(p(b"-PT9223372036854775808S"), @"-9223372036854775808s");
    }

    #[test]
    fn err_signed_duration() {
        let p = |input: &[u8]| {
            SpanParser::new().parse_signed_duration(input).unwrap_err()
        };

        insta::assert_snapshot!(
            p(b"P0d"),
            @r#"failed to parse "P0d" as an ISO 8601 duration string: parsing ISO 8601 duration into a `SignedDuration` requires that the duration contain a time component and no components of days or greater"#,
        );
        insta::assert_snapshot!(
            p(b"PT0d"),
            @r#"failed to parse "PT0d" as an ISO 8601 duration string: expected to find time unit designator suffix (H, M or S), but found "d" instead"#,
        );
        insta::assert_snapshot!(
            p(b"P0dT1s"),
            @r#"failed to parse "P0dT1s" as an ISO 8601 duration string: parsing ISO 8601 duration into a `SignedDuration` requires that the duration contain a time component and no components of days or greater"#,
        );

        insta::assert_snapshot!(
            p(b""),
            @r#"failed to parse "" as an ISO 8601 duration string: expected to find duration beginning with 'P' or 'p', but found end of input"#,
        );
        insta::assert_snapshot!(
            p(b"P"),
            @r#"failed to parse "P" as an ISO 8601 duration string: parsing ISO 8601 duration into a `SignedDuration` requires that the duration contain a time component and no components of days or greater"#,
        );
        insta::assert_snapshot!(
            p(b"PT"),
            @r#"failed to parse "PT" as an ISO 8601 duration string: found a time designator (T or t) in an ISO 8601 duration string in "PT", but did not find any time units"#,
        );
        insta::assert_snapshot!(
            p(b"PTs"),
            @r#"failed to parse "PTs" as an ISO 8601 duration string: found a time designator (T or t) in an ISO 8601 duration string in "PTs", but did not find any time units"#,
        );

        insta::assert_snapshot!(
            p(b"PT1s1m"),
            @r#"failed to parse "PT1s1m" as an ISO 8601 duration string: found value 1 with unit minute after unit second, but units must be written from largest to smallest (and they can't be repeated)"#,
        );
        insta::assert_snapshot!(
            p(b"PT1s1h"),
            @r#"failed to parse "PT1s1h" as an ISO 8601 duration string: found value 1 with unit hour after unit second, but units must be written from largest to smallest (and they can't be repeated)"#,
        );
        insta::assert_snapshot!(
            p(b"PT1m1h"),
            @r#"failed to parse "PT1m1h" as an ISO 8601 duration string: found value 1 with unit hour after unit minute, but units must be written from largest to smallest (and they can't be repeated)"#,
        );

        insta::assert_snapshot!(
            p(b"-PT9223372036854775809s"),
            @r#"failed to parse "-PT9223372036854775809s" as an ISO 8601 duration string: `-9223372036854775809` seconds is too big (or small) to fit into a signed 64-bit integer"#,
        );
        insta::assert_snapshot!(
            p(b"PT9223372036854775808s"),
            @r#"failed to parse "PT9223372036854775808s" as an ISO 8601 duration string: `9223372036854775808` seconds is too big (or small) to fit into a signed 64-bit integer"#,
        );

        insta::assert_snapshot!(
            p(b"PT1m9223372036854775807s"),
            @r#"failed to parse "PT1m9223372036854775807s" as an ISO 8601 duration string: accumulated `SignedDuration` of `1m` overflowed when adding 9223372036854775807 of unit second"#,
        );
        insta::assert_snapshot!(
            p(b"PT2562047788015215.6h"),
            @r#"failed to parse "PT2562047788015215.6h" as an ISO 8601 duration string: accumulated `SignedDuration` of `2562047788015215h` overflowed when adding 0.600000000 of unit hour"#,
        );
    }

    #[test]
    fn ok_unsigned_duration() {
        let p = |input: &[u8]| {
            SpanParser::new().parse_unsigned_duration(input).unwrap()
        };

        insta::assert_debug_snapshot!(p(b"PT0s"), @"0ns");
        insta::assert_debug_snapshot!(p(b"PT0.000000001s"), @"1ns");
        insta::assert_debug_snapshot!(p(b"PT1s"), @"1s");
        insta::assert_debug_snapshot!(p(b"+PT1s"), @"1s");
        insta::assert_debug_snapshot!(p(b"PT59s"), @"59s");
        insta::assert_debug_snapshot!(p(b"PT60s"), @"60s");
        insta::assert_debug_snapshot!(p(b"PT1m"), @"60s");
        insta::assert_debug_snapshot!(p(b"PT1m0.000000001s"), @"60.000000001s");
        insta::assert_debug_snapshot!(p(b"PT1.25m"), @"75s");
        insta::assert_debug_snapshot!(p(b"PT1h"), @"3600s");
        insta::assert_debug_snapshot!(p(b"PT1h0.000000001s"), @"3600.000000001s");
        insta::assert_debug_snapshot!(p(b"PT1.25h"), @"4500s");

        insta::assert_debug_snapshot!(p(b"PT2562047788015215h30m7.999999999s"), @"9223372036854775807.999999999s");
        insta::assert_debug_snapshot!(p(b"PT5124095576030431H15.999999999S"), @"18446744073709551615.999999999s");

        insta::assert_debug_snapshot!(p(b"PT9223372036854775807S"), @"9223372036854775807s");
        insta::assert_debug_snapshot!(p(b"PT9223372036854775808S"), @"9223372036854775808s");
        insta::assert_debug_snapshot!(p(b"PT18446744073709551615S"), @"18446744073709551615s");
        insta::assert_debug_snapshot!(p(b"PT1M18446744073709551555S"), @"18446744073709551615s");
    }

    #[test]
    fn err_unsigned_duration() {
        #[track_caller]
        fn p(input: &[u8]) -> crate::Error {
            SpanParser::new().parse_unsigned_duration(input).unwrap_err()
        }

        insta::assert_snapshot!(
            p(b"-PT1S"),
            @r#"failed to parse "-PT1S" as an ISO 8601 duration string: cannot parse negative duration into unsigned `std::time::Duration`"#,
        );
        insta::assert_snapshot!(
            p(b"-PT0S"),
            @r#"failed to parse "-PT0S" as an ISO 8601 duration string: cannot parse negative duration into unsigned `std::time::Duration`"#,
        );

        insta::assert_snapshot!(
            p(b"P0d"),
            @r#"failed to parse "P0d" as an ISO 8601 duration string: parsing ISO 8601 duration into a `SignedDuration` requires that the duration contain a time component and no components of days or greater"#,
        );
        insta::assert_snapshot!(
            p(b"PT0d"),
            @r#"failed to parse "PT0d" as an ISO 8601 duration string: expected to find time unit designator suffix (H, M or S), but found "d" instead"#,
        );
        insta::assert_snapshot!(
            p(b"P0dT1s"),
            @r#"failed to parse "P0dT1s" as an ISO 8601 duration string: parsing ISO 8601 duration into a `SignedDuration` requires that the duration contain a time component and no components of days or greater"#,
        );

        insta::assert_snapshot!(
            p(b""),
            @r#"failed to parse "" as an ISO 8601 duration string: expected to find duration beginning with 'P' or 'p', but found end of input"#,
        );
        insta::assert_snapshot!(
            p(b"P"),
            @r#"failed to parse "P" as an ISO 8601 duration string: parsing ISO 8601 duration into a `SignedDuration` requires that the duration contain a time component and no components of days or greater"#,
        );
        insta::assert_snapshot!(
            p(b"PT"),
            @r#"failed to parse "PT" as an ISO 8601 duration string: found a time designator (T or t) in an ISO 8601 duration string in "PT", but did not find any time units"#,
        );
        insta::assert_snapshot!(
            p(b"PTs"),
            @r#"failed to parse "PTs" as an ISO 8601 duration string: found a time designator (T or t) in an ISO 8601 duration string in "PTs", but did not find any time units"#,
        );

        insta::assert_snapshot!(
            p(b"PT1s1m"),
            @r#"failed to parse "PT1s1m" as an ISO 8601 duration string: found value 1 with unit minute after unit second, but units must be written from largest to smallest (and they can't be repeated)"#,
        );
        insta::assert_snapshot!(
            p(b"PT1s1h"),
            @r#"failed to parse "PT1s1h" as an ISO 8601 duration string: found value 1 with unit hour after unit second, but units must be written from largest to smallest (and they can't be repeated)"#,
        );
        insta::assert_snapshot!(
            p(b"PT1m1h"),
            @r#"failed to parse "PT1m1h" as an ISO 8601 duration string: found value 1 with unit hour after unit minute, but units must be written from largest to smallest (and they can't be repeated)"#,
        );

        insta::assert_snapshot!(
            p(b"-PT9223372036854775809S"),
            @r#"failed to parse "-PT9223372036854775809S" as an ISO 8601 duration string: cannot parse negative duration into unsigned `std::time::Duration`"#,
        );
        insta::assert_snapshot!(
            p(b"PT18446744073709551616S"),
            @r#"failed to parse "PT18446744073709551616S" as an ISO 8601 duration string: number `18446744073709551616` too big to parse into 64-bit integer"#,
        );

        insta::assert_snapshot!(
            p(b"PT5124095576030431H16.999999999S"),
            @r#"failed to parse "PT5124095576030431H16.999999999S" as an ISO 8601 duration string: accumulated `std::time::Duration` of `18446744073709551600s` overflowed when adding 16 of unit second"#,
        );
        insta::assert_snapshot!(
            p(b"PT1M18446744073709551556S"),
            @r#"failed to parse "PT1M18446744073709551556S" as an ISO 8601 duration string: accumulated `std::time::Duration` of `60s` overflowed when adding 18446744073709551556 of unit second"#,
        );
        insta::assert_snapshot!(
            p(b"PT5124095576030431.5H"),
            @r#"failed to parse "PT5124095576030431.5H" as an ISO 8601 duration string: accumulated `std::time::Duration` of `18446744073709551600s` overflowed when adding 0.500000000 of unit hour"#,
        );
    }

    #[test]
    fn ok_temporal_duration_basic() {
        let p = |input: &[u8]| SpanParser::new().parse_span(input).unwrap();

        insta::assert_debug_snapshot!(p(b"P5d"), @"5d");
        insta::assert_debug_snapshot!(p(b"-P5d"), @"5d ago");
        insta::assert_debug_snapshot!(p(b"+P5d"), @"5d");
        insta::assert_debug_snapshot!(p(b"P5DT1s"), @"5d 1s");
        insta::assert_debug_snapshot!(p(b"PT1S"), @"1s");
        insta::assert_debug_snapshot!(p(b"PT0S"), @"0s");
        insta::assert_debug_snapshot!(p(b"P0Y"), @"0s");
        insta::assert_debug_snapshot!(p(b"P1Y1M1W1DT1H1M1S"), @"1y 1mo 1w 1d 1h 1m 1s");
        insta::assert_debug_snapshot!(p(b"P1y1m1w1dT1h1m1s"), @"1y 1mo 1w 1d 1h 1m 1s");
    }

    #[test]
    fn ok_temporal_duration_fractional() {
        let p = |input: &[u8]| SpanParser::new().parse_span(input).unwrap();

        insta::assert_debug_snapshot!(p(b"PT0.5h"), @"30m");
        insta::assert_debug_snapshot!(p(b"PT0.123456789h"), @"7m 24s 444ms 440µs 400ns");
        insta::assert_debug_snapshot!(p(b"PT1.123456789h"), @"1h 7m 24s 444ms 440µs 400ns");

        insta::assert_debug_snapshot!(p(b"PT0.5m"), @"30s");
        insta::assert_debug_snapshot!(p(b"PT0.123456789m"), @"7s 407ms 407µs 340ns");
        insta::assert_debug_snapshot!(p(b"PT1.123456789m"), @"1m 7s 407ms 407µs 340ns");

        insta::assert_debug_snapshot!(p(b"PT0.5s"), @"500ms");
        insta::assert_debug_snapshot!(p(b"PT0.123456789s"), @"123ms 456µs 789ns");
        insta::assert_debug_snapshot!(p(b"PT1.123456789s"), @"1s 123ms 456µs 789ns");

        // The tests below all have a whole second value that exceeds the
        // maximum allowed seconds in a span. But they should still parse
        // correctly by spilling over into milliseconds, microseconds and
        // nanoseconds.
        insta::assert_debug_snapshot!(p(b"PT1902545624836.854775807s"), @"631107417600s 631107417600000ms 631107417600000000µs 9223372036854775807ns");
        insta::assert_debug_snapshot!(p(b"PT175307616h10518456960m640330789636.854775807s"), @"175307616h 10518456960m 631107417600s 9223372036854ms 775µs 807ns");
        insta::assert_debug_snapshot!(p(b"-PT1902545624836.854775807s"), @"631107417600s 631107417600000ms 631107417600000000µs 9223372036854775807ns ago");
        insta::assert_debug_snapshot!(p(b"-PT175307616h10518456960m640330789636.854775807s"), @"175307616h 10518456960m 631107417600s 9223372036854ms 775µs 807ns ago");
    }

    #[test]
    fn ok_temporal_duration_unbalanced() {
        let p = |input: &[u8]| SpanParser::new().parse_span(input).unwrap();

        insta::assert_debug_snapshot!(
            p(b"PT175307616h10518456960m1774446656760s"), @"175307616h 10518456960m 631107417600s 631107417600000ms 512231821560000000µs");
        insta::assert_debug_snapshot!(
            p(b"Pt843517082H"), @"175307616h 10518456960m 631107417600s 631107417600000ms 512231824800000000µs");
        insta::assert_debug_snapshot!(
            p(b"Pt843517081H"), @"175307616h 10518456960m 631107417600s 631107417600000ms 512231821200000000µs");
    }

    #[test]
    fn ok_temporal_datetime_basic() {
        let p = |input| {
            DateTimeParser::new().parse_temporal_datetime(input).unwrap()
        };

        insta::assert_debug_snapshot!(p(b"2024-06-01"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: None,
                offset: None,
                annotations: ParsedAnnotations {
                    input: "",
                    time_zone: None,
                },
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01[America/New_York]"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01[America/New_York]",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: None,
                offset: None,
                annotations: ParsedAnnotations {
                    input: "[America/New_York]",
                    time_zone: Some(
                        Named {
                            critical: false,
                            name: "America/New_York",
                        },
                    ),
                },
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T01:02:03"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01T01:02:03",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "01:02:03",
                        time: 01:02:03,
                        extended: true,
                    },
                ),
                offset: None,
                annotations: ParsedAnnotations {
                    input: "",
                    time_zone: None,
                },
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T01:02:03-05"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01T01:02:03-05",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "01:02:03",
                        time: 01:02:03,
                        extended: true,
                    },
                ),
                offset: Some(
                    ParsedOffset {
                        kind: Numeric(
                            -05,
                        ),
                    },
                ),
                annotations: ParsedAnnotations {
                    input: "",
                    time_zone: None,
                },
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T01:02:03-05[America/New_York]"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01T01:02:03-05[America/New_York]",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "01:02:03",
                        time: 01:02:03,
                        extended: true,
                    },
                ),
                offset: Some(
                    ParsedOffset {
                        kind: Numeric(
                            -05,
                        ),
                    },
                ),
                annotations: ParsedAnnotations {
                    input: "[America/New_York]",
                    time_zone: Some(
                        Named {
                            critical: false,
                            name: "America/New_York",
                        },
                    ),
                },
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T01:02:03Z[America/New_York]"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01T01:02:03Z[America/New_York]",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "01:02:03",
                        time: 01:02:03,
                        extended: true,
                    },
                ),
                offset: Some(
                    ParsedOffset {
                        kind: Zulu,
                    },
                ),
                annotations: ParsedAnnotations {
                    input: "[America/New_York]",
                    time_zone: Some(
                        Named {
                            critical: false,
                            name: "America/New_York",
                        },
                    ),
                },
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T01:02:03-01[America/New_York]"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01T01:02:03-01[America/New_York]",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "01:02:03",
                        time: 01:02:03,
                        extended: true,
                    },
                ),
                offset: Some(
                    ParsedOffset {
                        kind: Numeric(
                            -01,
                        ),
                    },
                ),
                annotations: ParsedAnnotations {
                    input: "[America/New_York]",
                    time_zone: Some(
                        Named {
                            critical: false,
                            name: "America/New_York",
                        },
                    ),
                },
            },
            input: "",
        }
        "###);
    }

    #[test]
    fn ok_temporal_datetime_incomplete() {
        let p = |input| {
            DateTimeParser::new().parse_temporal_datetime(input).unwrap()
        };

        insta::assert_debug_snapshot!(p(b"2024-06-01T01"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01T01",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "01",
                        time: 01:00:00,
                        extended: false,
                    },
                ),
                offset: None,
                annotations: ParsedAnnotations {
                    input: "",
                    time_zone: None,
                },
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T0102"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01T0102",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "0102",
                        time: 01:02:00,
                        extended: false,
                    },
                ),
                offset: None,
                annotations: ParsedAnnotations {
                    input: "",
                    time_zone: None,
                },
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T01:02"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01T01:02",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "01:02",
                        time: 01:02:00,
                        extended: true,
                    },
                ),
                offset: None,
                annotations: ParsedAnnotations {
                    input: "",
                    time_zone: None,
                },
            },
            input: "",
        }
        "###);
    }

    #[test]
    fn ok_temporal_datetime_separator() {
        let p = |input| {
            DateTimeParser::new().parse_temporal_datetime(input).unwrap()
        };

        insta::assert_debug_snapshot!(p(b"2024-06-01t01:02:03"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01t01:02:03",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "01:02:03",
                        time: 01:02:03,
                        extended: true,
                    },
                ),
                offset: None,
                annotations: ParsedAnnotations {
                    input: "",
                    time_zone: None,
                },
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01 01:02:03"), @r###"
        Parsed {
            value: ParsedDateTime {
                input: "2024-06-01 01:02:03",
                date: ParsedDate {
                    input: "2024-06-01",
                    date: 2024-06-01,
                },
                time: Some(
                    ParsedTime {
                        input: "01:02:03",
                        time: 01:02:03,
                        extended: true,
                    },
                ),
                offset: None,
                annotations: ParsedAnnotations {
                    input: "",
                    time_zone: None,
                },
            },
            input: "",
        }
        "###);
    }

    #[test]
    fn ok_temporal_time_basic() {
        let p =
            |input| DateTimeParser::new().parse_temporal_time(input).unwrap();

        insta::assert_debug_snapshot!(p(b"01:02:03"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02:03",
                time: 01:02:03,
                extended: true,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"130113"), @r###"
        Parsed {
            value: ParsedTime {
                input: "130113",
                time: 13:01:13,
                extended: false,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"T01:02:03"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02:03",
                time: 01:02:03,
                extended: true,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"T010203"), @r###"
        Parsed {
            value: ParsedTime {
                input: "010203",
                time: 01:02:03,
                extended: false,
            },
            input: "",
        }
        "###);
    }

    #[test]
    fn ok_temporal_time_from_full_datetime() {
        let p =
            |input| DateTimeParser::new().parse_temporal_time(input).unwrap();

        insta::assert_debug_snapshot!(p(b"2024-06-01T01:02:03"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02:03",
                time: 01:02:03,
                extended: true,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T01:02:03.123"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02:03.123",
                time: 01:02:03.123,
                extended: true,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T01"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01",
                time: 01:00:00,
                extended: false,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T0102"), @r###"
        Parsed {
            value: ParsedTime {
                input: "0102",
                time: 01:02:00,
                extended: false,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T010203"), @r###"
        Parsed {
            value: ParsedTime {
                input: "010203",
                time: 01:02:03,
                extended: false,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2024-06-01T010203-05"), @r###"
        Parsed {
            value: ParsedTime {
                input: "010203",
                time: 01:02:03,
                extended: false,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(
            p(b"2024-06-01T010203-05[America/New_York]"), @r###"
        Parsed {
            value: ParsedTime {
                input: "010203",
                time: 01:02:03,
                extended: false,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(
            p(b"2024-06-01T010203[America/New_York]"), @r###"
        Parsed {
            value: ParsedTime {
                input: "010203",
                time: 01:02:03,
                extended: false,
            },
            input: "",
        }
        "###);
    }

    #[test]
    fn err_temporal_time_ambiguous() {
        let p = |input| {
            DateTimeParser::new().parse_temporal_time(input).unwrap_err()
        };

        insta::assert_snapshot!(
            p(b"010203"),
            @r###"parsed time from "010203" is ambiguous with a month-day date"###,
        );
        insta::assert_snapshot!(
            p(b"130112"),
            @r###"parsed time from "130112" is ambiguous with a year-month date"###,
        );
    }

    #[test]
    fn err_temporal_time_missing_time() {
        let p = |input| {
            DateTimeParser::new().parse_temporal_time(input).unwrap_err()
        };

        insta::assert_snapshot!(
            p(b"2024-06-01[America/New_York]"),
            @r###"successfully parsed date from "2024-06-01[America/New_York]", but no time component was found"###,
        );
        // 2099 is not a valid time, but 2099-12-01 is a valid date, so this
        // carves a path where a full datetime parse is OK, but a basic
        // time-only parse is not.
        insta::assert_snapshot!(
            p(b"2099-12-01[America/New_York]"),
            @r###"successfully parsed date from "2099-12-01[America/New_York]", but no time component was found"###,
        );
        // Like above, but this time we use an invalid date. As a result, we
        // get an error reported not on the invalid date, but on how it is an
        // invalid time. (Because we're asking for a time here.)
        insta::assert_snapshot!(
            p(b"2099-13-01[America/New_York]"),
            @r###"failed to parse minute in time "2099-13-01[America/New_York]": minute is not valid: parameter 'minute' with value 99 is not in the required range of 0..=59"###,
        );
    }

    #[test]
    fn err_temporal_time_zulu() {
        let p = |input| {
            DateTimeParser::new().parse_temporal_time(input).unwrap_err()
        };

        insta::assert_snapshot!(
            p(b"T00:00:00Z"),
            @"cannot parse civil time from string with a Zulu offset, parse as a `Timestamp` and convert to a civil time instead",
        );
        insta::assert_snapshot!(
            p(b"00:00:00Z"),
            @"cannot parse plain time from string with a Zulu offset, parse as a `Timestamp` and convert to a plain time instead",
        );
        insta::assert_snapshot!(
            p(b"000000Z"),
            @"cannot parse plain time from string with a Zulu offset, parse as a `Timestamp` and convert to a plain time instead",
        );
        insta::assert_snapshot!(
            p(b"2099-12-01T00:00:00Z"),
            @"cannot parse plain time from full datetime string with a Zulu offset, parse as a `Timestamp` and convert to a plain time instead",
        );
    }

    #[test]
    fn ok_date_basic() {
        let p = |input| DateTimeParser::new().parse_date_spec(input).unwrap();

        insta::assert_debug_snapshot!(p(b"2010-03-14"), @r###"
        Parsed {
            value: ParsedDate {
                input: "2010-03-14",
                date: 2010-03-14,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"20100314"), @r###"
        Parsed {
            value: ParsedDate {
                input: "20100314",
                date: 2010-03-14,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"2010-03-14T01:02:03"), @r###"
        Parsed {
            value: ParsedDate {
                input: "2010-03-14",
                date: 2010-03-14,
            },
            input: "T01:02:03",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"-009999-03-14"), @r###"
        Parsed {
            value: ParsedDate {
                input: "-009999-03-14",
                date: -009999-03-14,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"+009999-03-14"), @r###"
        Parsed {
            value: ParsedDate {
                input: "+009999-03-14",
                date: 9999-03-14,
            },
            input: "",
        }
        "###);
    }

    #[test]
    fn err_date_empty() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"").unwrap_err(),
            @r###"failed to parse year in date "": expected four digit year (or leading sign for six digit year), but found end of input"###,
        );
    }

    #[test]
    fn err_date_year() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"123").unwrap_err(),
            @r###"failed to parse year in date "123": expected four digit year (or leading sign for six digit year), but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"123a").unwrap_err(),
            @r###"failed to parse year in date "123a": failed to parse "123a" as year (a four digit integer): invalid digit, expected 0-9 but got a"###,
        );

        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"-9999").unwrap_err(),
            @r###"failed to parse year in date "-9999": expected six digit year (because of a leading sign), but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"+9999").unwrap_err(),
            @r###"failed to parse year in date "+9999": expected six digit year (because of a leading sign), but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"-99999").unwrap_err(),
            @r###"failed to parse year in date "-99999": expected six digit year (because of a leading sign), but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"+99999").unwrap_err(),
            @r###"failed to parse year in date "+99999": expected six digit year (because of a leading sign), but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"-99999a").unwrap_err(),
            @r###"failed to parse year in date "-99999a": failed to parse "99999a" as year (a six digit integer): invalid digit, expected 0-9 but got a"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"+999999").unwrap_err(),
            @r###"failed to parse year in date "+999999": year is not valid: parameter 'year' with value 999999 is not in the required range of -9999..=9999"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"-010000").unwrap_err(),
            @r###"failed to parse year in date "-010000": year is not valid: parameter 'year' with value 10000 is not in the required range of -9999..=9999"###,
        );
    }

    #[test]
    fn err_date_month() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"2024-").unwrap_err(),
            @r###"failed to parse month in date "2024-": expected two digit month, but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"2024").unwrap_err(),
            @r###"failed to parse month in date "2024": expected two digit month, but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"2024-13-01").unwrap_err(),
            @r###"failed to parse month in date "2024-13-01": month is not valid: parameter 'month' with value 13 is not in the required range of 1..=12"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"20241301").unwrap_err(),
            @r###"failed to parse month in date "20241301": month is not valid: parameter 'month' with value 13 is not in the required range of 1..=12"###,
        );
    }

    #[test]
    fn err_date_day() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"2024-12-").unwrap_err(),
            @r###"failed to parse day in date "2024-12-": expected two digit day, but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"202412").unwrap_err(),
            @r###"failed to parse day in date "202412": expected two digit day, but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"2024-12-40").unwrap_err(),
            @r###"failed to parse day in date "2024-12-40": day is not valid: parameter 'day' with value 40 is not in the required range of 1..=31"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"2024-11-31").unwrap_err(),
            @r###"date parsed from "2024-11-31" is not valid: parameter 'day' with value 31 is not in the required range of 1..=30"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"2024-02-30").unwrap_err(),
            @r###"date parsed from "2024-02-30" is not valid: parameter 'day' with value 30 is not in the required range of 1..=29"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"2023-02-29").unwrap_err(),
            @r###"date parsed from "2023-02-29" is not valid: parameter 'day' with value 29 is not in the required range of 1..=28"###,
        );
    }

    #[test]
    fn err_date_separator() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"2024-1231").unwrap_err(),
            @r###"failed to parse separator after month: expected '-' separator, but found "3" instead"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_date_spec(b"202412-31").unwrap_err(),
            @"failed to parse separator after month: expected no separator after month since none was found after the year, but found a '-' separator",
        );
    }

    #[test]
    fn ok_time_basic() {
        let p = |input| DateTimeParser::new().parse_time_spec(input).unwrap();

        insta::assert_debug_snapshot!(p(b"01:02:03"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02:03",
                time: 01:02:03,
                extended: true,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"010203"), @r###"
        Parsed {
            value: ParsedTime {
                input: "010203",
                time: 01:02:03,
                extended: false,
            },
            input: "",
        }
        "###);
    }

    #[test]
    fn ok_time_fractional() {
        let p = |input| DateTimeParser::new().parse_time_spec(input).unwrap();

        insta::assert_debug_snapshot!(p(b"01:02:03.123456789"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02:03.123456789",
                time: 01:02:03.123456789,
                extended: true,
            },
            input: "",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"010203.123456789"), @r###"
        Parsed {
            value: ParsedTime {
                input: "010203.123456789",
                time: 01:02:03.123456789,
                extended: false,
            },
            input: "",
        }
        "###);

        insta::assert_debug_snapshot!(p(b"01:02:03.9"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02:03.9",
                time: 01:02:03.9,
                extended: true,
            },
            input: "",
        }
        "###);
    }

    #[test]
    fn ok_time_no_fractional() {
        let p = |input| DateTimeParser::new().parse_time_spec(input).unwrap();

        insta::assert_debug_snapshot!(p(b"01:02.123456789"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02",
                time: 01:02:00,
                extended: true,
            },
            input: ".123456789",
        }
        "###);
    }

    #[test]
    fn ok_time_leap() {
        let p = |input| DateTimeParser::new().parse_time_spec(input).unwrap();

        insta::assert_debug_snapshot!(p(b"01:02:60"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02:60",
                time: 01:02:59,
                extended: true,
            },
            input: "",
        }
        "###);
    }

    #[test]
    fn ok_time_mixed_format() {
        let p = |input| DateTimeParser::new().parse_time_spec(input).unwrap();

        insta::assert_debug_snapshot!(p(b"01:0203"), @r###"
        Parsed {
            value: ParsedTime {
                input: "01:02",
                time: 01:02:00,
                extended: true,
            },
            input: "03",
        }
        "###);
        insta::assert_debug_snapshot!(p(b"0102:03"), @r###"
        Parsed {
            value: ParsedTime {
                input: "0102",
                time: 01:02:00,
                extended: false,
            },
            input: ":03",
        }
        "###);
    }

    #[test]
    fn err_time_empty() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"").unwrap_err(),
            @r###"failed to parse hour in time "": expected two digit hour, but found end of input"###,
        );
    }

    #[test]
    fn err_time_hour() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"a").unwrap_err(),
            @r###"failed to parse hour in time "a": expected two digit hour, but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"1a").unwrap_err(),
            @r###"failed to parse hour in time "1a": failed to parse "1a" as hour (a two digit integer): invalid digit, expected 0-9 but got a"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"24").unwrap_err(),
            @r###"failed to parse hour in time "24": hour is not valid: parameter 'hour' with value 24 is not in the required range of 0..=23"###,
        );
    }

    #[test]
    fn err_time_minute() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:").unwrap_err(),
            @r###"failed to parse minute in time "01:": expected two digit minute, but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:a").unwrap_err(),
            @r###"failed to parse minute in time "01:a": expected two digit minute, but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:1a").unwrap_err(),
            @r###"failed to parse minute in time "01:1a": failed to parse "1a" as minute (a two digit integer): invalid digit, expected 0-9 but got a"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:60").unwrap_err(),
            @r###"failed to parse minute in time "01:60": minute is not valid: parameter 'minute' with value 60 is not in the required range of 0..=59"###,
        );
    }

    #[test]
    fn err_time_second() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:02:").unwrap_err(),
            @r###"failed to parse second in time "01:02:": expected two digit second, but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:02:a").unwrap_err(),
            @r###"failed to parse second in time "01:02:a": expected two digit second, but found end of input"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:02:1a").unwrap_err(),
            @r###"failed to parse second in time "01:02:1a": failed to parse "1a" as second (a two digit integer): invalid digit, expected 0-9 but got a"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:02:61").unwrap_err(),
            @r###"failed to parse second in time "01:02:61": second is not valid: parameter 'second' with value 61 is not in the required range of 0..=59"###,
        );
    }

    #[test]
    fn err_time_fractional() {
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:02:03.").unwrap_err(),
            @r###"failed to parse fractional nanoseconds in time "01:02:03.": found decimal after seconds component, but did not find any decimal digits after decimal"###,
        );
        insta::assert_snapshot!(
            DateTimeParser::new().parse_time_spec(b"01:02:03.a").unwrap_err(),
            @r###"failed to parse fractional nanoseconds in time "01:02:03.a": found decimal after seconds component, but did not find any decimal digits after decimal"###,
        );
    }
}
