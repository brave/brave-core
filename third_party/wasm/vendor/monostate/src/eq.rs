use crate::string::ConstStr;

impl<const V: char> Eq for crate::MustBeChar<V> {}
impl<const V: u128> Eq for crate::MustBePosInt<V> {}
impl<const V: i128> Eq for crate::MustBeNegInt<V> {}
impl<const V: u8> Eq for crate::MustBeU8<V> {}
impl<const V: u16> Eq for crate::MustBeU16<V> {}
impl<const V: u32> Eq for crate::MustBeU32<V> {}
impl<const V: u64> Eq for crate::MustBeU64<V> {}
impl<const V: u128> Eq for crate::MustBeU128<V> {}
impl<const V: i8> Eq for crate::MustBeI8<V> {}
impl<const V: i16> Eq for crate::MustBeI16<V> {}
impl<const V: i32> Eq for crate::MustBeI32<V> {}
impl<const V: i64> Eq for crate::MustBeI64<V> {}
impl<const V: i128> Eq for crate::MustBeI128<V> {}
impl<const V: bool> Eq for crate::MustBeBool<V> {}
impl<V: ConstStr> Eq for crate::MustBeStr<V> {}
