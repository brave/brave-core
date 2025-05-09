//! Serde `Deserializer` module

use crate::de::deserialize_bool;
use crate::encoding::Decoder;
use crate::errors::serialize::DeError;
use crate::escape::unescape;
use serde::de::{DeserializeSeed, EnumAccess, VariantAccess, Visitor};
use serde::{self, forward_to_deserialize_any, serde_if_integer128};
use std::borrow::Cow;

/// A deserializer for a xml escaped and encoded value
///
/// # Note
///
/// Escaping the value is actually not always necessary, for instance
/// when converting to float, we don't expect any escapable character
/// anyway
#[derive(Clone, Debug)]
pub struct EscapedDeserializer<'a> {
    decoder: Decoder,
    /// Possible escaped value of text/CDATA or attribute value
    escaped_value: Cow<'a, [u8]>,
    /// If `true`, value requires unescaping before using
    escaped: bool,
}

impl<'a> EscapedDeserializer<'a> {
    pub fn new(escaped_value: Cow<'a, [u8]>, decoder: Decoder, escaped: bool) -> Self {
        EscapedDeserializer {
            decoder,
            escaped_value,
            escaped,
        }
    }
}

macro_rules! deserialize_num {
    ($method:ident, $visit:ident) => {
        fn $method<V>(self, visitor: V) -> Result<V::Value, Self::Error>
        where
            V: Visitor<'de>,
        {
            let value = self.decoder.decode(self.escaped_value.as_ref())?.parse()?;

            visitor.$visit(value)
        }
    };
}

impl<'de, 'a> serde::Deserializer<'de> for EscapedDeserializer<'a> {
    type Error = DeError;

    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    fn deserialize_str<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        let decoded = self.decoder.decode(&self.escaped_value)?;
        if self.escaped {
            match unescape(&decoded)? {
                Cow::Borrowed(s) => visitor.visit_str(s),
                Cow::Owned(s) => visitor.visit_string(s),
            }
        } else {
            match decoded {
                Cow::Borrowed(s) => visitor.visit_str(s),
                Cow::Owned(s) => visitor.visit_string(s),
            }
        }
    }

    /// Returns [`DeError::Unsupported`]
    fn deserialize_bytes<V>(self, _visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        Err(DeError::Unsupported(
            "binary data content is not supported by XML format".into(),
        ))
    }

    /// Forwards deserialization to the [`deserialize_bytes`](#method.deserialize_bytes)
    fn deserialize_byte_buf<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_bytes(visitor)
    }

    fn deserialize_string<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    fn deserialize_bool<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        deserialize_bool(self.escaped_value.as_ref(), self.decoder, visitor)
    }

    fn deserialize_char<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    fn deserialize_unit<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_unit()
    }

    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        if self.escaped_value.as_ref().is_empty() {
            visitor.visit_none()
        } else {
            visitor.visit_some(self)
        }
    }

    fn deserialize_enum<V>(
        self,
        _name: &str,
        _variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_enum(self)
    }

    fn deserialize_newtype_struct<V>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_newtype_struct(self)
    }

    deserialize_num!(deserialize_i64, visit_i64);
    deserialize_num!(deserialize_i32, visit_i32);
    deserialize_num!(deserialize_i16, visit_i16);
    deserialize_num!(deserialize_i8, visit_i8);
    deserialize_num!(deserialize_u64, visit_u64);
    deserialize_num!(deserialize_u32, visit_u32);
    deserialize_num!(deserialize_u16, visit_u16);
    deserialize_num!(deserialize_u8, visit_u8);
    deserialize_num!(deserialize_f64, visit_f64);
    deserialize_num!(deserialize_f32, visit_f32);

    serde_if_integer128! {
        deserialize_num!(deserialize_i128, visit_i128);
        deserialize_num!(deserialize_u128, visit_u128);
    }

    forward_to_deserialize_any! {
        unit_struct seq tuple tuple_struct map struct identifier ignored_any
    }
}

impl<'de, 'a> EnumAccess<'de> for EscapedDeserializer<'a> {
    type Error = DeError;
    type Variant = Self;

    fn variant_seed<V>(self, seed: V) -> Result<(V::Value, Self), Self::Error>
    where
        V: DeserializeSeed<'de>,
    {
        let name = seed.deserialize(self.clone())?;
        Ok((name, self))
    }
}

impl<'de, 'a> VariantAccess<'de> for EscapedDeserializer<'a> {
    type Error = DeError;

    fn unit_variant(self) -> Result<(), Self::Error> {
        Ok(())
    }

    fn newtype_variant_seed<T>(self, seed: T) -> Result<T::Value, Self::Error>
    where
        T: DeserializeSeed<'de>,
    {
        seed.deserialize(self)
    }

    fn tuple_variant<V>(self, _len: usize, _visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }

    fn struct_variant<V>(
        self,
        _fields: &'static [&'static str],
        _visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        unimplemented!()
    }
}
