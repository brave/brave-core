#![feature(test)]

extern crate quick_protobuf;
extern crate test;

#[macro_use]
extern crate lazy_static;

use test::{black_box, Bencher};

use quick_protobuf::{BytesReader, Writer};

const LEN: i32 = 10_000;

lazy_static! {
    static ref BUFFER: Vec<u8> = {
        let mut buf = Vec::new();
        {
            let mut writer = Writer::new(&mut buf);
            for i in 0..LEN {
                writer.write_int32(i).unwrap();
            }
        }
        buf
    };
}

#[bench]
fn read_varint32(b: &mut Bencher) {
    b.iter(|| {
        let mut reader = BytesReader::from_bytes(&BUFFER);
        for _ in 0..LEN {
            let _ = black_box(reader.read_varint32(&BUFFER).unwrap());
        }
        assert!(reader.is_eof());
    })
}

#[bench]
fn read_varint64(b: &mut Bencher) {
    b.iter(|| {
        let mut reader = BytesReader::from_bytes(&BUFFER);
        for _ in 0..LEN {
            let _ = black_box(reader.read_varint64(&BUFFER).unwrap());
        }
        assert!(reader.is_eof());
    })
}

#[bench]
fn read_varint64_and_is_eof(b: &mut Bencher) {
    b.iter(|| {
        let mut reader = BytesReader::from_bytes(&BUFFER);
        for _ in 0..LEN {
            assert!(!reader.is_eof());
            let _ = black_box(reader.read_varint64(&BUFFER).unwrap());
        }
    })
}
