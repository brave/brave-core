use crate::string::ConstStr;

impl<const V: char> Default for crate::MustBeChar<V> {
    fn default() -> Self {
        crate::MustBeChar::<V>
    }
}

impl<const V: u128> Default for crate::MustBePosInt<V> {
    fn default() -> Self {
        crate::MustBePosInt::<V>
    }
}

impl<const V: i128> Default for crate::MustBeNegInt<V> {
    fn default() -> Self {
        crate::MustBeNegInt::<V>
    }
}

impl<const V: u8> Default for crate::MustBeU8<V> {
    fn default() -> Self {
        crate::MustBeU8::<V>
    }
}

impl<const V: u16> Default for crate::MustBeU16<V> {
    fn default() -> Self {
        crate::MustBeU16::<V>
    }
}

impl<const V: u32> Default for crate::MustBeU32<V> {
    fn default() -> Self {
        crate::MustBeU32::<V>
    }
}

impl<const V: u64> Default for crate::MustBeU64<V> {
    fn default() -> Self {
        crate::MustBeU64::<V>
    }
}

impl<const V: u128> Default for crate::MustBeU128<V> {
    fn default() -> Self {
        crate::MustBeU128::<V>
    }
}

impl<const V: i8> Default for crate::MustBeI8<V> {
    fn default() -> Self {
        crate::MustBeI8::<V>
    }
}

impl<const V: i16> Default for crate::MustBeI16<V> {
    fn default() -> Self {
        crate::MustBeI16::<V>
    }
}

impl<const V: i32> Default for crate::MustBeI32<V> {
    fn default() -> Self {
        crate::MustBeI32::<V>
    }
}

impl<const V: i64> Default for crate::MustBeI64<V> {
    fn default() -> Self {
        crate::MustBeI64::<V>
    }
}

impl<const V: i128> Default for crate::MustBeI128<V> {
    fn default() -> Self {
        crate::MustBeI128::<V>
    }
}

impl<const V: bool> Default for crate::MustBeBool<V> {
    fn default() -> Self {
        crate::MustBeBool::<V>
    }
}

impl<V> Default for crate::MustBeStr<V>
where
    V: ConstStr,
{
    fn default() -> Self {
        crate::MustBeStr::<V>
    }
}
