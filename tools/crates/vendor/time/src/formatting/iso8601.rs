//! Helpers for implementing formatting for ISO 8601.

use core::cmp::min;
use std::io;

use deranged::{ru8, ru16, ru32};
use num_conv::prelude::*;

use crate::error;
use crate::format_description::modifier::Padding;
use crate::format_description::well_known::Iso8601;
use crate::format_description::well_known::iso8601::{
    DateKind, EncodedConfig, OffsetPrecision, TimePrecision,
};
use crate::formatting::{
    ComponentProvider, format_float, format_four_digits_pad_zero, format_int_padded,
    format_single_digit, format_six_digits_pad_zero, format_three_digits, format_two_digits, write,
    write_if, write_if_else,
};
use crate::unit::*;

/// Format the date portion of ISO 8601.
pub(super) fn format_date<V, const CONFIG: EncodedConfig>(
    output: &mut (impl io::Write + ?Sized),
    value: &V,
    state: &mut V::State,
) -> Result<usize, error::Format>
where
    V: ComponentProvider,
{
    let mut bytes = 0;

    match Iso8601::<CONFIG>::DATE_KIND {
        DateKind::Calendar => {
            let year = value.calendar_year(state).get();

            if Iso8601::<CONFIG>::YEAR_IS_SIX_DIGITS {
                bytes += write_if_else(output, year < 0, "-", "+")?;
                // Safety: `calendar_year` returns a value whose absolute value is guaranteed to be
                // less than 1,000,000.
                bytes += format_six_digits_pad_zero(output, unsafe {
                    ru32::new_unchecked(year.unsigned_abs())
                })?;
            } else {
                let year = ru16::new(year.cast_unsigned().truncate())
                    .ok_or(error::Format::InvalidComponent("year"))?;
                bytes += format_four_digits_pad_zero(output, year)?;
            }
            bytes += write_if(output, Iso8601::<CONFIG>::USE_SEPARATORS, "-")?;
            // Safety: `month` is guaranteed to be in the range `1..=12`.
            bytes += format_two_digits(
                output,
                unsafe { ru8::new_unchecked(u8::from(value.month(state))) },
                Padding::Zero,
            )?;
            bytes += write_if(output, Iso8601::<CONFIG>::USE_SEPARATORS, "-")?;
            bytes += format_two_digits(output, value.day(state).expand(), Padding::Zero)?;
        }
        DateKind::Week => {
            let year = value.iso_year(state).get();

            if Iso8601::<CONFIG>::YEAR_IS_SIX_DIGITS {
                bytes += write_if_else(output, year < 0, "-", "+")?;
                // Safety: `iso_year` returns a value whose absolute value is guaranteed to be less
                // than 1,000,000.
                bytes += format_six_digits_pad_zero(output, unsafe {
                    ru32::new_unchecked(year.unsigned_abs())
                })?;
            } else {
                let year = ru16::new(year.cast_unsigned().truncate())
                    .ok_or(error::Format::InvalidComponent("year"))?;
                bytes += format_four_digits_pad_zero(output, year)?;
            }
            bytes += write_if_else(output, Iso8601::<CONFIG>::USE_SEPARATORS, "-W", "W")?;
            bytes +=
                format_two_digits(output, value.iso_week_number(state).expand(), Padding::Zero)?;
            bytes += write_if(output, Iso8601::<CONFIG>::USE_SEPARATORS, "-")?;
            // Safety: The value is in the range `1..=7`.
            bytes += format_single_digit(output, unsafe {
                ru8::new_unchecked(value.weekday(state).number_from_monday())
            })?;
        }
        DateKind::Ordinal => {
            let year = value.calendar_year(state).get();

            if Iso8601::<CONFIG>::YEAR_IS_SIX_DIGITS {
                bytes += write_if_else(output, year < 0, "-", "+")?;
                // Safety: `calendar_year` returns a value whose absolute value is guaranteed to be
                // less than 1,000,000.
                bytes += format_six_digits_pad_zero(output, unsafe {
                    ru32::new_unchecked(year.unsigned_abs())
                })?;
            } else {
                let year = ru16::new(year.cast_unsigned().truncate())
                    .ok_or(error::Format::InvalidComponent("year"))?;
                bytes += format_four_digits_pad_zero(output, year)?;
            }
            bytes += write_if(output, Iso8601::<CONFIG>::USE_SEPARATORS, "-")?;
            bytes += format_three_digits(output, value.ordinal(state).expand(), Padding::Zero)?;
        }
    }

    Ok(bytes)
}

