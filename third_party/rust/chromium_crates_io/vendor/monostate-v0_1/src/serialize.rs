use crate::string::ConstStr;
use serde::{Serialize, Serializer};

impl<const V: char> Serialize for crate::MustBeChar<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_char(V)
    }
}

impl<const V: u8> Serialize for crate::MustBeU8<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_u8(V)
    }
}

impl<const V: u16> Serialize for crate::MustBeU16<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_u16(V)
    }
}

impl<const V: u32> Serialize for crate::MustBeU32<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_u32(V)
    }
}

impl<const V: u64> Serialize for crate::MustBeU64<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_u64(V)
    }
}

impl<const V: u128> Serialize for crate::MustBeU128<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_u128(V)
    }
}

impl<const V: i8> Serialize for crate::MustBeI8<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_i8(V)
    }
}

impl<const V: i16> Serialize for crate::MustBeI16<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_i16(V)
    }
}

impl<const V: i32> Serialize for crate::MustBeI32<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_i32(V)
    }
}

impl<const V: i64> Serialize for crate::MustBeI64<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_i64(V)
    }
}

impl<const V: i128> Serialize for crate::MustBeI128<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_i128(V)
    }
}

impl<const V: bool> Serialize for crate::MustBeBool<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_bool(V)
    }
}

impl<V: ConstStr> Serialize for crate::MustBeStr<V> {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_str(V::VALUE)
    }
}
