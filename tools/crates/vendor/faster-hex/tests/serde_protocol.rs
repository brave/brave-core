#![cfg(feature = "serde")]

use serde::de::{value::Error, Error as _, Visitor};
use serde::Deserializer;
use std::cell::Cell;
use std::collections::VecDeque;

#[test]
fn serialization_reads_the_input_view_once() {
    struct ChangingBytes(Cell<usize>);

    impl AsRef<[u8]> for ChangingBytes {
        fn as_ref(&self) -> &[u8] {
            let call = self.0.get();
            self.0.set(call + 1);
            if call == 0 {
                &[0xab]
            } else {
                &[0xcd, 0xef]
            }
        }
    }

    for optional in [false, true] {
        let bytes = ChangingBytes(Cell::new(0));
        let mut output = Vec::new();
        let mut serializer = serde_json::Serializer::new(&mut output);
        if optional {
            faster_hex::option_withpfx_uppercase::serialize(&Some(&bytes), &mut serializer)
                .unwrap();
        } else {
            faster_hex::withpfx_uppercase::serialize(&bytes, &mut serializer).unwrap();
        }
        assert_eq!(bytes.0.get(), 1);
        assert_eq!(output, br#""0xAB""#);
    }
}

#[test]
fn optional_hex_retains_the_binary_format_option_tag() {
    #[derive(Debug, PartialEq, serde::Serialize, serde::Deserialize)]
    struct Data {
        #[serde(with = "faster_hex::option_withpfx_ignorecase")]
        prefixed: Option<Vec<u8>>,
        #[serde(with = "faster_hex::option_nopfx_ignorecase")]
        bare: Option<Vec<u8>>,
    }

    #[derive(Debug, serde::Serialize, serde::Deserialize)]
    struct Text {
        prefixed: Option<String>,
        bare: Option<String>,
    }

    for bytes in [None, Some(vec![]), Some(vec![0xab, 0xcd])] {
        let text = bytes.as_ref().map(hex::encode);
        let expected = Text {
            prefixed: text.as_ref().map(|hex| format!("0x{hex}")),
            bare: text,
        };
        let input = Data {
            prefixed: bytes.clone(),
            bare: bytes,
        };
        let mut actual_storage = [0; 64];
        let actual = postcard::to_slice(&input, &mut actual_storage).unwrap();
        let mut expected_storage = [0; 64];
        let expected_bytes = postcard::to_slice(&expected, &mut expected_storage).unwrap();
        assert_eq!(actual, expected_bytes);
        assert_eq!(postcard::from_bytes::<Data>(actual).unwrap(), input);
        assert_eq!(
            serde_json::to_string(&input).unwrap(),
            serde_json::to_string(&expected).unwrap()
        );
    }
}

macro_rules! array_policy_test {
    ($name:ident, $required:literal, $optional:literal, $prefix:literal, $upper:expr, $strict:expr) => {
        #[test]
        fn $name() {
            #[derive(Debug, PartialEq, serde::Serialize, serde::Deserialize)]
            struct Data<const N: usize> {
                #[serde(with = $required)]
                bytes: [u8; N],
                #[serde(with = $optional)]
                optional: Option<[u8; N]>,
            }

            #[derive(serde::Serialize)]
            struct Text<'a> {
                bytes: &'a str,
                optional: Option<&'a str>,
            }

            fn roundtrip<const N: usize>(bytes: [u8; N]) {
                let hex = if $upper {
                    hex::encode_upper(bytes)
                } else {
                    hex::encode(bytes)
                };
                let text = format!("{}{}", $prefix, hex);
                for optional in [None, Some(bytes)] {
                    let input = Data { bytes, optional };
                    let expected = Text {
                        bytes: &text,
                        optional: optional.map(|_| text.as_str()),
                    };
                    let json = serde_json::to_string(&input).unwrap();
                    assert_eq!(json, serde_json::to_string(&expected).unwrap());
                    assert_eq!(serde_json::from_str::<Data<N>>(&json).unwrap(), input);
                    assert_eq!(
                        serde_json::from_reader::<_, Data<N>>(json.as_bytes()).unwrap(),
                        input
                    );
                    let mut actual_storage = [0; 600];
                    let actual = postcard::to_slice(&input, &mut actual_storage).unwrap();
                    let mut expected_storage = [0; 600];
                    let expected_bytes =
                        postcard::to_slice(&expected, &mut expected_storage).unwrap();
                    assert_eq!(actual, expected_bytes);
                    assert_eq!(postcard::from_bytes::<Data<N>>(actual).unwrap(), input);
                }
            }

            roundtrip([]);
            roundtrip([0xab]);
            roundtrip([0xcd; 32]);
            roundtrip([0x01; 64]);
            roundtrip(core::array::from_fn::<_, 129, _>(|i| (i * 37) as u8));

            for (hex, actual) in [("", 0), ("abcd", 2)] {
                for json in [
                    format!(r#"{{"bytes":"{}{hex}","optional":null}}"#, $prefix),
                    format!(
                        r#"{{"bytes":"{}00","optional":"{}{hex}"}}"#,
                        $prefix, $prefix
                    ),
                ] {
                    let error = serde_json::from_str::<Data<1>>(&json).unwrap_err();
                    assert!(error
                        .to_string()
                        .contains(&format!("expected 1 decoded bytes, got {actual}")));
                }
            }
            let json = format!(r#"{{"bytes":"{}Ab","optional":null}}"#, $prefix);
            let mixed = serde_json::from_str::<Data<1>>(&json);
            if $strict {
                let diagnostic = if $upper {
                    "invalid hex byte 0x62 at index 1"
                } else {
                    "invalid hex byte 0x41 at index 0"
                };
                assert!(mixed.unwrap_err().to_string().contains(diagnostic));
            } else {
                assert_eq!(mixed.unwrap().bytes, [0xab]);
            }
        }
    };
}

array_policy_test!(
    array_default,
    "faster_hex::array",
    "faster_hex::option_withpfx_ignorecase::array",
    "0x",
    false,
    false
);
array_policy_test!(
    array_no_prefix,
    "faster_hex::nopfx_ignorecase::array",
    "faster_hex::option_nopfx_ignorecase::array",
    "",
    false,
    false
);
array_policy_test!(
    array_lower,
    "faster_hex::withpfx_lowercase::array",
    "faster_hex::option_withpfx_lowercase::array",
    "0x",
    false,
    true
);
array_policy_test!(
    array_upper,
    "faster_hex::withpfx_uppercase::array",
    "faster_hex::option_withpfx_uppercase::array",
    "0x",
    true,
    true
);
array_policy_test!(
    array_lower_no_prefix,
    "faster_hex::nopfx_lowercase::array",
    "faster_hex::option_nopfx_lowercase::array",
    "",
    false,
    true
);
array_policy_test!(
    array_upper_no_prefix,
    "faster_hex::nopfx_uppercase::array",
    "faster_hex::option_nopfx_uppercase::array",
    "",
    true,
    true
);

enum Token<'a> {
    Str(&'a str),
    BorrowedStr(&'a str),
    String(String),
    Bytes(&'a [u8]),
    BorrowedBytes(&'a [u8]),
    ByteBuf(Vec<u8>),
    Bool,
}

// A format that honors deserialize_string, but deliberately rejects any other
// hint. These tokens were all accepted by Serde's original String visitor.
struct StringFormat<'a> {
    token: Token<'a>,
    calls: &'a Cell<usize>,
}

impl<'de> Deserializer<'de> for StringFormat<'de> {
    type Error = Error;

    fn deserialize_any<V: Visitor<'de>>(self, _: V) -> Result<V::Value, Error> {
        Err(Error::custom("unexpected deserialize_any hint"))
    }

    fn deserialize_string<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        self.calls.set(self.calls.get() + 1);
        match self.token {
            Token::Str(text) => visitor.visit_str(text),
            Token::BorrowedStr(text) => visitor.visit_borrowed_str(text),
            Token::String(text) => visitor.visit_string(text),
            Token::Bytes(bytes) => visitor.visit_bytes(bytes),
            Token::BorrowedBytes(bytes) => visitor.visit_borrowed_bytes(bytes),
            Token::ByteBuf(bytes) => visitor.visit_byte_buf(bytes),
            Token::Bool => visitor.visit_bool(true),
        }
    }

    serde::forward_to_deserialize_any! {
        bool i8 i16 i32 i64 u8 u16 u32 u64 f32 f64 char str bytes byte_buf
        option unit unit_struct newtype_struct seq tuple tuple_struct map struct
        enum identifier ignored_any
    }
}

struct OptionalFormat<'a>(Option<StringFormat<'a>>, bool);

