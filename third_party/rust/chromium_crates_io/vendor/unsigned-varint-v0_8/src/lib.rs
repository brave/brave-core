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

//! Unsigned varint encodes unsigned integers in 7-bit groups. The most
//! significant bit (MSB) in each byte indicates if another byte follows
//! (MSB = 1), or not (MSB = 0).

#![forbid(unsafe_code, unused_imports, unused_variables)]
#![cfg_attr(not(feature = "std"), no_std)]

pub mod decode;
pub mod encode;

#[cfg(feature = "std")]
pub mod io;

#[cfg(feature = "futures")]
pub mod aio;

#[cfg(any(feature = "codec", feature = "asynchronous_codec"))]
pub mod codec;

#[cfg(feature = "nom")]
pub mod nom;
