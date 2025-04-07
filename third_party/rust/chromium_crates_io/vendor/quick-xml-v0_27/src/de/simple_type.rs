//! Contains Serde `Deserializer` for XML [simple types] [as defined] in the XML Schema.
//!
//! [simple types]: https://www.w3schools.com/xml/el_simpletype.asp
//! [as defined]: https://www.w3.org/TR/xmlschema11-1/#Simple_Type_Definition

use crate::de::{deserialize_bool, str2bool};
use crate::encoding::Decoder;
use crate::errors::serialize::DeError;
use crate::escape::unescape;
use memchr::memchr;
use serde::de::{DeserializeSeed, Deserializer, EnumAccess, SeqAccess, VariantAccess, Visitor};
use serde::{self, serde_if_integer128};
use std::borrow::Cow;
use std::ops::{Deref, Range};

macro_rules! deserialize_num {
    ($method:ident, $visit:ident) => {
        fn $method<V>(self, visitor: V) -> Result<V::Value, Self::Error>
        where
            V: Visitor<'de>,
        {
            visitor.$visit(self.content.as_str().parse()?)
        }
    };
    ($method:ident => $visit:ident) => {
        fn $method<V>(self, visitor: V) -> Result<V::Value, Self::Error>
        where
            V: Visitor<'de>,
        {
            let string = self.decode()?;
            visitor.$visit(string.as_str().parse()?)
        }
    };
}

