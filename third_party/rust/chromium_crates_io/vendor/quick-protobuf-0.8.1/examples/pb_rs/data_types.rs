// Automatically generated rust module for 'data_types.proto' file

#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(unused_imports)]
#![allow(unknown_lints)]
#![allow(clippy::all)]
#![cfg_attr(rustfmt, rustfmt_skip)]


use std::borrow::Cow;
use std::collections::HashMap;
type KVMap<K, V> = HashMap<K, V>;
use quick_protobuf::{MessageInfo, MessageRead, MessageWrite, BytesReader, Writer, WriterBackend, Result};
use quick_protobuf::sizeofs::*;
use super::*;

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum FooEnum {
    FIRST_VALUE = 1,
    SECOND_VALUE = 2,
}

impl Default for FooEnum {
    fn default() -> Self {
        FooEnum::FIRST_VALUE
    }
}

impl From<i32> for FooEnum {
    fn from(i: i32) -> Self {
        match i {
            1 => FooEnum::FIRST_VALUE,
            2 => FooEnum::SECOND_VALUE,
            _ => Self::default(),
        }
    }
}

impl<'a> From<&'a str> for FooEnum {
    fn from(s: &'a str) -> Self {
        match s {
            "FIRST_VALUE" => FooEnum::FIRST_VALUE,
            "SECOND_VALUE" => FooEnum::SECOND_VALUE,
            _ => Self::default(),
        }
    }
}

#[allow(clippy::derive_partial_eq_without_eq)]
#[derive(Debug, Default, PartialEq, Clone)]
pub struct BarMessage {
    pub b_required_int32: i32,
}

impl<'a> MessageRead<'a> for BarMessage {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = Self::default();
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(8) => msg.b_required_int32 = r.read_int32(bytes)?,
                Ok(t) => { r.read_unknown(bytes, t)?; }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}

impl MessageWrite for BarMessage {
    fn get_size(&self) -> usize {
        0
        + 1 + sizeof_varint(*(&self.b_required_int32) as u64)
    }

    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        w.write_with_tag(8, |w| w.write_int32(*&self.b_required_int32))?;
        Ok(())
    }
}

#[allow(clippy::derive_partial_eq_without_eq)]
#[derive(Debug, Default, PartialEq, Clone)]
pub struct FooMessage<'a> {
    pub f_int32: Option<i32>,
    pub f_int64: Option<i64>,
    pub f_uint32: Option<u32>,
    pub f_uint64: Option<u64>,
    pub f_sint32: Option<i32>,
    pub f_sint64: i64,
    pub f_bool: bool,
    pub f_FooEnum: Option<FooEnum>,
    pub f_fixed64: Option<u64>,
    pub f_sfixed64: Option<i64>,
    pub f_fixed32: u32,
    pub f_sfixed32: Option<i32>,
    pub f_double: Option<f64>,
    pub f_float: Option<f32>,
    pub f_bytes: Option<Cow<'a, [u8]>>,
    pub f_string: Option<Cow<'a, str>>,
    pub f_self_message: Option<Box<FooMessage<'a>>>,
    pub f_bar_message: Option<BarMessage>,
    pub f_repeated_int32: Vec<i32>,
    pub f_repeated_packed_int32: Vec<i32>,
    pub f_repeated_packed_float: Cow<'a, [f32]>,
    pub f_imported: Option<a::b::ImportedMessage>,
    pub f_baz: Option<BazMessage>,
    pub f_nested: Option<mod_BazMessage::Nested>,
    pub f_nested_enum: Option<mod_BazMessage::mod_Nested::NestedEnum>,
    pub f_map: KVMap<Cow<'a, str>, i32>,
    pub test_oneof: mod_FooMessage::OneOftest_oneof<'a>,
}