impl<'de> Deserializer<'de> for OptionalFormat<'de> {
    type Error = Error;

    fn deserialize_any<V: Visitor<'de>>(self, _: V) -> Result<V::Value, Error> {
        Err(Error::custom("unexpected deserialize_any hint"))
    }

    fn deserialize_option<V: Visitor<'de>>(self, visitor: V) -> Result<V::Value, Error> {
        match self.0 {
            Some(format) => visitor.visit_some(format),
            None if self.1 => visitor.visit_unit(),
            None => visitor.visit_none(),
        }
    }

    serde::forward_to_deserialize_any! {
        bool i8 i16 i32 i64 u8 u16 u32 u64 f32 f64 char str string bytes byte_buf
        unit unit_struct newtype_struct seq tuple tuple_struct map struct enum
        identifier ignored_any
    }
}

fn tokens(text: &str) -> [Token<'_>; 6] {
    [
        Token::Str(text),
        Token::BorrowedStr(text),
        Token::String(text.into()),
        Token::Bytes(text.as_bytes()),
        Token::BorrowedBytes(text.as_bytes()),
        Token::ByteBuf(text.as_bytes().into()),
    ]
}

#[test]
fn string_hint_and_all_text_ownership_modes_remain_supported() {
    for text in ["0x", "0x0123aBcDeF"] {
        let expected = hex::decode(&text[2..]).unwrap();
        for token in tokens(text) {
            let calls = Cell::new(0);
            let actual: VecDeque<u8> = faster_hex::deserialize(StringFormat {
                token,
                calls: &calls,
            })
            .unwrap();
            assert_eq!(actual.into_iter().collect::<Vec<_>>(), expected);
            assert_eq!(calls.get(), 1);
        }
        for token in tokens(text) {
            let calls = Cell::new(0);
            let actual: Option<VecDeque<u8>> =
                faster_hex::option_withpfx_ignorecase::deserialize(OptionalFormat(
                    Some(StringFormat {
                        token,
                        calls: &calls,
                    }),
                    false,
                ))
                .unwrap();
            assert_eq!(actual.unwrap().into_iter().collect::<Vec<_>>(), expected);
            assert_eq!(calls.get(), 1);
        }
    }
    for unit in [false, true] {
        let actual: Option<Vec<u8>> =
            faster_hex::option_withpfx_ignorecase::deserialize(OptionalFormat(None, unit)).unwrap();
        assert_eq!(actual, None);
    }
}

