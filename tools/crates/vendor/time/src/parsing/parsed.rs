//! Information parsed from an input and format description.

use core::num::NonZero;

use deranged::{
    Option_ri8, Option_ri16, Option_ri32, Option_ri128, Option_ru8, Option_ru16, Option_ru32, ri8,
    ri16, ri32, ri128, ru8, ru16, ru32,
};
use num_conv::prelude::*;

use crate::date::{MAX_YEAR, MIN_YEAR};
use crate::error::TryFromParsed::InsufficientInformation;
#[cfg(feature = "alloc")]
use crate::format_description::OwnedFormatItem;
use crate::format_description::format_description_v3::FormatDescriptionV3Inner;
use crate::format_description::{BorrowedFormatItem, Component, Period};
use crate::internal_macros::{bug, const_try_opt};
use crate::parsing::ParsedItem;
use crate::parsing::component::{
    parse_calendar_year_century_extended_range, parse_calendar_year_century_standard_range,
    parse_calendar_year_full_extended_range, parse_calendar_year_full_standard_range,
    parse_calendar_year_last_two, parse_day, parse_end, parse_hour_12, parse_hour_24, parse_ignore,
    parse_iso_year_century_extended_range, parse_iso_year_century_standard_range,
    parse_iso_year_full_extended_range, parse_iso_year_full_standard_range,
    parse_iso_year_last_two, parse_minute, parse_month_long, parse_month_numerical,
    parse_month_short, parse_offset_hour, parse_offset_minute, parse_offset_second, parse_ordinal,
    parse_period, parse_second, parse_subsecond, parse_unix_timestamp_microsecond,
    parse_unix_timestamp_millisecond, parse_unix_timestamp_nanosecond, parse_unix_timestamp_second,
    parse_week_number_iso, parse_week_number_monday, parse_week_number_sunday, parse_weekday_long,
    parse_weekday_monday, parse_weekday_short, parse_weekday_sunday,
};
use crate::unit::{Day, Hour, Minute, Nanosecond, Second};
use crate::{
    Date, Month, OffsetDateTime, PlainDateTime, Time, Timestamp, UtcDateTime, UtcOffset, Weekday,
    error,
};

/// Sealed to prevent downstream implementations.
mod sealed {
    use super::*;

    /// A trait to allow `parse_item` to be generic.
    pub trait AnyFormatItem {
        /// Parse a single item, returning the remaining input on success.
        fn parse_item<'a>(
            &self,
            parsed: &mut Parsed,
            input: &'a [u8],
        ) -> Result<&'a [u8], error::ParseFromDescription>;
    }
}

impl sealed::AnyFormatItem for BorrowedFormatItem<'_> {
    #[inline(always)]
    fn parse_item<'a>(
        &self,
        parsed: &mut Parsed,
        input: &'a [u8],
    ) -> Result<&'a [u8], error::ParseFromDescription> {
        match self {
            #[expect(deprecated)]
            Self::Literal(literal) => Parsed::parse_literal(input, literal),
            Self::StringLiteral(literal) => Parsed::parse_literal(input, literal.as_bytes()),
            Self::Component(component) => parsed.parse_component(input, *component),
            Self::Compound(compound) => parsed.parse_items(input, compound),
            Self::Optional(item) => parsed.parse_item(input, *item).or(Ok(input)),
            Self::First(items) => {
                let mut first_err = None;

                for item in items.iter() {
                    match parsed.parse_item(input, item) {
                        Ok(remaining_input) => return Ok(remaining_input),
                        Err(err) if first_err.is_none() => first_err = Some(err),
                        Err(_) => {}
                    }
                }

                match first_err {
                    Some(err) => Err(err),
                    // This location will be reached if the slice is empty, skipping the `for` loop.
                    // As this case is expected to be uncommon, there's no need to check up front.
                    None => Ok(input),
                }
            }
        }
    }
}

#[cfg(feature = "alloc")]
impl sealed::AnyFormatItem for OwnedFormatItem {
    #[inline]
    fn parse_item<'a>(
        &self,
        parsed: &mut Parsed,
        input: &'a [u8],
    ) -> Result<&'a [u8], error::ParseFromDescription> {
        match self {
            #[expect(deprecated)]
            Self::Literal(literal) => Parsed::parse_literal(input, literal),
            Self::StringLiteral(literal) => Parsed::parse_literal(input, literal.as_bytes()),
            Self::Component(component) => parsed.parse_component(input, *component),
            Self::Compound(compound) => parsed.parse_items(input, compound),
            Self::Optional(item) => parsed.parse_item(input, item.as_ref()).or(Ok(input)),
            Self::First(items) => {
                let mut first_err = None;

                for item in items.iter() {
                    match parsed.parse_item(input, item) {
                        Ok(remaining_input) => return Ok(remaining_input),
                        Err(err) if first_err.is_none() => first_err = Some(err),
                        Err(_) => {}
                    }
                }

                match first_err {
                    Some(err) => Err(err),
                    // This location will be reached if the slice is empty, skipping the `for` loop.
                    // As this case is expected to be uncommon, there's no need to check up front.
                    None => Ok(input),
                }
            }
        }
    }
}

