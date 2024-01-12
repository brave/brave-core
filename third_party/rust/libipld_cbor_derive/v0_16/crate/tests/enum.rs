use libipld::cbor::DagCborCodec;
use libipld::codec::assert_roundtrip;
use libipld::{ipld, DagCbor};

#[derive(Clone, Copy, DagCbor, Debug, Eq, PartialEq)]
#[ipld(repr = "int")]
pub enum EnumInt {
    Variant = 1,
    Other = 0,
}

#[test]
fn enum_int() {
    assert_roundtrip(DagCborCodec, &EnumInt::Variant, &ipld!(1));
    assert_roundtrip(DagCborCodec, &EnumInt::Other, &ipld!(0));
}

#[derive(Clone, DagCbor, Debug, Eq, PartialEq)]
#[ipld(repr = "string")]
pub enum EnumString {
    #[ipld(rename = "test")]
    Variant,
    Other,
}

#[test]
fn enum_string() {
    assert_roundtrip(DagCborCodec, &EnumString::Variant, &ipld!("test"));
    assert_roundtrip(DagCborCodec, &EnumString::Other, &ipld!("Other"));
}
