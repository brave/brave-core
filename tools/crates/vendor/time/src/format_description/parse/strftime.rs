use alloc::string::String;
use alloc::vec::Vec;

use crate::error::InvalidFormatDescription;
use crate::format_description::modifier::Padding;
use crate::format_description::parse::{
    Error, ErrorInner, Location, Spanned, SpannedValue, unused,
};
use crate::format_description::{BorrowedFormatItem, Component, OwnedFormatItem, modifier};
use crate::internal_macros::try_likely_ok;

/// Parse a sequence of items from the [`strftime` format description][strftime docs].
///
/// The only heap allocation required is for the `Vec` itself. All components are bound to the
/// lifetime of the input.
///
/// [strftime docs]: https://man7.org/linux/man-pages/man3/strftime.3.html
#[doc(alias = "parse_strptime_borrowed")]
#[inline]
pub fn parse_strftime_borrowed(
    s: &str,
) -> Result<Vec<BorrowedFormatItem<'_>>, InvalidFormatDescription> {
    let mut items = Vec::with_capacity(s.bytes().filter(|&b| b == b'%').count().saturating_add(2));
    for item in Tokenizer::new(s.as_bytes()) {
        items.push(try_likely_ok!(item));
    }
    Ok(items)
}

/// Parse a sequence of items from the [`strftime` format description][strftime docs].
///
/// This requires heap allocation for some owned items.
///
/// [strftime docs]: https://man7.org/linux/man-pages/man3/strftime.3.html
#[doc(alias = "parse_strptime_owned")]
#[inline]
pub fn parse_strftime_owned(s: &str) -> Result<OwnedFormatItem, InvalidFormatDescription> {
    parse_strftime_borrowed(s).map(Into::into)
}

struct Tokenizer<'input> {
    input: &'input [u8],
    byte_pos: u32,
}

impl Tokenizer<'_> {
    #[inline]
    const fn new(input: &[u8]) -> Tokenizer<'_> {
        Tokenizer { input, byte_pos: 0 }
    }
}

impl<'input> Iterator for Tokenizer<'input> {
    type Item = Result<BorrowedFormatItem<'input>, Error>;

    #[inline]
    fn next(&mut self) -> Option<Self::Item> {
        if self.input.is_empty() {
            return None;
        }

        if self.input[0] != b'%' {
            let bytes = self
                .input
                .iter()
                .position(|&b| b == b'%')
                .unwrap_or(self.input.len()) as u32;

            // Safety: `parse_strftime` functions only accept strings and only UTF-8 is consumed, so
            // UTF-8 validation is unnecessary.
            let value = unsafe { str::from_utf8_unchecked(&self.input[..bytes as usize]) };
            self.input = &self.input[bytes as usize..];
            self.byte_pos += bytes;

            return Some(Ok(BorrowedFormatItem::StringLiteral(value)));
        }

        let padding = match self.input.get(1) {
            Some(&b'_') => Some(Padding::Space),
            Some(&b'-') => Some(Padding::None),
            Some(&b'0') => Some(Padding::Zero),
            Some(_) => None,
            None => {
                return Some(Err(error_expected_end(Location {
                    byte: self.byte_pos,
                })));
            }
        };

        let (component, advance) = match (padding, self.input.get(2)) {
            (Some(_), Some(&component)) => (component, 3),
            (Some(_), None) => {
                return Some(Err(error_expected_end(Location {
                    byte: self.byte_pos + 2,
                })));
            }
            (None, _) => (self.input[1], 2),
        };

        let component_loc = Location {
            byte: self.byte_pos + (advance - 1) as u32,
        };
        self.input = &self.input[advance..];
        self.byte_pos += advance as u32;
        Some(parse_component(
            padding,
            component.spanned(component_loc.to_self()),
        ))
    }
}

#[cold]
fn error_expected_end(location: Location) -> Error {
    Error {
        _inner: unused(location.error("unexpected end of input")),
        public: InvalidFormatDescription::Expected {
            what: "valid escape sequence",
            index: location.byte as usize,
        },
    }
}

