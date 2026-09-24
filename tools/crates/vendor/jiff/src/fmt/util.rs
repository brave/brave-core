use crate::{
    error::{err, ErrorContext},
    fmt::Parsed,
    util::{c::Sign, escape, parse, t},
    Error, SignedDuration, Span, Unit,
};

/// A simple formatter for converting `i64` values to ASCII byte strings.
///
/// This avoids going through the formatting machinery which seems to
/// substantially slow things down.
///
/// The `itoa` crate does the same thing as this formatter, but is a bit
/// faster. We roll our own which is a bit slower, but gets us enough of a win
/// to be satisfied with and with (almost) pure safe code.
///
/// By default, this only includes the sign if it's negative. To always include
/// the sign, set `force_sign` to `true`.
#[derive(Clone, Copy, Debug)]
pub(crate) struct DecimalFormatter {
    force_sign: Option<bool>,
    minimum_digits: u8,
    padding_byte: u8,
}

impl DecimalFormatter {
    /// Creates a new decimal formatter using the default configuration.
    pub(crate) const fn new() -> DecimalFormatter {
        DecimalFormatter {
            force_sign: None,
            minimum_digits: 0,
            padding_byte: b'0',
        }
    }

    /// Format the given value using this configuration as a signed decimal
    /// ASCII number.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(crate) const fn format_signed(&self, value: i64) -> Decimal {
        Decimal::signed(self, value)
    }

    /// Format the given value using this configuration as an unsigned decimal
    /// ASCII number.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(crate) const fn format_unsigned(&self, value: u64) -> Decimal {
        Decimal::unsigned(self, value)
    }

    /// Forces the sign to be rendered, even if it's positive.
    ///
    /// When `zero_is_positive` is true, then a zero value is formatted with a
    /// positive sign. Otherwise, it is formatted with a negative sign.
    ///
    /// Regardless of this setting, a sign is never emitted when formatting an
    /// unsigned integer.
    #[cfg(test)]
    pub(crate) const fn force_sign(
        self,
        zero_is_positive: bool,
    ) -> DecimalFormatter {
        DecimalFormatter { force_sign: Some(zero_is_positive), ..self }
    }

    /// The minimum number of digits/padding that this number should be
    /// formatted with. If the number would have fewer digits than this, then
    /// it is padded out with the padding byte (which is zero by default) until
    /// the minimum is reached.
    ///
    /// The minimum number of digits is capped at the maximum number of digits
    /// for an i64 value (19) or a u64 value (20).
    pub(crate) const fn padding(self, mut digits: u8) -> DecimalFormatter {
        if digits > Decimal::MAX_I64_DIGITS {
            digits = Decimal::MAX_I64_DIGITS;
        }
        DecimalFormatter { minimum_digits: digits, ..self }
    }

    /// The padding byte to use when `padding` is set.
    ///
    /// The default is `0`.
    pub(crate) const fn padding_byte(self, byte: u8) -> DecimalFormatter {
        DecimalFormatter { padding_byte: byte, ..self }
    }

    /// Returns the minimum number of digits for a signed value.
    const fn get_signed_minimum_digits(&self) -> u8 {
        if self.minimum_digits <= Decimal::MAX_I64_DIGITS {
            self.minimum_digits
        } else {
            Decimal::MAX_I64_DIGITS
        }
    }

    /// Returns the minimum number of digits for an unsigned value.
    const fn get_unsigned_minimum_digits(&self) -> u8 {
        if self.minimum_digits <= Decimal::MAX_U64_DIGITS {
            self.minimum_digits
        } else {
            Decimal::MAX_U64_DIGITS
        }
    }
}

impl Default for DecimalFormatter {
    fn default() -> DecimalFormatter {
        DecimalFormatter::new()
    }
}

/// A formatted decimal number that can be converted to a sequence of bytes.
#[derive(Debug)]
pub(crate) struct Decimal {
    buf: [u8; Self::MAX_LEN as usize],
    start: u8,
    end: u8,
}

impl Decimal {
    /// Discovered via
    /// `i64::MIN.to_string().len().max(u64::MAX.to_string().len())`.
    const MAX_LEN: u8 = 20;
    /// Discovered via `i64::MAX.to_string().len()`.
    const MAX_I64_DIGITS: u8 = 19;
    /// Discovered via `u64::MAX.to_string().len()`.
    const MAX_U64_DIGITS: u8 = 20;

    /// Using the given formatter, turn the value given into an unsigned
    /// decimal representation using ASCII bytes.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    const fn unsigned(
        formatter: &DecimalFormatter,
        mut value: u64,
    ) -> Decimal {
        let mut decimal = Decimal {
            buf: [0; Self::MAX_LEN as usize],
            start: Self::MAX_LEN,
            end: Self::MAX_LEN,
        };
        loop {
            decimal.start -= 1;

            let digit = (value % 10) as u8;
            value /= 10;
            decimal.buf[decimal.start as usize] = b'0' + digit;
            if value == 0 {
                break;
            }
        }

        while decimal.len() < formatter.get_unsigned_minimum_digits() {
            decimal.start -= 1;
            decimal.buf[decimal.start as usize] = formatter.padding_byte;
        }
        decimal
    }

    /// Using the given formatter, turn the value given into a signed decimal
    /// representation using ASCII bytes.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    const fn signed(formatter: &DecimalFormatter, mut value: i64) -> Decimal {
        // Specialize the common case to generate tighter codegen.
        if value >= 0 && formatter.force_sign.is_none() {
            let mut decimal = Decimal {
                buf: [0; Self::MAX_LEN as usize],
                start: Self::MAX_LEN,
                end: Self::MAX_LEN,
            };
            loop {
                decimal.start -= 1;

                let digit = (value % 10) as u8;
                value /= 10;
                decimal.buf[decimal.start as usize] = b'0' + digit;
                if value == 0 {
                    break;
                }
            }

            while decimal.len() < formatter.get_signed_minimum_digits() {
                decimal.start -= 1;
                decimal.buf[decimal.start as usize] = formatter.padding_byte;
            }
            return decimal;
        }
        Decimal::signed_cold(formatter, value)
    }

    #[cold]
    #[inline(never)]
    const fn signed_cold(formatter: &DecimalFormatter, value: i64) -> Decimal {
        let sign = value.signum();
        let Some(mut value) = value.checked_abs() else {
            let buf = [
                b'-', b'9', b'2', b'2', b'3', b'3', b'7', b'2', b'0', b'3',
                b'6', b'8', b'5', b'4', b'7', b'7', b'5', b'8', b'0', b'8',
            ];
            return Decimal { buf, start: 0, end: Self::MAX_LEN };
        };
        let mut decimal = Decimal {
            buf: [0; Self::MAX_LEN as usize],
            start: Self::MAX_LEN,
            end: Self::MAX_LEN,
        };
        loop {
            decimal.start -= 1;

            let digit = (value % 10) as u8;
            value /= 10;
            decimal.buf[decimal.start as usize] = b'0' + digit;
            if value == 0 {
                break;
            }
        }
        while decimal.len() < formatter.get_signed_minimum_digits() {
            decimal.start -= 1;
            decimal.buf[decimal.start as usize] = formatter.padding_byte;
        }
        if sign < 0 {
            decimal.start -= 1;
            decimal.buf[decimal.start as usize] = b'-';
        } else if let Some(zero_is_positive) = formatter.force_sign {
            let ascii_sign =
                if sign > 0 || zero_is_positive { b'+' } else { b'-' };
            decimal.start -= 1;
            decimal.buf[decimal.start as usize] = ascii_sign;
        }
        decimal
    }

    /// Returns the total number of ASCII bytes (including the sign) that are
    /// used to represent this decimal number.
    #[inline]
    const fn len(&self) -> u8 {
        self.end - self.start
    }

    /// Returns the ASCII representation of this decimal as a byte slice.
    ///
    /// The slice returned is guaranteed to be valid ASCII.
    #[inline]
    fn as_bytes(&self) -> &[u8] {
        &self.buf[usize::from(self.start)..usize::from(self.end)]
    }

    /// Returns the ASCII representation of this decimal as a string slice.
    #[inline]
    pub(crate) fn as_str(&self) -> &str {
        // SAFETY: This is safe because all bytes written to `self.buf` are
        // guaranteed to be ASCII (including in its initial state), and thus,
        // any subsequence is guaranteed to be valid UTF-8.
        unsafe { core::str::from_utf8_unchecked(self.as_bytes()) }
    }
}

