use serde::{Deserialize, Serialize};

use serde_ipld_dagcbor::{from_slice, to_vec, DecodeError};

#[derive(Debug, Serialize, Deserialize, PartialEq, Eq)]
enum Enum {
    A,
    B,
}

#[derive(Debug, Serialize, Deserialize, PartialEq, Eq)]
struct EnumStruct {
    e: Enum,
}

#[test]
fn test_enum() {
    let enum_struct = EnumStruct { e: Enum::B };
    let raw = &to_vec(&enum_struct).unwrap();
    println!("raw enum {:?}", raw);
    let re: EnumStruct = from_slice(raw).unwrap();
    assert_eq!(enum_struct, re);
}

#[repr(u16)]
#[derive(Debug, Serialize, Deserialize, PartialEq, Eq)]
enum ReprEnum {
    A,
    B,
}

#[derive(Debug, Serialize, Deserialize, PartialEq, Eq)]
struct ReprEnumStruct {
    e: ReprEnum,
}

#[test]
fn test_repr_enum() {
    let repr_enum_struct = ReprEnumStruct { e: ReprEnum::B };
    let re: ReprEnumStruct = from_slice(&to_vec(&repr_enum_struct).unwrap()).unwrap();
    assert_eq!(repr_enum_struct, re);
}

#[derive(Debug, Serialize, Deserialize, PartialEq, Eq)]
enum DataEnum {
    A(u32),
    B(bool, u8),
    C { x: u8, y: String },
}

#[test]
fn test_data_enum() {
    let data_enum_a = DataEnum::A(4);
    let re_a: DataEnum = from_slice(&to_vec(&data_enum_a).unwrap()).unwrap();
    assert_eq!(data_enum_a, re_a);
    let data_enum_b = DataEnum::B(true, 42);
    let re_b: DataEnum = from_slice(&to_vec(&data_enum_b).unwrap()).unwrap();
    assert_eq!(data_enum_b, re_b);
    let data_enum_c = DataEnum::C {
        x: 3,
        y: "foo".to_owned(),
    };
    println!("{:?}", &to_vec(&data_enum_c).unwrap());
    let re_c: DataEnum = from_slice(&to_vec(&data_enum_c).unwrap()).unwrap();
    assert_eq!(data_enum_c, re_c);
}

#[test]
fn test_newtype_struct() {
    #[derive(Debug, Deserialize, Serialize, PartialEq, Eq)]
    pub struct Newtype(u8);
    assert_eq!(to_vec(&142u8).unwrap(), to_vec(&Newtype(142u8)).unwrap());
    assert_eq!(from_slice::<Newtype>(&[24, 142]).unwrap(), Newtype(142));
}

#[derive(Deserialize, PartialEq, Debug)]
enum Foo {
    #[serde(rename = "require")]
    Require,
}

#[test]
fn test_variable_length_array_error() {
    let slice = b"\x9F\x67\x72\x65\x71\x75\x69\x72\x65\xFF";
    let value: Result<Vec<Foo>, _> = from_slice(slice);
    assert!(matches!(value.unwrap_err(), DecodeError::IndefiniteSize));
}

#[derive(Serialize, Deserialize, PartialEq, Debug)]
enum Bar {
    Empty,
    Number(i32),
    Flag(String, bool),
    Point { x: i32, y: i32 },
}
