#![cfg(all(feature = "serde1", feature = "use_std"))]

use std::iter::FromIterator;
use std::collections::BTreeMap;
use serde::{ Serialize, Deserialize };
use serde_cbor::value::Value;
use cbor4ii::serde::to_vec;


#[track_caller]
fn de<'a,T>(bytes: &'a [u8], _value: &T)
    -> T
where T: Deserialize<'a>
{
    serde_cbor::from_slice(bytes).unwrap()
}

macro_rules! assert_test {
    ( $value:expr ) => {{
        let buf = to_vec(Vec::new(), &$value).unwrap();
        let value = de(&buf, &$value);
        assert_eq!(value, $value);
    }}
}

#[test]
fn test_serialize_compat() {
    let value = vec![
        Some(0x99u32),
        None,
        Some(0x33u32)
    ];
    assert_test!(value);

    #[derive(Serialize, Deserialize, Eq, PartialEq, Debug)]
    enum Enum {
        Unit,
        NewType(i32),
        Tuple(String, bool),
        Struct { x: i32, y: i32 },
    }
    assert_test!(Enum::Unit);
    assert_test!(Enum::NewType(0x999));
    assert_test!(Enum::Tuple("123".into(), false));
    assert_test!(Enum::Struct { x: 0x99, y: -0x99 });

    #[derive(Serialize, Deserialize, Eq, PartialEq, Debug)]
    #[serde(untagged)]
    enum UntaggedEnum {
        Bar(i32),
        Foo(Box<str>)
    }

    #[derive(Serialize, Deserialize, Eq, PartialEq, Debug)]
    struct NewType<T>(T);

    #[derive(Serialize, Deserialize, Eq, PartialEq, Debug)]
    struct Test {
        name: char,
        test: TestMap,
        #[serde(with = "serde_bytes")]
        bytes: Vec<u8>,
        #[serde(with = "serde_bytes")]
        bytes2: Vec<u8>,
        map: BTreeMap<String, Enum>,
        untag: (UntaggedEnum, UntaggedEnum),
        new: NewType<UntaggedEnum>
    }
    #[derive(Serialize, Deserialize, Eq, PartialEq, Debug)]
    struct TestMap(BTreeMap<TestObj, BoxSet>);
    #[derive(Serialize, Deserialize, Eq, PartialEq, Ord, PartialOrd, Debug)]
    struct TestObj(String);
    #[derive(Serialize, Deserialize, Eq, PartialEq, Debug)]
    struct BoxSet(Vec<TestObj>);

    let test = Test {
        name: 'G',
        test: TestMap({
            let mut map = BTreeMap::new();
            map.insert(TestObj("obj".into()), BoxSet(Vec::new()));
            map.insert(TestObj("obj2".into()), BoxSet(vec![
                TestObj("obj3".into()),
                TestObj("obj4".into()),
                TestObj(String::new())
            ]));
            map
        }),
        bytes: Vec::from("bbbbbbbbbbb".as_bytes()),
        bytes2: Vec::new(),
        map: {
            let mut map = BTreeMap::new();
            map.insert("key0".into(), Enum::Unit);
            map.insert("key1".into(), Enum::Tuple("value".into(), true));
            map.insert("key2".into(), Enum::Struct {
                x: -1,
                y: 0x123
            });
            map.insert("key3".into(), Enum::NewType(-999));
            map
        },
        untag: (UntaggedEnum::Foo("a".into()), UntaggedEnum::Bar(0)),
        new: NewType(UntaggedEnum::Foo("???".into()))
    };
    assert_test!(test);
}

#[test]
fn test_serialize_display() {
    use std::fmt;
    use serde::ser::Serializer;

    struct Display<'a>(&'a dyn fmt::Debug);

    impl fmt::Display for Display<'_> {
        fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
            fmt::Debug::fmt(self.0, f)
        }
    }

    impl Serialize for Display<'_> {
        fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
        where
            S: Serializer
        {
            serializer.collect_str(self)
        }
    }

    #[derive(Serialize)]
    struct Test<'a> {
        print: Display<'a>
    }

    #[derive(Deserialize)]
    struct Test2 {
        print: String
    }

    {
        let a = (1u32, "test", 0x99u64);
        let test = Test {
            print: Display(&a)
        };
        let buf = to_vec(Vec::new(), &test).unwrap();
        let value: Test2 = serde_cbor::from_slice(&buf).unwrap();
        assert_eq!(value.print, format!("{:?}", a));
    }

    {
        let a = ([0x999i64; 100], vec!['#'; 100], &[..; 100][..]);
        let test = Test {
            print: Display(&a)
        };
        let buf = to_vec(Vec::new(), &test).unwrap();
        let value: Test2 = serde_cbor::from_slice(&buf).unwrap();
        assert!(buf.len() > 300);
        assert_eq!(value.print, format!("{:?}", a));
    }
}

// copy from https://github.com/pyfisch/cbor/blob/master/tests/value.rs

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
struct TupleStruct(String, i32, u64);

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
struct U8Struct(u8);

#[derive(Serialize, Deserialize, Clone, Debug, PartialEq)]
struct Struct<'a> {
    tuple_struct: TupleStruct,
    tuple: (String, f32, f64),
    map: BTreeMap<String, String>,
    bytes: &'a [u8],
    array: Vec<String>,
    u8_array: Vec<U8Struct>,
}

#[test]
fn serde() {
    let tuple_struct = TupleStruct(format!("test"), -60, 3000);

    let tuple = (format!("hello"), -50.0040957, -12.094635556478);

    let map = BTreeMap::from_iter(
        [
            (format!("key1"), format!("value1")),
            (format!("key2"), format!("value2")),
            (format!("key3"), format!("value3")),
            (format!("key4"), format!("value4")),
        ]
        .iter()
        .cloned(),
    );

    let bytes = b"test byte string";

    let array = vec![format!("one"), format!("two"), format!("three")];
    let u8_array = vec![U8Struct(0), U8Struct(1), U8Struct(1)];

    let data = Struct {
        tuple_struct,
        tuple,
        map,
        bytes,
        array,
        u8_array,
    };

    let value = serde_cbor::value::to_value(data.clone()).unwrap();
    println!("{:?}", value);

    let data_ser = to_vec(Vec::new(), &value).unwrap();
    let data_de_value: Value = serde_cbor::from_slice(&data_ser).unwrap();

    fn as_object(value: &Value) -> &BTreeMap<Value, Value> {
        if let Value::Map(ref v) = value {
            return v;
        }
        panic!()
    }

    for ((k1, v1), (k2, v2)) in as_object(&value)
        .iter()
        .zip(as_object(&data_de_value).iter())
    {
        assert_eq!(k1, k2);
        assert_eq!(v1, v2);
    }

    assert_eq!(value, data_de_value);
}