#[cold]
fn error_unsupported_modifier(component: Spanned<u8>) -> Error {
    Error {
        _inner: unused(ErrorInner {
            _message: "unsupported modifier",
            _span: component.span,
        }),
        public: InvalidFormatDescription::NotSupported {
            what: "modifier",
            context: "",
            index: component.span.start.byte as usize,
        },
    }
}

#[cold]
fn error_unsupported_component(component: Spanned<u8>) -> Error {
    Error {
        _inner: unused(ErrorInner {
            _message: "unsupported component",
            _span: component.span,
        }),
        public: InvalidFormatDescription::NotSupported {
            what: "component",
            context: "",
            index: component.span.start.byte as usize,
        },
    }
}

#[cold]
fn error_invalid_component(component: Spanned<u8>) -> Error {
    let name = if component.is_ascii() {
        // Safety: The byte is a single ASCII character, which is guaranteed to be valid
        // UTF-8.
        unsafe { String::from_utf8_unchecked(Vec::from([*component])) }
    } else {
        String::from(char::REPLACEMENT_CHARACTER)
    };

    Error {
        _inner: unused(ErrorInner {
            _message: "invalid component",
            _span: component.span,
        }),
        public: InvalidFormatDescription::InvalidComponentName {
            name,
            index: component.span.start.byte as usize,
        },
    }
}

