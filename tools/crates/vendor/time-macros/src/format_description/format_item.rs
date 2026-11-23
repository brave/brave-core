use std::num::NonZero;
use std::str::{self, FromStr};

use super::{Error, Location, OptionExt as _, Span, Spanned, Unused, ast, unused};
use crate::FormatDescriptionVersion;
use crate::format_description::{SpannedValue as _, public};

macro_rules! parse_modifiers {
    ($version:expr, $modifiers:expr, struct {}) => {{
        struct Parsed {}
        drop(($version, $modifiers));
        Ok(Parsed {})
    }};
    ($version:expr, $modifiers:expr, struct { $($field:ident : $modifier:ident),* $(,)? }) => {
        'block: {
            struct Parsed {
                $($field: Option<Spanned<<$modifier as ModifierValue>::Type>>),*
            }

            let mut parsed = Parsed {
                $($field: None),*
            };

            for modifier in $modifiers {
                $(if ident_eq($version, *modifier.key, stringify!($field)) {
                    if parsed.$field.is_some() {
                        break 'block Err(modifier.key.span.error("duplicate modifier key"));
                    }
                    match <$modifier>::from_modifier_value($version, &modifier.value) {
                        Ok(value) => {
                            parsed.$field = Some(
                                <<$modifier as ModifierValue>::Type>::from(value)
                                    .spanned(modifier.value.span)
                            )
                        },
                        Err(err) => break 'block Err(err),
                    }
                    continue;
                })*
                break 'block Err(modifier.key.span.error("invalid modifier key"));
            }

            Ok(parsed)
        }
    };
}

fn ident_eq(version: FormatDescriptionVersion, provided: &str, expected: &str) -> bool {
    if version.is_at_least_v3() {
        provided == expected
    } else {
        provided.len() == expected.len()
            && core::iter::zip(provided.bytes(), expected.bytes())
                .all(|(p, e)| p.to_ascii_lowercase() == e)
    }
}

pub(super) fn parse<'a>(
    ast_items: impl Iterator<Item = Result<ast::Item<'a>, Error>>,
) -> impl Iterator<Item = Result<Item<'a>, Error>> {
    ast_items.map(|ast_item| ast_item.and_then(Item::from_ast))
}