impl<'a> MessageRead<'a> for FooMessage<'a> {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = FooMessage {
            f_sint64: 4i64,
            f_bool: true,
            ..Self::default()
        };
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(8) => msg.f_int32 = Some(r.read_int32(bytes)?),
                Ok(16) => msg.f_int64 = Some(r.read_int64(bytes)?),
                Ok(24) => msg.f_uint32 = Some(r.read_uint32(bytes)?),
                Ok(32) => msg.f_uint64 = Some(r.read_uint64(bytes)?),
                Ok(40) => msg.f_sint32 = Some(r.read_sint32(bytes)?),
                Ok(48) => msg.f_sint64 = r.read_sint64(bytes)?,
                Ok(56) => msg.f_bool = r.read_bool(bytes)?,
                Ok(64) => msg.f_FooEnum = Some(r.read_enum(bytes)?),
                Ok(73) => msg.f_fixed64 = Some(r.read_fixed64(bytes)?),
                Ok(81) => msg.f_sfixed64 = Some(r.read_sfixed64(bytes)?),
                Ok(93) => msg.f_fixed32 = r.read_fixed32(bytes)?,
                Ok(101) => msg.f_sfixed32 = Some(r.read_sfixed32(bytes)?),
                Ok(105) => msg.f_double = Some(r.read_double(bytes)?),
                Ok(117) => msg.f_float = Some(r.read_float(bytes)?),
                Ok(122) => msg.f_bytes = Some(r.read_bytes(bytes).map(Cow::Borrowed)?),
                Ok(130) => msg.f_string = Some(r.read_string(bytes).map(Cow::Borrowed)?),
                Ok(138) => msg.f_self_message = Some(Box::new(r.read_message::<FooMessage>(bytes)?)),
                Ok(146) => msg.f_bar_message = Some(r.read_message::<BarMessage>(bytes)?),
                Ok(152) => msg.f_repeated_int32.push(r.read_int32(bytes)?),
                Ok(162) => msg.f_repeated_packed_int32 = r.read_packed(bytes, |r, bytes| Ok(r.read_int32(bytes)?))?,
                Ok(170) => msg.f_repeated_packed_float = r.read_packed_fixed(bytes)?.into(),
                Ok(178) => msg.f_imported = Some(r.read_message::<a::b::ImportedMessage>(bytes)?),
                Ok(186) => msg.f_baz = Some(r.read_message::<BazMessage>(bytes)?),
                Ok(194) => msg.f_nested = Some(r.read_message::<mod_BazMessage::Nested>(bytes)?),
                Ok(200) => msg.f_nested_enum = Some(r.read_enum(bytes)?),
                Ok(210) => {
                    let (key, value) = r.read_map(bytes, |r, bytes| Ok(r.read_string(bytes).map(Cow::Borrowed)?), |r, bytes| Ok(r.read_int32(bytes)?))?;
                    msg.f_map.insert(key, value);
                }
                Ok(216) => msg.test_oneof = mod_FooMessage::OneOftest_oneof::f1(r.read_int32(bytes)?),
                Ok(224) => msg.test_oneof = mod_FooMessage::OneOftest_oneof::f2(r.read_bool(bytes)?),
                Ok(234) => msg.test_oneof = mod_FooMessage::OneOftest_oneof::f3(r.read_string(bytes).map(Cow::Borrowed)?),
                Ok(t) => { r.read_unknown(bytes, t)?; }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}

