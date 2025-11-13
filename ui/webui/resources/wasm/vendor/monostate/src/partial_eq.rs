use crate::string::ConstStr;

impl<const V: char, const W: char> PartialEq<crate::MustBeChar<W>> for crate::MustBeChar<V> {
    fn eq(&self, _: &crate::MustBeChar<W>) -> bool {
        V == W
    }
}

impl<const V: u128, const W: u128> PartialEq<crate::MustBePosInt<W>> for crate::MustBePosInt<V> {
    fn eq(&self, _: &crate::MustBePosInt<W>) -> bool {
        V == W
    }
}

impl<const V: i128, const W: i128> PartialEq<crate::MustBeNegInt<W>> for crate::MustBeNegInt<V> {
    fn eq(&self, _: &crate::MustBeNegInt<W>) -> bool {
        V == W
    }
}

impl<const V: u8, const W: u8> PartialEq<crate::MustBeU8<W>> for crate::MustBeU8<V> {
    fn eq(&self, _: &crate::MustBeU8<W>) -> bool {
        V == W
    }
}

impl<const V: u16, const W: u16> PartialEq<crate::MustBeU16<W>> for crate::MustBeU16<V> {
    fn eq(&self, _: &crate::MustBeU16<W>) -> bool {
        V == W
    }
}

impl<const V: u32, const W: u32> PartialEq<crate::MustBeU32<W>> for crate::MustBeU32<V> {
    fn eq(&self, _: &crate::MustBeU32<W>) -> bool {
        V == W
    }
}

impl<const V: u64, const W: u64> PartialEq<crate::MustBeU64<W>> for crate::MustBeU64<V> {
    fn eq(&self, _: &crate::MustBeU64<W>) -> bool {
        V == W
    }
}

impl<const V: u128, const W: u128> PartialEq<crate::MustBeU128<W>> for crate::MustBeU128<V> {
    fn eq(&self, _: &crate::MustBeU128<W>) -> bool {
        V == W
    }
}

impl<const V: i8, const W: i8> PartialEq<crate::MustBeI8<W>> for crate::MustBeI8<V> {
    fn eq(&self, _: &crate::MustBeI8<W>) -> bool {
        V == W
    }
}

impl<const V: i16, const W: i16> PartialEq<crate::MustBeI16<W>> for crate::MustBeI16<V> {
    fn eq(&self, _: &crate::MustBeI16<W>) -> bool {
        V == W
    }
}

impl<const V: i32, const W: i32> PartialEq<crate::MustBeI32<W>> for crate::MustBeI32<V> {
    fn eq(&self, _: &crate::MustBeI32<W>) -> bool {
        V == W
    }
}

impl<const V: i64, const W: i64> PartialEq<crate::MustBeI64<W>> for crate::MustBeI64<V> {
    fn eq(&self, _: &crate::MustBeI64<W>) -> bool {
        V == W
    }
}

impl<const V: i128, const W: i128> PartialEq<crate::MustBeI128<W>> for crate::MustBeI128<V> {
    fn eq(&self, _: &crate::MustBeI128<W>) -> bool {
        V == W
    }
}

impl<const V: bool, const W: bool> PartialEq<crate::MustBeBool<W>> for crate::MustBeBool<V> {
    fn eq(&self, _: &crate::MustBeBool<W>) -> bool {
        V == W
    }
}

impl<V, W> PartialEq<crate::MustBeStr<W>> for crate::MustBeStr<V>
where
    V: ConstStr,
    W: ConstStr,
{
    fn eq(&self, _: &crate::MustBeStr<W>) -> bool {
        V::VALUE == W::VALUE
    }
}