/// A simple formatter for converting fractional components to ASCII byte
/// strings.
///
/// We only support precision to 9 decimal places, which corresponds to
/// nanosecond precision as a fractional second component.
#[derive(Clone, Copy, Debug)]
pub(crate) struct FractionalFormatter {
    precision: Option<u8>,
}

impl FractionalFormatter {
    /// Creates a new fractional formatter using the given precision settings.
    pub(crate) const fn new() -> FractionalFormatter {
        FractionalFormatter { precision: None }
    }

    /// Format the given value using this configuration as a decimal ASCII
    /// fractional number.
    pub(crate) const fn format(&self, value: u32) -> Fractional {
        Fractional::new(self, value)
    }

    /// Set the precision.
    ///
    /// If the `precision` is greater than `9`, then it is clamped to `9`.
    ///
    /// When the precision is not set, then it is automatically determined
    /// based on the value.
    pub(crate) const fn precision(
        self,
        precision: Option<u8>,
    ) -> FractionalFormatter {
        let precision = match precision {
            None => None,
            Some(p) if p > 9 => Some(9),
            Some(p) => Some(p),
        };
        FractionalFormatter { precision, ..self }
    }

    /// Returns true if and only if at least one digit will be written for the
    /// given value.
    ///
    /// This is useful for callers that need to know whether to write
    /// a decimal separator, e.g., `.`, before the digits.
    pub(crate) fn will_write_digits(self, value: u32) -> bool {
        self.precision.map_or_else(|| value != 0, |p| p > 0)
    }

    /// Returns true if and only if this formatter has an explicit non-zero
    /// precision setting.
    ///
    /// This is useful for determining whether something like `0.000` needs to
    /// be written in the case of a `precision=Some(3)` setting and a zero
    /// value.
    pub(crate) fn has_non_zero_fixed_precision(self) -> bool {
        self.precision.map_or(false, |p| p > 0)
    }

    /// Returns true if and only if this formatter has fixed zero precision.
    /// That is, no matter what is given as input, a fraction is never written.
    pub(crate) fn has_zero_fixed_precision(self) -> bool {
        self.precision.map_or(false, |p| p == 0)
    }
}

/// A formatted fractional number that can be converted to a sequence of bytes.
#[derive(Debug)]
pub(crate) struct Fractional {
    buf: [u8; Self::MAX_LEN as usize],
    end: u8,
}

impl Fractional {
    /// Since we don't support precision bigger than this.
    const MAX_LEN: u8 = 9;

    /// Using the given formatter, turn the value given into a fractional
    /// decimal representation using ASCII bytes.
    ///
    /// Note that the fractional number returned *may* expand to an empty
    /// slice of bytes. This occurs whenever the precision is set to `0`, or
    /// when the precision is not set and the value is `0`. Any non-zero
    /// explicitly set precision guarantees that the slice returned is not
    /// empty.
    ///
    /// This panics if the value given isn't in the range `0..=999_999_999`.
    pub(crate) const fn new(
        formatter: &FractionalFormatter,
        mut value: u32,
    ) -> Fractional {
        assert!(value <= 999_999_999);
        let mut fractional = Fractional {
            buf: [b'0'; Self::MAX_LEN as usize],
            end: Self::MAX_LEN,
        };
        let mut i = 9;
        loop {
            i -= 1;

            let digit = (value % 10) as u8;
            value /= 10;
            fractional.buf[i] += digit;
            if value == 0 {
                break;
            }
        }
        if let Some(precision) = formatter.precision {
            fractional.end = precision;
        } else {
            while fractional.end > 0
                && fractional.buf[fractional.end as usize - 1] == b'0'
            {
                fractional.end -= 1;
            }
        }
        fractional
    }

    /// Returns the ASCII representation of this fractional number as a byte
    /// slice. The slice returned may be empty.
    ///
    /// The slice returned is guaranteed to be valid ASCII.
    pub(crate) fn as_bytes(&self) -> &[u8] {
        &self.buf[..usize::from(self.end)]
    }

    /// Returns the ASCII representation of this fractional number as a string
    /// slice. The slice returned may be empty.
    pub(crate) fn as_str(&self) -> &str {
        // SAFETY: This is safe because all bytes written to `self.buf` are
        // guaranteed to be ASCII (including in its initial state), and thus,
        // any subsequence is guaranteed to be valid UTF-8.
        unsafe { core::str::from_utf8_unchecked(self.as_bytes()) }
    }
}

/// A container for holding a partially parsed duration.
///
/// This is used for parsing into `Span`, `SignedDuration` and (hopefully
/// soon) `std::time::Duration`. It's _also_ used for both the ISO 8601
/// duration and "friendly" format.
///
/// This replaced a significant chunk of code that was bespoke to each
/// combination of duration type _and_ format.
///
/// The idea behind it is that we parse each duration component as an unsigned
/// 64-bit integer and keep track of the sign separately. This is a critical
/// aspect that was motivated by being able to roundtrip all legal values of
/// a 96-bit signed integer number of nanoseconds (i.e., `SignedDuration`).
/// In particular, if we used `i64` to represent each component, then it
/// makes it much more difficult to parse, e.g., `9223372036854775808
/// seconds ago`. Namely, `9223372036854775808` is not a valid `i64` but
/// `-9223372036854775808` is. Notably, the sign is indicated by a suffix,
/// so we don't know it's negative when parsing the integer itself. So we
/// represent all components as their unsigned absolute value and apply the
/// sign at the end.
///
/// This also centralizes a lot of thorny duration math and opens up the
/// opportunity for tighter optimization.
#[derive(Debug, Default)]
pub(crate) struct DurationUnits {
    /// The parsed unit values in descending order. That is, nanoseconds are
    /// at index 0 while years are at index 9.
    values: [u64; 10],
    /// Any fractional component parsed. The fraction is necessarily a fraction
    /// of the minimum unit if present.
    fraction: Option<u32>,
    /// The sign of the duration. This may be set at any time.
    ///
    /// Note that this defaults to zero! So callers will always want to set
    /// this.
    sign: Sign,
    /// The smallest unit value that was explicitly set.
    min: Option<Unit>,
    /// The largest unit value that was explicitly set.
    max: Option<Unit>,
    /// Whether there are any non-zero units.
    any_non_zero_units: bool,
}

