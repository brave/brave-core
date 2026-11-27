#[allow(unused_imports)]
use crate as defmt;
use crate::{Format, Formatter, Str};

pub trait Truncate<U> {
    fn truncate(self) -> U;
}

macro_rules! impl_truncate {
    ($($from:ty => $into:ty),*) => {
        $(impl Truncate<$into> for $from {
            fn truncate(self) -> $into {
                self as $into
            }
        })*
    };
}

// We implement `Truncate<X> for X` so that the macro can unconditionally use it,
// even if no truncation is performed.
impl_truncate!(
    u8   => u8,
    u16  => u8,
    u32  => u8,
    u64  => u8,
    u128 => u8,
    u16  => u16,
    u32  => u16,
    u64  => u16,
    u128 => u16,
    u32  => u32,
    u64  => u32,
    u128 => u32,
    u64  => u64,
    u128 => u64,
    u128 => u128
);

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub struct NoneError;

impl Format for NoneError {
    fn format(&self, _fmt: Formatter) {
        unreachable!();
    }

    fn _format_tag() -> Str {
        defmt_macros::internp!("Unwrap of a None option value")
    }

    fn _format_data(&self) {}
}

/// Transform `self` into a `Result`
///
/// # Call sites
/// * [`defmt::unwrap!`]
pub trait IntoResult {
    type Ok;
    type Error;
    fn into_result(self) -> Result<Self::Ok, Self::Error>;
}

impl<T> IntoResult for Option<T> {
    type Ok = T;
    type Error = NoneError;

    #[inline]
    fn into_result(self) -> Result<T, NoneError> {
        self.ok_or(NoneError)
    }
}

impl<T, E> IntoResult for Result<T, E> {
    type Ok = T;
    type Error = E;

    #[inline]
    fn into_result(self) -> Self {
        self
    }
}
