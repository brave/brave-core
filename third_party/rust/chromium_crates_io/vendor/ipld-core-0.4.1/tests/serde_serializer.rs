#![cfg(feature = "serde")]

extern crate alloc;

use alloc::collections::BTreeMap;
use core::convert::TryFrom;

use serde::ser;
use serde_bytes::ByteBuf;
use serde_derive::Serialize;

use ipld_core::cid::Cid;
use ipld_core::ipld::Ipld;
use ipld_core::serde::to_ipld;

fn assert_serialized<T>(input: T, ipld: Ipld)
where
    T: ser::Serialize,
{
    let serialized = to_ipld(input).unwrap();
    assert_eq!(serialized, ipld);
}

#[test]
#[allow(clippy::let_unit_value)]
fn ipld_serializer_unit() {
    let unit = ();
    let serialized = to_ipld(unit);
    assert!(serialized.is_err());
}

#[test]
fn ipld_serializer_unit_struct() {
    #[derive(Clone, Debug, Serialize, PartialEq)]
    struct UnitStruct;

    let unit_struct = UnitStruct;
    let serialized = to_ipld(unit_struct);
    assert!(serialized.is_err());
}

#[test]
fn ipld_serializer_bool() {
    let bool = false;
    let ipld = Ipld::Bool(bool);
    assert_serialized(bool, ipld);
}

#[test]
fn ipld_serializer_u8() {
    let integer = 34u8;
    let ipld = Ipld::Integer(integer.into());
    assert_serialized(integer, ipld);
}

#[test]
fn ipld_serializer_u16() {
    let integer = 345u16;
    let ipld = Ipld::Integer(integer.into());
    assert_serialized(integer, ipld);
}

#[test]
fn ipld_serializer_u32() {
    let integer = 345678u32;
    let ipld = Ipld::Integer(integer.into());
    assert_serialized(integer, ipld);
}

#[test]
fn ipld_serializer_u64() {
    let integer = 34567890123u64;
    let ipld = Ipld::Integer(integer.into());
    assert_serialized(integer, ipld);
}

#[test]
fn ipld_serializer_i8() {
    let integer = -23i8;
    let ipld = Ipld::Integer(integer.into());
    assert_serialized(integer, ipld);
}

#[test]
fn ipld_serializer_i16() {
    let integer = 2345i16;
    let ipld = Ipld::Integer(integer.into());
    assert_serialized(integer, ipld);
}

#[test]
fn ipld_serializer_i32() {
    let integer = 234567i32;
    let ipld = Ipld::Integer(integer.into());
    assert_serialized(integer, ipld);
}

#[test]
fn ipld_serializer_i64() {
    let integer = 2345678901i64;
    let ipld = Ipld::Integer(integer.into());
    assert_serialized(integer, ipld);
}

#[test]
fn ipld_serializer_i128() {
    let integer = 34567890123467890123i128;
    let ipld = Ipld::Integer(integer);
    assert_serialized(integer, ipld);
}

#[test]
fn ipld_serializer_f32() {
    let float = 7.3f32;
    let ipld = Ipld::Float(float.into());
    assert_serialized(float, ipld);
}

#[test]
fn ipld_serializer_f64() {
    let float = 427.8f64;
    let ipld = Ipld::Float(float);
    assert_serialized(float, ipld);
}

#[test]
fn ipld_serializer_char() {
    let char = 'x';
    let ipld = Ipld::String(char.to_string());
    assert_serialized(char, ipld);
}

#[test]
fn ipld_serializer_str() {
    let str: &str = "hello";
    let ipld = Ipld::String(str.to_string());
    assert_serialized(str, ipld);
}

#[test]
fn ipld_serializer_bytes() {
    let bytes = vec![0x68, 0x65, 0x6c, 0x6c, 0x6f];
    let ipld = Ipld::Bytes(bytes.clone());
    assert_serialized(ByteBuf::from(bytes), ipld);
}

#[test]
fn ipld_serializer_list() {
    let list = vec![0x68, 0x65, 0x6c, 0x6c, 0x6f];
    let ipld = Ipld::List(vec![
        Ipld::Integer(0x68),
        Ipld::Integer(0x65),
        Ipld::Integer(0x6c),
        Ipld::Integer(0x6c),
        Ipld::Integer(0x6f),
    ]);
    assert_serialized(list, ipld);
}

