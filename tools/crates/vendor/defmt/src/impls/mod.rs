macro_rules! default_format {
    () => {
        #[inline]
        fn format(&self, _fmt: Formatter) {
            crate::export::istr(&Self::_format_tag());
            self._format_data();
        }
    };
}

macro_rules! delegate_format {
    ($ty:ty, $self_:ident, $val:expr) => {
        #[inline]
        fn format(&$self_, fmt: Formatter) {
            <$ty as Format>::format($val, fmt)
        }

        #[inline]
        fn _format_tag() -> Str {
            <$ty as Format>::_format_tag()
        }

        #[inline]
        fn _format_data(&$self_) {
            <$ty as Format>::_format_data($val)
        }
    };
}

pub mod adapter;
#[cfg(feature = "alloc")]
mod alloc_;
mod arrays;
mod core_;
mod primitives;
mod tuples;

use defmt_macros::internp;

use crate::{self as defmt, Format, Formatter, Str};
