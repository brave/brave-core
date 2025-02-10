use std::collections::BTreeMap;

use serde::Deserialize;
use serde::Deserializer;
use serde::Serialize;
use snapbox::assert_data_eq;
use snapbox::prelude::*;
use snapbox::str;
use toml::map::Map;
use toml::Table;
use toml::Value;

macro_rules! t {
    ($e:expr) => {
        match $e {
            Ok(t) => t,
            Err(e) => panic!("{} failed with {}", stringify!($e), e),
        }
    };
}

macro_rules! equivalent {
    ($literal:expr, $toml:expr,) => {{
        let toml = $toml;
        let literal = $literal;

        // Through a string equivalent
        println!("to_string");
        assert_data_eq!(
            t!(toml::to_string(&toml)),
            t!(toml::to_string(&literal)).raw()
        );
        println!("literal, from_str(toml)");
        assert_eq!(literal, t!(toml::from_str(&t!(toml::to_string(&toml)))));
        println!("toml, from_str(literal)");
        assert_eq!(toml, t!(toml::from_str(&t!(toml::to_string(&literal)))));

        // In/out of Value is equivalent
        println!("Table::try_from(literal)");
        assert_eq!(toml, t!(Table::try_from(literal.clone())));
        println!("Value::try_from(literal)");
        assert_eq!(
            Value::Table(toml.clone()),
            t!(Value::try_from(literal.clone()))
        );
        println!("toml.try_into()");
        assert_eq!(literal, t!(toml.clone().try_into()));
        println!("Value::Table(toml).try_into()");
        assert_eq!(literal, t!(Value::Table(toml.clone()).try_into()));
    }};
}

macro_rules! error {
    ($ty:ty, $toml:expr, $msg_parse:expr, $msg_decode:expr) => {{
        println!("attempting parsing");
        match toml::from_str::<$ty>(&$toml.to_string()) {
            Ok(_) => panic!("successful"),
            Err(e) => assert_data_eq!(e.to_string(), $msg_parse.raw()),
        }

        println!("attempting toml decoding");
        match $toml.try_into::<$ty>() {
            Ok(_) => panic!("successful"),
            Err(e) => assert_data_eq!(e.to_string(), $msg_decode.raw()),
        }
    }};
}

macro_rules! map( ($($k:ident: $v:expr),*) => ({
    let mut _m = Map::new();
    $(_m.insert(stringify!($k).to_owned(), t!(Value::try_from($v)));)*
    _m
}) );

#[test]
fn smoke() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a: isize,
    }

    equivalent!(Foo { a: 2 }, map! { a: Value::Integer(2) },);
}

#[test]
fn smoke_hyphen() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a_b: isize,
    }

    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo2 {
        #[serde(rename = "a-b")]
        a_b: isize,
    }

    equivalent! {
        Foo { a_b: 2 },
        map! { a_b: Value::Integer(2)},
    }

    let mut m = Map::new();
    m.insert("a-b".to_owned(), Value::Integer(2));
    equivalent! {
        Foo2 { a_b: 2 },
        m,
    }
}

#[test]
fn nested() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a: isize,
        b: Bar,
    }
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Bar {
        a: String,
    }

    equivalent! {
        Foo { a: 2, b: Bar { a: "test".to_owned() } },
        map! {
            a: Value::Integer(2),
            b: map! {
                a: Value::String("test".to_owned())
            }
        },
    }
}

#[test]
fn application_decode_error() {
    #[derive(PartialEq, Debug)]
    struct Range10(usize);
    impl<'de> Deserialize<'de> for Range10 {
        fn deserialize<D: Deserializer<'de>>(d: D) -> Result<Range10, D::Error> {
            let x: usize = Deserialize::deserialize(d)?;
            if x > 10 {
                Err(serde::de::Error::custom("more than 10"))
            } else {
                Ok(Range10(x))
            }
        }
    }
    let d_good = Value::Integer(5);
    let d_bad1 = Value::String("not an isize".to_owned());
    let d_bad2 = Value::Integer(11);

    assert_eq!(Range10(5), d_good.try_into().unwrap());

    let err1: Result<Range10, _> = d_bad1.try_into();
    assert!(err1.is_err());
    let err2: Result<Range10, _> = d_bad2.try_into();
    assert!(err2.is_err());
}

