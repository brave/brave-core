use libipld::cbor::DagCborCodec;
use libipld::codec::assert_roundtrip;
use libipld::{ipld, DagCbor};

#[derive(Clone, DagCbor, Debug, Default, Eq, PartialEq)]
#[ipld(repr = "tuple")]
struct ListRepr {
    a: bool,
    b: bool,
}

#[derive(Clone, DagCbor, Debug, Eq, PartialEq)]
#[ipld(repr = "kinded")]
enum KindedRepr {
    A(bool),
    B { a: u32 },
}

#[derive(Clone, DagCbor, Debug, Eq, PartialEq)]
#[ipld(repr = "string")]
enum KindedRepr2 {
    A,
    B,
}

fn main() {
    assert_roundtrip(DagCborCodec, &ListRepr::default(), &ipld!([false, false]));

    assert_roundtrip(DagCborCodec, &KindedRepr::A(true), &ipld!([true]));

    assert_roundtrip(DagCborCodec, &KindedRepr::B { a: 42 }, &ipld!({ "a": 42 }));

    assert_roundtrip(DagCborCodec, &KindedRepr2::B, &ipld!("B"));
}
