use std::{
    borrow::Cow,
    convert::{TryFrom, TryInto},
};

use quick_protobuf::{BytesReader, MessageRead};

use super::test_owned_pb::*;

#[test]
fn test_owned() {
    let t = trybuild::TestCases::new();
    t.compile_fail("tests/rust_protobuf/v2/test_owned_must_compile_error.rs");
}

#[test]
fn test_owned_basic_functionality() {
    let encoded: Vec<u8> = vec![
        0x0au8, 0x07u8, 0x74u8, 0x65u8, 0x73u8, 0x74u8, 0x69u8, 0x6eu8, 0x67u8,
    ]; // s = "testing"

    let foo_owned = FooOwned::try_from(encoded.clone()).unwrap();

    let foo_owned_temp = FooOwned::try_from(encoded.clone()).unwrap();
    let mut foo_owned_temp_mut = FooOwned::try_from(encoded.clone()).unwrap();
    let reencoded: Vec<u8> = foo_owned_temp.try_into().unwrap();

    let mut br_temp = BytesReader::from_bytes(&encoded);
    let foo = Foo::from_reader(&mut br_temp, &encoded).unwrap();

    let buf = foo_owned.buf().clone();
    let proto = foo_owned.proto().clone();
    let proto_mut = foo_owned_temp_mut.proto_mut();
    proto_mut.s = Some(Cow::from("the secret cow level"));

    let static_foo = Foo {
        s: Some(Cow::from("the moo moo farm")),
    };
    let from_static_foo = FooOwned::from(static_foo);

    assert_eq!(Some(Cow::from("testing")), foo_owned.proto().s); // test decoding and `proto()`

    assert_eq!(encoded, buf); // test `buf()`
    assert_eq!(foo, proto); // test `proto()`
    assert_eq!(
        foo_owned_temp_mut.proto().s,
        Some(Cow::from("the secret cow level"))
    ); // test `proto_mut()`
    assert_eq!(format!("{:?}", foo_owned), format!("{:?}", foo)); // test `try_from()` and `Debug`
    assert_eq!(encoded, reencoded); // test `try_into()`
    assert_eq!(
        Some(Cow::from("the moo moo farm")),
        from_static_foo.proto().s
    ); // test `from()`
}