impl<'a> MessageWrite for FooMessage<'a> {
    fn get_size(&self) -> usize {
        0
        + self.f_int32.as_ref().map_or(0, |m| 1 + sizeof_varint(*(m) as u64))
        + self.f_int64.as_ref().map_or(0, |m| 1 + sizeof_varint(*(m) as u64))
        + self.f_uint32.as_ref().map_or(0, |m| 1 + sizeof_varint(*(m) as u64))
        + self.f_uint64.as_ref().map_or(0, |m| 1 + sizeof_varint(*(m) as u64))
        + self.f_sint32.as_ref().map_or(0, |m| 1 + sizeof_sint32(*(m)))
        + if self.f_sint64 == 4i64 { 0 } else { 1 + sizeof_sint64(*(&self.f_sint64)) }
        + if self.f_bool == true { 0 } else { 1 + sizeof_varint(*(&self.f_bool) as u64) }
        + self.f_FooEnum.as_ref().map_or(0, |m| 1 + sizeof_varint(*(m) as u64))
        + self.f_fixed64.as_ref().map_or(0, |_| 1 + 8)
        + self.f_sfixed64.as_ref().map_or(0, |_| 1 + 8)
        + if self.f_fixed32 == 0u32 { 0 } else { 1 + 4 }
        + self.f_sfixed32.as_ref().map_or(0, |_| 1 + 4)
        + self.f_double.as_ref().map_or(0, |_| 1 + 8)
        + self.f_float.as_ref().map_or(0, |_| 1 + 4)
        + self.f_bytes.as_ref().map_or(0, |m| 1 + sizeof_len((m).len()))
        + self.f_string.as_ref().map_or(0, |m| 2 + sizeof_len((m).len()))
        + self.f_self_message.as_ref().map_or(0, |m| 2 + sizeof_len((m).get_size()))
        + self.f_bar_message.as_ref().map_or(0, |m| 2 + sizeof_len((m).get_size()))
        + self.f_repeated_int32.iter().map(|s| 2 + sizeof_varint(*(s) as u64)).sum::<usize>()
        + if self.f_repeated_packed_int32.is_empty() { 0 } else { 2 + sizeof_len(self.f_repeated_packed_int32.iter().map(|s| sizeof_varint(*(s) as u64)).sum::<usize>()) }
        + if self.f_repeated_packed_float.is_empty() { 0 } else { 2 + sizeof_len(self.f_repeated_packed_float.len() * 4) }
        + self.f_imported.as_ref().map_or(0, |m| 2 + sizeof_len((m).get_size()))
        + self.f_baz.as_ref().map_or(0, |m| 2 + sizeof_len((m).get_size()))
        + self.f_nested.as_ref().map_or(0, |m| 2 + sizeof_len((m).get_size()))
        + self.f_nested_enum.as_ref().map_or(0, |m| 2 + sizeof_varint(*(m) as u64))
        + self.f_map.iter().map(|(k, v)| 2 + sizeof_len(2 + sizeof_len((k).len()) + sizeof_varint(*(v) as u64))).sum::<usize>()
        + match self.test_oneof {
            mod_FooMessage::OneOftest_oneof::f1(ref m) => 2 + sizeof_varint(*(m) as u64),
            mod_FooMessage::OneOftest_oneof::f2(ref m) => 2 + sizeof_varint(*(m) as u64),
            mod_FooMessage::OneOftest_oneof::f3(ref m) => 2 + sizeof_len((m).len()),
            mod_FooMessage::OneOftest_oneof::None => 0,
    }    }

    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        if let Some(ref s) = self.f_int32 { w.write_with_tag(8, |w| w.write_int32(*s))?; }
        if let Some(ref s) = self.f_int64 { w.write_with_tag(16, |w| w.write_int64(*s))?; }
        if let Some(ref s) = self.f_uint32 { w.write_with_tag(24, |w| w.write_uint32(*s))?; }
        if let Some(ref s) = self.f_uint64 { w.write_with_tag(32, |w| w.write_uint64(*s))?; }
        if let Some(ref s) = self.f_sint32 { w.write_with_tag(40, |w| w.write_sint32(*s))?; }
        if self.f_sint64 != 4i64 { w.write_with_tag(48, |w| w.write_sint64(*&self.f_sint64))?; }
        if self.f_bool != true { w.write_with_tag(56, |w| w.write_bool(*&self.f_bool))?; }
        if let Some(ref s) = self.f_FooEnum { w.write_with_tag(64, |w| w.write_enum(*s as i32))?; }
        if let Some(ref s) = self.f_fixed64 { w.write_with_tag(73, |w| w.write_fixed64(*s))?; }
        if let Some(ref s) = self.f_sfixed64 { w.write_with_tag(81, |w| w.write_sfixed64(*s))?; }
        if self.f_fixed32 != 0u32 { w.write_with_tag(93, |w| w.write_fixed32(*&self.f_fixed32))?; }
        if let Some(ref s) = self.f_sfixed32 { w.write_with_tag(101, |w| w.write_sfixed32(*s))?; }
        if let Some(ref s) = self.f_double { w.write_with_tag(105, |w| w.write_double(*s))?; }
        if let Some(ref s) = self.f_float { w.write_with_tag(117, |w| w.write_float(*s))?; }
        if let Some(ref s) = self.f_bytes { w.write_with_tag(122, |w| w.write_bytes(&**s))?; }
        if let Some(ref s) = self.f_string { w.write_with_tag(130, |w| w.write_string(&**s))?; }
        if let Some(ref s) = self.f_self_message { w.write_with_tag(138, |w| w.write_message(&**s))?; }
        if let Some(ref s) = self.f_bar_message { w.write_with_tag(146, |w| w.write_message(s))?; }
        for s in &self.f_repeated_int32 { w.write_with_tag(152, |w| w.write_int32(*s))?; }
        w.write_packed_with_tag(162, &self.f_repeated_packed_int32, |w, m| w.write_int32(*m), &|m| sizeof_varint(*(m) as u64))?;
        w.write_packed_fixed_with_tag(170, &self.f_repeated_packed_float)?;
        if let Some(ref s) = self.f_imported { w.write_with_tag(178, |w| w.write_message(s))?; }
        if let Some(ref s) = self.f_baz { w.write_with_tag(186, |w| w.write_message(s))?; }
        if let Some(ref s) = self.f_nested { w.write_with_tag(194, |w| w.write_message(s))?; }
        if let Some(ref s) = self.f_nested_enum { w.write_with_tag(200, |w| w.write_enum(*s as i32))?; }
        for (k, v) in self.f_map.iter() { w.write_with_tag(210, |w| w.write_map(2 + sizeof_len((k).len()) + sizeof_varint(*(v) as u64), 10, |w| w.write_string(&**k), 16, |w| w.write_int32(*v)))?; }
        match self.test_oneof {            mod_FooMessage::OneOftest_oneof::f1(ref m) => { w.write_with_tag(216, |w| w.write_int32(*m))? },
            mod_FooMessage::OneOftest_oneof::f2(ref m) => { w.write_with_tag(224, |w| w.write_bool(*m))? },
            mod_FooMessage::OneOftest_oneof::f3(ref m) => { w.write_with_tag(234, |w| w.write_string(&**m))? },
            mod_FooMessage::OneOftest_oneof::None => {},
    }        Ok(())
    }
}

