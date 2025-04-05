// Copyright 2020 Parity Technologies (UK) Ltd.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

//! Decode using [`std::io::Read`] types.

use crate::{decode, encode};
use std::{fmt, io};

macro_rules! gen {
    ($($name:ident, $d:expr, $t:ident, $b:ident);*) => {
        $(
            #[doc = " Try to read and decode a "]
            #[doc = $d]
            #[doc = " from the given `Read` type."]
            pub fn $name<R: io::Read>(mut reader: R) -> Result<$t, ReadError> {
                let mut b = encode::$b();
                for i in 0 .. b.len() {
                    let n = reader.read(&mut b[i .. i + 1])?;
                    if n == 0 {
                        return Err(ReadError::Io(io::ErrorKind::UnexpectedEof.into()))
                    }
                    if decode::is_last(b[i]) {
                        return Ok(decode::$t(&b[..= i])?.0)
                    }
                }
                Err(decode::Error::Overflow.into())
            }
        )*
    }
}

gen! {
    read_u8,    "`u8`",    u8,    u8_buffer;
    read_u16,   "`u16`",   u16,   u16_buffer;
    read_u32,   "`u32`",   u32,   u32_buffer;
    read_u64,   "`u64`",   u64,   u64_buffer;
    read_u128,  "`u128`",  u128,  u128_buffer;
    read_usize, "`usize`", usize, usize_buffer
}

/// Possible read errors.
#[non_exhaustive]
#[derive(Debug)]
pub enum ReadError {
    Io(io::Error),
    Decode(decode::Error)
}

impl fmt::Display for ReadError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            ReadError::Io(e) => write!(f, "i/o error: {}", e),
            ReadError::Decode(e) => write!(f, "decode error: {}", e)
        }
    }
}

impl std::error::Error for ReadError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        if let ReadError::Io(e) = self {
            Some(e)
        } else {
            None
        }
    }
}

impl From<io::Error> for ReadError {
    fn from(e: io::Error) -> Self {
        ReadError::Io(e)
    }
}

impl From<decode::Error> for ReadError {
    fn from(e: decode::Error) -> Self {
        ReadError::Decode(e)
    }
}

impl Into<io::Error> for ReadError {
    fn into(self) -> io::Error {
        match self {
            ReadError::Io(e) => e,
            ReadError::Decode(e) => e.into(),
        }
    }
}
