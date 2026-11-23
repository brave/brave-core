//! A version 3 format description.
//!
//! Unlike versions 1 and 2, this is opaque so as to permit any changes necessary without breaking
//! downstream users. Other than `FormatDescriptionV3`, all items are internal.

#[cfg(feature = "alloc")]
use alloc::boxed::Box;
use core::fmt;

use crate::format_description::modifier;

/// A complete description of how to format and parse a type.
///
/// Both for forwards compatibility and to enable optimizations, this type is deliberately opaque
/// and cannot be constructed by users of the crate. Instead, it is returned by the
/// `format_description!` macro (when `version=3` is used) as well as the `parse_borrowed` and
/// `parse_owned` methods.
#[derive(Clone)]
pub struct FormatDescriptionV3<'a> {
    /// The inner `enum` that controls all business logic.
    pub(crate) inner: FormatDescriptionV3Inner<'a>,
    /// The maximum number of bytes that are needed to format any value using this format
    /// description.
    #[cfg(feature = "formatting")]
    pub(crate) max_bytes_needed: usize,
}

impl fmt::Debug for FormatDescriptionV3<'_> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        self.inner.fmt(f)
    }
}

impl FormatDescriptionV3<'_> {
    /// Convert the format description to an owned version, enabling it to be stored without regard
    /// for lifetime.
    #[cfg(feature = "alloc")]
    #[inline]
    pub fn to_owned(self) -> FormatDescriptionV3<'static> {
        FormatDescriptionV3 {
            inner: self.inner.into_owned(),
            #[cfg(feature = "formatting")]
            max_bytes_needed: self.max_bytes_needed,
        }
    }
}

