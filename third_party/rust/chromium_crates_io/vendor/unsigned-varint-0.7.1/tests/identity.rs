// Copyright 2018 Parity Technologies (UK) Ltd.
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

use quickcheck::QuickCheck;
use std::{u8, u16, u32, u64, u128};
use unsigned_varint::{decode::{self, Error}, encode};

#[test]
fn identity_u8() {
    let mut buf = encode::u8_buffer();
    for n in 0 .. u8::MAX {
        assert_eq!(n, decode::u8(encode::u8(n, &mut buf)).unwrap().0)
    }
}

#[test]
fn identity_u16() {
    let mut buf = encode::u16_buffer();
    for n in 0 .. u16::MAX {
        assert_eq!(n, decode::u16(encode::u16(n, &mut buf)).unwrap().0)
    }
}

#[test]
fn identity_u32() {
    let mut buf = encode::u32_buffer();
    for n in 0 .. 1000_000 {
        assert_eq!(n, decode::u32(encode::u32(n, &mut buf)).unwrap().0)
    }
    assert_eq!(u32::MAX, decode::u32(encode::u32(u32::MAX, &mut buf)).unwrap().0)
}

#[test]
fn identity_u64() {
    let mut buf = encode::u64_buffer();
    for n in 0 .. 1000_000 {
        assert_eq!(n, decode::u64(encode::u64(n, &mut buf)).unwrap().0)
    }
    assert_eq!(u64::MAX, decode::u64(encode::u64(u64::MAX, &mut buf)).unwrap().0)
}

#[test]
fn identity_u128() {
    let mut buf = encode::u128_buffer();
    for n in 0 .. 1000_000 {
        assert_eq!(n, decode::u128(encode::u128(n, &mut buf)).unwrap().0)
    }
    assert_eq!(u128::MAX, decode::u128(encode::u128(u128::MAX, &mut buf)).unwrap().0)
}

#[test]
fn identity() {
    fn prop(n: u64) -> bool {
        let mut buf = encode::u64_buffer();
        Ok(n) == decode::u64(encode::u64(n, &mut buf)).map(|r| r.0)
    }
    QuickCheck::new()
        .tests(1_000_000)
        .min_tests_passed(1_000_000)
        .max_tests(1_000_000)
        .quickcheck(prop as fn(u64) -> bool)
}

#[test]
fn various() {
    assert_eq!(Some(Error::Insufficient), decode::u8(&[]).err());
    assert_eq!(Some(Error::Insufficient), decode::u8(&[0x80]).err());
    assert_eq!(1, decode::u8(&[1]).unwrap().0);
    assert_eq!(127, decode::u8(&[0b0111_1111]).unwrap().0);
    assert_eq!(128, decode::u8(&[0b1000_0000, 1]).unwrap().0);
    assert_eq!(255, decode::u8(&[0b1111_1111, 1]).unwrap().0);
    assert_eq!(16384, decode::u16(&[0x80, 0x80, 1]).unwrap().0);
    assert_eq!(300, decode::u16(&[0b1010_1100, 0b0000_0010]).unwrap().0);
    assert_eq!(
        Some(Error::Overflow),
        decode::u64(&[0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80]).err()
    );
    assert_eq!(
        Some(Error::Insufficient),
        decode::u64(&[0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80]).err()
    );
    assert_eq!(
        0xFFFFFFFFFFFFFFFF,
        decode::u64(&[0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 1]).unwrap().0
    )
}

#[cfg(feature = "codec")]
#[test]
fn identity_codec() {
    use bytes::{Bytes, BytesMut};
    use quickcheck::Gen;
    use tokio_util::codec::{Encoder, Decoder};
    use unsigned_varint::codec::UviBytes;

    fn prop(mut xs: Vec<u8>) -> bool {
        let mut codec = UviBytes::default();
        xs.truncate(codec.max_len());
        let input = Bytes::from(xs);
        let mut buffer = BytesMut::with_capacity(input.len());
        assert!(codec.encode(input.clone(), &mut buffer).is_ok());
        input == codec.decode(&mut buffer).expect("Ok").expect("Some").freeze()
    }

    QuickCheck::new().gen(Gen::new(512 * 1024))
        .quickcheck(prop as fn(Vec<u8>) -> bool)
}

