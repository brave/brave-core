//! Typed, validated representation of a parsed format description.

use alloc::borrow::ToOwned as _;
use core::num::NonZero;
use core::str::{self, FromStr};

use super::lexer_ast::Modifier;
use super::{Error, OptionExt as _, Span, Spanned, SpannedValue as _, unused};
use crate::error::InvalidFormatDescription;
use crate::format_description::__private::FormatDescriptionV3Inner;
use crate::hint;
use crate::internal_macros::{bug, try_likely_ok};

macro_rules! parse_modifiers {
    ($version:expr, $modifiers:expr, struct { $($field:ident : $modifier:ident),* $(,)? }) => {
        'block: {
            struct Parsed {
                $($field: Option<Spanned<<$modifier as ModifierValue>::Type>>),*
            }

            let mut parsed = Parsed {
                $($field: None),*
            };

            for modifier in $modifiers {
                $(if ident_eq::<VERSION>(&modifier.key, stringify!($field)) {
                    hint::cold_path();
                    if version!(3..) && parsed.$field.is_some() {
                        break 'block Err(Error {
                            _inner: unused(modifier.key_span().error("duplicate modifier key")),
                            public: InvalidFormatDescription::DuplicateModifier {
                                name: stringify!($field),
                                index: modifier.key.location.byte as usize,
                            }
                        });
                    }
                    match <$modifier>::from_modifier_value::<VERSION>(
                        || modifier.value_span(),
                        modifier.value,
                    ) {
                        Ok(value) => {
                            parsed.$field = Some(
                                <<$modifier as ModifierValue>::Type>::from(value)
                                    .spanned(modifier.value_span())
                            )
                        },
                        Err(err) => {
                            hint::cold_path();
                            break 'block Err(err)
                        },
                    }
                    continue;
                })*

                hint::cold_path();
                break 'block Err(Error {
                    _inner: unused(modifier.key_span().error("invalid modifier key")),
                    public: InvalidFormatDescription::InvalidModifier {
                        value: (*modifier.key).to_owned(),
                        index: modifier.key.location.byte as usize,
                    }
                });
            }

            Ok(parsed)
        }
    };
}

#[inline]
pub(super) fn ident_eq<const VERSION: u8>(provided: &str, expected: &str) -> bool {
    assert_version!();
    if version!(3..) {
        provided == expected
    } else {
        provided.len() == expected.len()
            && core::iter::zip(provided.bytes(), expected.bytes())
                .all(|(p, e)| p.to_ascii_lowercase() == e)
    }
}

