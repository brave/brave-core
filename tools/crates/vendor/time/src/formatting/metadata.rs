use core::iter::Sum;
use core::ops::{Add, Deref};

use crate::PrivateMethod;
use crate::format_description::format_description_v3::FormatDescriptionV3Inner;
use crate::format_description::well_known::iso8601::EncodedConfig;
use crate::format_description::well_known::{Iso8601, Rfc2822, Rfc3339};
use crate::format_description::{
    BorrowedFormatItem, Component, FormatDescriptionV3, OwnedFormatItem, modifier,
};
use crate::internal_macros::bug;

/// Metadata about a format description.
#[derive(Debug)]
pub(crate) struct Metadata {
    /// The maximum number of bytes needed for the provided format description.
    ///
    /// The number of bytes written should never exceed this value, but it may be less. This is
    /// used to pre-allocate a buffer of the appropriate size for formatting.
    pub(crate) max_bytes_needed: usize,
    /// Whether the output of the provided format description is guaranteed to be valid UTF-8.
    ///
    /// This is used to determine whether the output can be soundly converted to a `String` without
    /// checking for UTF-8 validity.
    pub(crate) guaranteed_utf8: bool,
}

impl Default for Metadata {
    #[inline]
    fn default() -> Self {
        Self {
            max_bytes_needed: 0,
            guaranteed_utf8: true,
        }
    }
}

impl Add for Metadata {
    type Output = Self;

    #[inline]
    fn add(self, rhs: Self) -> Self::Output {
        Self {
            max_bytes_needed: self.max_bytes_needed + rhs.max_bytes_needed,
            guaranteed_utf8: self.guaranteed_utf8 && rhs.guaranteed_utf8,
        }
    }
}

impl Sum for Metadata {
    #[inline]
    fn sum<I>(iter: I) -> Self
    where
        I: Iterator<Item = Self>,
    {
        iter.fold(Self::default(), Self::add)
    }
}

/// A trait for computing metadata about a format description.
pub(crate) trait ComputeMetadata {
    /// Compute the metadata for a format description.
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata;
}

impl ComputeMetadata for Rfc2822 {
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        Metadata {
            max_bytes_needed: 31,
            guaranteed_utf8: true,
        }
    }
}

impl ComputeMetadata for Rfc3339 {
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        Metadata {
            max_bytes_needed: 35,
            guaranteed_utf8: true,
        }
    }
}

impl<const CONFIG: EncodedConfig> ComputeMetadata for Iso8601<CONFIG> {
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        const {
            use crate::format_description::well_known::iso8601::{
                DateKind, OffsetPrecision, TimePrecision,
            };

            let date_width = if Self::FORMAT_DATE {
                let year_width = if Self::YEAR_IS_SIX_DIGITS {
                    7 // sign + 6 digits
                } else {
                    4 // sign is not present when the year is four digits
                };
                let num_dashes = match Self::DATE_KIND {
                    DateKind::Calendar if Self::USE_SEPARATORS => 2,
                    DateKind::Week | DateKind::Ordinal if Self::USE_SEPARATORS => 1,
                    DateKind::Calendar | DateKind::Week | DateKind::Ordinal => 0,
                };
                let part_of_year_width = match Self::DATE_KIND {
                    DateKind::Calendar => 4,
                    DateKind::Week => 4,
                    DateKind::Ordinal => 3,
                };

                year_width + num_dashes + part_of_year_width
            } else {
                0
            };

            let time_width = if Self::FORMAT_TIME {
                let t_separator = (Self::USE_SEPARATORS || Self::FORMAT_DATE) as usize;
                let num_colons = match Self::TIME_PRECISION {
                    TimePrecision::Minute { .. } if Self::USE_SEPARATORS => 1,
                    TimePrecision::Second { .. } if Self::USE_SEPARATORS => 2,
                    TimePrecision::Hour { .. }
                    | TimePrecision::Minute { .. }
                    | TimePrecision::Second { .. } => 0,
                };
                let pre_decimal_digits = match Self::TIME_PRECISION {
                    TimePrecision::Hour { .. } => 2,
                    TimePrecision::Minute { .. } => 4,
                    TimePrecision::Second { .. } => 6,
                };
                let fractional_bytes = match Self::TIME_PRECISION {
                    TimePrecision::Hour { decimal_digits }
                    | TimePrecision::Minute { decimal_digits }
                    | TimePrecision::Second { decimal_digits } => {
                        if let Some(digits) = decimal_digits {
                            // add one for decimal point
                            1 + digits.get() as usize
                        } else {
                            0
                        }
                    }
                };

                t_separator + num_colons + pre_decimal_digits + fractional_bytes
            } else {
                0
            };

            let offset_width = if Self::FORMAT_OFFSET {
                match Self::OFFSET_PRECISION {
                    OffsetPrecision::Hour => 3,
                    OffsetPrecision::Minute if Self::USE_SEPARATORS => 6,
                    OffsetPrecision::Minute => 5,
                }
            } else {
                0
            };

            Metadata {
                max_bytes_needed: date_width + time_width + offset_width,
                guaranteed_utf8: true,
            }
        }
    }
}

