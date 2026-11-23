use super::*;
use crate::export;

impl Format for () {
    default_format!();

    #[inline]
    fn _format_tag() -> Str {
        internp!("()")
    }

    #[inline]
    fn _format_data(&self) {}
}

macro_rules! tuple {
    ( $format:expr, ($($name:ident),+) ) => (
        impl<$($name:Format),+> Format for ($($name,)+) where last_type!($($name,)+): ?Sized {
            default_format!();

            #[inline]
            fn _format_tag() -> Str {
                internp!($format)
            }

            #[inline]
            #[allow(non_snake_case, unused_assignments)]
            fn _format_data(&self) {
                let ($(ref $name,)+) = *self;
                $(
                    export::istr(&$name::_format_tag());
                    $name._format_data();
                )+
            }
        }
    )
}

macro_rules! last_type {
    ($a:ident,) => { $a };
    ($a:ident, $($rest_a:ident,)+) => { last_type!($($rest_a,)+) };
}

tuple! { "({=?})", (T0) }
tuple! { "({=?}, {=?})", (T0, T1) }
tuple! { "({=?}, {=?}, {=?})", (T0, T1, T2) }
tuple! { "({=?}, {=?}, {=?}, {=?})", (T0, T1, T2, T3) }
tuple! { "({=?}, {=?}, {=?}, {=?}, {=?})", (T0, T1, T2, T3, T4) }
tuple! { "({=?}, {=?}, {=?}, {=?}, {=?}, {=?})", (T0, T1, T2, T3, T4, T5) }
tuple! { "({=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?})", (T0, T1, T2, T3, T4, T5, T6) }
tuple! { "({=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?})", (T0, T1, T2, T3, T4, T5, T6, T7) }
tuple! { "({=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?})", (T0, T1, T2, T3, T4, T5, T6, T7, T8) }
tuple! { "({=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?})", (T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) }
tuple! { "({=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?})", (T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) }
tuple! { "({=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?}, {=?})", (T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) }