pub(super) fn parse_optional_format_modifier<const VERSION: u8>(
    modifiers: &[Modifier<'_>],
) -> Result<Spanned<bool>, Error> {
    let modifiers = parse_modifiers!(VERSION, modifiers, struct {
        format: OptionalFormat,
    })?;

    Ok(modifiers.format.transpose().map(|val| val.unwrap_or(true)))
}

/// Declare the `Component` struct.
macro_rules! component_definition {
    (@if_required required then { $($then:tt)* } $(else { $($else:tt)* })?) => { $($then)* };
    (@if_required then { $($then:tt)* } $(else { $($else:tt)* })?) => { $($($else)*)? };
    (@if_from_str from_str then { $($then:tt)* } $(else { $($else:tt)* })?) => { $($then)* };
    (@if_from_str then { $($then:tt)* } $(else { $($else:tt)* })?) => { $($($else)*)? };
    (@if_year "year" $($then:tt)*) => { $($then)* };
    (@if_year $lit:tt $($then:tt)*) => {};

    ($vis:vis enum $name:ident {$(
        $variant:ident = $parse_variant:tt {$(
            $(#[$required:tt])?
            $field:ident = $parse_field:literal:
            Option<$(#[$from_str:tt])? $field_type:ty>
        ),* $(,)?}
    ),* $(,)?}) => {
        $vis enum $name {
            $($variant($variant),)*
        }

        $($vis struct $variant {
            $($field: Spanned<Option<$field_type>>),*
        })*

        $(impl $variant {
            /// Parse the component from the AST, given its modifiers.
            #[inline]
            fn with_modifiers<const VERSION: u8>(
                modifiers: &[Modifier<'_>],
                _component_span: Span,
            ) -> Result<Self, Error>
            {
                assert_version!();

                // rustc will complain if the modifier is empty.
                #[allow(unused_mut)]
                let mut this = Self {
                    $($field: None.spanned(Span::DUMMY)),*
                };

                for modifier in modifiers {
                    $(if ident_eq::<VERSION>(&modifier.key, $parse_field) {
                        if version!(3..) && this.$field.is_some() {
                            hint::cold_path();
                            return Err(Error {
                                _inner: unused(modifier.key_span().error("duplicate modifier key")),
                                public: InvalidFormatDescription::DuplicateModifier {
                                    name: stringify!($field),
                                    index: modifier.key.location.byte as usize,
                                }
                            });
                        }
                        this.$field = Some(
                            component_definition!(@if_from_str $($from_str)?
                                then {
                                    parse_from_modifier_value::<$field_type>(
                                        || modifier.value_span(),
                                        modifier.value,
                                    )?
                                } else {
                                    <$field_type>::from_modifier_value::<VERSION>(
                                        || modifier.value_span(),
                                        modifier.value,
                                    )?
                                }
                            )
                        ).spanned(modifier.key_value_span());
                        continue;
                    })*

                    hint::cold_path();
                    return Err(Error {
                        _inner: unused(modifier.key_span().error("invalid modifier key")),
                        public: InvalidFormatDescription::InvalidModifier {
                            value: (*modifier.key).to_owned(),
                            index: modifier.key.location.byte as usize,
                        }
                    });
                }

                $(component_definition! { @if_required $($required)? then {
                    if this.$field.is_none() {
                        hint::cold_path();
                        return Err(Error {
                            _inner: unused(_component_span.error("missing required modifier")),
                            public:
                                InvalidFormatDescription::MissingRequiredModifier {
                                    name: $parse_field,
                                    index: _component_span.start.byte as usize,
                                }
                        });
                    }
                }})*

                Ok(this)
            }
        })*

        /// Parse a component from the AST, given its name and modifiers.
        #[inline]
        pub(super) fn component_from_ast<const VERSION: u8>(
            name: &Spanned<&str>,
            modifiers: &[Modifier<'_>],
        ) -> Result<AstComponent, Error> {
            assert_version!();

            $(if ident_eq::<VERSION>(&name, $parse_variant) {
                #[allow(unused_mut)] // only used for some variants
                let mut component = AstComponent::$variant(
                    try_likely_ok!($variant::with_modifiers::<VERSION>(&modifiers, name.span))
                );
                component_definition!(@if_year $parse_variant
                    #[cfg(feature = "large-dates")]
                    if version!(3..)
                        && let AstComponent::Year(y) = &mut component
                        && y.range.value.is_none()
                    {
                        y.range = Some(YearRange::Standard).spanned(Span::DUMMY);
                    }
                );
                return Ok(component);
            })*

            hint::cold_path();
            Err(Error {
                _inner: unused(name.span.error("invalid component")),
                public: InvalidFormatDescription::InvalidComponentName {
                    name: (**name).to_owned(),
                    index: name.span.start.byte as usize,
                },
            })
        }
    }
}

// Keep in alphabetical order.
component_definition! {
    pub(super) enum AstComponent {
        Day = "day" {
            padding = "padding": Option<Padding>,
        },
        End = "end" {
            trailing_input = "trailing_input": Option<TrailingInput>,
        },
        Hour = "hour" {
            padding = "padding": Option<Padding>,
            base = "repr": Option<HourBase>,
        },
        Ignore = "ignore" {
            #[required]
            count = "count": Option<#[from_str] NonZero<u16>>,
        },
        Minute = "minute" {
            padding = "padding": Option<Padding>,
        },
        Month = "month" {
            padding = "padding": Option<Padding>,
            repr = "repr": Option<MonthRepr>,
            case_sensitive = "case_sensitive": Option<MonthCaseSensitive>,
        },
        OffsetHour = "offset_hour" {
            sign_behavior = "sign": Option<SignBehavior>,
            padding = "padding": Option<Padding>,
        },
        OffsetMinute = "offset_minute" {
            padding = "padding": Option<Padding>,
        },
        OffsetSecond = "offset_second" {
            padding = "padding": Option<Padding>,
        },
        Ordinal = "ordinal" {
            padding = "padding": Option<Padding>,
        },
        Period = "period" {
            case = "case": Option<PeriodCase>,
            case_sensitive = "case_sensitive": Option<PeriodCaseSensitive>,
        },
        Second = "second" {
            padding = "padding": Option<Padding>,
        },
        Subsecond = "subsecond" {
            digits = "digits": Option<SubsecondDigits>,
        },
        UnixTimestamp = "unix_timestamp" {
            precision = "precision": Option<UnixTimestampPrecision>,
            sign_behavior = "sign": Option<SignBehavior>,
        },
        Weekday = "weekday" {
            repr = "repr": Option<WeekdayRepr>,
            one_indexed = "one_indexed": Option<WeekdayOneIndexed>,
            case_sensitive = "case_sensitive": Option<WeekdayCaseSensitive>,
        },
        WeekNumber = "week_number" {
            padding = "padding": Option<Padding>,
            repr = "repr": Option<WeekNumberRepr>,
        },
        Year = "year" {
            padding = "padding": Option<Padding>,
            repr = "repr": Option<YearRepr>,
            range = "range": Option<YearRange>,
            base = "base": Option<YearBase>,
            sign_behavior = "sign": Option<SignBehavior>,
        },
    }
}

macro_rules! impl_from_ast_component_for {
    ($([$reject_nonsensical:literal] $ty:ty),+ $(,)?) => {$(
        impl TryFrom<AstComponent> for $ty {
            type Error = Error;

            #[inline]
            fn try_from(component: AstComponent) -> Result<Self, Self::Error> {
                macro_rules! reject_modifier {
                    ($modifier:ident, $modifier_str:literal, $context:literal) => {
                        if $reject_nonsensical && $modifier.value.is_some() {
                            hint::cold_path();
                            return Err(Error {
                                _inner: unused($modifier.span.error(concat!(
                                    "the '",
                                    $modifier_str,
                                    "' modifier is not valid ",
                                    $context
                                ))),
                                public: InvalidFormatDescription::InvalidModifierCombination {
                                    modifier: $modifier_str,
                                    context: $context,
                                    index: $modifier.span.start.byte as usize,
                                },
                            });
                        }
                    };
                }

                use crate::format_description::modifier;
                Ok(match component {
                    AstComponent::Day(Day { padding }) => Self::Day(modifier::Day {
                        padding: padding.unwrap_or_default().into(),
                    }),
                    AstComponent::End(End { trailing_input }) => Self::End(modifier::End {
                        trailing_input: trailing_input.unwrap_or_default().into(),
                    }),
                    AstComponent::Hour(Hour { padding, base }) => match base.unwrap_or_default() {
                        HourBase::Twelve => Self::Hour12(modifier::Hour12 {
                            padding: padding.unwrap_or_default().into(),
                        }),
                        HourBase::TwentyFour => Self::Hour24(modifier::Hour24 {
                            padding: padding.unwrap_or_default().into(),
                        }),
                    },
                    AstComponent::Ignore(Ignore { count }) => Self::Ignore(modifier::Ignore {
                        count: match *count {
                            Some(value) => value,
                            None => bug!("required modifier was not set"),
                        },
                    }),
                    AstComponent::Minute(Minute { padding }) => Self::Minute(modifier::Minute {
                        padding: padding.unwrap_or_default().into(),
                    }),
                    AstComponent::Month(Month {
                        padding,
                        repr,
                        case_sensitive,
                    }) => match repr.unwrap_or_default() {
                        MonthRepr::Numerical => {
                            reject_modifier!(
                                case_sensitive,
                                "case_sensitive",
                                "for numerical month"
                            );
                            Self::MonthNumerical(modifier::MonthNumerical {
                                padding: padding.unwrap_or_default().into(),
                            })
                        },
                        MonthRepr::Long => {
                            reject_modifier!(padding, "padding", "for long month");
                            Self::MonthLong(modifier::MonthLong {
                                case_sensitive: case_sensitive.unwrap_or_default().into(),
                            })
                        },
                        MonthRepr::Short => {
                            reject_modifier!(padding, "padding", "for short month");
                            Self::MonthShort(modifier::MonthShort {
                                case_sensitive: case_sensitive.unwrap_or_default().into(),
                            })
                        },
                    },
                    AstComponent::OffsetHour(OffsetHour {
                        sign_behavior,
                        padding,
                    }) => Self::OffsetHour(modifier::OffsetHour {
                        sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                        padding: padding.unwrap_or_default().into(),
                    }),
                    AstComponent::OffsetMinute(OffsetMinute { padding }) => {
                        Self::OffsetMinute(modifier::OffsetMinute {
                            padding: padding.unwrap_or_default().into(),
                        })
                    }
                    AstComponent::OffsetSecond(OffsetSecond { padding }) => {
                        Self::OffsetSecond(modifier::OffsetSecond {
                            padding: padding.unwrap_or_default().into(),
                        })
                    }
                    AstComponent::Ordinal(Ordinal { padding }) => Self::Ordinal(modifier::Ordinal {
                        padding: padding.unwrap_or_default().into(),
                    }),
                    AstComponent::Period(Period {
                        case,
                        case_sensitive,
                    }) => Self::Period(modifier::Period {
                        is_uppercase: case.unwrap_or_default().into(),
                        case_sensitive: case_sensitive.unwrap_or_default().into(),
                    }),
                    AstComponent::Second(Second { padding }) => Self::Second(modifier::Second {
                        padding: padding.unwrap_or_default().into(),
                    }),
                    AstComponent::Subsecond(Subsecond { digits }) => {
                        Self::Subsecond(modifier::Subsecond {
                            digits: digits.unwrap_or_default().into(),
                        })
                    },
                    AstComponent::UnixTimestamp(UnixTimestamp {
                        precision,
                        sign_behavior,
                    }) => match precision.unwrap_or_default() {
                        UnixTimestampPrecision::Second => {
                            Self::UnixTimestampSecond(modifier::UnixTimestampSecond {
                                sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                            })
                        }
                        UnixTimestampPrecision::Millisecond => {
                            Self::UnixTimestampMillisecond(modifier::UnixTimestampMillisecond {
                                sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                            })
                        }
                        UnixTimestampPrecision::Microsecond => {
                            Self::UnixTimestampMicrosecond(modifier::UnixTimestampMicrosecond {
                                sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                            })
                        }
                        UnixTimestampPrecision::Nanosecond => {
                            Self::UnixTimestampNanosecond(modifier::UnixTimestampNanosecond {
                                sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                            })
                        }
                    },
                    AstComponent::Weekday(Weekday {
                        repr,
                        one_indexed,
                        case_sensitive,
                    }) => match repr.unwrap_or_default() {
                        WeekdayRepr::Short => {
                            reject_modifier!(one_indexed, "one_indexed", "for short weekday");
                            Self::WeekdayShort(modifier::WeekdayShort {
                                case_sensitive: case_sensitive.unwrap_or_default().into(),
                            })
                        },
                        WeekdayRepr::Long => {
                            reject_modifier!(one_indexed, "one_indexed", "for long weekday");
                            Self::WeekdayLong(modifier::WeekdayLong {
                                case_sensitive: case_sensitive.unwrap_or_default().into(),
                            })
                        },
                        WeekdayRepr::Sunday => {
                            reject_modifier!(
                                case_sensitive,
                                "case_sensitive",
                                "for numerical weekday"
                            );
                            Self::WeekdaySunday(modifier::WeekdaySunday {
                                one_indexed: one_indexed.unwrap_or_default().into(),
                            })
                        },
                        WeekdayRepr::Monday => {
                            reject_modifier!(
                                case_sensitive,
                                "case_sensitive",
                                "for numerical weekday"
                            );
                            Self::WeekdayMonday(modifier::WeekdayMonday {
                                one_indexed: one_indexed.unwrap_or_default().into(),
                            })
                        },
                    },
                    AstComponent::WeekNumber(WeekNumber { padding, repr }) => {
                        match repr.unwrap_or_default() {
                            WeekNumberRepr::Iso => {
                                Self::WeekNumberIso(modifier::WeekNumberIso {
                                    padding: padding.unwrap_or_default().into(),
                                })
                            },
                            WeekNumberRepr::Sunday => {
                                Self::WeekNumberSunday(modifier::WeekNumberSunday {
                                    padding: padding.unwrap_or_default().into(),
                                })
                            },
                            WeekNumberRepr::Monday => {
                                Self::WeekNumberMonday(modifier::WeekNumberMonday {
                                    padding: padding.unwrap_or_default().into(),
                                })
                            },
                        }
                    }
                    AstComponent::Year(Year {
                        padding,
                        repr,
                        range,
                        base,
                        sign_behavior,
                    }) => {
                        #[cfg(not(feature = "large-dates"))]
                        reject_modifier!(
                            range,
                            "range",
                            "when the `large-dates` feature is not enabled"
                        );

                        match (
                            base.unwrap_or_default(),
                            repr.unwrap_or_default(),
                            range.unwrap_or_default(),
                        ) {
                            #[cfg(feature = "large-dates")]
                            (YearBase::Calendar, YearRepr::Full, YearRange::Extended) => {
                                Self::CalendarYearFullExtendedRange(
                                    modifier::CalendarYearFullExtendedRange {
                                        padding: padding.unwrap_or_default().into(),
                                        sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                                    },
                                )
                            }
                            (YearBase::Calendar, YearRepr::Full, _) => {
                                Self::CalendarYearFullStandardRange(
                                    modifier::CalendarYearFullStandardRange {
                                        padding: padding.unwrap_or_default().into(),
                                        sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                                    },
                                )
                            }
                            #[cfg(feature = "large-dates")]
                            (YearBase::Calendar, YearRepr::Century, YearRange::Extended) => {
                                Self::CalendarYearCenturyExtendedRange(
                                    modifier::CalendarYearCenturyExtendedRange {
                                        padding: padding.unwrap_or_default().into(),
                                        sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                                    },
                                )
                            }
                            (YearBase::Calendar, YearRepr::Century, _) => {
                                Self::CalendarYearCenturyStandardRange(
                                    modifier::CalendarYearCenturyStandardRange {
                                        padding: padding.unwrap_or_default().into(),
                                        sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                                    },
                                )
                            }
                            #[cfg(feature = "large-dates")]
                            (YearBase::IsoWeek, YearRepr::Full, YearRange::Extended) => {
                                Self::IsoYearFullExtendedRange(modifier::IsoYearFullExtendedRange {
                                    padding: padding.unwrap_or_default().into(),
                                    sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                                })
                            }
                            (YearBase::IsoWeek, YearRepr::Full, _) => {
                                Self::IsoYearFullStandardRange(modifier::IsoYearFullStandardRange {
                                    padding: padding.unwrap_or_default().into(),
                                    sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                                })
                            }
                            #[cfg(feature = "large-dates")]
                            (YearBase::IsoWeek, YearRepr::Century, YearRange::Extended) => {
                                Self::IsoYearCenturyExtendedRange(
                                    modifier::IsoYearCenturyExtendedRange {
                                        padding: padding.unwrap_or_default().into(),
                                        sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                                    },
                                )
                            }
                            (YearBase::IsoWeek, YearRepr::Century, _) => {
                                Self::IsoYearCenturyStandardRange(
                                    modifier::IsoYearCenturyStandardRange {
                                        padding: padding.unwrap_or_default().into(),
                                        sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                                    },
                                )
                            }
                            (YearBase::Calendar, YearRepr::LastTwo, _) => {
                                #[cfg(feature = "large-dates")]
                                reject_modifier!(range, "range", "when `repr:last_two` is used");
                                reject_modifier!(
                                    sign_behavior,
                                    "sign",
                                    "when `repr:last_two` is used"
                                );
                                Self::CalendarYearLastTwo(modifier::CalendarYearLastTwo {
                                    padding: padding.unwrap_or_default().into(),
                                })
                            }
                            (YearBase::IsoWeek, YearRepr::LastTwo, _) => {
                                #[cfg(feature = "large-dates")]
                                reject_modifier!(range, "range", "when `repr:last_two` is used");
                                reject_modifier!(
                                    sign_behavior,
                                    "sign",
                                    "when `repr:last_two` is used"
                                );
                                Self::IsoYearLastTwo(modifier::IsoYearLastTwo {
                                    padding: padding.unwrap_or_default().into(),
                                })
                            }
                        }
                    }
                })
            }
        })+
    }
}

impl_from_ast_component_for!(
    [false] crate::format_description::Component,
    [true] FormatDescriptionV3Inner<'_>,
);

/// Get the target type for a given enum.
macro_rules! target_ty {
    ($name:ident $type:ty) => {
        $type
    };
    ($name:ident) => {
        $crate::format_description::modifier::$name
    };
}

/// Get the target value for a given enum.
macro_rules! target_value {
    ($name:ident $variant:ident $value:expr) => {
        $value
    };
    ($name:ident $variant:ident) => {
        $crate::format_description::modifier::$name::$variant
    };
}

trait ModifierValue {
    type Type;
}

/// Declare the various modifiers.
///
/// For the general case, ordinary syntax can be used. Note that you _must_ declare a default
/// variant. The only significant change is that the string representation of the variant must be
/// provided after the variant name. For example, `Numerical = b"numerical"` declares a variant
/// named `Numerical` with the string representation `b"numerical"`. This is the value that will be
/// used when parsing the modifier. The value is not case sensitive.
///
/// If the type in the public API does not have the same name as the type in the internal
/// representation, then the former must be specified in parenthesis after the internal name. For
/// example, `HourBase(bool)` has an internal name "HourBase", but is represented as a boolean in
/// the public API.
///
/// By default, the internal variant name is assumed to be the same as the public variant name. If
/// this is not the case, the qualified path to the variant must be specified in parenthesis after
/// the internal variant name. For example, `Twelve(true)` has an internal variant name "Twelve",
/// but is represented as `true` in the public API.
macro_rules! modifier {
    ($(
        $(#[expect($expect_inner:meta)])?
        enum $name:ident $(($target_ty:ty))? {
            $(
                $(#[$attr:meta])?
                $variant:ident $(($target_value:expr))? = $parse_variant:literal
            ),* $(,)?
        }
    )+) => {$(
        #[derive(Default, Clone, Copy)]
        enum $name {
            $($(#[$attr])? $variant),*
        }

        impl $name {
            /// Parse the modifier from its string representation.
            #[inline]
            fn from_modifier_value<const VERSION: u8>(
                value_span: impl FnOnce() -> Span,
                value: &str,
            ) -> Result<Self, Error>
            {
                assert_version!();

                $(if ident_eq::<VERSION>(&value, $parse_variant) {
                    return Ok(Self::$variant);
                })*

                hint::cold_path();
                let span = value_span();
                Err(Error {
                    _inner: unused(span.error("invalid modifier value")),
                    public: InvalidFormatDescription::InvalidModifier {
                        value: value.to_owned(),
                        index: span.start.byte as usize,
                    },
                })
            }
        }

        $(#[expect($expect_inner)])?
        impl ModifierValue for $name {
            type Type = target_ty!($name $($target_ty)?);
        }

        $(#[expect($expect_inner)])?
        impl From<$name> for target_ty!($name $($target_ty)?) {
            #[inline]
            fn from(modifier: $name) -> Self {
                match modifier {
                    $($name::$variant => target_value!($name $variant $($target_value)?)),*
                }
            }
        }
    )+};
}

// Keep in alphabetical order.
modifier! {
    enum HourBase(bool) {
        Twelve(true) = "12",
        #[default]
        TwentyFour(false) = "24",
    }

    enum MonthCaseSensitive(bool) {
        False(false) = "false",
        #[default]
        True(true) = "true",
    }

    #[expect(deprecated)]
    enum MonthRepr {
        #[default]
        Numerical = "numerical",
        Long = "long",
        Short = "short",
    }

    enum OptionalFormat(bool) {
        False(false) = "false",
        #[default]
        True(true) = "true",
    }

    enum Padding {
        Space = "space",
        #[default]
        Zero = "zero",
        None = "none",
    }

    enum PeriodCase(bool) {
        Lower(false) = "lower",
        #[default]
        Upper(true) = "upper",
    }

    enum PeriodCaseSensitive(bool) {
        False(false) = "false",
        #[default]
        True(true) = "true",
    }

    enum SignBehavior(bool) {
        #[default]
        Automatic(false) = "automatic",
        Mandatory(true) = "mandatory",
    }

    enum SubsecondDigits {
        One = "1",
        Two = "2",
        Three = "3",
        Four = "4",
        Five = "5",
        Six = "6",
        Seven = "7",
        Eight = "8",
        Nine = "9",
        #[default]
        OneOrMore = "1+",
    }

    enum TrailingInput {
        #[default]
        Prohibit = "prohibit",
        Discard = "discard",
    }

    #[expect(deprecated)]
    enum UnixTimestampPrecision {
        #[default]
        Second = "second",
        Millisecond = "millisecond",
        Microsecond = "microsecond",
        Nanosecond = "nanosecond",
    }

    #[expect(deprecated)]
    enum WeekNumberRepr {
        #[default]
        Iso = "iso",
        Sunday = "sunday",
        Monday = "monday",
    }

    enum WeekdayCaseSensitive(bool) {
        False(false) = "false",
        #[default]
        True(true) = "true",
    }

    enum WeekdayOneIndexed(bool) {
        False(false) = "false",
        #[default]
        True(true) = "true",
    }

    #[expect(deprecated)]
    enum WeekdayRepr {
        Short = "short",
        #[default]
        Long = "long",
        Sunday = "sunday",
        Monday = "monday",
    }

    enum YearBase(bool) {
        #[default]
        Calendar(false) = "calendar",
        IsoWeek(true) = "iso_week",
    }

    #[expect(deprecated)]
    enum YearRepr {
        #[default]
        Full = "full",
        Century = "century",
        LastTwo = "last_two",
    }

    // For v1 and v2 format descriptions, the default is `extended`. For v3 format descriptions,
    // the default is `standard`. For backwards compatibility, the default here needs to stay
    // `extended`.
    #[expect(deprecated)]
    enum YearRange {
        Standard = "standard",
        #[default]
        Extended = "extended",
    }
}

/// Parse a modifier value using `FromStr`. Requires the modifier value to be valid UTF-8.
#[inline]
fn parse_from_modifier_value<T>(value_span: impl FnOnce() -> Span, value: &str) -> Result<T, Error>
where
    T: FromStr,
{
    value.parse::<T>().map_err(|_| {
        hint::cold_path();
        let span = value_span();
        Error {
            _inner: unused(span.error("invalid modifier value")),
            public: InvalidFormatDescription::InvalidModifier {
                value: value.to_owned(),
                index: span.start.byte as usize,
            },
        }
    })
}
