//! Deserializing TOML into Rust structures.
//!
//! This module contains all the Serde support for deserializing TOML documents into Rust structures.

use serde::de::DeserializeOwned;

mod array;
mod datetime;
mod key;
mod spanned;
mod table;
mod table_enum;
mod value;

use array::ArrayDeserializer;
use datetime::DatetimeDeserializer;
use key::KeyDeserializer;
use spanned::SpannedDeserializer;
use table::TableMapAccess;
use table_enum::TableEnumDeserializer;

pub use value::ValueDeserializer;

/// Errors that can occur when deserializing a type.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error {
    inner: crate::TomlError,
}

impl Error {
    pub(crate) fn custom<T>(msg: T, span: Option<std::ops::Range<usize>>) -> Self
    where
        T: std::fmt::Display,
    {
        Error {
            inner: crate::TomlError::custom(msg.to_string(), span),
        }
    }

    /// Add key while unwinding
    pub fn add_key(&mut self, key: String) {
        self.inner.add_key(key)
    }

    /// What went wrong
    pub fn message(&self) -> &str {
        self.inner.message()
    }

    /// The start/end index into the original document where the error occurred
    pub fn span(&self) -> Option<std::ops::Range<usize>> {
        self.inner.span()
    }

    pub(crate) fn set_span(&mut self, span: Option<std::ops::Range<usize>>) {
        self.inner.set_span(span);
    }
}

impl serde::de::Error for Error {
    fn custom<T>(msg: T) -> Self
    where
        T: std::fmt::Display,
    {
        Error::custom(msg, None)
    }
}

impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        self.inner.fmt(f)
    }
}

impl From<crate::TomlError> for Error {
    fn from(e: crate::TomlError) -> Error {
        Self { inner: e }
    }
}

impl From<Error> for crate::TomlError {
    fn from(e: Error) -> crate::TomlError {
        e.inner
    }
}

impl std::error::Error for Error {}

/// Convert a value into `T`.
pub fn from_str<T>(s: &'_ str) -> Result<T, Error>
where
    T: DeserializeOwned,
{
    let de = s.parse::<Deserializer>()?;
    T::deserialize(de)
}

/// Convert a value into `T`.
pub fn from_slice<T>(s: &'_ [u8]) -> Result<T, Error>
where
    T: DeserializeOwned,
{
    let s = std::str::from_utf8(s).map_err(|e| Error::custom(e, None))?;
    from_str(s)
}

/// Convert a document into `T`.
pub fn from_document<T>(d: crate::Document) -> Result<T, Error>
where
    T: DeserializeOwned,
{
    let deserializer = Deserializer::new(d);
    T::deserialize(deserializer)
}

/// Deserialization for TOML [documents][crate::Document].
pub struct Deserializer {
    input: crate::Document,
}

impl Deserializer {
    /// Deserialization implementation for TOML.
    pub fn new(input: crate::Document) -> Self {
        Self { input }
    }
}

impl std::str::FromStr for Deserializer {
    type Err = Error;

    /// Parses a document from a &str
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let d = crate::parser::parse_document(s).map_err(Error::from)?;
        Ok(Self::new(d))
    }
}

// Note: this is wrapped by `toml::de::Deserializer` and any trait methods
// implemented here need to be wrapped there
impl<'de> serde::Deserializer<'de> for Deserializer {
    type Error = Error;

    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: serde::de::Visitor<'de>,
    {
        let original = self.input.original;
        self.input
            .root
            .into_deserializer()
            .deserialize_any(visitor)
            .map_err(|mut e: Self::Error| {
                e.inner.set_original(original);
                e
            })
    }

    // `None` is interpreted as a missing field so be sure to implement `Some`
    // as a present field.
    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, Error>
    where
        V: serde::de::Visitor<'de>,
    {
        let original = self.input.original;
        self.input
            .root
            .into_deserializer()
            .deserialize_option(visitor)
            .map_err(|mut e: Self::Error| {
                e.inner.set_original(original);
                e
            })
    }

    fn deserialize_newtype_struct<V>(
        self,
        name: &'static str,
        visitor: V,
    ) -> Result<V::Value, Error>
    where
        V: serde::de::Visitor<'de>,
    {
        let original = self.input.original;
        self.input
            .root
            .into_deserializer()
            .deserialize_newtype_struct(name, visitor)
            .map_err(|mut e: Self::Error| {
                e.inner.set_original(original);
                e
            })
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
        let original = self.input.original;
        self.input
            .root
            .into_deserializer()
            .deserialize_struct(name, fields, visitor)
            .map_err(|mut e: Self::Error| {
                e.inner.set_original(original);
                e
            })
    }

    // Called when the type to deserialize is an enum, as opposed to a field in the type.
    fn deserialize_enum<V>(
        self,
        name: &'static str,
        variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Error>
    where
        V: serde::de::Visitor<'de>,
    {
        let original = self.input.original;
        self.input
            .root
            .into_deserializer()
            .deserialize_enum(name, variants, visitor)
            .map_err(|mut e: Self::Error| {
                e.inner.set_original(original);
                e
            })
    }

    serde::forward_to_deserialize_any! {
        bool u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 char str string seq
        bytes byte_buf map unit
        ignored_any unit_struct tuple_struct tuple identifier
    }
}

impl<'de> serde::de::IntoDeserializer<'de, crate::de::Error> for Deserializer {
    type Deserializer = Deserializer;

    fn into_deserializer(self) -> Self::Deserializer {
        self
    }
}

impl<'de> serde::de::IntoDeserializer<'de, crate::de::Error> for crate::Document {
    type Deserializer = Deserializer;

    fn into_deserializer(self) -> Self::Deserializer {
        Deserializer::new(self)
    }
}

pub(crate) fn validate_struct_keys(
    table: &crate::table::KeyValuePairs,
    fields: &'static [&'static str],
) -> Result<(), Error> {
    let extra_fields = table
        .iter()
        .filter_map(|(key, val)| {
            if !fields.contains(&key.as_str()) {
                Some(val.clone())
            } else {
                None
            }
        })
        .collect::<Vec<_>>();

    if extra_fields.is_empty() {
        Ok(())
    } else {
        Err(Error::custom(
            format!(
                "unexpected keys in table: {}, available keys: {}",
                extra_fields
                    .iter()
                    .map(|k| k.key.get())
                    .collect::<Vec<_>>()
                    .join(", "),
                fields.join(", "),
            ),
            extra_fields[0].key.span(),
        ))
    }
}
