//! `num_conv` is a crate to convert between integer types without using `as` casts. This provides
//! better certainty when refactoring, makes the exact behavior of code more explicit, and allows
//! using turbofish syntax. The crate is currently in the process of being uplifted into the
//! standard library; see [rust-lang/rust#154330](https://github.com/rust-lang/rust/issues/154330)
//! for details.

#![no_std]

/// Anonymously import all extension traits.
///
/// This allows you to use the methods without worrying about polluting the namespace or importing
/// them individually.
///
/// ```rust
/// use num_conv::prelude::*;
/// ```
pub mod prelude {
    #[allow(deprecated)]
    pub use crate::{Extend as _, Truncate as _, Widen as _};
}

mod sealed {
    pub trait Integer {}

    macro_rules! impl_integer {
        ($($t:ty)*) => {$(
            impl Integer for $t {}
        )*};
    }

    impl_integer! {
        u8 u16 u32 u64 u128 usize
        i8 i16 i32 i64 i128 isize
    }

    #[deprecated(since = "0.2.2", note = "use `WidenTargetSealed` instead")]
    pub trait ExtendTargetSealed<T> {
        #[deprecated(since = "0.2.2", note = "use `widen` instead")]
        fn extend(self) -> T;
    }

    pub trait WidenTargetSealed<T> {
        fn widen(self) -> T;
    }

    pub trait TruncateTargetSealed<T> {
        fn truncate(self) -> T;
        fn saturating_truncate(self) -> T;
        fn checked_truncate(self) -> Option<T>;
    }
}

/// A type that can be used with turbofish syntax in [`Extend::extend`].
///
/// It is unlikely that you will want to use this trait directly. You are probably looking for the
/// [`Extend`] trait.
#[deprecated(since = "0.2.2", note = "use `WidenTarget` instead")]
#[allow(deprecated)]
pub trait ExtendTarget<T>: sealed::ExtendTargetSealed<T> {}

/// A type that can be used with turbofish syntax in [`Widen::widen`].
///
/// It is unlikely that you will want to use this trait directly. You are probably looking for the
/// [`Widen`] trait.
pub trait WidenTarget<T>: sealed::WidenTargetSealed<T> {}

/// A type that can be used with turbofish syntax in [`Truncate::truncate`].
///
/// It is unlikely that you will want to use this trait directly. You are probably looking for the
/// [`Truncate`] trait.
pub trait TruncateTarget<T>: sealed::TruncateTargetSealed<T> {}

/// Widen to an integer of the same size or larger, preserving its value.
///
/// ```rust
/// # use num_conv::Widen;
/// assert_eq!(0_u8.widen::<u16>(), 0_u16);
/// assert_eq!(0_u16.widen::<u32>(), 0_u32);
/// assert_eq!(0_u32.widen::<u64>(), 0_u64);
/// assert_eq!(0_u64.widen::<u128>(), 0_u128);
/// ```
///
/// ```rust
/// # use num_conv::Widen;
/// assert_eq!((-1_i8).widen::<i16>(), -1_i16);
/// assert_eq!((-1_i16).widen::<i32>(), -1_i32);
/// assert_eq!((-1_i32).widen::<i64>(), -1_i64);
/// assert_eq!((-1_i64).widen::<i128>(), -1_i128);
/// ```
pub trait Widen: sealed::Integer {
    /// Widen an integer to an integer of the same size or larger, preserving its value.
    fn widen<T>(self) -> T
    where
        Self: WidenTarget<T>;
}

impl<T: sealed::Integer> Widen for T {
    fn widen<U>(self) -> U
    where
        T: WidenTarget<U>,
    {
        sealed::WidenTargetSealed::widen(self)
    }
}

