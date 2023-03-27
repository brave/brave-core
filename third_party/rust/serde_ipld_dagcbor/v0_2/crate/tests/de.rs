use std::collections::BTreeMap;

use libipld_core::ipld::Ipld;
use serde_ipld_dagcbor::{de, to_vec, DecodeError};

#[test]
fn test_string1() {
    let ipld: Result<Ipld, _> = de::from_slice(&[0x66, 0x66, 0x6f, 0x6f, 0x62, 0x61, 0x72]);
    assert_eq!(ipld.unwrap(), Ipld::String("foobar".to_string()));
}

#[test]
fn test_string2() {
    let ipld: Result<Ipld, _> = de::from_slice(&[
        0x71, 0x49, 0x20, 0x6d, 0x65, 0x74, 0x20, 0x61, 0x20, 0x74, 0x72, 0x61, 0x76, 0x65, 0x6c,
        0x6c, 0x65, 0x72,
    ]);
    assert_eq!(ipld.unwrap(), Ipld::String("I met a traveller".to_string()));
}

#[test]
fn test_string3() {
    let slice = b"\x78\x2fI met a traveller from an antique land who said";
    let ipld: Result<Ipld, _> = de::from_slice(slice);
    assert_eq!(
        ipld.unwrap(),
        Ipld::String("I met a traveller from an antique land who said".to_string())
    );
}

#[test]
fn test_byte_string() {
    let ipld: Result<Ipld, _> = de::from_slice(&[0x46, 0x66, 0x6f, 0x6f, 0x62, 0x61, 0x72]);
    assert_eq!(ipld.unwrap(), Ipld::Bytes(b"foobar".to_vec()));
}

#[test]
fn test_numbers1() {
    let ipld: Result<Ipld, _> = de::from_slice(&[0x00]);
    assert_eq!(ipld.unwrap(), Ipld::Integer(0));
}

#[test]
fn test_numbers2() {
    let ipld: Result<Ipld, _> = de::from_slice(&[0x1a, 0x00, 0xbc, 0x61, 0x4e]);
    assert_eq!(ipld.unwrap(), Ipld::Integer(12345678));
}

#[test]
fn test_numbers3() {
    let ipld: Result<Ipld, _> = de::from_slice(&[0x39, 0x07, 0xde]);
    assert_eq!(ipld.unwrap(), Ipld::Integer(-2015));
}

#[test]
fn test_bool() {
    let ipld: Result<Ipld, _> = de::from_slice(b"\xf4");
    assert_eq!(ipld.unwrap(), Ipld::Bool(false));
}

#[test]
fn test_trailing_bytes() {
    let ipld: Result<Ipld, _> = de::from_slice(b"\xf4trailing");
    assert!(matches!(ipld.unwrap_err(), DecodeError::TrailingData));
}

#[test]
fn test_list1() {
    let ipld: Result<Ipld, _> = de::from_slice(b"\x83\x01\x02\x03");
    assert_eq!(
        ipld.unwrap(),
        Ipld::List(vec![Ipld::Integer(1), Ipld::Integer(2), Ipld::Integer(3)])
    );
}

#[test]
fn test_list2() {
    let ipld: Result<Ipld, _> = de::from_slice(b"\x82\x01\x82\x02\x81\x03");
    assert_eq!(
        ipld.unwrap(),
        Ipld::List(vec![
            Ipld::Integer(1),
            Ipld::List(vec![Ipld::Integer(2), Ipld::List(vec![Ipld::Integer(3)])])
        ])
    );
}

#[test]
fn test_object() {
    let ipld: Result<Ipld, _> = de::from_slice(b"\xa5aaaAabaBacaCadaDaeaE");
    let mut object = BTreeMap::new();
    object.insert("a".to_string(), Ipld::String("A".to_string()));
    object.insert("b".to_string(), Ipld::String("B".to_string()));
    object.insert("c".to_string(), Ipld::String("C".to_string()));
    object.insert("d".to_string(), Ipld::String("D".to_string()));
    object.insert("e".to_string(), Ipld::String("E".to_string()));
    assert_eq!(ipld.unwrap(), Ipld::Map(object));
}

#[test]
fn test_indefinite_object() {
    let ipld: Result<Ipld, _> = de::from_slice(b"\xbfaa\x01ab\x9f\x02\x03\xff\xff");
    let mut object = BTreeMap::new();
    object.insert("a".to_string(), Ipld::Integer(1));
    object.insert(
        "b".to_string(),
        Ipld::List(vec![Ipld::Integer(2), Ipld::Integer(3)]),
    );
    assert_eq!(ipld.unwrap(), Ipld::Map(object));
}