/// The inner `enum` of a version 3 format description. Controls all business logic.
// public via `crate::format_description::__private` for macro use
#[non_exhaustive]
#[derive(Clone)]
pub enum FormatDescriptionV3Inner<'a> {
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
    /// A string that is formatted as-is.
    BorrowedLiteral(&'a str),
    /// A series of literals or components that collectively form a partial or complete description.
    BorrowedCompound(&'a [Self]),
    /// An item that may or may not be present when parsing. If parsing fails, there will be no
    /// effect on the resulting `struct`.
    BorrowedOptional {
        /// Whether the item should be formatted.
        format: bool,
        /// The item in question.
        item: &'a Self,
    },
    /// A series of items where, when parsing, the first successful parse is used. When formatting,
    /// the first item is used. If no items are present, both formatting and parsing are no-ops.
    BorrowedFirst(&'a [Self]),
    /// A string that is formatted as-is.
    #[cfg(feature = "alloc")]
    OwnedLiteral(Box<str>),
    /// A series of literals or components that collectively form a partial or complete description.
    #[cfg(feature = "alloc")]
    OwnedCompound(Box<[Self]>),
    /// An item that may or may not be present when parsing. If parsing fails, there will be no
    /// effect on the resulting `struct`.
    #[cfg(feature = "alloc")]
    OwnedOptional {
        /// Whether the item should be formatted.
        format: bool,
        /// The item in question.
        item: Box<Self>,
    },
    /// A series of items where, when parsing, the first successful parse is used. When formatting,
    /// the first item is used. If no items are present, both formatting and parsing are no-ops.
    #[cfg(feature = "alloc")]
    OwnedFirst(Box<[Self]>),
}

impl fmt::Debug for FormatDescriptionV3Inner<'_> {
    #[inline]
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Day(modifier) => modifier.fmt(f),
            Self::MonthShort(modifier) => modifier.fmt(f),
            Self::MonthLong(modifier) => modifier.fmt(f),
            Self::MonthNumerical(modifier) => modifier.fmt(f),
            Self::Ordinal(modifier) => modifier.fmt(f),
            Self::WeekdayShort(modifier) => modifier.fmt(f),
            Self::WeekdayLong(modifier) => modifier.fmt(f),
            Self::WeekdaySunday(modifier) => modifier.fmt(f),
            Self::WeekdayMonday(modifier) => modifier.fmt(f),
            Self::WeekNumberIso(modifier) => modifier.fmt(f),
            Self::WeekNumberSunday(modifier) => modifier.fmt(f),
            Self::WeekNumberMonday(modifier) => modifier.fmt(f),
            Self::CalendarYearFullExtendedRange(modifier) => modifier.fmt(f),
            Self::CalendarYearFullStandardRange(modifier) => modifier.fmt(f),
            Self::IsoYearFullExtendedRange(modifier) => modifier.fmt(f),
            Self::IsoYearFullStandardRange(modifier) => modifier.fmt(f),
            Self::CalendarYearCenturyExtendedRange(modifier) => modifier.fmt(f),
            Self::CalendarYearCenturyStandardRange(modifier) => modifier.fmt(f),
            Self::IsoYearCenturyExtendedRange(modifier) => modifier.fmt(f),
            Self::IsoYearCenturyStandardRange(modifier) => modifier.fmt(f),
            Self::CalendarYearLastTwo(modifier) => modifier.fmt(f),
            Self::IsoYearLastTwo(modifier) => modifier.fmt(f),
            Self::Hour12(modifier) => modifier.fmt(f),
            Self::Hour24(modifier) => modifier.fmt(f),
            Self::Minute(modifier) => modifier.fmt(f),
            Self::Period(modifier) => modifier.fmt(f),
            Self::Second(modifier) => modifier.fmt(f),
            Self::Subsecond(modifier) => modifier.fmt(f),
            Self::OffsetHour(modifier) => modifier.fmt(f),
            Self::OffsetMinute(modifier) => modifier.fmt(f),
            Self::OffsetSecond(modifier) => modifier.fmt(f),
            Self::Ignore(modifier) => modifier.fmt(f),
            Self::UnixTimestampSecond(modifier) => modifier.fmt(f),
            Self::UnixTimestampMillisecond(modifier) => modifier.fmt(f),
            Self::UnixTimestampMicrosecond(modifier) => modifier.fmt(f),
            Self::UnixTimestampNanosecond(modifier) => modifier.fmt(f),
            Self::End(modifier) => modifier.fmt(f),
            Self::BorrowedLiteral(literal) => f.debug_tuple("Literal").field(literal).finish(),
            Self::BorrowedCompound(compound) => f.debug_tuple("Compound").field(compound).finish(),
            Self::BorrowedOptional {
                format: should_format,
                item,
            } => f
                .debug_struct("Optional")
                .field("should_format", should_format)
                .field("item", item)
                .finish(),
            Self::BorrowedFirst(items) => f.debug_tuple("First").field(items).finish(),
            #[cfg(feature = "alloc")]
            Self::OwnedLiteral(literal) => f.debug_tuple("Literal").field(literal).finish(),
            #[cfg(feature = "alloc")]
            Self::OwnedCompound(compound) => f.debug_tuple("Compound").field(compound).finish(),
            #[cfg(feature = "alloc")]
            Self::OwnedOptional {
                format: should_format,
                item,
            } => f
                .debug_struct("Optional")
                .field("should_format", should_format)
                .field("item", item)
                .finish(),
            #[cfg(feature = "alloc")]
            Self::OwnedFirst(items) => f.debug_tuple("First").field(items).finish(),
        }
    }
}

