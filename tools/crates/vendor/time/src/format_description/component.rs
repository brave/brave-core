//! Part of a format description.

use crate::format_description::modifier;

/// A component of a larger format description.
#[non_exhaustive]
#[allow(deprecated)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Component {
    /// Day of the month.
    Day(modifier::Day),
    /// Month of the year in the abbreviated form (e.g. "Jan").
    MonthShort(modifier::MonthShort),
    /// Month of the year in the full form (e.g. "January").
    MonthLong(modifier::MonthLong),
    /// Month of the year in the numerical form (e.g. "1" for January).
    MonthNumerical(modifier::MonthNumerical),
    /// Ordinal day of the year.
    Ordinal(modifier::Ordinal),
    /// Weekday in the abbreviated form (e.g. "Mon").
    WeekdayShort(modifier::WeekdayShort),
    /// Weekday in the full form (e.g. "Monday").
    WeekdayLong(modifier::WeekdayLong),
    /// Weekday number where Sunday is either 0 or 1 depending on the modifier.
    WeekdaySunday(modifier::WeekdaySunday),
    /// Weekday number where Monday is either 0 or 1 depending on the modifier.
    WeekdayMonday(modifier::WeekdayMonday),
    /// Week number of the year, where week 1 starts is the week beginning on Monday that contains
    /// January 4.
    WeekNumberIso(modifier::WeekNumberIso),
    /// Week number of the year, where week 1 starts on the first Sunday of the calendar year.
    WeekNumberSunday(modifier::WeekNumberSunday),
    /// Week number of the year, where week 1 starts on the first Monday of the calendar year.
    WeekNumberMonday(modifier::WeekNumberMonday),
    /// The calendar year. Supports the extended range.
    CalendarYearFullExtendedRange(modifier::CalendarYearFullExtendedRange),
    /// The calendar year. Does not support the extended range.
    CalendarYearFullStandardRange(modifier::CalendarYearFullStandardRange),
    /// The ISO week-based year. Supports the extended range.
    IsoYearFullExtendedRange(modifier::IsoYearFullExtendedRange),
    /// The ISO week-based year. Does not support the extended range.
    IsoYearFullStandardRange(modifier::IsoYearFullStandardRange),
    /// The century of the calendar year. Supports the extended range.
    CalendarYearCenturyExtendedRange(modifier::CalendarYearCenturyExtendedRange),
    /// The century of the calendar year. Does not support the extended range.
    CalendarYearCenturyStandardRange(modifier::CalendarYearCenturyStandardRange),
    /// The century of the ISO week-based year. Supports the extended range.
    IsoYearCenturyExtendedRange(modifier::IsoYearCenturyExtendedRange),
    /// The century of the ISO week-based year. Does not support the extended range.
    IsoYearCenturyStandardRange(modifier::IsoYearCenturyStandardRange),
    /// The last two digits of the calendar year.
    CalendarYearLastTwo(modifier::CalendarYearLastTwo),
    /// The last two digits of the ISO week-based year.
    IsoYearLastTwo(modifier::IsoYearLastTwo),
    /// Hour of the day using the 12-hour clock.
    Hour12(modifier::Hour12),
    /// Hour of the day using the 24-hour clock.
    Hour24(modifier::Hour24),
    /// Minute within the hour.
    Minute(modifier::Minute),
    /// AM/PM part of the time.
    Period(modifier::Period),
    /// Second within the minute.
    Second(modifier::Second),
    /// Subsecond within the second.
    Subsecond(modifier::Subsecond),
    /// Hour of the UTC offset.
    OffsetHour(modifier::OffsetHour),
    /// Minute within the hour of the UTC offset.
    OffsetMinute(modifier::OffsetMinute),
    /// Second within the minute of the UTC offset.
    OffsetSecond(modifier::OffsetSecond),
    /// A number of bytes to ignore when parsing. This has no effect on formatting.
    Ignore(modifier::Ignore),
    /// A Unix timestamp in seconds.
    UnixTimestampSecond(modifier::UnixTimestampSecond),
    /// A Unix timestamp in milliseconds.
    UnixTimestampMillisecond(modifier::UnixTimestampMillisecond),
    /// A Unix timestamp in microseconds.
    UnixTimestampMicrosecond(modifier::UnixTimestampMicrosecond),
    /// A Unix timestamp in nanoseconds.
    UnixTimestampNanosecond(modifier::UnixTimestampNanosecond),
    /// The end of input. Parsing this component will fail if there is any input remaining. This
    /// component neither affects formatting nor consumes any input when parsing.
    End(modifier::End),

    // Start of deprecated components that are no longer emitted by macros or parsers. They must
    // be maintained for backward compatibility, as downstream users could have constructed them
    // manually.
    /// Month of the year.
    #[deprecated(
        since = "0.3.48",
        note = "use `MonthShort`, `MonthLong`, or `MonthNumeric` instead"
    )]
    Month(modifier::Month),
    /// Day of the week.
    #[deprecated(
        since = "0.3.48",
        note = "use `WeekdayShort`, `WeekdayLong`, or `WeekdaySunday`, or `WeekdayMonday` instead"
    )]
    Weekday(modifier::Weekday),
    /// Week within the year.
    #[deprecated(
        since = "0.3.48",
        note = "use `WeekNumberIso`, `WeekNumberSunday`, or `WeekNumberMonday` instead"
    )]
    WeekNumber(modifier::WeekNumber),
    /// Hour of the day.
    #[deprecated(since = "0.3.48", note = "use `Hour12` or `Hour24` instead")]
    Hour(modifier::Hour),
    /// A Unix timestamp.
    #[deprecated(
        since = "0.3.48",
        note = "use `UnixTimestampSeconds`, `UnixTimestampMilliseconds`, \
                `UnixTimestampMicroseconds`, or `UnixTimestampNanoseconds` instead"
    )]
    UnixTimestamp(modifier::UnixTimestamp),
    /// Year of the date.
    #[deprecated(
        since = "0.3.48",
        note = "use one of the various `Year*` components instead"
    )]
    Year(modifier::Year),
}

