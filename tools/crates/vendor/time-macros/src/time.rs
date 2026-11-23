use std::iter::{self, Peekable};

use proc_macro::{Span, TokenStream, TokenTree, token_stream};
use time_core::unit::*;

use crate::Error;
use crate::helpers::{consume_any_ident, consume_number, consume_punct};
use crate::to_tokens::ToTokenStream;

enum Period {
    Am,
    Pm,
    _24,
}

pub(crate) struct Time {
    pub(crate) hour: u8,
    pub(crate) minute: u8,
    pub(crate) second: u8,
    pub(crate) nanosecond: u32,
}

fn parse_second_and_nanosecond(
    chars: &mut Peekable<token_stream::IntoIter>,
) -> Result<(Span, u8, u32), Error> {
    match chars.next() {
        Some(TokenTree::Literal(literal)) => {
            let span = literal.span();
            let raw = literal.to_string().replace('_', "");

            if let Some((second, subsecond)) = raw.split_once('.') {
                let Ok(second) = second.parse() else {
                    return Err(Error::InvalidComponent {
                        name: "second",
                        value: raw,
                        span_start: Some(span.start()),
                        span_end: Some(span.end()),
                    });
                };

                let subsecond = subsecond
                    .chars()
                    .chain(iter::repeat('0'))
                    .take(9)
                    .collect::<String>();
                let Ok(nanosecond) = subsecond.parse() else {
                    return Err(Error::InvalidComponent {
                        name: "second",
                        value: raw,
                        span_start: Some(span.start()),
                        span_end: Some(span.end()),
                    });
                };

                Ok((span, second, nanosecond))
            } else {
                let Ok(second) = raw.parse() else {
                    return Err(Error::InvalidComponent {
                        name: "second",
                        value: raw,
                        span_start: Some(span.start()),
                        span_end: Some(span.end()),
                    });
                };
                Ok((span, second, 0))
            }
        }
        Some(tree) => Err(Error::UnexpectedToken { tree }),
        None => Err(Error::UnexpectedEndOfInput),
    }
}

pub(crate) fn parse(chars: &mut Peekable<token_stream::IntoIter>) -> Result<Time, Error> {
    fn consume_period(chars: &mut Peekable<token_stream::IntoIter>) -> (Option<Span>, Period) {
        if let Ok(span) = consume_any_ident(&["am", "AM"], chars) {
            (Some(span), Period::Am)
        } else if let Ok(span) = consume_any_ident(&["pm", "PM"], chars) {
            (Some(span), Period::Pm)
        } else {
            (None, Period::_24)
        }
    }

    let (hour_span, hour) = consume_number("hour", chars)?;

    let ((minute_span, minute), (second_span, second, nanosecond), (period_span, period)) =
        match consume_period(chars) {
            // Nothing but the 12-hour clock hour and AM/PM
            (period_span @ Some(_), period) => (
                (Span::mixed_site(), 0),
                (Span::mixed_site(), 0, 0),
                (period_span, period),
            ),
            (None, _) => {
                consume_punct(':', chars)?;
                let (minute_span, minute) = consume_number::<u8>("minute", chars)?;
                let (second_span, second, nanosecond) = if consume_punct(':', chars).is_ok() {
                    parse_second_and_nanosecond(chars)?
                } else {
                    (Span::mixed_site(), 0, 0)
                };
                let (period_span, period) = consume_period(chars);
                (
                    (minute_span, minute),
                    (second_span, second, nanosecond),
                    (period_span, period),
                )
            }
        };

    let hour = match (hour, period) {
        (0, Period::Am | Period::Pm) => {
            return Err(Error::InvalidComponent {
                name: "hour",
                value: hour.to_string(),
                span_start: Some(hour_span.start()),
                span_end: Some(period_span.unwrap_or_else(|| hour_span.end())),
            });
        }
        (12, Period::Am) => 0,
        (12, Period::Pm) => 12,
        (hour, Period::Am | Period::_24) => hour,
        (hour, Period::Pm) => hour + 12,
    };

    if hour >= Hour::per_t(Day) {
        Err(Error::InvalidComponent {
            name: "hour",
            value: hour.to_string(),
            span_start: Some(hour_span.start()),
            span_end: Some(period_span.unwrap_or_else(|| hour_span.end())),
        })
    } else if minute >= Minute::per_t(Hour) {
        Err(Error::InvalidComponent {
            name: "minute",
            value: minute.to_string(),
            span_start: Some(minute_span.start()),
            span_end: Some(minute_span.end()),
        })
    } else if second >= Second::per_t(Minute) {
        Err(Error::InvalidComponent {
            name: "second",
            value: second.to_string(),
            span_start: Some(second_span.start()),
            span_end: Some(second_span.end()),
        })
    } else {
        Ok(Time {
            hour,
            minute,
            second,
            nanosecond,
        })
    }
}

impl ToTokenStream for Time {
    fn append_to(self, ts: &mut TokenStream) {
        quote_append! { ts
            unsafe {
                ::time::Time::__from_hms_nanos_unchecked(
                    #(self.hour),
                    #(self.minute),
                    #(self.second),
                    #(self.nanosecond),
                )
            }
        }
    }
}
