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

use quickcheck::{Arbitrary, Gen};
use unsigned_varint::encode;

#[cfg(feature = "std")]
#[test]
fn read_arbitrary() {
    use unsigned_varint::io;

    fn property(n: RandomUvi) {
        let mut r = std::io::Cursor::new(n.bytes());
        match n {
            RandomUvi::U8(n,  _)   => assert_eq!(n, io::read_u8(&mut r).unwrap()),
            RandomUvi::U16(n, _)   => assert_eq!(n, io::read_u16(&mut r).unwrap()),
            RandomUvi::U32(n, _)   => assert_eq!(n, io::read_u32(&mut r).unwrap()),
            RandomUvi::U64(n, _)   => assert_eq!(n, io::read_u64(&mut r).unwrap()),
            RandomUvi::U128(n, _)  => assert_eq!(n, io::read_u128(&mut r).unwrap()),
            RandomUvi::Usize(n, _) => assert_eq!(n, io::read_usize(&mut r).unwrap())
        }
    }
    quickcheck::quickcheck(property as fn(RandomUvi))
}

#[cfg(feature = "futures")]
#[test]
fn async_read_arbitrary() {
    use unsigned_varint::aio;

    fn property(n: RandomUvi) {
        futures_executor::block_on(async move {
            let mut r = futures_util::io::Cursor::new(n.bytes());
            match n {
                RandomUvi::U8(n,  _)   => assert_eq!(n, aio::read_u8(&mut r).await.unwrap()),
                RandomUvi::U16(n, _)   => assert_eq!(n, aio::read_u16(&mut r).await.unwrap()),
                RandomUvi::U32(n, _)   => assert_eq!(n, aio::read_u32(&mut r).await.unwrap()),
                RandomUvi::U64(n, _)   => assert_eq!(n, aio::read_u64(&mut r).await.unwrap()),
                RandomUvi::U128(n, _)  => assert_eq!(n, aio::read_u128(&mut r).await.unwrap()),
                RandomUvi::Usize(n, _) => assert_eq!(n, aio::read_usize(&mut r).await.unwrap())
            }
        })
    }
    quickcheck::quickcheck(property as fn(RandomUvi))
}

#[cfg(feature = "nom")]
#[test]
fn nom_read_arbitrary() {
    use unsigned_varint::nom;

    fn property(n: RandomUvi) {
        let input = n.bytes();
        let empty = &[][..];

        match n {
            RandomUvi::U8(n, _)    => assert_eq!((empty, n), nom::u8(input).unwrap()),
            RandomUvi::U16(n, _)   => assert_eq!((empty, n), nom::u16(input).unwrap()),
            RandomUvi::U32(n, _)   => assert_eq!((empty, n), nom::u32(input).unwrap()),
            RandomUvi::U64(n, _)   => assert_eq!((empty, n), nom::u64(input).unwrap()),
            RandomUvi::U128(n, _)  => assert_eq!((empty, n), nom::u128(input).unwrap()),
            RandomUvi::Usize(n, _) => assert_eq!((empty, n), nom::usize(input).unwrap()),
        }
    }
    quickcheck::quickcheck(property as fn(RandomUvi))
}

#[derive(Debug, Clone, PartialEq, Eq)]
enum RandomUvi {
    U8(u8, Vec<u8>),
    U16(u16, Vec<u8>),
    U32(u32, Vec<u8>),
    U64(u64, Vec<u8>),
    U128(u128, Vec<u8>),
    Usize(usize, Vec<u8>),
}

impl RandomUvi {
    #[cfg(feature = "std")]
    fn bytes(&self) -> &[u8] {
        match self {
            RandomUvi::U8(_,  v) => v,
            RandomUvi::U16(_, v) => v,
            RandomUvi::U32(_, v) => v,
            RandomUvi::U64(_, v) => v,
            RandomUvi::U128(_, v) => v,
            RandomUvi::Usize(_, v) => v
        }
    }
}

impl Arbitrary for RandomUvi {
    fn arbitrary(g: &mut Gen) -> Self {
        let n: u128 = Arbitrary::arbitrary(g);
        match n % 6 {
            0 => {
                let mut b = encode::u8_buffer();
                RandomUvi::U8(n as u8, Vec::from(encode::u8(n as u8, &mut b)))
            }
            1 => {
                let mut b = encode::u16_buffer();
                RandomUvi::U16(n as u16, Vec::from(encode::u16(n as u16, &mut b)))
            }
            2 => {
                let mut b = encode::u32_buffer();
                RandomUvi::U32(n as u32, Vec::from(encode::u32(n as u32, &mut b)))
            }
            3 => {
                let mut b = encode::u64_buffer();
                RandomUvi::U64(n as u64, Vec::from(encode::u64(n as u64, &mut b)))
            }
            4 => {
                let mut b = encode::u128_buffer();
                RandomUvi::U128(n, Vec::from(encode::u128(n, &mut b)))
            }
            _ => {
                let mut b = encode::usize_buffer();
                RandomUvi::Usize(n as usize, Vec::from(encode::usize(n as usize, &mut b)))
            }
        }
    }
}

