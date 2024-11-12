use std::{collections::BTreeMap, iter};

use serde::de::value::{self, MapDeserializer, SeqDeserializer};
use serde_bytes::{ByteBuf, Bytes};
use serde_derive::Serialize;
use serde_ipld_dagcbor::{
    from_slice,
    ser::{BufWriter, Serializer},
    to_vec,
};

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
    // u128 within u64 range
    let vec = to_vec(&(u64::MAX as u128)).unwrap();
    assert_eq!(vec, b"\x1b\xff\xff\xff\xff\xff\xff\xff\xff");
    // u128 out of range
    assert!(to_vec(&(u64::MAX as u128 + 1)).is_err());
    // i128 within u64 range
    let vec = to_vec(&(u64::MAX as i128)).unwrap();
    assert_eq!(vec, b"\x1b\xff\xff\xff\xff\xff\xff\xff\xff");
    // i128 within -u64 range
    let vec = to_vec(&(-(u64::MAX as i128))).unwrap();
    assert_eq!(vec, b"\x3B\xff\xff\xff\xff\xff\xff\xff\xfe");
    // minimum CBOR integer value
    let vec = to_vec(&(-(u64::MAX as i128 + 1))).unwrap();
    assert_eq!(vec, b"\x3B\xff\xff\xff\xff\xff\xff\xff\xff");
    // i128 out of -u64 range
    assert!(to_vec(&i128::MIN).is_err());
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

/// This test checks that the keys of a map are sorted correctly, independently of the order of the
/// input.
#[test]
fn test_key_order_transcode_map() {
    // CBOR encoded {"a": 1, "b": 2}
    let expected = [0xa2, 0x61, 0x61, 0x01, 0x61, 0x62, 0x02];

    let data = vec![("b", 2), ("a", 1)];
    let deserializer: MapDeserializer<'_, _, value::Error> = MapDeserializer::new(data.into_iter());
    let writer = BufWriter::new(Vec::new());
    let mut serializer = Serializer::new(writer);
    serde_transcode::transcode(deserializer, &mut serializer).unwrap();
    let result = serializer.into_inner().into_inner();
    assert_eq!(result, expected);
}

// This test makes sure that even unbound lists are not encoded as such (as lists in DAG-CBOR need
// to be finite).
#[test]
fn test_non_unbound_list() {
    // Create an iterator that has no size hint. This would trigger the "unbounded code path" for
    // sequences.
    let one_two_three_iter = iter::successors(
        Some(1),
        move |&num| {
            if num < 3 {
                Some(num + 1)
            } else {
                None
            }
        },
    );

    // CBOR encoded [1, 2, 3]
    let expected = [0x83, 0x01, 0x02, 0x03];

    let deserializer: SeqDeserializer<_, value::Error> = SeqDeserializer::new(one_two_three_iter);
    let writer = BufWriter::new(Vec::new());
    let mut serializer = Serializer::new(writer);
    serde_transcode::transcode(deserializer, &mut serializer).unwrap();
    let result = serializer.into_inner().into_inner();
    assert_eq!(result, expected);
}

#[test]
fn test_struct_canonical() {
    #[derive(Serialize)]
    struct First {
        a: u32,
        b: u32,
    }
    #[derive(Serialize)]
    struct Second {
        b: u32,
        a: u32,
    }

    let first = First { a: 1, b: 2 };
    let second = Second { a: 1, b: 2 };

    let first_bytes = serde_ipld_dagcbor::to_vec(&first).unwrap();
    let second_bytes = serde_ipld_dagcbor::to_vec(&second).unwrap();

    assert_eq!(first_bytes, second_bytes);
    // Do not only make sure that the order is the same, but also that it's correct.
    assert_eq!(first_bytes, b"\xa2\x61a\x01\x61b\x02")
}

#[test]
fn test_struct_variant_canonical() {
    // The `abc` is there to make sure it really follows the DAG-CBOR sorting order, which sorts by
    // length of the keys first, then lexicographically. It means that `abc` sorts *after* `b`.
    #[derive(Serialize)]
    enum First {
        Data { a: u8, b: u8, abc: u8 },
    }

    #[derive(Serialize)]
    enum Second {
        Data { b: u8, abc: u8, a: u8 },
    }

    let first = First::Data { a: 1, b: 2, abc: 3 };
    let second = Second::Data { a: 1, b: 2, abc: 3 };

    let first_bytes = serde_ipld_dagcbor::to_vec(&first).unwrap();
    let second_bytes = serde_ipld_dagcbor::to_vec(&second).unwrap();

    assert_eq!(first_bytes, second_bytes);
    // Do not only make sure that the order is the same, but also that it's correct.
    assert_eq!(
        first_bytes,
        b"\xa1\x64Data\xa3\x61a\x01\x61b\x02\x63abc\x03"
    )
}