impl DurationUnits {
    /// Set the duration component value for the given unit.
    ///
    /// The value here is always unsigned. To deal with negative values, set
    /// the sign independently. It will be accounted for when using one of this
    /// type's methods for converting to a concrete duration type.
    ///
    /// # Panics
    ///
    /// When this is called after `set_fraction`.
    ///
    /// # Errors
    ///
    /// Since this is meant to be used in service of duration parsing and all
    /// duration parsing proceeds from largest to smallest units, this will
    /// return an error if the given unit is bigger than or equal to any
    /// previously set unit. This also implies that this can only be called
    /// at most once for each unit value.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(crate) fn set_unit_value(
        &mut self,
        unit: Unit,
        value: u64,
    ) -> Result<(), Error> {
        assert!(self.fraction.is_none());

        if let Some(min) = self.min {
            if min <= unit {
                return Err(err!(
                    "found value {value:?} with unit {unit} \
                     after unit {prev_unit}, but units must be \
                     written from largest to smallest \
                     (and they can't be repeated)",
                    unit = unit.singular(),
                    prev_unit = min.singular(),
                ));
            }
        }
        // Given the above check, the given unit must be smaller than any we
        // have seen so far.
        self.min = Some(unit);
        // The maximum unit is always the first unit set, since we can never
        // see a unit bigger than it without an error occurring.
        if self.max.is_none() {
            self.max = Some(unit);
        }
        self.values[unit.as_usize()] = value;
        self.any_non_zero_units = self.any_non_zero_units || value != 0;
        Ok(())
    }

    /// A convenience routine for setting values parsed from an `HH:MM:SS`
    /// format (including the fraction).
    ///
    /// # Errors
    ///
    /// This forwards errors from `DurationUnits::set_unit_value`. It will also
    /// return an error is the minimum parsed unit (so far) is smaller than
    /// days. (Since `HH:MM:SS` can only appear after units of years, months,
    /// weeks or days.)
    pub(crate) fn set_hms(
        &mut self,
        hours: u64,
        minutes: u64,
        seconds: u64,
        fraction: Option<u32>,
    ) -> Result<(), Error> {
        if let Some(min) = self.min {
            if min <= Unit::Hour {
                return Err(err!(
                    "found `HH:MM:SS` after unit {min}, \
                             but `HH:MM:SS` can only appear after \
                             years, months, weeks or days",
                    min = min.singular(),
                ));
            }
        }
        self.set_unit_value(Unit::Hour, hours)?;
        self.set_unit_value(Unit::Minute, minutes)?;
        self.set_unit_value(Unit::Second, seconds)?;
        if let Some(fraction) = fraction {
            self.set_fraction(fraction)?;
        }
        Ok(())
    }

    /// Set the fractional value.
    ///
    /// This is always interpreted as a fraction of the minimal unit.
    ///
    /// Callers must ensure this is called after the last call to
    /// `DurationUnits::set_unit_value`.
    ///
    /// # Panics
    ///
    /// When `fraction` is not in the range `0..=999_999_999`. Callers are
    /// expected to uphold this invariant.
    ///
    /// # Errors
    ///
    /// This will return an error if the minimum unit is `Unit::Nanosecond`.
    /// (Because fractional nanoseconds are not supported.) This will also
    /// return an error if the minimum unit is bigger than `Unit::Hour`.
    pub(crate) fn set_fraction(&mut self, fraction: u32) -> Result<(), Error> {
        assert!(fraction <= 999_999_999);
        if self.min == Some(Unit::Nanosecond) {
            return Err(err!("fractional nanoseconds are not supported"));
        }
        if let Some(min) = self.min {
            if min > Unit::Hour {
                return Err(err!(
                    "fractional {plural} are not supported",
                    plural = min.plural()
                ));
            }
        }
        self.fraction = Some(fraction);
        Ok(())
    }

    /// Set the sign associated with the components.
    ///
    /// The sign applies to the entire duration. There is no support for
    /// having some components signed and some unsigned.
    ///
    /// If no sign is set, then it is assumed to be zero. Note also that
    /// even if a sign is explicitly set *and* all unit values are zero,
    /// then the sign will be set to zero.
    pub(crate) fn set_sign(&mut self, sign: Sign) {
        self.sign = sign;
    }

    /// Convert these duration components to a `Span`.
    ///
    /// # Errors
    ///
    /// If any individual unit exceeds the limits of a `Span`, or if the units
    /// combine to exceed what can be represented by a `Span`, then this
    /// returns an error.
    ///
    /// This also returns an error if no units were set.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(crate) fn to_span(&self) -> Result<Span, Error> {
        // When every unit value is less than this, *and* there is
        // no fractional component, then we trigger a fast path that
        // doesn't need to bother with error handling and careful
        // handling of the sign.
        //
        // Why do we use the maximum year value? Because years are
        // the "biggest" unit, it follows that there can't be any
        // other unit whose limit is smaller than years as a
        // dimenionless quantity. That is, if all parsed unit values
        // are no bigger than the maximum year, then we know all
        // parsed unit values are necessarily within their
        // appropriate limits.
        const LIMIT: u64 = t::SpanYears::MAX_SELF.get_unchecked() as u64;

        // If we have a fraction or a particularly large unit,
        // bail out to the general case.
        if self.fraction.is_some()
            || self.values.iter().any(|&value| value > LIMIT)
            // If no unit was set, it's an error case.
            || self.max.is_none()
        {
            return self.to_span_general();
        }

        let mut span = Span::new();

        let years = self.values[Unit::Year.as_usize()] as i16;
        let months = self.values[Unit::Month.as_usize()] as i32;
        let weeks = self.values[Unit::Week.as_usize()] as i32;
        let days = self.values[Unit::Day.as_usize()] as i32;
        let hours = self.values[Unit::Hour.as_usize()] as i32;
        let mins = self.values[Unit::Minute.as_usize()] as i64;
        let secs = self.values[Unit::Second.as_usize()] as i64;
        let millis = self.values[Unit::Millisecond.as_usize()] as i64;
        let micros = self.values[Unit::Microsecond.as_usize()] as i64;
        let nanos = self.values[Unit::Nanosecond.as_usize()] as i64;

        span = span.years_unchecked(years);
        span = span.months_unchecked(months);
        span = span.weeks_unchecked(weeks);
        span = span.days_unchecked(days);
        span = span.hours_unchecked(hours);
        span = span.minutes_unchecked(mins);
        span = span.seconds_unchecked(secs);
        span = span.milliseconds_unchecked(millis);
        span = span.microseconds_unchecked(micros);
        span = span.nanoseconds_unchecked(nanos);

        // The unchecked setters above don't manipulate
        // the sign, which defaults to zero. So we need to
        // set it even when it's positive.
        span = span.sign_unchecked(self.get_sign().as_ranged_integer());

        Ok(span)
    }

    /// The "general" implementation of `DurationUnits::to_span`.
    ///
    /// This handles all possible cases, including fractional units, with good
    /// error handling. Basically, we take this path when we think an error
    /// _could_ occur. But this function is more bloaty and does more work, so
    /// the more it can be avoided, the better.
    #[cold]
    #[inline(never)]
    fn to_span_general(&self) -> Result<Span, Error> {
        fn error_context(unit: Unit, value: i64) -> Error {
            err!(
                "failed to set value {value:?} as {unit} unit on span",
                unit = unit.singular(),
            )
        }

        #[cfg_attr(feature = "perf-inline", inline(always))]
        fn set_time_unit(
            unit: Unit,
            value: i64,
            span: Span,
            set: impl FnOnce(Span) -> Result<Span, Error>,
        ) -> Result<Span, Error> {
            #[cold]
            #[inline(never)]
            fn fractional_fallback(
                err: Error,
                unit: Unit,
                value: i64,
                span: Span,
            ) -> Result<Span, Error> {
                // Fractional calendar units aren't supported. Neither are
                // fractional nanoseconds. So there's nothing we can do in
                // this case.
                if unit > Unit::Hour || unit == Unit::Nanosecond {
                    Err(err)
                } else {
                    // This is annoying, but because we can write out a larger
                    // number of hours/minutes/seconds than what we actually
                    // support, we need to be prepared to parse an unbalanced
                    // span if our time units are too big here. In essence,
                    // this lets a single time unit "overflow" into smaller
                    // units if it exceeds the limits.
                    fractional_time_to_span(unit, value, 0, span)
                }
            }

            set(span)
                .or_else(|err| fractional_fallback(err, unit, value, span))
                .with_context(|| error_context(unit, value))
        }

        let (min, _) = self.get_min_max_units()?;
        let mut span = Span::new();

        if self.values[Unit::Year.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Year)?;
            span = span
                .try_years(value)
                .with_context(|| error_context(Unit::Year, value))?;
        }
        if self.values[Unit::Month.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Month)?;
            span = span
                .try_months(value)
                .with_context(|| error_context(Unit::Month, value))?;
        }
        if self.values[Unit::Week.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Week)?;
            span = span
                .try_weeks(value)
                .with_context(|| error_context(Unit::Week, value))?;
        }
        if self.values[Unit::Day.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Day)?;
            span = span
                .try_days(value)
                .with_context(|| error_context(Unit::Day, value))?;
        }
        if self.values[Unit::Hour.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Hour)?;
            span = set_time_unit(Unit::Hour, value, span, |span| {
                span.try_hours(value)
            })?;
        }
        if self.values[Unit::Minute.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Minute)?;
            span = set_time_unit(Unit::Minute, value, span, |span| {
                span.try_minutes(value)
            })?;
        }
        if self.values[Unit::Second.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Second)?;
            span = set_time_unit(Unit::Second, value, span, |span| {
                span.try_seconds(value)
            })?;
        }
        if self.values[Unit::Millisecond.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Millisecond)?;
            span = set_time_unit(Unit::Millisecond, value, span, |span| {
                span.try_milliseconds(value)
            })?;
        }
        if self.values[Unit::Microsecond.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Microsecond)?;
            span = set_time_unit(Unit::Microsecond, value, span, |span| {
                span.try_microseconds(value)
            })?;
        }
        if self.values[Unit::Nanosecond.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Nanosecond)?;
            span = set_time_unit(Unit::Nanosecond, value, span, |span| {
                span.try_nanoseconds(value)
            })?;
        }

        if let Some(fraction) = self.get_fraction()? {
            let value = self.get_unit_value(min)?;
            span = fractional_time_to_span(min, value, fraction, span)?;
        }

        Ok(span)
    }

    /// Convert these duration components to a `SignedDuration`.
    ///
    /// # Errors
    ///
    /// If the total number of nanoseconds represented by all units combined
    /// exceeds what can bit in a 96-bit signed integer, then an error is
    /// returned.
    ///
    /// An error is also returned if any calendar units (days or greater) were
    /// set or if no units were set.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(crate) fn to_signed_duration(&self) -> Result<SignedDuration, Error> {
        // When every unit value is less than this, *and* there is
        // no fractional component, then we trigger a fast path that
        // doesn't need to bother with error handling and careful
        // handling of the sign.
        //
        // Why `999`? Well, I think it's nice to use one limit for all
        // units to make the comparisons simpler (although we could
        // use more targeted values to admit more cases, I didn't try
        // that). But specifically, this means we can have `999ms 999us
        // 999ns` as a maximal subsecond value without overflowing
        // the nanosecond component of a `SignedDuration`. This lets
        // us "just do math" without needing to check each result and
        // handle errors.
        const LIMIT: u64 = 999;

        if self.fraction.is_some()
            || self.values[..Unit::Day.as_usize()]
                .iter()
                .any(|&value| value > LIMIT)
            || self.max.map_or(true, |max| max > Unit::Hour)
        {
            return self.to_signed_duration_general();
        }

        let hours = self.values[Unit::Hour.as_usize()] as i64;
        let mins = self.values[Unit::Minute.as_usize()] as i64;
        let secs = self.values[Unit::Second.as_usize()] as i64;
        let millis = self.values[Unit::Millisecond.as_usize()] as i32;
        let micros = self.values[Unit::Microsecond.as_usize()] as i32;
        let nanos = self.values[Unit::Nanosecond.as_usize()] as i32;

        let total_secs = (hours * 3600) + (mins * 60) + secs;
        let total_nanos = (millis * 1_000_000) + (micros * 1_000) + nanos;
        let mut sdur =
            SignedDuration::new_without_nano_overflow(total_secs, total_nanos);
        if self.get_sign().is_negative() {
            sdur = -sdur;
        }

        Ok(sdur)
    }

    /// The "general" implementation of `DurationUnits::to_signed_duration`.
    ///
    /// This handles all possible cases, including fractional units, with good
    /// error handling. Basically, we take this path when we think an error
    /// _could_ occur. But this function is more bloaty and does more work, so
    /// the more it can be avoided, the better.
    #[cold]
    #[inline(never)]
    fn to_signed_duration_general(&self) -> Result<SignedDuration, Error> {
        let (min, max) = self.get_min_max_units()?;
        if max > Unit::Hour {
            return Err(err!(
                "parsing {unit} units into a `SignedDuration` is not supported \
                 (perhaps try parsing into a `Span` instead)",
                unit = max.singular(),
            ));
        }

        let mut sdur = SignedDuration::ZERO;
        if self.values[Unit::Hour.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Hour)?;
            sdur = SignedDuration::try_from_hours(value)
                .and_then(|nanos| sdur.checked_add(nanos))
                .ok_or_else(|| {
                    err!(
                        "accumulated `SignedDuration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Hour.singular(),
                    )
                })?;
        }
        if self.values[Unit::Minute.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Minute)?;
            sdur = SignedDuration::try_from_mins(value)
                .and_then(|nanos| sdur.checked_add(nanos))
                .ok_or_else(|| {
                    err!(
                        "accumulated `SignedDuration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Minute.singular(),
                    )
                })?;
        }
        if self.values[Unit::Second.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Second)?;
            sdur = SignedDuration::from_secs(value)
                .checked_add(sdur)
                .ok_or_else(|| {
                    err!(
                        "accumulated `SignedDuration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Second.singular(),
                    )
                })?;
        }
        if self.values[Unit::Millisecond.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Millisecond)?;
            sdur = SignedDuration::from_millis(value)
                .checked_add(sdur)
                .ok_or_else(|| {
                    err!(
                        "accumulated `SignedDuration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Millisecond.singular(),
                    )
                })?;
        }
        if self.values[Unit::Microsecond.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Microsecond)?;
            sdur = SignedDuration::from_micros(value)
                .checked_add(sdur)
                .ok_or_else(|| {
                    err!(
                        "accumulated `SignedDuration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Microsecond.singular(),
                    )
                })?;
        }
        if self.values[Unit::Nanosecond.as_usize()] != 0 {
            let value = self.get_unit_value(Unit::Nanosecond)?;
            sdur = SignedDuration::from_nanos(value)
                .checked_add(sdur)
                .ok_or_else(|| {
                    err!(
                        "accumulated `SignedDuration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Nanosecond.singular(),
                    )
                })?;
        }

        if let Some(fraction) = self.get_fraction()? {
            sdur = sdur
                .checked_add(fractional_duration(min, fraction)?)
                .ok_or_else(|| {
                    err!(
                        "accumulated `SignedDuration` of `{sdur:?}` \
                         overflowed when adding 0.{fraction} of unit {unit}",
                        unit = min.singular(),
                    )
                })?;
        }

        Ok(sdur)
    }

    /// Convert these duration components to a `core::time::Duration`.
    ///
    /// # Errors
    ///
    /// If the total number of nanoseconds represented by all units combined
    /// exceeds what can bit in a 96-bit signed integer, then an error is
    /// returned.
    ///
    /// An error is also returned if any calendar units (days or greater) were
    /// set or if no units were set.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    pub(crate) fn to_unsigned_duration(
        &self,
    ) -> Result<core::time::Duration, Error> {
        // When every unit value is less than this, *and* there is
        // no fractional component, then we trigger a fast path that
        // doesn't need to bother with error handling and careful
        // handling of the sign.
        //
        // Why `999`? Well, I think it's nice to use one limit for all
        // units to make the comparisons simpler (although we could
        // use more targeted values to admit more cases, I didn't try
        // that). But specifically, this means we can have `999ms 999us
        // 999ns` as a maximal subsecond value without overflowing
        // the nanosecond component of a `core::time::Duration`. This lets
        // us "just do math" without needing to check each result and
        // handle errors.
        const LIMIT: u64 = 999;

        if self.fraction.is_some()
            || self.values[..Unit::Day.as_usize()]
                .iter()
                .any(|&value| value > LIMIT)
            || self.max.map_or(true, |max| max > Unit::Hour)
            || self.sign.is_negative()
        {
            return self.to_unsigned_duration_general();
        }

        let hours = self.values[Unit::Hour.as_usize()];
        let mins = self.values[Unit::Minute.as_usize()];
        let secs = self.values[Unit::Second.as_usize()];
        let millis = self.values[Unit::Millisecond.as_usize()] as u32;
        let micros = self.values[Unit::Microsecond.as_usize()] as u32;
        let nanos = self.values[Unit::Nanosecond.as_usize()] as u32;

        let total_secs = (hours * 3600) + (mins * 60) + secs;
        let total_nanos = (millis * 1_000_000) + (micros * 1_000) + nanos;
        let sdur = core::time::Duration::new(total_secs, total_nanos);

        Ok(sdur)
    }

    /// The "general" implementation of `DurationUnits::to_unsigned_duration`.
    ///
    /// This handles all possible cases, including fractional units, with good
    /// error handling. Basically, we take this path when we think an error
    /// _could_ occur. But this function is more bloaty and does more work, so
    /// the more it can be avoided, the better.
    #[cold]
    #[inline(never)]
    fn to_unsigned_duration_general(
        &self,
    ) -> Result<core::time::Duration, Error> {
        #[inline]
        const fn try_from_hours(hours: u64) -> Option<core::time::Duration> {
            // OK because (SECS_PER_MINUTE*MINS_PER_HOUR)!={-1,0}.
            const MAX_HOUR: u64 = u64::MAX / (60 * 60);
            if hours > MAX_HOUR {
                return None;
            }
            Some(core::time::Duration::from_secs(hours * 60 * 60))
        }

        #[inline]
        const fn try_from_mins(mins: u64) -> Option<core::time::Duration> {
            // OK because SECS_PER_MINUTE!={-1,0}.
            const MAX_MINUTE: u64 = u64::MAX / 60;
            if mins > MAX_MINUTE {
                return None;
            }
            Some(core::time::Duration::from_secs(mins * 60))
        }

        if self.sign.is_negative() {
            return Err(err!(
                "cannot parse negative duration into unsigned \
                 `std::time::Duration`",
            ));
        }

        let (min, max) = self.get_min_max_units()?;
        if max > Unit::Hour {
            return Err(err!(
                "parsing {unit} units into a `std::time::Duration` \
                 is not supported (perhaps try parsing into a `Span` instead)",
                unit = max.singular(),
            ));
        }

        let mut sdur = core::time::Duration::ZERO;
        if self.values[Unit::Hour.as_usize()] != 0 {
            let value = self.values[Unit::Hour.as_usize()];
            sdur = try_from_hours(value)
                .and_then(|nanos| sdur.checked_add(nanos))
                .ok_or_else(|| {
                    err!(
                        "accumulated `std::time::Duration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Hour.singular(),
                    )
                })?;
        }
        if self.values[Unit::Minute.as_usize()] != 0 {
            let value = self.values[Unit::Minute.as_usize()];
            sdur = try_from_mins(value)
                .and_then(|nanos| sdur.checked_add(nanos))
                .ok_or_else(|| {
                    err!(
                        "accumulated `std::time::Duration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Minute.singular(),
                    )
                })?;
        }
        if self.values[Unit::Second.as_usize()] != 0 {
            let value = self.values[Unit::Second.as_usize()];
            sdur = core::time::Duration::from_secs(value)
                .checked_add(sdur)
                .ok_or_else(|| {
                    err!(
                        "accumulated `std::time::Duration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Second.singular(),
                    )
                })?;
        }
        if self.values[Unit::Millisecond.as_usize()] != 0 {
            let value = self.values[Unit::Millisecond.as_usize()];
            sdur = core::time::Duration::from_millis(value)
                .checked_add(sdur)
                .ok_or_else(|| {
                    err!(
                        "accumulated `std::time::Duration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Millisecond.singular(),
                    )
                })?;
        }
        if self.values[Unit::Microsecond.as_usize()] != 0 {
            let value = self.values[Unit::Microsecond.as_usize()];
            sdur = core::time::Duration::from_micros(value)
                .checked_add(sdur)
                .ok_or_else(|| {
                    err!(
                        "accumulated `std::time::Duration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                        unit = Unit::Microsecond.singular(),
                    )
                })?;
        }
        if self.values[Unit::Nanosecond.as_usize()] != 0 {
            let value = self.values[Unit::Nanosecond.as_usize()];
            sdur = core::time::Duration::from_nanos(value)
                .checked_add(sdur)
                .ok_or_else(|| {
                err!(
                    "accumulated `std::time::Duration` of `{sdur:?}` \
                         overflowed when adding {value} of unit {unit}",
                    unit = Unit::Nanosecond.singular(),
                )
            })?;
        }

        if let Some(fraction) = self.get_fraction()? {
            sdur = sdur
                .checked_add(
                    fractional_duration(min, fraction)?.unsigned_abs(),
                )
                .ok_or_else(|| {
                    err!(
                        "accumulated `std::time::Duration` of `{sdur:?}` \
                         overflowed when adding 0.{fraction} of unit {unit}",
                        unit = min.singular(),
                    )
                })?;
        }

        Ok(sdur)
    }

    /// Returns the minimum unit set.
    ///
    /// This only returns `None` when no units have been set.
    pub(crate) fn get_min(&self) -> Option<Unit> {
        self.min
    }

    /// Returns the minimum and maximum units set.
    ///
    /// This returns an error if no units were set. (Since this means there
    /// were no parsed duration components.)
    fn get_min_max_units(&self) -> Result<(Unit, Unit), Error> {
        let (Some(min), Some(max)) = (self.min, self.max) else {
            return Err(err!("no parsed duration components"));
        };
        Ok((min, max))
    }

    /// Returns the corresponding unit value using the set signed-ness.
    #[cfg_attr(feature = "perf-inline", inline(always))]
    fn get_unit_value(&self, unit: Unit) -> Result<i64, Error> {
        const I64_MIN_ABS: u64 = i64::MIN.unsigned_abs();

        #[cold]
        #[inline(never)]
        fn general(unit: Unit, value: u64, sign: Sign) -> Result<i64, Error> {
            // As a weird special case, when we need to represent i64::MIN,
            // we'll have a unit value of `|i64::MIN|` as a `u64`. We can't
            // convert that to a positive `i64` first, since it will overflow.
            if sign.is_negative() && value == I64_MIN_ABS {
                return Ok(i64::MIN);
            }
            // Otherwise, if a conversion to `i64` fails, then that failure
            // is correct.
            let mut value = i64::try_from(value).map_err(|_| {
                err!(
                    "`{sign}{value}` {unit} is too big (or small) \
                     to fit into a signed 64-bit integer",
                    unit = unit.plural()
                )
            })?;
            if sign.is_negative() {
                value = value.checked_neg().ok_or_else(|| {
                    err!(
                        "`{sign}{value}` {unit} is too big (or small) \
                         to fit into a signed 64-bit integer",
                        unit = unit.plural()
                    )
                })?;
            }
            Ok(value)
        }

        let sign = self.get_sign();
        let value = self.values[unit.as_usize()];
        if value >= I64_MIN_ABS {
            return general(unit, value, sign);
        }
        let mut value = value as i64;
        if sign.is_negative() {
            value = -value;
        }
        Ok(value)
    }

    /// Returns the fraction using the set signed-ness.
    ///
    /// This returns `None` when no fraction has been set.
    fn get_fraction(&self) -> Result<Option<i32>, Error> {
        let Some(fraction) = self.fraction else {
            return Ok(None);
        };
        // OK because `set_fraction` guarantees `0..=999_999_999`.
        let mut fraction = fraction as i32;
        if self.get_sign().is_negative() {
            // OK because `set_fraction` guarantees `0..=999_999_999`.
            fraction = -fraction;
        }
        Ok(Some(fraction))
    }

    /// Returns the sign that should be applied to each individual unit.
    fn get_sign(&self) -> Sign {
        if self.any_non_zero_units {
            self.sign
        } else {
            Sign::Zero
        }
    }
}