#[inline]
fn parse_component(
    padding: Option<Padding>,
    component: Spanned<u8>,
) -> Result<BorrowedFormatItem<'static>, Error> {
    /// Helper macro to create a component.
    macro_rules! component {
        ($name:ident { $($inner:tt)* }) => {
            BorrowedFormatItem::Component(Component::$name(modifier::$name {
                $($inner)*
            }))
        }
    }

    Ok(match *component {
        b'%' => BorrowedFormatItem::StringLiteral("%"),
        b'a' => component!(WeekdayShort {
            case_sensitive: true
        }),
        b'A' => component!(WeekdayLong {
            case_sensitive: true,
        }),
        b'b' | b'h' => component!(MonthShort {
            case_sensitive: true,
        }),
        b'B' => component!(MonthLong {
            case_sensitive: true,
        }),
        b'c' => BorrowedFormatItem::Compound(&[
            component!(WeekdayShort {
                case_sensitive: true,
            }),
            BorrowedFormatItem::StringLiteral(" "),
            component!(MonthShort {
                case_sensitive: true,
            }),
            BorrowedFormatItem::StringLiteral(" "),
            component!(Day {
                padding: Padding::Space
            }),
            BorrowedFormatItem::StringLiteral(" "),
            component!(Hour24 {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(":"),
            component!(Minute {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(":"),
            component!(Second {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(" "),
            #[cfg(feature = "large-dates")]
            component!(CalendarYearFullExtendedRange {
                padding: Padding::Zero,
                sign_is_mandatory: false,
            }),
            #[cfg(not(feature = "large-dates"))]
            component!(CalendarYearFullStandardRange {
                padding: Padding::Zero,
                sign_is_mandatory: false,
            }),
        ]),
        #[cfg(feature = "large-dates")]
        b'C' => component!(CalendarYearCenturyExtendedRange {
            padding: padding.unwrap_or(Padding::Zero),
            sign_is_mandatory: false,
        }),
        #[cfg(not(feature = "large-dates"))]
        b'C' => component!(CalendarYearCenturyStandardRange {
            padding: padding.unwrap_or(Padding::Zero),
            sign_is_mandatory: false,
        }),
        b'd' => component!(Day {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b'D' => BorrowedFormatItem::Compound(&[
            component!(MonthNumerical {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral("/"),
            component!(Day {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral("/"),
            component!(CalendarYearLastTwo {
                padding: Padding::Zero,
            }),
        ]),
        b'e' => component!(Day {
            padding: padding.unwrap_or(Padding::Space),
        }),
        b'F' => BorrowedFormatItem::Compound(&[
            #[cfg(feature = "large-dates")]
            component!(CalendarYearFullExtendedRange {
                padding: Padding::Zero,
                sign_is_mandatory: false,
            }),
            #[cfg(not(feature = "large-dates"))]
            component!(CalendarYearFullStandardRange {
                padding: Padding::Zero,
                sign_is_mandatory: false,
            }),
            BorrowedFormatItem::StringLiteral("-"),
            component!(MonthNumerical {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral("-"),
            component!(Day {
                padding: Padding::Zero,
            }),
        ]),
        b'g' => component!(IsoYearLastTwo {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        #[cfg(feature = "large-dates")]
        b'G' => component!(IsoYearFullExtendedRange {
            padding: Padding::Zero,
            sign_is_mandatory: false,
        }),
        #[cfg(not(feature = "large-dates"))]
        b'G' => component!(IsoYearFullStandardRange {
            padding: Padding::Zero,
            sign_is_mandatory: false,
        }),
        b'H' => component!(Hour24 {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b'I' => component!(Hour12 {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b'j' => component!(Ordinal {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b'k' => component!(Hour24 {
            padding: padding.unwrap_or(Padding::Space),
        }),
        b'l' => component!(Hour12 {
            padding: padding.unwrap_or(Padding::Space),
        }),
        b'm' => component!(MonthNumerical {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b'M' => component!(Minute {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b'n' => BorrowedFormatItem::StringLiteral("\n"),
        b'O' => return Err(error_unsupported_modifier(component)),
        b'p' => component!(Period {
            is_uppercase: true,
            case_sensitive: true
        }),
        b'P' => component!(Period {
            is_uppercase: false,
            case_sensitive: true
        }),
        b'r' => BorrowedFormatItem::Compound(&[
            component!(Hour12 {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(":"),
            component!(Minute {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(":"),
            component!(Second {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(" "),
            component!(Period {
                is_uppercase: true,
                case_sensitive: true,
            }),
        ]),
        b'R' => BorrowedFormatItem::Compound(&[
            component!(Hour24 {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(":"),
            component!(Minute {
                padding: Padding::Zero,
            }),
        ]),
        b's' => component!(UnixTimestampSecond {
            sign_is_mandatory: false,
        }),
        b'S' => component!(Second {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b't' => BorrowedFormatItem::StringLiteral("\t"),
        b'T' => BorrowedFormatItem::Compound(&[
            component!(Hour24 {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(":"),
            component!(Minute {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(":"),
            component!(Second {
                padding: Padding::Zero,
            }),
        ]),
        b'u' => component!(WeekdayMonday { one_indexed: true }),
        b'U' => component!(WeekNumberSunday {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b'V' => component!(WeekNumberIso {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b'w' => component!(WeekdaySunday { one_indexed: true }),
        b'W' => component!(WeekNumberMonday {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        b'x' => BorrowedFormatItem::Compound(&[
            component!(MonthNumerical {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral("/"),
            component!(Day {
                padding: Padding::Zero
            }),
            BorrowedFormatItem::StringLiteral("/"),
            component!(CalendarYearLastTwo {
                padding: Padding::Zero,
            }),
        ]),
        b'X' => BorrowedFormatItem::Compound(&[
            component!(Hour24 {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(":"),
            component!(Minute {
                padding: Padding::Zero,
            }),
            BorrowedFormatItem::StringLiteral(":"),
            component!(Second {
                padding: Padding::Zero,
            }),
        ]),
        b'y' => component!(CalendarYearLastTwo {
            padding: padding.unwrap_or(Padding::Zero),
        }),
        #[cfg(feature = "large-dates")]
        b'Y' => component!(CalendarYearFullExtendedRange {
            padding: Padding::Zero,
            sign_is_mandatory: false,
        }),
        #[cfg(not(feature = "large-dates"))]
        b'Y' => component!(CalendarYearFullStandardRange {
            padding: Padding::Zero,
            sign_is_mandatory: false,
        }),
        b'z' => BorrowedFormatItem::Compound(&[
            component!(OffsetHour {
                sign_is_mandatory: true,
                padding: Padding::Zero,
            }),
            component!(OffsetMinute {
                padding: Padding::Zero,
            }),
        ]),
        b'Z' => return Err(error_unsupported_component(component)),
        _ => return Err(error_invalid_component(component)),
    })
}
