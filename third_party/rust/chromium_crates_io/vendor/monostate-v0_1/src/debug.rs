use crate::string::ConstStr;
use core::fmt::{self, Debug};

impl<const V: char> Debug for crate::MustBeChar<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({:?})", V)
    }
}

impl<const V: u128> Debug for crate::MustBePosInt<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({})", V)
    }
}

impl<const V: i128> Debug for crate::MustBeNegInt<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({})", V)
    }
}

impl<const V: u8> Debug for crate::MustBeU8<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}u8)", V)
    }
}

impl<const V: u16> Debug for crate::MustBeU16<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}u16)", V)
    }
}

impl<const V: u32> Debug for crate::MustBeU32<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}u32)", V)
    }
}

impl<const V: u64> Debug for crate::MustBeU64<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}u64)", V)
    }
}

impl<const V: u128> Debug for crate::MustBeU128<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}u128)", V)
    }
}

impl<const V: i8> Debug for crate::MustBeI8<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}i8)", V)
    }
}

impl<const V: i16> Debug for crate::MustBeI16<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}i16)", V)
    }
}

impl<const V: i32> Debug for crate::MustBeI32<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}i32)", V)
    }
}

impl<const V: i64> Debug for crate::MustBeI64<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}i64)", V)
    }
}

impl<const V: i128> Debug for crate::MustBeI128<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({}i128)", V)
    }
}

impl<const V: bool> Debug for crate::MustBeBool<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({})", V)
    }
}

impl<V: ConstStr> Debug for crate::MustBeStr<V> {
    fn fmt(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
        write!(formatter, "MustBe!({:?})", V::VALUE)
    }
}