#[test]
fn ipld_serializer_tuple() {
    let tuple = (true, "hello".to_string());
    let ipld = Ipld::List(vec![Ipld::Bool(tuple.0), Ipld::String(tuple.1.clone())]);
    assert_serialized(tuple, ipld);
}

#[test]
fn ipld_serializer_tuple_struct() {
    #[derive(Clone, Debug, Serialize, PartialEq)]
    struct TupleStruct(u8, bool);

    let tuple_struct = TupleStruct(82, true);
    let ipld = Ipld::List(vec![Ipld::Integer(82), Ipld::Bool(true)]);
    assert_serialized(tuple_struct, ipld);
}

#[test]
fn ipld_serializer_map() {
    let map = BTreeMap::from([("hello".to_string(), true), ("world!".to_string(), false)]);
    let ipld = Ipld::Map(BTreeMap::from([
        ("hello".to_string(), Ipld::Bool(true)),
        ("world!".to_string(), Ipld::Bool(false)),
    ]));
    assert_serialized(map, ipld);
}

/// A CID is deserialized through a newtype struct.
#[test]
fn ipld_serializer_cid() {
    let cid = Cid::try_from("bafkreie74tgmnxqwojhtumgh5dzfj46gi4mynlfr7dmm7duwzyvnpw7h7m").unwrap();
    let ipld = Ipld::Link(cid);
    assert_serialized(cid, ipld);
}

#[test]
fn ipld_serializer_newtype_struct() {
    #[derive(Clone, Debug, Serialize, PartialEq)]
    struct Wrapped(u8);

    let newtype_struct = Wrapped(3);
    let ipld = Ipld::Integer(3);
    assert_serialized(newtype_struct, ipld);
}

/// An additional test, just to make sure that wrapped CIDs also work.
#[test]
fn ipld_serializer_newtype_struct_cid() {
    #[derive(Clone, Debug, Serialize, PartialEq)]
    struct Wrapped(Cid);

    let cid = Cid::try_from("bafkreie74tgmnxqwojhtumgh5dzfj46gi4mynlfr7dmm7duwzyvnpw7h7m").unwrap();
    let newtype_struct = Wrapped(cid);
    let ipld = Ipld::Link(cid);
    assert_serialized(newtype_struct, ipld);
}

#[test]
fn ipld_serializer_option() {
    let option_some: Option<u8> = Some(58u8);
    let option_none: Option<u8> = None;
    let ipld_some = Ipld::Integer(58);
    let ipld_none = Ipld::Null;
    assert_serialized(option_some, ipld_some);
    assert_serialized(option_none, ipld_none);
}

#[test]
fn ipld_serializer_enum() {
    #[derive(Clone, Debug, Serialize, PartialEq)]
    enum MyEnum {
        One,
        Two(u8),
        Three { value: bool },
    }

    let enum_one = MyEnum::One;
    let ipld_one = Ipld::String("One".into());
    assert_serialized(enum_one, ipld_one);

    let enum_two = MyEnum::Two(4);
    let ipld_two = Ipld::Map(BTreeMap::from([("Two".into(), Ipld::Integer(4))]));
    assert_serialized(enum_two, ipld_two);

    let enum_three = MyEnum::Three { value: true };
    let ipld_three = Ipld::Map(BTreeMap::from([(
        "Three".into(),
        Ipld::Map(BTreeMap::from([("value".into(), Ipld::Bool(true))])),
    )]));
    assert_serialized(enum_three, ipld_three);
}

#[test]
fn ipld_serializer_struct() {
    #[derive(Clone, Debug, Serialize, PartialEq)]
    struct MyStruct {
        hello: u8,
        world: bool,
    }

    let my_struct = MyStruct {
        hello: 91,
        world: false,
    };
    let ipld = Ipld::Map(BTreeMap::from([
        ("hello".into(), Ipld::Integer(my_struct.hello.into())),
        ("world".into(), Ipld::Bool(my_struct.world)),
    ]));
    assert_serialized(my_struct, ipld);
}