#[test]
fn array() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a: Vec<isize>,
    }

    equivalent! {
        Foo { a: vec![1, 2, 3, 4] },
        map! {
            a: Value::Array(vec![
                Value::Integer(1),
                Value::Integer(2),
                Value::Integer(3),
                Value::Integer(4)
            ])
        },
    };
}

#[test]
fn inner_structs_with_options() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a: Option<Box<Foo>>,
        b: Bar,
    }
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Bar {
        a: String,
        b: f64,
    }

    equivalent! {
        Foo {
            a: Some(Box::new(Foo {
                a: None,
                b: Bar { a: "foo".to_owned(), b: 4.5 },
            })),
            b: Bar { a: "bar".to_owned(), b: 1.0 },
        },
        map! {
            a: map! {
                b: map! {
                    a: Value::String("foo".to_owned()),
                    b: Value::Float(4.5)
                }
            },
            b: map! {
                a: Value::String("bar".to_owned()),
                b: Value::Float(1.0)
            }
        },
    }
}

#[test]
#[cfg(feature = "preserve_order")]
fn hashmap() {
    use std::collections::HashSet;

    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        set: HashSet<char>,
        map: BTreeMap<String, isize>,
    }

    equivalent! {
        Foo {
            set: {
                let mut s = HashSet::new();
                s.insert('a');
                s
            },
            map: {
                let mut m = BTreeMap::new();
                m.insert("bar".to_owned(), 4);
                m.insert("foo".to_owned(), 10);
                m
            }
        },
        map! {
            set: Value::Array(vec![Value::String("a".to_owned())]),
            map: map! {
                bar: Value::Integer(4),
                foo: Value::Integer(10)
            }
        },
    }
}

#[test]
fn table_array() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a: Vec<Bar>,
    }
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Bar {
        a: isize,
    }

    equivalent! {
        Foo { a: vec![Bar { a: 1 }, Bar { a: 2 }] },
        map! {
            a: Value::Array(vec![
                Value::Table(map!{ a: Value::Integer(1) }),
                Value::Table(map!{ a: Value::Integer(2) }),
            ])
        },
    }
}