/// All information parsed.
///
/// This information is directly used to construct the final values.
///
/// Most users will not need think about this struct in any way. It is public to allow for manual
/// control over values, in the instance that the default parser is insufficient.
#[derive(Debug, Clone, Copy)]
pub struct Parsed {
    /// Calendar year.
    year: Option_ri32<{ MIN_YEAR }, { MAX_YEAR }>,
    /// All digits except the last two of the calendar year.
    year_century: Option_ri16<{ (MIN_YEAR / 100) as i16 }, { (MAX_YEAR / 100) as i16 }>,
    /// The last two digits of the calendar year.
    year_last_two: Option_ru8<0, 99>,
    /// Year of the [ISO week date](https://en.wikipedia.org/wiki/ISO_week_date).
    iso_year: Option_ri32<{ MIN_YEAR }, { MAX_YEAR }>,
    /// All digits except the last two of the ISO week year.
    iso_year_century: Option_ri16<{ (MIN_YEAR / 100) as i16 }, { (MAX_YEAR / 100) as i16 }>,
    /// The last two digits of the ISO week year.
    iso_year_last_two: Option_ru8<0, 99>,
    /// Month of the year.
    month: Option<Month>,
    /// Week of the year, where week one begins on the first Sunday of the calendar year.
    sunday_week_number: Option_ru8<0, 53>,
    /// Week of the year, where week one begins on the first Monday of the calendar year.
    monday_week_number: Option_ru8<0, 53>,
    /// Week of the year, where week one is the Monday-to-Sunday period containing January 4.
    iso_week_number: Option_ru8<1, 53>,
    /// Day of the week.
    weekday: Option<Weekday>,
    /// Day of the year.
    ordinal: Option_ru16<1, 366>,
    /// Day of the month.
    day: Option_ru8<1, 31>,
    /// Hour within the day.
    hour_24: Option_ru8<0, { Hour::per_t::<u8>(Day) - 1 }>,
    /// Hour within the 12-hour period (midnight to noon or vice versa). This is typically used in
    /// conjunction with AM/PM, which is indicated by the `hour_12_is_pm` field.
    hour_12: Option_ru8<1, 12>,
    /// Whether the `hour_12` field indicates a time that "PM".
    hour_12_is_pm: Option<bool>,
    /// Minute within the hour.
    minute: Option_ru8<0, { Minute::per_t::<u8>(Hour) - 1 }>,
    /// Second within the minute.
    // do not subtract one, as leap seconds may be allowed
    second: Option_ru8<0, { Second::per_t::<u8>(Minute) }>,
    /// Nanosecond within the second.
    subsecond: Option_ru32<0, { Nanosecond::per_t::<u32>(Second) - 1 }>,
    /// Whole hours of the UTC offset.
    offset_hour: Option_ri8<-25, 25>,
    /// Minutes within the hour of the UTC offset.
    offset_minute:
        Option_ri8<{ -Minute::per_t::<i8>(Hour) + 1 }, { Minute::per_t::<i8>(Hour) - 1 }>,
    /// Seconds within the minute of the UTC offset.
    offset_second:
        Option_ri8<{ -Second::per_t::<i8>(Minute) + 1 }, { Second::per_t::<i8>(Minute) - 1 }>,
    /// The Unix timestamp in nanoseconds.
    unix_timestamp_nanos: Option_ri128<
        {
            OffsetDateTime::new_in_offset(Date::MIN, Time::MIDNIGHT, UtcOffset::UTC)
                .unix_timestamp_nanos()
        },
        {
            OffsetDateTime::new_in_offset(Date::MAX, Time::MAX, UtcOffset::UTC)
                .unix_timestamp_nanos()
        },
    >,
    /// Indicates whether the [`UtcOffset`] is negative. This information is obtained when parsing
    /// the offset hour, but may not otherwise be stored due to "-0" being equivalent to "0".
    offset_is_negative: bool,
    /// Indicates whether the `year_century` component is negative. This information is obtained
    /// when parsing, but may not otherwise be stored due to "-0" being equivalent to "0".
    year_century_is_negative: bool,
    /// Indicates whether the `iso_year_century` component is negative. This information is
    /// obtained when parsing, but may not otherwise be stored due to "-0" being equivalent to "0".
    iso_year_century_is_negative: bool,
    /// Indicates whether a leap second is permitted to be parsed. This is required by some
    /// well-known formats.
    pub(super) leap_second_allowed: bool,
}

impl Default for Parsed {
    #[inline]
    fn default() -> Self {
        Self::new()
    }
}

impl Parsed {
    /// Create a new instance of `Parsed` with no information known.
    #[inline]
    pub const fn new() -> Self {
        Self {
            year: Option_ri32::None,
            year_century: Option_ri16::None,
            year_last_two: Option_ru8::None,
            iso_year: Option_ri32::None,
            iso_year_century: Option_ri16::None,
            iso_year_last_two: Option_ru8::None,
            month: None,
            sunday_week_number: Option_ru8::None,
            monday_week_number: Option_ru8::None,
            iso_week_number: Option_ru8::None,
            weekday: None,
            ordinal: Option_ru16::None,
            day: Option_ru8::None,
            hour_24: Option_ru8::None,
            hour_12: Option_ru8::None,
            hour_12_is_pm: None,
            minute: Option_ru8::None,
            second: Option_ru8::None,
            subsecond: Option_ru32::None,
            offset_hour: Option_ri8::None,
            offset_minute: Option_ri8::None,
            offset_second: Option_ri8::None,
            unix_timestamp_nanos: Option_ri128::None,
            offset_is_negative: false,
            year_century_is_negative: false,
            iso_year_century_is_negative: false,
            leap_second_allowed: false,
        }
    }

