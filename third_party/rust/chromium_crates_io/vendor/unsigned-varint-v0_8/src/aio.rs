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

//! Decode using [`futures_io::AsyncRead`] types.

use crate::{decode, encode, io::ReadError};
use futures_io::AsyncRead;
use futures_util::io::AsyncReadExt;
use std::io;

macro_rules! gen {
    ($($name:ident, $d:expr, $t:ident, $b:ident);*) => {
        $(
            #[doc = " Try to read and decode a "]
            #[doc = $d]
            #[doc = " from the given `AsyncRead` type."]
            pub async fn $name<R: AsyncRead + Unpin>(mut reader: R) -> Result<$t, ReadError> {
                let mut b = encode::$b();
                for i in 0 .. b.len() {
                    let n = reader.read(&mut b[i .. i + 1]).await?;
                    if n == 0 {
                        return Err(ReadError::Io(io::ErrorKind::UnexpectedEof.into()))
                    }
                    if decode::is_last(b[i]) {
                        return Ok(decode::$t(&b[..= i])?.0)
                    }
                }
                Err(decode::Error::Overflow)?
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