#[test]
fn array_decoding_keeps_the_text_hint_and_input_modes() {
    for token in tokens("0xaB") {
        let calls = Cell::new(0);
        let bytes: [u8; 1] = faster_hex::array::deserialize(StringFormat {
            token,
            calls: &calls,
        })
        .unwrap();
        assert_eq!(bytes, [0xab]);
        assert_eq!(calls.get(), 1);
    }
    for token in tokens("0xaB") {
        let calls = Cell::new(0);
        let bytes: Option<[u8; 1]> =
            faster_hex::option_withpfx_ignorecase::array::deserialize(OptionalFormat(
                Some(StringFormat {
                    token,
                    calls: &calls,
                }),
                false,
            ))
            .unwrap();
        assert_eq!(bytes, Some([0xab]));
        assert_eq!(calls.get(), 1);
    }
    for unit in [false, true] {
        let bytes: Option<[u8; 1]> =
            faster_hex::option_withpfx_ignorecase::array::deserialize(OptionalFormat(None, unit))
                .unwrap();
        assert_eq!(bytes, None);
    }
    for (text, expected) in [
        ("0Xg", "invalid prefix"),
        ("0xg", "invalid length"),
        ("0xgg00", "expected 1 decoded bytes, got 2"),
        ("0xgg", "invalid hex byte 0x67 at index 0"),
        ("0xé", "invalid hex byte 0xc3 at index 0"),
    ] {
        let calls = Cell::new(0);
        let result: Result<[u8; 1], _> = faster_hex::array::deserialize(StringFormat {
            token: Token::BorrowedStr(text),
            calls: &calls,
        });
        assert_eq!(result.unwrap_err().to_string(), expected);
    }
}