#[test]
fn type_errors() {
    #[derive(Deserialize)]
    #[allow(dead_code)]
    struct Foo {
        bar: isize,
    }

    #[derive(Deserialize)]
    #[allow(dead_code)]
    struct Bar {
        foo: Foo,
    }

    error! {
        Foo,
        map! {
            bar: Value::String("a".to_owned())
        },
        str![[r#"
TOML parse error at line 1, column 7
  |
1 | bar = "a"
  |       ^^^
invalid type: string "a", expected isize

"#]],
        str![[r#"
invalid type: string "a", expected isize
in `bar`

"#]]
    }

    error! {
        Bar,
        map! {
            foo: map! {
                bar: Value::String("a".to_owned())
            }
        },
        str![[r#"
TOML parse error at line 2, column 7
  |
2 | bar = "a"
  |       ^^^
invalid type: string "a", expected isize

"#]],
        str![[r#"
invalid type: string "a", expected isize
in `foo.bar`

"#]]
    }
}

#[test]
fn missing_errors() {
    #[derive(Serialize, Deserialize, PartialEq, Debug)]
    struct Foo {
        bar: isize,
    }

    error! {
        Foo,
        map! { },
        str![[r#"
TOML parse error at line 1, column 1
  |
1 | 
  | ^
missing field `bar`

"#]],
        str![[r#"
missing field `bar`

"#]]
    }
}

#[test]
fn parse_enum() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a: E,
    }
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    #[serde(untagged)]
    enum E {
        Bar(isize),
        Baz(String),
        Last(Foo2),
    }
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo2 {
        test: String,
    }

    equivalent! {
        Foo { a: E::Bar(10) },
        map! { a: Value::Integer(10) },
    }

    equivalent! {
        Foo { a: E::Baz("foo".to_owned()) },
        map! { a: Value::String("foo".to_owned()) },
    }

    equivalent! {
        Foo { a: E::Last(Foo2 { test: "test".to_owned() }) },
        map! { a: map! { test: Value::String("test".to_owned()) } },
    }
}

#[test]
fn parse_enum_string() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a: Sort,
    }

    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    #[serde(rename_all = "lowercase")]
    enum Sort {
        Asc,
        Desc,
    }

    equivalent! {
        Foo { a: Sort::Desc },
        map! { a: Value::String("desc".to_owned()) },
    }
}

#[test]
fn parse_tuple_variant() {
    #[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
    struct Document {
        inner: Vec<Enum>,
    }

    #[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
    enum Enum {
        Int(i32, i32),
        String(String, String),
    }

    let input = Document {
        inner: vec![
            Enum::Int(1, 1),
            Enum::String("2".to_owned(), "2".to_owned()),
        ],
    };
    let raw = toml::to_string(&input).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
[[inner]]
Int = [1, 1]

[[inner]]
String = ["2", "2"]

"#]]
        .raw()
    );

    equivalent! {
        Document {
            inner: vec![
                Enum::Int(1, 1),
                Enum::String("2".to_owned(), "2".to_owned()),
            ],
        },
        map! {
            inner: vec![
                map! { Int: [1, 1] },
                map! { String: ["2".to_owned(), "2".to_owned()] },
            ]
        },
    }
}

#[test]
fn parse_struct_variant() {
    #[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
    struct Document {
        inner: Vec<Enum>,
    }

    #[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
    enum Enum {
        Int { first: i32, second: i32 },
        String { first: String, second: String },
    }

    let input = Document {
        inner: vec![
            Enum::Int {
                first: 1,
                second: 1,
            },
            Enum::String {
                first: "2".to_owned(),
                second: "2".to_owned(),
            },
        ],
    };
    let raw = toml::to_string(&input).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
[[inner]]

[inner.Int]
first = 1
second = 1

[[inner]]

[inner.String]
first = "2"
second = "2"

"#]]
        .raw()
    );

    equivalent! {
        Document {
            inner: vec![
                Enum::Int { first: 1, second: 1 },
                Enum::String { first: "2".to_owned(), second: "2".to_owned() },
            ],
        },
        map! {
            inner: vec![
                map! { Int: map! { first: 1, second: 1 } },
                map! { String: map! { first: "2".to_owned(), second: "2".to_owned() } },
            ]
        },
    }
}

#[test]
#[cfg(feature = "preserve_order")]
fn map_key_unit_variants() {
    #[derive(Serialize, Deserialize, PartialEq, Eq, Debug, Clone, PartialOrd, Ord)]
    enum Sort {
        #[serde(rename = "ascending")]
        Asc,
        Desc,
    }

    let mut map = BTreeMap::new();
    map.insert(Sort::Asc, 1);
    map.insert(Sort::Desc, 2);

    equivalent! {
        map,
        map! { ascending: Value::Integer(1), Desc: Value::Integer(2) },
    }
}

// #[test]
// fn unused_fields() {
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Foo { a: isize }
//
//     let v = Foo { a: 2 };
//     let mut d = Decoder::new(Table(map! {
//         a, Integer(2),
//         b, Integer(5)
//     }));
//     assert_eq!(v, t!(Deserialize::deserialize(&mut d)));
//
//     assert_eq!(d.toml, Some(Table(map! {
//         b, Integer(5)
//     })));
// }
//
// #[test]
// fn unused_fields2() {
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Foo { a: Bar }
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Bar { a: isize }
//
//     let v = Foo { a: Bar { a: 2 } };
//     let mut d = Decoder::new(Table(map! {
//         a, Table(map! {
//             a, Integer(2),
//             b, Integer(5)
//         })
//     }));
//     assert_eq!(v, t!(Deserialize::deserialize(&mut d)));
//
//     assert_eq!(d.toml, Some(Table(map! {
//         a, Table(map! {
//             b, Integer(5)
//         })
//     })));
// }
//
// #[test]
// fn unused_fields3() {
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Foo { a: Bar }
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Bar { a: isize }
//
//     let v = Foo { a: Bar { a: 2 } };
//     let mut d = Decoder::new(Table(map! {
//         a, Table(map! {
//             a, Integer(2)
//         })
//     }));
//     assert_eq!(v, t!(Deserialize::deserialize(&mut d)));
//
//     assert_eq!(d.toml, None);
// }
//
// #[test]
// fn unused_fields4() {
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Foo { a: BTreeMap<String, String> }
//
//     let v = Foo { a: map! { a, "foo".to_owned() } };
//     let mut d = Decoder::new(Table(map! {
//         a, Table(map! {
//             a, Value::String("foo".to_owned())
//         })
//     }));
//     assert_eq!(v, t!(Deserialize::deserialize(&mut d)));
//
//     assert_eq!(d.toml, None);
// }
//
// #[test]
// fn unused_fields5() {
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Foo { a: Vec<String> }
//
//     let v = Foo { a: vec!["a".to_owned()] };
//     let mut d = Decoder::new(Table(map! {
//         a, Array(vec![Value::String("a".to_owned())])
//     }));
//     assert_eq!(v, t!(Deserialize::deserialize(&mut d)));
//
//     assert_eq!(d.toml, None);
// }
//
// #[test]
// fn unused_fields6() {
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Foo { a: Option<Vec<String>> }
//
//     let v = Foo { a: Some(vec![]) };
//     let mut d = Decoder::new(Table(map! {
//         a, Array(vec![])
//     }));
//     assert_eq!(v, t!(Deserialize::deserialize(&mut d)));
//
//     assert_eq!(d.toml, None);
// }
//
// #[test]
// fn unused_fields7() {
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Foo { a: Vec<Bar> }
//     #[derive(Serialize, Deserialize, PartialEq, Debug)]
//     struct Bar { a: isize }
//
//     let v = Foo { a: vec![Bar { a: 1 }] };
//     let mut d = Decoder::new(Table(map! {
//         a, Array(vec![Table(map! {
//             a, Integer(1),
//             b, Integer(2)
//         })])
//     }));
//     assert_eq!(v, t!(Deserialize::deserialize(&mut d)));
//
//     assert_eq!(d.toml, Some(Table(map! {
//         a, Array(vec![Table(map! {
//             b, Integer(2)
//         })])
//     })));
// }

#[test]
fn empty_arrays() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a: Vec<Bar>,
    }
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Bar;

    equivalent! {
        Foo { a: vec![] },
        map! {a: Value::Array(Vec::new())},
    }
}

#[test]
fn empty_arrays2() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a: Option<Vec<Bar>>,
    }
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Bar;

    equivalent! {
        Foo { a: None },
        map! {},
    }

    equivalent! {
        Foo { a: Some(vec![]) },
        map! { a: Value::Array(vec![]) },
    }
}

#[test]
fn extra_keys() {
    #[derive(Serialize, Deserialize)]
    struct Foo {
        a: isize,
    }

    let toml = map! { a: Value::Integer(2), b: Value::Integer(2) };
    assert!(toml.clone().try_into::<Foo>().is_ok());
    assert!(toml::from_str::<Foo>(&toml.to_string()).is_ok());
}

#[test]
fn newtypes() {
    #[derive(Deserialize, Serialize, PartialEq, Debug, Clone)]
    struct A {
        b: B,
    }

    #[derive(Deserialize, Serialize, PartialEq, Debug, Clone)]
    struct B(u32);

    equivalent! {
        A { b: B(2) },
        map! { b: Value::Integer(2) },
    }
}

#[test]
fn newtypes2() {
    #[derive(Deserialize, Serialize, PartialEq, Debug, Clone)]
    struct A {
        b: B,
    }

    #[derive(Deserialize, Serialize, PartialEq, Debug, Clone)]
    struct B(Option<C>);

    #[derive(Deserialize, Serialize, PartialEq, Debug, Clone)]
    struct C {
        x: u32,
        y: u32,
        z: u32,
    }

    equivalent! {
        A { b: B(Some(C { x: 0, y: 1, z: 2 })) },
        map! {
            b: map! {
                x: Value::Integer(0),
                y: Value::Integer(1),
                z: Value::Integer(2)
            }
        },
    }
}

#[test]
fn newtype_variant() {
    #[derive(Copy, Clone, Debug, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
    struct Struct {
        field: Enum,
    }

    #[derive(Copy, Clone, Debug, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
    enum Enum {
        Variant(u8),
    }

    equivalent! {
        Struct { field: Enum::Variant(21) },
        map! {
            field: map! {
                Variant: Value::Integer(21)
            }
        },
    }
}

#[test]
fn newtype_key() {
    #[derive(PartialEq, Eq, PartialOrd, Ord, Hash, Debug, Clone, Serialize, Deserialize)]
    struct NewType(String);

    type CustomKeyMap = BTreeMap<NewType, u32>;

    equivalent! {
        [
            (NewType("x".to_owned()), 1),
            (NewType("y".to_owned()), 2),
        ].into_iter().collect::<CustomKeyMap>(),
        map! {
            x: Value::Integer(1),
            y: Value::Integer(2)
        },
    }
}

#[derive(Debug, Default, PartialEq, Serialize, Deserialize)]
struct CanBeEmpty {
    a: Option<String>,
    b: Option<String>,
}

#[test]
fn table_structs_empty() {
    let text = "[bar]\n\n[baz]\n\n[bazv]\na = \"foo\"\n\n[foo]\n";
    let value: BTreeMap<String, CanBeEmpty> = toml::from_str(text).unwrap();
    let mut expected: BTreeMap<String, CanBeEmpty> = BTreeMap::new();
    expected.insert("bar".to_owned(), CanBeEmpty::default());
    expected.insert("baz".to_owned(), CanBeEmpty::default());
    expected.insert(
        "bazv".to_owned(),
        CanBeEmpty {
            a: Some("foo".to_owned()),
            b: None,
        },
    );
    expected.insert("foo".to_owned(), CanBeEmpty::default());
    assert_eq!(value, expected);
    assert_data_eq!(toml::to_string(&value).unwrap(), text.raw());
}

#[test]
fn fixed_size_array() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Entity {
        pos: [i32; 2],
    }

    equivalent! {
        Entity { pos: [1, 2] },
        map! {
            pos: Value::Array(vec![
                Value::Integer(1),
                Value::Integer(2),
            ])
        },
    }
}

#[test]
fn homogeneous_tuple() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Collection {
        elems: (i64, i64, i64),
    }

    equivalent! {
        Collection { elems: (0, 1, 2) },
        map! {
            elems: Value::Array(vec![
                Value::Integer(0),
                Value::Integer(1),
                Value::Integer(2),
            ])
        },
    }
}

