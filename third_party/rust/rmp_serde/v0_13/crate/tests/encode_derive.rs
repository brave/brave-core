extern crate serde;
#[macro_use]
extern crate serde_derive;
extern crate rmp;
extern crate rmp_serde as rmps;

use serde::Serialize;
use rmps::Serializer;

#[test]
fn pass_unit_struct() {
    #[derive(Serialize)]
    struct Unit;

    let mut buf = Vec::new();
    Unit.serialize(&mut Serializer::new(&mut buf)).unwrap();

    // Expect: `[]`.
    assert_eq!(vec![0x90], buf);
}

#[test]
fn pass_unit_variant() {
    #[derive(Serialize)]
    enum Enum {
        V1,
        V2,
    }

    let mut buf = Vec::new();
    Enum::V1.serialize(&mut Serializer::new(&mut buf)).unwrap();
    Enum::V2.serialize(&mut Serializer::new(&mut buf)).unwrap();

    // Expect: [0, []] [1, []].
    assert_eq!(vec![0x92, 0x00, 0x90, 0x92, 0x01, 0x90], buf);
}

#[test]
fn pass_newtype_struct() {
    #[derive(Serialize)]
    struct Struct(u64);

    let val = Struct(42);
    let mut buf = Vec::new();
    val.serialize(&mut Serializer::new(&mut buf)).unwrap();

    // Expect: [42].
    assert_eq!(vec![0x91, 0x2a], buf);
}

#[test]
fn pass_newtype_variant() {
    #[derive(Serialize)]
    enum Enum {
        V1,
        V2(u64),
    }

    let mut buf = Vec::new();
    Enum::V1.serialize(&mut Serializer::new(&mut buf)).unwrap();
    Enum::V2(42).serialize(&mut Serializer::new(&mut buf)).unwrap();

    // Expect: [0, []] [1, [42]].
    assert_eq!(vec![0x92, 0x00, 0x90, 0x92, 0x01, 0x91, 0x2a], buf);
}

#[test]
fn pass_tuple_struct() {
    #[derive(Serialize)]
    struct Struct(u32, u64);

    let val = Struct(42, 100500);
    let mut buf = Vec::new();
    val.serialize(&mut Serializer::new(&mut buf)).unwrap();

    // Expect: [42, 100500].
    assert_eq!(vec![0x92, 0x2a, 0xce, 0x00, 0x01, 0x88, 0x94], buf);
}

#[test]
fn pass_tuple_variant() {
    #[derive(Serialize)]
    enum Enum {
        V1,
        V2(u32, u64),
    }

    let mut buf = Vec::new();
    Enum::V1.serialize(&mut Serializer::new(&mut buf)).unwrap();
    Enum::V2(42, 100500).serialize(&mut Serializer::new(&mut buf)).unwrap();

    // Expect: [0, []] [2, [42, 100500]].
    assert_eq!(vec![0x92, 0x00, 0x90, 0x92, 0x01, 0x92, 0x2a, 0xce, 0x00, 0x01, 0x88, 0x94], buf);
}

#[test]
fn pass_struct() {
    #[derive(Serialize)]
    struct Struct {
        f1: u32,
        f2: u32,
    }

    let val = Struct {
        f1: 42,
        f2: 100500,
    };
    let mut buf = Vec::new();
    val.serialize(&mut Serializer::new(&mut buf)).unwrap();

    // Expect: [42, 100500].
    assert_eq!(vec![0x92, 0x2a, 0xce, 0x0, 0x1, 0x88, 0x94], buf);
}

#[test]
fn serialize_struct_variant() {
    #[derive(Serialize)]
    enum Enum {
        V1 {
            f1: u32,
        },
        V2 {
            f1: u32,
        },
    }

    let mut buf = Vec::new();
    Enum::V1 { f1: 42 }.serialize(&mut Serializer::new(&mut buf)).unwrap();
    Enum::V2 { f1: 43 }.serialize(&mut Serializer::new(&mut buf)).unwrap();

    // Expect: [0, [42]] [1, [43]].
    assert_eq!(vec![0x92, 0x00, 0x91, 0x2a, 0x92, 0x01, 0x91, 0x2b], buf);
}

#[test]
fn pass_struct_as_map() {
    use std::io::Write;
    use rmp::Marker;
    use rmp::encode::{ValueWriteError, write_map_len, write_str};
    use rmps::encode::VariantWriter;

    #[derive(Serialize)]
    struct Dog {
        name: String,
        age: u16,
    }

    let dog = Dog {
        name: "Bobby".into(),
        age: 8,
    };

    struct StructMapWriter;

    impl VariantWriter for StructMapWriter {
        fn write_struct_len<W: Write>(&self, wr: &mut W, len: u32) ->
            Result<Marker, ValueWriteError>
        {
            write_map_len(wr, len)
        }

        fn write_field_name<W: Write>(&self, wr: &mut W, key: &str) ->
            Result<(), ValueWriteError>
        {
            write_str(wr, key)
        }
    }

    let mut se = Serializer::with(Vec::new(), StructMapWriter);
    dog.serialize(&mut se).unwrap();

    // Expect: {"name": "Bobby", "age": 8}.
    assert_eq!(vec![0x82, 0xa4, 0x6e, 0x61, 0x6d, 0x65, 0xa5, 0x42, 0x6f, 0x62, 0x62, 0x79, 0xa3, 0x61, 0x67, 0x65, 0x08],
        se.into_inner());
}
