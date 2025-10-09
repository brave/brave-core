use crate::string::ConstStr;
use core::hash::{Hash, Hasher};

impl<const V: char> Hash for crate::MustBeChar<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: u128> Hash for crate::MustBePosInt<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: i128> Hash for crate::MustBeNegInt<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: u8> Hash for crate::MustBeU8<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: u16> Hash for crate::MustBeU16<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: u32> Hash for crate::MustBeU32<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: u64> Hash for crate::MustBeU64<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: u128> Hash for crate::MustBeU128<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: i8> Hash for crate::MustBeI8<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: i16> Hash for crate::MustBeI16<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: i32> Hash for crate::MustBeI32<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: i64> Hash for crate::MustBeI64<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: i128> Hash for crate::MustBeI128<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<const V: bool> Hash for crate::MustBeBool<V> {
    fn hash<H: Hasher>(&self, _: &mut H) {}
}

impl<V> Hash for crate::MustBeStr<V>
where
    V: ConstStr,
{
    fn hash<H: Hasher>(&self, _: &mut H) {}
}