impl ComputeMetadata for FormatDescriptionV3<'_> {
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        Metadata {
            max_bytes_needed: self.max_bytes_needed,
            guaranteed_utf8: true,
        }
    }
}

impl ComputeMetadata for FormatDescriptionV3Inner<'_> {
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        bug!(
            "`FormatDescriptionV3Inner` should never be directly used to compute metadata. \
             Instead, the metadata should be pre-computed and stored in `FormatDescriptionV3`."
        )
    }
}

impl ComputeMetadata for BorrowedFormatItem<'_> {
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        match self {
            #[expect(deprecated)]
            Self::Literal(bytes) => Metadata {
                max_bytes_needed: bytes.len(),
                guaranteed_utf8: false,
            },
            Self::StringLiteral(s) => Metadata {
                max_bytes_needed: s.len(),
                guaranteed_utf8: true,
            },
            Self::Component(component) => component.compute_metadata(PrivateMethod),
            Self::Compound(borrowed_format_items) => {
                borrowed_format_items.compute_metadata(PrivateMethod)
            }
            Self::Optional(borrowed_format_item) => {
                borrowed_format_item.compute_metadata(PrivateMethod)
            }
            Self::First(borrowed_format_items) => borrowed_format_items
                .first()
                .map_or_else(Metadata::default, |item| {
                    item.compute_metadata(PrivateMethod)
                }),
        }
    }
}

impl ComputeMetadata for OwnedFormatItem {
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        match self {
            #[expect(deprecated)]
            Self::Literal(bytes) => Metadata {
                max_bytes_needed: bytes.len(),
                guaranteed_utf8: false,
            },
            Self::StringLiteral(s) => Metadata {
                max_bytes_needed: s.len(),
                guaranteed_utf8: true,
            },
            Self::Component(component) => component.compute_metadata(PrivateMethod),
            Self::Compound(owned_format_items) => {
                owned_format_items.compute_metadata(PrivateMethod)
            }
            Self::Optional(owned_format_item) => owned_format_item.compute_metadata(PrivateMethod),
            Self::First(owned_format_items) => owned_format_items
                .first()
                .map_or_else(Metadata::default, |item| {
                    item.compute_metadata(PrivateMethod)
                }),
        }
    }
}

impl ComputeMetadata for Component {
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        let max_bytes_needed = match self {
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

            // Start of deprecated components that are no longer emitted by macros or parsers.
            #[expect(deprecated)]
            Self::Month(modifier) => match modifier.repr {
                modifier::MonthRepr::Numerical => 2,
                modifier::MonthRepr::Long => 9,
                modifier::MonthRepr::Short => 3,
            },
            #[expect(deprecated)]
            Self::Weekday(modifier) => match modifier.repr {
                modifier::WeekdayRepr::Short => 3,
                modifier::WeekdayRepr::Long => 9,
                modifier::WeekdayRepr::Sunday | modifier::WeekdayRepr::Monday => 1,
            },
            #[expect(deprecated)]
            Self::WeekNumber(_) => 2,
            #[expect(deprecated)]
            Self::Hour(_) => 2,
            #[cfg(feature = "large-dates")]
            #[expect(deprecated)]
            Self::UnixTimestamp(modifier) => match modifier.precision {
                modifier::UnixTimestampPrecision::Second => 15,
                modifier::UnixTimestampPrecision::Millisecond => 18,
                modifier::UnixTimestampPrecision::Microsecond => 21,
                modifier::UnixTimestampPrecision::Nanosecond => 24,
            },
            #[cfg(not(feature = "large-dates"))]
            #[expect(deprecated)]
            Self::UnixTimestamp(modifier) => match modifier.precision {
                modifier::UnixTimestampPrecision::Second => 13,
                modifier::UnixTimestampPrecision::Millisecond => 16,
                modifier::UnixTimestampPrecision::Microsecond => 19,
                modifier::UnixTimestampPrecision::Nanosecond => 22,
            },
            #[cfg(feature = "large-dates")]
            #[expect(deprecated)]
            Self::Year(modifier) => match modifier.repr {
                modifier::YearRepr::Full => 7,
                modifier::YearRepr::Century => 5,
                modifier::YearRepr::LastTwo => 2,
            },
            #[cfg(not(feature = "large-dates"))]
            #[expect(deprecated)]
            Self::Year(modifier) => match modifier.repr {
                modifier::YearRepr::Full => 5,
                modifier::YearRepr::Century => 3,
                modifier::YearRepr::LastTwo => 2,
            },
        };

        Metadata {
            max_bytes_needed,
            guaranteed_utf8: true,
        }
    }
}

impl<T> ComputeMetadata for [T]
where
    T: ComputeMetadata,
{
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        self.iter()
            .map(|item| item.compute_metadata(PrivateMethod))
            .sum()
    }
}

impl<T> ComputeMetadata for T
where
    T: Deref<Target: ComputeMetadata>,
{
    #[inline]
    fn compute_metadata(&self, _: PrivateMethod) -> Metadata {
        self.deref().compute_metadata(PrivateMethod)
    }
}
