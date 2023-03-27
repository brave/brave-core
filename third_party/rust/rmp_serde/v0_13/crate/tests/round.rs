extern crate serde;
#[macro_use]
extern crate serde_derive;
extern crate rmp;
extern crate rmp_serde as rmps;

use std::borrow::Cow;
use std::io::Cursor;

use serde::{Deserialize, Serialize};
use rmps::{Deserializer, Serializer};

#[test]
fn round_trip_option() {
    #[derive(Debug, PartialEq, Serialize, Deserialize)]
    struct Foo {
        v: Option<Vec<u8>>,
    }

    let expected = Foo { v: None };

    let mut buf = Vec::new();
    expected.serialize(&mut Serializer::new(&mut buf)).unwrap();

    let mut de = Deserializer::new(Cursor::new(&buf[..]));

    assert_eq!(expected, Deserialize::deserialize(&mut de).unwrap());
}

#[test]
fn round_trip_cow() {
    #[derive(Serialize, Deserialize, Debug, PartialEq)]
    struct Foo<'a> {
        v: Cow<'a, [u8]>,
    }

    let expected = Foo { v : Cow::Borrowed(&[]) };

    let mut buf = Vec::new();
    expected.serialize(&mut Serializer::new(&mut buf)).unwrap();

    let mut de = Deserializer::new(Cursor::new(&buf[..]));

    assert_eq!(expected, Deserialize::deserialize(&mut de).unwrap());
}

#[test]
fn round_trip_option_cow() {
    use std::borrow::Cow;
    use std::io::Cursor;
    use serde::Serialize;

    #[derive(Serialize, Deserialize, Debug, PartialEq)]
    struct Foo<'a> {
        v: Option<Cow<'a, [u8]>>,
    }

    let expected = Foo { v : None };

    let mut buf = Vec::new();
    expected.serialize(&mut Serializer::new(&mut buf)).unwrap();

    let mut de = Deserializer::new(Cursor::new(&buf[..]));

    assert_eq!(expected, Deserialize::deserialize(&mut de).unwrap());
}

#[test]
fn round_enum_with_nested_struct() {
    use serde::Serialize;

    #[derive(Serialize, Deserialize, Debug, PartialEq)]
    struct Newtype(String);

    #[derive(Serialize, Deserialize, Debug, PartialEq)]
    enum Enum {
        A(Newtype),
        B,
    }

    let expected = Enum::A(Newtype("le message".into()));
    let mut buf = Vec::new();
    expected.serialize(&mut Serializer::new(&mut buf)).unwrap();

    let mut de = Deserializer::new(&buf[..]);

    assert_eq!(expected, Deserialize::deserialize(&mut de).unwrap());
}

// Checks whether deserialization and serialization can both work with structs as maps
#[test]
fn round_struct_as_map() {
    use rmps::to_vec_named;
    use rmps::decode::from_slice;

    #[derive(Debug, Serialize, Deserialize, PartialEq, Eq)]
    struct Dog1 {
        name: String,
        age: u16,
    }
    #[derive(Debug, Serialize, Deserialize, PartialEq, Eq)]
    struct Dog2 {
        age: u16,
        name: String,
    }

    let dog1 = Dog1 {
        name: "Frankie".into(),
        age: 42,
    };

    let serialized: Vec<u8> = to_vec_named(&dog1).unwrap();
    let deserialized: Dog2 = from_slice(&serialized).unwrap();

    let check = Dog1 {
        age: deserialized.age,
        name: deserialized.name,
    };

    assert_eq!(dog1, check);
}