/// Parses an optional fractional number from the start of `input`.
///
/// If `input` does not begin with a `.` (or a `,`), then this returns `None`
/// and no input is consumed. Otherwise, up to 9 ASCII digits are parsed after
/// the decimal separator.
///
/// While this is most typically used to parse the fractional component of
/// second units, it is also used to parse the fractional component of hours or
/// minutes in ISO 8601 duration parsing, and milliseconds and microseconds in
/// the "friendly" duration format. The return type in that case is obviously a
/// misnomer, but the range of possible values is still correct. (That is, the
/// fractional component of an hour is still limited to 9 decimal places per
/// the Temporal spec.)
///
/// The number returned is guaranteed to be in the range `0..=999_999_999`.
#[cfg_attr(feature = "perf-inline", inline(always))]
pub(crate) fn parse_temporal_fraction<'i>(
    input: &'i [u8],
) -> Result<Parsed<'i, Option<u32>>, Error> {
    // TimeFraction :::
    //   TemporalDecimalFraction
    //
    // TemporalDecimalFraction :::
    //   TemporalDecimalSeparator DecimalDigit
    //   TemporalDecimalSeparator DecimalDigit DecimalDigit
    //   TemporalDecimalSeparator DecimalDigit DecimalDigit DecimalDigit
    //   TemporalDecimalSeparator DecimalDigit DecimalDigit DecimalDigit
    //                            DecimalDigit
    //   TemporalDecimalSeparator DecimalDigit DecimalDigit DecimalDigit
    //                            DecimalDigit DecimalDigit
    //   TemporalDecimalSeparator DecimalDigit DecimalDigit DecimalDigit
    //                            DecimalDigit DecimalDigit DecimalDigit
    //   TemporalDecimalSeparator DecimalDigit DecimalDigit DecimalDigit
    //                            DecimalDigit DecimalDigit DecimalDigit
    //                            DecimalDigit
    //   TemporalDecimalSeparator DecimalDigit DecimalDigit DecimalDigit
    //                            DecimalDigit DecimalDigit DecimalDigit
    //                            DecimalDigit DecimalDigit
    //   TemporalDecimalSeparator DecimalDigit DecimalDigit DecimalDigit
    //                            DecimalDigit DecimalDigit DecimalDigit
    //                            DecimalDigit DecimalDigit DecimalDigit
    //
    // TemporalDecimalSeparator ::: one of
    //   . ,
    //
    // DecimalDigit :: one of
    //   0 1 2 3 4 5 6 7 8 9

    #[inline(never)]
    fn imp<'i>(mut input: &'i [u8]) -> Result<Parsed<'i, Option<u32>>, Error> {
        let mkdigits = parse::slicer(input);
        while mkdigits(input).len() <= 8
            && input.first().map_or(false, u8::is_ascii_digit)
        {
            input = &input[1..];
        }
        let digits = mkdigits(input);
        if digits.is_empty() {
            return Err(err!(
                "found decimal after seconds component, \
                 but did not find any decimal digits after decimal",
            ));
        }
        // I believe this error can never happen, since we know we have no more
        // than 9 ASCII digits. Any sequence of 9 ASCII digits can be parsed
        // into an `i64`.
        let nanoseconds = parse::fraction(digits).map_err(|err| {
            err!(
                "failed to parse {digits:?} as fractional component \
                 (up to 9 digits, nanosecond precision): {err}",
                digits = escape::Bytes(digits),
            )
        })?;
        // OK because parsing is forcefully limited to 9 digits,
        // which can never be greater than `999_999_99`,
        // which is less than `u32::MAX`.
        let nanoseconds = nanoseconds as u32;
        Ok(Parsed { value: Some(nanoseconds), input })
    }

    if input.is_empty() || (input[0] != b'.' && input[0] != b',') {
        return Ok(Parsed { value: None, input });
    }
    imp(&input[1..])
}

