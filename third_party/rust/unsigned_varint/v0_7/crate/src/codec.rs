// Copyright 2018-2019 Parity Technologies (UK) Ltd.
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

//! `Encoder`/`Decoder` implementations for tokio or asynchronous_codec.

use bytes::{Buf, BufMut, Bytes, BytesMut};
use crate::{encode, decode::{self, Error}};
use std::{io, marker::PhantomData, usize};

/// Encoder/Decoder of unsigned-varint values
#[derive(Default)]
pub struct Uvi<T>(PhantomData<T>);

macro_rules! encoder_decoder_impls {
    ($typ:ident, $arr:ident) => {
        impl Uvi<$typ> {
            fn serialise(&mut self, item: $typ, dst: &mut BytesMut) {
                let mut buf = encode::$arr();
                dst.extend_from_slice(encode::$typ(item, &mut buf))
            }

            fn deserialise(&mut self, src: &mut BytesMut) -> Result<Option<$typ>, io::Error> {
                let (number, consumed) =
                    match decode::$typ(src.as_ref()) {
                        Ok((n, rem)) => (n, src.len() - rem.len()),
                        Err(Error::Insufficient) => return Ok(None),
                        Err(e) => return Err(io::Error::new(io::ErrorKind::Other, e))
                    };
                src.advance(consumed);
                Ok(Some(number))
            }
        }

        #[cfg(feature = "codec")]
        impl tokio_util::codec::Encoder<$typ> for Uvi<$typ> {
            type Error = io::Error;

            fn encode(&mut self, item: $typ, dst: &mut BytesMut) -> Result<(), Self::Error> {
                self.serialise(item, dst);
                Ok(())
            }
        }

        #[cfg(feature = "codec")]
        impl tokio_util::codec::Decoder for Uvi<$typ> {
            type Item = $typ;
            type Error = io::Error;

            fn decode(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
                self.deserialise(src)
            }
        }

        #[cfg(feature = "asynchronous_codec")]
        impl asynchronous_codec::Encoder for Uvi<$typ> {
            type Item = $typ;
            type Error = io::Error;

            fn encode(&mut self, item: Self::Item, dst: &mut BytesMut) -> Result<(), Self::Error> {
                self.serialise(item, dst);
                Ok(())
            }
        }

        #[cfg(feature = "asynchronous_codec")]
        impl asynchronous_codec::Decoder for Uvi<$typ> {
            type Item = $typ;
            type Error = io::Error;

            fn decode(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
                self.deserialise(src)
            }
        }
    }
}

encoder_decoder_impls!(u8, u8_buffer);
encoder_decoder_impls!(u16, u16_buffer);
encoder_decoder_impls!(u32, u32_buffer);
encoder_decoder_impls!(u64, u64_buffer);
encoder_decoder_impls!(u128, u128_buffer);
encoder_decoder_impls!(usize, usize_buffer);

/// Encoder/Decoder of unsigned-varint, length-prefixed bytes
pub struct UviBytes<T = Bytes> {
    /// the variable-length integer encoder/decoder
    varint_codec: Uvi<usize>,
    /// number of bytes expected in the current frame (for decoding only)
    len: Option<usize>,
    /// maximum permitted number of bytes per frame
    max: usize,
    _ty: PhantomData<T>
}

impl<T> Default for UviBytes<T> {
    fn default() -> Self {
        Self {
            varint_codec: Default::default(),
            len: None,
            max: 128 * 1024 * 1024,
            _ty: PhantomData
        }
    }
}

impl<T> UviBytes<T> {
    /// Limit the maximum allowed length of bytes.
    pub fn set_max_len(&mut self, val: usize) {
        self.max = val
    }

    /// Return the maximum allowed number of bytes to encode/decode.
    pub fn max_len(&self) -> usize {
        self.max
    }

    fn deserialise(&mut self, src: &mut BytesMut) -> Result<Option<BytesMut>, io::Error> {
        if self.len.is_none() {
            self.len = self.varint_codec.deserialise(src)?
        }
        if let Some(n) = self.len.take() {
            if n > self.max {
                return Err(io::Error::new(io::ErrorKind::PermissionDenied, "len > max"))
            }
            if n <= src.len() {
                return Ok(Some(src.split_to(n)))
            }
            let add = n - src.len();
            src.reserve(add);
            self.len = Some(n)
        }
        Ok(None)
    }
}

impl<T: Buf> UviBytes<T> {
    fn serialise(&mut self, item: T, dst: &mut BytesMut) -> Result<(), io::Error> {
        if item.remaining() > self.max {
            return Err(io::Error::new(io::ErrorKind::PermissionDenied, "len > max when encoding"));
        }
        self.varint_codec.serialise(item.remaining(), dst);
        dst.reserve(item.remaining());
        dst.put(item);
        Ok(())
    }
}


#[cfg(feature = "codec")]
impl<T: Buf> tokio_util::codec::Encoder<T> for UviBytes<T> {
    type Error = io::Error;

    fn encode(&mut self, item: T, dst: &mut BytesMut) -> Result<(), Self::Error> {
        self.serialise(item, dst)
    }
}

#[cfg(feature = "codec")]
impl<T> tokio_util::codec::Decoder for UviBytes<T> {
    type Item = BytesMut;
    type Error = io::Error;

    fn decode(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        self.deserialise(src)
    }
}

#[cfg(feature = "asynchronous_codec")]
impl<T: Buf> asynchronous_codec::Encoder for UviBytes<T> {
    type Item = T;
    type Error = io::Error;

    fn encode(&mut self, item: Self::Item, dst: &mut BytesMut) -> Result<(), Self::Error> {
        self.serialise(item, dst)
    }
}

#[cfg(feature = "asynchronous_codec")]
impl<T> asynchronous_codec::Decoder for UviBytes<T> {
    type Item = BytesMut;
    type Error = io::Error;

    fn decode(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        self.deserialise(src)
    }
}

