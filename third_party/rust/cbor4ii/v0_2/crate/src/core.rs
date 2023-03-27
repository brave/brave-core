//! core module

pub mod types;
pub mod enc;
pub mod dec;
pub mod utils;

#[cfg(feature = "use_alloc")]
use crate::alloc::{ vec::Vec, boxed::Box, string::String };


/// Major type
pub mod major {
    pub const UNSIGNED: u8 = 0;
    pub const NEGATIVE: u8 = 1;
    pub const BYTES:    u8 = 2;
    pub const STRING:   u8 = 3;
    pub const ARRAY:    u8 = 4;
    pub const MAP:      u8 = 5;
    pub const TAG:      u8 = 6;
    pub const SIMPLE:   u8 = 7;
}

pub(crate) mod marker {
    pub const START: u8 = 0x1f;
    pub const FALSE: u8 = 0xf4; // simple(20)
    pub const TRUE: u8  = 0xf5; // simple(21)
    pub const NULL: u8  = 0xf6; // simple(22)
    pub const UNDEFINED: u8 = 0xf7; // simple(23)
    pub const F16: u8   = 0xf9;
    pub const F32: u8   = 0xfa;
    pub const F64: u8   = 0xfb;
    pub const BREAK: u8   = 0xff;
}


#[cfg(feature = "use_alloc")]
#[derive(Debug, PartialEq)]
#[non_exhaustive]
pub enum Value {
    Null,
    Bool(bool),
    Integer(i128),
    Float(f64),
    Bytes(Vec<u8>),
    Text(String),
    Array(Vec<Value>),
    Map(Vec<(Value, Value)>),
    Tag(u64, Box<Value>)
}

#[cfg(feature = "use_alloc")]
impl enc::Encode for Value {
    fn encode<W: enc::Write>(&self, writer: &mut W) -> Result<(), enc::Error<W::Error>> {
        match self {
            Value::Null => types::Null.encode(writer),
            Value::Bool(v) => v.encode(writer),
            Value::Integer(v) => v.encode(writer),
            Value::Float(v) => v.encode(writer),
            Value::Bytes(v) => types::Bytes(v.as_slice()).encode(writer),
            Value::Text(v) => v.as_str().encode(writer),
            Value::Array(v) => v.as_slice().encode(writer),
            Value::Map(v) => types::Map(v.as_slice()).encode(writer),
            Value::Tag(tag, v) => types::Tag(*tag, &**v).encode(writer)
        }
    }
}

#[cfg(feature = "use_alloc")]
impl<'de> dec::Decode<'de> for Value {
    fn decode_with<R: dec::Read<'de>>(byte: u8, reader: &mut R) -> Result<Self, dec::Error<R::Error>> {
        use crate::util::ScopeGuard;

        if !reader.step_in() {
            return Err(dec::Error::DepthLimit);
        }

        let mut reader = ScopeGuard(reader, |reader| reader.step_out());
        let reader = &mut *reader;

        match byte >> 5 {
            major::UNSIGNED => u64::decode_with(byte, reader)
                .map(|i| Value::Integer(i.into())),
            major::NEGATIVE => {
                let types::Negative(v) = <types::Negative<u64>>::decode_with(byte, reader)?;
                let v = i128::from(v);
                let v = v.checked_add(1)
                    .ok_or(dec::Error::Overflow { name: "Value::Integer" })?;
                Ok(Value::Integer(-v))
            },
            major::BYTES => <types::Bytes<Vec<u8>>>::decode_with(byte, reader)
                .map(|buf| Value::Bytes(buf.0)),
            major::STRING => String::decode_with(byte, reader)
                .map(Value::Text),
            major::ARRAY => <Vec<Value>>::decode_with(byte, reader)
                .map(Value::Array),
            major::MAP => <types::Map<Vec<(Value, Value)>>>::decode_with(byte, reader)
                .map(|map| Value::Map(map.0)),
            major::TAG => {
                let tag = <types::Tag<Value>>::decode_with(byte, reader)?;
                Ok(Value::Tag(tag.0, Box::new(tag.1)))
            },
            major::SIMPLE => match byte {
                marker::FALSE => Ok(Value::Bool(false)),
                marker::TRUE => Ok(Value::Bool(true)),
                marker::NULL | marker::UNDEFINED => Ok(Value::Null),
                #[cfg(feature = "half-f16")]
                marker::F16 => {
                    let v = half::f16::decode_with(byte, reader)?;
                    Ok(Value::Float(v.into()))
                },
                marker::F32 => f32::decode_with(byte, reader)
                    .map(|v| Value::Float(v.into())),
                marker::F64 => f64::decode_with(byte, reader)
                    .map(Value::Float),
                _ => Err(dec::Error::Unsupported { byte })
            },
            _ => Err(dec::Error::Unsupported { byte })
        }
    }
}

