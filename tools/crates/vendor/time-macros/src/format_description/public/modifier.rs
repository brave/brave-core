use std::num::NonZero;

use proc_macro::{Delimiter, Group, Ident, Span, TokenStream, TokenTree};

use crate::to_tokens::{ToTokenStream, ToTokenTree};

macro_rules! to_tokens {
    (
        $(#[$struct_attr:meta])*
        $struct_vis:vis struct $struct_name:ident {$(
            $(#[$field_attr:meta])*
            $field_vis:vis $field_name:ident : $field_ty:ty = $default:pat
        ),* $(,)?}
    ) => {
        $(#[$struct_attr])*
        #[derive(Clone, Copy)]
        $struct_vis struct $struct_name {$(
            $(#[$field_attr])*
            $field_vis $field_name: $field_ty
        ),*}

        impl ToTokenTree for $struct_name {
            fn into_token_tree(self) -> TokenTree {
                let Self {$($field_name),*} = self;

                #[allow(clippy::redundant_pattern_matching)]
                if matches!(($(&$field_name,)*), ($($default,)*)) {
                    return TokenTree::Group(Group::new(
                        Delimiter::None,
                        quote_! { $struct_name::default() }
                    ));
                }

                let mut tokens = quote_! {
                    $struct_name::default()
                };
                $(
                    #[allow(clippy::redundant_pattern_matching)]
                    if !matches!($field_name, $default) {
                        let method_name = Ident::new(concat!("with_", stringify!($field_name)), Span::mixed_site());
                        quote_append!(tokens .#(method_name)(#S($field_name)));
                    }
                )*

                TokenTree::Group(Group::new(
                    Delimiter::Brace,
                    tokens,
                ))
            }
        }
    };

    (
        $(#[$enum_attr:meta])*
        $enum_vis:vis enum $enum_name:ident {$(
            $(#[$variant_attr:meta])*
            $variant_name:ident
        ),+ $(,)?}
    ) => {
        $(#[$enum_attr])*
        #[derive(Clone, Copy)]
        $enum_vis enum $enum_name {$(
            $(#[$variant_attr])*
            $variant_name
        ),+}

        impl ToTokenStream for $enum_name {
            fn append_to(self, ts: &mut TokenStream) {
                quote_append! { ts
                    $enum_name::
                };
                let name = match self {
                    $(Self::$variant_name => stringify!($variant_name)),+
                };
                ts.extend([TokenTree::Ident(Ident::new(name, Span::mixed_site()))]);
            }
        }
    }
}

to_tokens! {
    pub(crate) struct Day {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct MonthShort {
        pub(crate) case_sensitive: bool = true,
    }
}

to_tokens! {
    pub(crate) struct MonthLong {
        pub(crate) case_sensitive: bool = true,
    }
}

to_tokens! {
    pub(crate) struct MonthNumerical {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct Ordinal {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct WeekdayShort {
        pub(crate) case_sensitive: bool = true,
    }
}

to_tokens! {
    pub(crate) struct WeekdayLong {
        pub(crate) case_sensitive: bool = true,
    }
}

to_tokens! {
    pub(crate) struct WeekdaySunday {
        pub(crate) one_indexed: bool = true,
    }
}

to_tokens! {
    pub(crate) struct WeekdayMonday {
        pub(crate) one_indexed: bool = true,
    }
}

to_tokens! {
    pub(crate) struct WeekNumberIso {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct WeekNumberSunday {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct WeekNumberMonday {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct CalendarYearFullStandardRange {
        pub(crate) padding: Padding = Padding::Zero,
        pub(crate) sign_is_mandatory: bool = false,
    }
}

#[cfg(feature = "large-dates")]
to_tokens! {
    pub(crate) struct CalendarYearFullExtendedRange {
        pub(crate) padding: Padding = Padding::Zero,
        pub(crate) sign_is_mandatory: bool = false,
    }
}

to_tokens! {
    pub(crate) struct CalendarYearCenturyStandardRange {
        pub(crate) padding: Padding = Padding::Zero,
        pub(crate) sign_is_mandatory: bool = false,
    }
}

#[cfg(feature = "large-dates")]
to_tokens! {
    pub(crate) struct CalendarYearCenturyExtendedRange {
        pub(crate) padding: Padding = Padding::Zero,
        pub(crate) sign_is_mandatory: bool = false,
    }
}

to_tokens! {
    pub(crate) struct IsoYearFullStandardRange {
        pub(crate) padding: Padding = Padding::Zero,
        pub(crate) sign_is_mandatory: bool = false,
    }
}

#[cfg(feature = "large-dates")]
to_tokens! {
    pub(crate) struct IsoYearFullExtendedRange {
        pub(crate) padding: Padding = Padding::Zero,
        pub(crate) sign_is_mandatory: bool = false,
    }
}

to_tokens! {
    pub(crate) struct IsoYearCenturyStandardRange {
        pub(crate) padding: Padding = Padding::Zero,
        pub(crate) sign_is_mandatory: bool = false,
    }
}

#[cfg(feature = "large-dates")]
to_tokens! {
    pub(crate) struct IsoYearCenturyExtendedRange {
        pub(crate) padding: Padding = Padding::Zero,
        pub(crate) sign_is_mandatory: bool = false,
    }
}

to_tokens! {
    pub(crate) struct CalendarYearLastTwo {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct IsoYearLastTwo {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct Hour12 {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct Hour24 {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct Minute {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct Period {
        pub(crate) is_uppercase: bool = true,
        pub(crate) case_sensitive: bool = true,
    }
}

to_tokens! {
    pub(crate) struct Second {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) enum SubsecondDigits {
        One,
        Two,
        Three,
        Four,
        Five,
        Six,
        Seven,
        Eight,
        Nine,
        OneOrMore,
    }
}

to_tokens! {
    pub(crate) struct Subsecond {
        pub(crate) digits: SubsecondDigits = SubsecondDigits::OneOrMore,
    }
}

to_tokens! {
    pub(crate) struct OffsetHour {
        pub(crate) sign_is_mandatory: bool = false,
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct OffsetMinute {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) struct OffsetSecond {
        pub(crate) padding: Padding = Padding::Zero,
    }
}

to_tokens! {
    pub(crate) enum Padding {
        Space,
        Zero,
        None,
    }
}

#[derive(Clone, Copy)]
pub(crate) struct Ignore {
    pub(crate) count: NonZero<u16>,
}

impl ToTokenTree for Ignore {
    fn into_token_tree(self) -> TokenTree {
        quote_group! {{
            Ignore::count(#(self.count))
        }}
    }
}

to_tokens! {
    pub(crate) struct UnixTimestampSecond {
        pub(crate) sign_is_mandatory: bool = false,
    }
}

to_tokens! {
    pub(crate) struct UnixTimestampMillisecond {
        pub(crate) sign_is_mandatory: bool = false,
    }
}

to_tokens! {
    pub(crate) struct UnixTimestampMicrosecond {
        pub(crate) sign_is_mandatory: bool = false,
    }
}

to_tokens! {
    pub(crate) struct UnixTimestampNanosecond {
        pub(crate) sign_is_mandatory: bool = false,
    }
}

to_tokens! {
    pub(crate) enum TrailingInput {
        Prohibit,
        Discard,
    }
}

to_tokens! {
    pub(crate) struct End {
        pub(crate) trailing_input: TrailingInput = TrailingInput::Prohibit,
    }
}