#[test]
fn errors_preserve_prefix_length_character_and_utf8_precedence() {
    for (text, expected) in [
        ("0Xg", "invalid prefix"),
        ("0xg", "invalid length"),
        ("0xgg", "invalid hex byte 0x67 at index 0"),
        ("0xé", "invalid hex byte 0xc3 at index 0"),
    ] {
        for token in tokens(text) {
            let calls = Cell::new(0);
            let result: Result<Vec<u8>, _> = faster_hex::deserialize(StringFormat {
                token,
                calls: &calls,
            });
            assert_eq!(result.unwrap_err().to_string(), expected);
        }
    }
    for token in [
        Token::Bytes(&[0xff]),
        Token::BorrowedBytes(&[0xff]),
        Token::ByteBuf(vec![0xff]),
    ] {
        let calls = Cell::new(0);
        let result: Result<Vec<u8>, _> = faster_hex::deserialize(StringFormat {
            token,
            calls: &calls,
        });
        assert_eq!(
            result.unwrap_err().to_string(),
            "invalid value: byte array, expected a string"
        );
    }
    let calls = Cell::new(0);
    let result: Result<Vec<u8>, _> = faster_hex::deserialize(StringFormat {
        token: Token::Bool,
        calls: &calls,
    });
    assert_eq!(
        result.unwrap_err().to_string(),
        "invalid type: boolean `true`, expected a string"
    );
}

#[test]
fn json_streams_and_escaped_strings_match_slice_input() {
    #[derive(Debug, PartialEq, serde::Deserialize)]
    struct Data {
        #[serde(with = "faster_hex")]
        bytes: Vec<u8>,
        #[serde(with = "faster_hex::option_withpfx_ignorecase")]
        optional: Option<Vec<u8>>,
    }
    for json in [
        r#"{"bytes":"0xaB","optional":"0xCd"}"#,
        r#"{"bytes":"\u0030x\u0061B","optional":"0xC\u0064"}"#,
        r#"{"bytes":"0xaB","optional":null}"#,
    ] {
        let slice: Data = serde_json::from_slice(json.as_bytes()).unwrap();
        let stream: Data = serde_json::from_reader(json.as_bytes()).unwrap();
        assert_eq!(slice, stream);
        assert_eq!(slice.bytes, [0xab]);
    }
}

macro_rules! bounded_policy_test {
    ($name:ident, $serialize:literal, $deserialize:literal, $option_serialize:literal, $option_deserialize:literal, $prefix:literal, $upper:expr, $strict:expr) => {
        #[test]
        fn $name() {
            #[derive(Debug, PartialEq, serde::Serialize, serde::Deserialize)]
            struct Data {
                #[serde(serialize_with = $serialize, deserialize_with = $deserialize)]
                bytes: Vec<u8>,
                #[serde(serialize_with = $option_serialize, deserialize_with = $option_deserialize)]
                optional: Option<Vec<u8>>,
            }
            #[derive(serde::Serialize)]
            struct Text<'a> {
                bytes: &'a str,
                optional: Option<&'a str>,
            }
            for bytes in [vec![], vec![0xab], vec![0xab, 0xcd]] {
                let hex = if $upper {
                    hex::encode_upper(&bytes)
                } else {
                    hex::encode(&bytes)
                };
                let text = format!("{}{hex}", $prefix);
                for optional in [None, Some(bytes.clone())] {
                    let expected = Text {
                        bytes: &text,
                        optional: optional.as_ref().map(|_| text.as_str()),
                    };
                    let input = Data {
                        bytes: bytes.clone(),
                        optional,
                    };
                    let json = serde_json::to_string(&input).unwrap();
                    assert_eq!(json, serde_json::to_string(&expected).unwrap());
                    assert_eq!(serde_json::from_str::<Data>(&json).unwrap(), input);
                    assert_eq!(
                        serde_json::from_reader::<_, Data>(json.as_bytes()).unwrap(),
                        input
                    );
                    let escaped = json.replace('a', "\\u0061").replace('A', "\\u0041");
                    assert_eq!(serde_json::from_str::<Data>(&escaped).unwrap(), input);
                    let mut actual_storage = [0; 32];
                    let actual = postcard::to_slice(&input, &mut actual_storage).unwrap();
                    let mut expected_storage = [0; 32];
                    assert_eq!(
                        actual,
                        postcard::to_slice(&expected, &mut expected_storage).unwrap()
                    );
                    assert_eq!(postcard::from_bytes::<Data>(actual).unwrap(), input);
                }
            }
            // This adapter limits acceptance, not serialization. Both required
            // and optional over-limit fields still serialize as ordinary hex.
            for input in [
                Data {
                    bytes: vec![0; 3],
                    optional: None,
                },
                Data {
                    bytes: vec![],
                    optional: Some(vec![0; 3]),
                },
            ] {
                let json = serde_json::to_string(&input).unwrap();
                let error = serde_json::from_str::<Data>(&json).unwrap_err();
                assert!(error
                    .to_string()
                    .contains("expected at most 2 decoded bytes, got 3"));
                let mut storage = [0; 32];
                let binary = postcard::to_slice(&input, &mut storage).unwrap();
                assert!(postcard::from_bytes::<Data>(binary).is_err());
            }
            let mixed = format!(r#"{{"bytes":"{}Ab","optional":null}}"#, $prefix);
            let result = serde_json::from_str::<Data>(&mixed);
            if $strict {
                assert!(result.is_err());
            } else {
                assert_eq!(result.unwrap().bytes, [0xab]);
            }
        }
    };
}

