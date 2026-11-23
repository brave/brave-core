use core::num::{self, Wrapping};

use super::*;

macro_rules! non_zero {
    ($type:ty, $hint:literal) => {
        impl Format for $type {
            fn format(&self, fmt: Formatter) {
                crate::write!(fmt, $hint, self.get());
            }
        }
    };
}

non_zero! {num::NonZeroI8, "{=i8}"}
non_zero! {num::NonZeroI16, "{=i16}"}
non_zero! {num::NonZeroI32, "{=i32}"}
non_zero! {num::NonZeroI64, "{=i64}"}
non_zero! {num::NonZeroI128, "{=i128}"}
non_zero! {num::NonZeroIsize, "{=isize}"}
non_zero! {num::NonZeroU8, "{=u8}"}
non_zero! {num::NonZeroU16, "{=u16}"}
non_zero! {num::NonZeroU32, "{=u32}"}
non_zero! {num::NonZeroU64, "{=u64}"}
non_zero! {num::NonZeroU128, "{=u128}"}
non_zero! {num::NonZeroUsize, "{=usize}"}

impl Format for num::TryFromIntError {
    fn format(&self, fmt: Formatter) {
        crate::write!(fmt, "TryFromIntError(())");
    }
}

impl<T: Format> Format for Wrapping<T> {
    fn format(&self, fmt: Formatter) {
        self.0.format(fmt);
    }
}