impl<'a> FormatDescriptionV3Inner<'a> {
    /// Recursively convert to an owned version, doing so in-place when possible.
    #[cfg(feature = "alloc")]
    fn make_owned_in_place(&mut self) {
        use alloc::borrow::ToOwned as _;
        use alloc::boxed::Box;

        match self {
            Self::BorrowedLiteral(literal) => {
                *self = Self::OwnedLiteral(literal.to_owned().into_boxed_str());
            }
            Self::BorrowedCompound(compound) => {
                *self = Self::OwnedCompound(
                    compound
                        .iter()
                        .cloned()
                        .map(|item| item.into_owned())
                        .collect(),
                );
            }
            Self::BorrowedOptional { format, item } => {
                *self = Self::OwnedOptional {
                    format: *format,
                    item: Box::new(item.clone().into_owned()),
                };
            }
            Self::BorrowedFirst(items) => {
                *self = Self::OwnedFirst(
                    items
                        .iter()
                        .cloned()
                        .map(|item| item.into_owned())
                        .collect(),
                );
            }
            Self::OwnedCompound(compound) => {
                for item in compound {
                    item.make_owned_in_place();
                }
            }
            Self::OwnedOptional { format: _, item } => {
                item.make_owned_in_place();
            }
            Self::OwnedFirst(items) => {
                for item in items {
                    item.make_owned_in_place();
                }
            }
            FormatDescriptionV3Inner::Day(_)
            | FormatDescriptionV3Inner::MonthShort(_)
            | FormatDescriptionV3Inner::MonthLong(_)
            | FormatDescriptionV3Inner::MonthNumerical(_)
            | FormatDescriptionV3Inner::Ordinal(_)
            | FormatDescriptionV3Inner::WeekdayShort(_)
            | FormatDescriptionV3Inner::WeekdayLong(_)
            | FormatDescriptionV3Inner::WeekdaySunday(_)
            | FormatDescriptionV3Inner::WeekdayMonday(_)
            | FormatDescriptionV3Inner::WeekNumberIso(_)
            | FormatDescriptionV3Inner::WeekNumberSunday(_)
            | FormatDescriptionV3Inner::WeekNumberMonday(_)
            | FormatDescriptionV3Inner::CalendarYearFullExtendedRange(_)
            | FormatDescriptionV3Inner::CalendarYearFullStandardRange(_)
            | FormatDescriptionV3Inner::IsoYearFullExtendedRange(_)
            | FormatDescriptionV3Inner::IsoYearFullStandardRange(_)
            | FormatDescriptionV3Inner::CalendarYearCenturyExtendedRange(_)
            | FormatDescriptionV3Inner::CalendarYearCenturyStandardRange(_)
            | FormatDescriptionV3Inner::IsoYearCenturyExtendedRange(_)
            | FormatDescriptionV3Inner::IsoYearCenturyStandardRange(_)
            | FormatDescriptionV3Inner::CalendarYearLastTwo(_)
            | FormatDescriptionV3Inner::IsoYearLastTwo(_)
            | FormatDescriptionV3Inner::Hour12(_)
            | FormatDescriptionV3Inner::Hour24(_)
            | FormatDescriptionV3Inner::Minute(_)
            | FormatDescriptionV3Inner::Period(_)
            | FormatDescriptionV3Inner::Second(_)
            | FormatDescriptionV3Inner::Subsecond(_)
            | FormatDescriptionV3Inner::OffsetHour(_)
            | FormatDescriptionV3Inner::OffsetMinute(_)
            | FormatDescriptionV3Inner::OffsetSecond(_)
            | FormatDescriptionV3Inner::Ignore(_)
            | FormatDescriptionV3Inner::UnixTimestampSecond(_)
            | FormatDescriptionV3Inner::UnixTimestampMillisecond(_)
            | FormatDescriptionV3Inner::UnixTimestampMicrosecond(_)
            | FormatDescriptionV3Inner::UnixTimestampNanosecond(_)
            | FormatDescriptionV3Inner::End(_)
            | FormatDescriptionV3Inner::OwnedLiteral(_) => {
                // no-op, as these variants do not contain any references
            }
        }
    }

    /// Convert the format description to an owned version in place, replacing borrowed
    /// components with their owned equivalents.
    #[cfg(feature = "alloc")]
    pub(super) fn into_owned(mut self) -> FormatDescriptionV3Inner<'static> {
        self.make_owned_in_place();