#[test]
fn test_indefinite_list() {
    let ipld: Result<Ipld, _> = de::from_slice(b"\x9f\x01\x02\x03\xff");
    assert_eq!(
        ipld.unwrap(),
        Ipld::List(vec![Ipld::Integer(1), Ipld::Integer(2), Ipld::Integer(3)])
    );
}

#[test]
fn test_indefinite_string() {
    let ipld: Result<Ipld, _> =
        de::from_slice(b"\x7f\x65Mary \x64Had \x62a \x67Little \x60\x64Lamb\xff");
    assert_eq!(
        ipld.unwrap(),
        Ipld::String("Mary Had a Little Lamb".to_string())
    );
}

#[test]
fn test_indefinite_byte_string() {
    let ipld: Result<Ipld, _> = de::from_slice(b"\x5f\x42\x01\x23\x42\x45\x67\xff");
    assert_eq!(ipld.unwrap(), Ipld::Bytes(b"\x01#Eg".to_vec()));
}

#[test]
fn test_multiple_indefinite_strings() {
    let input = b"\x82\x7f\x65Mary \x64Had \x62a \x67Little \x60\x64Lamb\xff\x5f\x42\x01\x23\x42\x45\x67\xff";
    let ipld: Result<Ipld, _> = de::from_slice(input);
    assert_eq!(
        ipld.unwrap(),
        Ipld::List(vec![
            Ipld::String("Mary Had a Little Lamb".to_string()),
            Ipld::Bytes(b"\x01#Eg".to_vec())
        ])
    );
}

#[test]
fn test_float() {
    let ipld: Result<Ipld, _> = de::from_slice(b"\xfa\x47\xc3\x50\x00");
    assert_eq!(ipld.unwrap(), Ipld::Float(100000.0));
}

#[test]
fn test_rejected_tag() {
    let ipld: Result<Ipld, _> =
        de::from_slice(&[0xd9, 0xd9, 0xf7, 0x66, 0x66, 0x6f, 0x6f, 0x62, 0x61, 0x72]);
    assert!(matches!(
        ipld.unwrap_err(),
        DecodeError::TypeMismatch {
            name: "CBOR tag",
            byte: 0xf7
        }
    ));
}

#[test]
fn test_crazy_list() {
    let slice = b"\x86\x1b\x00\x00\x00\x1c\xbe\x99\x1d\xc7\x3b\x00\x7a\xcf\x51\xdc\x51\x70\xdb\x3a\x1b\x3a\x06\xdd\xf5\xf6\xfb\x41\x76\x5e\xb1\xf8\x00\x00\x00";
    let ipld: Vec<Ipld> = de::from_slice(slice).unwrap();
    assert_eq!(
        ipld,
        vec![
            Ipld::Integer(123456789959),
            Ipld::Integer(-34567897654325468),
            Ipld::Integer(-456787678),
            Ipld::Bool(true),
            Ipld::Null,
            Ipld::Float(23456543.5),
        ]
    );
}

#[test]
fn test_nan() {
    let ipld: Result<f64, _> = de::from_slice(b"\xf9\x7e\x00");
    assert!(matches!(
        ipld.unwrap_err(),
        DecodeError::TypeMismatch { .. }
    ));
}

#[test]
// The file was reported as not working by user kie0tauB
// but it parses to a cbor value.
fn test_kietaub_file() {
    let file = include_bytes!("kietaub.cbor");
    let value_result: Result<Ipld, _> = de::from_slice(file);
    value_result.unwrap();
}

#[test]
fn test_option_roundtrip() {
    let obj1 = Some(10u32);

    let v = to_vec(&obj1).unwrap();
    let obj2: Result<Option<u32>, _> = de::from_slice(&v[..]);
    println!("{:?}", obj2);

    assert_eq!(obj1, obj2.unwrap());
}

#[test]
fn test_option_none_roundtrip() {
    let obj1 = None;

    let v = to_vec(&obj1).unwrap();
    println!("{:?}", v);
    let obj2: Result<Option<u32>, _> = de::from_slice(&v[..]);

    assert_eq!(obj1, obj2.unwrap());
}

