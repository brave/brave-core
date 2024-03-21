// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(unused_imports)]
#![allow(unknown_lints)]
#![allow(clippy::all)]
#![cfg_attr(rustfmt, rustfmt_skip)]

use super::*;
use quick_protobuf::sizeofs::*;
use quick_protobuf::{BytesReader, MessageRead, MessageWrite, Result, Writer, WriterBackend};
use core::convert::TryFrom;
use std::borrow::Cow;
use std::io::Write;
use core::ops::Deref;
use core::ops::DerefMut;

#[derive(Debug, Default, PartialEq, Clone)]
pub struct Data<'a> {
    pub Type: mod_Data::DataType,
    pub Data: Option<Cow<'a, [u8]>>,
    pub filesize: Option<u64>,
    pub blocksizes: Vec<u64>,
    pub hashType: Option<u64>,
    pub fanout: Option<u64>,
    pub mode: Option<u32>,
    pub mtime: Option<UnixTime>,
}
impl<'a> MessageRead<'a> for Data<'a> {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = Self::default();
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(8) => msg.Type = r.read_enum(bytes)?,
                Ok(18) => msg.Data = Some(r.read_bytes(bytes).map(Cow::Borrowed)?),
                Ok(24) => msg.filesize = Some(r.read_uint64(bytes)?),
                Ok(32) => msg.blocksizes.push(r.read_uint64(bytes)?),
                Ok(40) => msg.hashType = Some(r.read_uint64(bytes)?),
                Ok(48) => msg.fanout = Some(r.read_uint64(bytes)?),
                Ok(56) => msg.mode = Some(r.read_uint32(bytes)?),
                Ok(66) => msg.mtime = Some(r.read_message::<UnixTime>(bytes)?),
                Ok(t) => {
                    r.read_unknown(bytes, t)?;
                }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}
impl<'a> MessageWrite for Data<'a> {
    fn get_size(&self) -> usize {
        0 + 1
            + sizeof_varint(*(&self.Type) as u64)
            + self.Data.as_ref().map_or(0, |m| 1 + sizeof_len((m).len()))
            + self
                .filesize
                .as_ref()
                .map_or(0, |m| 1 + sizeof_varint(*(m) as u64))
            + self
                .blocksizes
                .iter()
                .map(|s| 1 + sizeof_varint(*(s) as u64))
                .sum::<usize>()
            + self
                .hashType
                .as_ref()
                .map_or(0, |m| 1 + sizeof_varint(*(m) as u64))
            + self
                .fanout
                .as_ref()
                .map_or(0, |m| 1 + sizeof_varint(*(m) as u64))
            + self
                .mode
                .as_ref()
                .map_or(0, |m| 1 + sizeof_varint(*(m) as u64))
            + self
                .mtime
                .as_ref()
                .map_or(0, |m| 1 + sizeof_len((m).get_size()))
    }
    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        w.write_with_tag(8, |w| w.write_enum(*&self.Type as i32))?;
        if let Some(ref s) = self.Data {
            w.write_with_tag(18, |w| w.write_bytes(&**s))?;
        }
        if let Some(ref s) = self.filesize {
            w.write_with_tag(24, |w| w.write_uint64(*s))?;
        }
        for s in &self.blocksizes {
            w.write_with_tag(32, |w| w.write_uint64(*s))?;
        }
        if let Some(ref s) = self.hashType {
            w.write_with_tag(40, |w| w.write_uint64(*s))?;
        }
        if let Some(ref s) = self.fanout {
            w.write_with_tag(48, |w| w.write_uint64(*s))?;
        }
        if let Some(ref s) = self.mode {
            w.write_with_tag(56, |w| w.write_uint32(*s))?;
        }
        if let Some(ref s) = self.mtime {
            w.write_with_tag(66, |w| w.write_message(s))?;
        }
        Ok(())
    }
}
pub mod mod_Data {
    #[derive(Debug, PartialEq, Eq, Clone, Copy)]
    pub enum DataType {
        Raw = 0,
        Directory = 1,
        File = 2,
        Metadata = 3,
        Symlink = 4,
        HAMTShard = 5,
    }
    impl Default for DataType {
        fn default() -> Self {
            DataType::Raw
        }
    }
    impl From<i32> for DataType {
        fn from(i: i32) -> Self {
            match i {
                0 => DataType::Raw,
                1 => DataType::Directory,
                2 => DataType::File,
                3 => DataType::Metadata,
                4 => DataType::Symlink,
                5 => DataType::HAMTShard,
                _ => Self::default(),
            }
        }
    }
    impl<'a> From<&'a str> for DataType {
        fn from(s: &'a str) -> Self {
            match s {
                "Raw" => DataType::Raw,
                "Directory" => DataType::Directory,
                "File" => DataType::File,
                "Metadata" => DataType::Metadata,
                "Symlink" => DataType::Symlink,
                "HAMTShard" => DataType::HAMTShard,
                _ => Self::default(),
            }
        }
    }
    impl From<DataType> for i32 {
        fn from(dt: DataType) -> Self {
            match dt {
                DataType::Raw => 0,
                DataType::Directory => 1,
                DataType::File => 2,
                DataType::Metadata => 3,
                DataType::Symlink => 4,
                DataType::HAMTShard => 5,
            }
        }
    }
}
#[derive(Debug, Default, PartialEq, Clone)]
pub struct UnixTime {
    pub Seconds: i64,
    pub FractionalNanoseconds: Option<u32>,
}
impl<'a> MessageRead<'a> for UnixTime {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = Self::default();
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(8) => msg.Seconds = r.read_int64(bytes)?,
                Ok(21) => msg.FractionalNanoseconds = Some(r.read_fixed32(bytes)?),
                Ok(t) => {
                    r.read_unknown(bytes, t)?;
                }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}
impl MessageWrite for UnixTime {
    fn get_size(&self) -> usize {
        0 + 1
            + sizeof_varint(*(&self.Seconds) as u64)
            + self.FractionalNanoseconds.as_ref().map_or(0, |_| 1 + 4)
    }
    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        w.write_with_tag(8, |w| w.write_int64(*&self.Seconds))?;
        if let Some(ref s) = self.FractionalNanoseconds {
            w.write_with_tag(21, |w| w.write_fixed32(*s))?;
        }
        Ok(())
    }
}
#[derive(Debug, Default, PartialEq, Clone)]
pub struct Metadata<'a> {
    pub MimeType: Option<Cow<'a, str>>,
}
impl<'a> MessageRead<'a> for Metadata<'a> {
    fn from_reader(r: &mut BytesReader, bytes: &'a [u8]) -> Result<Self> {
        let mut msg = Self::default();
        while !r.is_eof() {
            match r.next_tag(bytes) {
                Ok(10) => msg.MimeType = Some(r.read_string(bytes).map(Cow::Borrowed)?),
                Ok(t) => {
                    r.read_unknown(bytes, t)?;
                }
                Err(e) => return Err(e),
            }
        }
        Ok(msg)
    }
}
impl<'a> MessageWrite for Metadata<'a> {
    fn get_size(&self) -> usize {
        0 + self
            .MimeType
            .as_ref()
            .map_or(0, |m| 1 + sizeof_len((m).len()))
    }
    fn write_message<W: WriterBackend>(&self, w: &mut Writer<W>) -> Result<()> {
        if let Some(ref s) = self.MimeType {
            w.write_with_tag(10, |w| w.write_string(&**s))?;
        }
        Ok(())
    }
}