    /// Parse a [`FormatDescriptionV3Inner`], mutating the struct. The remaining input is returned
    /// as the `Ok` value.
    #[inline]
    pub(crate) fn parse_v3_inner<'a>(
        &mut self,
        mut input: &'a [u8],
        format_description: &FormatDescriptionV3Inner<'_>,
    ) -> Result<&'a [u8], error::ParseFromDescription> {
        use error::ParseFromDescription::InvalidComponent;

        match format_description {
            FormatDescriptionV3Inner::Day(modifiers) => parse_day(input, *modifiers)
                .and_then(|parsed| parsed.consume_value(|value| self.set_day(value)))
                .ok_or(InvalidComponent("day")),
            FormatDescriptionV3Inner::MonthShort(modifiers) => parse_month_short(input, *modifiers)
                .and_then(|parsed| parsed.consume_value(|value| self.set_month(value)))
                .ok_or(InvalidComponent("month")),
            FormatDescriptionV3Inner::MonthLong(modifiers) => parse_month_long(input, *modifiers)
                .and_then(|parsed| parsed.consume_value(|value| self.set_month(value)))
                .ok_or(InvalidComponent("month")),
            FormatDescriptionV3Inner::MonthNumerical(modifiers) => {
                parse_month_numerical(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_month(value)))
                    .ok_or(InvalidComponent("month"))
            }
            FormatDescriptionV3Inner::Ordinal(modifiers) => parse_ordinal(input, *modifiers)
                .and_then(|parsed| parsed.consume_value(|value| self.set_ordinal(value)))
                .ok_or(InvalidComponent("ordinal")),
            FormatDescriptionV3Inner::WeekdayShort(modifiers) => {
                parse_weekday_short(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_weekday(value)))
                    .ok_or(InvalidComponent("weekday"))
            }
            FormatDescriptionV3Inner::WeekdayLong(modifiers) => {
                parse_weekday_long(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_weekday(value)))
                    .ok_or(InvalidComponent("weekday"))
            }
            FormatDescriptionV3Inner::WeekdaySunday(modifiers) => {
                parse_weekday_sunday(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_weekday(value)))
                    .ok_or(InvalidComponent("weekday"))
            }
            FormatDescriptionV3Inner::WeekdayMonday(modifiers) => {
                parse_weekday_monday(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_weekday(value)))
                    .ok_or(InvalidComponent("weekday"))
            }
            FormatDescriptionV3Inner::WeekNumberIso(modifiers) => {
                parse_week_number_iso(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_iso_week_number(NonZero::new(value)?))
                    })
                    .ok_or(InvalidComponent("week number"))
            }
            FormatDescriptionV3Inner::WeekNumberSunday(modifiers) => {
                parse_week_number_sunday(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_sunday_week_number(value))
                    })
                    .ok_or(InvalidComponent("week number"))
            }
            FormatDescriptionV3Inner::WeekNumberMonday(modifiers) => {
                parse_week_number_monday(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_monday_week_number(value))
                    })
                    .ok_or(InvalidComponent("week number"))
            }
            FormatDescriptionV3Inner::CalendarYearFullExtendedRange(modifiers) => {
                parse_calendar_year_full_extended_range(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_year(value)))
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::CalendarYearFullStandardRange(modifiers) => {
                parse_calendar_year_full_standard_range(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_year(value)))
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::IsoYearFullExtendedRange(modifiers) => {
                parse_iso_year_full_extended_range(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_iso_year(value)))
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::IsoYearFullStandardRange(modifiers) => {
                parse_iso_year_full_standard_range(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_iso_year(value)))
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::CalendarYearCenturyExtendedRange(modifiers) => {
                parse_calendar_year_century_extended_range(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|(value, is_negative)| {
                            self.set_year_century(value, is_negative)
                        })
                    })
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::CalendarYearCenturyStandardRange(modifiers) => {
                parse_calendar_year_century_standard_range(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|(value, is_negative)| {
                            self.set_year_century(value, is_negative)
                        })
                    })
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::IsoYearCenturyExtendedRange(modifiers) => {
                parse_iso_year_century_extended_range(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|(value, is_negative)| {
                            self.set_iso_year_century(value, is_negative)
                        })
                    })
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::IsoYearCenturyStandardRange(modifiers) => {
                parse_iso_year_century_standard_range(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|(value, is_negative)| {
                            self.set_iso_year_century(value, is_negative)
                        })
                    })
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::CalendarYearLastTwo(modifiers) => {
                parse_calendar_year_last_two(input, *modifiers)
                    .and_then(|parsed| parsed.consume_value(|value| self.set_year_last_two(value)))
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::IsoYearLastTwo(modifiers) => {
                parse_iso_year_last_two(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_iso_year_last_two(value))
                    })
                    .ok_or(InvalidComponent("year"))
            }
            FormatDescriptionV3Inner::Hour12(modifiers) => parse_hour_12(input, *modifiers)
                .and_then(|parsed| {
                    parsed.consume_value(|value| self.set_hour_12(NonZero::new(value)?))
                })
                .ok_or(InvalidComponent("hour")),
            FormatDescriptionV3Inner::Hour24(modifiers) => parse_hour_24(input, *modifiers)
                .and_then(|parsed| parsed.consume_value(|value| self.set_hour_24(value)))
                .ok_or(InvalidComponent("hour")),
            FormatDescriptionV3Inner::Minute(modifiers) => parse_minute(input, *modifiers)
                .and_then(|parsed| parsed.consume_value(|value| self.set_minute(value)))
                .ok_or(InvalidComponent("minute")),
            FormatDescriptionV3Inner::Period(modifiers) => parse_period(input, *modifiers)
                .and_then(|parsed| {
                    parsed.consume_value(|value| self.set_hour_12_is_pm(value == Period::Pm))
                })
                .ok_or(InvalidComponent("period")),
            FormatDescriptionV3Inner::Second(modifiers) => parse_second(input, *modifiers)
                .and_then(|parsed| parsed.consume_value(|value| self.set_second(value)))
                .ok_or(InvalidComponent("second")),
            FormatDescriptionV3Inner::Subsecond(modifiers) => parse_subsecond(input, *modifiers)
                .and_then(|parsed| parsed.consume_value(|value| self.set_subsecond(value)))
                .ok_or(InvalidComponent("subsecond")),
            FormatDescriptionV3Inner::OffsetHour(modifiers) => parse_offset_hour(input, *modifiers)
                .and_then(|parsed| {
                    parsed.consume_value(|(value, is_negative)| {
                        self.set_offset_hour(value)?;
                        self.offset_is_negative = is_negative;
                        Some(())
                    })
                })
                .ok_or(InvalidComponent("offset hour")),
            FormatDescriptionV3Inner::OffsetMinute(modifiers) => {
                parse_offset_minute(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_offset_minute_signed(value))
                    })
                    .ok_or(InvalidComponent("offset minute"))
            }
            FormatDescriptionV3Inner::OffsetSecond(modifiers) => {
                parse_offset_second(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_offset_second_signed(value))
                    })
                    .ok_or(InvalidComponent("offset second"))
            }
            FormatDescriptionV3Inner::Ignore(modifiers) => {
                let remaining = parse_ignore(input, *modifiers)
                    .map(ParsedItem::<()>::into_inner)
                    .ok_or(InvalidComponent("ignore"))?;

                // Check that the first byte of the remaining input (if any) is not a UTF-8
                // continuation byte. If it is, then we know that the remaining input is not valid
                // UTF-8 and need to return an error. This is needed because v3 format descriptions
                // are guaranteed to be UTF-8.
                if remaining.is_empty() || remaining[0] & 0xC0 != 0x80 {
                    Ok(remaining)
                } else {
                    Err(InvalidComponent("ignore"))
                }
            }
            FormatDescriptionV3Inner::UnixTimestampSecond(modifiers) => {
                parse_unix_timestamp_second(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_unix_timestamp_nanos(value))
                    })
                    .ok_or(InvalidComponent("unix_timestamp"))
            }
            FormatDescriptionV3Inner::UnixTimestampMillisecond(modifiers) => {
                parse_unix_timestamp_millisecond(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_unix_timestamp_nanos(value))
                    })
                    .ok_or(InvalidComponent("unix_timestamp"))
            }
            FormatDescriptionV3Inner::UnixTimestampMicrosecond(modifiers) => {
                parse_unix_timestamp_microsecond(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_unix_timestamp_nanos(value))
                    })
                    .ok_or(InvalidComponent("unix_timestamp"))
            }
            FormatDescriptionV3Inner::UnixTimestampNanosecond(modifiers) => {
                parse_unix_timestamp_nanosecond(input, *modifiers)
                    .and_then(|parsed| {
                        parsed.consume_value(|value| self.set_unix_timestamp_nanos(value))
                    })
                    .ok_or(InvalidComponent("unix_timestamp"))
            }
            FormatDescriptionV3Inner::End(modifiers) => parse_end(input, *modifiers)
                .map(ParsedItem::<()>::into_inner)
                .ok_or(error::ParseFromDescription::UnexpectedTrailingCharacters),
            FormatDescriptionV3Inner::BorrowedLiteral(literal) => {
                Self::parse_literal(input, literal.as_bytes())
            }
            FormatDescriptionV3Inner::BorrowedCompound(items) => {
                let mut this = *self;
                for item in *items {
                    input = this.parse_v3_inner(input, item)?;
                }
                *self = this;
                Ok(input)
            }
            FormatDescriptionV3Inner::BorrowedOptional { format: _, item } => {
                self.parse_v3_inner(input, item).or(Ok(input))
            }
            FormatDescriptionV3Inner::BorrowedFirst(items) => {
                let mut first_err = None;

                for item in items.iter() {
                    match self.parse_v3_inner(input, item) {
                        Ok(remaining_input) => return Ok(remaining_input),
                        Err(err) if first_err.is_none() => first_err = Some(err),
                        Err(_) => {}
                    }
                }

                match first_err {
                    Some(err) => Err(err),
                    // This location will be reached if the slice is empty, skipping the `for` loop.
                    // As this case is expected to be uncommon, there's no need to check up front.
                    None => Ok(input),
                }
            }
            #[cfg(feature = "alloc")]
            FormatDescriptionV3Inner::OwnedLiteral(literal) => {
                Self::parse_literal(input, literal.as_bytes())
            }
            #[cfg(feature = "alloc")]
            FormatDescriptionV3Inner::OwnedCompound(items) => {
                let mut this = *self;
                for item in items {
                    input = this.parse_v3_inner(input, item)?;
                }
                *self = this;
                Ok(input)
            }
            #[cfg(feature = "alloc")]
            FormatDescriptionV3Inner::OwnedOptional { format: _, item } => {
                self.parse_v3_inner(input, item).or(Ok(input))
            }
            #[cfg(feature = "alloc")]
            FormatDescriptionV3Inner::OwnedFirst(items) => {
                let mut first_err = None;

                for item in items {
                    match self.parse_v3_inner(input, item) {
                        Ok(remaining_input) => return Ok(remaining_input),
                        Err(err) if first_err.is_none() => first_err = Some(err),
                        Err(_) => {}
                    }
                }

                match first_err {
                    Some(err) => Err(err),
                    // This location will be reached if the slice is empty, skipping the `for` loop.
                    // As this case is expected to be uncommon, there's no need to check up front.
                    None => Ok(input),
                }
            }
        }
    }

    /// Parse a single [`BorrowedFormatItem`] or [`OwnedFormatItem`], mutating the struct. The
    /// remaining input is returned as the `Ok` value.
    ///
    /// If a [`BorrowedFormatItem::Optional`] or [`OwnedFormatItem::Optional`] is passed, parsing
    /// will not fail; the input will be returned as-is if the expected format is not present.
    #[inline]
    pub fn parse_item<'a>(
        &mut self,
        input: &'a [u8],
        item: &impl sealed::AnyFormatItem,
    ) -> Result<&'a [u8], error::ParseFromDescription> {
        item.parse_item(self, input)
    }

    /// Parse a sequence of [`BorrowedFormatItem`]s or [`OwnedFormatItem`]s, mutating the struct.
    /// The remaining input is returned as the `Ok` value.
    ///
    /// This method will fail if any of the contained [`BorrowedFormatItem`]s or
    /// [`OwnedFormatItem`]s fail to parse. `self` will not be mutated in this instance.
    #[inline]
    pub fn parse_items<'a>(
        &mut self,
        mut input: &'a [u8],
        items: &[impl sealed::AnyFormatItem],
    ) -> Result<&'a [u8], error::ParseFromDescription> {
        // Make a copy that we can mutate. It will only be set to the user's copy if everything
        // succeeds.
        let mut this = *self;
        for item in items {
            input = this.parse_item(input, item)?;
        }
        *self = this;
        Ok(input)
    }

    /// Parse a literal byte sequence. The remaining input is returned as the `Ok` value.
    #[inline]
    pub fn parse_literal<'a>(
        input: &'a [u8],
        literal: &[u8],
    ) -> Result<&'a [u8], error::ParseFromDescription> {
        input
            .strip_prefix(literal)
            .ok_or(error::ParseFromDescription::InvalidLiteral)
    }

    /// Parse a single component, mutating the struct. The remaining input is returned as the `Ok`
    /// value.
    #[inline]
    pub fn parse_component<'a>(
        &mut self,
        input: &'a [u8],
        component: Component,
    ) -> Result<&'a [u8], error::ParseFromDescription> {
        use error::ParseFromDescription::InvalidComponent;

        let v3_fd: FormatDescriptionV3Inner<'static> = component.into();

        // Legacy behavior: `Ignore` consumes bytes without enforcing UTF-8 boundaries.
        if let FormatDescriptionV3Inner::Ignore(modifiers) = v3_fd {
            return parse_ignore(input, modifiers)
                .map(ParsedItem::<()>::into_inner)
                .ok_or(InvalidComponent("ignore"));
        }

        // For all other components, defer to the v3-aware parser.
        self.parse_v3_inner(input, &v3_fd)
    }
}

