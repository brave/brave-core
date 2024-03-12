#![cfg(feature = "use_std")]

use std::convert::Infallible;
use anyhow::Context;
use cbor4ii::core::Value;
use cbor4ii::core::enc::{ self, Encode };
use cbor4ii::core::dec::{ self, Decode };
use cbor4ii::core::utils::{ BufWriter, SliceReader };

#[test]
fn test_decode_value() {
    macro_rules! test {
        ( @ $input:expr ) => {
            let buf = data_encoding::BASE64.decode($input.as_bytes()).unwrap();
            let mut reader = SliceReader::new(buf.as_slice());
            let _ = Value::decode(&mut reader);
        };
        ( $( $input:expr );* $( ; )? ) => {
            $(
                test!(@ $input );
            )*
        }
    }

    test!{
        "ig==";
        "eoY=";
        "v6a/v6a/v7+/pq6urq6urq6urq6urq6urq6urq6urq6urq6urqaurq6urq6urq4krq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6uv7+mv7+/v6aurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urr+/pr+/v7+mrq6urq6urq6urq6urq6urq6urq6urq6upq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6uQK6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urqSurq6urq6urq6urq6urq6urq6urq6urq6urq6uv7+uJa6urq6urq6urq6urq6urq6urq6urq6urq6urq6uv7+uJa6urq6urq6urq6urq6urq6uQK6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upK6urq6urq6urq6urq6urq6urq6urq6urq6urq6/v64lrq6urq6urq6urq6urq6urq6urq6urq6urq6urq6/v64lrq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6krq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upK6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urqaurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upK6urq6urq6urq6urq6urq6urq6urq6urq6urq6mrq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upK6urq6urq6urq6urq6urq6urq6urq6urq6urq6/v64lrq6urq6urq6urq6urq6urq6urq6urq6urq6urq6/v64lrq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6krq6urq6urq6urq6urq6urq6urq6urq6urq6urr+/riWurq6urq6urq6urq6urq6urq6urq6urq6urq6urr+/riWurq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6uQK6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urqSurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6mrq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6krq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6krq6urq6urq6urq6urq6urq6urq6urq6urqSurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upK6urq6urq6urq6urq6urq6urq6urq6urq6urq6/v64lrq6urq6urq6urq6urq6urq6urq6urq6urq6urq6/v64lrq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6krq6urq6urq6urq6urq6urq6urq6urq6urq6urr+/riWurq6urq6urq6urq6urg0AAAAAAAAArq6urq6urr+/riWurq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6uQK6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urqSurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6mrq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6krq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6krq6urq6urq6urq6urq6urq6urq6urq6urq6urr+/riWurq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6krq6urq6urq6vrq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urrCwsLCwsLCwsLCwsLCwsLCwsLCwrq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urkCurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6/v64lrq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upK6urq6urq6ur66urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6wsLCwsLCwsLCwsLCwsLCwsLCwsK6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urqaurq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upK6urq6urq6urq6urq6urq6urq6urq6urq6urq6/v64lrq6urq6urq6urq6urq6urq5Arq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6upK6urq6urq6ur66urq6urq6urq6urq6urq6urq6/v64lrq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6wsLCwsLCwsLCwsLCwsLCwsLCwsK6urq6urq6urq6urq6urq6urq6urq6urq6urq6urq6urrCwsLCwsLCwsLCwsLCwsLCwsLCwsIQEAAAAgQCwsLCwsLCwsLCwsK6urq6urq6u";
        "v/b29vYBAAAAAAAABPb29gn29vb29pkAEfb29vb29vb2f3///39//3//f/9//3//f/9//3//f///f/9//3//f/8ICAgICAgI9vf39wgICAgICAgI+EAKCgr39wv1CAgICCgILggItAgICAgICAgICAgAAACgAAgICAAICAgICAgI9vf39wgICAgICAgI+EAKCgr39wv1CAgICCgILggItAgICAgICAgICAgAAACgAAgICA=="
    }
}

#[test]
fn test_decode_buf_segment() -> anyhow::Result<()> {
    let mut writer = BufWriter::new(Vec::new());
    enc::StrStart.encode(&mut writer)?;
    "test".encode(&mut writer)?;
    "test2".encode(&mut writer)?;
    "test3".encode(&mut writer)?;
    enc::End.encode(&mut writer)?;

    let mut reader = SliceReader::new(writer.buffer());
    let output = String::decode(&mut reader)?;
    assert_eq!("testtest2test3", output);

    Ok(())
}