#[cfg(feature = "serde1")]
impl serde::Serialize for Value {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer
    {
        use serde::ser::{ Error, SerializeMap, SerializeSeq };

        match self {
            Value::Null => serializer.serialize_none(),
            Value::Bool(v) => serializer.serialize_bool(*v),
            Value::Integer(v) => serializer.serialize_i128(*v),
            Value::Float(v) => serializer.serialize_f64(*v),
            Value::Bytes(v) => serializer.serialize_bytes(v),
            Value::Text(v) => serializer.serialize_str(v),
            Value::Array(v) => {
                let mut seq = serializer.serialize_seq(Some(v.len()))?;
                for value in v.iter() {
                    seq.serialize_element(value)?;
                }
                seq.end()
            },
            Value::Map(v) => {
                let mut map = serializer.serialize_map(Some(v.len()))?;
                for (k, v) in v.iter() {
                    map.serialize_entry(k, v)?;
                }
                map.end()
            },
            Value::Tag(..) =>
                Err(S::Error::custom("serialization Tag via serde is not supported"))
        }
    }
}

#[cfg(feature = "serde1")]
impl<'de> serde::Deserialize<'de> for Value {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>
    {
        use serde::de::{ Error, Visitor, SeqAccess, MapAccess };

        struct ValueVisitor;

        impl<'de> Visitor<'de> for ValueVisitor {
            type Value = Value;

            fn expecting(&self, formatter: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
                formatter.write_str("invalid input or unsupported type")
            }

            #[inline]
            fn visit_bool<E>(self, v: bool) -> Result<Self::Value, E>
            where E: Error
            {
                Ok(Value::Bool(v))
            }

            #[inline]
            fn visit_i64<E>(self, v: i64) -> Result<Self::Value, E>
            where E: Error
            {
                Ok(Value::Integer(v.into()))
            }

            #[inline]
            fn visit_i128<E>(self, v: i128) -> Result<Self::Value, E>
            where E: Error
            {
                Ok(Value::Integer(v))
            }

            #[inline]
            fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
            where E: Error,
            {
                Ok(Value::Integer(v.into()))
            }

            #[inline]
            fn visit_u128<E>(self, v: u128) -> Result<Self::Value, E>
            where E: Error,
            {
                use core::convert::TryFrom;

                let v = i128::try_from(v).map_err(E::custom)?;
                Ok(Value::Integer(v))
            }

            #[inline]
            fn visit_f64<E>(self, v: f64) -> Result<Self::Value, E>
            where E: Error,
            {
                Ok(Value::Float(v))
            }

            #[inline]
            fn visit_char<E>(self, v: char) -> Result<Self::Value, E>
            where E: Error,
            {
                Ok(Value::Text(v.into()))
            }

            #[inline]
            fn visit_str<E>(self, v: &str) -> Result<Self::Value, E>
            where E: Error,
            {
                Ok(Value::Text(v.into()))
            }

            #[inline]
            fn visit_string<E>(self, v: String) -> Result<Self::Value, E>
            where E: Error,
            {
                Ok(Value::Text(v))
            }

            #[inline]
            fn visit_bytes<E>(self, v: &[u8]) -> Result<Self::Value, E>
            where E: Error,
            {
                Ok(Value::Bytes(v.into()))
            }

            #[inline]
            fn visit_byte_buf<E>(self, v: Vec<u8>) -> Result<Self::Value, E>
            where E: Error,
            {
                Ok(Value::Bytes(v))
            }

            #[inline]
            fn visit_none<E>(self) -> Result<Self::Value, E>
            where E: Error,
            {
                Ok(Value::Null)
            }

            #[inline]
            fn visit_unit<E>(self) -> Result<Self::Value, E>
            where E: Error,
            {
                self.visit_none()
            }

            #[inline]
            fn visit_seq<A>(self, mut seq: A) -> Result<Self::Value, A::Error>
            where A: SeqAccess<'de>,
            {
                let mut list = seq.size_hint()
                    .map(|n| Vec::with_capacity(core::cmp::min(n, 256)))
                    .unwrap_or_else(Vec::new);

                while let Some(v) = seq.next_element()? {
                    list.push(v);
                }

                Ok(Value::Array(list))
            }

            #[inline]
            fn visit_map<A>(self, mut map: A) -> Result<Self::Value, A::Error>
            where A: MapAccess<'de>,
            {
                let mut list = map.size_hint()
                    .map(|n| Vec::with_capacity(core::cmp::min(n, 256)))
                    .unwrap_or_else(Vec::new);

                while let Some((k, v)) = map.next_entry()? {
                    list.push((k, v));
                }

                Ok(Value::Map(list))
            }
        }

        deserializer.deserialize_any(ValueVisitor)
    }
}