/// Getter methods
impl Parsed {
    /// Obtain the `year` component.
    #[inline]
    pub const fn year(&self) -> Option<i32> {
        self.year.get_primitive()
    }

    /// Obtain the `year_century` component.
    ///
    /// If the year is zero, the sign of the century is not stored. To differentiate between
    /// positive and negative zero, use `year_century_is_negative`.
    #[inline]
    pub const fn year_century(&self) -> Option<i16> {
        self.year_century.get_primitive()
    }

    /// Obtain the `year_century_is_negative` component.
    ///
    /// This indicates whether the value returned from `year_century` is negative. If the year is
    /// zero, it is necessary to call this method for disambiguation.
    #[inline]
    pub const fn year_century_is_negative(&self) -> Option<bool> {
        match self.year_century() {
            Some(_) => Some(self.year_century_is_negative),
            None => None,
        }
    }

    /// Obtain the `year_last_two` component.
    #[inline]
    pub const fn year_last_two(&self) -> Option<u8> {
        self.year_last_two.get_primitive()
    }

    /// Obtain the `iso_year` component.
    #[inline]
    pub const fn iso_year(&self) -> Option<i32> {
        self.iso_year.get_primitive()
    }

    /// Obtain the `iso_year_century` component.
    ///
    /// If the year is zero, the sign of the century is not stored. To differentiate between
    /// positive and negative zero, use `iso_year_century_is_negative`.
    #[inline]
    pub const fn iso_year_century(&self) -> Option<i16> {
        self.iso_year_century.get_primitive()
    }