#[test]
fn test_decode_bad_reader_buf() -> anyhow::Result<()> {
    let mut writer = BufWriter::new(Vec::new());
    "test".encode(&mut writer)?;

    struct LongReader<'a>(&'a [u8]);

    impl<'de> dec::Read<'de> for LongReader<'de> {
        type Error = Infallible;

        #[inline]
        fn fill<'b>(&'b mut self, _want: usize) -> Result<dec::Reference<'de, 'b>, Self::Error> {
            Ok(dec::Reference::Short(&self.0))
        }

        #[inline]
        fn advance(&mut self, n: usize) {
            let len = core::cmp::min(self.0.len(), n);
            self.0 = &self.0[len..];
        }
    }

    let mut buf = writer.into_inner();
    buf.resize(1024, 0);
    let mut reader = LongReader(&buf);
    let output = String::decode(&mut reader)?;
    assert_eq!("test", output);

    Ok(())
}

#[test]
fn test_decode_array_map() -> anyhow::Result<()> {
    // array bounded
    let mut writer = BufWriter::new(Vec::new());
    (&[0u32, 1, 2, 3, 4, 5][..]).encode(&mut writer)?;

    let mut reader = SliceReader::new(writer.buffer());
    let dec::ArrayStart(len) = dec::ArrayStart::decode(&mut reader)?;
    let len = len.context("expect len")?;
    for i in 0..len {
        let n = u64::decode(&mut reader)?;
        assert_eq!(n, i as u64);
    }

    // map unbounded
    let mut writer = BufWriter::new(Vec::new());
    enc::MapStartUnbounded.encode(&mut writer)?;
    for i in 0u64..6 {
        i.encode(&mut writer)?;
    }
    enc::End.encode(&mut writer)?;

    let mut reader = SliceReader::new(writer.buffer());
    let dec::MapStart(len) = dec::MapStart::decode(&mut reader)?;
    assert_eq!(len, None);
    let mut count = 0u64;
    while !dec::is_break(&mut reader)? {
        let n = u64::decode(&mut reader)?;
        assert_eq!(n, count);
        count += 1;
    }

    Ok(())
}

#[test]
fn test_value_i128() -> anyhow::Result<()> {
    #[inline]
    fn strip_zero(input: &[u8]) -> &[u8] {
        let pos = input.iter()
            .position(|&n| n != 0x0)
            .unwrap_or(input.len());
        &input[pos..]
    }

    let n = u64::MAX as i128 + 99;

    let mut writer = BufWriter::new(Vec::new());
    Value::Integer(n).encode(&mut writer)?;

    let mut reader = SliceReader::new(writer.buffer());
    let value = Value::decode(&mut reader)?;

    let int_bytes = n.to_be_bytes();
    let int_bytes = strip_zero(&int_bytes);
    assert_eq!(value, Value::Tag(2, Box::new(Value::Bytes(int_bytes.into()))));

    Ok(())
}

#[test]
fn test_mut_ref_write_read() -> anyhow::Result<()> {
    fn test_write_str<W: enc::Write>(mut writer: W, input: &str) {
        input.encode(&mut writer).unwrap()
    }

    fn test_read_str<'de, R: dec::Read<'de>>(mut reader: R) -> &'de str {
        <&str>::decode(&mut reader).unwrap()
    }

    let input = "123";

    let mut writer = BufWriter::new(Vec::new());
    test_write_str(&mut writer, input);

    let mut reader = SliceReader::new(writer.buffer());
    let output = test_read_str(&mut reader);

    assert_eq!(input, output);

    Ok(())
}

#[test]
fn test_regression_ignore_tag() {
    let tag = Value::Tag(u64::MAX - 1, Box::new(Value::Text("hello world".into())));

    let mut buf = BufWriter::new(Vec::new());
    tag.encode(&mut buf).unwrap();

    {
        let mut reader = SliceReader::new(buf.buffer());
        let tag2 = Value::decode(&mut reader).unwrap();
        assert_eq!(tag, tag2);
    }

    {
        let mut reader = SliceReader::new(buf.buffer());
        let _ignored = dec::IgnoredAny::decode(&mut reader).unwrap();
    }
}

