// Copyright (c) 2020 Aleksa Sarai <cyphar@cyphar.com>
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

//! `nom` combinators to decode unsigned varints.

use crate::decode::{self, Error};
use nom::{error::ErrorKind, Err as NomErr, IResult, Needed};

macro_rules! gen {
    ($($type:ident, $d:expr);*) => {
        $(
            #[doc = " `nom` combinator to decode a variable-length encoded "]
            #[doc = $d]
            #[doc = "."]
            pub fn $type(input: &[u8]) -> IResult<&[u8], $type, (&[u8], ErrorKind)> {
                let (n, remain) = decode::$type(input).map_err(|err| match err {
                    Error::Insufficient => NomErr::Incomplete(Needed::Unknown),
                    Error::Overflow => NomErr::Error((input, ErrorKind::TooLarge)),
                    Error::NotMinimal => NomErr::Error((input, ErrorKind::Verify)),
                })?;
                Ok((remain, n))
            }
        )*
    }
}

gen! {
    u8,    "`u8`";
    u16,   "`u16`";
    u32,   "`u32`";
    u64,   "`u64`";
    u128,  "`u128`";
    usize, "`usize`"
}