#[test]
fn homogeneous_tuple_struct() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Object(Vec<String>, Vec<String>, Vec<String>);

    equivalent! {
        map! {
            obj: Object(vec!["foo".to_owned()], vec![], vec!["bar".to_owned(), "baz".to_owned()])
        },
        map! {
            obj: Value::Array(vec![
                Value::Array(vec![
                    Value::String("foo".to_owned()),
                ]),
                Value::Array(vec![]),
                Value::Array(vec![
                    Value::String("bar".to_owned()),
                    Value::String("baz".to_owned()),
                ]),
            ])
        },
    }
}

#[test]
fn json_interoperability() {
    #[derive(Serialize, Deserialize)]
    struct Foo {
        any: Value,
    }

    let _foo: Foo = serde_json::from_str(
        r#"
        {"any":1}
    "#,
    )
    .unwrap();
}

#[test]
fn error_includes_key() {
    #[derive(Debug, Serialize, Deserialize)]
    struct Package {
        name: String,
        version: String,
        authors: Vec<String>,
        profile: Profile,
    }

    #[derive(Debug, Serialize, Deserialize)]
    struct Profile {
        dev: Dev,
    }

    #[derive(Debug, Serialize, Deserialize)]
    struct Dev {
        debug: U32OrBool,
    }

    #[derive(Clone, Debug, Deserialize, Serialize, Eq, PartialEq)]
    #[serde(untagged, expecting = "expected a boolean or an integer")]
    pub(crate) enum U32OrBool {
        U32(u32),
        Bool(bool),
    }

    let res: Result<Package, _> = toml::from_str(
        r#"
[package]
name = "foo"
version = "0.0.0"
authors = []

[profile.dev]
debug = 'a'
"#,
    );
    let err = res.unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 8, column 9
  |
8 | debug = 'a'
  |         ^^^
expected a boolean or an integer

"#]]
    );

    let res: Result<Package, _> = toml::from_str(
        r#"
[package]
name = "foo"
version = "0.0.0"
authors = []

[profile]
dev = { debug = 'a' }
"#,
    );
    let err = res.unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str![[r#"
TOML parse error at line 8, column 17
  |
8 | dev = { debug = 'a' }
  |                 ^^^
expected a boolean or an integer

"#]]
    );
}

