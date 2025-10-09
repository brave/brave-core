use crate::string::ConstStr;
use core::fmt::{Debug, Display};
use core::hash::Hash;
use core::panic::{RefUnwindSafe, UnwindSafe};
use serde::Serialize;

pub trait MustBe: Sealed {
    type Type: Copy
        + Debug
        + Default
        + Display
        + Hash
        + Ord
        + RefUnwindSafe
        + Send
        + Serialize
        + Sync
        + Unpin
        + UnwindSafe;
    const VALUE: Self::Type;
}

impl<const V: char> crate::MustBeChar<V> {
    pub const VALUE: char = V;
}

impl<const V: char> MustBe for crate::MustBeChar<V> {
    type Type = char;
    const VALUE: Self::Type = V;
}

impl<const V: u8> crate::MustBeU8<V> {
    pub const VALUE: u8 = V;
}

impl<const V: u8> MustBe for crate::MustBeU8<V> {
    type Type = u8;
    const VALUE: Self::Type = V;
}

impl<const V: u16> crate::MustBeU16<V> {
    pub const VALUE: u16 = V;
}

impl<const V: u16> MustBe for crate::MustBeU16<V> {
    type Type = u16;
    const VALUE: Self::Type = V;
}

impl<const V: u32> crate::MustBeU32<V> {
    pub const VALUE: u32 = V;
}

impl<const V: u32> MustBe for crate::MustBeU32<V> {
    type Type = u32;
    const VALUE: Self::Type = V;
}

impl<const V: u64> crate::MustBeU64<V> {
    pub const VALUE: u64 = V;
}

impl<const V: u64> MustBe for crate::MustBeU64<V> {
    type Type = u64;
    const VALUE: Self::Type = V;
}

impl<const V: u128> crate::MustBeU128<V> {
    pub const VALUE: u128 = V;
}

impl<const V: u128> MustBe for crate::MustBeU128<V> {
    type Type = u128;
    const VALUE: Self::Type = V;
}

impl<const V: i8> crate::MustBeI8<V> {
    pub const VALUE: i8 = V;
}

impl<const V: i8> MustBe for crate::MustBeI8<V> {
    type Type = i8;
    const VALUE: Self::Type = V;
}

impl<const V: i16> crate::MustBeI16<V> {
    pub const VALUE: i16 = V;
}

impl<const V: i16> MustBe for crate::MustBeI16<V> {
    type Type = i16;
    const VALUE: Self::Type = V;
}

impl<const V: i32> crate::MustBeI32<V> {
    pub const VALUE: i32 = V;
}

impl<const V: i32> MustBe for crate::MustBeI32<V> {
    type Type = i32;
    const VALUE: Self::Type = V;
}

impl<const V: i64> crate::MustBeI64<V> {
    pub const VALUE: i64 = V;
}

impl<const V: i64> MustBe for crate::MustBeI64<V> {
    type Type = i64;
    const VALUE: Self::Type = V;
}

impl<const V: i128> crate::MustBeI128<V> {
    pub const VALUE: i128 = V;
}

impl<const V: i128> MustBe for crate::MustBeI128<V> {
    type Type = i128;
    const VALUE: Self::Type = V;
}

impl<const V: bool> crate::MustBeBool<V> {
    pub const VALUE: bool = V;
}

impl<const V: bool> MustBe for crate::MustBeBool<V> {
    type Type = bool;
    const VALUE: Self::Type = V;
}

impl<V: ConstStr> crate::MustBeStr<V> {
    pub const VALUE: &'static str = V::VALUE;
}

impl<V: ConstStr> MustBe for crate::MustBeStr<V> {
    type Type = &'static str;
    const VALUE: Self::Type = V::VALUE;
}

pub trait Sealed {}
impl<const V: char> Sealed for crate::MustBeChar<V> {}
impl<const V: u8> Sealed for crate::MustBeU8<V> {}
impl<const V: u16> Sealed for crate::MustBeU16<V> {}
impl<const V: u32> Sealed for crate::MustBeU32<V> {}
impl<const V: u64> Sealed for crate::MustBeU64<V> {}
impl<const V: u128> Sealed for crate::MustBeU128<V> {}
impl<const V: i8> Sealed for crate::MustBeI8<V> {}
impl<const V: i16> Sealed for crate::MustBeI16<V> {}
impl<const V: i32> Sealed for crate::MustBeI32<V> {}
impl<const V: i64> Sealed for crate::MustBeI64<V> {}
impl<const V: i128> Sealed for crate::MustBeI128<V> {}
impl<const V: bool> Sealed for crate::MustBeBool<V> {}
impl<V: ConstStr> Sealed for crate::MustBeStr<V> {}