/// This routine returns a span based on the given unit and value with
/// fractional time applied to it.
///
/// For example, given a span like `P1dT1.5h`, the `unit` would be
/// `Unit::Hour`, the `value` would be `1` and the `fraction` would be
/// `500_000_000`. The span given would just be `1d`. The span returned would
/// be `P1dT1h30m`.
///
/// Note that `fraction` can be a fractional hour, minute, second, millisecond
/// or microsecond (even though its type suggests its only a fraction of a
/// second). When milliseconds or microseconds, the given fraction has any
/// sub-nanosecond precision truncated.
///
/// # Errors
///
/// This can error if the resulting units would be too large for the limits on
/// a `span`. This also errors if `unit` is not `Hour`, `Minute`, `Second`,
/// `Millisecond` or `Microsecond`.
#[inline(never)]
fn fractional_time_to_span(
    unit: Unit,
    value: i64,
    fraction: i32,
    mut span: Span,
) -> Result<Span, Error> {
    const MAX_HOURS: i64 = t::SpanHours::MAX_SELF.get_unchecked() as i64;
    const MAX_MINS: i64 = t::SpanMinutes::MAX_SELF.get_unchecked() as i64;
    const MAX_SECS: i64 = t::SpanSeconds::MAX_SELF.get_unchecked() as i64;
    const MAX_MILLIS: i128 =
        t::SpanMilliseconds::MAX_SELF.get_unchecked() as i128;
    const MAX_MICROS: i128 =
        t::SpanMicroseconds::MAX_SELF.get_unchecked() as i128;
    const MIN_HOURS: i64 = t::SpanHours::MIN_SELF.get_unchecked() as i64;
    const MIN_MINS: i64 = t::SpanMinutes::MIN_SELF.get_unchecked() as i64;
    const MIN_SECS: i64 = t::SpanSeconds::MIN_SELF.get_unchecked() as i64;
    const MIN_MILLIS: i128 =
        t::SpanMilliseconds::MIN_SELF.get_unchecked() as i128;
    const MIN_MICROS: i128 =
        t::SpanMicroseconds::MIN_SELF.get_unchecked() as i128;

    // We switch everything over to nanoseconds and then divy that up as
    // appropriate. In general, we always create a balanced span, but there
    // are some cases where we can't. For example, if one serializes a span
    // with both the maximum number of seconds and the maximum number of
    // milliseconds, then this just can't be balanced due to the limits on
    // each of the units. When this kind of span is serialized to a string,
    // it results in a second value that is actually bigger than the maximum
    // allowed number of seconds in a span. So here, we have to reverse that
    // operation and spread the seconds over smaller units. This in turn
    // creates an unbalanced span. Annoying.
    //
    // The above is why we have `if unit_value > MAX { <do adjustments> }` in
    // the balancing code below. Basically, if we overshoot our limit, we back
    // out anything over the limit and carry it over to the lesser units. If
    // our value is truly too big, then the final call to set nanoseconds will
    // fail.
    let mut sdur = fractional_time_to_duration(unit, value, fraction)?;

    if unit >= Unit::Hour && !sdur.is_zero() {
        let (mut hours, rem) = sdur.as_hours_with_remainder();
        sdur = rem;
        if hours > MAX_HOURS {
            sdur += SignedDuration::from_hours(hours - MAX_HOURS);
            hours = MAX_HOURS;
        } else if hours < MIN_HOURS {
            sdur += SignedDuration::from_hours(hours - MIN_HOURS);
            hours = MIN_HOURS;
        }
        // OK because we just checked that our units are in range.
        span = span.hours(hours);
    }
    if unit >= Unit::Minute && !sdur.is_zero() {
        let (mut mins, rem) = sdur.as_mins_with_remainder();
        sdur = rem;
        if mins > MAX_MINS {
            sdur += SignedDuration::from_mins(mins - MAX_MINS);
            mins = MAX_MINS;
        } else if mins < MIN_MINS {
            sdur += SignedDuration::from_mins(mins - MIN_MINS);
            mins = MIN_MINS;
        }
        // OK because we just checked that our units are in range.
        span = span.minutes(mins);
    }
    if unit >= Unit::Second && !sdur.is_zero() {
        let (mut secs, rem) = sdur.as_secs_with_remainder();
        sdur = rem;
        if secs > MAX_SECS {
            sdur += SignedDuration::from_secs(secs - MAX_SECS);
            secs = MAX_SECS;
        } else if secs < MIN_SECS {
            sdur += SignedDuration::from_secs(secs - MIN_SECS);
            secs = MIN_SECS;
        }
        // OK because we just checked that our units are in range.
        span = span.seconds(secs);
    }
    if unit >= Unit::Millisecond && !sdur.is_zero() {
        let (mut millis, rem) = sdur.as_millis_with_remainder();
        sdur = rem;
        if millis > MAX_MILLIS {
            sdur += SignedDuration::from_millis_i128(millis - MAX_MILLIS);
            millis = MAX_MILLIS;
        } else if millis < MIN_MILLIS {
            sdur += SignedDuration::from_millis_i128(millis - MIN_MILLIS);
            millis = MIN_MILLIS;
        }
        // OK because we just checked that our units are in range.
        span = span.milliseconds(i64::try_from(millis).unwrap());
    }
    if unit >= Unit::Microsecond && !sdur.is_zero() {
        let (mut micros, rem) = sdur.as_micros_with_remainder();
        sdur = rem;
        if micros > MAX_MICROS {
            sdur += SignedDuration::from_micros_i128(micros - MAX_MICROS);
            micros = MAX_MICROS;
        } else if micros < MIN_MICROS {
            sdur += SignedDuration::from_micros_i128(micros - MIN_MICROS);
            micros = MIN_MICROS;
        }
        // OK because we just checked that our units are in range.
        span = span.microseconds(i64::try_from(micros).unwrap());
    }
    if !sdur.is_zero() {
        let nanos = sdur.as_nanos();
        let nanos64 = i64::try_from(nanos).map_err(|_| {
            err!(
                "failed to set nanosecond value {nanos} (it overflows \
                 `i64`) on span determined from {value}.{fraction}",
            )
        })?;
        span = span.try_nanoseconds(nanos64).with_context(|| {
            err!(
                "failed to set nanosecond value {nanos64} on span \
                 determined from {value}.{fraction}",
            )
        })?;
    }

    Ok(span)
}