#[test]
fn test_unit() {
    let unit = ();
    let v = to_vec(&unit).unwrap();
    assert_eq!(v, [0xf6], "unit is serialized as NULL.");
    let result: Result<(), _> = from_slice(&v);
    assert!(result.is_ok(), "unit was successfully deserialized");
}

#[test]
fn test_variable_length_map() {
    let slice = b"\xbf\x67\x6d\x65\x73\x73\x61\x67\x65\x64\x70\x6f\x6e\x67\xff";
    let ipld: Ipld = de::from_slice(slice).unwrap();
    let mut map = BTreeMap::new();
    map.insert("message".to_string(), Ipld::String("pong".to_string()));
    assert_eq!(ipld, Ipld::Map(map))
}

#[test]
fn test_object_determinism_roundtrip() {
    let expected = b"\xa2aa\x01ab\x82\x02\x03";

    // 0.1% chance of not catching failure
    for _ in 0..10 {
        assert_eq!(
            &to_vec(&de::from_slice::<Ipld>(expected).unwrap()).unwrap(),
            expected
        );
    }
}

#[test]
fn crash() {
    let file = include_bytes!("crash.cbor");
    let value_result: Result<Ipld, _> = de::from_slice(file);
    assert!(matches!(value_result.unwrap_err(), DecodeError::Eof));
}

use serde_ipld_dagcbor::de::from_slice;
use std::net::{IpAddr, Ipv4Addr};
#[test]
fn test_ipaddr_deserialization() {
    let ip = IpAddr::V4(Ipv4Addr::new(127, 0, 0, 1));
    let buf = to_vec(&ip).unwrap();
    let deserialized_ip = from_slice::<IpAddr>(&buf).unwrap();
    assert_eq!(ip, deserialized_ip);
}

#[test]
fn attempt_stack_overflow() {
    // Create a tag 17, followed by 999 more tag 17:
    // 17(17(17(17(17(17(17(17(17(17(17(17(17(17(17(17(17(17(...
    // This causes deep recursion in the decoder and may
    // exhaust the stack and therfore result in a stack overflow.
    let input = vec![0xd1; 1000];
    serde_ipld_dagcbor::from_slice::<Ipld>(&input).expect_err("recursion limit");
}

#[test]
fn truncated_object() {
    let input: Vec<u8> = [
        &b"\x84\x87\xD8\x2A\x58\x27\x00\x01\x71\xA0\xE4\x02\x20\x83\xEC\x9F\x76\x1D"[..],
        &b"\xB5\xEE\xA0\xC8\xE1\xB5\x74\x0D\x1F\x0A\x1D\xB1\x8A\x52\x6B\xCB\x42\x69"[..],
        &b"\xFD\x99\x24\x9E\xCE\xA9\xE8\xFD\x24\xD8\x2A\x58\x27\x00\x01\x71\xA0\xE4"[..],
        &b"\x02\x20\xF1\x9B\xC1\x42\x83\x31\xB1\x39\xB3\x3F\x43\x02\x87\xCC\x1C\x12"[..],
        &b"\xF2\x84\x47\xA3\x9B\x07\x59\x40\x17\x68\xFE\xE8\x09\xBB\xF2\x54\xD8\x2A"[..],
        &b"\x58\x27\x00\x01\x71\xA0\xE4\x02\x20\xB0\x75\x09\x92\x78\x6B\x6B\x4C\xED"[..],
        &b"\xF0\xE1\x50\xA3\x1C\xAB\xDF\x25\xA9\x26\x8C\x63\xDD\xCB\x25\x73\x6B\xF5"[..],
        &b"\x8D\xE8\xA4\x24\x29"[..],
    ]
    .concat();
    serde_ipld_dagcbor::from_slice::<Ipld>(&input).expect_err("truncated");
}

#[test]
fn invalid_string() {
    // Non UTF-8 byte sequence, but using major type 3 (text string)
    let input = [0x63, 0xc5, 0x01, 0x02];
    let result = serde_ipld_dagcbor::from_slice::<Ipld>(&input);
    assert!(matches!(
        result.unwrap_err(),
        DecodeError::InvalidUtf8 { .. }
    ));
}

#[cfg(feature = "_do_not_use_its_unsafe_and_invalid_cbor")]
#[test]
fn do_not_use_its_unsafe_and_invalid_cbor_test() {
    let input = [0x63, 0xc5, 0x01, 0x02];
    let result = serde_ipld_dagcbor::from_slice::<Ipld>(&input);
    assert!(result.is_ok())
}