#[test]
fn newline_key_value() {
    #[derive(Debug, Serialize, Deserialize)]
    struct Package {
        name: String,
    }

    let package = Package {
        name: "foo".to_owned(),
    };
    let raw = toml::to_string_pretty(&package).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
name = "foo"

"#]]
    );
}

#[test]
fn newline_table() {
    #[derive(Debug, Serialize, Deserialize)]
    struct Manifest {
        package: Package,
    }

    #[derive(Debug, Serialize, Deserialize)]
    struct Package {
        name: String,
    }

    let package = Manifest {
        package: Package {
            name: "foo".to_owned(),
        },
    };
    let raw = toml::to_string_pretty(&package).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
[package]
name = "foo"

"#]]
    );
}

#[test]
fn newline_dotted_table() {
    #[derive(Debug, Serialize, Deserialize)]
    struct Manifest {
        profile: Profile,
    }

    #[derive(Debug, Serialize, Deserialize)]
    struct Profile {
        dev: Dev,
    }

    #[derive(Debug, Serialize, Deserialize)]
    struct Dev {
        debug: U32OrBool,
    }

    #[derive(Clone, Debug, Deserialize, Serialize, Eq, PartialEq)]
    #[serde(untagged, expecting = "expected a boolean or an integer")]
    pub(crate) enum U32OrBool {
        U32(u32),
        Bool(bool),
    }

    let package = Manifest {
        profile: Profile {
            dev: Dev {
                debug: U32OrBool::Bool(true),
            },
        },
    };
    let raw = toml::to_string_pretty(&package).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
[profile.dev]
debug = true

"#]]
    );
}

