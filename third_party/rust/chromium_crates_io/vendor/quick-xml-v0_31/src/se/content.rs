//! Contains serializer for content of an XML element

use crate::de::TEXT_KEY;
use crate::errors::serialize::DeError;
use crate::se::element::{ElementSerializer, Struct, Tuple};
use crate::se::simple_type::{QuoteTarget, SimpleTypeSerializer};
use crate::se::{Indent, QuoteLevel, XmlName};
use serde::ser::{
    Impossible, Serialize, SerializeSeq, SerializeTuple, SerializeTupleStruct, Serializer,
};
use serde::serde_if_integer128;
use std::fmt::Write;

macro_rules! write_primitive {
    ($method:ident ( $ty:ty )) => {
        #[inline]
        fn $method(self, value: $ty) -> Result<Self::Ok, Self::Error> {
            self.into_simple_type_serializer().$method(value)?;
            Ok(())
        }
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// A serializer used to serialize content of an element. It does not write
/// surrounding tags. Unlike the [`ElementSerializer`], this serializer serializes
/// enums using variant names as tag names, i. e. as `<variant>...</variant>`
///
/// This serializer does the following:
/// - numbers converted to a decimal representation and serialized as naked strings;
/// - booleans serialized ether as `"true"` or `"false"`;
/// - strings and characters are serialized as naked strings;
/// - `None` does not write anything;
/// - `Some` and newtypes are serialized as an inner type using the same serializer;
/// - units (`()`) and unit structs does not write anything;
/// - sequences, tuples and tuple structs are serialized without delimiters.
///   `[1, 2, 3]` would be serialized as `123` (if not using indent);
/// - structs and maps are not supported ([`DeError::Unsupported`] is returned);
/// - enums:
///   - unit variants are serialized as self-closed `<variant/>`;
///   - newtype variants are serialized as inner value wrapped in `<variant>...</variant>`;
///   - tuple variants are serialized as sequences where each element is wrapped
///     in `<variant>...</variant>`;
///   - struct variants are serialized as a sequence of fields wrapped in
///     `<variant>...</variant>`. Each field is serialized recursively using
///     either [`ElementSerializer`], `ContentSerializer` (`$value` fields), or
///     [`SimpleTypeSerializer`] (`$text` fields). In particular, the empty struct
///     is serialized as `<variant/>`;
///
/// Usage of empty tags depends on the [`Self::expand_empty_elements`] setting.
///
/// The difference between this serializer and [`SimpleTypeSerializer`] is in how
/// sequences and maps are serialized. Unlike `SimpleTypeSerializer` it supports
/// any types in sequences and serializes them as list of elements, but that has
/// drawbacks. Sequence of primitives would be serialized without delimiters and
/// it will be impossible to distinguish between them. Even worse, when serializing
/// with indent, sequence of strings become one big string with additional content
/// and it would be impossible to distinguish between content of the original
/// strings and inserted indent characters.
pub struct ContentSerializer<'w, 'i, W: Write> {
    pub writer: &'w mut W,
    /// Defines which XML characters need to be escaped in text content
    pub level: QuoteLevel,
    /// Current indentation level. Note, that `Indent::None` means that there is
    /// no indentation at all, but `write_indent == false` means only, that indent
    /// writing is disabled in this instantiation of `ContentSerializer`, but
    /// child serializers should have access to the actual state of indentation.
    pub(super) indent: Indent<'i>,
    /// If `true`, then current indent will be written before writing the content,
    /// but only if content is not empty.
    pub write_indent: bool,
    // If `true`, then empty elements will be serialized as `<element></element>`
    // instead of `<element/>`.
    pub expand_empty_elements: bool,
    //TODO: add settings to disallow consequent serialization of primitives
}

impl<'w, 'i, W: Write> ContentSerializer<'w, 'i, W> {
    /// Turns this serializer into serializer of a text content
    #[inline]
    pub fn into_simple_type_serializer(self) -> SimpleTypeSerializer<'i, &'w mut W> {
        //TODO: Customization point: choose between CDATA and Text representation
        SimpleTypeSerializer {
            writer: self.writer,
            target: QuoteTarget::Text,
            level: self.level,
            indent: if self.write_indent {
                self.indent
            } else {
                Indent::None
            },
        }
    }

    /// Creates new serializer that shares state with this serializer and
    /// writes to the same underlying writer
    #[inline]
    pub fn new_seq_element_serializer(&mut self) -> ContentSerializer<W> {
        ContentSerializer {
            writer: self.writer,
            level: self.level,
            indent: self.indent.borrow(),
            write_indent: self.write_indent,
            expand_empty_elements: self.expand_empty_elements,
        }
    }

    /// Writes `name` as self-closed tag
    #[inline]
    pub(super) fn write_empty(mut self, name: XmlName) -> Result<(), DeError> {
        self.write_indent()?;
        if self.expand_empty_elements {
            self.writer.write_char('<')?;
            self.writer.write_str(name.0)?;
            self.writer.write_str("></")?;
            self.writer.write_str(name.0)?;
            self.writer.write_char('>')?;
        } else {
            self.writer.write_str("<")?;
            self.writer.write_str(name.0)?;
            self.writer.write_str("/>")?;
        }
        Ok(())
    }

    /// Writes simple type content between `name` tags
    pub(super) fn write_wrapped<S>(mut self, name: XmlName, serialize: S) -> Result<(), DeError>
    where
        S: for<'a> FnOnce(SimpleTypeSerializer<'i, &'a mut W>) -> Result<&'a mut W, DeError>,
    {
        self.write_indent()?;
        self.writer.write_char('<')?;
        self.writer.write_str(name.0)?;
        self.writer.write_char('>')?;

        let writer = serialize(self.into_simple_type_serializer())?;

        writer.write_str("</")?;
        writer.write_str(name.0)?;
        writer.write_char('>')?;
        Ok(())
    }

    pub(super) fn write_indent(&mut self) -> Result<(), DeError> {
        if self.write_indent {
            self.indent.write_indent(&mut self.writer)?;
            self.write_indent = false;
        }
        Ok(())
    }
}

impl<'w, 'i, W: Write> Serializer for ContentSerializer<'w, 'i, W> {
    type Ok = ();
    type Error = DeError;

    type SerializeSeq = Self;
    type SerializeTuple = Self;
    type SerializeTupleStruct = Self;
    type SerializeTupleVariant = Tuple<'w, 'i, W>;
    type SerializeMap = Impossible<Self::Ok, Self::Error>;
    type SerializeStruct = Impossible<Self::Ok, Self::Error>;
    type SerializeStructVariant = Struct<'w, 'i, W>;

    write_primitive!(serialize_bool(bool));

    write_primitive!(serialize_i8(i8));
    write_primitive!(serialize_i16(i16));
    write_primitive!(serialize_i32(i32));
    write_primitive!(serialize_i64(i64));

    write_primitive!(serialize_u8(u8));
    write_primitive!(serialize_u16(u16));
    write_primitive!(serialize_u32(u32));
    write_primitive!(serialize_u64(u64));

    serde_if_integer128! {
        write_primitive!(serialize_i128(i128));
        write_primitive!(serialize_u128(u128));
    }

    write_primitive!(serialize_f32(f32));
    write_primitive!(serialize_f64(f64));

    write_primitive!(serialize_char(char));
    write_primitive!(serialize_bytes(&[u8]));

    #[inline]
    fn serialize_str(self, value: &str) -> Result<Self::Ok, Self::Error> {
        if !value.is_empty() {
            self.into_simple_type_serializer().serialize_str(value)?;
        }
        Ok(())
    }

    /// Does not write anything
    #[inline]
    fn serialize_none(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }

    fn serialize_some<T: ?Sized + Serialize>(self, value: &T) -> Result<Self::Ok, Self::Error> {
        value.serialize(self)
    }

    /// Does not write anything
    #[inline]
    fn serialize_unit(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }

    /// Does not write anything
    #[inline]
    fn serialize_unit_struct(self, _name: &'static str) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }

    /// If `variant` is a special `$text` variant, then do nothing, otherwise
    /// checks `variant` for XML name validity and writes `<variant/>`.
    fn serialize_unit_variant(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
    ) -> Result<Self::Ok, Self::Error> {
        if variant == TEXT_KEY {
            Ok(())
        } else {
            let name = XmlName::try_from(variant)?;
            self.write_empty(name)
        }
    }

    fn serialize_newtype_struct<T: ?Sized + Serialize>(
        self,
        _name: &'static str,
        value: &T,
    ) -> Result<Self::Ok, Self::Error> {
        value.serialize(self)
    }

    /// If `variant` is a special `$text` variant, then writes `value` as a `xs:simpleType`,
    /// otherwise checks `variant` for XML name validity and writes `value` as a new
    /// `<variant>` element.
    fn serialize_newtype_variant<T: ?Sized + Serialize>(
        self,
        _name: &'static str,
        _variant_index: u32,
        variant: &'static str,
        value: &T,
    ) -> Result<Self::Ok, Self::Error> {
        if variant == TEXT_KEY {
            value.serialize(self.into_simple_type_serializer())?;
            Ok(())
        } else {
            value.serialize(ElementSerializer {
                key: XmlName::try_from(variant)?,
                ser: self,
            })
        }
    }

    #[inline]
    fn serialize_seq(self, _len: Option<usize>) -> Result<Self::SerializeSeq, Self::Error> {
        Ok(self)
    }

    #[inline]
    fn serialize_tuple(self, len: usize) -> Result<Self::SerializeTuple, Self::Error> {
        self.serialize_seq(Some(len))
    }

    #[inline]
    fn serialize_tuple_struct(
        self,
        _name: &'static str,
        len: usize,
    ) -> Result<Self::SerializeTupleStruct, Self::Error> {
        self.serialize_tuple(len)
    }

    /// Serializes variant as a tuple with name `variant`, producing
    ///
    /// ```xml
    /// <variant><!-- 1st element of a tuple --></variant>
    /// <variant><!-- 2nd element of a tuple --></variant>
    /// <!-- ... -->
    /// <variant><!-- Nth element of a tuple --></variant>
    /// ```
    #[inline]
    fn serialize_tuple_variant(
        self,
        name: &'static str,
        _variant_index: u32,
        variant: &'static str,
        len: usize,
    ) -> Result<Self::SerializeTupleVariant, Self::Error> {
        if variant == TEXT_KEY {
            self.into_simple_type_serializer()
                .serialize_tuple_struct(name, len)
                .map(Tuple::Text)
        } else {
            let ser = ElementSerializer {
                key: XmlName::try_from(variant)?,
                ser: self,
            };
            ser.serialize_tuple_struct(name, len).map(Tuple::Element)
        }
    }

    fn serialize_map(self, _len: Option<usize>) -> Result<Self::SerializeMap, Self::Error> {
        Err(DeError::Unsupported(
            format!("serialization of map types is not supported in `$value` field").into(),
        ))
    }

    #[inline]
    fn serialize_struct(
        self,
        name: &'static str,
        _len: usize,
    ) -> Result<Self::SerializeStruct, Self::Error> {
        Err(DeError::Unsupported(
            format!("serialization of struct `{name}` is not supported in `$value` field").into(),
        ))
    }

    /// Serializes variant as an element with name `variant`, producing
    ///
    /// ```xml
    /// <variant>
    ///   <!-- struct fields... -->
    /// </variant>
    /// ```
    ///
    /// If struct has no fields which is represented by nested elements or a text,
    /// it may be serialized as self-closed element `<variant/>`.
    #[inline]
    fn serialize_struct_variant(
        self,
        name: &'static str,
        _variant_index: u32,
        variant: &'static str,
        len: usize,
    ) -> Result<Self::SerializeStructVariant, Self::Error> {
        if variant == TEXT_KEY {
            Err(DeError::Unsupported(
                format!("cannot serialize `$text` struct variant of `{}` enum", name).into(),
            ))
        } else {
            let ser = ElementSerializer {
                key: XmlName::try_from(variant)?,
                ser: self,
            };
            ser.serialize_struct(name, len)
        }
    }
}

impl<'w, 'i, W: Write> SerializeSeq for ContentSerializer<'w, 'i, W> {
    type Ok = ();
    type Error = DeError;

    fn serialize_element<T>(&mut self, value: &T) -> Result<(), Self::Error>
    where
        T: ?Sized + Serialize,
    {
        value.serialize(self.new_seq_element_serializer())?;
        // Write indent for next element
        self.write_indent = true;
        Ok(())
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        Ok(())
    }
}

impl<'w, 'i, W: Write> SerializeTuple for ContentSerializer<'w, 'i, W> {
    type Ok = ();
    type Error = DeError;

    #[inline]
    fn serialize_element<T>(&mut self, value: &T) -> Result<(), Self::Error>
    where
        T: ?Sized + Serialize,
    {
        SerializeSeq::serialize_element(self, value)
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        SerializeSeq::end(self)
    }
}

impl<'w, 'i, W: Write> SerializeTupleStruct for ContentSerializer<'w, 'i, W> {
    type Ok = ();
    type Error = DeError;

    #[inline]
    fn serialize_field<T>(&mut self, value: &T) -> Result<(), Self::Error>
    where
        T: ?Sized + Serialize,
    {
        SerializeSeq::serialize_element(self, value)
    }

    #[inline]
    fn end(self) -> Result<Self::Ok, Self::Error> {
        SerializeSeq::end(self)
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/// Make tests public to reuse types in `elements::tests` module
#[cfg(test)]
pub(super) mod tests {
    use super::*;
    use crate::utils::Bytes;
    use serde::Serialize;
    use std::collections::BTreeMap;

    #[derive(Debug, Serialize, PartialEq)]
    pub struct Unit;

    #[derive(Debug, Serialize, PartialEq)]
    #[serde(rename = "<\"&'>")]
    pub struct UnitEscaped;

    #[derive(Debug, Serialize, PartialEq)]
    pub struct Newtype(pub usize);

    #[derive(Debug, Serialize, PartialEq)]
    pub struct Tuple(pub &'static str, pub usize);

    #[derive(Debug, Serialize, PartialEq)]
    pub struct Struct {
        pub key: &'static str,
        pub val: (usize, usize),
    }

    /// Struct with a special `$text` field
    #[derive(Debug, Serialize, PartialEq)]
    pub struct Text<T> {
        pub before: &'static str,
        #[serde(rename = "$text")]
        pub content: T,
        pub after: &'static str,
    }

    /// Struct with a special `$value` field
    #[derive(Debug, Serialize, PartialEq)]
    pub struct Value<T> {
        pub before: &'static str,
        #[serde(rename = "$value")]
        pub content: T,
        pub after: &'static str,
    }

    /// Attributes identified by starting with `@` character
    #[derive(Debug, Serialize, PartialEq)]
    pub struct Attributes {
        #[serde(rename = "@key")]
        pub key: &'static str,
        #[serde(rename = "@val")]
        pub val: (usize, usize),
    }
    #[derive(Debug, Serialize, PartialEq)]
    pub struct AttributesBefore {
        #[serde(rename = "@key")]
        pub key: &'static str,
        pub val: usize,
    }
    #[derive(Debug, Serialize, PartialEq)]
    pub struct AttributesAfter {
        pub key: &'static str,
        #[serde(rename = "@val")]
        pub val: usize,
    }

    #[derive(Debug, Serialize, PartialEq)]
    pub enum Enum {
        Unit,
        /// Variant name becomes a tag name, but the name of variant is invalid
        /// XML name. Serialization of this element should be forbidden
        #[serde(rename = "<\"&'>")]
        UnitEscaped,
        Newtype(usize),
        Tuple(&'static str, usize),
        Struct {
            key: &'static str,
            /// Should be serialized as elements
            val: (usize, usize),
        },
        Attributes {
            #[serde(rename = "@key")]
            key: &'static str,
            #[serde(rename = "@val")]
            val: (usize, usize),
        },
        AttributesBefore {
            #[serde(rename = "@key")]
            key: &'static str,
            val: usize,
        },
        AttributesAfter {
            key: &'static str,
            #[serde(rename = "@val")]
            val: usize,
        },
    }

    #[derive(Debug, Serialize, PartialEq)]
    pub enum SpecialEnum<T> {
        /// Struct variant with a special `$text` field
        Text {
            before: &'static str,
            #[serde(rename = "$text")]
            content: T,
            after: &'static str,
        },
        /// Struct variant with a special `$value` field
        Value {
            before: &'static str,
            #[serde(rename = "$value")]
            content: T,
            after: &'static str,
        },
    }

    mod without_indent {
        use super::Struct;
        use super::*;
        use pretty_assertions::assert_eq;

        /// Checks that given `$data` successfully serialized as `$expected`
        macro_rules! serialize_as {
            ($name:ident: $data:expr => $expected:expr) => {
                #[test]
                fn $name() {
                    let mut buffer = String::new();
                    let ser = ContentSerializer {
                        writer: &mut buffer,
                        level: QuoteLevel::Full,
                        indent: Indent::None,
                        write_indent: false,
                        expand_empty_elements: false,
                    };

                    $data.serialize(ser).unwrap();
                    assert_eq!(buffer, $expected);
                }
            };
        }

        /// Checks that attempt to serialize given `$data` results to a
        /// serialization error `$kind` with `$reason`
        macro_rules! err {
            ($name:ident: $data:expr => $kind:ident($reason:literal)) => {
                #[test]
                fn $name() {
                    let mut buffer = String::new();
                    let ser = ContentSerializer {
                        writer: &mut buffer,
                        level: QuoteLevel::Full,
                        indent: Indent::None,
                        write_indent: false,
                        expand_empty_elements: false,
                    };

                    match $data.serialize(ser).unwrap_err() {
                        DeError::$kind(e) => assert_eq!(e, $reason),
                        e => panic!(
                            "Expected `{}({})`, found `{:?}`",
                            stringify!($kind),
                            $reason,
                            e
                        ),
                    }
                    // We could write something before fail
                    // assert_eq!(buffer, "");
                }
            };
        }

        // Primitives is serialized in the same way as for SimpleTypeSerializer
        serialize_as!(false_: false => "false");
        serialize_as!(true_:  true  => "true");

        serialize_as!(i8_:    -42i8                => "-42");
        serialize_as!(i16_:   -4200i16             => "-4200");
        serialize_as!(i32_:   -42000000i32         => "-42000000");
        serialize_as!(i64_:   -42000000000000i64   => "-42000000000000");
        serialize_as!(isize_: -42000000000000isize => "-42000000000000");

        serialize_as!(u8_:    42u8                => "42");
        serialize_as!(u16_:   4200u16             => "4200");
        serialize_as!(u32_:   42000000u32         => "42000000");
        serialize_as!(u64_:   42000000000000u64   => "42000000000000");
        serialize_as!(usize_: 42000000000000usize => "42000000000000");

        serde_if_integer128! {
            serialize_as!(i128_: -420000000000000000000000000000i128 => "-420000000000000000000000000000");
            serialize_as!(u128_:  420000000000000000000000000000u128 => "420000000000000000000000000000");
        }

        serialize_as!(f32_: 4.2f32 => "4.2");
        serialize_as!(f64_: 4.2f64 => "4.2");

        serialize_as!(char_non_escaped: 'h' => "h");
        serialize_as!(char_lt:   '<' => "&lt;");
        serialize_as!(char_gt:   '>' => "&gt;");
        serialize_as!(char_amp:  '&' => "&amp;");
        serialize_as!(char_apos: '\'' => "&apos;");
        serialize_as!(char_quot: '"' => "&quot;");
        //TODO: add a setting to escape leading/trailing spaces, in order to
        // pretty-print does not change the content
        serialize_as!(char_space: ' ' => " ");

        serialize_as!(str_non_escaped: "non-escaped string" => "non-escaped string");
        serialize_as!(str_escaped: "<\"escaped & string'>" => "&lt;&quot;escaped &amp; string&apos;&gt;");

        err!(bytes: Bytes(b"<\"escaped & bytes'>") => Unsupported("`serialize_bytes` not supported yet"));

        serialize_as!(option_none: Option::<Enum>::None => "");
        serialize_as!(option_some: Some("non-escaped string") => "non-escaped string");
        serialize_as!(option_some_empty_str: Some("") => "");

        serialize_as!(unit: () => "");
        serialize_as!(unit_struct: Unit => "");
        serialize_as!(unit_struct_escaped: UnitEscaped => "");

        // Unlike SimpleTypeSerializer, enumeration values serialized as tags
        serialize_as!(enum_unit: Enum::Unit => "<Unit/>");
        err!(enum_unit_escaped: Enum::UnitEscaped
            => Unsupported("character `<` is not allowed at the start of an XML name `<\"&'>`"));

        // Newtypes recursively applies ContentSerializer
        serialize_as!(newtype: Newtype(42) => "42");
        serialize_as!(enum_newtype: Enum::Newtype(42) => "<Newtype>42</Newtype>");

        // Note that sequences of primitives serialized without delimiters!
        serialize_as!(seq: vec![1, 2, 3] => "123");
        serialize_as!(seq_empty: Vec::<usize>::new() => "");
        serialize_as!(tuple: ("<\"&'>", "with\t\r\n spaces", 3usize)
            => "&lt;&quot;&amp;&apos;&gt;\
                with\t\r\n spaces\
                3");
        serialize_as!(tuple_struct: Tuple("first", 42)
            => "first\
                42");
        serialize_as!(enum_tuple: Enum::Tuple("first", 42)
            => "<Tuple>first</Tuple>\
                <Tuple>42</Tuple>");

        // Structured types cannot be serialized without surrounding tag, which
        // only `enum` can provide
        err!(map: BTreeMap::from([("_1", 2), ("_3", 4)])
            => Unsupported("serialization of map types is not supported in `$value` field"));
        err!(struct_: Struct { key: "answer", val: (42, 42) }
            => Unsupported("serialization of struct `Struct` is not supported in `$value` field"));
        serialize_as!(enum_struct: Enum::Struct { key: "answer", val: (42, 42) }
            => "<Struct>\
                    <key>answer</key>\
                    <val>42</val>\
                    <val>42</val>\
                </Struct>");

        /// Special field name `$text` should be serialized as a text content
        mod text_field {
            use super::*;
            use pretty_assertions::assert_eq;

            err!(map: BTreeMap::from([("$text", 2), ("_3", 4)])
                => Unsupported("serialization of map types is not supported in `$value` field"));
            err!(struct_:
                Text {
                    before: "answer",
                    content: (42, 42),
                    after: "answer",
                }
                => Unsupported("serialization of struct `Text` is not supported in `$value` field"));
            serialize_as!(enum_struct:
                SpecialEnum::Text {
                    before: "answer",
                    content: (42, 42),
                    after: "answer",
                }
                => "<Text>\
                        <before>answer</before>\
                        42 42\
                        <after>answer</after>\
                    </Text>");
        }

        /// `$text` field inside a struct variant of an enum
        mod enum_with_text_field {
            use super::*;
            use pretty_assertions::assert_eq;

            macro_rules! text {
                ($name:ident: $data:expr => $expected:literal) => {
                    serialize_as!($name:
                        SpecialEnum::Text {
                            before: "answer",
                            content: $data,
                            after: "answer",
                        }
                        => concat!(
                            "<Text><before>answer</before>",
                            $expected,
                            "<after>answer</after></Text>",
                        ));
                };
            }

            text!(false_: false => "false");
            text!(true_:  true  => "true");

            text!(i8_:    -42i8                => "-42");
            text!(i16_:   -4200i16             => "-4200");
            text!(i32_:   -42000000i32         => "-42000000");
            text!(i64_:   -42000000000000i64   => "-42000000000000");
            text!(isize_: -42000000000000isize => "-42000000000000");

            text!(u8_:    42u8                => "42");
            text!(u16_:   4200u16             => "4200");
            text!(u32_:   42000000u32         => "42000000");
            text!(u64_:   42000000000000u64   => "42000000000000");
            text!(usize_: 42000000000000usize => "42000000000000");

            serde_if_integer128! {
                text!(i128_: -420000000000000000000000000000i128 => "-420000000000000000000000000000");
                text!(u128_:  420000000000000000000000000000u128 => "420000000000000000000000000000");
            }

            text!(f32_: 4.2f32 => "4.2");
            text!(f64_: 4.2f64 => "4.2");

            text!(char_non_escaped: 'h' => "h");
            text!(char_lt:   '<' => "&lt;");
            text!(char_gt:   '>' => "&gt;");
            text!(char_amp:  '&' => "&amp;");
            text!(char_apos: '\'' => "&apos;");
            text!(char_quot: '"' => "&quot;");
            //TODO: add a setting to escape leading/trailing spaces, in order to
            // pretty-print does not change the content
            text!(char_space: ' ' => " ");

            text!(str_non_escaped: "non-escaped string" => "non-escaped string");
            text!(str_escaped: "<\"escaped & string'>" => "&lt;&quot;escaped &amp; string&apos;&gt;");

            err!(bytes:
                SpecialEnum::Text {
                    before: "answer",
                    content: Bytes(b"<\"escaped & bytes'>"),
                    after: "answer",
                }
                => Unsupported("`serialize_bytes` not supported yet"));

            text!(option_none: Option::<&str>::None => "");
            text!(option_some: Some("non-escaped string") => "non-escaped string");
            text!(option_some_empty_str: Some("") => "");

            text!(unit: () => "");
            text!(unit_struct: Unit => "");
            text!(unit_struct_escaped: UnitEscaped => "");

            text!(enum_unit: Enum::Unit => "Unit");
            text!(enum_unit_escaped: Enum::UnitEscaped => "&lt;&quot;&amp;&apos;&gt;");

            text!(newtype: Newtype(42) => "42");
            // We have no space where name of a variant can be stored
            err!(enum_newtype:
                SpecialEnum::Text {
                    before: "answer",
                    content: Enum::Newtype(42),
                    after: "answer",
                }
                => Unsupported("cannot serialize enum newtype variant `Enum::Newtype` as text content value"));

            // Sequences are serialized separated by spaces, all spaces inside are escaped
            text!(seq: vec![1, 2, 3] => "1 2 3");
            text!(seq_empty: Vec::<usize>::new() => "");
            text!(tuple: ("<\"&'>", "with\t\n\r spaces", 3usize)
                => "&lt;&quot;&amp;&apos;&gt; \
                    with&#9;&#10;&#13;&#32;spaces \
                    3");
            text!(tuple_struct: Tuple("first", 42) => "first 42");
            // We have no space where name of a variant can be stored
            err!(enum_tuple:
                SpecialEnum::Text {
                    before: "answer",
                    content: Enum::Tuple("first", 42),
                    after: "answer",
                }
                => Unsupported("cannot serialize enum tuple variant `Enum::Tuple` as text content value"));

            // Complex types cannot be serialized in `$text` field
            err!(map:
                SpecialEnum::Text {
                    before: "answer",
                    content: BTreeMap::from([("_1", 2), ("_3", 4)]),
                    after: "answer",
                }
                => Unsupported("cannot serialize map as text content value"));
            err!(struct_:
                SpecialEnum::Text {
                    before: "answer",
                    content: Struct { key: "answer", val: (42, 42) },
                    after: "answer",
                }
                => Unsupported("cannot serialize struct `Struct` as text content value"));
            err!(enum_struct:
                SpecialEnum::Text {
                    before: "answer",
                    content: Enum::Struct { key: "answer", val: (42, 42) },
                    after: "answer",
                }
                => Unsupported("cannot serialize enum struct variant `Enum::Struct` as text content value"));
        }

        /// `$value` field inside a struct variant of an enum
        mod enum_with_value_field {
            use super::*;
            use pretty_assertions::assert_eq;

            macro_rules! value {
                ($name:ident: $data:expr => $expected:literal) => {
                    serialize_as!($name:
                        SpecialEnum::Value {
                            before: "answer",
                            content: $data,
                            after: "answer",
                        }
                        => concat!(
                            "<Value><before>answer</before>",
                            $expected,
                            "<after>answer</after></Value>",
                        ));
                };
            }

            value!(false_: false => "false");
            value!(true_:  true  => "true");

            value!(i8_:    -42i8                => "-42");
            value!(i16_:   -4200i16             => "-4200");
            value!(i32_:   -42000000i32         => "-42000000");
            value!(i64_:   -42000000000000i64   => "-42000000000000");
            value!(isize_: -42000000000000isize => "-42000000000000");

            value!(u8_:    42u8                => "42");
            value!(u16_:   4200u16             => "4200");
            value!(u32_:   42000000u32         => "42000000");
            value!(u64_:   42000000000000u64   => "42000000000000");
            value!(usize_: 42000000000000usize => "42000000000000");

            serde_if_integer128! {
                value!(i128_: -420000000000000000000000000000i128 => "-420000000000000000000000000000");
                value!(u128_:  420000000000000000000000000000u128 => "420000000000000000000000000000");
            }

            value!(f32_: 4.2f32 => "4.2");
            value!(f64_: 4.2f64 => "4.2");

            value!(char_non_escaped: 'h' => "h");
            value!(char_lt:   '<' => "&lt;");
            value!(char_gt:   '>' => "&gt;");
            value!(char_amp:  '&' => "&amp;");
            value!(char_apos: '\'' => "&apos;");
            value!(char_quot: '"' => "&quot;");
            //TODO: add a setting to escape leading/trailing spaces, in order to
            // pretty-print does not change the content
            value!(char_space: ' ' => " ");

            value!(str_non_escaped: "non-escaped string" => "non-escaped string");
            value!(str_escaped: "<\"escaped & string'>" => "&lt;&quot;escaped &amp; string&apos;&gt;");

            err!(bytes:
                SpecialEnum::Value {
                    before: "answer",
                    content: Bytes(b"<\"escaped & bytes'>"),
                    after: "answer",
                }
                => Unsupported("`serialize_bytes` not supported yet"));

            value!(option_none: Option::<&str>::None => "");
            value!(option_some: Some("non-escaped string") => "non-escaped string");
            value!(option_some_empty_str: Some("") => "");

            value!(unit: () => "");
            value!(unit_struct: Unit => "");
            value!(unit_struct_escaped: UnitEscaped => "");

            value!(enum_unit: Enum::Unit => "<Unit/>");
            err!(enum_unit_escaped:
                SpecialEnum::Value {
                    before: "answer",
                    content: Enum::UnitEscaped,
                    after: "answer",
                }
                => Unsupported("character `<` is not allowed at the start of an XML name `<\"&'>`"));

            value!(newtype: Newtype(42) => "42");
            value!(enum_newtype: Enum::Newtype(42) => "<Newtype>42</Newtype>");

            // Note that sequences of primitives serialized without delimiters!
            value!(seq: vec![1, 2, 3] => "123");
            value!(seq_empty: Vec::<usize>::new() => "");
            value!(tuple: ("<\"&'>", "with\t\n\r spaces", 3usize)
                => "&lt;&quot;&amp;&apos;&gt;\
                    with\t\n\r spaces\
                    3");
            value!(tuple_struct: Tuple("first", 42) => "first42");
            value!(enum_tuple: Enum::Tuple("first", 42)
                => "<Tuple>first</Tuple>\
                    <Tuple>42</Tuple>");

            // We cannot wrap map or struct in any container and should not
            // flatten it, so it is impossible to serialize maps and structs
            err!(map:
                SpecialEnum::Value {
                    before: "answer",
                    content: BTreeMap::from([("_1", 2), ("_3", 4)]),
                    after: "answer",
                }
                => Unsupported("serialization of map types is not supported in `$value` field"));
            err!(struct_:
                SpecialEnum::Value {
                    before: "answer",
                    content: Struct { key: "answer", val: (42, 42) },
                    after: "answer",
                }
                => Unsupported("serialization of struct `Struct` is not supported in `$value` field"));
            value!(enum_struct:
                Enum::Struct { key: "answer", val: (42, 42) }
                => "<Struct>\
                        <key>answer</key>\
                        <val>42</val>\
                        <val>42</val>\
                    </Struct>");
        }

        mod attributes {
            use super::*;
            use pretty_assertions::assert_eq;

            err!(map_attr: BTreeMap::from([("@key1", 1), ("@key2", 2)])
                => Unsupported("serialization of map types is not supported in `$value` field"));
            err!(map_mixed: BTreeMap::from([("@key1", 1), ("key2", 2)])
                => Unsupported("serialization of map types is not supported in `$value` field"));

            err!(struct_: Attributes { key: "answer", val: (42, 42) }
                => Unsupported("serialization of struct `Attributes` is not supported in `$value` field"));
            err!(struct_before: AttributesBefore { key: "answer", val: 42 }
                => Unsupported("serialization of struct `AttributesBefore` is not supported in `$value` field"));
            err!(struct_after: AttributesAfter { key: "answer", val: 42 }
                => Unsupported("serialization of struct `AttributesAfter` is not supported in `$value` field"));

            serialize_as!(enum_: Enum::Attributes { key: "answer", val: (42, 42) }
                => r#"<Attributes key="answer" val="42 42"/>"#);
            serialize_as!(enum_before: Enum::AttributesBefore { key: "answer", val: 42 }
                => r#"<AttributesBefore key="answer"><val>42</val></AttributesBefore>"#);
            serialize_as!(enum_after: Enum::AttributesAfter { key: "answer", val: 42 }
                => r#"<AttributesAfter val="42"><key>answer</key></AttributesAfter>"#);
        }
    }

    mod with_indent {
        use super::Struct;
        use super::*;
        use crate::writer::Indentation;
        use pretty_assertions::assert_eq;

        /// Checks that given `$data` successfully serialized as `$expected`
        macro_rules! serialize_as {
            ($name:ident: $data:expr => $expected:expr) => {
                #[test]
                fn $name() {
                    let mut buffer = String::new();
                    let ser = ContentSerializer {
                        writer: &mut buffer,
                        level: QuoteLevel::Full,
                        indent: Indent::Owned(Indentation::new(b' ', 2)),
                        write_indent: false,
                        expand_empty_elements: false,
                    };

                    $data.serialize(ser).unwrap();
                    assert_eq!(buffer, $expected);
                }
            };
        }

        /// Checks that attempt to serialize given `$data` results to a
        /// serialization error `$kind` with `$reason`
        macro_rules! err {
            ($name:ident: $data:expr => $kind:ident($reason:literal)) => {
                #[test]
                fn $name() {
                    let mut buffer = String::new();
                    let ser = ContentSerializer {
                        writer: &mut buffer,
                        level: QuoteLevel::Full,
                        indent: Indent::Owned(Indentation::new(b' ', 2)),
                        write_indent: false,
                        expand_empty_elements: false,
                    };

                    match $data.serialize(ser).unwrap_err() {
                        DeError::$kind(e) => assert_eq!(e, $reason),
                        e => panic!(
                            "Expected `{}({})`, found `{:?}`",
                            stringify!($kind),
                            $reason,
                            e
                        ),
                    }
                    // We can write something before fail
                    // assert_eq!(buffer, "");
                }
            };
        }

        serialize_as!(false_: false => "false");
        serialize_as!(true_:  true  => "true");

        serialize_as!(i8_:    -42i8                => "-42");
        serialize_as!(i16_:   -4200i16             => "-4200");
        serialize_as!(i32_:   -42000000i32         => "-42000000");
        serialize_as!(i64_:   -42000000000000i64   => "-42000000000000");
        serialize_as!(isize_: -42000000000000isize => "-42000000000000");

        serialize_as!(u8_:    42u8                => "42");
        serialize_as!(u16_:   4200u16             => "4200");
        serialize_as!(u32_:   42000000u32         => "42000000");
        serialize_as!(u64_:   42000000000000u64   => "42000000000000");
        serialize_as!(usize_: 42000000000000usize => "42000000000000");

        serde_if_integer128! {
            serialize_as!(i128_: -420000000000000000000000000000i128 => "-420000000000000000000000000000");
            serialize_as!(u128_:  420000000000000000000000000000u128 => "420000000000000000000000000000");
        }

        serialize_as!(f32_: 4.2f32 => "4.2");
        serialize_as!(f64_: 4.2f64 => "4.2");

        serialize_as!(char_non_escaped: 'h' => "h");
        serialize_as!(char_lt:   '<' => "&lt;");
        serialize_as!(char_gt:   '>' => "&gt;");
        serialize_as!(char_amp:  '&' => "&amp;");
        serialize_as!(char_apos: '\'' => "&apos;");
        serialize_as!(char_quot: '"' => "&quot;");
        //TODO: add a setting to escape leading/trailing spaces, in order to
        // pretty-print does not change the content
        serialize_as!(char_space: ' ' => " ");

        serialize_as!(str_non_escaped: "non-escaped string" => "non-escaped string");
        serialize_as!(str_escaped: "<\"escaped & string'>" => "&lt;&quot;escaped &amp; string&apos;&gt;");

        err!(bytes: Bytes(b"<\"escaped & bytes'>") => Unsupported("`serialize_bytes` not supported yet"));

        serialize_as!(option_none: Option::<Enum>::None => "");
        serialize_as!(option_some: Some(Enum::Unit) => "<Unit/>");

        serialize_as!(unit: () => "");
        serialize_as!(unit_struct: Unit => "");
        serialize_as!(unit_struct_escaped: UnitEscaped => "");

        // Unlike SimpleTypeSerializer, enumeration values serialized as tags
        serialize_as!(enum_unit: Enum::Unit => "<Unit/>");
        err!(enum_unit_escaped: Enum::UnitEscaped
            => Unsupported("character `<` is not allowed at the start of an XML name `<\"&'>`"));

        // Newtypes recursively applies ContentSerializer
        serialize_as!(newtype: Newtype(42) => "42");
        serialize_as!(enum_newtype: Enum::Newtype(42) => "<Newtype>42</Newtype>");

        // Note that sequences of primitives serialized without delimiters other that indent!
        serialize_as!(seq: vec![1, 2, 3]
            => "1\n\
                2\n\
                3");
        serialize_as!(seq_empty: Vec::<usize>::new() => "");
        serialize_as!(tuple: ("<\"&'>", "with\t\r\n spaces", 3usize)
            => "&lt;&quot;&amp;&apos;&gt;\n\
                with\t\r\n spaces\n\
                3");
        serialize_as!(tuple_struct: Tuple("first", 42)
            => "first\n\
                42");
        serialize_as!(enum_tuple: Enum::Tuple("first", 42)
            => "<Tuple>first</Tuple>\n\
                <Tuple>42</Tuple>");

        // Structured types cannot be serialized without surrounding tag, which
        // only `enum` can provide
        err!(map: BTreeMap::from([("_1", 2), ("_3", 4)])
            => Unsupported("serialization of map types is not supported in `$value` field"));
        err!(struct_: Struct { key: "answer", val: (42, 42) }
            => Unsupported("serialization of struct `Struct` is not supported in `$value` field"));
        serialize_as!(enum_struct: Enum::Struct { key: "answer", val: (42, 42) }
            => "<Struct>\n  \
                    <key>answer</key>\n  \
                    <val>42</val>\n  \
                    <val>42</val>\n\
                </Struct>");

        /// Special field name `$text` should be serialized as text content
        mod text_field {
            use super::*;
            use pretty_assertions::assert_eq;

            err!(map: BTreeMap::from([("$text", 2), ("_3", 4)])
                => Unsupported("serialization of map types is not supported in `$value` field"));
            err!(struct_:
                Text {
                    before: "answer",
                    content: (42, 42),
                    after: "answer",
                }
                => Unsupported("serialization of struct `Text` is not supported in `$value` field"));
            serialize_as!(enum_struct:
                SpecialEnum::Text {
                    before: "answer",
                    content: (42, 42),
                    after: "answer",
                }
                => "<Text>\n  \
                        <before>answer</before>\n  \
                        42 42\n  \
                        <after>answer</after>\n\
                    </Text>");
        }

        /// `$text` field inside a struct variant of an enum
        mod enum_with_text_field {
            use super::*;
            use pretty_assertions::assert_eq;

            macro_rules! text {
                ($name:ident: $data:expr) => {
                    serialize_as!($name:
                        SpecialEnum::Text {
                            before: "answer",
                            content: $data,
                            after: "answer",
                        }
                        => "<Text>\n  \
                                <before>answer</before>\n  \
                                <after>answer</after>\n\
                            </Text>");
                };
                ($name:ident: $data:expr => $expected:literal) => {
                    serialize_as!($name:
                        SpecialEnum::Text {
                            before: "answer",
                            content: $data,
                            after: "answer",
                        }
                        => concat!(
                            "<Text>\n  <before>answer</before>\n  ",
                            $expected,
                            "\n  <after>answer</after>\n</Text>",
                        ));
                };
            }

            text!(false_: false => "false");
            text!(true_:  true  => "true");

            text!(i8_:    -42i8                => "-42");
            text!(i16_:   -4200i16             => "-4200");
            text!(i32_:   -42000000i32         => "-42000000");
            text!(i64_:   -42000000000000i64   => "-42000000000000");
            text!(isize_: -42000000000000isize => "-42000000000000");

            text!(u8_:    42u8                => "42");
            text!(u16_:   4200u16             => "4200");
            text!(u32_:   42000000u32         => "42000000");
            text!(u64_:   42000000000000u64   => "42000000000000");
            text!(usize_: 42000000000000usize => "42000000000000");

            serde_if_integer128! {
                text!(i128_: -420000000000000000000000000000i128 => "-420000000000000000000000000000");
                text!(u128_:  420000000000000000000000000000u128 => "420000000000000000000000000000");
            }

            text!(f32_: 4.2f32 => "4.2");
            text!(f64_: 4.2f64 => "4.2");

            text!(char_non_escaped: 'h' => "h");
            text!(char_lt:   '<' => "&lt;");
            text!(char_gt:   '>' => "&gt;");
            text!(char_amp:  '&' => "&amp;");
            text!(char_apos: '\'' => "&apos;");
            text!(char_quot: '"' => "&quot;");
            //TODO: add a setting to escape leading/trailing spaces, in order to
            // pretty-print does not change the content
            text!(char_space: ' ' => " ");

            text!(str_non_escaped: "non-escaped string" => "non-escaped string");
            text!(str_escaped: "<\"escaped & string'>" => "&lt;&quot;escaped &amp; string&apos;&gt;");

            err!(bytes:
                SpecialEnum::Text {
                    before: "answer",
                    content: Bytes(b"<\"escaped & bytes'>"),
                    after: "answer",
                }
                => Unsupported("`serialize_bytes` not supported yet"));

            text!(option_none: Option::<&str>::None);
            text!(option_some: Some("non-escaped string") => "non-escaped string");
            text!(option_some_empty_str: Some(""));

            text!(unit: ());
            text!(unit_struct: Unit);
            text!(unit_struct_escaped: UnitEscaped);

            text!(enum_unit: Enum::Unit => "Unit");
            text!(enum_unit_escaped: Enum::UnitEscaped => "&lt;&quot;&amp;&apos;&gt;");

            text!(newtype: Newtype(42) => "42");
            // We have no space where name of a variant can be stored
            err!(enum_newtype:
                SpecialEnum::Text {
                    before: "answer",
                    content: Enum::Newtype(42),
                    after: "answer",
                }
                => Unsupported("cannot serialize enum newtype variant `Enum::Newtype` as text content value"));

            // Sequences are serialized separated by spaces, all spaces inside are escaped
            text!(seq: vec![1, 2, 3] => "1 2 3");
            text!(seq_empty: Vec::<usize>::new());
            text!(tuple: ("<\"&'>", "with\t\n\r spaces", 3usize)
                => "&lt;&quot;&amp;&apos;&gt; \
                    with&#9;&#10;&#13;&#32;spaces \
                    3");
            text!(tuple_struct: Tuple("first", 42) => "first 42");
            // We have no space where name of a variant can be stored
            err!(enum_tuple:
                SpecialEnum::Text {
                    before: "answer",
                    content: Enum::Tuple("first", 42),
                    after: "answer",
                }
                => Unsupported("cannot serialize enum tuple variant `Enum::Tuple` as text content value"));

            // Complex types cannot be serialized in `$text` field
            err!(map:
                SpecialEnum::Text {
                    before: "answer",
                    content: BTreeMap::from([("_1", 2), ("_3", 4)]),
                    after: "answer",
                }
                => Unsupported("cannot serialize map as text content value"));
            err!(struct_:
                SpecialEnum::Text {
                    before: "answer",
                    content: Struct { key: "answer", val: (42, 42) },
                    after: "answer",
                }
                => Unsupported("cannot serialize struct `Struct` as text content value"));
            err!(enum_struct:
                SpecialEnum::Text {
                    before: "answer",
                    content: Enum::Struct { key: "answer", val: (42, 42) },
                    after: "answer",
                }
                => Unsupported("cannot serialize enum struct variant `Enum::Struct` as text content value"));
        }

        /// `$value` field inside a struct variant of an enum
        mod enum_with_value_field {
            use super::*;
            use pretty_assertions::assert_eq;

            macro_rules! value {
                ($name:ident: $data:expr) => {
                    serialize_as!($name:
                        SpecialEnum::Value {
                            before: "answer",
                            content: $data,
                            after: "answer",
                        }
                        => "<Value>\n  \
                                <before>answer</before>\n  \
                                <after>answer</after>\n\
                            </Value>");
                };
                ($name:ident: $data:expr => $expected:literal) => {
                    serialize_as!($name:
                        SpecialEnum::Value {
                            before: "answer",
                            content: $data,
                            after: "answer",
                        }
                        => concat!(
                            "<Value>\n  <before>answer</before>\n  ",
                            $expected,
                            "\n  <after>answer</after>\n</Value>",
                        ));
                };
            }

            value!(false_: false => "false");
            value!(true_:  true  => "true");

            value!(i8_:    -42i8                => "-42");
            value!(i16_:   -4200i16             => "-4200");
            value!(i32_:   -42000000i32         => "-42000000");
            value!(i64_:   -42000000000000i64   => "-42000000000000");
            value!(isize_: -42000000000000isize => "-42000000000000");

            value!(u8_:    42u8                => "42");
            value!(u16_:   4200u16             => "4200");
            value!(u32_:   42000000u32         => "42000000");
            value!(u64_:   42000000000000u64   => "42000000000000");
            value!(usize_: 42000000000000usize => "42000000000000");

            serde_if_integer128! {
                value!(i128_: -420000000000000000000000000000i128 => "-420000000000000000000000000000");
                value!(u128_:  420000000000000000000000000000u128 => "420000000000000000000000000000");
            }

            value!(f32_: 4.2f32 => "4.2");
            value!(f64_: 4.2f64 => "4.2");

            value!(char_non_escaped: 'h' => "h");
            value!(char_lt:   '<' => "&lt;");
            value!(char_gt:   '>' => "&gt;");
            value!(char_amp:  '&' => "&amp;");
            value!(char_apos: '\'' => "&apos;");
            value!(char_quot: '"' => "&quot;");
            //TODO: add a setting to escape leading/trailing spaces, in order to
            // pretty-print does not change the content
            value!(char_space: ' ' => " ");

            value!(str_non_escaped: "non-escaped string" => "non-escaped string");
            value!(str_escaped: "<\"escaped & string'>" => "&lt;&quot;escaped &amp; string&apos;&gt;");

            err!(bytes:
                SpecialEnum::Value {
                    before: "answer",
                    content: Bytes(b"<\"escaped & bytes'>"),
                    after: "answer",
                }
                => Unsupported("`serialize_bytes` not supported yet"));

            value!(option_none: Option::<&str>::None);
            value!(option_some: Some("non-escaped string") => "non-escaped string");
            value!(option_some_empty_str: Some(""));

            value!(unit: ());
            value!(unit_struct: Unit);
            value!(unit_struct_escaped: UnitEscaped);

            value!(enum_unit: Enum::Unit => "<Unit/>");
            err!(enum_unit_escaped:
                SpecialEnum::Value {
                    before: "answer",
                    content: Enum::UnitEscaped,
                    after: "answer",
                }
                => Unsupported("character `<` is not allowed at the start of an XML name `<\"&'>`"));

            value!(newtype: Newtype(42) => "42");
            value!(enum_newtype: Enum::Newtype(42) => "<Newtype>42</Newtype>");

            // Note that sequences of primitives serialized without delimiters!
            value!(seq: vec![1, 2, 3] => "1\n  2\n  3");
            value!(seq_empty: Vec::<usize>::new());
            value!(tuple: ("<\"&'>", "with\t\n\r spaces", 3usize)
                => "&lt;&quot;&amp;&apos;&gt;\n  \
                    with\t\n\r spaces\n  \
                    3");
            value!(tuple_struct: Tuple("first", 42) => "first\n  42");
            value!(enum_tuple: Enum::Tuple("first", 42)
                => "<Tuple>first</Tuple>\n  \
                    <Tuple>42</Tuple>");

            // We cannot wrap map or struct in any container and should not
            // flatten it, so it is impossible to serialize maps and structs
            err!(map:
                SpecialEnum::Value {
                    before: "answer",
                    content: BTreeMap::from([("_1", 2), ("_3", 4)]),
                    after: "answer",
                }
                => Unsupported("serialization of map types is not supported in `$value` field"));
            err!(struct_:
                SpecialEnum::Value {
                    before: "answer",
                    content: Struct { key: "answer", val: (42, 42) },
                    after: "answer",
                }
                => Unsupported("serialization of struct `Struct` is not supported in `$value` field"));
            value!(enum_struct:
                Enum::Struct { key: "answer", val: (42, 42) }
                => "<Struct>\n    \
                        <key>answer</key>\n    \
                        <val>42</val>\n    \
                        <val>42</val>\n  \
                    </Struct>");
        }

        mod attributes {
            use super::*;
            use pretty_assertions::assert_eq;

            err!(map_attr: BTreeMap::from([("@key1", 1), ("@key2", 2)])
                => Unsupported("serialization of map types is not supported in `$value` field"));
            err!(map_mixed: BTreeMap::from([("@key1", 1), ("key2", 2)])
                => Unsupported("serialization of map types is not supported in `$value` field"));

            err!(struct_: Attributes { key: "answer", val: (42, 42) }
                => Unsupported("serialization of struct `Attributes` is not supported in `$value` field"));
            err!(struct_before: AttributesBefore { key: "answer", val: 42 }
                => Unsupported("serialization of struct `AttributesBefore` is not supported in `$value` field"));
            err!(struct_after: AttributesAfter { key: "answer", val: 42 }
                => Unsupported("serialization of struct `AttributesAfter` is not supported in `$value` field"));

            serialize_as!(enum_: Enum::Attributes { key: "answer", val: (42, 42) }
                => r#"<Attributes key="answer" val="42 42"/>"#);
            serialize_as!(enum_before: Enum::AttributesBefore { key: "answer", val: 42 }
                => "<AttributesBefore key=\"answer\">\n  \
                        <val>42</val>\n\
                    </AttributesBefore>");
            serialize_as!(enum_after: Enum::AttributesAfter { key: "answer", val: 42 }
                => "<AttributesAfter val=\"42\">\n  \
                        <key>answer</key>\n\
                    </AttributesAfter>");
        }
    }
}
