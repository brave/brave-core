// Prohibit dangerous things we definitely don't want
#![deny(clippy::integer_arithmetic)]
#![deny(clippy::cast_possible_truncation)]
#![deny(clippy::indexing_slicing)]
// Style lints
#![warn(clippy::cast_lossless)]

use std::{str, mem, convert::TryInto};
use crate::ByteOrder;

#[derive(Debug, Copy, Clone)]
pub struct UnexpectedEof {}

impl std::fmt::Display for UnexpectedEof {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Unexpected end of file")
    }
}

impl std::error::Error for UnexpectedEof {}

pub trait RawNumber: Sized {
    fn parse(s: &mut Stream) -> Option<Self>;
}

impl RawNumber for u8 {
    #[inline]
    fn parse(s: &mut Stream) -> Option<Self> {
        s.data.get(s.offset).copied()
    }
}

impl RawNumber for i8 {
    #[inline]
    fn parse(s: &mut Stream) -> Option<Self> {
        s.data.get(s.offset).map(|x| *x as i8)
    }
}

impl RawNumber for u16 {
    #[inline]
    fn parse(s: &mut Stream) -> Option<Self> {
        let start = s.offset;
        let end = s.offset.checked_add(mem::size_of::<Self>())?;
        let num = u16::from_ne_bytes(s.data.get(start..end)?.try_into().unwrap());
        match s.byte_order {
            ByteOrder::LittleEndian => Some(num),
            ByteOrder::BigEndian => Some(num.to_be()),
        }
    }
}

impl RawNumber for i16 {
    #[inline]
    fn parse(s: &mut Stream) -> Option<Self> {
        u16::parse(s).map(|x| x as i16)
    }
}

impl RawNumber for u32 {
    #[inline]
    fn parse(s: &mut Stream) -> Option<Self> {
        let start = s.offset;
        let end = s.offset.checked_add(mem::size_of::<Self>())?;
        let num = u32::from_ne_bytes(s.data.get(start..end)?.try_into().unwrap());
        match s.byte_order {
            ByteOrder::LittleEndian => Some(num),
            ByteOrder::BigEndian => Some(num.to_be()),
        }
    }
}

impl RawNumber for u64 {
    #[inline]
    fn parse(s: &mut Stream) -> Option<Self> {
        let start = s.offset;
        let end = s.offset.checked_add(mem::size_of::<Self>())?;
        let num = u64::from_ne_bytes(s.data.get(start..end)?.try_into().unwrap());
        match s.byte_order {
            ByteOrder::LittleEndian => Some(num),
            ByteOrder::BigEndian => Some(num.to_be()),
        }
    }
}

#[derive(Clone, Copy)]
pub struct Stream<'a> {
    data: &'a [u8],
    offset: usize,
    byte_order: ByteOrder,
}

impl<'a> Stream<'a> {
    #[inline]
    pub fn new(data: &'a [u8], byte_order: ByteOrder) -> Self {
        Stream {
            data,
            offset: 0,
            byte_order,
        }
    }

    #[inline]
    pub fn new_at(data: &'a [u8], offset: usize, byte_order: ByteOrder) -> Result<Self, UnexpectedEof> {
        if offset < data.len() {
            Ok(Stream {
                data,
                offset,
                byte_order,
            })
        } else {
            Err(UnexpectedEof{})
        }
    }

    #[inline]
    pub fn at_end(&self) -> bool {
        self.offset >= self.data.len()
    }

    #[inline]
    pub fn offset(&self) -> usize {
        self.offset
    }

    #[inline]
    pub fn skip<T: RawNumber>(&mut self) -> Result<(), UnexpectedEof> {
        self.skip_len(mem::size_of::<T>())
    }

    #[inline]
    pub fn skip_len(&mut self, len: usize) -> Result<(), UnexpectedEof> {
        let new_offset = self.offset.checked_add(len);
        match new_offset {
            Some(valid_offset) => {self.offset = valid_offset; Ok(())}
            None => {Err(UnexpectedEof{})}
        }
    }

    #[inline]
    pub fn read<T: RawNumber>(&mut self) -> Result<T, UnexpectedEof> {
        let new_offset = self.offset.checked_add(mem::size_of::<T>()).ok_or(UnexpectedEof{})?;
        let v = T::parse(self).ok_or(UnexpectedEof{})?;
        self.offset = new_offset;
        Ok(v)
    }

    #[inline]
    pub fn read_bytes(&mut self, len: usize) -> Result<&'a [u8], UnexpectedEof> {
        let new_offset = self.offset.checked_add(len)
            .ok_or(UnexpectedEof{})?;
        let bytes = &self.data.get(self.offset..new_offset)
            .ok_or(UnexpectedEof{})?;
        self.offset = new_offset;
        Ok(bytes)
    }

    #[inline]
    pub fn remaining(&self) -> usize {
        self.data.len().saturating_sub(self.offset)
    }
}

pub fn parse_null_string(data: &[u8], start: usize) -> Option<&str> {
    match data.get(start..)?.iter().position(|c| *c == b'\0') {
        Some(i) if i != 0 => str::from_utf8(data.get(start..start.checked_add(i)?)?).ok(),
        _ => None,
    }
}
