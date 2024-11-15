use serde::de::IntoDeserializer;

use super::Error;

pub(crate) struct KeyDeserializer {
    span: Option<std::ops::Range<usize>>,
    key: crate::Key,
}

impl KeyDeserializer {
    pub(crate) fn new(key: crate::Key, span: Option<std::ops::Range<usize>>) -> Self {
        KeyDeserializer { span, key }
    }
}

impl<'de> IntoDeserializer<'de, Error> for KeyDeserializer {
    type Deserializer = Self;

    fn into_deserializer(self) -> Self::Deserializer {
        self
    }
}

impl<'de> serde::de::Deserializer<'de> for KeyDeserializer {
    type Error = Error;

    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Error>
    where
        V: serde::de::Visitor<'de>,
    {
        self.key.into_deserializer().deserialize_any(visitor)
    }

    fn deserialize_enum<V>(
        self,
        name: &str,
        variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: serde::de::Visitor<'de>,
    {
        let _ = name;
        let _ = variants;
        visitor.visit_enum(self)
    }

    fn deserialize_struct<V>(
        self,
        name: &'static str,
        fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Error>
    where
        V: serde::de::Visitor<'de>,
    {
        if serde_spanned::__unstable::is_spanned(name, fields) {
            if let Some(span) = self.span.clone() {
                return visitor.visit_map(super::SpannedDeserializer::new(self.key.get(), span));
            }
        }
        self.deserialize_any(visitor)
    }

    fn deserialize_newtype_struct<V>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, Error>
    where
        V: serde::de::Visitor<'de>,
    {
        visitor.visit_newtype_struct(self)
    }

    serde::forward_to_deserialize_any! {
        bool u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 char str string seq
        bytes byte_buf map option unit
        ignored_any unit_struct tuple_struct tuple identifier
    }
}

impl<'de> serde::de::EnumAccess<'de> for KeyDeserializer {
    type Error = Error;
    type Variant = UnitOnly<Self::Error>;

    fn variant_seed<T>(self, seed: T) -> Result<(T::Value, Self::Variant), Self::Error>
    where
        T: serde::de::DeserializeSeed<'de>,
    {
        seed.deserialize(self).map(unit_only)
    }
}

pub(crate) struct UnitOnly<E> {
    marker: std::marker::PhantomData<E>,
}

fn unit_only<T, E>(t: T) -> (T, UnitOnly<E>) {
    (
        t,
        UnitOnly {
            marker: std::marker::PhantomData,
        },
    )
}

impl<'de, E> serde::de::VariantAccess<'de> for UnitOnly<E>
where
    E: serde::de::Error,
{
    type Error = E;

    fn unit_variant(self) -> Result<(), Self::Error> {
        Ok(())
    }

    fn newtype_variant_seed<T>(self, _seed: T) -> Result<T::Value, Self::Error>
    where
        T: serde::de::DeserializeSeed<'de>,
    {
        Err(serde::de::Error::invalid_type(
            serde::de::Unexpected::UnitVariant,
            &"newtype variant",
        ))
    }

    fn tuple_variant<V>(self, _len: usize, _visitor: V) -> Result<V::Value, Self::Error>
    where
        V: serde::de::Visitor<'de>,
    {
        Err(serde::de::Error::invalid_type(
            serde::de::Unexpected::UnitVariant,
            &"tuple variant",
        ))
    }

    fn struct_variant<V>(
        self,
        _fields: &'static [&'static str],
        _visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: serde::de::Visitor<'de>,
    {
        Err(serde::de::Error::invalid_type(
            serde::de::Unexpected::UnitVariant,
            &"struct variant",
        ))
    }
}