impl From<Component> for super::format_description_v3::FormatDescriptionV3Inner<'_> {
    #[inline]
    fn from(component: Component) -> Self {
        match component {
            Component::Day(modifier) => Self::Day(modifier),
            Component::MonthShort(modifier) => Self::MonthShort(modifier),
            Component::MonthLong(modifier) => Self::MonthLong(modifier),
            Component::MonthNumerical(modifier) => Self::MonthNumerical(modifier),
            Component::Ordinal(modifier) => Self::Ordinal(modifier),
            Component::WeekdayShort(modifier) => Self::WeekdayShort(modifier),
            Component::WeekdayLong(modifier) => Self::WeekdayLong(modifier),
            Component::WeekdaySunday(modifier) => Self::WeekdaySunday(modifier),
            Component::WeekdayMonday(modifier) => Self::WeekdayMonday(modifier),
            Component::WeekNumberIso(modifier) => Self::WeekNumberIso(modifier),
            Component::WeekNumberSunday(modifier) => Self::WeekNumberSunday(modifier),
            Component::WeekNumberMonday(modifier) => Self::WeekNumberMonday(modifier),
            Component::CalendarYearFullExtendedRange(modifier) => {
                Self::CalendarYearFullExtendedRange(modifier)
            }
            Component::CalendarYearFullStandardRange(modifier) => {
                Self::CalendarYearFullStandardRange(modifier)
            }
            Component::IsoYearFullExtendedRange(modifier) => {
                Self::IsoYearFullExtendedRange(modifier)
            }
            Component::IsoYearFullStandardRange(modifier) => {
                Self::IsoYearFullStandardRange(modifier)
            }
            Component::CalendarYearCenturyExtendedRange(modifier) => {
                Self::CalendarYearCenturyExtendedRange(modifier)
            }
            Component::CalendarYearCenturyStandardRange(modifier) => {
                Self::CalendarYearCenturyStandardRange(modifier)
            }
            Component::IsoYearCenturyExtendedRange(modifier) => {
                Self::IsoYearCenturyExtendedRange(modifier)
            }
            Component::IsoYearCenturyStandardRange(modifier) => {
                Self::IsoYearCenturyStandardRange(modifier)
            }
            Component::CalendarYearLastTwo(modifier) => Self::CalendarYearLastTwo(modifier),
            Component::IsoYearLastTwo(modifier) => Self::IsoYearLastTwo(modifier),
            Component::Hour12(modifier) => Self::Hour12(modifier),
            Component::Hour24(modifier) => Self::Hour24(modifier),
            Component::Minute(modifier) => Self::Minute(modifier),
            Component::Period(modifier) => Self::Period(modifier),
            Component::Second(modifier) => Self::Second(modifier),
            Component::Subsecond(modifier) => Self::Subsecond(modifier),
            Component::OffsetHour(modifier) => Self::OffsetHour(modifier),
            Component::OffsetMinute(modifier) => Self::OffsetMinute(modifier),
            Component::OffsetSecond(modifier) => Self::OffsetSecond(modifier),
            Component::Ignore(modifier) => Self::Ignore(modifier),
            Component::UnixTimestampSecond(modifier) => Self::UnixTimestampSecond(modifier),
            Component::UnixTimestampMillisecond(modifier) => {
                Self::UnixTimestampMillisecond(modifier)
            }
            Component::UnixTimestampMicrosecond(modifier) => {
                Self::UnixTimestampMicrosecond(modifier)
            }
            Component::UnixTimestampNanosecond(modifier) => Self::UnixTimestampNanosecond(modifier),
            Component::End(modifier) => Self::End(modifier),

            // Start of deprecated components.
            #[expect(deprecated)]
            Component::Month(modifier) => match modifier.repr {
                modifier::MonthRepr::Short => Self::MonthShort(
                    modifier::MonthShort::default().with_case_sensitive(modifier.case_sensitive),
                ),
                modifier::MonthRepr::Long => Self::MonthLong(
                    modifier::MonthLong::default().with_case_sensitive(modifier.case_sensitive),
                ),
                modifier::MonthRepr::Numerical => Self::MonthNumerical(
                    modifier::MonthNumerical::default().with_padding(modifier.padding),
                ),
            },
            #[expect(deprecated)]
            Component::Weekday(modifier) => match modifier.repr {
                modifier::WeekdayRepr::Short => Self::WeekdayShort(
                    modifier::WeekdayShort::default().with_case_sensitive(modifier.case_sensitive),
                ),
                modifier::WeekdayRepr::Long => Self::WeekdayLong(
                    modifier::WeekdayLong::default().with_case_sensitive(modifier.case_sensitive),
                ),
                modifier::WeekdayRepr::Sunday => Self::WeekdaySunday(
                    modifier::WeekdaySunday::default().with_one_indexed(modifier.one_indexed),
                ),
                modifier::WeekdayRepr::Monday => Self::WeekdayMonday(
                    modifier::WeekdayMonday::default().with_one_indexed(modifier.one_indexed),
                ),
            },
            #[expect(deprecated)]
            Component::WeekNumber(modifier) => match modifier.repr {
                modifier::WeekNumberRepr::Iso => Self::WeekNumberIso(
                    modifier::WeekNumberIso::default().with_padding(modifier.padding),
                ),
                modifier::WeekNumberRepr::Sunday => Self::WeekNumberSunday(
                    modifier::WeekNumberSunday::default().with_padding(modifier.padding),
                ),
                modifier::WeekNumberRepr::Monday => Self::WeekNumberMonday(
                    modifier::WeekNumberMonday::default().with_padding(modifier.padding),
                ),
            },
            #[expect(deprecated)]
            Component::Hour(modifier) => {
                if modifier.is_12_hour_clock {
                    Self::Hour12(modifier::Hour12::default().with_padding(modifier.padding))
                } else {
                    Self::Hour24(modifier::Hour24::default().with_padding(modifier.padding))
                }
            }
            #[expect(deprecated)]
            Component::UnixTimestamp(modifier) => match modifier.precision {
                modifier::UnixTimestampPrecision::Second => Self::UnixTimestampSecond(
                    modifier::UnixTimestampSecond::default()
                        .with_sign_is_mandatory(modifier.sign_is_mandatory),
                ),
                modifier::UnixTimestampPrecision::Millisecond => Self::UnixTimestampMillisecond(
                    modifier::UnixTimestampMillisecond::default()
                        .with_sign_is_mandatory(modifier.sign_is_mandatory),
                ),
                modifier::UnixTimestampPrecision::Microsecond => Self::UnixTimestampMicrosecond(
                    modifier::UnixTimestampMicrosecond::default()
                        .with_sign_is_mandatory(modifier.sign_is_mandatory),
                ),
                modifier::UnixTimestampPrecision::Nanosecond => Self::UnixTimestampNanosecond(
                    modifier::UnixTimestampNanosecond::default()
                        .with_sign_is_mandatory(modifier.sign_is_mandatory),
                ),
            },
            #[expect(deprecated)]
            Component::Year(modifier) => {
                match (modifier.iso_week_based, modifier.repr, modifier.range) {
                    (true, modifier::YearRepr::Full, modifier::YearRange::Standard) => {
                        Self::IsoYearFullStandardRange(
                            modifier::IsoYearFullStandardRange::default()
                                .with_padding(modifier.padding)
                                .with_sign_is_mandatory(modifier.sign_is_mandatory),
                        )
                    }
                    (true, modifier::YearRepr::Full, modifier::YearRange::Extended) => {
                        Self::IsoYearFullExtendedRange(
                            modifier::IsoYearFullExtendedRange::default()
                                .with_padding(modifier.padding)
                                .with_sign_is_mandatory(modifier.sign_is_mandatory),
                        )
                    }
                    (false, modifier::YearRepr::Full, modifier::YearRange::Standard) => {
                        Self::CalendarYearFullStandardRange(
                            modifier::CalendarYearFullStandardRange::default()
                                .with_padding(modifier.padding)
                                .with_sign_is_mandatory(modifier.sign_is_mandatory),
                        )
                    }
                    (false, modifier::YearRepr::Full, modifier::YearRange::Extended) => {
                        Self::CalendarYearFullExtendedRange(
                            modifier::CalendarYearFullExtendedRange::default()
                                .with_padding(modifier.padding)
                                .with_sign_is_mandatory(modifier.sign_is_mandatory),
                        )
                    }
                    (true, modifier::YearRepr::Century, modifier::YearRange::Standard) => {
                        Self::IsoYearCenturyStandardRange(
                            modifier::IsoYearCenturyStandardRange::default()
                                .with_padding(modifier.padding)
                                .with_sign_is_mandatory(modifier.sign_is_mandatory),
                        )
                    }
                    (true, modifier::YearRepr::Century, modifier::YearRange::Extended) => {
                        Self::IsoYearCenturyExtendedRange(
                            modifier::IsoYearCenturyExtendedRange::default()
                                .with_padding(modifier.padding)
                                .with_sign_is_mandatory(modifier.sign_is_mandatory),
                        )
                    }
                    (false, modifier::YearRepr::Century, modifier::YearRange::Standard) => {
                        Self::CalendarYearCenturyStandardRange(
                            modifier::CalendarYearCenturyStandardRange::default()
                                .with_padding(modifier.padding)
                                .with_sign_is_mandatory(modifier.sign_is_mandatory),
                        )
                    }
                    (false, modifier::YearRepr::Century, modifier::YearRange::Extended) => {
                        Self::CalendarYearCenturyExtendedRange(
                            modifier::CalendarYearCenturyExtendedRange::default()
                                .with_padding(modifier.padding)
                                .with_sign_is_mandatory(modifier.sign_is_mandatory),
                        )
                    }
                    (true, modifier::YearRepr::LastTwo, modifier::YearRange::Standard) => {
                        Self::IsoYearLastTwo(
                            modifier::IsoYearLastTwo::default().with_padding(modifier.padding),
                        )
                    }
                    (true, modifier::YearRepr::LastTwo, modifier::YearRange::Extended) => {
                        Self::IsoYearLastTwo(
                            modifier::IsoYearLastTwo::default().with_padding(modifier.padding),
                        )
                    }
                    (false, modifier::YearRepr::LastTwo, modifier::YearRange::Standard) => {
                        Self::CalendarYearLastTwo(
                            modifier::CalendarYearLastTwo::default().with_padding(modifier.padding),
                        )
                    }
                    (false, modifier::YearRepr::LastTwo, modifier::YearRange::Extended) => {
                        Self::CalendarYearLastTwo(
                            modifier::CalendarYearLastTwo::default().with_padding(modifier.padding),
                        )
                    }
                }
            }
        }
    }
}