    /// Obtain the `iso_year_century_is_negative` component.
    ///
    /// This indicates whether the value returned from `iso_year_century` is negative. If the year
    /// is zero, it is necessary to call this method for disambiguation.
    #[inline]
    pub const fn iso_year_century_is_negative(&self) -> Option<bool> {
        match self.iso_year_century() {
            Some(_) => Some(self.iso_year_century_is_negative),
            None => None,
        }
    }

    /// Obtain the `iso_year_last_two` component.
    #[inline]
    pub const fn iso_year_last_two(&self) -> Option<u8> {
        self.iso_year_last_two.get_primitive()
    }

    /// Obtain the `month` component.
    #[inline]
    pub const fn month(&self) -> Option<Month> {
        self.month
    }

    /// Obtain the `sunday_week_number` component.
    #[inline]
    pub const fn sunday_week_number(&self) -> Option<u8> {
        self.sunday_week_number.get_primitive()
    }

    /// Obtain the `monday_week_number` component.
    #[inline]
    pub const fn monday_week_number(&self) -> Option<u8> {
        self.monday_week_number.get_primitive()
    }

    /// Obtain the `iso_week_number` component.
    #[inline]
    pub const fn iso_week_number(&self) -> Option<NonZero<u8>> {
        NonZero::new(const_try_opt!(self.iso_week_number.get_primitive()))
    }

    /// Obtain the `weekday` component.
    #[inline]
    pub const fn weekday(&self) -> Option<Weekday> {
        self.weekday
    }

    /// Obtain the `ordinal` component.
    #[inline]
    pub const fn ordinal(&self) -> Option<NonZero<u16>> {
        NonZero::new(const_try_opt!(self.ordinal.get_primitive()))
    }

    /// Obtain the `day` component.
    #[inline]
    pub const fn day(&self) -> Option<NonZero<u8>> {
        NonZero::new(const_try_opt!(self.day.get_primitive()))
    }

    /// Obtain the `hour_24` component.
    #[inline]
    pub const fn hour_24(&self) -> Option<u8> {
        self.hour_24.get_primitive()
    }

    /// Obtain the `hour_12` component.
    #[inline]
    pub const fn hour_12(&self) -> Option<NonZero<u8>> {
        NonZero::new(const_try_opt!(self.hour_12.get_primitive()))
    }

    /// Obtain the `hour_12_is_pm` component.
    #[inline]
    pub const fn hour_12_is_pm(&self) -> Option<bool> {
        self.hour_12_is_pm
    }

    /// Obtain the `minute` component.
    #[inline]
    pub const fn minute(&self) -> Option<u8> {
        self.minute.get_primitive()
    }

    /// Obtain the `second` component.
    #[inline]
    pub const fn second(&self) -> Option<u8> {
        self.second.get_primitive()
    }

    /// Obtain the `subsecond` component.
    #[inline]
    pub const fn subsecond(&self) -> Option<u32> {
        self.subsecond.get_primitive()
    }

    /// Obtain the `offset_hour` component.
    #[inline]
    pub const fn offset_hour(&self) -> Option<i8> {
        self.offset_hour.get_primitive()
    }

    /// Obtain the absolute value of the `offset_minute` component.
    #[doc(hidden)]
    #[deprecated(since = "0.3.8", note = "use `parsed.offset_minute_signed()` instead")]
    #[inline]
    pub const fn offset_minute(&self) -> Option<u8> {
        Some(const_try_opt!(self.offset_minute_signed()).unsigned_abs())
    }

    /// Obtain the `offset_minute` component.
    #[inline]
    pub const fn offset_minute_signed(&self) -> Option<i8> {
        match (self.offset_minute.get_primitive(), self.offset_is_negative) {
            (Some(offset_minute), true) => Some(-offset_minute),
            (Some(offset_minute), _) => Some(offset_minute),
            (None, _) => None,
        }
    }

    /// Obtain the absolute value of the `offset_second` component.
    #[doc(hidden)]
    #[deprecated(since = "0.3.8", note = "use `parsed.offset_second_signed()` instead")]
    #[inline]
    pub const fn offset_second(&self) -> Option<u8> {
        Some(const_try_opt!(self.offset_second_signed()).unsigned_abs())
    }

    /// Obtain the `offset_second` component.
    #[inline]
    pub const fn offset_second_signed(&self) -> Option<i8> {
        match (self.offset_second.get_primitive(), self.offset_is_negative) {
            (Some(offset_second), true) => Some(-offset_second),
            (Some(offset_second), _) => Some(offset_second),
            (None, _) => None,
        }
    }

    /// Obtain the `unix_timestamp_nanos` component.
    #[inline]
    pub const fn unix_timestamp_nanos(&self) -> Option<i128> {
        self.unix_timestamp_nanos.get_primitive()
    }
}

/// Generate setters based on the builders.
macro_rules! setters {
    ($($name:ident $setter:ident $builder:ident $type:ty;)*) => {$(
        #[doc = concat!("Set the `", stringify!($name), "` component.")]
        #[inline]
        pub const fn $setter(&mut self, value: $type) -> Option<()> {
            match self.$builder(value) {
                Some(value) => {
                    *self = value;
                    Some(())
                },
                None => None,
            }
        }
    )*};
}

/// Setter methods
///
/// All setters return `Option<()>`, which is `Some` if the value was set, and `None` if not. The
/// setters _may_ fail if the value is invalid, though behavior is not guaranteed.
impl Parsed {
    setters! {
        year set_year with_year i32;
    }

    /// Set the `year_century` component.
    ///
    /// If the value is zero, the sign of the century is taken from the second parameter. Otherwise
    /// the sign is inferred from the value.
    #[inline]
    pub const fn set_year_century(&mut self, value: i16, is_negative: bool) -> Option<()> {
        self.year_century = Option_ri16::Some(const_try_opt!(ri16::new(value)));
        if value != 0 {
            self.year_century_is_negative = value.is_negative();
        } else {
            self.year_century_is_negative = is_negative;
        }
        Some(())
    }

    setters! {
        year_last_two set_year_last_two with_year_last_two u8;
        iso_year set_iso_year with_iso_year i32;
        iso_year_last_two set_iso_year_last_two with_iso_year_last_two u8;
    }

    /// Set the `iso_year_century` component.
    ///
    /// If the value is zero, the sign of the century is taken from the second parameter. Otherwise
    /// the sign is inferred from the value.
    #[inline]
    pub const fn set_iso_year_century(&mut self, value: i16, is_negative: bool) -> Option<()> {
        self.iso_year_century = Option_ri16::Some(const_try_opt!(ri16::new(value)));
        if value != 0 {
            self.iso_year_century_is_negative = value.is_negative();
        } else {
            self.iso_year_century_is_negative = is_negative;
        }
        Some(())
    }