pub mod mod_FooMessage {

use super::*;

#[derive(Debug, PartialEq, Clone)]
pub enum OneOftest_oneof<'a> {
    f1(i32),
    f2(bool),
    f3(Cow<'a, str>),
    None,
}

impl<'a> Default for OneOftest_oneof<'a> {
    fn default() -> Self {
        OneOftest_oneof::None
    }
}

}

#[allow(clippy::derive_partial_eq_without_eq)]
#[derive(Debug, Default, PartialEq, Clone)]
pub struct BazMessage {
    pub nested: Option<mod_BazMessage::Nested>,
}

impl<'a> MessageRead<'a> for BazMessage {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = Self::default();
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(10) => msg.nested = Some(r.read_message::<mod_BazMessage::Nested>(bytes)?),
                Ok(t) => { r.read_unknown(bytes, t)?; }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}

impl MessageWrite for BazMessage {
    fn get_size(&self) -> usize {
        0
        + self.nested.as_ref().map_or(0, |m| 1 + sizeof_len((m).get_size()))
    }

    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        if let Some(ref s) = self.nested { w.write_with_tag(10, |w| w.write_message(s))?; }
        Ok(())
    }
}

pub mod mod_BazMessage {

use super::*;

#[allow(clippy::derive_partial_eq_without_eq)]
#[derive(Debug, Default, PartialEq, Clone)]
pub struct Nested {
    pub f_nested: mod_BazMessage::mod_Nested::NestedMessage,
}

impl<'a> MessageRead<'a> for Nested {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = Self::default();
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(10) => msg.f_nested = r.read_message::<mod_BazMessage::mod_Nested::NestedMessage>(bytes)?,
                Ok(t) => { r.read_unknown(bytes, t)?; }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}

impl MessageWrite for Nested {
    fn get_size(&self) -> usize {
        0
        + 1 + sizeof_len((&self.f_nested).get_size())
    }

    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        w.write_with_tag(10, |w| w.write_message(&self.f_nested))?;
        Ok(())
    }
}

pub mod mod_Nested {

use super::*;

#[allow(clippy::derive_partial_eq_without_eq)]
#[derive(Debug, Default, PartialEq, Clone)]
pub struct NestedMessage {
    pub f_nested: i32,
}

impl<'a> MessageRead<'a> for NestedMessage {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = Self::default();
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(8) => msg.f_nested = r.read_int32(bytes)?,
                Ok(t) => { r.read_unknown(bytes, t)?; }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}

impl MessageWrite for NestedMessage {
    fn get_size(&self) -> usize {
        0
        + 1 + sizeof_varint(*(&self.f_nested) as u64)
    }

    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        w.write_with_tag(8, |w| w.write_int32(*&self.f_nested))?;
        Ok(())
    }
}

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum NestedEnum {
    Foo = 0,
    Bar = 1,
    Baz = 2,
}

impl Default for NestedEnum {
    fn default() -> Self {
        NestedEnum::Foo
    }
}

impl From<i32> for NestedEnum {
    fn from(i: i32) -> Self {
        match i {
            0 => NestedEnum::Foo,
            1 => NestedEnum::Bar,
            2 => NestedEnum::Baz,
            _ => Self::default(),
        }
    }
}

impl<'a> From<&'a str> for NestedEnum {
    fn from(s: &'a str) -> Self {
        match s {
            "Foo" => NestedEnum::Foo,
            "Bar" => NestedEnum::Bar,
            "Baz" => NestedEnum::Baz,
            _ => Self::default(),
        }
    }
}

}

}

