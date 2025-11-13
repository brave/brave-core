use crate::format;
use crate::string::ConstStr;
use core::fmt::{self, Write as _};
use core::str;
use serde::de::{Deserialize, Deserializer, Error, Unexpected, Visitor};

impl<'de, const V: char> Deserialize<'de> for crate::MustBeChar<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeCharVisitor(char);

        impl<'de> Visitor<'de> for MustBeCharVisitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "char {:?}", self.0)
            }

            fn visit_char<E>(self, v: char) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Char(v), &self))
                }
            }
        }

        deserializer
            .deserialize_char(MustBeCharVisitor(V))
            .map(|()| crate::MustBeChar)
    }
}

impl<'de, const V: u128> Deserialize<'de> for crate::MustBePosInt<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBePosIntVisitor(u128);

        impl<'de> Visitor<'de> for MustBePosIntVisitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}`", self.0)
            }

            fn visit_i64<E>(self, v: i64) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v >= 0 && v as u128 == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Signed(v), &self))
                }
            }

            fn visit_i128<E>(self, v: i128) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v >= 0 && v as u128 == self.0 {
                    Ok(())
                } else {
                    let mut buf = [0u8; 50];
                    let mut writer = format::Buf::new(&mut buf);
                    write!(writer, "integer `{}`", v).unwrap();
                    Err(Error::invalid_value(
                        Unexpected::Other(writer.as_str()),
                        &self,
                    ))
                }
            }

            fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v as u128 == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Unsigned(v), &self))
                }
            }

            fn visit_u128<E>(self, v: u128) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    let mut buf = [0u8; 49];
                    let mut writer = format::Buf::new(&mut buf);
                    write!(writer, "integer `{}`", v).unwrap();
                    Err(Error::invalid_value(
                        Unexpected::Other(writer.as_str()),
                        &self,
                    ))
                }
            }
        }

        deserializer
            .deserialize_any(MustBePosIntVisitor(V))
            .map(|()| crate::MustBePosInt)
    }
}

impl<'de, const V: i128> Deserialize<'de> for crate::MustBeNegInt<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeNegIntVisitor(i128);

        impl<'de> Visitor<'de> for MustBeNegIntVisitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}`", self.0)
            }

            fn visit_i64<E>(self, v: i64) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v as i128 == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Signed(v), &self))
                }
            }

            fn visit_i128<E>(self, v: i128) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    let mut buf = [0u8; 50];
                    let mut writer = format::Buf::new(&mut buf);
                    write!(writer, "integer `{}`", v).unwrap();
                    Err(Error::invalid_value(
                        Unexpected::Other(writer.as_str()),
                        &self,
                    ))
                }
            }

            fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v as i128 == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Unsigned(v), &self))
                }
            }

            fn visit_u128<E>(self, v: u128) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if self.0 >= 0 && v == self.0 as u128 {
                    Ok(())
                } else {
                    let mut buf = [0u8; 49];
                    let mut writer = format::Buf::new(&mut buf);
                    write!(writer, "integer `{}`", v).unwrap();
                    Err(Error::invalid_value(
                        Unexpected::Other(writer.as_str()),
                        &self,
                    ))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeNegIntVisitor(V))
            .map(|()| crate::MustBeNegInt)
    }
}

impl<'de, const V: u8> Deserialize<'de> for crate::MustBeU8<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeU8Visitor(u8);

        impl<'de> Visitor<'de> for MustBeU8Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as u8", self.0)
            }

            fn visit_u8<E>(self, v: u8) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Unsigned(v as u64), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeU8Visitor(V))
            .map(|()| crate::MustBeU8)
    }
}

impl<'de, const V: u16> Deserialize<'de> for crate::MustBeU16<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeU16Visitor(u16);

        impl<'de> Visitor<'de> for MustBeU16Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as u16", self.0)
            }

            fn visit_u16<E>(self, v: u16) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Unsigned(v as u64), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeU16Visitor(V))
            .map(|()| crate::MustBeU16)
    }
}

impl<'de, const V: u32> Deserialize<'de> for crate::MustBeU32<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeU32Visitor(u32);

        impl<'de> Visitor<'de> for MustBeU32Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as u32", self.0)
            }

            fn visit_u32<E>(self, v: u32) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Unsigned(v as u64), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeU32Visitor(V))
            .map(|()| crate::MustBeU32)
    }
}