/// Like `fractional_time_to_span`, but just converts the fraction of the given
/// unit to a signed duration.
///
/// Since a signed duration doesn't keep track of individual units, there is
/// no loss of fidelity between it and ISO 8601 durations like there is for
/// `Span`.
///
/// Note that `fraction` can be a fractional hour, minute, second, millisecond
/// or microsecond (even though its type suggests it's only a fraction of a
/// second). When milliseconds or microseconds, the given fraction has any
/// sub-nanosecond precision truncated.
///
/// # Errors
///
/// This returns an error if `unit` is not `Hour`, `Minute`, `Second`,
/// `Millisecond` or `Microsecond`.
#[inline(never)]
fn fractional_time_to_duration(
    unit: Unit,
    value: i64,
    fraction: i32,
) -> Result<SignedDuration, Error> {
    let sdur = duration_unit_value(unit, value)?;
    let fraction_dur = fractional_duration(unit, fraction)?;
    sdur.checked_add(fraction_dur).ok_or_else(|| {
        err!(
            "accumulated `SignedDuration` of `{sdur:?}` overflowed \
             when adding `{fraction_dur:?}` (from fractional {unit} units)",
            unit = unit.singular(),
        )
    })
}

/// Converts the fraction of the given unit to a signed duration.
///
/// Since a signed duration doesn't keep track of individual units, there is
/// no loss of fidelity between it and ISO 8601 durations like there is for
/// `Span`. Thus, we can do something far less complicated.
///
/// # Panics
///
/// When `fraction` isn't in the range `-999_999_999..=999_999_999`.
///
/// # Errors
///
/// This returns an error if `unit` is not `Hour`, `Minute`, `Second`,
/// `Millisecond` or `Microsecond`.
#[inline(never)]
fn fractional_duration(
    unit: Unit,
    fraction: i32,
) -> Result<SignedDuration, Error> {
    let fraction = i64::from(fraction);
    let nanos = match unit {
        Unit::Hour => fraction * t::SECONDS_PER_HOUR.value(),
        Unit::Minute => fraction * t::SECONDS_PER_MINUTE.value(),
        Unit::Second => fraction,
        Unit::Millisecond => fraction / t::NANOS_PER_MICRO.value(),
        Unit::Microsecond => fraction / t::NANOS_PER_MILLI.value(),
        unit => {
            return Err(err!(
                "fractional {unit} units are not allowed",
                unit = unit.singular(),
            ))
        }
    };
    Ok(SignedDuration::from_nanos(nanos))
}