/// Extend to an integer of the same size or larger, preserving its value.
///
/// ```rust
/// # use num_conv::Extend;
/// assert_eq!(0_u8.extend::<u16>(), 0_u16);
/// assert_eq!(0_u16.extend::<u32>(), 0_u32);
/// assert_eq!(0_u32.extend::<u64>(), 0_u64);
/// assert_eq!(0_u64.extend::<u128>(), 0_u128);
/// ```
///
/// ```rust
/// # use num_conv::Extend;
/// assert_eq!((-1_i8).extend::<i16>(), -1_i16);
/// assert_eq!((-1_i16).extend::<i32>(), -1_i32);
/// assert_eq!((-1_i32).extend::<i64>(), -1_i64);
/// assert_eq!((-1_i64).extend::<i128>(), -1_i128);
/// ```
#[deprecated(since = "0.2.2", note = "use `Widen` instead")]
#[allow(deprecated)]
pub trait Extend: sealed::Integer {
    /// Extend an integer to an integer of the same size or larger, preserving its value.
    fn extend<T>(self) -> T
    where
        Self: ExtendTarget<T>;
}

#[allow(deprecated)]
impl<T: sealed::Integer> Extend for T {
    fn extend<U>(self) -> U
    where
        T: ExtendTarget<U>,
    {
        sealed::ExtendTargetSealed::extend(self)
    }
}

/// Truncate to an integer of the same size or smaller.
///
/// Preserve the least significant bits with `.truncate()`:
///
/// ```rust
/// # use num_conv::Truncate;
/// assert_eq!(u16::MAX.truncate::<u8>(), u8::MAX);
/// assert_eq!(u32::MAX.truncate::<u16>(), u16::MAX);
/// assert_eq!(u64::MAX.truncate::<u32>(), u32::MAX);
/// assert_eq!(u128::MAX.truncate::<u64>(), u64::MAX);
/// ```
///
/// ```rust
/// # use num_conv::Truncate;
/// assert_eq!((-1_i16).truncate::<i8>(), -1_i8);
/// assert_eq!((-1_i32).truncate::<i16>(), -1_i16);
/// assert_eq!((-1_i64).truncate::<i32>(), -1_i32);
/// assert_eq!((-1_i128).truncate::<i64>(), -1_i64);
/// ```
///
/// Saturate to the numeric bounds with `.saturating_truncate()`:
///
/// ```rust
/// # use num_conv::Truncate;
/// assert_eq!(500_u16.saturating_truncate::<u8>(), u8::MAX);
/// assert_eq!(u32::MAX.saturating_truncate::<u16>(), u16::MAX);
/// assert_eq!(u64::MAX.saturating_truncate::<u32>(), u32::MAX);
/// assert_eq!(u128::MAX.saturating_truncate::<u64>(), u64::MAX);
/// ```
///
/// ```rust
/// # use num_conv::Truncate;
/// assert_eq!((-500_i16).saturating_truncate::<i8>(), i8::MIN);
/// assert_eq!(i32::MIN.saturating_truncate::<i16>(), i16::MIN);
/// assert_eq!(i64::MIN.saturating_truncate::<i32>(), i32::MIN);
/// assert_eq!(i128::MIN.saturating_truncate::<i64>(), i64::MIN);
/// ```
///
/// Checked with `.checked_truncate()`, returning `None` if the value is out of range:
///
/// ```rust
/// # use num_conv::Truncate;
/// assert_eq!(u16::MAX.checked_truncate::<u8>(), None);
/// assert_eq!(u32::MAX.checked_truncate::<u16>(), None);
/// assert_eq!(u64::MAX.checked_truncate::<u32>(), None);
/// assert_eq!(u128::MAX.checked_truncate::<u64>(), None);
/// ```
///
/// ```rust
/// # use num_conv::Truncate;
/// assert_eq!(i16::MIN.checked_truncate::<i8>(), None);
/// assert_eq!(i32::MIN.checked_truncate::<i16>(), None);
/// assert_eq!(i64::MIN.checked_truncate::<i32>(), None);
/// assert_eq!(i128::MIN.checked_truncate::<i64>(), None);
/// ```
pub trait Truncate: sealed::Integer {
    /// Truncate an integer to an integer of the same size or smaller, preserving the least
    /// significant bits.
    fn truncate<T>(self) -> T
    where
        Self: TruncateTarget<T>;