#[test]
fn newline_mixed_tables() {
    #[derive(Debug, Serialize, Deserialize)]
    struct Manifest {
        cargo_features: Vec<String>,
        package: Package,
        profile: Profile,
    }

    #[derive(Debug, Serialize, Deserialize)]
    struct Package {
        name: String,
        version: String,
        authors: Vec<String>,
    }

    #[derive(Debug, Serialize, Deserialize)]
    struct Profile {
        dev: Dev,
    }

    #[derive(Debug, Serialize, Deserialize)]
    struct Dev {
        debug: U32OrBool,
    }

    #[derive(Clone, Debug, Deserialize, Serialize, Eq, PartialEq)]
    #[serde(untagged, expecting = "expected a boolean or an integer")]
    pub(crate) enum U32OrBool {
        U32(u32),
        Bool(bool),
    }

    let package = Manifest {
        cargo_features: vec![],
        package: Package {
            name: "foo".to_owned(),
            version: "1.0.0".to_owned(),
            authors: vec![],
        },
        profile: Profile {
            dev: Dev {
                debug: U32OrBool::Bool(true),
            },
        },
    };
    let raw = toml::to_string_pretty(&package).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
cargo_features = []

[package]
name = "foo"
version = "1.0.0"
authors = []

[profile.dev]
debug = true

"#]]
    );
}

#[test]
fn integer_min() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a_b: i64,
    }

    equivalent! {
        Foo { a_b: i64::MIN },
        map! { a_b: Value::Integer(i64::MIN) },
    }
}

#[test]
fn integer_too_big() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a_b: u64,
    }

    let native = Foo { a_b: u64::MAX };
    let err = Table::try_from(native.clone()).unwrap_err();
    assert_data_eq!(err.to_string(), str!["u64 value was too large"].raw());
    let err = toml::to_string(&native).unwrap_err();
    assert_data_eq!(
        err.to_string(),
        str!["out-of-range value for u64 type"].raw()
    );
}

#[test]
fn integer_max() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a_b: i64,
    }

    equivalent! {
        Foo { a_b: i64::MAX },
        map! { a_b: Value::Integer(i64::MAX) },
    }
}