/// Returns the given parsed value, interpreted as the given unit, as a
/// `SignedDuration`.
///
/// If the given unit is not supported for signed durations (i.e., calendar
/// units), or if converting the given value to a `SignedDuration` for the
/// given units overflows, then an error is returned.
#[cfg_attr(feature = "perf-inline", inline(always))]
fn duration_unit_value(
    unit: Unit,
    value: i64,
) -> Result<SignedDuration, Error> {
    // Convert our parsed unit into a number of nanoseconds.
    //
    // Note also that overflow isn't possible here for units less than minutes,
    // since a `SignedDuration` supports all `i64` second values.
    let sdur = match unit {
        Unit::Hour => {
            let seconds = value
                .checked_mul(t::SECONDS_PER_HOUR.value())
                .ok_or_else(|| {
                    err!("converting {value} hours to seconds overflows i64")
                })?;
            SignedDuration::from_secs(seconds)
        }
        Unit::Minute => {
            let seconds = value
                .checked_mul(t::SECONDS_PER_MINUTE.value())
                .ok_or_else(|| {
                    err!("converting {value} minutes to seconds overflows i64")
                })?;
            SignedDuration::from_secs(seconds)
        }
        Unit::Second => SignedDuration::from_secs(value),
        Unit::Millisecond => SignedDuration::from_millis(value),
        Unit::Microsecond => SignedDuration::from_micros(value),
        Unit::Nanosecond => SignedDuration::from_nanos(value),
        unsupported => {
            return Err(err!(
                "parsing {unit} units into a `SignedDuration` is not supported \
                 (perhaps try parsing into a `Span` instead)",
                unit = unsupported.singular(),
            ));
        }
    };
    Ok(sdur)
}