    /// Truncate an integer to an integer of the same size or smaller, saturating to the numeric
    /// bounds instead of wrapping.
    fn saturating_truncate<T>(self) -> T
    where
        Self: TruncateTarget<T>;

    /// Truncate an integer to an integer of the same size or smaller, returning `None` if the value
    /// is out of range.
    fn checked_truncate<T>(self) -> Option<T>
    where
        Self: TruncateTarget<T>;
}

impl<T: sealed::Integer> Truncate for T {
    fn truncate<U>(self) -> U
    where
        T: TruncateTarget<U>,
    {
        sealed::TruncateTargetSealed::truncate(self)
    }

    fn saturating_truncate<U>(self) -> U
    where
        T: TruncateTarget<U>,
    {
        sealed::TruncateTargetSealed::saturating_truncate(self)
    }

    fn checked_truncate<U>(self) -> Option<U>
    where
        T: TruncateTarget<U>,
    {
        sealed::TruncateTargetSealed::checked_truncate(self)
    }
}

macro_rules! impl_widen {
    ($($from:ty => $($to:ty),+;)*) => {$($(
        const _: () = assert!(
            core::mem::size_of::<$from>() <= core::mem::size_of::<$to>(),
            concat!(
                "cannot widen ",
                stringify!($from),
                " to ",
                stringify!($to),
                " because ",
                stringify!($from),
                " is larger than ",
                stringify!($to)
            )
        );

        #[allow(deprecated)]
        impl sealed::ExtendTargetSealed<$to> for $from {
            #[inline]
            fn extend(self) -> $to {
                self as _
            }
        }

        impl sealed::WidenTargetSealed<$to> for $from {
            #[inline]
            fn widen(self) -> $to {
                self as _
            }
        }

        #[allow(deprecated)]
        impl ExtendTarget<$to> for $from {}
        impl WidenTarget<$to> for $from {}
    )+)*};
}

macro_rules! impl_truncate {
    ($($($from:ty),+ => $to:ty;)*) => {$($(
        const _: () = assert!(
            core::mem::size_of::<$from>() >= core::mem::size_of::<$to>(),
            concat!(
                "cannot truncate ",
                stringify!($from),
                " to ",
                stringify!($to),
                " because ",
                stringify!($from),
                " is smaller than ",
                stringify!($to)
            )
        );

        impl sealed::TruncateTargetSealed<$to> for $from {
            #[inline]
            fn truncate(self) -> $to {
                self as _
            }

            #[inline]
            fn saturating_truncate(self) -> $to {
                if self > <$to>::MAX as _ {
                    <$to>::MAX
                } else if self < <$to>::MIN as _ {
                    <$to>::MIN
                } else {
                    self as _
                }
            }

            #[inline]
            fn checked_truncate(self) -> Option<$to> {
                if self > <$to>::MAX as _ || self < <$to>::MIN as _ {
                    None
                } else {
                    Some(self as _)
                }
            }
        }

        impl TruncateTarget<$to> for $from {}
    )+)*};
}

impl_widen! {
    u8 => u8, u16, u32, u64, u128, usize;
    u16 => u16, u32, u64, u128, usize;
    u32 => u32, u64, u128;
    u64 => u64, u128;
    u128 => u128;
    usize => usize;

    i8 => i8, i16, i32, i64, i128, isize;
    i16 => i16, i32, i64, i128, isize;
    i32 => i32, i64, i128;
    i64 => i64, i128;
    i128 => i128;
    isize => isize;
}

impl_truncate! {
    u8, u16, u32, u64, u128, usize => u8;
    u16, u32, u64, u128, usize => u16;
    u32, u64, u128 => u32;
    u64, u128 => u64;
    u128 => u128;
    usize => usize;

    i8, i16, i32, i64, i128, isize => i8;
    i16, i32, i64, i128, isize => i16;
    i32, i64, i128 => i32;
    i64, i128 => i64;
    i128 => i128;
    isize => isize;
}
