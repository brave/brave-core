use proc_macro::{Ident, Span, TokenStream};

use super::modifier;
use crate::to_tokens::ToTokenStream;

macro_rules! declare_component {
    ($($(#[cfg($cfg_inner:meta)])* $name:ident)*) => {
        #[derive(Clone, Copy)]
        pub(crate) enum Component {$(
            $(#[cfg($cfg_inner)])*
            $name(modifier::$name),
        )*}

        impl ToTokenStream for Component {
            fn append_to(self, ts: &mut TokenStream) {
                let mut mts = TokenStream::new();

                let component = match self {$(
                    $(#[cfg($cfg_inner)])*
                    Self::$name(modifier) => {
                        modifier.append_to(&mut mts);
                        stringify!($name)
                    }
                )*};
                let component = Ident::new(component, Span::mixed_site());

                quote_append! { ts
                    Component::#(component)(#S(mts))
                }
            }
        }
    };
}

declare_component! {
    Day
    MonthShort
    MonthLong
    MonthNumerical
    Ordinal
    WeekdayShort
    WeekdayLong
    WeekdaySunday
    WeekdayMonday
    WeekNumberIso
    WeekNumberSunday
    WeekNumberMonday
    #[cfg(feature = "large-dates")]
    CalendarYearFullExtendedRange
    CalendarYearFullStandardRange
    #[cfg(feature = "large-dates")]
    IsoYearFullExtendedRange
    IsoYearFullStandardRange
    #[cfg(feature = "large-dates")]
    CalendarYearCenturyExtendedRange
    CalendarYearCenturyStandardRange
    #[cfg(feature = "large-dates")]
    IsoYearCenturyExtendedRange
    IsoYearCenturyStandardRange
    CalendarYearLastTwo
    IsoYearLastTwo
    Hour12
    Hour24
    Minute
    Period
    Second
    Subsecond
    OffsetHour
    OffsetMinute
    OffsetSecond
    Ignore
    UnixTimestampSecond
    UnixTimestampMillisecond
    UnixTimestampMicrosecond
    UnixTimestampNanosecond
    End
}