macro_rules! unsupported {
    (
        $deserialize:ident
        $(
            ($($type:ty),*)
        )?
        => $message:literal
    ) => {
        #[inline]
        fn $deserialize<V: Visitor<'de>>(
            self,
            $($(_: $type,)*)?
            _visitor: V
        ) -> Result<V::Value, Self::Error> {
            Err(DeError::Unsupported($message.into()))
        }
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// A version of [`Cow`] that can borrow from two different buffers, one of them
/// is a deserializer input, and conceptually contains only part of owned data.
///
/// # Lifetimes
/// - `'de` -- lifetime of the data that deserializer borrow from the parsed input
/// - `'a` -- lifetime of the data that owned by a deserializer
enum Content<'de, 'a> {
    /// An input borrowed from the parsed data
    Input(&'de str),
    /// An input borrowed from the buffer owned by another deserializer
    Slice(&'a str),
    /// An input taken from an external deserializer, owned by that deserializer.
    /// Only part of this data, located after offset represented by `usize`, used
    /// to deserialize data, the other is a garbage that can't be dropped because
    /// we do not want to make reallocations if they will not required.
    Owned(String, usize),
}
impl<'de, 'a> Content<'de, 'a> {
    /// Returns string representation of the content
    fn as_str(&self) -> &str {
        match self {
            Content::Input(s) => s,
            Content::Slice(s) => s,
            Content::Owned(s, offset) => s.split_at(*offset).1,
        }
    }

    /// Supply to the visitor a borrowed string, a string slice, or an owned
    /// string depending on the kind of input. Unlike [`Self::deserialize_item`],
    /// the whole [`Self::Owned`] string will be passed to the visitor.
    ///
    /// Calls
    /// - `visitor.visit_borrowed_str` if data borrowed from the input
    /// - `visitor.visit_str` if data borrowed from another source
    /// - `visitor.visit_string` if data owned by this type
    #[inline]
    fn deserialize_all<V>(self, visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        match self {
            Content::Input(s) => visitor.visit_borrowed_str(s),
            Content::Slice(s) => visitor.visit_str(s),
            Content::Owned(s, _) => visitor.visit_string(s),
        }
    }

    /// Supply to the visitor a borrowed string, a string slice, or an owned
    /// string depending on the kind of input. Unlike [`Self::deserialize_all`],
    /// only part of [`Self::Owned`] string will be passed to the visitor.
    ///
    /// Calls
    /// - `visitor.visit_borrowed_str` if data borrowed from the input
    /// - `visitor.visit_str` if data borrowed from another source
    /// - `visitor.visit_string` if data owned by this type
    #[inline]
    fn deserialize_item<V>(self, visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        match self {
            Content::Input(s) => visitor.visit_borrowed_str(s),
            Content::Slice(s) => visitor.visit_str(s),
            Content::Owned(s, 0) => visitor.visit_string(s),
            Content::Owned(s, offset) => visitor.visit_str(s.split_at(offset).1),
        }
    }
}

/// A deserializer that handles ordinary [simple type definition][item] with
/// `{variety} = atomic`, or an ordinary [simple type] definition with
/// `{variety} = union` whose basic members are all atomic.
///
/// This deserializer can deserialize only primitive types:
/// - numbers
/// - booleans
/// - strings
/// - units
/// - options
/// - unit variants of enums
///
/// Identifiers represented as strings and deserialized accordingly.
///
/// Deserialization of all other types returns [`Unsupported`][DeError::Unsupported] error.
///
/// The `Owned` variant of the content acts as a storage for data, allocated by
/// an external deserializer that pass it via [`ListIter`].
///
/// [item]: https://www.w3.org/TR/xmlschema11-1/#std-item_type_definition
/// [simple type]: https://www.w3.org/TR/xmlschema11-1/#Simple_Type_Definition
struct AtomicDeserializer<'de, 'a> {
    /// Content of the attribute value, text content or CDATA content
    content: Content<'de, 'a>,
    /// If `true`, `content` in an escaped form and should be unescaped before use
    escaped: bool,
}

impl<'de, 'a> Deserializer<'de> for AtomicDeserializer<'de, 'a> {
    type Error = DeError;

    /// Forwards deserialization to the [`Self::deserialize_str`]
    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    /// According to the <https://www.w3.org/TR/xmlschema-2/#boolean>,
    /// valid boolean representations are only `"true"`, `"false"`, `"1"`,
    /// and `"0"`. But this method also handles following:
    ///
    /// |`bool` |XML content
    /// |-------|-------------------------------------------------------------
    /// |`true` |`"True"`,  `"TRUE"`,  `"t"`, `"Yes"`, `"YES"`, `"yes"`, `"y"`
    /// |`false`|`"False"`, `"FALSE"`, `"f"`, `"No"`,  `"NO"`,  `"no"`,  `"n"`
    fn deserialize_bool<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        str2bool(self.content.as_str(), visitor)
    }

    deserialize_num!(deserialize_i8, visit_i8);
    deserialize_num!(deserialize_i16, visit_i16);
    deserialize_num!(deserialize_i32, visit_i32);
    deserialize_num!(deserialize_i64, visit_i64);

    deserialize_num!(deserialize_u8, visit_u8);
    deserialize_num!(deserialize_u16, visit_u16);
    deserialize_num!(deserialize_u32, visit_u32);
    deserialize_num!(deserialize_u64, visit_u64);

    serde_if_integer128! {
        deserialize_num!(deserialize_i128, visit_i128);
        deserialize_num!(deserialize_u128, visit_u128);
    }

    deserialize_num!(deserialize_f32, visit_f32);
    deserialize_num!(deserialize_f64, visit_f64);

    /// Forwards deserialization to the [`Self::deserialize_str`]
    fn deserialize_char<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    /// Supply to the visitor borrowed string, string slice, or owned string
    /// depending on the kind of input and presence of the escaped data.
    ///
    /// If string requires unescaping, then calls [`Visitor::visit_string`] with
    /// new allocated buffer with unescaped data.
    ///
    /// Otherwise calls
    /// - [`Visitor::visit_borrowed_str`] if data borrowed from the input
    /// - [`Visitor::visit_str`] if data borrowed from other deserializer
    /// - [`Visitor::visit_string`] if data owned by this deserializer
    fn deserialize_str<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        if self.escaped {
            match unescape(self.content.as_str())? {
                Cow::Borrowed(_) => self.content.deserialize_item(visitor),
                Cow::Owned(s) => visitor.visit_string(s),
            }
        } else {
            self.content.deserialize_item(visitor)
        }
    }

    fn deserialize_string<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    /// If `content` is an empty string then calls [`Visitor::visit_none`],
    /// otherwise calls [`Visitor::visit_some`] with itself
    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        if self.content.as_str().is_empty() {
            visitor.visit_none()
        } else {
            visitor.visit_some(self)
        }
    }

    fn deserialize_unit<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_unit()
    }

    /// Forwards deserialization to the [`Self::deserialize_unit`]
    fn deserialize_unit_struct<V>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_unit(visitor)
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

    fn deserialize_enum<V>(
        self,
        _name: &'static str,
        _variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_enum(self)
    }

    /// Forwards deserialization to the [`Self::deserialize_str`]
    fn deserialize_identifier<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    fn deserialize_ignored_any<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_unit()
    }

    unsupported!(deserialize_bytes        => "byte arrays are not supported as `xs:list` items");
    unsupported!(deserialize_byte_buf     => "byte arrays are not supported as `xs:list` items");
    unsupported!(deserialize_seq          => "sequences are not supported as `xs:list` items");
    unsupported!(deserialize_tuple(usize) => "tuples are not supported as `xs:list` items");
    unsupported!(deserialize_tuple_struct(&'static str, usize) => "tuples are not supported as `xs:list` items");
    unsupported!(deserialize_map          => "maps are not supported as `xs:list` items");
    unsupported!(deserialize_struct(&'static str, &'static [&'static str]) => "structures are not supported as `xs:list` items");
}

impl<'de, 'a> EnumAccess<'de> for AtomicDeserializer<'de, 'a> {
    type Error = DeError;
    type Variant = AtomicUnitOnly;

    fn variant_seed<V>(self, seed: V) -> Result<(V::Value, Self::Variant), DeError>
    where
        V: DeserializeSeed<'de>,
    {
        let name = seed.deserialize(self)?;
        Ok((name, AtomicUnitOnly))
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Deserializer of variant data, that supports only unit variants.
/// Attempt to deserialize newtype, tuple or struct variant will return a
/// [`DeError::Unsupported`] error.
pub struct AtomicUnitOnly;
impl<'de> VariantAccess<'de> for AtomicUnitOnly {
    type Error = DeError;

    #[inline]
    fn unit_variant(self) -> Result<(), DeError> {
        Ok(())
    }

    fn newtype_variant_seed<T>(self, _seed: T) -> Result<T::Value, DeError>
    where
        T: DeserializeSeed<'de>,
    {
        Err(DeError::Unsupported(
            "enum newtype variants are not supported as `xs:list` items".into(),
        ))
    }

    fn tuple_variant<V>(self, _len: usize, _visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        Err(DeError::Unsupported(
            "enum tuple variants are not supported as `xs:list` items".into(),
        ))
    }

    fn struct_variant<V>(
        self,
        _fields: &'static [&'static str],
        _visitor: V,
    ) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        Err(DeError::Unsupported(
            "enum struct variants are not supported as `xs:list` items".into(),
        ))
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Iterator over string sub-slices delimited by one or several spaces.
/// Contains decoded value of the `simpleType`.
/// Iteration ends when list contains `None`.
struct ListIter<'de, 'a> {
    /// If `Some`, contains unconsumed data of the list
    content: Option<Content<'de, 'a>>,
    /// If `true`, `content` in escaped form and should be unescaped before use
    escaped: bool,
}
impl<'de, 'a> SeqAccess<'de> for ListIter<'de, 'a> {
    type Error = DeError;

    fn next_element_seed<T>(&mut self, seed: T) -> Result<Option<T::Value>, DeError>
    where
        T: DeserializeSeed<'de>,
    {
        if let Some(mut content) = self.content.take() {
            const DELIMITER: u8 = b' ';

            loop {
                let string = content.as_str();
                if string.is_empty() {
                    return Ok(None);
                }
                return match memchr(DELIMITER, string.as_bytes()) {
                    // No delimiters in the `content`, deserialize it as a whole atomic
                    None => seed.deserialize(AtomicDeserializer {
                        content,
                        escaped: self.escaped,
                    }),
                    // `content` started with a space, skip them all
                    Some(0) => {
                        // Skip all spaces
                        let start = string.as_bytes().iter().position(|ch| *ch != DELIMITER);
                        content = match (start, content) {
                            // We cannot find any non-space character, so string contains only spaces
                            (None, _) => return Ok(None),
                            // Borrow result from input or deserializer depending on the initial borrowing
                            (Some(start), Content::Input(s)) => Content::Input(s.split_at(start).1),
                            (Some(start), Content::Slice(s)) => Content::Slice(s.split_at(start).1),
                            // Skip additional bytes if we own data
                            (Some(start), Content::Owned(s, skip)) => {
                                Content::Owned(s, skip + start)
                            }
                        };
                        continue;
                    }
                    // `content` started from an atomic
                    Some(end) => match content {
                        // Borrow for the next iteration from input or deserializer depending on
                        // the initial borrowing
                        Content::Input(s) => {
                            let (item, rest) = s.split_at(end);
                            self.content = Some(Content::Input(rest));

                            seed.deserialize(AtomicDeserializer {
                                content: Content::Input(item),
                                escaped: self.escaped,
                            })
                        }
                        Content::Slice(s) => {
                            let (item, rest) = s.split_at(end);
                            self.content = Some(Content::Slice(rest));

                            seed.deserialize(AtomicDeserializer {
                                content: Content::Slice(item),
                                escaped: self.escaped,
                            })
                        }
                        // Skip additional bytes if we own data for next iteration, but deserialize from
                        // the borrowed data from our buffer
                        Content::Owned(s, skip) => {
                            let item = s.split_at(skip + end).0;
                            let result = seed.deserialize(AtomicDeserializer {
                                content: Content::Slice(item),
                                escaped: self.escaped,
                            });

                            self.content = Some(Content::Owned(s, skip + end));

                            result
                        }
                    },
                }
                .map(Some);
            }
        }
        Ok(None)
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// A version of [`Cow`] that can borrow from two different buffers, one of them
/// is a deserializer input.
///
/// # Lifetimes
/// - `'de` -- lifetime of the data that deserializer borrow from the parsed input
/// - `'a` -- lifetime of the data that owned by a deserializer
enum CowRef<'de, 'a> {
    /// An input borrowed from the parsed data
    Input(&'de [u8]),
    /// An input borrowed from the buffer owned by another deserializer
    Slice(&'a [u8]),
    /// An input taken from an external deserializer, owned by that deserializer
    Owned(Vec<u8>),
}
impl<'de, 'a> Deref for CowRef<'de, 'a> {
    type Target = [u8];

    fn deref(&self) -> &[u8] {
        match self {
            Self::Input(slice) => slice,
            Self::Slice(slice) => slice,
            Self::Owned(ref v) => v,
        }
    }
}

/// A deserializer for an xml probably escaped and encoded value of XSD [simple types].
/// This deserializer will borrow from the input as much as possible.
///
/// `deserialize_any()` returns the whole string that deserializer contains.
///
/// Escaping the value is actually not always necessary, for instance when
/// converting to a float, we don't expect any escapable character anyway.
/// In that cases deserializer skips unescaping step.
///
/// Used for deserialize values from:
/// - attribute values (`<... ...="value" ...>`)
/// - text content (`<...>text</...>`)
/// - CDATA content (`<...><![CDATA[cdata]]></...>`)
///
/// [simple types]: https://www.w3.org/TR/xmlschema11-1/#Simple_Type_Definition
pub struct SimpleTypeDeserializer<'de, 'a> {
    /// - In case of attribute contains escaped attribute value
    /// - In case of text contains escaped text value
    /// - In case of CData contains unescaped cdata value
    content: CowRef<'de, 'a>,
    /// If `true`, `content` in escaped form and should be unescaped before use
    escaped: bool,
    /// Decoder used to deserialize string data, numeric and boolean data.
    /// Not used for deserializing raw byte buffers
    decoder: Decoder,
}

impl<'de, 'a> SimpleTypeDeserializer<'de, 'a> {
    /// Creates a deserializer from a value, that possible borrowed from input
    pub fn from_text_content(value: Cow<'de, str>) -> Self {
        let content = match value {
            Cow::Borrowed(slice) => CowRef::Input(slice.as_bytes()),
            Cow::Owned(content) => CowRef::Owned(content.into_bytes()),
        };
        Self::new(content, false, Decoder::utf8())
    }

    /// Creates a deserializer from a part of value at specified range
    pub fn from_part(
        value: &'a Cow<'de, [u8]>,
        range: Range<usize>,
        escaped: bool,
        decoder: Decoder,
    ) -> Self {
        let content = match value {
            Cow::Borrowed(slice) => CowRef::Input(&slice[range]),
            Cow::Owned(slice) => CowRef::Slice(&slice[range]),
        };
        Self::new(content, escaped, decoder)
    }

    /// Constructor for tests
    #[inline]
    fn new(content: CowRef<'de, 'a>, escaped: bool, decoder: Decoder) -> Self {
        Self {
            content,
            escaped,
            decoder,
        }
    }

    /// Decodes raw bytes using the encoding specified.
    /// The method will borrow if has the UTF-8 compatible representation.
    #[inline]
    fn decode<'b>(&'b self) -> Result<Content<'de, 'b>, DeError> {
        Ok(match self.content {
            CowRef::Input(content) => match self.decoder.decode(content)? {
                Cow::Borrowed(content) => Content::Input(content),
                Cow::Owned(content) => Content::Owned(content, 0),
            },
            CowRef::Slice(content) => match self.decoder.decode(content)? {
                Cow::Borrowed(content) => Content::Slice(content),
                Cow::Owned(content) => Content::Owned(content, 0),
            },
            CowRef::Owned(ref content) => match self.decoder.decode(content)? {
                Cow::Borrowed(content) => Content::Slice(content),
                Cow::Owned(content) => Content::Owned(content, 0),
            },
        })
    }
}

impl<'de, 'a> Deserializer<'de> for SimpleTypeDeserializer<'de, 'a> {
    type Error = DeError;

    /// Forwards deserialization to the [`Self::deserialize_str`]
    fn deserialize_any<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    fn deserialize_bool<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        deserialize_bool(&self.content, self.decoder, visitor)
    }

    deserialize_num!(deserialize_i8  => visit_i8);
    deserialize_num!(deserialize_i16 => visit_i16);
    deserialize_num!(deserialize_i32 => visit_i32);
    deserialize_num!(deserialize_i64 => visit_i64);

    deserialize_num!(deserialize_u8  => visit_u8);
    deserialize_num!(deserialize_u16 => visit_u16);
    deserialize_num!(deserialize_u32 => visit_u32);
    deserialize_num!(deserialize_u64 => visit_u64);

    serde_if_integer128! {
        deserialize_num!(deserialize_i128 => visit_i128);
        deserialize_num!(deserialize_u128 => visit_u128);
    }

    deserialize_num!(deserialize_f32 => visit_f32);
    deserialize_num!(deserialize_f64 => visit_f64);

    /// Forwards deserialization to the [`Self::deserialize_str`]
    fn deserialize_char<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    fn deserialize_str<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        let content = self.decode()?;
        if self.escaped {
            match unescape(content.as_str())? {
                Cow::Borrowed(_) => content.deserialize_all(visitor),
                Cow::Owned(s) => visitor.visit_string(s),
            }
        } else {
            content.deserialize_all(visitor)
        }
    }

    /// Forwards deserialization to the [`Self::deserialize_str`]
    fn deserialize_string<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
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

    /// Forwards deserialization to the [`Self::deserialize_bytes`]
    fn deserialize_byte_buf<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_bytes(visitor)
    }

    fn deserialize_option<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        if self.content.is_empty() {
            visitor.visit_none()
        } else {
            visitor.visit_some(self)
        }
    }

    fn deserialize_unit<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_unit()
    }

    /// Forwards deserialization to the [`Self::deserialize_unit`]
    fn deserialize_unit_struct<V>(
        self,
        _name: &'static str,
        visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_unit(visitor)
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

    fn deserialize_seq<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_seq(ListIter {
            content: Some(self.decode()?),
            escaped: self.escaped,
        })
    }

    /// Representation of tuples the same as [sequences][Self::deserialize_seq].
    fn deserialize_tuple<V>(self, _len: usize, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_seq(visitor)
    }

    /// Representation of named tuples the same as [unnamed tuples][Self::deserialize_tuple].
    fn deserialize_tuple_struct<V>(
        self,
        _name: &'static str,
        len: usize,
        visitor: V,
    ) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        self.deserialize_tuple(len, visitor)
    }

    unsupported!(deserialize_map => "maps are not supported for XSD `simpleType`s");
    unsupported!(deserialize_struct(&'static str, &'static [&'static str])
                 => "structures are not supported for XSD `simpleType`s");

    fn deserialize_enum<V>(
        self,
        _name: &'static str,
        _variants: &'static [&'static str],
        visitor: V,
    ) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_enum(self)
    }

    /// Forwards deserialization to the [`Self::deserialize_str`]
    fn deserialize_identifier<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        self.deserialize_str(visitor)
    }

    fn deserialize_ignored_any<V>(self, visitor: V) -> Result<V::Value, Self::Error>
    where
        V: Visitor<'de>,
    {
        visitor.visit_unit()
    }
}

