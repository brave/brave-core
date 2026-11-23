use std::iter::Peekable;

use proc_macro::{TokenTree, token_stream};
use time_core::unit::*;
use time_core::util::days_in_year;

use crate::date::{Date, MAX_YEAR, MIN_YEAR};
use crate::helpers::{consume_punct, parse_number};
use crate::time::Time;
use crate::to_tokens::ToTokenStream;
use crate::utc_datetime::UtcDateTime;
use crate::{Error, utc_datetime};

const MAX: i64 = UtcDateTime {
    date: Date {
        year: MAX_YEAR,
        ordinal: days_in_year(MAX_YEAR),
    },
    time: Time {
        hour: 23,
        minute: 59,
        second: 59,
        nanosecond: 999_999_999,
    },
}
.to_timestamp()
.0;

const MIN: i64 = UtcDateTime {
    date: Date {
        year: MIN_YEAR,
        ordinal: 1,
    },
    time: Time {
        hour: 0,
        minute: 0,
        second: 0,
        nanosecond: 0,
    },
}
.to_timestamp()
.0;

pub(crate) struct Timestamp {
    seconds: i64,
    nanoseconds: u32,
}

pub(crate) fn parse(tokens: &mut Peekable<token_stream::IntoIter>) -> Result<Timestamp, Error> {
    // If the input can be parsed as a date-time, do that. Otherwise, try to parse as a timestamp
    // directly.

    let mut tokens2 = tokens.clone();
    if let Ok(udt) = utc_datetime::parse(&mut tokens2) {
        *tokens = tokens2;
        let (seconds, nanoseconds) = udt.to_timestamp();
        return Ok(Timestamp {
            seconds,
            nanoseconds,
        });
    }

    let (sign_span, is_negative) = match consume_punct('-', tokens) {
        Ok(span) => (Some(span), true),
        Err(_) => (None, false),
    };
    let (span, raw) = match tokens.next() {
        Some(TokenTree::Literal(literal)) => (literal.span(), literal.to_string()),
        Some(tree) => return Err(Error::UnexpectedToken { tree }),
        None => return Err(Error::UnexpectedEndOfInput),
    };
    let (seconds_str, fractional) = match raw.split_once('.') {
        Some((int, frac)) => (int, Some(frac)),
        None => (raw.as_str(), None),
    };

    let seconds = parse_number::<i64>("timestamp", seconds_str)?;
    let nanoseconds = if let Some(fractional) = fractional {
        // It's simpler to rely on existing helpers than to reimplement everything here. We can't do
        // this for the overall value due to the large range of valid timestamps, meaning that 9+
        // digits of precision is not guaranteed.
        let frac = format!("0.{fractional}");
        let parsed = parse_number::<f64>("timestamp", &frac)?;
        (parsed.fract() * Nanosecond::per_t::<f64>(Second)).round() as u32
    } else {
        0
    };

    let (seconds, nanoseconds) = match (is_negative, fractional.is_some()) {
        (true, true) => (-seconds - 1, 1_000_000_000 - nanoseconds),
        (true, false) => (-seconds, 0),
        (false, _) => (seconds, nanoseconds),
    };

    if seconds > MAX {
        return Err(Error::Custom {
            message: "timestamp is too large".into(),
            span_start: sign_span.or(Some(span)),
            span_end: Some(span),
        });
    }
    if seconds < MIN {
        return Err(Error::Custom {
            message: "timestamp is too small".into(),
            span_start: sign_span.or(Some(span)),
            span_end: Some(span),
        });
    }

    Ok(Timestamp {
        seconds,
        nanoseconds,
    })
}

impl ToTokenStream for Timestamp {
    fn append_to(self, ts: &mut proc_macro::TokenStream) {
        quote_append! { ts
            unsafe {
                ::time::Timestamp::__new_unchecked(
                    #(self.seconds),
                    #(self.nanoseconds)
                )
            }
        }
    }
}
