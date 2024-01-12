use libipld::cbor::DagCborCodec;
use libipld::codec::assert_roundtrip;
use libipld::{ipld, DagCbor, Ipld};
use std::collections::BTreeMap;

#[derive(Clone, DagCbor, Debug, Default, PartialEq)]
struct NamedStruct {
    boolean: bool,
    integer: u32,
    float: f64,
    string: String,
    bytes: Vec<u8>,
    list: Vec<Ipld>,
    map: BTreeMap<String, Ipld>,
    //link: Cid,
}

#[derive(Clone, DagCbor, Debug, Default, Eq, PartialEq)]
struct TupleStruct(bool, u32);

#[derive(Clone, DagCbor, Debug, Default, Eq, PartialEq)]
struct UnitStruct;

#[derive(Clone, DagCbor, Debug, Eq, PartialEq)]
enum Enum {
    A,
    B(bool, u32),
    C { boolean: bool, int: u32 },
}

#[derive(Clone, DagCbor, Debug, PartialEq)]
struct Nested {
    ipld: Ipld,
    list_of_derived: Vec<Enum>,
    map_of_derived: BTreeMap<String, NamedStruct>,
}

fn main() {
    assert_roundtrip(
        DagCborCodec,
        &NamedStruct::default(),
        &ipld!({
            "boolean": false,
            "integer": 0,
            "float": 0.0,
            "string": "",
            "bytes": [],
            "list": [],
            "map": {},
        }),
    );

    assert_roundtrip(DagCborCodec, &TupleStruct::default(), &ipld!([false, 0]));

    assert_roundtrip(DagCborCodec, &UnitStruct::default(), &ipld!(null));

    assert_roundtrip(DagCborCodec, &Enum::A, &ipld!({ "A": null }));

    assert_roundtrip(
        DagCborCodec,
        &Enum::B(true, 42),
        &ipld!({ "B": [true, 42] }),
    );

    assert_roundtrip(
        DagCborCodec,
        &Enum::C {
            boolean: true,
            int: 42,
        },
        &ipld!({ "C": { "boolean": true, "int": 42} }),
    );
}
