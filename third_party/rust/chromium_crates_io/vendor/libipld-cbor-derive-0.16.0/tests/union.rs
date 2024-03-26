use libipld::cbor::DagCborCodec;
use libipld::codec::assert_roundtrip;
use libipld::{ipld, DagCbor};

#[derive(Clone, Copy, DagCbor, Debug, Eq, PartialEq)]
#[ipld(repr = "keyed")]
pub enum Keyed {
    A,
    #[ipld(rename = "b")]
    #[ipld(repr = "value")]
    B(bool),
    #[ipld(repr = "value")]
    C {
        n: u32,
    },
    D(bool),
    E {
        boolean: bool,
    },
}

#[test]
fn union_keyed() {
    assert_roundtrip(DagCborCodec, &Keyed::A, &ipld!({ "A": null }));
    assert_roundtrip(DagCborCodec, &Keyed::B(true), &ipld!({"b": true}));
    assert_roundtrip(DagCborCodec, &Keyed::B(false), &ipld!({"b": false}));
    assert_roundtrip(DagCborCodec, &Keyed::C { n: 1 }, &ipld!({"C": 1}));
    assert_roundtrip(DagCborCodec, &Keyed::D(true), &ipld!({"D": [true]}));
    assert_roundtrip(
        DagCborCodec,
        &Keyed::E { boolean: true },
        &ipld!({"E": { "boolean": true }}),
    );
}

#[derive(Clone, Copy, DagCbor, Debug, Eq, PartialEq)]
#[ipld(repr = "kinded")]
pub enum Kinded {
    A,
    #[ipld(rename = "b")]
    #[ipld(repr = "value")]
    B(bool),
    #[ipld(repr = "value")]
    C {
        n: u32,
    },
    D(bool),
    E {
        boolean: bool,
    },
}

#[test]
fn union_kinded() {
    assert_roundtrip(DagCborCodec, &Kinded::A, &ipld!(null));
    assert_roundtrip(DagCborCodec, &Kinded::B(true), &ipld!(true));
    assert_roundtrip(DagCborCodec, &Kinded::B(false), &ipld!(false));
    assert_roundtrip(DagCborCodec, &Kinded::C { n: 1 }, &ipld!(1));
    assert_roundtrip(DagCborCodec, &Kinded::D(true), &ipld!([true]));
    assert_roundtrip(
        DagCborCodec,
        &Kinded::E { boolean: true },
        &ipld!({ "boolean": true }),
    );
}

#[derive(Clone, Copy, DagCbor, Debug, Eq, PartialEq)]
#[ipld(repr = "int-tuple")]
pub enum IntTuple {
    A,
    #[ipld(rename = "b")]
    #[ipld(repr = "value")]
    B(bool),
    #[ipld(repr = "value")]
    C {
        n: u32,
    },
    D(bool),
    E {
        boolean: bool,
    },
}

#[test]
fn union_int_tuple() {
    assert_roundtrip(DagCborCodec, &IntTuple::A, &ipld!([0, null]));
    assert_roundtrip(DagCborCodec, &IntTuple::B(true), &ipld!([1, true]));
    assert_roundtrip(DagCborCodec, &IntTuple::B(false), &ipld!([1, false]));
    assert_roundtrip(DagCborCodec, &IntTuple::C { n: 1 }, &ipld!([2, 1]));
    assert_roundtrip(DagCborCodec, &IntTuple::D(true), &ipld!([3, [true]]));
    assert_roundtrip(
        DagCborCodec,
        &IntTuple::E { boolean: true },
        &ipld!([4, { "boolean": true }]),
    );
}