bounded_policy_test!(
    bounded_default,
    "faster_hex::serialize",
    "faster_hex::deserialize_bounded::<2, _, _>",
    "faster_hex::option_withpfx_ignorecase::serialize",
    "faster_hex::option_withpfx_ignorecase::deserialize_bounded::<2, _, _>",
    "0x",
    false,
    false
);
bounded_policy_test!(
    bounded_no_prefix,
    "faster_hex::nopfx_ignorecase::serialize",
    "faster_hex::nopfx_ignorecase::deserialize_bounded::<2, _, _>",
    "faster_hex::option_nopfx_ignorecase::serialize",
    "faster_hex::option_nopfx_ignorecase::deserialize_bounded::<2, _, _>",
    "",
    false,
    false
);
bounded_policy_test!(
    bounded_lower,
    "faster_hex::withpfx_lowercase::serialize",
    "faster_hex::withpfx_lowercase::deserialize_bounded::<2, _, _>",
    "faster_hex::option_withpfx_lowercase::serialize",
    "faster_hex::option_withpfx_lowercase::deserialize_bounded::<2, _, _>",
    "0x",
    false,
    true
);
bounded_policy_test!(
    bounded_lower_no_prefix,
    "faster_hex::nopfx_lowercase::serialize",
    "faster_hex::nopfx_lowercase::deserialize_bounded::<2, _, _>",
    "faster_hex::option_nopfx_lowercase::serialize",
    "faster_hex::option_nopfx_lowercase::deserialize_bounded::<2, _, _>",
    "",
    false,
    true
);
bounded_policy_test!(
    bounded_upper,
    "faster_hex::withpfx_uppercase::serialize",
    "faster_hex::withpfx_uppercase::deserialize_bounded::<2, _, _>",
    "faster_hex::option_withpfx_uppercase::serialize",
    "faster_hex::option_withpfx_uppercase::deserialize_bounded::<2, _, _>",
    "0x",
    true,
    true
);
bounded_policy_test!(
    bounded_upper_no_prefix,
    "faster_hex::nopfx_uppercase::serialize",
    "faster_hex::nopfx_uppercase::deserialize_bounded::<2, _, _>",
    "faster_hex::option_nopfx_uppercase::serialize",
    "faster_hex::option_nopfx_uppercase::deserialize_bounded::<2, _, _>",
    "",
    true,
    true
);

