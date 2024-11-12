use alloc::{borrow::ToOwned, collections::BTreeMap, format, string::String, vec::Vec};
use core::{convert::TryFrom, fmt};

use cid::serde::{BytesToCidVisitor, CID_SERDE_PRIVATE_IDENTIFIER};
use cid::Cid;
use serde::{
    de::{self, IntoDeserializer},
    forward_to_deserialize_any, Deserialize,
};

use crate::{ipld::Ipld, serde::SerdeError};

/// Deserialize instances of [`crate::ipld::Ipld`].
///
/// # Example
///
/// ```
/// use std::collections::BTreeMap;
///
/// use serde_derive::Deserialize;
/// use ipld_core::ipld::Ipld;
/// use ipld_core::serde::from_ipld;
///
/// #[derive(Deserialize)]
/// struct Person {
///     name: String,
///     age: u8,
///     hobbies: Vec<String>,
///     is_cool: bool,
/// }
///
/// let ipld = Ipld::Map({
///     BTreeMap::from([
///         ("name".into(), Ipld::String("Hello World!".into())),
///         ("age".into(), Ipld::Integer(52)),
///         (
///             "hobbies".into(),
///             Ipld::List(vec![
///                 Ipld::String("geography".into()),
///                 Ipld::String("programming".into()),
///             ]),
///         ),
///         ("is_cool".into(), Ipld::Bool(true)),
///     ])
/// });
///
/// let person = from_ipld(ipld);
/// assert!(matches!(person, Ok(Person { .. })));
/// ```
// NOTE vmx 2021-12-22: Taking by value is also what `serde_json` does.
pub fn from_ipld<T>(value: Ipld) -> Result<T, SerdeError>
where
    T: serde::de::DeserializeOwned,
{
    T::deserialize(value)
}

impl<'de> de::Deserialize<'de> for Ipld {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        struct IpldVisitor;

        impl<'de> de::Visitor<'de> for IpldVisitor {
            type Value = Ipld;

            fn expecting(&self, fmt: &mut fmt::Formatter<'_>) -> fmt::Result {
                fmt.write_str("any valid IPLD kind")
            }

            #[inline]
            fn visit_str<E>(self, value: &str) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                Ok(Ipld::String(String::from(value)))
            }

            #[inline]
            fn visit_bytes<E>(self, v: &[u8]) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                self.visit_byte_buf(v.to_owned())
            }

            #[inline]
            fn visit_byte_buf<E>(self, v: Vec<u8>) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                Ok(Ipld::Bytes(v))
            }

            #[inline]
            fn visit_u64<E>(self, v: u64) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                Ok(Ipld::Integer(v.into()))
            }

            #[inline]
            fn visit_i64<E>(self, v: i64) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                Ok(Ipld::Integer(v.into()))
            }

            #[inline]
            fn visit_i128<E>(self, v: i128) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                Ok(Ipld::Integer(v))
            }

            #[inline]
            fn visit_f64<E>(self, v: f64) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                Ok(Ipld::Float(v))
            }

            #[inline]
            fn visit_bool<E>(self, v: bool) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                Ok(Ipld::Bool(v))
            }

            #[inline]
            fn visit_none<E>(self) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                Ok(Ipld::Null)
            }

            #[inline]
            fn visit_some<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
            where
                D: de::Deserializer<'de>,
            {
                Deserialize::deserialize(deserializer)
            }

            #[inline]
            fn visit_unit<E>(self) -> Result<Self::Value, E>
            where
                E: de::Error,
            {
                Ok(Ipld::Null)
            }

            #[inline]
            fn visit_seq<V>(self, mut visitor: V) -> Result<Self::Value, V::Error>
            where
                V: de::SeqAccess<'de>,
            {
                let mut vec = Vec::with_capacity(visitor.size_hint().unwrap_or(0));

                while let Some(elem) = visitor.next_element()? {
                    vec.push(elem);
                }

                Ok(Ipld::List(vec))
            }

            #[inline]
            fn visit_map<V>(self, mut visitor: V) -> Result<Self::Value, V::Error>
            where
                V: de::MapAccess<'de>,
            {
                let mut values = BTreeMap::new();

                while let Some((key, value)) = visitor.next_entry()? {
                    let prev_value = values.insert(key, value);
                    if prev_value.is_some() {
                        return Err(de::Error::custom("Duplicate map key"));
                    }
                }

                Ok(Ipld::Map(values))
            }

            /// Newtype structs are only used to deserialize CIDs.
            #[inline]
            fn visit_newtype_struct<D>(self, deserializer: D) -> Result<Self::Value, D::Error>
            where
                D: de::Deserializer<'de>,
            {
                deserializer
                    .deserialize_bytes(BytesToCidVisitor)
                    .map(Ipld::Link)
            }
        }

        deserializer.deserialize_any(IpldVisitor)
    }
}