#[test]
fn float_min() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a_b: f64,
    }

    equivalent! {
        Foo { a_b: f64::MIN },
        map! { a_b: Value::Float(f64::MIN) },
    }
}

#[test]
fn float_max() {
    #[derive(Serialize, Deserialize, PartialEq, Debug, Clone)]
    struct Foo {
        a_b: f64,
    }

    equivalent! {
        Foo { a_b: f64::MAX },
        map! { a_b: Value::Float(f64::MAX) },
    }
}

#[test]
fn unsupported_root_type() {
    let native = "value";
    let err = toml::to_string_pretty(&native).unwrap_err();
    assert_data_eq!(err.to_string(), str!["unsupported rust type"].raw());
}

#[test]
fn unsupported_nested_type() {
    #[derive(Debug, Serialize, Deserialize)]
    struct Foo {
        unused: (),
    }

    let native = Foo { unused: () };
    let err = toml::to_string_pretty(&native).unwrap_err();
    assert_data_eq!(err.to_string(), str!["unsupported unit type"].raw());
}

#[test]
fn table_type_enum_regression_issue_388() {
    #[derive(Deserialize)]
    struct DataFile {
        #[allow(dead_code)]
        data: Compare,
    }

    #[derive(Deserialize)]
    #[allow(dead_code)]
    enum Compare {
        Gt(u32),
    }

    let dotted_table = r#"
        data.Gt = 5
        "#;
    assert!(toml::from_str::<DataFile>(dotted_table).is_ok());

    let inline_table = r#"
        data = { Gt = 5 }
        "#;
    assert!(toml::from_str::<DataFile>(inline_table).is_ok());
}

#[test]
fn serialize_datetime_issue_333() {
    use toml::{to_string, value::Date, value::Datetime};

    #[derive(Serialize)]
    struct Struct {
        date: Datetime,
    }

    let toml = to_string(&Struct {
        date: Datetime {
            date: Some(Date {
                year: 2022,
                month: 1,
                day: 1,
            }),
            time: None,
            offset: None,
        },
    })
    .unwrap();
    assert_eq!(toml, "date = 2022-01-01\n");
}

#[test]
fn datetime_offset_issue_496() {
    let original = "value = 1911-01-01T10:11:12-00:36\n";
    let toml = original.parse::<Table>().unwrap();
    let output = toml.to_string();
    assert_data_eq!(output, original.raw());
}

#[test]
fn serialize_date() {
    use toml::value::Date;

    #[derive(Serialize)]
    struct Document {
        date: Date,
    }

    let input = Document {
        date: Date {
            year: 2024,
            month: 1,
            day: 1,
        },
    };
    let raw = toml::to_string(&input).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
date = 2024-01-01

"#]]
        .raw()
    );
}

#[test]
fn serialize_time() {
    use toml::value::Time;

    #[derive(Serialize)]
    struct Document {
        date: Time,
    }

    let input = Document {
        date: Time {
            hour: 5,
            minute: 0,
            second: 0,
            nanosecond: 0,
        },
    };
    let raw = toml::to_string(&input).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
date = 05:00:00

"#]]
        .raw()
    );
}

#[test]
fn deserialize_date() {
    use toml::value::Date;

    #[derive(Debug, Deserialize)]
    struct Document {
        date: Date,
    }

    let document: Document = toml::from_str("date = 2024-01-01").unwrap();
    assert_eq!(
        document.date,
        Date {
            year: 2024,
            month: 1,
            day: 1
        }
    );

    let err = toml::from_str::<Document>("date = 2024-01-01T05:00:00").unwrap_err();
    assert_data_eq!(
        err.message(),
        str!["invalid type: local datetime, expected local date"]
    );
}

#[test]
fn deserialize_time() {
    use toml::value::Time;

    #[derive(Debug, Deserialize)]
    struct Document {
        time: Time,
    }

    let document: Document = toml::from_str("time = 05:00:00").unwrap();
    assert_eq!(
        document.time,
        Time {
            hour: 5,
            minute: 0,
            second: 0,
            nanosecond: 0,
        }
    );

    let err = toml::from_str::<Document>("time = 2024-01-01T05:00:00").unwrap_err();
    assert_data_eq!(
        err.message(),
        str!["invalid type: local datetime, expected local time"]
    );
}

