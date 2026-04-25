#![allow(
    clippy::derive_partial_eq_without_eq,
    clippy::let_underscore_untyped,
    clippy::uninlined_format_args
)]

use monostate::MustBe;
use serde::{Deserialize, Serialize};
use std::fmt::Debug;
use std::mem;

#[test]
fn test_serialize_deserialize() {
    #[derive(Deserialize, Serialize, Debug, PartialEq)]
    struct Struct {
        kind: MustBe!("Struct"),
    }

    let s = Struct {
        kind: MustBe!("Struct"),
    };
    let j = serde_json::to_string(&s).unwrap();
    assert_eq!(j, "{\"kind\":\"Struct\"}");

    let s2 = serde_json::from_str::<Struct>(&j).unwrap();
    assert_eq!(s, s2);

    let bad_j = "{\"kind\":\"unknown\"}";
    let err = serde_json::from_str::<Struct>(bad_j).unwrap_err();
    assert_eq!(
        err.to_string(),
        "invalid value: string \"unknown\", expected string \"Struct\" at line 1 column 17",
    );
}

#[test]
fn test_untagged_enum() {
    #[derive(Serialize, Deserialize)]
    #[serde(untagged)]
    enum ApiResponse {
        Success {
            success: MustBe!(true),
        },
        Error {
            success: MustBe!(false),
            message: String,
        },
    }

    let success = "{\"success\":true}";
    let response: ApiResponse = serde_json::from_str(success).unwrap();
    match response {
        ApiResponse::Success {
            success: MustBe!(true),
        } => {}
        ApiResponse::Error { .. } => panic!(),
    }

    let error = "{\"success\":false,\"message\":\"...\"}";
    let response: ApiResponse = serde_json::from_str(error).unwrap();
    match response {
        ApiResponse::Error {
            success: MustBe!(false),
            ..
        } => {}
        ApiResponse::Success { .. } => panic!(),
    }
}

#[test]
fn test_debug() {
    #[track_caller]
    fn assert_debug(must_be: impl Debug, expected: &str) {
        assert_eq!(format!("{:?}", must_be), expected);
    }

    assert_debug(MustBe!('x'), "MustBe!('x')");
    assert_debug(MustBe!(1), "MustBe!(1)");
    assert_debug(MustBe!(-1), "MustBe!(-1)");
    assert_debug(MustBe!(1u8), "MustBe!(1u8)");
    assert_debug(MustBe!(1u16), "MustBe!(1u16)");
    assert_debug(MustBe!(1u32), "MustBe!(1u32)");
    assert_debug(MustBe!(1u64), "MustBe!(1u64)");
    assert_debug(MustBe!(1u128), "MustBe!(1u128)");
    assert_debug(MustBe!(1i8), "MustBe!(1i8)");
    assert_debug(MustBe!(1i16), "MustBe!(1i16)");
    assert_debug(MustBe!(1i32), "MustBe!(1i32)");
    assert_debug(MustBe!(1i64), "MustBe!(1i64)");
    assert_debug(MustBe!(1i128), "MustBe!(1i128)");
    assert_debug(MustBe!(true), "MustBe!(true)");
    assert_debug(MustBe!("string"), "MustBe!(\"string\")");
}

#[test]
fn test_cmp() {
    assert_eq!(MustBe!(4), MustBe!(4));
    assert_ne!(MustBe!(4), MustBe!(5));
    assert!(MustBe!(4) < MustBe!(5));
}

#[test]
fn test_long_string() {
    let _ = MustBe!("\
        Rust is blazingly fast and memory-efficient: with no runtime or garbage collector, it can power performance-critical services, run on embedded devices, and easily integrate with other languages. \
        Rust’s rich type system and ownership model guarantee memory-safety and thread-safety — enabling you to eliminate many classes of bugs at compile-time. \
        Rust has great documentation, a friendly compiler with useful error messages, and top-notch tooling — an integrated package manager and build tool, smart multi-editor support with auto-completion and type inspections, an auto-formatter, and more.\
    ");
}

#[test]
fn test_utf8() {
    let string = "$£€𐍈";
    let mut chars = string.chars();
    assert_eq!(chars.next().unwrap().len_utf8(), 1);
    assert_eq!(chars.next().unwrap().len_utf8(), 2);
    assert_eq!(chars.next().unwrap().len_utf8(), 3);
    assert_eq!(chars.next().unwrap().len_utf8(), 4);
    assert!(chars.next().is_none());

    let must_be = MustBe!("$£€𐍈");
    assert_eq!(
        serde_json::to_string(string).unwrap(),
        serde_json::to_string(&must_be).unwrap(),
    );
}

#[test]
fn test_layout() {
    let must_be = MustBe!("s");
    assert_eq!(0, mem::size_of_val(&must_be));
    assert_eq!(1, mem::align_of_val(&must_be));

    let must_be = MustBe!(1);
    assert_eq!(0, mem::size_of_val(&must_be));
    assert_eq!(1, mem::align_of_val(&must_be));
}

#[test]
fn test_autotraits() {
    fn assert_send(_: impl Send) {}
    fn assert_sync(_: impl Sync) {}

    assert_send(MustBe!(""));
    assert_sync(MustBe!(""));
    assert_send(MustBe!(true));
    assert_sync(MustBe!(true));
}