macro_rules! impl_deserialize_integer {
    ($ty:ident, $deserialize:ident, $visit:ident) => {
        fn $deserialize<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
            match self {
                Self::Integer(integer) => match $ty::try_from(integer) {
                    Ok(int) => visitor.$visit(int),
                    Err(_) => error(format!(
                        "`Ipld::Integer` value was bigger than `{}`",
                        stringify!($ty)
                    )),
                },
                _ => error(format!(
                    "Only `Ipld::Integer` can be deserialized to `{}`, input was `{:#?}`",
                    stringify!($ty),
                    self
                )),
            }
        }
    };
}

/// A Deserializer for CIDs.
///
/// A separate deserializer is needed to make sure we always deserialize only CIDs as `Ipld::Link`
/// and don't deserialize arbitrary bytes.
struct CidDeserializer(Cid);

impl<'de> de::Deserializer<'de> for CidDeserializer {
    type Error = SerdeError;

    #[inline]
    fn deserialize_any<V: de::Visitor<'de>>(self, _visitor: V) -> Result<V::Value, Self::Error> {
        error("Only bytes can be deserialized into a CID")
    }

    fn deserialize_bytes<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        visitor.visit_bytes(&self.0.to_bytes())
    }

    forward_to_deserialize_any! {
        bool byte_buf char enum f32 f64  i8 i16 i32 i64 identifier ignored_any map newtype_struct
        option seq str string struct tuple tuple_struct  u8 u16 u32 u64 unit unit_struct
    }
}

/// Deserialize from an [`Ipld`] enum into a Rust type.
///
/// The deserialization will return an error if you try to deserialize into an integer type that
/// would be too small to hold the value stored in [`Ipld::Integer`].
///
/// [`Ipld::Floats`] can be converted to `f32` if there is no of precision, else it will error.
impl<'de> de::Deserializer<'de> for Ipld {
    type Error = SerdeError;

    #[inline]
    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: de::Visitor<'de>,
    {
        match self {
            Self::Null => visitor.visit_none(),
            Self::Bool(bool) => visitor.visit_bool(bool),
            Self::Integer(i128) => visitor.visit_i128(i128),
            Self::Float(f64) => visitor.visit_f64(f64),
            Self::String(string) => visitor.visit_str(&string),
            Self::Bytes(bytes) => visitor.visit_bytes(&bytes),
            Self::List(list) => visit_seq(list, visitor),
            Self::Map(map) => visit_map(map, visitor),
            Self::Link(cid) => visitor.visit_newtype_struct(CidDeserializer(cid)),
        }
    }

    fn deserialize_unit<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::Null => visitor.visit_unit(),
            _ => error(format!(
                "Only `Ipld::Null` can be deserialized to unit, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_bool<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::Bool(bool) => visitor.visit_bool(bool),
            _ => error(format!(
                "Only `Ipld::Bool` can be deserialized to bool, input was `{:#?}`",
                self
            )),
        }
    }

    impl_deserialize_integer!(i8, deserialize_i8, visit_i8);
    impl_deserialize_integer!(i16, deserialize_i16, visit_i16);
    impl_deserialize_integer!(i32, deserialize_i32, visit_i32);
    impl_deserialize_integer!(i64, deserialize_i64, visit_i64);

    impl_deserialize_integer!(u8, deserialize_u8, visit_u8);
    impl_deserialize_integer!(u16, deserialize_u16, visit_u16);
    impl_deserialize_integer!(u32, deserialize_u32, visit_u32);
    impl_deserialize_integer!(u64, deserialize_u64, visit_u64);

