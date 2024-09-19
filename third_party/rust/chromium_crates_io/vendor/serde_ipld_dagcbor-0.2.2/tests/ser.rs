use serde_bytes::{ByteBuf, Bytes};
use serde_ipld_dagcbor::{from_slice, to_vec};
use std::collections::BTreeMap;

#[test]
fn test_string() {
    let value = "foobar".to_owned();
    assert_eq!(&to_vec(&value).unwrap()[..], b"ffoobar");
}

#[test]
fn test_list() {
    let value = vec![1, 2, 3];
    assert_eq!(&to_vec(&value).unwrap()[..], b"\x83\x01\x02\x03");
}

#[test]
fn test_object() {
    let mut object = BTreeMap::new();
    object.insert("a".to_owned(), "A".to_owned());
    object.insert("b".to_owned(), "B".to_owned());
    object.insert("c".to_owned(), "C".to_owned());
    object.insert("d".to_owned(), "D".to_owned());
    object.insert("e".to_owned(), "E".to_owned());
    let vec = to_vec(&object).unwrap();
    let test_object = from_slice(&vec[..]).unwrap();
    assert_eq!(object, test_object);
}

#[test]
fn test_float() {
    let vec = to_vec(&12.3f64).unwrap();
    assert_eq!(vec, b"\xfb@(\x99\x99\x99\x99\x99\x9a");
}

#[test]
fn test_f32() {
    let vec = to_vec(&4000.5f32).unwrap();
    assert_eq!(vec, b"\xfb\x40\xaf\x41\x00\x00\x00\x00\x00");
}

#[test]
fn test_infinity() {
    let vec = to_vec(&::std::f64::INFINITY);
    assert!(vec.is_err(), "Only finite numbers are supported.");
}

#[test]
fn test_neg_infinity() {
    let vec = to_vec(&::std::f64::NEG_INFINITY);
    assert!(vec.is_err(), "Only finite numbers are supported.");
}

#[test]
fn test_nan() {
    let vec = to_vec(&::std::f32::NAN);
    assert!(vec.is_err(), "Only finite numbers are supported.");
}

#[test]
fn test_integer() {
    // u8
    let vec = to_vec(&24).unwrap();
    assert_eq!(vec, b"\x18\x18");
    // i8
    let vec = to_vec(&-5).unwrap();
    assert_eq!(vec, b"\x24");
    // i16
    let vec = to_vec(&-300).unwrap();
    assert_eq!(vec, b"\x39\x01\x2b");
    // i32
    let vec = to_vec(&-23567997).unwrap();
    assert_eq!(vec, b"\x3a\x01\x67\x9e\x7c");
    // u64
    let vec = to_vec(&::std::u64::MAX).unwrap();
    assert_eq!(vec, b"\x1b\xff\xff\xff\xff\xff\xff\xff\xff");
}

#[test]
fn test_ip_addr() {
    use std::net::Ipv4Addr;

    let addr = Ipv4Addr::new(8, 8, 8, 8);
    let vec = to_vec(&addr).unwrap();
    println!("{:?}", vec);
    assert_eq!(vec.len(), 5);
    let test_addr: Ipv4Addr = from_slice(&vec).unwrap();
    assert_eq!(addr, test_addr);
}

/// Test all of CBOR's fixed-length byte string types
#[test]
fn test_byte_string() {
    // Very short byte strings have 1-byte headers
    let short = serde_bytes::Bytes::new(&[0, 1, 2, 255]);
    let short_s = to_vec(&short).unwrap();
    assert_eq!(short_s, [0x44, 0, 1, 2, 255]);

    // byte strings > 23 bytes have 2-byte headers
    let medium = Bytes::new(&[
        0u8, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 255,
    ]);
    let medium_s = to_vec(&medium).unwrap();
    assert_eq!(
        medium_s,
        [
            0x58, 24, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
            22, 255
        ]
    );

    // byte strings > 256 bytes have 3-byte headers
    let long_vec = ByteBuf::from((0..256).map(|i| (i & 0xFF) as u8).collect::<Vec<_>>());
    let long_s = to_vec(&long_vec).unwrap();
    assert_eq!(&long_s[0..3], [0x59, 1, 0]);
    assert_eq!(&long_s[3..], &long_vec[..]);

    // byte strings > 2^16 bytes have 5-byte headers
    let very_long_vec = ByteBuf::from((0..65536).map(|i| (i & 0xFF) as u8).collect::<Vec<_>>());
    let very_long_s = to_vec(&very_long_vec).unwrap();
    assert_eq!(&very_long_s[0..5], [0x5a, 0, 1, 0, 0]);
    assert_eq!(&very_long_s[5..], &very_long_vec[..]);

    // byte strings > 2^32 bytes have 9-byte headers, but they take too much RAM
    // to test in Travis.
}
