#![cfg(feature = "serde")]

extern crate alloc;

use alloc::collections::BTreeMap;
use core::convert::TryFrom;

use serde_test::{assert_de_tokens, Token};

use ipld_core::cid::{serde::CID_SERDE_PRIVATE_IDENTIFIER, Cid};
use ipld_core::ipld::Ipld;

#[test]
fn ipld_deserialize_null() {
    let ipld = Ipld::Null;
    assert_de_tokens(&ipld, &[Token::None]);
}

#[test]
fn ipld_deserialize_null_as_unit() {
    let ipld = Ipld::Null;
    assert_de_tokens(&ipld, &[Token::Unit]);
}

#[test]
fn ipld_deserialize_null_as_unit_struct() {
    let ipld = Ipld::Null;
    assert_de_tokens(&ipld, &[Token::UnitStruct { name: "foo" }]);
}

#[test]
fn ipld_deserialize_bool() {
    let bool = true;
    let ipld = Ipld::Bool(bool);
    assert_de_tokens(&ipld, &[Token::Bool(bool)]);
}

#[test]
fn ipld_deserialize_integer_u() {
    let integer = 32u8;
    let ipld = Ipld::Integer(integer.into());
    assert_de_tokens(&ipld, &[Token::U8(integer)]);
    assert_de_tokens(&ipld, &[Token::U16(integer.into())]);
    assert_de_tokens(&ipld, &[Token::U32(integer.into())]);
    assert_de_tokens(&ipld, &[Token::U64(integer.into())]);
}

#[test]
fn ipld_deserialize_integer_i() {
    let integer = -32i8;
    let ipld = Ipld::Integer(integer.into());
    assert_de_tokens(&ipld, &[Token::I8(integer)]);
    assert_de_tokens(&ipld, &[Token::I16(integer.into())]);
    assert_de_tokens(&ipld, &[Token::I32(integer.into())]);
    assert_de_tokens(&ipld, &[Token::I64(integer.into())]);
}

#[test]
fn ipld_deserialize_float() {
    let float = 32.41f32;
    let ipld = Ipld::Float(float.into());
    assert_de_tokens(&ipld, &[Token::F32(float)]);
    assert_de_tokens(&ipld, &[Token::F64(float.into())]);
}

#[test]
fn ipld_deserialize_string() {
    let string = "hello";
    let ipld = Ipld::String(string.into());
    assert_de_tokens(&ipld, &[Token::Str(string)]);
    assert_de_tokens(&ipld, &[Token::BorrowedStr(string)]);
    assert_de_tokens(&ipld, &[Token::String(string)]);
}

#[test]
fn ipld_deserialize_string_char() {
    let char = 'h';
    let ipld = Ipld::String(char.into());
    assert_de_tokens(&ipld, &[Token::Char(char)]);
}

#[test]
fn ipld_deserialize_bytes() {
    let bytes = vec![0x68, 0x65, 0x6c, 0x6c, 0x6f];
    let ipld = Ipld::Bytes(bytes);
    assert_de_tokens(&ipld, &[Token::Bytes(b"hello")]);
    assert_de_tokens(&ipld, &[Token::BorrowedBytes(b"hello")]);
    assert_de_tokens(&ipld, &[Token::ByteBuf(b"hello")]);
}

#[test]
fn ipld_deserialize_list() {
    let ipld = Ipld::List(vec![Ipld::Bool(false), Ipld::Float(22.7)]);
    assert_de_tokens(
        &ipld,
        &[
            Token::Seq { len: Some(2) },
            Token::Bool(false),
            Token::F64(22.7),
            Token::SeqEnd,
        ],
    );
}

#[test]
fn ipld_deserialize_map() {
    let ipld = Ipld::Map(BTreeMap::from([
        ("hello".to_string(), Ipld::Bool(true)),
        ("world!".to_string(), Ipld::Bool(false)),
    ]));
    assert_de_tokens(
        &ipld,
        &[
            Token::Map { len: Some(2) },
            Token::Str("hello"),
            Token::Bool(true),
            Token::Str("world!"),
            Token::Bool(false),
            Token::MapEnd,
        ],
    );
}

#[test]
fn ipld_deserialize_link() {
    let cid = Cid::try_from("bafkreie74tgmnxqwojhtumgh5dzfj46gi4mynlfr7dmm7duwzyvnpw7h7m").unwrap();
    let ipld = Ipld::Link(cid);
    assert_de_tokens(
        &ipld,
        &[
            Token::NewtypeStruct {
                name: CID_SERDE_PRIVATE_IDENTIFIER,
            },
            Token::Bytes(&[
                1, 85, 18, 32, 159, 228, 204, 198, 222, 22, 114, 79, 58, 48, 199, 232, 242, 84,
                243, 198, 71, 25, 134, 172, 177, 248, 216, 207, 142, 150, 206, 42, 215, 219, 231,
                251,
            ]),
        ],
    );
}

#[test]
#[should_panic]
fn ipld_deserialize_link_not_as_bytes() {
    let cid = Cid::try_from("bafkreie74tgmnxqwojhtumgh5dzfj46gi4mynlfr7dmm7duwzyvnpw7h7m").unwrap();
    let ipld = Ipld::Link(cid);
    assert_de_tokens(
        &ipld,
        &[Token::Bytes(&[
            1, 85, 18, 32, 159, 228, 204, 198, 222, 22, 114, 79, 58, 48, 199, 232, 242, 84, 243,
            198, 71, 25, 134, 172, 177, 248, 216, 207, 142, 150, 206, 42, 215, 219, 231, 251,
        ])],
    );
}