    setters! {
        month set_month with_month Month;
        sunday_week_number set_sunday_week_number with_sunday_week_number u8;
        monday_week_number set_monday_week_number with_monday_week_number u8;
        iso_week_number set_iso_week_number with_iso_week_number NonZero<u8>;
        weekday set_weekday with_weekday Weekday;
        ordinal set_ordinal with_ordinal NonZero<u16>;
        day set_day with_day NonZero<u8>;
        hour_24 set_hour_24 with_hour_24 u8;
        hour_12 set_hour_12 with_hour_12 NonZero<u8>;
        hour_12_is_pm set_hour_12_is_pm with_hour_12_is_pm bool;
        minute set_minute with_minute u8;
        second set_second with_second u8;
        subsecond set_subsecond with_subsecond u32;
        offset_hour set_offset_hour with_offset_hour i8;
        offset_minute set_offset_minute_signed with_offset_minute_signed i8;
        offset_second set_offset_second_signed with_offset_second_signed i8;
        unix_timestamp_nanos set_unix_timestamp_nanos with_unix_timestamp_nanos i128;
    }

    /// Set the `offset_minute` component.
    #[doc(hidden)]
    #[deprecated(
        since = "0.3.8",
        note = "use `parsed.set_offset_minute_signed()` instead"
    )]
    #[inline]
    pub const fn set_offset_minute(&mut self, value: u8) -> Option<()> {
        if value > i8::MAX.cast_unsigned() {
            None
        } else {
            self.set_offset_minute_signed(value.cast_signed())
        }
    }

    /// Set the `offset_minute` component.
    #[doc(hidden)]
    #[deprecated(
        since = "0.3.8",
        note = "use `parsed.set_offset_second_signed()` instead"
    )]
    #[inline]
    pub const fn set_offset_second(&mut self, value: u8) -> Option<()> {
        if value > i8::MAX.cast_unsigned() {
            None
        } else {
            self.set_offset_second_signed(value.cast_signed())
        }
    }
}

/// Builder methods
///
/// All builder methods return `Option<Self>`, which is `Some` if the value was set, and `None` if
/// not. The builder methods _may_ fail if the value is invalid, though behavior is not guaranteed.
impl Parsed {
    /// Set the `year` component and return `self`.
    #[inline]
    pub const fn with_year(mut self, value: i32) -> Option<Self> {
        self.year = Option_ri32::Some(const_try_opt!(ri32::new(value)));
        Some(self)
    }

    /// Set the `year_century` component and return `self`.
    ///
    /// If the value is zero, the sign of the century is taken from the second parameter. Otherwise
    /// the sign is inferred from the value.
    #[inline]
    pub const fn with_year_century(mut self, value: i16, is_negative: bool) -> Option<Self> {
        self.year_century = Option_ri16::Some(const_try_opt!(ri16::new(value)));
        if value != 0 {
            self.year_century_is_negative = value.is_negative();
        } else {
            self.year_century_is_negative = is_negative;
        }
        Some(self)
    }

    /// Set the `year_last_two` component and return `self`.
    #[inline]
    pub const fn with_year_last_two(mut self, value: u8) -> Option<Self> {
        self.year_last_two = Option_ru8::Some(const_try_opt!(ru8::new(value)));
        Some(self)
    }

    /// Set the `iso_year` component and return `self`.
    #[inline]
    pub const fn with_iso_year(mut self, value: i32) -> Option<Self> {
        self.iso_year = Option_ri32::Some(const_try_opt!(ri32::new(value)));
        Some(self)
    }

    /// Set the `iso_year_century` component and return `self`.
    ///
    /// If the value is zero, the sign of the century is taken from the second parameter. Otherwise
    /// the sign is inferred from the value.
    #[inline]
    pub const fn with_iso_year_century(mut self, value: i16, is_negative: bool) -> Option<Self> {
        self.iso_year_century = Option_ri16::Some(const_try_opt!(ri16::new(value)));
        if value != 0 {
            self.iso_year_century_is_negative = value.is_negative();
        } else {
            self.iso_year_century_is_negative = is_negative;
        }
        Some(self)
    }

    /// Set the `iso_year_last_two` component and return `self`.
    #[inline]
    pub const fn with_iso_year_last_two(mut self, value: u8) -> Option<Self> {
        self.iso_year_last_two = Option_ru8::Some(const_try_opt!(ru8::new(value)));
        Some(self)
    }

    /// Set the `month` component and return `self`.
    #[inline]
    pub const fn with_month(mut self, value: Month) -> Option<Self> {
        self.month = Some(value);
        Some(self)
    }

    /// Set the `sunday_week_number` component and return `self`.
    #[inline]
    pub const fn with_sunday_week_number(mut self, value: u8) -> Option<Self> {
        self.sunday_week_number = Option_ru8::Some(const_try_opt!(ru8::new(value)));
        Some(self)
    }

    /// Set the `monday_week_number` component and return `self`.
    #[inline]
    pub const fn with_monday_week_number(mut self, value: u8) -> Option<Self> {
        self.monday_week_number = Option_ru8::Some(const_try_opt!(ru8::new(value)));
        Some(self)
    }

    /// Set the `iso_week_number` component and return `self`.
    #[inline]
    pub const fn with_iso_week_number(mut self, value: NonZero<u8>) -> Option<Self> {
        self.iso_week_number = Option_ru8::Some(const_try_opt!(ru8::new(value.get())));
        Some(self)
    }

    /// Set the `weekday` component and return `self`.
    #[inline]
    pub const fn with_weekday(mut self, value: Weekday) -> Option<Self> {
        self.weekday = Some(value);
        Some(self)
    }

    /// Set the `ordinal` component and return `self`.
    #[inline]
    pub const fn with_ordinal(mut self, value: NonZero<u16>) -> Option<Self> {
        self.ordinal = Option_ru16::Some(const_try_opt!(ru16::new(value.get())));
        Some(self)
    }

    /// Set the `day` component and return `self`.
    #[inline]
    pub const fn with_day(mut self, value: NonZero<u8>) -> Option<Self> {
        self.day = Option_ru8::Some(const_try_opt!(ru8::new(value.get())));
        Some(self)
    }

    /// Set the `hour_24` component and return `self`.
    #[inline]
    pub const fn with_hour_24(mut self, value: u8) -> Option<Self> {
        self.hour_24 = Option_ru8::Some(const_try_opt!(ru8::new(value)));
        Some(self)
    }

    /// Set the `hour_12` component and return `self`.
    #[inline]
    pub const fn with_hour_12(mut self, value: NonZero<u8>) -> Option<Self> {
        self.hour_12 = Option_ru8::Some(const_try_opt!(ru8::new(value.get())));
        Some(self)
    }

