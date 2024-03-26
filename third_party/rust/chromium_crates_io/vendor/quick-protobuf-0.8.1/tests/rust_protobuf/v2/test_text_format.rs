use super::test_text_format_pb::*;

fn t<F: FnMut(&mut TestTypes)>(_: &str, mut setter: F) {
    let mut m = TestTypes::default();
    setter(&mut m);
    //     validates that it compiles properly only
    //     assert_eq!(&*print_to_string(&m), expected);
}

#[test]
fn test_singular() {
    t("int32_singular: 99", |m| m.int32_singular = Some(99));
    t("double_singular: 99", |m| m.double_singular = Some(99.0));
    t("float_singular: 99", |m| m.float_singular = Some(99.0));
    t("int32_singular: 99", |m| m.int32_singular = Some(99));
    t("int64_singular: 99", |m| m.int64_singular = Some(99));
    t("uint32_singular: 99", |m| m.uint32_singular = Some(99));
    t("uint64_singular: 99", |m| m.uint64_singular = Some(99));
    t("sint32_singular: 99", |m| m.sint32_singular = Some(99));
    t("sint64_singular: 99", |m| m.sint64_singular = Some(99));
    t("fixed32_singular: 99", |m| m.fixed32_singular = Some(99));
    t("fixed64_singular: 99", |m| m.fixed64_singular = Some(99));
    t("sfixed32_singular: 99", |m| m.sfixed32_singular = Some(99));
    t("sfixed64_singular: 99", |m| m.sfixed64_singular = Some(99));
    t("bool_singular: false", |m| m.bool_singular = Some(false));
    t("string_singular: \"abc\"", |m| {
        m.string_singular = Some("abc".into())
    });
    t("bytes_singular: \"def\"", |m| {
        m.bytes_singular = Some(b"def".to_vec().into())
    });
    t("test_enum_singular: DARK", |m| {
        m.test_enum_singular = Some(TestEnum::DARK)
    });
    t("test_message_singular {}", |m| {
        m.test_message_singular = Some(TestMessage::default());
    });
}

#[test]
fn test_repeated_one() {
    t("int32_repeated: 99", |m| m.int32_repeated.push(99));
    t("double_repeated: 99", |m| m.double_repeated.push(99.0));
    t("float_repeated: 99", |m| m.float_repeated.push(99.0));
    t("int32_repeated: 99", |m| m.int32_repeated.push(99));
    t("int64_repeated: 99", |m| m.int64_repeated.push(99));
    t("uint32_repeated: 99", |m| m.uint32_repeated.push(99));
    t("uint64_repeated: 99", |m| m.uint64_repeated.push(99));
    t("sint32_repeated: 99", |m| m.sint32_repeated.push(99));
    t("sint64_repeated: 99", |m| m.sint64_repeated.push(99));
    t("fixed32_repeated: 99", |m| m.fixed32_repeated.push(99));
    t("fixed64_repeated: 99", |m| m.fixed64_repeated.push(99));
    t("sfixed32_repeated: 99", |m| m.sfixed32_repeated.push(99));
    t("sfixed64_repeated: 99", |m| m.sfixed64_repeated.push(99));
    t("bool_repeated: false", |m| m.bool_repeated.push(false));
    t("string_repeated: \"abc\"", |m| {
        m.string_repeated.push("abc".into())
    });
    t("bytes_repeated: \"def\"", |m| {
        m.bytes_repeated.push(b"def".to_vec().into())
    });
    t("test_enum_repeated: DARK", |m| {
        m.test_enum_repeated.push(TestEnum::DARK)
    });
    t("test_message_repeated {}", |m| {
        m.test_message_repeated.push(TestMessage::default());
    });
}

#[test]
fn test_repeated_multiple() {
    t(
        "uint32_singular: 30 int32_repeated: 10 int32_repeated: -20",
        |m| {
            m.uint32_singular = Some(30);
            m.int32_repeated.push(10);
            m.int32_repeated.push(-20);
        },
    );
}

#[test]
fn test_complex_message() {
    t("test_message_singular {value: 30}", |m| {
        m.test_message_singular = Some(TestMessage { value: Some(30) })
    });
}

// #[test]
// fn test_string_escaped() {
//     let mut m = TestTypes::new();
//     m.set_string_singular("quote\"newline\nbackslash\\del\x7f".to_string());
//     assert_eq!("string_singular: \
//                \"quote\\\"newline\\nbackslash\\\\del\\177\"", &*format!("{:?}", m));
// }
//
// #[test]
// fn test_pretty() {
//     let mut tm = TestMessage::new();
//     tm.set_value(23);
//     let mut m = TestTypes::new();
//     m.set_test_message_singular(tm);
//     m.set_string_singular("abc".to_string());
//     m.mut_string_repeated().push("def".to_string());
//     m.mut_string_repeated().push("ghi".to_string());
//     assert_eq!("string_singular: \"abc\"\ntest_message_singular \
//                {\n  value: 23\n}\nstring_repeated: \"def\"\n\
//                string_repeated: \"ghi\"\n", &*format!("{:#?}", m));
// }
//
// #[test]
// fn test_rust_identifier() {
//     let mut m = TestTextFormatRustIdentifier::new();
//     m.set_field_const(true);
//     assert_eq!("const: true", &*format!("{:?}", m));
// }
