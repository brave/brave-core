// Automatically generated rust module for 'no_std.proto' file

#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(unused_imports)]
#![allow(unknown_lints)]
#![allow(clippy::all)]
#![cfg_attr(rustfmt, rustfmt_skip)]


use alloc::vec::Vec;
use alloc::borrow::Cow;
use quick_protobuf::{MessageInfo, MessageRead, MessageWrite, BytesReader, Writer, WriterBackend, Result};
use quick_protobuf::sizeofs::*;
use super::super::*;

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum MyEnum {
    Val0 = 0,
    Val1 = 1,
}

impl Default for MyEnum {
    fn default() -> Self {
        MyEnum::Val0
    }
}

impl From<i32> for MyEnum {
    fn from(i: i32) -> Self {
        match i {
            0 => MyEnum::Val0,
            1 => MyEnum::Val1,
            _ => Self::default(),
        }
    }
}

impl<'a> From<&'a str> for MyEnum {
    fn from(s: &'a str) -> Self {
        match s {
            "Val0" => MyEnum::Val0,
            "Val1" => MyEnum::Val1,
            _ => Self::default(),
        }
    }
}

#[allow(clippy::derive_partial_eq_without_eq)]
#[derive(Debug, Default, PartialEq, Clone)]
pub struct EmbeddedMessage {
    pub val: i32,
    pub e: protos::no_std::MyEnum,
}

impl<'a> MessageRead<'a> for EmbeddedMessage {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = Self::default();
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(8) => msg.val = r.read_int32(bytes)?,
                Ok(16) => msg.e = r.read_enum(bytes)?,
                Ok(t) => { r.read_unknown(bytes, t)?; }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}

impl MessageWrite for EmbeddedMessage {
    fn get_size(&self) -> usize {
        0
        + if self.val == 0i32 { 0 } else { 1 + sizeof_varint(*(&self.val) as u64) }
        + if self.e == protos::no_std::MyEnum::Val0 { 0 } else { 1 + sizeof_varint(*(&self.e) as u64) }
    }

    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        if self.val != 0i32 { w.write_with_tag(8, |w| w.write_int32(*&self.val))?; }
        if self.e != protos::no_std::MyEnum::Val0 { w.write_with_tag(16, |w| w.write_enum(*&self.e as i32))?; }
        Ok(())
    }
}

#[allow(clippy::derive_partial_eq_without_eq)]
#[derive(Debug, Default, PartialEq, Clone)]
pub struct NoStdMessage<'a> {
    pub num: u32,
    pub nums: Cow<'a, [u32]>,
    pub message: Option<protos::no_std::EmbeddedMessage>,
    pub messages: Vec<protos::no_std::EmbeddedMessage>,
}

impl<'a> MessageRead<'a> for NoStdMessage<'a> {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = Self::default();
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(13) => msg.num = r.read_fixed32(bytes)?,
                Ok(18) => msg.nums = r.read_packed_fixed(bytes)?.into(),
                Ok(26) => msg.message = Some(r.read_message::<protos::no_std::EmbeddedMessage>(bytes)?),
                Ok(34) => msg.messages.push(r.read_message::<protos::no_std::EmbeddedMessage>(bytes)?),
                Ok(t) => { r.read_unknown(bytes, t)?; }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}

impl<'a> MessageWrite for NoStdMessage<'a> {
    fn get_size(&self) -> usize {
        0
        + if self.num == 0u32 { 0 } else { 1 + 4 }
        + if self.nums.is_empty() { 0 } else { 1 + sizeof_len(self.nums.len() * 4) }
        + self.message.as_ref().map_or(0, |m| 1 + sizeof_len((m).get_size()))
        + self.messages.iter().map(|s| 1 + sizeof_len((s).get_size())).sum::<usize>()
    }

    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        if self.num != 0u32 { w.write_with_tag(13, |w| w.write_fixed32(*&self.num))?; }
        w.write_packed_fixed_with_tag(18, &self.nums)?;
        if let Some(ref s) = self.message { w.write_with_tag(26, |w| w.write_message(s))?; }
        for s in &self.messages { w.write_with_tag(34, |w| w.write_message(s))?; }
        Ok(())
    }
}

