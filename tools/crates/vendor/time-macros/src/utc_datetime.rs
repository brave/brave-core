use std::iter::Peekable;

use proc_macro::{TokenStream, token_stream};
use time_core::unit::*;

use crate::date::Date;
use crate::error::Error;
use crate::time::Time;
use crate::to_tokens::ToTokenStream;
use crate::{date, time};

pub(crate) struct UtcDateTime {
    pub(crate) date: Date,
    pub(crate) time: Time,
}

impl UtcDateTime {
    pub(crate) const fn to_timestamp(&self) -> (i64, u32) {
        const UNIX_EPOCH_JULIAN_DAY: i64 = 2_440_588;

        let julian_day = {
            let (year, ordinal) = (self.date.year, self.date.ordinal);
            let adj_year = year + 999_999;
            let century = adj_year / 100;

            let days_before_year = (1461 * adj_year as i64 / 4) as i32 - century + century / 4;
            days_before_year + ordinal as i32 - 363_521_075
        };

        let days = (julian_day as i64 - UNIX_EPOCH_JULIAN_DAY) * Second::per_t::<i64>(Day);
        let hours = self.time.hour as i64 * Second::per_t::<i64>(Hour);
        let minutes = self.time.minute as i64 * Second::per_t::<i64>(Minute);
        let seconds = self.time.second as i64;

        let nanoseconds = if self.date.year < 1970 {
            1_000_000_000 - self.time.nanosecond
        } else {
            self.time.nanosecond
        };

        (days + hours + minutes + seconds, nanoseconds)
    }
}

pub(crate) fn parse(chars: &mut Peekable<token_stream::IntoIter>) -> Result<UtcDateTime, Error> {
    let date = date::parse(chars)?;
    let time = time::parse(chars)?;

    if let Some(token) = chars.peek() {
        return Err(Error::UnexpectedToken {
            tree: token.clone(),
        });
    }

    Ok(UtcDateTime { date, time })
}

impl ToTokenStream for UtcDateTime {
    fn append_to(self, ts: &mut TokenStream) {
        quote_append! { ts
            ::time::UtcDateTime::new(
                #S(self.date),
                #S(self.time),
            )
        }
    }
}