    /// Set the `hour_12_is_pm` component and return `self`.
    #[inline]
    pub const fn with_hour_12_is_pm(mut self, value: bool) -> Option<Self> {
        self.hour_12_is_pm = Some(value);
        Some(self)
    }

    /// Set the `minute` component and return `self`.
    #[inline]
    pub const fn with_minute(mut self, value: u8) -> Option<Self> {
        self.minute = Option_ru8::Some(const_try_opt!(ru8::new(value)));
        Some(self)
    }

    /// Set the `second` component and return `self`.
    #[inline]
    pub const fn with_second(mut self, value: u8) -> Option<Self> {
        self.second = Option_ru8::Some(const_try_opt!(ru8::new(value)));
        Some(self)
    }

    /// Set the `subsecond` component and return `self`.
    #[inline]
    pub const fn with_subsecond(mut self, value: u32) -> Option<Self> {
        self.subsecond = Option_ru32::Some(const_try_opt!(ru32::new(value)));
        Some(self)
    }

    /// Set the `offset_hour` component and return `self`.
    #[inline]
    pub const fn with_offset_hour(mut self, value: i8) -> Option<Self> {
        self.offset_hour = Option_ri8::Some(const_try_opt!(ri8::new(value)));
        Some(self)
    }

    /// Set the `offset_minute` component and return `self`.
    #[doc(hidden)]
    #[deprecated(
        since = "0.3.8",
        note = "use `parsed.with_offset_minute_signed()` instead"
    )]
    #[inline]
    pub const fn with_offset_minute(self, value: u8) -> Option<Self> {
        if value > i8::MAX.cast_unsigned() {
            None
        } else {
            self.with_offset_minute_signed(value.cast_signed())
        }
    }

    /// Set the `offset_minute` component and return `self`.
    #[inline]
    pub const fn with_offset_minute_signed(mut self, value: i8) -> Option<Self> {
        self.offset_minute = Option_ri8::Some(const_try_opt!(ri8::new(value)));
        Some(self)
    }

    /// Set the `offset_minute` component and return `self`.
    #[doc(hidden)]
    #[deprecated(
        since = "0.3.8",
        note = "use `parsed.with_offset_second_signed()` instead"
    )]
    #[inline]
    pub const fn with_offset_second(self, value: u8) -> Option<Self> {
        if value > i8::MAX.cast_unsigned() {
            None
        } else {
            self.with_offset_second_signed(value.cast_signed())
        }
    }

    /// Set the `offset_second` component and return `self`.
    #[inline]
    pub const fn with_offset_second_signed(mut self, value: i8) -> Option<Self> {
        self.offset_second = Option_ri8::Some(const_try_opt!(ri8::new(value)));
        Some(self)
    }

    /// Set the `unix_timestamp_nanos` component and return `self`.
    #[inline]
    pub const fn with_unix_timestamp_nanos(mut self, value: i128) -> Option<Self> {
        self.unix_timestamp_nanos = Option_ri128::Some(const_try_opt!(ri128::new(value)));
        Some(self)
    }
}

impl TryFrom<Parsed> for Date {
    type Error = error::TryFromParsed;

    #[inline]
    fn try_from(mut parsed: Parsed) -> Result<Self, Self::Error> {
        /// Match on the components that need to be present.
        macro_rules! match_ {
            (_ => $catch_all:expr $(,)?) => {
                $catch_all
            };
            (($($name:ident),* $(,)?) => $arm:expr, $($rest:tt)*) => {
                if let ($(Some($name)),*) = ($(parsed.$name()),*) {
                    $arm
                } else {
                    match_!($($rest)*)
                }
            };
        }

        /// Get the value needed to adjust the ordinal day for Sunday and Monday-based week
        /// numbering.
        #[inline]
        const fn adjustment(year: i32) -> i16 {
            // Safety: `ordinal` is not zero.
            match unsafe { Date::__from_ordinal_date_unchecked(year, 1) }.weekday() {
                Weekday::Monday => 7,
                Weekday::Tuesday => 1,
                Weekday::Wednesday => 2,
                Weekday::Thursday => 3,
                Weekday::Friday => 4,
                Weekday::Saturday => 5,
                Weekday::Sunday => 6,
            }
        }

        // If we do not have the year but we have *both* the century and the last two digits, we can
        // construct the year. Likewise for the ISO year.
        if let (None, Some(century), Some(is_negative), Some(last_two)) = (
            parsed.year(),
            parsed.year_century(),
            parsed.year_century_is_negative(),
            parsed.year_last_two(),
        ) {
            let year = if is_negative {
                100 * century.widen::<i32>() - last_two.cast_signed().widen::<i32>()
            } else {
                100 * century.widen::<i32>() + last_two.cast_signed().widen::<i32>()
            };
            parsed.year = Option_ri32::from(ri32::new(year));
        }
        if let (None, Some(century), Some(is_negative), Some(last_two)) = (
            parsed.iso_year(),
            parsed.iso_year_century(),
            parsed.iso_year_century_is_negative(),
            parsed.iso_year_last_two(),
        ) {
            let iso_year = if is_negative {
                100 * century.widen::<i32>() - last_two.cast_signed().widen::<i32>()
            } else {
                100 * century.widen::<i32>() + last_two.cast_signed().widen::<i32>()
            };
            parsed.iso_year = Option_ri32::from(ri32::new(iso_year));
        }

        match_! {
            (year, ordinal) => Ok(Self::from_ordinal_date(year, ordinal.get())?),
            (year, month, day) => Ok(Self::from_calendar_date(year, month, day.get())?),
            (iso_year, iso_week_number, weekday) => Ok(Self::from_iso_week_date(
                iso_year,
                iso_week_number.get(),
                weekday,
            )?),
            (year, sunday_week_number, weekday) => Ok(Self::from_ordinal_date(
                year,
                (sunday_week_number.cast_signed().widen::<i16>() * 7
                    + weekday.number_days_from_sunday().cast_signed().widen::<i16>()
                    - adjustment(year)
                    + 1).cast_unsigned(),
            )?),
            (year, monday_week_number, weekday) => Ok(Self::from_ordinal_date(
                year,
                (monday_week_number.cast_signed().widen::<i16>() * 7
                    + weekday.number_days_from_monday().cast_signed().widen::<i16>()
                    - adjustment(year)
                    + 1).cast_unsigned(),
            )?),
            _ => Err(InsufficientInformation),
        }
    }
}

impl TryFrom<Parsed> for Time {
    type Error = error::TryFromParsed;