impl<'de, 'a> EnumAccess<'de> for SimpleTypeDeserializer<'de, 'a> {
    type Error = DeError;
    type Variant = SimpleTypeUnitOnly;

    fn variant_seed<V>(self, seed: V) -> Result<(V::Value, Self::Variant), DeError>
    where
        V: DeserializeSeed<'de>,
    {
        let name = seed.deserialize(self)?;
        Ok((name, SimpleTypeUnitOnly))
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Deserializer of variant data, that supports only unit variants.
/// Attempt to deserialize newtype, tuple or struct variant will return a
/// [`DeError::Unsupported`] error.
pub struct SimpleTypeUnitOnly;
impl<'de> VariantAccess<'de> for SimpleTypeUnitOnly {
    type Error = DeError;

    #[inline]
    fn unit_variant(self) -> Result<(), DeError> {
        Ok(())
    }

    fn newtype_variant_seed<T>(self, _seed: T) -> Result<T::Value, DeError>
    where
        T: DeserializeSeed<'de>,
    {
        Err(DeError::Unsupported(
            "enum newtype variants are not supported for XSD `simpleType`s".into(),
        ))
    }

    fn tuple_variant<V>(self, _len: usize, _visitor: V) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        Err(DeError::Unsupported(
            "enum tuple variants are not supported for XSD `simpleType`s".into(),
        ))
    }

    fn struct_variant<V>(
        self,
        _fields: &'static [&'static str],
        _visitor: V,
    ) -> Result<V::Value, DeError>
    where
        V: Visitor<'de>,
    {
        Err(DeError::Unsupported(
            "enum struct variants are not supported for XSD `simpleType`s".into(),
        ))
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#[cfg(test)]
mod tests {
    use super::*;
    use crate::se::simple_type::{QuoteTarget, SimpleTypeSerializer};
    use crate::se::{Indent, QuoteLevel};
    use crate::utils::{ByteBuf, Bytes};
    use serde::de::IgnoredAny;
    use serde::{Deserialize, Serialize};
    use std::collections::HashMap;

    macro_rules! simple_only {
        ($encoding:ident, $name:ident: $type:ty = $xml:expr => $result:expr) => {
            #[test]
            fn $name() {
                let decoder = Decoder::$encoding();
                let xml = $xml;
                let de = SimpleTypeDeserializer::new(CowRef::Input(xml.as_ref()), true, decoder);
                let data: $type = Deserialize::deserialize(de).unwrap();

                assert_eq!(data, $result);
            }
        };
    }

    macro_rules! simple {
        ($encoding:ident, $name:ident: $type:ty = $xml:expr => $result:expr) => {
            #[test]
            fn $name() {
                let decoder = Decoder::$encoding();
                let xml = $xml;
                let de = SimpleTypeDeserializer::new(CowRef::Input(xml.as_ref()), true, decoder);
                let data: $type = Deserialize::deserialize(de).unwrap();

                assert_eq!(data, $result);

                // Roundtrip to ensure that serializer corresponds to deserializer
                assert_eq!(
                    data.serialize(SimpleTypeSerializer {
                        writer: String::new(),
                        target: QuoteTarget::Text,
                        level: QuoteLevel::Full,
                        indent: Indent::None,
                    })
                    .unwrap(),
                    xml
                );
            }
        };
    }

    macro_rules! err {
        ($encoding:ident, $name:ident: $type:ty = $xml:expr => $kind:ident($reason:literal)) => {
            #[test]
            fn $name() {
                let decoder = Decoder::$encoding();
                let xml = $xml;
                let de = SimpleTypeDeserializer::new(CowRef::Input(xml.as_ref()), true, decoder);
                let err = <$type as Deserialize>::deserialize(de).unwrap_err();

                match err {
                    DeError::$kind(e) => assert_eq!(e, $reason),
                    _ => panic!(
                        "Expected `{}({})`, found `{:?}`",
                        stringify!($kind),
                        $reason,
                        err
                    ),
                }
            }
        };
    }

    #[derive(Debug, Deserialize, Serialize, PartialEq)]
    struct Unit;

    #[derive(Debug, Deserialize, Serialize, PartialEq)]
    struct Newtype(String);

    #[derive(Debug, Deserialize, Serialize, PartialEq)]
    struct BorrowedNewtype<'a>(&'a str);

    #[derive(Debug, Deserialize, Serialize, PartialEq)]
    struct Struct {
        key: String,
        val: usize,
    }

    #[derive(Debug, Deserialize, Serialize, PartialEq)]
    enum Enum {
        Unit,
        Newtype(String),
        Tuple(String, usize),
        Struct { key: String, val: usize },
    }

    #[derive(Debug, Deserialize, PartialEq)]
    #[serde(field_identifier)]
    enum Id {
        Field,
    }

    #[derive(Debug, Deserialize)]
    #[serde(transparent)]
    struct Any(IgnoredAny);
    impl PartialEq for Any {
        fn eq(&self, _other: &Any) -> bool {
            true
        }
    }

    /// Tests for deserialize atomic and union values, as defined in XSD specification
    mod atomic {
        use super::*;
        use crate::se::simple_type::AtomicSerializer;
        use pretty_assertions::assert_eq;

        /// Checks that given `$input` successfully deserializing into given `$result`
        macro_rules! deserialized_to_only {
            ($name:ident: $type:ty = $input:literal => $result:expr) => {
                #[test]
                fn $name() {
                    let de = AtomicDeserializer {
                        content: Content::Input($input),
                        escaped: true,
                    };
                    let data: $type = Deserialize::deserialize(de).unwrap();

                    assert_eq!(data, $result);
                }
            };
        }

        /// Checks that given `$input` successfully deserializing into given `$result`
        /// and the result is serialized back to the `$input`
        macro_rules! deserialized_to {
            ($name:ident: $type:ty = $input:literal => $result:expr) => {
                #[test]
                fn $name() {
                    let de = AtomicDeserializer {
                        content: Content::Input($input),
                        escaped: true,
                    };
                    let data: $type = Deserialize::deserialize(de).unwrap();

                    assert_eq!(data, $result);

                    // Roundtrip to ensure that serializer corresponds to deserializer
                    assert_eq!(
                        data.serialize(AtomicSerializer {
                            writer: String::new(),
                            target: QuoteTarget::Text,
                            level: QuoteLevel::Full,
                        })
                        .unwrap(),
                        $input
                    );
                }
            };
        }

        /// Checks that attempt to deserialize given `$input` as a `$type` results to a
        /// deserialization error `$kind` with `$reason`
        macro_rules! err {
            ($name:ident: $type:ty = $input:literal => $kind:ident($reason:literal)) => {
                #[test]
                fn $name() {
                    let de = AtomicDeserializer {
                        content: Content::Input($input),
                        escaped: true,
                    };
                    let err = <$type as Deserialize>::deserialize(de).unwrap_err();

                    match err {
                        DeError::$kind(e) => assert_eq!(e, $reason),
                        _ => panic!(
                            "Expected `{}({})`, found `{:?}`",
                            stringify!($kind),
                            $reason,
                            err
                        ),
                    }
                }
            };
        }

        deserialized_to!(false_: bool = "false" => false);
        deserialized_to!(true_: bool  = "true" => true);

        deserialized_to!(i8_:  i8  = "-2" => -2);
        deserialized_to!(i16_: i16 = "-2" => -2);
        deserialized_to!(i32_: i32 = "-2" => -2);
        deserialized_to!(i64_: i64 = "-2" => -2);

        deserialized_to!(u8_:  u8  = "3" => 3);
        deserialized_to!(u16_: u16 = "3" => 3);
        deserialized_to!(u32_: u32 = "3" => 3);
        deserialized_to!(u64_: u64 = "3" => 3);

        serde_if_integer128! {
            deserialized_to!(i128_: i128 = "-2" => -2);
            deserialized_to!(u128_: u128 = "2" => 2);
        }

        deserialized_to!(f32_: f32 = "1.23" => 1.23);
        deserialized_to!(f64_: f64 = "1.23" => 1.23);

        deserialized_to!(char_unescaped: char = "h" => 'h');
        deserialized_to!(char_escaped: char = "&lt;" => '<');

        deserialized_to!(string: String = "&lt;escaped&#32;string" => "<escaped string");
        // Serializer will escape space. Because borrowing has meaning only for deserializer,
        // no need to test roundtrip here, it is already tested with non-borrowing version
        deserialized_to_only!(borrowed_str: &str = "non-escaped string" => "non-escaped string");
        err!(escaped_str: &str = "escaped&#32;string"
                => Custom("invalid type: string \"escaped string\", expected a borrowed string"));

        err!(byte_buf: ByteBuf = "&lt;escaped&#32;string"
                => Unsupported("byte arrays are not supported as `xs:list` items"));
        err!(borrowed_bytes: Bytes = "non-escaped string"
                => Unsupported("byte arrays are not supported as `xs:list` items"));

        deserialized_to!(option_none: Option<&str> = "" => None);
        deserialized_to!(option_some: Option<&str> = "non-escaped-string" => Some("non-escaped-string"));

        deserialized_to_only!(unit: () = "<root>anything</root>" => ());
        deserialized_to_only!(unit_struct: Unit = "<root>anything</root>" => Unit);

        deserialized_to!(newtype_owned: Newtype = "&lt;escaped&#32;string" => Newtype("<escaped string".into()));
        // Serializer will escape space. Because borrowing has meaning only for deserializer,
        // no need to test roundtrip here, it is already tested with non-borrowing version
        deserialized_to_only!(newtype_borrowed: BorrowedNewtype = "non-escaped string"
                => BorrowedNewtype("non-escaped string"));

        err!(seq: Vec<()> = "non-escaped string"
                => Unsupported("sequences are not supported as `xs:list` items"));
        err!(tuple: ((), ()) = "non-escaped string"
                => Unsupported("tuples are not supported as `xs:list` items"));
        err!(tuple_struct: ((), ()) = "non-escaped string"
                => Unsupported("tuples are not supported as `xs:list` items"));

        err!(map: HashMap<(), ()> = "non-escaped string"
                => Unsupported("maps are not supported as `xs:list` items"));
        err!(struct_: Struct = "non-escaped string"
                => Unsupported("structures are not supported as `xs:list` items"));

        deserialized_to!(enum_unit: Enum = "Unit" => Enum::Unit);
        err!(enum_newtype: Enum = "Newtype"
                => Unsupported("enum newtype variants are not supported as `xs:list` items"));
        err!(enum_tuple: Enum = "Tuple"
                => Unsupported("enum tuple variants are not supported as `xs:list` items"));
        err!(enum_struct: Enum = "Struct"
                => Unsupported("enum struct variants are not supported as `xs:list` items"));
        err!(enum_other: Enum = "any data"
                => Custom("unknown variant `any data`, expected one of `Unit`, `Newtype`, `Tuple`, `Struct`"));

        deserialized_to_only!(identifier: Id = "Field" => Id::Field);
        deserialized_to_only!(ignored_any: Any = "any data" => Any(IgnoredAny));

        /// Checks that deserialization from an owned content is working
        #[test]
        #[cfg(feature = "encoding")]
        fn owned_data() {
            let de = AtomicDeserializer {
                content: Content::Owned("string slice".into(), 7),
                escaped: true,
            };
            assert_eq!(de.content.as_str(), "slice");

            let data: String = Deserialize::deserialize(de).unwrap();
            assert_eq!(data, "slice");
        }

        /// Checks that deserialization from a content borrowed from some
        /// buffer other that input is working
        #[test]
        fn borrowed_from_deserializer() {
            let de = AtomicDeserializer {
                content: Content::Slice("string slice"),
                escaped: true,
            };
            assert_eq!(de.content.as_str(), "string slice");

            let data: String = Deserialize::deserialize(de).unwrap();
            assert_eq!(data, "string slice");
        }
    }

    /// Module for testing list accessor
    mod list {
        use super::*;
        use pretty_assertions::assert_eq;

        #[test]
        fn empty() {
            let mut seq = ListIter {
                content: Some(Content::Input("")),
                escaped: true,
            };

            assert_eq!(seq.next_element::<&str>().unwrap(), None);
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
        }

        #[test]
        fn only_spaces() {
            let mut seq = ListIter {
                content: Some(Content::Input("  ")),
                escaped: true,
            };

            assert_eq!(seq.next_element::<&str>().unwrap(), None);
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
        }

        #[test]
        fn one_item() {
            let mut seq = ListIter {
                content: Some(Content::Input("abc")),
                escaped: true,
            };

            assert_eq!(seq.next_element::<&str>().unwrap(), Some("abc"));
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
        }

        #[test]
        fn two_items() {
            let mut seq = ListIter {
                content: Some(Content::Input("abc def")),
                escaped: true,
            };

            assert_eq!(seq.next_element::<&str>().unwrap(), Some("abc"));
            assert_eq!(seq.next_element::<&str>().unwrap(), Some("def"));
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
        }

        #[test]
        fn leading_spaces() {
            let mut seq = ListIter {
                content: Some(Content::Input("  def")),
                escaped: true,
            };

            assert_eq!(seq.next_element::<&str>().unwrap(), Some("def"));
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
        }

        #[test]
        fn trailing_spaces() {
            let mut seq = ListIter {
                content: Some(Content::Input("abc  ")),
                escaped: true,
            };

            assert_eq!(seq.next_element::<&str>().unwrap(), Some("abc"));
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
            assert_eq!(seq.next_element::<&str>().unwrap(), None);
        }

        #[test]
        fn mixed_types() {
            let mut seq = ListIter {
                content: Some(Content::Input("string 1.23 42 true false h Unit")),
                escaped: true,
            };

            assert_eq!(seq.next_element::<&str>().unwrap(), Some("string"));
            assert_eq!(seq.next_element::<f32>().unwrap(), Some(1.23));
            assert_eq!(seq.next_element::<u32>().unwrap(), Some(42));
            assert_eq!(seq.next_element::<bool>().unwrap(), Some(true));
            assert_eq!(seq.next_element::<bool>().unwrap(), Some(false));
            assert_eq!(seq.next_element::<char>().unwrap(), Some('h'));
            assert_eq!(seq.next_element::<Enum>().unwrap(), Some(Enum::Unit));
            assert_eq!(seq.next_element::<()>().unwrap(), None);
            assert_eq!(seq.next_element::<()>().unwrap(), None);
        }
    }

    mod utf8 {
        use super::*;
        use pretty_assertions::assert_eq;

        simple!(utf8, i8_:  i8  = "-2" => -2);
        simple!(utf8, i16_: i16 = "-2" => -2);
        simple!(utf8, i32_: i32 = "-2" => -2);
        simple!(utf8, i64_: i64 = "-2" => -2);

        simple!(utf8, u8_:  u8  = "3" => 3);
        simple!(utf8, u16_: u16 = "3" => 3);
        simple!(utf8, u32_: u32 = "3" => 3);
        simple!(utf8, u64_: u64 = "3" => 3);

        serde_if_integer128! {
            simple!(utf8, i128_: i128 = "-2" => -2);
            simple!(utf8, u128_: u128 = "2" => 2);
        }

        simple!(utf8, f32_: f32 = "1.23" => 1.23);
        simple!(utf8, f64_: f64 = "1.23" => 1.23);

        simple!(utf8, false_: bool = "false" => false);
        simple!(utf8, true_: bool  = "true" => true);
        simple!(utf8, char_unescaped: char = "h" => 'h');
        simple!(utf8, char_escaped: char = "&lt;" => '<');

        simple!(utf8, string: String = "&lt;escaped string" => "<escaped string");
        err!(utf8, byte_buf: ByteBuf = "&lt;escaped&#32;string"
             => Unsupported("binary data content is not supported by XML format"));

        simple!(utf8, borrowed_str: &str = "non-escaped string" => "non-escaped string");
        err!(utf8, borrowed_bytes: Bytes = "&lt;escaped&#32;string"
             => Unsupported("binary data content is not supported by XML format"));

        simple!(utf8, option_none: Option<&str> = "" => None);
        simple!(utf8, option_some: Option<&str> = "non-escaped string" => Some("non-escaped string"));

        simple_only!(utf8, unit: () = "any data" => ());
        simple_only!(utf8, unit_struct: Unit = "any data" => Unit);

        // Serializer will not escape space because this is unnecessary.
        // Because borrowing has meaning only for deserializer, no need to test
        // roundtrip here, it is already tested for strings where compatible list
        // of escaped characters is used
        simple_only!(utf8, newtype_owned: Newtype = "&lt;escaped&#32;string"
            => Newtype("<escaped string".into()));
        simple_only!(utf8, newtype_borrowed: BorrowedNewtype = "non-escaped string"
            => BorrowedNewtype("non-escaped string"));

        err!(utf8, map: HashMap<(), ()> = "any data"
             => Unsupported("maps are not supported for XSD `simpleType`s"));
        err!(utf8, struct_: Struct = "any data"
             => Unsupported("structures are not supported for XSD `simpleType`s"));

        simple!(utf8, enum_unit: Enum = "Unit" => Enum::Unit);
        err!(utf8, enum_newtype: Enum = "Newtype"
             => Unsupported("enum newtype variants are not supported for XSD `simpleType`s"));
        err!(utf8, enum_tuple: Enum = "Tuple"
             => Unsupported("enum tuple variants are not supported for XSD `simpleType`s"));
        err!(utf8, enum_struct: Enum = "Struct"
             => Unsupported("enum struct variants are not supported for XSD `simpleType`s"));
        err!(utf8, enum_other: Enum = "any data"
             => Custom("unknown variant `any data`, expected one of `Unit`, `Newtype`, `Tuple`, `Struct`"));

        simple_only!(utf8, identifier: Id = "Field" => Id::Field);
        simple_only!(utf8, ignored_any: Any = "any data" => Any(IgnoredAny));
    }

    #[cfg(feature = "encoding")]
    mod utf16 {
        use super::*;
        use pretty_assertions::assert_eq;

        fn to_utf16(string: &str) -> Vec<u8> {
            let mut bytes = Vec::new();
            for ch in string.encode_utf16() {
                bytes.extend_from_slice(&ch.to_le_bytes());
            }
            bytes
        }

        macro_rules! utf16 {
            ($name:ident: $type:ty = $xml:literal => $result:expr) => {
                simple_only!(utf16, $name: $type = to_utf16($xml) => $result);
            };
        }

        macro_rules! unsupported {
            ($name:ident: $type:ty = $xml:literal => $err:literal) => {
                err!(utf16, $name: $type = to_utf16($xml) => Unsupported($err));
            };
        }

        utf16!(i8_:  i8  = "-2" => -2);
        utf16!(i16_: i16 = "-2" => -2);
        utf16!(i32_: i32 = "-2" => -2);
        utf16!(i64_: i64 = "-2" => -2);

        utf16!(u8_:  u8  = "3" => 3);
        utf16!(u16_: u16 = "3" => 3);
        utf16!(u32_: u32 = "3" => 3);
        utf16!(u64_: u64 = "3" => 3);

        serde_if_integer128! {
            utf16!(i128_: i128 = "-2" => -2);
            utf16!(u128_: u128 = "2" => 2);
        }

        utf16!(f32_: f32 = "1.23" => 1.23);
        utf16!(f64_: f64 = "1.23" => 1.23);

        utf16!(false_: bool = "false" => false);
        utf16!(true_: bool  = "true" => true);
        utf16!(char_unescaped: char = "h" => 'h');
        utf16!(char_escaped: char = "&lt;" => '<');

        utf16!(string: String = "&lt;escaped&#32;string" => "<escaped string");
        unsupported!(borrowed_bytes: Bytes = "&lt;escaped&#32;string"
                     => "binary data content is not supported by XML format");

        utf16!(option_none: Option<()> = "" => None);
        utf16!(option_some: Option<()> = "any data" => Some(()));

        utf16!(unit: () = "any data" => ());
        utf16!(unit_struct: Unit = "any data" => Unit);

        utf16!(newtype_owned: Newtype = "&lt;escaped&#32;string" => Newtype("<escaped string".into()));

        // UTF-16 data never borrow because data was decoded not in-place
        err!(utf16, newtype_borrowed: BorrowedNewtype = to_utf16("non-escaped string")
             => Custom("invalid type: string \"non-escaped string\", expected a borrowed string"));

        unsupported!(map: HashMap<(), ()> = "any data"
                     => "maps are not supported for XSD `simpleType`s");
        unsupported!(struct_: Struct = "any data"
                     => "structures are not supported for XSD `simpleType`s");

        utf16!(enum_unit: Enum = "Unit" => Enum::Unit);
        unsupported!(enum_newtype: Enum = "Newtype"
                     => "enum newtype variants are not supported for XSD `simpleType`s");
        unsupported!(enum_tuple: Enum = "Tuple"
                     => "enum tuple variants are not supported for XSD `simpleType`s");
        unsupported!(enum_struct: Enum = "Struct"
                     => "enum struct variants are not supported for XSD `simpleType`s");
        err!(utf16, enum_other: Enum = to_utf16("any data")
             => Custom("unknown variant `any data`, expected one of `Unit`, `Newtype`, `Tuple`, `Struct`"));

        utf16!(identifier: Id = "Field" => Id::Field);
        utf16!(ignored_any: Any = "any data" => Any(IgnoredAny));
    }
}