pub(super) enum Item<'a> {
    Literal(&'a [u8]),
    StringLiteral(&'a str),
    Component(Component),
    Optional {
        value: Vec<Self>,
        format: Spanned<bool>,
        _span: Unused<Span>,
    },
    First {
        value: Vec<Vec<Self>>,
        _span: Unused<Span>,
    },
}

impl<'a> Item<'a> {
    pub(super) fn from_ast(ast_item: ast::Item<'a>) -> Result<Self, Error> {
        Ok(match ast_item {
            ast::Item::Literal {
                version,
                value: Spanned { value, span },
            } => {
                if let Ok(value) = str::from_utf8(value) {
                    Item::StringLiteral(value)
                } else if version.is_at_least_v3() {
                    return Err(span.error("v3 format descriptions must be valid UTF-8"));
                } else {
                    Item::Literal(value)
                }
            }
            ast::Item::Component {
                version,
                opening_bracket,
                _leading_whitespace: _,
                name,
                modifiers,
                nested_format_descriptions,
                _trailing_whitespace: _,
                closing_bracket,
            } => {
                if let Some(first_nested_fd) = nested_format_descriptions.first()
                    && first_nested_fd.leading_whitespace.is_none()
                {
                    return Err(Span {
                        start: opening_bracket,
                        end: closing_bracket,
                    }
                    .error("missing leading whitespace before nested format description"));
                }

                if ident_eq(version, *name, "optional") {
                    Self::optional_from_parts(
                        version,
                        opening_bracket,
                        &modifiers,
                        nested_format_descriptions,
                        closing_bracket,
                    )?
                } else if ident_eq(version, *name, "first") {
                    let _modifiers = parse_modifiers!(version, modifiers, struct {})?;

                    if version.is_at_least_v3() && nested_format_descriptions.is_empty() {
                        return Err(Span {
                            start: opening_bracket,
                            end: closing_bracket,
                        }
                        .error(
                            "the `first` component requires at least one nested format description",
                        ));
                    }

                    let items = nested_format_descriptions
                        .into_iter()
                        .map(|nested_format_description| {
                            nested_format_description
                                .items
                                .into_iter()
                                .map(Item::from_ast)
                                .collect()
                        })
                        .collect::<Result<_, _>>()?;

                    Item::First {
                        value: items,
                        _span: unused(opening_bracket.to(closing_bracket)),
                    }
                } else {
                    if !nested_format_descriptions.is_empty() {
                        return Err(Span {
                            start: opening_bracket,
                            end: closing_bracket,
                        }
                        .error("this component does not support nested format descriptions"));
                    }

                    Item::Component(component_from_ast(version, &name, &modifiers)?)
                }
            }
        })
    }

    fn optional_from_parts(
        version: FormatDescriptionVersion,
        opening_bracket: Location,
        modifiers: &[ast::Modifier<'a>],
        nested_format_descriptions: Vec<ast::NestedFormatDescription<'a>>,
        closing_bracket: Location,
    ) -> Result<Self, Error> {
        let modifiers = parse_modifiers!(version, modifiers, struct {
            format: OptionalFormat,
        })?;

        let nested_format_description = if let Some(second_fd) = nested_format_descriptions.get(1) {
            return Err(Span {
                start: second_fd.opening_bracket,
                end: second_fd.closing_bracket,
            }
            .error("the `optional` component only allows a single nested format description"));
        } else if let Ok([nested_format_description]) =
            <[_; 1]>::try_from(nested_format_descriptions)
        {
            nested_format_description
        } else {
            return Err(Span {
                start: opening_bracket,
                end: closing_bracket,
            }
            .error("missing nested format description for `optional` component"));
        };

        let format = modifiers.format.transpose().map(|val| val.unwrap_or(true));
        let items = nested_format_description
            .items
            .into_iter()
            .map(Item::from_ast)
            .collect::<Result<_, _>>()?;

        if version.is_at_most_v2() && !*format {
            return Err(format
                .span
                .error("v1 and v2 format descriptions must format optional items"));
        }

        Ok(Item::Optional {
            value: items,
            format,
            _span: unused(opening_bracket.to(closing_bracket)),
        })
    }
}

impl TryFrom<(FormatDescriptionVersion, Item<'_>)> for public::OwnedFormatItemInner {
    type Error = Error;

    fn try_from(
        (version, item): (FormatDescriptionVersion, Item<'_>),
    ) -> Result<Self, Self::Error> {
        Ok(match item {
            Item::Literal(literal) => Self::Literal(literal.to_vec()),
            Item::StringLiteral(string) => Self::StringLiteral(string.to_owned()),
            Item::Component(component) => Self::Component((version, component).try_into()?),
            Item::Optional {
                format,
                value,
                _span: _,
            } => Self::Optional {
                format: *format,
                item: Box::new((version, value).try_into()?),
            },
            Item::First { value, _span: _ } => Self::First(
                value
                    .into_iter()
                    .map(|item| (version, item).try_into())
                    .collect::<Result<_, _>>()?,
            ),
        })
    }
}

impl<'a> TryFrom<(FormatDescriptionVersion, Vec<Item<'a>>)> for public::OwnedFormatItemInner {
    type Error = Error;

    fn try_from(
        (version, items): (FormatDescriptionVersion, Vec<Item<'a>>),
    ) -> Result<Self, Self::Error> {
        Ok(Self::Compound(
            items
                .into_iter()
                .map(|item| (version, item).try_into())
                .collect::<Result<_, _>>()?,
        ))
    }
}

macro_rules! component_definition {
    (@if_required required then { $($then:tt)* } $(else { $($else:tt)* })?) => { $($then)* };
    (@if_required then { $($then:tt)* } $(else { $($else:tt)* })?) => { $($($else)*)? };
    (@if_from_str from_str then { $($then:tt)* } $(else { $($else:tt)* })?) => { $($then)* };
    (@if_from_str then { $($then:tt)* } $(else { $($else:tt)* })?) => { $($($else)*)? };

    ($vis:vis enum $name:ident {
        $($variant:ident = $parse_variant:literal {$(
            $(#[$required:tt])?
            $field:ident = $parse_field:literal:
            Option<$(#[$from_str:tt])? $field_type:ty>
        ),* $(,)?}),* $(,)?
    }) => {
        $vis enum $name {
            $($variant($variant),)*
        }

        $($vis struct $variant {
            $($field: Spanned<Option<$field_type>>),*
        })*

        $(impl $variant {
            fn with_modifiers(
                version: FormatDescriptionVersion,
                modifiers: &[ast::Modifier<'_>],
                _component_span: Span,
            ) -> Result<Self, Error>
            {
                #[allow(unused_mut)]
                let mut this = Self {
                    $($field: None.spanned(Span::dummy())),*
                };

                for modifier in modifiers {
                    $(#[allow(clippy::string_lit_as_bytes)]
                    if ident_eq(version, *modifier.key, $parse_field) {
                        this.$field = Some(
                            component_definition!(@if_from_str $($from_str)?
                                then {
                                    parse_from_modifier_value::<$field_type>(&modifier.value)?
                                } else {
                                    <$field_type>::from_modifier_value(version, &modifier.value)?
                                }
                            )
                        ).spanned(modifier.key_value_span());
                        continue;
                    })*
                    return Err(modifier.key.span.error("invalid modifier key"));
                }

                $(component_definition! { @if_required $($required)? then {
                    if this.$field.is_none() {
                        return Err(_component_span.error("missing required modifier"));
                    }
                }})*

                Ok(this)
            }
        })*

        fn component_from_ast(
            version: FormatDescriptionVersion,
            name: &Spanned<&str>,
            modifiers: &[ast::Modifier<'_>],
        ) -> Result<Component, Error> {
            $(#[allow(clippy::string_lit_as_bytes)]
            if ident_eq(version, &name, $parse_variant) {
                return Ok(Component::$variant(
                    $variant::with_modifiers(version, &modifiers, name.span)?
                ));
            })*
            Err(name.span.error("invalid component"))
        }
    }
}

component_definition! {
    pub(super) enum Component {
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

impl TryFrom<(FormatDescriptionVersion, Component)> for public::Component {
    type Error = Error;

    #[inline]
    fn try_from(
        (version, component): (FormatDescriptionVersion, Component),
    ) -> Result<Self, Self::Error> {
        macro_rules! reject_modifier {
            ($modifier:ident, $modifier_str:literal, $context:literal) => {
                if version.is_at_least_v3() && $modifier.value.is_some() {
                    return Err($modifier.span.error(concat!(
                        "the '",
                        $modifier_str,
                        "' modifier is not valid ",
                        $context
                    )));
                }
            };
        }

        use crate::format_description::public::modifier;
        Ok(match component {
            Component::Day(Day { padding }) => Self::Day(modifier::Day {
                padding: padding.unwrap_or_default().into(),
            }),
            Component::End(End { trailing_input }) => Self::End(modifier::End {
                trailing_input: trailing_input.unwrap_or_default().into(),
            }),
            Component::Hour(Hour { padding, base }) => match base.unwrap_or_default() {
                HourBase::Twelve => Self::Hour12(modifier::Hour12 {
                    padding: padding.unwrap_or_default().into(),
                }),
                HourBase::TwentyFour => Self::Hour24(modifier::Hour24 {
                    padding: padding.unwrap_or_default().into(),
                }),
            },
            Component::Ignore(Ignore { count }) => Self::Ignore(modifier::Ignore {
                count: match *count {
                    Some(value) => value,
                    None => bug!("required modifier was not set"),
                },
            }),
            Component::Minute(Minute { padding }) => Self::Minute(modifier::Minute {
                padding: padding.unwrap_or_default().into(),
            }),
            Component::Month(Month {
                padding,
                repr,
                case_sensitive,
            }) => match repr.unwrap_or_default() {
                MonthRepr::Numerical => {
                    reject_modifier!(case_sensitive, "case_sensitive", "for numerical month");
                    Self::MonthNumerical(modifier::MonthNumerical {
                        padding: padding.unwrap_or_default().into(),
                    })
                }
                MonthRepr::Long => {
                    reject_modifier!(padding, "padding", "for long month");
                    Self::MonthLong(modifier::MonthLong {
                        case_sensitive: case_sensitive.unwrap_or_default().into(),
                    })
                }
                MonthRepr::Short => {
                    reject_modifier!(padding, "padding", "for short month");
                    Self::MonthShort(modifier::MonthShort {
                        case_sensitive: case_sensitive.unwrap_or_default().into(),
                    })
                }
            },
            Component::OffsetHour(OffsetHour {
                sign_behavior,
                padding,
            }) => Self::OffsetHour(modifier::OffsetHour {
                sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                padding: padding.unwrap_or_default().into(),
            }),
            Component::OffsetMinute(OffsetMinute { padding }) => {
                Self::OffsetMinute(modifier::OffsetMinute {
                    padding: padding.unwrap_or_default().into(),
                })
            }
            Component::OffsetSecond(OffsetSecond { padding }) => {
                Self::OffsetSecond(modifier::OffsetSecond {
                    padding: padding.unwrap_or_default().into(),
                })
            }
            Component::Ordinal(Ordinal { padding }) => Self::Ordinal(modifier::Ordinal {
                padding: padding.unwrap_or_default().into(),
            }),
            Component::Period(Period {
                case,
                case_sensitive,
            }) => Self::Period(modifier::Period {
                is_uppercase: case.unwrap_or_default().into(),
                case_sensitive: case_sensitive.unwrap_or_default().into(),
            }),
            Component::Second(Second { padding }) => Self::Second(modifier::Second {
                padding: padding.unwrap_or_default().into(),
            }),
            Component::Subsecond(Subsecond { digits }) => Self::Subsecond(modifier::Subsecond {
                digits: digits.unwrap_or_default().into(),
            }),
            Component::UnixTimestamp(UnixTimestamp {
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
            Component::Weekday(Weekday {
                repr,
                one_indexed,
                case_sensitive,
            }) => match repr.unwrap_or_default() {
                WeekdayRepr::Short => {
                    reject_modifier!(one_indexed, "one_indexed", "for short weekday");
                    Self::WeekdayShort(modifier::WeekdayShort {
                        case_sensitive: case_sensitive.unwrap_or_default().into(),
                    })
                }
                WeekdayRepr::Long => {
                    reject_modifier!(one_indexed, "one_indexed", "for long weekday");
                    Self::WeekdayLong(modifier::WeekdayLong {
                        case_sensitive: case_sensitive.unwrap_or_default().into(),
                    })
                }
                WeekdayRepr::Sunday => {
                    reject_modifier!(case_sensitive, "case_sensitive", "for numerical weekday");
                    Self::WeekdaySunday(modifier::WeekdaySunday {
                        one_indexed: one_indexed.unwrap_or_default().into(),
                    })
                }
                WeekdayRepr::Monday => {
                    reject_modifier!(case_sensitive, "case_sensitive", "for numerical weekday");
                    Self::WeekdayMonday(modifier::WeekdayMonday {
                        one_indexed: one_indexed.unwrap_or_default().into(),
                    })
                }
            },
            Component::WeekNumber(WeekNumber { padding, repr }) => match repr.unwrap_or_default() {
                WeekNumberRepr::Iso => Self::WeekNumberIso(modifier::WeekNumberIso {
                    padding: padding.unwrap_or_default().into(),
                }),
                WeekNumberRepr::Sunday => Self::WeekNumberSunday(modifier::WeekNumberSunday {
                    padding: padding.unwrap_or_default().into(),
                }),
                WeekNumberRepr::Monday => Self::WeekNumberMonday(modifier::WeekNumberMonday {
                    padding: padding.unwrap_or_default().into(),
                }),
            },
            Component::Year(Year {
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

                // Handle the change in default modifier value between versions. For v1 and v2, the
                // default is `extended`, but for v3, the default is `standard`.
                let computed_range = if version.is_at_least_v3() {
                    range.unwrap_or(YearRange::Standard)
                } else {
                    range.unwrap_or_default()
                };

                match (
                    base.unwrap_or_default(),
                    repr.unwrap_or_default(),
                    computed_range,
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
                    (YearBase::Calendar, YearRepr::Full, _) => Self::CalendarYearFullStandardRange(
                        modifier::CalendarYearFullStandardRange {
                            padding: padding.unwrap_or_default().into(),
                            sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                        },
                    ),
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
                        Self::IsoYearCenturyExtendedRange(modifier::IsoYearCenturyExtendedRange {
                            padding: padding.unwrap_or_default().into(),
                            sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                        })
                    }
                    (YearBase::IsoWeek, YearRepr::Century, _) => {
                        Self::IsoYearCenturyStandardRange(modifier::IsoYearCenturyStandardRange {
                            padding: padding.unwrap_or_default().into(),
                            sign_is_mandatory: sign_behavior.unwrap_or_default().into(),
                        })
                    }
                    (YearBase::Calendar, YearRepr::LastTwo, _) => {
                        #[cfg(feature = "large-dates")]
                        reject_modifier!(range, "range", "when `repr:last_two` is used");
                        reject_modifier!(sign_behavior, "sign", "when `repr:last_two` is used");
                        Self::CalendarYearLastTwo(modifier::CalendarYearLastTwo {
                            padding: padding.unwrap_or_default().into(),
                        })
                    }
                    (YearBase::IsoWeek, YearRepr::LastTwo, _) => {
                        #[cfg(feature = "large-dates")]
                        reject_modifier!(range, "range", "when `repr:last_two` is used");
                        reject_modifier!(sign_behavior, "sign", "when `repr:last_two` is used");
                        Self::IsoYearLastTwo(modifier::IsoYearLastTwo {
                            padding: padding.unwrap_or_default().into(),
                        })
                    }
                }
            }
        })
    }
}

macro_rules! target_ty {
    ($name:ident $type:ty) => {
        $type
    };
    ($name:ident) => {
        super::public::modifier::$name
    };
}

/// Get the target value for a given enum.
macro_rules! target_value {
    ($name:ident $variant:ident $value:expr) => {
        $value
    };
    ($name:ident $variant:ident) => {
        super::public::modifier::$name::$variant
    };
}

macro_rules! if_not_parse_only {
    (@parse_only $($x:tt)*) => {};
    ($($x:tt)*) => { $($x)* };
}

trait ModifierValue {
    type Type;
}

macro_rules! modifier {
    ($(
        $(@$instruction:ident)? enum $name:ident $(($target_ty:ty))? {
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
            fn from_modifier_value(
                version: FormatDescriptionVersion,
                value: &Spanned<&str>,
            ) -> Result<Self, Error>
            {
                $(if ident_eq(version, &value, $parse_variant) {
                    return Ok(Self::$variant);
                })*
                Err(value.span.error("invalid modifier value"))
            }
        }

        if_not_parse_only! { $(@$instruction)?
            impl ModifierValue for $name {
                type Type = target_ty!($name $($target_ty)?);
            }

            impl From<$name> for target_ty!($name $($target_ty)?) {
                #[inline]
                fn from(modifier: $name) -> Self {
                    match modifier {
                        $($name::$variant => target_value!($name $variant $($target_value)?)),*
                    }
                }
            }
        }
    )+};
}

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

    @parse_only enum MonthRepr {
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

    @parse_only enum UnixTimestampPrecision {
        #[default]
        Second = "second",
        Millisecond = "millisecond",
        Microsecond = "microsecond",
        Nanosecond = "nanosecond",
    }

    @parse_only enum WeekNumberRepr {
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

    @parse_only enum WeekdayRepr {
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

    @parse_only enum YearRepr {
        #[default]
        Full = "full",
        Century = "century",
        LastTwo = "last_two",
    }

    // `Extended` is the default for v1 and v2 format descriptions, but `Standard` is the default
    // for v3 format descriptions. To ensure the macro outputs the correct code, we need to use
    // `Extended` as the default for symmetry with the runtime parser.
    @parse_only enum YearRange {
        Standard = "standard",
        #[default]
        Extended = "extended",
    }
}

fn parse_from_modifier_value<T: FromStr>(value: &Spanned<&str>) -> Result<T, Error> {
    value
        .parse::<T>()
        .map_err(|_| value.span.error("invalid modifier value"))
}