    fn deserialize_f32<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::Float(float) => {
                if !float.is_finite() {
                    error(format!("`Ipld::Float` must be a finite number, not infinity or NaN, input was `{}`", float))
                } else if (float as f32) as f64 != float {
                    error(
                        "`Ipld::Float` cannot be deserialized to `f32`, without loss of precision`",
                    )
                } else {
                    visitor.visit_f32(float as f32)
                }
            }
            _ => error(format!(
                "Only `Ipld::Float` can be deserialized to `f32`, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_f64<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::Float(float) => {
                if float.is_finite() {
                    visitor.visit_f64(float)
                } else {
                    error(format!("`Ipld::Float` must be a finite number, not infinity or NaN, input was `{}`", float))
                }
            }
            _ => error(format!(
                "Only `Ipld::Float` can be deserialized to `f64`, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_char<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::String(string) => {
                if string.chars().count() == 1 {
                    visitor.visit_char(string.chars().next().unwrap())
                } else {
                    error("`Ipld::String` was longer than a single character")
                }
            }
            _ => error(format!(
                "Only `Ipld::String` can be deserialized to string, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_str<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::String(string) => visitor.visit_str(&string),
            _ => error(format!(
                "Only `Ipld::String` can be deserialized to string, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_string<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::String(string) => visitor.visit_string(string),
            _ => error(format!(
                "Only `Ipld::String` can be deserialized to string, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_bytes<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::Bytes(bytes) => visitor.visit_bytes(&bytes),
            _ => error(format!(
                "Only `Ipld::Bytes` can be deserialized to bytes, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_byte_buf<V: de::Visitor<'de>>(
        self,
        visitor: V,
    ) -> Result<V::Value, Self::Error> {
        match self {
            Self::Bytes(bytes) => visitor.visit_byte_buf(bytes),
            _ => error(format!(
                "Only `Ipld::Bytes` can be deserialized to bytes, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_seq<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::List(list) => visit_seq(list, visitor),
            _ => error(format!(
                "Only `Ipld::List` can be deserialized to sequence, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_tuple<V: de::Visitor<'de>>(
        self,
        len: usize,
        visitor: V,
    ) -> Result<V::Value, Self::Error> {
        match self {
            Self::List(list) => {
                if len == list.len() {
                    visit_seq(list, visitor)
                } else {
                    error(format!("The tuple size must match the length of the `Ipld::List`, tuple size: {}, `Ipld::List` length: {}", len, list.len()))
                }
            }
            _ => error(format!(
                "Only `Ipld::List` can be deserialized to tuple, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_tuple_struct<V: de::Visitor<'de>>(
        self,
        _name: &str,
        len: usize,
        visitor: V,
    ) -> Result<V::Value, Self::Error> {
        self.deserialize_tuple(len, visitor)
    }

    fn deserialize_map<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::Map(map) => visit_map(map, visitor),
            _ => error(format!(
                "Only `Ipld::Map` can be deserialized to map, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_identifier<V: de::Visitor<'de>>(
        self,
        visitor: V,
    ) -> Result<V::Value, Self::Error> {
        match self {
            Self::String(string) => visitor.visit_str(&string),
            _ => error(format!(
                "Only `Ipld::String` can be deserialized to identifier, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_struct<V: de::Visitor<'de>>(
        self,
        _name: &str,
        _fields: &[&str],
        visitor: V,
    ) -> Result<V::Value, Self::Error> {
        match self {
            Self::Map(map) => visit_map(map, visitor),
            _ => error(format!(
                "Only `Ipld::Map` can be deserialized to struct, input was `{:#?}`",
                self
            )),
        }
    }

    fn deserialize_unit_struct<V: de::Visitor<'de>>(
        self,
        _name: &str,
        _visitor: V,
    ) -> Result<V::Value, Self::Error> {
        error("Unit struct cannot be deserialized")
    }

    fn deserialize_newtype_struct<V: de::Visitor<'de>>(
        self,
        name: &str,
        visitor: V,
    ) -> Result<V::Value, Self::Error> {
        if name == CID_SERDE_PRIVATE_IDENTIFIER {
            match self {
                Ipld::Link(cid) => visitor.visit_newtype_struct(CidDeserializer(cid)),
                _ => error(format!(
                    "Only `Ipld::Link`s can be deserialized to CIDs, input was `{:#?}`",
                    self
                )),
            }
        } else {
            visitor.visit_newtype_struct(self)
        }
    }

    // Heavily based on
    // https://github.com/serde-rs/json/blob/95f67a09399d546d9ecadeb747a845a77ff309b2/src/value/de.rs#L249
    fn deserialize_enum<V: de::Visitor<'de>>(
        self,
        _name: &str,
        _variants: &[&str],
        visitor: V,
    ) -> Result<V::Value, Self::Error> {
        let (variant, value) = match self {
            Ipld::Map(map) => {
                let mut iter = map.into_iter();
                let (variant, value) = match iter.next() {
                    Some(v) => v,
                    None => {
                        return error(
                            "Only `Ipld::Map`s with a single key can be deserialized to `enum`, input had no keys"
                        );
                    }
                };
                // Enums are encoded in IPLD as maps with a single key-value pair
                if iter.next().is_some() {
                    return error(
                        "Only `Ipld::Map`s with a single key can be deserialized to `enum`, input had more keys"
                    );
                }
                (variant, Some(value))
            }
            Ipld::String(variant) => (variant, None),
            _ => return error(format!(
                    "Only `Ipld::Map` and `Ipld::String` can be deserialized to `enum`, input was `{:#?}`",
                    self
            )),
        };

        visitor.visit_enum(EnumDeserializer { variant, value })
    }

    // Heavily based on
    // https://github.com/serde-rs/json/blob/95f67a09399d546d9ecadeb747a845a77ff309b2/src/value/de.rs#L446
    fn deserialize_ignored_any<V: de::Visitor<'de>>(
        self,
        visitor: V,
    ) -> Result<V::Value, Self::Error> {
        drop(self);
        visitor.visit_unit()
    }

    fn deserialize_option<V: de::Visitor<'de>>(self, visitor: V) -> Result<V::Value, Self::Error> {
        match self {
            Self::Null => visitor.visit_none(),
            _ => visitor.visit_some(self),
        }
    }

    fn is_human_readable(&self) -> bool {
        false
    }
}

fn visit_map<'de, V>(map: BTreeMap<String, Ipld>, visitor: V) -> Result<V::Value, SerdeError>
where
    V: de::Visitor<'de>,
{
    let mut deserializer = MapDeserializer::new(map);
    visitor.visit_map(&mut deserializer)
}

fn visit_seq<'de, V>(list: Vec<Ipld>, visitor: V) -> Result<V::Value, SerdeError>
where
    V: de::Visitor<'de>,
{
    let mut deserializer = SeqDeserializer::new(list);
    visitor.visit_seq(&mut deserializer)
}

// Heavily based on
// https://github.com/serde-rs/json/blob/95f67a09399d546d9ecadeb747a845a77ff309b2/src/value/de.rs#L601
struct MapDeserializer {
    iter: <BTreeMap<String, Ipld> as IntoIterator>::IntoIter,
    value: Option<Ipld>,
}

impl MapDeserializer {
    fn new(map: BTreeMap<String, Ipld>) -> Self {
        Self {
            iter: map.into_iter(),
            value: None,
        }
    }
}

impl<'de> de::MapAccess<'de> for MapDeserializer {
    type Error = SerdeError;

    fn next_key_seed<K>(&mut self, seed: K) -> Result<Option<K::Value>, Self::Error>
    where
        K: de::DeserializeSeed<'de>,
    {
        match self.iter.next() {
            Some((key, value)) => {
                self.value = Some(value);
                seed.deserialize(Ipld::String(key)).map(Some)
            }
            None => Ok(None),
        }
    }

    fn next_value_seed<T>(&mut self, seed: T) -> Result<T::Value, Self::Error>
    where
        T: de::DeserializeSeed<'de>,
    {
        match self.value.take() {
            Some(value) => seed.deserialize(value),
            None => error("value is missing"),
        }
    }

    fn size_hint(&self) -> Option<usize> {
        match self.iter.size_hint() {
            (lower, Some(upper)) if lower == upper => Some(upper),
            _ => None,
        }
    }
}

// Heavily based on
// https://github.com/serde-rs/json/blob/95f67a09399d546d9ecadeb747a845a77ff309b2/src/value/de.rs#L554
struct SeqDeserializer {
    iter: <Vec<Ipld> as IntoIterator>::IntoIter,
}

impl SeqDeserializer {
    fn new(vec: Vec<Ipld>) -> Self {
        Self {
            iter: vec.into_iter(),
        }
    }
}

impl<'de> de::SeqAccess<'de> for SeqDeserializer {
    type Error = SerdeError;

    fn next_element_seed<T>(&mut self, seed: T) -> Result<Option<T::Value>, Self::Error>
    where
        T: de::DeserializeSeed<'de>,
    {
        match self.iter.next() {
            Some(value) => seed.deserialize(value).map(Some),
            None => Ok(None),
        }
    }

    fn size_hint(&self) -> Option<usize> {
        match self.iter.size_hint() {
            (lower, Some(upper)) if lower == upper => Some(upper),
            _ => None,
        }
    }
}

// Heavily based on
// https://github.com/serde-rs/json/blob/95f67a09399d546d9ecadeb747a845a77ff309b2/src/value/de.rs#L455
struct EnumDeserializer {
    variant: String,
    value: Option<Ipld>,
}

impl<'de> de::EnumAccess<'de> for EnumDeserializer {
    type Error = SerdeError;
    type Variant = VariantDeserializer;

    fn variant_seed<V>(self, seed: V) -> Result<(V::Value, Self::Variant), Self::Error>
    where
        V: de::DeserializeSeed<'de>,
    {
        let variant = self.variant.into_deserializer();
        let visitor = VariantDeserializer(self.value);
        seed.deserialize(variant).map(|v| (v, visitor))
    }
}

// Heavily based on
// https://github.com/serde-rs/json/blob/95f67a09399d546d9ecadeb747a845a77ff309b2/src/value/de.rs#L482
struct VariantDeserializer(Option<Ipld>);

impl<'de> de::VariantAccess<'de> for VariantDeserializer {
    type Error = SerdeError;

    fn unit_variant(self) -> Result<(), Self::Error> {
        match self.0 {
            Some(value) => de::Deserialize::deserialize(value),
            None => Ok(()),
        }
    }

    fn newtype_variant_seed<T>(self, seed: T) -> Result<T::Value, Self::Error>
    where
        T: de::DeserializeSeed<'de>,
    {
        match self.0 {
            Some(value) => seed.deserialize(value),
            None => Err(de::Error::invalid_type(
                de::Unexpected::UnitVariant,
                &"newtype variant",
            )),
        }
    }

    fn tuple_variant<V>(self, len: usize, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: de::Visitor<'de>,
    {
        match self.0 {
            Some(Ipld::List(list)) => {
                if len == list.len() {
                    visit_seq(list, visitor)
                } else {
                    error(format!("The tuple variant size must match the length of the `Ipld::List`, tuple variant size: {}, `Ipld::List` length: {}", len, list.len()))
                }
            }
            Some(_) => error(format!(
                "Only `Ipld::List` can be deserialized to tuple variant, input was `{:#?}`",
                self.0
            )),
            None => Err(de::Error::invalid_type(
                de::Unexpected::UnitVariant,
                &"tuple variant",
            )),
        }
    }

    fn struct_variant<V>(
        self,
        _fields: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: de::Visitor<'de>,
    {
        match self.0 {
            Some(Ipld::Map(v)) => visit_map(v, visitor),
            Some(_) => error(format!(
                "Only `Ipld::Map` can be deserialized to struct variant, input was `{:#?}`",
                self.0
            )),
            None => Err(de::Error::invalid_type(
                de::Unexpected::UnitVariant,
                &"struct variant",
            )),
        }
    }
}

/// Returns a general error.
fn error<S, T>(message: S) -> Result<T, SerdeError>
where
    S: AsRef<str> + fmt::Display,
{
    Err(de::Error::custom(message))
}