        // Safety: `make_owned_in_place` recursively eliminates all variants that contain
        // references, so we can transmute between lifetimes freely. ADTs do not vary in layout when
        // only the lifetime differs.
        unsafe {
            core::mem::transmute::<FormatDescriptionV3Inner<'a>, FormatDescriptionV3Inner<'static>>(
                self,
            )
        }
    }

    /// Convert the inner `enum` to a `FormatDescriptionV3`.
    #[inline]
    pub const fn into_opaque(self) -> FormatDescriptionV3<'a> {
        FormatDescriptionV3 {
            #[cfg(feature = "formatting")]
            max_bytes_needed: self.max_bytes_needed(),
            inner: self,
        }
    }

    /// Obtain the maximum number of bytes that are needed to format any value using this format
    /// description.
    #[cfg(feature = "formatting")]
    const fn max_bytes_needed(&self) -> usize {
        match self {
            Self::Day(_) => 2,
            Self::MonthShort(_) => 3,
            Self::MonthLong(_) => 9,
            Self::MonthNumerical(_) => 2,
            Self::Ordinal(_) => 3,
            Self::WeekdayShort(_) => 3,
            Self::WeekdayLong(_) => 9,
            Self::WeekdaySunday(_) | Self::WeekdayMonday(_) => 1,
            Self::WeekNumberIso(_) | Self::WeekNumberSunday(_) | Self::WeekNumberMonday(_) => 2,
            Self::CalendarYearFullExtendedRange(_) => 7,
            Self::CalendarYearFullStandardRange(_) => 5,
            Self::IsoYearFullExtendedRange(_) => 7,
            Self::IsoYearFullStandardRange(_) => 5,
            Self::CalendarYearCenturyExtendedRange(_) => 5,
            Self::CalendarYearCenturyStandardRange(_) => 3,
            Self::IsoYearCenturyExtendedRange(_) => 5,
            Self::IsoYearCenturyStandardRange(_) => 3,
            Self::CalendarYearLastTwo(_) => 2,
            Self::IsoYearLastTwo(_) => 2,
            Self::Hour12(_) | Self::Hour24(_) => 2,
            Self::Minute(_) | Self::Period(_) | Self::Second(_) => 2,
            Self::Subsecond(modifier) => match modifier.digits {
                modifier::SubsecondDigits::One => 1,
                modifier::SubsecondDigits::Two => 2,
                modifier::SubsecondDigits::Three => 3,
                modifier::SubsecondDigits::Four => 4,
                modifier::SubsecondDigits::Five => 5,
                modifier::SubsecondDigits::Six => 6,
                modifier::SubsecondDigits::Seven => 7,
                modifier::SubsecondDigits::Eight => 8,
                modifier::SubsecondDigits::Nine => 9,
                modifier::SubsecondDigits::OneOrMore => 9,
            },
            Self::OffsetHour(_) => 3,
            Self::OffsetMinute(_) | Self::OffsetSecond(_) => 2,
            #[cfg(feature = "large-dates")]
            Self::UnixTimestampSecond(_) => 15,
            #[cfg(not(feature = "large-dates"))]
            Self::UnixTimestampSecond(_) => 13,
            #[cfg(feature = "large-dates")]
            Self::UnixTimestampMillisecond(_) => 18,
            #[cfg(not(feature = "large-dates"))]
            Self::UnixTimestampMillisecond(_) => 16,
            #[cfg(feature = "large-dates")]
            Self::UnixTimestampMicrosecond(_) => 21,
            #[cfg(not(feature = "large-dates"))]
            Self::UnixTimestampMicrosecond(_) => 19,
            #[cfg(feature = "large-dates")]
            Self::UnixTimestampNanosecond(_) => 24,
            #[cfg(not(feature = "large-dates"))]
            Self::UnixTimestampNanosecond(_) => 22,
            Self::Ignore(_) | Self::End(_) => 0,
            FormatDescriptionV3Inner::BorrowedLiteral(s) => s.len(),
            FormatDescriptionV3Inner::BorrowedCompound(items) => {
                let mut max_bytes_needed = 0;
                let mut idx = 0;
                while idx < items.len() {
                    max_bytes_needed += items[idx].max_bytes_needed();
                    idx += 1;
                }
                max_bytes_needed
            }
            FormatDescriptionV3Inner::BorrowedOptional { format, item } => {
                if *format {
                    item.max_bytes_needed()
                } else {
                    0
                }
            }
            FormatDescriptionV3Inner::BorrowedFirst(items) => {
                if items.is_empty() {
                    0
                } else {
                    items[0].max_bytes_needed()
                }
            }
            FormatDescriptionV3Inner::OwnedLiteral(s) => s.len(),
            FormatDescriptionV3Inner::OwnedCompound(items) => {
                let mut max_bytes_needed = 0;
                let mut idx = 0;
                while idx < items.len() {
                    max_bytes_needed += items[idx].max_bytes_needed();
                    idx += 1;
                }
                max_bytes_needed
            }
            FormatDescriptionV3Inner::OwnedOptional { format, item } => {
                if *format {
                    item.max_bytes_needed()
                } else {
                    0
                }
            }
            FormatDescriptionV3Inner::OwnedFirst(items) => {
                if items.is_empty() {
                    0
                } else {
                    items[0].max_bytes_needed()
                }
            }
        }
    }
}

/// A component of a larger format description.
// public via `crate::format_description::__private` for macro use
#[non_exhaustive]
#[derive(Debug, Clone, Copy)]
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
}