#[test]
fn bounded_decoding_keeps_text_modes_and_optional_empty_values() {
    for text in ["0x", "0x0123aBcDeF"] {
        let expected = hex::decode(&text[2..]).unwrap();
        for token in tokens(text) {
            let calls = Cell::new(0);
            let result: VecDeque<u8> = faster_hex::deserialize_bounded::<5, _, _>(StringFormat {
                token,
                calls: &calls,
            })
            .unwrap();
            assert_eq!(result.into_iter().collect::<Vec<_>>(), expected);
            assert_eq!(calls.get(), 1);
        }
        for token in tokens(text) {
            let calls = Cell::new(0);
            let result: Option<VecDeque<u8>> =
                faster_hex::option_withpfx_ignorecase::deserialize_bounded::<5, _, _>(
                    OptionalFormat(
                        Some(StringFormat {
                            token,
                            calls: &calls,
                        }),
                        false,
                    ),
                )
                .unwrap();
            assert_eq!(result.unwrap().into_iter().collect::<Vec<_>>(), expected);
            assert_eq!(calls.get(), 1);
        }
    }
    for unit in [false, true] {
        let result: Option<Vec<u8>> =
            faster_hex::option_withpfx_ignorecase::deserialize_bounded::<0, _, _>(OptionalFormat(
                None, unit,
            ))
            .unwrap();
        assert_eq!(result, None);
    }
    let calls = Cell::new(0);
    let empty: Option<Vec<u8>> =
        faster_hex::option_withpfx_ignorecase::deserialize_bounded::<0, _, _>(OptionalFormat(
            Some(StringFormat {
                token: Token::BorrowedStr("0x"),
                calls: &calls,
            }),
            false,
        ))
        .unwrap();
    assert_eq!(empty, Some(vec![]));
    let unlimited: Vec<u8> =
        faster_hex::deserialize_bounded::<{ usize::MAX }, _, _>(StringFormat {
            token: Token::BorrowedStr("0xabcd"),
            calls: &calls,
        })
        .unwrap();
    assert_eq!(unlimited, [0xab, 0xcd]);
}

#[test]
fn bounded_errors_precede_collection_and_report_the_decoded_length() {
    use std::iter::FromIterator;
    use std::sync::atomic::{AtomicUsize, Ordering::Relaxed};
    static COLLECTIONS: AtomicUsize = AtomicUsize::new(0);
    #[derive(Debug, PartialEq)]
    struct Collected(Vec<u8>);
    impl FromIterator<u8> for Collected {
        fn from_iter<I: IntoIterator<Item = u8>>(iter: I) -> Self {
            COLLECTIONS.fetch_add(1, Relaxed);
            Self(iter.into_iter().collect())
        }
    }
    for (text, expected) in [
        ("0Xgg00", "invalid prefix"),
        ("0xgg0", "invalid length"),
        ("0xgg00", "expected at most 1 decoded bytes, got 2"),
        ("0xgg", "invalid hex byte 0x67 at index 0"),
        ("0xé", "invalid hex byte 0xc3 at index 0"),
    ] {
        for token in tokens(text) {
            let calls = Cell::new(0);
            let result: Result<Collected, _> =
                faster_hex::deserialize_bounded::<1, _, _>(StringFormat {
                    token,
                    calls: &calls,
                });
            assert_eq!(result.unwrap_err().to_string(), expected);
            assert_eq!(COLLECTIONS.load(Relaxed), 0);
        }
    }
    let calls = Cell::new(0);
    let result: Collected = faster_hex::deserialize_bounded::<1, _, _>(StringFormat {
        token: Token::BorrowedStr("0xab"),
        calls: &calls,
    })
    .unwrap();
    assert_eq!(result.0, [0xab]);
    assert_eq!(COLLECTIONS.load(Relaxed), 1);
    let result: Option<Collected> =
        faster_hex::option_withpfx_ignorecase::deserialize_bounded::<1, _, _>(OptionalFormat(
            Some(StringFormat {
                token: Token::BorrowedStr("0xcd"),
                calls: &calls,
            }),
            false,
        ))
        .unwrap();
    assert_eq!(result.unwrap().0, [0xcd]);
    assert_eq!(COLLECTIONS.load(Relaxed), 2);
}

#[test]
fn untagged_options_do_not_turn_limit_or_hex_errors_into_none() {
    #[derive(Debug, PartialEq, serde::Deserialize)]
    #[serde(untagged)]
    enum Envelope {
        Data {
            #[serde(
                deserialize_with = "faster_hex::option_withpfx_ignorecase::deserialize_bounded::<1, _, _>"
            )]
            bytes: Option<Vec<u8>>,
        },
    }
    for json in [r#"{"bytes":"0x0000"}"#, r#"{"bytes":"0xgg"}"#] {
        assert!(serde_json::from_str::<Envelope>(json).is_err());
    }
    assert_eq!(
        serde_json::from_str::<Envelope>(r#"{"bytes":"0xab"}"#).unwrap(),
        Envelope::Data {
            bytes: Some(vec![0xab])
        }
    );
    assert_eq!(
        serde_json::from_str::<Envelope>(r#"{"bytes":null}"#).unwrap(),
        Envelope::Data { bytes: None }
    );
}