/// Format the time portion of ISO 8601.
#[inline]
pub(super) fn format_time<V, const CONFIG: EncodedConfig>(
    output: &mut (impl io::Write + ?Sized),
    value: &V,
    state: &mut V::State,
) -> Result<usize, error::Format>
where
    V: ComponentProvider,
{
    let mut bytes = 0;

    // The "T" can only be omitted in extended format where there is no date being formatted.
    bytes += write_if(
        output,
        !Iso8601::<CONFIG>::USE_SEPARATORS || Iso8601::<CONFIG>::FORMAT_DATE,
        "T",
    )?;

    match Iso8601::<CONFIG>::TIME_PRECISION {
        TimePrecision::Hour { decimal_digits } => {
            let hours = (value.hour(state).get() as f64)
                + (value.minute(state).get() as f64) / Minute::per_t::<f64>(Hour)
                + (value.second(state).get() as f64) / Second::per_t::<f64>(Hour)
                + (value.nanosecond(state).get() as f64) / Nanosecond::per_t::<f64>(Hour);
            format_float(output, hours, 2, decimal_digits)?;
        }
        TimePrecision::Minute { decimal_digits } => {
            bytes += format_two_digits(output, value.hour(state).expand(), Padding::Zero)?;
            bytes += write_if(output, Iso8601::<CONFIG>::USE_SEPARATORS, ":")?;
            let minutes = (value.minute(state).get() as f64)
                + (value.second(state).get() as f64) / Second::per_t::<f64>(Minute)
                + (value.nanosecond(state).get() as f64) / Nanosecond::per_t::<f64>(Minute);
            bytes += format_float(output, minutes, 2, decimal_digits)?;
        }
        TimePrecision::Second { decimal_digits } => {
            bytes += format_two_digits(output, value.hour(state).expand(), Padding::Zero)?;
            bytes += write_if(output, Iso8601::<CONFIG>::USE_SEPARATORS, ":")?;
            bytes += format_two_digits(output, value.minute(state).expand(), Padding::Zero)?;
            bytes += write_if(output, Iso8601::<CONFIG>::USE_SEPARATORS, ":")?;
            bytes += format_two_digits(output, value.second(state).expand(), Padding::Zero)?;
            if let Some(digits) = decimal_digits {
                const POW_TABLE: [u64; 9] = [
                    1,
                    10,
                    100,
                    1_000,
                    10_000,
                    100_000,
                    1_000_000,
                    10_000_000,
                    100_000_000,
                ];

                bytes += write(output, ".")?;
                let nano = value.nanosecond(state).get() as u64;
                let sub_digits = min(digits.get(), 9);
                let truncated = nano / POW_TABLE[9 - sub_digits as usize];
                bytes += format_int_padded(output, truncated, sub_digits)?;
                for _ in 9..digits.get() {
                    bytes += write(output, "0")?;
                }
            }
        }
    }

    Ok(bytes)
}

/// Format the UTC offset portion of ISO 8601.
#[inline]
pub(super) fn format_offset<V, const CONFIG: EncodedConfig>(
    output: &mut (impl io::Write + ?Sized),
    value: &V,
    state: &mut V::State,
) -> Result<usize, error::Format>
where
    V: ComponentProvider,
{
    if Iso8601::<CONFIG>::FORMAT_TIME && value.offset_is_utc(state) {
        return Ok(write(output, "Z")?);
    }

    let mut bytes = 0;

    if value.offset_second(state).get() != 0 {
        return Err(error::Format::InvalidComponent("offset_second"));
    }
    bytes += write_if_else(output, value.offset_is_negative(state), "-", "+")?;
    // Safety: The value is in the range `-25..=25`.
    bytes += format_two_digits(
        output,
        unsafe { ru8::new_unchecked(value.offset_hour(state).get().unsigned_abs()) },
        Padding::Zero,
    )?;

    let minutes = value.offset_minute(state);

    if Iso8601::<CONFIG>::OFFSET_PRECISION == OffsetPrecision::Hour && minutes.get() != 0 {
        return Err(error::Format::InvalidComponent("offset_minute"));
    } else if Iso8601::<CONFIG>::OFFSET_PRECISION == OffsetPrecision::Minute {
        bytes += write_if(output, Iso8601::<CONFIG>::USE_SEPARATORS, ":")?;
        // Safety: The value is in the range `0..=59`.
        bytes += format_two_digits(
            output,
            unsafe { ru8::new_unchecked(minutes.get().unsigned_abs()) },
            Padding::Zero,
        )?;
    }

    Ok(bytes)
}
