use crate::string::ConstStr;
use core::cmp::Ordering;

impl<const V: char> Ord for crate::MustBeChar<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: u128> Ord for crate::MustBePosInt<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: i128> Ord for crate::MustBeNegInt<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: u8> Ord for crate::MustBeU8<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: u16> Ord for crate::MustBeU16<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: u32> Ord for crate::MustBeU32<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: u64> Ord for crate::MustBeU64<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: u128> Ord for crate::MustBeU128<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: i8> Ord for crate::MustBeI8<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: i16> Ord for crate::MustBeI16<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: i32> Ord for crate::MustBeI32<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: i64> Ord for crate::MustBeI64<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: i128> Ord for crate::MustBeI128<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<const V: bool> Ord for crate::MustBeBool<V> {
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}

impl<V> Ord for crate::MustBeStr<V>
where
    V: ConstStr,
{
    fn cmp(&self, _: &Self) -> Ordering {
        Ordering::Equal
    }
}
