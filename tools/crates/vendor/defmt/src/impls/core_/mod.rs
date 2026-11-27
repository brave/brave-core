//! Some of these objects don't expose enough to accurately report their debug state. In this case
//! we show as much state as we can. Users can always use `Debug2Format` to get more information,
//! at the cost of bringing core::fmt into the firmware and doing the layout work on device.
//!
//! We generally keep the type parameter trait bounds in case it becomes possible to use this
//! later, without making a backwards-incompatible change.

mod alloc_;
mod array;
mod cell;
mod fmt;
#[cfg(feature = "ip_in_core")]
mod net;
mod num;
mod ops;
mod panic;
mod ptr;
mod slice;
mod str;

use super::*;
use crate::export;

impl<T> Format for Option<T>
where
    T: Format,
{
    default_format!();

    #[inline]
    fn _format_tag() -> Str {
        internp!("None|Some({=?})")
    }

    #[inline]
    fn _format_data(&self) {
        match self {
            None => export::u8(&0),
            Some(x) => {
                export::u8(&1);
                export::istr(&T::_format_tag());
                x._format_data()
            }
        }
    }
}

impl<T, E> Format for Result<T, E>
where
    T: Format,
    E: Format,
{
    default_format!();

    #[inline]
    fn _format_tag() -> Str {
        internp!("Err({=?})|Ok({=?})")
    }

    #[inline]
    fn _format_data(&self) {
        match self {
            Err(e) => {
                export::u8(&0);
                export::istr(&E::_format_tag());
                e._format_data()
            }
            Ok(x) => {
                export::u8(&1);
                export::istr(&T::_format_tag());
                x._format_data()
            }
        }
    }
}

impl<T> Format for core::marker::PhantomData<T> {
    default_format!();

    #[inline]
    fn _format_tag() -> Str {
        internp!("PhantomData")
    }

    #[inline]
    fn _format_data(&self) {}
}

impl Format for core::convert::Infallible {
    default_format!();

    #[inline]
    fn _format_tag() -> Str {
        unreachable!();
    }

    #[inline]
    fn _format_data(&self) {
        unreachable!();
    }
}

impl Format for core::time::Duration {
    fn format(&self, fmt: Formatter) {
        crate::write!(
            fmt,
            "Duration {{ secs: {=u64}, nanos: {=u32} }}",
            self.as_secs(),
            self.subsec_nanos(),
        )
    }
}

impl<A, B> Format for core::iter::Zip<A, B>
where
    A: Format,
    B: Format,
{
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "Zip(..)")
    }
}