#[cfg(test)]
mod tests {
    use alloc::string::ToString;

    use super::*;

    #[test]
    fn decimal() {
        let x = DecimalFormatter::new().format_signed(i64::MIN);
        assert_eq!(x.as_str(), "-9223372036854775808");

        let x = DecimalFormatter::new().format_signed(i64::MIN + 1);
        assert_eq!(x.as_str(), "-9223372036854775807");

        let x = DecimalFormatter::new().format_signed(i64::MAX);
        assert_eq!(x.as_str(), "9223372036854775807");

        let x =
            DecimalFormatter::new().force_sign(true).format_signed(i64::MAX);
        assert_eq!(x.as_str(), "+9223372036854775807");

        let x = DecimalFormatter::new().format_signed(0);
        assert_eq!(x.as_str(), "0");

        let x = DecimalFormatter::new().force_sign(true).format_signed(0);
        assert_eq!(x.as_str(), "+0");

        let x = DecimalFormatter::new().force_sign(false).format_signed(0);
        assert_eq!(x.as_str(), "-0");

        let x = DecimalFormatter::new().padding(4).format_signed(0);
        assert_eq!(x.as_str(), "0000");

        let x = DecimalFormatter::new().padding(4).format_signed(789);
        assert_eq!(x.as_str(), "0789");

        let x = DecimalFormatter::new().padding(4).format_signed(-789);
        assert_eq!(x.as_str(), "-0789");

        let x = DecimalFormatter::new()
            .force_sign(true)
            .padding(4)
            .format_signed(789);
        assert_eq!(x.as_str(), "+0789");
    }

    #[test]
    fn fractional_auto() {
        let f = |n| FractionalFormatter::new().format(n).as_str().to_string();

        assert_eq!(f(0), "");
        assert_eq!(f(123_000_000), "123");
        assert_eq!(f(123_456_000), "123456");
        assert_eq!(f(123_456_789), "123456789");
        assert_eq!(f(456_789), "000456789");
        assert_eq!(f(789), "000000789");
    }

    #[test]
    fn fractional_precision() {
        let f = |precision, n| {
            FractionalFormatter::new()
                .precision(Some(precision))
                .format(n)
                .as_str()
                .to_string()
        };

        assert_eq!(f(0, 0), "");
        assert_eq!(f(1, 0), "0");
        assert_eq!(f(9, 0), "000000000");

        assert_eq!(f(3, 123_000_000), "123");
        assert_eq!(f(6, 123_000_000), "123000");
        assert_eq!(f(9, 123_000_000), "123000000");

        assert_eq!(f(3, 123_456_000), "123");
        assert_eq!(f(6, 123_456_000), "123456");
        assert_eq!(f(9, 123_456_000), "123456000");

        assert_eq!(f(3, 123_456_789), "123");
        assert_eq!(f(6, 123_456_789), "123456");
        assert_eq!(f(9, 123_456_789), "123456789");

        // We use truncation, no rounding.
        assert_eq!(f(2, 889_000_000), "88");
        assert_eq!(f(2, 999_000_000), "99");
    }
}