impl<'de, const V: u64> Deserialize<'de> for crate::MustBeU64<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeU64Visitor(u64);

        impl<'de> Visitor<'de> for MustBeU64Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as u64", self.0)
            }

            fn visit_u8<E>(self, v: u8) -> Result<Self::Value, E>
            where
                E: Error,
            {
                Err(E::invalid_type(Unexpected::Unsigned(v as u64), &self))
            }

            fn visit_u16<E>(self, v: u16) -> Result<Self::Value, E>
            where
                E: Error,
            {
                Err(E::invalid_type(Unexpected::Unsigned(v as u64), &self))
            }

            fn visit_u32<E>(self, v: u32) -> Result<Self::Value, E>
            where
                E: Error,
            {
                Err(E::invalid_type(Unexpected::Unsigned(v as u64), &self))
            }

            fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Unsigned(v), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeU64Visitor(V))
            .map(|()| crate::MustBeU64)
    }
}

impl<'de, const V: u128> Deserialize<'de> for crate::MustBeU128<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeU128Visitor(u128);

        impl<'de> Visitor<'de> for MustBeU128Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as u128", self.0)
            }

            fn visit_u128<E>(self, v: u128) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    let mut buf = [0u8; 49];
                    let mut writer = format::Buf::new(&mut buf);
                    write!(writer, "integer `{}`", v).unwrap();
                    Err(Error::invalid_value(
                        Unexpected::Other(writer.as_str()),
                        &self,
                    ))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeU128Visitor(V))
            .map(|()| crate::MustBeU128)
    }
}

impl<'de, const V: i8> Deserialize<'de> for crate::MustBeI8<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeI8Visitor(i8);

        impl<'de> Visitor<'de> for MustBeI8Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as i8", self.0)
            }

            fn visit_i8<E>(self, v: i8) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Signed(v as i64), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeI8Visitor(V))
            .map(|()| crate::MustBeI8)
    }
}

impl<'de, const V: i16> Deserialize<'de> for crate::MustBeI16<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeI16Visitor(i16);

        impl<'de> Visitor<'de> for MustBeI16Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as i16", self.0)
            }

            fn visit_i16<E>(self, v: i16) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Signed(v as i64), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeI16Visitor(V))
            .map(|()| crate::MustBeI16)
    }
}

impl<'de, const V: i32> Deserialize<'de> for crate::MustBeI32<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeI32Visitor(i32);

        impl<'de> Visitor<'de> for MustBeI32Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as i32", self.0)
            }

            fn visit_i32<E>(self, v: i32) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Signed(v as i64), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeI32Visitor(V))
            .map(|()| crate::MustBeI32)
    }
}

impl<'de, const V: i64> Deserialize<'de> for crate::MustBeI64<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeI64Visitor(i64);

        impl<'de> Visitor<'de> for MustBeI64Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as i64", self.0)
            }

            fn visit_i8<E>(self, v: i8) -> Result<Self::Value, E>
            where
                E: Error,
            {
                Err(E::invalid_type(Unexpected::Signed(v as i64), &self))
            }

            fn visit_i16<E>(self, v: i16) -> Result<Self::Value, E>
            where
                E: Error,
            {
                Err(E::invalid_type(Unexpected::Signed(v as i64), &self))
            }

            fn visit_i32<E>(self, v: i32) -> Result<Self::Value, E>
            where
                E: Error,
            {
                Err(E::invalid_type(Unexpected::Signed(v as i64), &self))
            }

            fn visit_i64<E>(self, v: i64) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Signed(v), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeI64Visitor(V))
            .map(|()| crate::MustBeI64)
    }
}

impl<'de, const V: i128> Deserialize<'de> for crate::MustBeI128<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeI128Visitor(i128);

        impl<'de> Visitor<'de> for MustBeI128Visitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "integer `{}` as i128", self.0)
            }

            fn visit_i128<E>(self, v: i128) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    let mut buf = [0u8; 50];
                    let mut writer = format::Buf::new(&mut buf);
                    write!(writer, "integer `{}`", v).unwrap();
                    Err(Error::invalid_value(
                        Unexpected::Other(writer.as_str()),
                        &self,
                    ))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeI128Visitor(V))
            .map(|()| crate::MustBeI128)
    }
}

impl<'de, const V: bool> Deserialize<'de> for crate::MustBeBool<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeBoolVisitor(bool);

        impl<'de> Visitor<'de> for MustBeBoolVisitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "boolean `{}`", self.0)
            }

            fn visit_bool<E>(self, v: bool) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Bool(v), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeBoolVisitor(V))
            .map(|()| crate::MustBeBool)
    }
}

impl<'de, V: ConstStr> Deserialize<'de> for crate::MustBeStr<V> {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        struct MustBeStrVisitor(&'static str);

        impl<'de> Visitor<'de> for MustBeStrVisitor {
            type Value = ();

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                write!(formatter, "string {:?}", self.0)
            }

            fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
            where
                E: Error,
            {
                if v == self.0 {
                    Ok(())
                } else {
                    Err(E::invalid_value(Unexpected::Str(v), &self))
                }
            }
        }

        deserializer
            .deserialize_any(MustBeStrVisitor(V::VALUE))
            .map(|()| crate::MustBeStr)
    }
}
