use serde::de::IntoDeserializer as _;

use crate::de::DatetimeDeserializer;
use crate::de::Error;

/// Deserialization implementation for TOML [values][crate::Value].
///
/// Can be created either directly from TOML strings, using [`std::str::FromStr`],
/// or from parsed [values][crate::Value] using [`serde::de::IntoDeserializer::into_deserializer`].
///
/// # Example
///
/// ```
/// # #[cfg(feature = "parse")] {
/// # #[cfg(feature = "display")] {
/// use serde::Deserialize;
///
/// #[derive(Deserialize)]
/// struct Config {
///     title: String,
///     owner: Owner,
/// }
///
/// #[derive(Deserialize)]
/// struct Owner {
///     name: String,
/// }
///
/// let value = r#"{ title = 'TOML Example', owner = { name = 'Lisa' } }"#;
/// let deserializer = value.parse::<toml_edit::de::ValueDeserializer>().unwrap();
/// let config = Config::deserialize(deserializer).unwrap();
/// assert_eq!(config.title, "TOML Example");
/// assert_eq!(config.owner.name, "Lisa");
/// # }
/// # }
/// ```
pub struct ValueDeserializer {
    input: crate::Item,
    validate_struct_keys: bool,
}

impl ValueDeserializer {
    pub(crate) fn new(input: crate::Item) -> Self {
        Self {
            input,
            validate_struct_keys: false,
        }
    }

    pub(crate) fn with_struct_key_validation(mut self) -> Self {
        self.validate_struct_keys = true;
        self
    }
}

// Note: this is wrapped by `toml::de::ValueDeserializer` and any trait methods
// implemented here need to be wrapped there
impl<'de> serde::Deserializer<'de> for ValueDeserializer {
    type Error = Error;

    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: serde::de::Visitor<'de>,
    {
        let span = self.input.span();
        match self.input {
            crate::Item::None => visitor.visit_none(),
            crate::Item::Value(crate::Value::String(v)) => visitor.visit_string(v.into_value()),
            crate::Item::Value(crate::Value::Integer(v)) => visitor.visit_i64(v.into_value()),
            crate::Item::Value(crate::Value::Float(v)) => visitor.visit_f64(v.into_value()),
            crate::Item::Value(crate::Value::Boolean(v)) => visitor.visit_bool(v.into_value()),
            crate::Item::Value(crate::Value::Datetime(v)) => {
                visitor.visit_map(DatetimeDeserializer::new(v.into_value()))
            }
            crate::Item::Value(crate::Value::Array(v)) => {
                v.into_deserializer().deserialize_any(visitor)
            }
            crate::Item::Value(crate::Value::InlineTable(v)) => {
                v.into_deserializer().deserialize_any(visitor)
            }
            crate::Item::Table(v) => v.into_deserializer().deserialize_any(visitor),
            crate::Item::ArrayOfTables(v) => v.into_deserializer().deserialize_any(visitor),
        }
        .map_err(|mut e: Self::Error| {
            if e.span().is_none() {
                e.set_span(span);
            }
            e
        })
    }

    // `None` is interpreted as a missing field so be sure to implement `Some`
    // as a present field.
    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, Error>
    where
        V: serde::de::Visitor<'de>,
    {
        let span = self.input.span();
        visitor.visit_some(self).map_err(|mut e: Self::Error| {
            if e.span().is_none() {
                e.set_span(span);
            }
            e
        })
    }

    fn deserialize_newtype_struct<V>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, Error>
    where
        V: serde::de::Visitor<'de>,
    {
        let span = self.input.span();
        visitor
            .visit_newtype_struct(self)
            .map_err(|mut e: Self::Error| {
                if e.span().is_none() {
                    e.set_span(span);
                }
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
        if serde_spanned::__unstable::is_spanned(name, fields) {
            if let Some(span) = self.input.span() {
                return visitor.visit_map(super::SpannedDeserializer::new(self, span));
            }
        }

        if name == toml_datetime::__unstable::NAME && fields == [toml_datetime::__unstable::FIELD] {
            let span = self.input.span();
            if let crate::Item::Value(crate::Value::Datetime(d)) = self.input {
                return visitor
                    .visit_map(DatetimeDeserializer::new(d.into_value()))
                    .map_err(|mut e: Self::Error| {
                        if e.span().is_none() {
                            e.set_span(span);
                        }
                        e
                    });
            }
        }

        if self.validate_struct_keys {
            let span = self.input.span();
            match &self.input {
                crate::Item::Table(values) => super::validate_struct_keys(&values.items, fields),
                crate::Item::Value(crate::Value::InlineTable(values)) => {
                    super::validate_struct_keys(&values.items, fields)
                }
                _ => Ok(()),
            }
            .map_err(|mut e: Self::Error| {
                if e.span().is_none() {
                    e.set_span(span);
                }
                e
            })?;
        }

        self.deserialize_any(visitor)
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
        let span = self.input.span();
        match self.input {
            crate::Item::Value(crate::Value::String(v)) => {
                visitor.visit_enum(v.into_value().into_deserializer())
            }
            crate::Item::Value(crate::Value::InlineTable(v)) => {
                if v.is_empty() {
                    Err(Error::custom(
                        "wanted exactly 1 element, found 0 elements",
                        v.span(),
                    ))
                } else if v.len() != 1 {
                    Err(Error::custom(
                        "wanted exactly 1 element, more than 1 element",
                        v.span(),
                    ))
                } else {
                    v.into_deserializer()
                        .deserialize_enum(name, variants, visitor)
                }
            }
            crate::Item::Table(v) => v
                .into_deserializer()
                .deserialize_enum(name, variants, visitor),
            e => Err(Error::custom("wanted string or table", e.span())),
        }
        .map_err(|mut e: Self::Error| {
            if e.span().is_none() {
                e.set_span(span);
            }
            e
        })
    }

    serde::forward_to_deserialize_any! {
        bool u8 u16 u32 u64 i8 i16 i32 i64 f32 f64 char str string seq
        bytes byte_buf map unit
        ignored_any unit_struct tuple_struct tuple identifier
    }
}

impl<'de> serde::de::IntoDeserializer<'de, Error> for ValueDeserializer {
    type Deserializer = Self;

    fn into_deserializer(self) -> Self::Deserializer {
        self
    }
}

impl<'de> serde::de::IntoDeserializer<'de, Error> for crate::Value {
    type Deserializer = ValueDeserializer;

    fn into_deserializer(self) -> Self::Deserializer {
        ValueDeserializer::new(crate::Item::Value(self))
    }
}

impl crate::Item {
    pub(crate) fn into_deserializer(self) -> ValueDeserializer {
        ValueDeserializer::new(self)
    }
}

#[cfg(feature = "parse")]
impl std::str::FromStr for ValueDeserializer {
    type Err = Error;

    /// Parses a value from a &str
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let v = crate::parser::parse_value(s).map_err(Error::from)?;
        Ok(v.into_deserializer())
    }
}