#[test]
fn test_regression_min_i64() {
    let mut buf = BufWriter::new(Vec::new());
    i64::MIN.encode(&mut buf).unwrap();

    let mut reader = SliceReader::new(buf.buffer());
    let min_i64 = i64::decode(&mut reader).unwrap();

    assert_eq!(min_i64, i64::MIN);
}

#[test]
fn test_regression_min_i128() {
    let mut buf = BufWriter::new(Vec::new());
    i128::MIN.encode(&mut buf).unwrap();

    let mut reader = SliceReader::new(buf.buffer());
    let min_i128 = i128::decode(&mut reader).unwrap();

    assert_eq!(min_i128, i128::MIN);
}

#[test]
fn test_regression_max_neg_64_as_i128() {
    let mut buf = BufWriter::new(Vec::new());
    let max_neg_64 = -i128::from(u64::MAX) - 1;
    max_neg_64.encode(&mut buf).unwrap();
    assert_eq!(buf.buffer(), [0x3b, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff]);

    let mut reader = SliceReader::new(buf.buffer());
    let decoded = i128::decode(&mut reader).unwrap();
    assert_eq!(decoded, -18446744073709551616);
}

#[test]
fn test_max_neg_32_as_i64() {
    let mut buf = BufWriter::new(Vec::new());
    let max_neg_32 = -i64::from(u32::MAX) - 1;
    max_neg_32.encode(&mut buf).unwrap();
    assert_eq!(buf.buffer(), [0x3a, 0xff, 0xff, 0xff, 0xff]);

    let mut reader = SliceReader::new(buf.buffer());
    let decoded = i64::decode(&mut reader).unwrap();
    assert_eq!(decoded, -4294967296);
}

#[test]
fn test_max_neg_16_as_i32() {
    let mut buf = BufWriter::new(Vec::new());
    let max_neg_16 = -i32::from(u16::MAX) - 1;
    max_neg_16.encode(&mut buf).unwrap();
    assert_eq!(buf.buffer(), [0x39, 0xff, 0xff]);

    let mut reader = SliceReader::new(buf.buffer());
    let decoded = i32::decode(&mut reader).unwrap();
    assert_eq!(decoded, -65536);
}

#[test]
fn test_max_neg_8_as_i16() {
    let mut buf = BufWriter::new(Vec::new());
    let max_neg_8 = -i16::from(u8::MAX) - 1;
    max_neg_8.encode(&mut buf).unwrap();
    assert_eq!(buf.buffer(), [0x38, 0xff]);

    let mut reader = SliceReader::new(buf.buffer());
    let decoded = i16::decode(&mut reader).unwrap();
    assert_eq!(decoded, -256);
}

#[test]
fn test_tag_start() {
    let mut reader_tag_len1 = SliceReader::new(&[0xd8, 0x2a]);
    let tag_len1 = dec::TagStart::decode(&mut reader_tag_len1).unwrap();
    assert_eq!(tag_len1.0, 42);

    let mut reader_tag_len2 = SliceReader::new(&[0xd9, 0x00, 0x2a]);
    let tag_len2 = dec::TagStart::decode(&mut reader_tag_len2).unwrap();
    assert_eq!(tag_len2.0, 42);

    let mut reader_tag_len4 = SliceReader::new(&[0xda, 0x00, 0x00, 0x00, 0x2a]);
    let tag_len4 = dec::TagStart::decode(&mut reader_tag_len4).unwrap();
    assert_eq!(tag_len4.0, 42);

    let mut reader_tag_len8 = SliceReader::new(
        &[0xdb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a]);
    let tag_len8 = dec::TagStart::decode(&mut reader_tag_len8).unwrap();
    assert_eq!(tag_len8.0, 42);
}


#[test]
fn test_ignored_any_eof_loop() {
    let mut buf = BufWriter::new(Vec::new());
    "aaa".encode(&mut buf).unwrap();

    // bad input
    let mut buf = buf.into_inner();
    buf.pop();

    let mut reader = SliceReader::new(&buf);
    let ret = dec::IgnoredAny::decode(&mut reader);

    match ret {
        Err(dec::Error::Eof) => (),
        _ => panic!()
    }
}
