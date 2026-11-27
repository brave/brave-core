#![warn(rust_2018_idioms)]
#![cfg(feature = "std")]

use std::io::{self, Write};

use bytes::{BufMut, BytesMut};

#[test]
fn write() {
    let mut writer = vec![].writer();
    assert_eq!(5, writer.write(b"hello").unwrap());
    assert_eq!(6, writer.write(b" world").unwrap());
    writer.flush().unwrap();
    assert_eq!(b"hello world", &writer.into_inner()[..]);
}

#[test]
fn write_partial() {
    let mut dst = [0; 8];
    let mut writer = (&mut dst[..]).writer();
    assert_eq!(5, writer.write(b"hello").unwrap());
    assert_eq!(3, writer.write(b" world").unwrap());
    assert_eq!(0, writer.write(b"!").unwrap());
    assert_eq!(b"hello wo", &dst);
}

#[test]
fn write_all() {
    let mut writer = BytesMut::new().writer();
    writer.write_all(b"hello").unwrap();
    writer.write_all(b"").unwrap();
    writer.write_all(b" world").unwrap();
    assert_eq!(b"hello world", &writer.into_inner()[..]);
}

#[test]
fn write_all_exact_fit() {
    let mut dst = [0; 5];
    let mut writer = (&mut dst[..]).writer();
    writer.write_all(b"hello").unwrap();
    assert_eq!(0, writer.get_ref().remaining_mut());
    assert_eq!(b"hello", &dst);
}

#[test]
fn write_all_write_zero() {
    let mut dst = [0; 8];
    let mut writer = (&mut dst[..]).writer();
    writer.write_all(b"hello").unwrap();

    // Writes what still fits before failing, like the default impl.
    let err = writer.write_all(b" world").unwrap_err();
    assert_eq!(io::ErrorKind::WriteZero, err.kind());
    assert_eq!(0, writer.get_ref().remaining_mut());
    assert_eq!(b"hello wo", &dst);
}

#[test]
fn write_all_limit() {
    let mut writer = BytesMut::new().limit(8).writer();
    writer.write_all(b"hello").unwrap();

    let err = writer.write_all(b" world").unwrap_err();
    assert_eq!(io::ErrorKind::WriteZero, err.kind());
    assert_eq!(b"hello wo", &writer.into_inner().into_inner()[..]);

    let mut writer = BytesMut::new().limit(0).writer();
    writer.write_all(b"").unwrap();
    assert!(writer.write_all(b"a").is_err());
}