    #[inline]
    fn try_from(parsed: Parsed) -> Result<Self, Self::Error> {
        let hour = match (parsed.hour_24(), parsed.hour_12(), parsed.hour_12_is_pm()) {
            (Some(hour), _, _) => hour,
            (_, Some(hour), Some(false)) if hour.get() == 12 => 0,
            (_, Some(hour), Some(true)) if hour.get() == 12 => 12,
            (_, Some(hour), Some(false)) => hour.get(),
            (_, Some(hour), Some(true)) => hour.get() + 12,
            _ => return Err(InsufficientInformation),
        };

        if parsed.hour_24().is_none()
            && parsed.hour_12().is_some()
            && parsed.hour_12_is_pm().is_some()
            && parsed.minute().is_none()
            && parsed.second().is_none()
            && parsed.subsecond().is_none()
        {
            return Ok(Self::from_hms_nano(hour, 0, 0, 0)?);
        }

        // Reject combinations such as hour-second with minute omitted.
        match (parsed.minute(), parsed.second(), parsed.subsecond()) {
            (None, None, None) => Ok(Self::from_hms_nano(hour, 0, 0, 0)?),
            (Some(minute), None, None) => Ok(Self::from_hms_nano(hour, minute, 0, 0)?),
            (Some(minute), Some(second), None) => Ok(Self::from_hms_nano(hour, minute, second, 0)?),
            (Some(minute), Some(second), Some(subsecond)) => {
                Ok(Self::from_hms_nano(hour, minute, second, subsecond)?)
            }
            _ => Err(InsufficientInformation),
        }
    }
}

#[inline]
fn utc_offset_try_from_parsed<const REQUIRED: bool>(
    parsed: Parsed,
) -> Result<UtcOffset, error::TryFromParsed> {
    let hour = match (REQUIRED, parsed.offset_hour()) {
        // An offset is required, but the hour is missing. Return an error.
        (true, None) => return Err(InsufficientInformation),
        // An offset is not required (e.g. for `UtcDateTime`). As the hour is missing, minutes and
        // seconds are not parsed. This is UTC.
        (false, None) => return Ok(UtcOffset::UTC),
        // Any other situation has an hour present.
        (_, Some(hour)) => hour,
    };
    let minute = parsed.offset_minute_signed();
    // Force `second` to be `None` if `minute` is `None`.
    let second = minute.and_then(|_| parsed.offset_second_signed());

    let minute = minute.unwrap_or(0);
    let second = second.unwrap_or(0);

    UtcOffset::from_hms(hour, minute, second).map_err(Into::into)
}

impl TryFrom<Parsed> for UtcOffset {
    type Error = error::TryFromParsed;

    #[inline]
    fn try_from(parsed: Parsed) -> Result<Self, Self::Error> {
        utc_offset_try_from_parsed::<true>(parsed)
    }
}

impl TryFrom<Parsed> for PlainDateTime {
    type Error = error::TryFromParsed;

    #[inline]
    fn try_from(parsed: Parsed) -> Result<Self, Self::Error> {
        Ok(Self::new(parsed.try_into()?, parsed.try_into()?))
    }
}

impl TryFrom<Parsed> for UtcDateTime {
    type Error = error::TryFromParsed;

    #[inline]
    fn try_from(mut parsed: Parsed) -> Result<Self, Self::Error> {
        if let Some(timestamp) = parsed.unix_timestamp_nanos() {
            let mut value = Self::from_unix_timestamp_nanos(timestamp)?;
            if let Some(subsecond) = parsed.subsecond() {
                value = value.replace_nanosecond(subsecond)?;
            }
            return Ok(value);
        }

        // Some well-known formats explicitly allow leap seconds. We don't currently support them,
        // so treat it as the nearest preceding moment that can be represented. Because leap seconds
        // always fall at the end of a month UTC, reject any that are at other times.
        let leap_second_input = if parsed.leap_second_allowed && parsed.second() == Some(60) {
            if parsed.set_second(59).is_none() {
                bug!("59 is a valid second");
            }
            if parsed.set_subsecond(999_999_999).is_none() {
                bug!("999_999_999 is a valid subsecond");
            }
            true
        } else {
            false
        };

        let dt = OffsetDateTime::new_in_offset(
            Date::try_from(parsed)?,
            Time::try_from(parsed)?,
            utc_offset_try_from_parsed::<false>(parsed)?,
        )
        .to_utc();

        if leap_second_input && !dt.is_valid_leap_second_stand_in() {
            return Err(error::TryFromParsed::ComponentRange(
                error::ComponentRange::conditional("second"),
            ));
        }
        Ok(dt)
    }
}

impl TryFrom<Parsed> for OffsetDateTime {
    type Error = error::TryFromParsed;

    #[inline]
    fn try_from(mut parsed: Parsed) -> Result<Self, Self::Error> {
        if let Some(timestamp) = parsed.unix_timestamp_nanos() {
            let mut value = Self::from_unix_timestamp_nanos(timestamp)?;
            if let Some(subsecond) = parsed.subsecond() {
                value = value.replace_nanosecond(subsecond)?;
            }
            return Ok(value);
        }

        // Some well-known formats explicitly allow leap seconds. We don't currently support them,
        // so treat it as the nearest preceding moment that can be represented. Because leap seconds
        // always fall at the end of a month UTC, reject any that are at other times.
        let leap_second_input = if parsed.leap_second_allowed && parsed.second() == Some(60) {
            if parsed.set_second(59).is_none() {
                bug!("59 is a valid second");
            }
            if parsed.set_subsecond(999_999_999).is_none() {
                bug!("999_999_999 is a valid subsecond");
            }
            true
        } else {
            false
        };

        let dt = Self::new_in_offset(
            Date::try_from(parsed)?,
            Time::try_from(parsed)?,
            UtcOffset::try_from(parsed)?,
        );

        if leap_second_input && !dt.is_valid_leap_second_stand_in() {
            return Err(error::TryFromParsed::ComponentRange(
                error::ComponentRange::conditional("second"),
            ));
        }
        Ok(dt)
    }
}

impl TryFrom<Parsed> for Timestamp {
    type Error = error::TryFromParsed;

    #[inline]
    fn try_from(parsed: Parsed) -> Result<Self, Self::Error> {
        // Fast path for when we have a timestamp.
        if let Some(timestamp) = parsed.unix_timestamp_nanos() {
            return Ok(Self::from_nanoseconds(timestamp)?);
        }
        // If that's not the case, fall back to a `UtcDateTime` and its parsing.
        if let Ok(dt) = UtcDateTime::try_from(parsed) {
            return Ok(dt.into());
        }
        Err(InsufficientInformation)
    }
}
