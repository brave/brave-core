use crate::string::ConstStr;
use core::cmp::Ordering;

impl<const V: char, const W: char> PartialOrd<crate::MustBeChar<W>> for crate::MustBeChar<V> {
    fn partial_cmp(&self, _: &crate::MustBeChar<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: u128, const W: u128> PartialOrd<crate::MustBePosInt<W>> for crate::MustBePosInt<V> {
    fn partial_cmp(&self, _: &crate::MustBePosInt<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: i128, const W: i128> PartialOrd<crate::MustBeNegInt<W>> for crate::MustBeNegInt<V> {
    fn partial_cmp(&self, _: &crate::MustBeNegInt<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: u8, const W: u8> PartialOrd<crate::MustBeU8<W>> for crate::MustBeU8<V> {
    fn partial_cmp(&self, _: &crate::MustBeU8<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: u16, const W: u16> PartialOrd<crate::MustBeU16<W>> for crate::MustBeU16<V> {
    fn partial_cmp(&self, _: &crate::MustBeU16<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: u32, const W: u32> PartialOrd<crate::MustBeU32<W>> for crate::MustBeU32<V> {
    fn partial_cmp(&self, _: &crate::MustBeU32<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: u64, const W: u64> PartialOrd<crate::MustBeU64<W>> for crate::MustBeU64<V> {
    fn partial_cmp(&self, _: &crate::MustBeU64<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: u128, const W: u128> PartialOrd<crate::MustBeU128<W>> for crate::MustBeU128<V> {
    fn partial_cmp(&self, _: &crate::MustBeU128<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: i8, const W: i8> PartialOrd<crate::MustBeI8<W>> for crate::MustBeI8<V> {
    fn partial_cmp(&self, _: &crate::MustBeI8<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: i16, const W: i16> PartialOrd<crate::MustBeI16<W>> for crate::MustBeI16<V> {
    fn partial_cmp(&self, _: &crate::MustBeI16<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: i32, const W: i32> PartialOrd<crate::MustBeI32<W>> for crate::MustBeI32<V> {
    fn partial_cmp(&self, _: &crate::MustBeI32<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: i64, const W: i64> PartialOrd<crate::MustBeI64<W>> for crate::MustBeI64<V> {
    fn partial_cmp(&self, _: &crate::MustBeI64<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: i128, const W: i128> PartialOrd<crate::MustBeI128<W>> for crate::MustBeI128<V> {
    fn partial_cmp(&self, _: &crate::MustBeI128<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<const V: bool, const W: bool> PartialOrd<crate::MustBeBool<W>> for crate::MustBeBool<V> {
    fn partial_cmp(&self, _: &crate::MustBeBool<W>) -> Option<Ordering> {
        Some(V.cmp(&W))
    }
}

impl<V, W> PartialOrd<crate::MustBeStr<W>> for crate::MustBeStr<V>
where
    V: ConstStr,
    W: ConstStr,
{
    fn partial_cmp(&self, _: &crate::MustBeStr<W>) -> Option<Ordering> {
        Some(V::VALUE.cmp(W::VALUE))
    }
}