#[test]
fn serialize_array_with_none_value() {
    #[derive(Serialize)]
    struct Document {
        values: Vec<Option<usize>>,
    }

    let input = Document {
        values: vec![Some(1), Some(2), Some(3)],
    };
    let raw = toml::to_string(&input).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
values = [1, 2, 3]

"#]]
        .raw()
    );

    let input = Document {
        values: vec![Some(1), None, Some(3)],
    };
    let err = toml::to_string(&input).unwrap_err();
    assert_data_eq!(err.to_string(), str!["unsupported None value"].raw());
}

#[test]
fn serialize_array_with_optional_struct_field() {
    #[derive(Debug, Deserialize, Serialize)]
    struct Document {
        values: Vec<OptionalField>,
    }

    #[derive(Debug, Deserialize, Serialize)]
    struct OptionalField {
        x: u8,
        y: Option<u8>,
    }

    let input = Document {
        values: vec![
            OptionalField { x: 0, y: Some(4) },
            OptionalField { x: 2, y: Some(5) },
            OptionalField { x: 3, y: Some(7) },
        ],
    };
    let raw = toml::to_string(&input).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
[[values]]
x = 0
y = 4

[[values]]
x = 2
y = 5

[[values]]
x = 3
y = 7

"#]]
        .raw()
    );

    let input = Document {
        values: vec![
            OptionalField { x: 0, y: Some(4) },
            OptionalField { x: 2, y: None },
            OptionalField { x: 3, y: Some(7) },
        ],
    };
    let raw = toml::to_string(&input).unwrap();
    assert_data_eq!(
        raw,
        str![[r#"
[[values]]
x = 0
y = 4

[[values]]
x = 2

[[values]]
x = 3
y = 7

"#]]
        .raw()
    );
}

#[test]
fn serialize_array_with_enum_of_optional_struct_field() {
    #[derive(Debug, Deserialize, Serialize)]
    struct Document {
        values: Vec<Choice>,
    }

    #[derive(Debug, Deserialize, Serialize)]
    enum Choice {
        Optional(OptionalField),
        Empty,
    }

    #[derive(Debug, Deserialize, Serialize)]
    struct OptionalField {
        x: u8,
        y: Option<u8>,
    }

    let input = Document {
        values: vec![
            Choice::Optional(OptionalField { x: 0, y: Some(4) }),
            Choice::Empty,
            Choice::Optional(OptionalField { x: 2, y: Some(5) }),
            Choice::Optional(OptionalField { x: 3, y: Some(7) }),
        ],
    };
    let raw = toml::to_string(&input).unwrap();
    assert_data_eq!(raw, str![[r#"
values = [{ Optional = { x = 0, y = 4 } }, "Empty", { Optional = { x = 2, y = 5 } }, { Optional = { x = 3, y = 7 } }]

"#]].raw());

    let input = Document {
        values: vec![
            Choice::Optional(OptionalField { x: 0, y: Some(4) }),
            Choice::Empty,
            Choice::Optional(OptionalField { x: 2, y: None }),
            Choice::Optional(OptionalField { x: 3, y: Some(7) }),
        ],
    };
    let raw = toml::to_string(&input).unwrap();
    assert_data_eq!(raw, str![[r#"
values = [{ Optional = { x = 0, y = 4 } }, "Empty", { Optional = { x = 2 } }, { Optional = { x = 3, y = 7 } }]

"#]].raw());
}

#[test]
fn span_for_sequence_as_map() {
    #[allow(dead_code)]
    #[derive(Deserialize)]
    struct Manifest {
        package: Package,
        bench: Vec<Bench>,
    }

    #[derive(Deserialize)]
    struct Package {}

    #[derive(Deserialize)]
    struct Bench {}

    let raw = r#"
[package]
name = "foo"
version = "0.1.0"
edition = "2021"
[[bench.foo]]
"#;
    let err = match toml::from_str::<Manifest>(raw) {
        Ok(_) => panic!("should fail"),
        Err(err) => err,
    };
    assert_eq!(err.span(), Some(61..66));
}
