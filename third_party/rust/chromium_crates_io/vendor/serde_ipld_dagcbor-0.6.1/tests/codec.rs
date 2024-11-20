#![cfg(all(feature = "std", not(feature = "no-cid-as-bytes")))]

use core::{convert::TryFrom, iter};

use ipld_core::{
    cid::Cid,
    codec::{Codec, Links},
    ipld,
    ipld::Ipld,
};
use serde_ipld_dagcbor::codec::DagCborCodec;

#[test]
fn test_codec_encode() {
    let data = "hello world!".to_string();
    let expected = b"\x6chello world!";

    let mut output = Vec::new();
    DagCborCodec::encode(&mut output, &data).unwrap();
    assert_eq!(output, expected);

    let encoded = DagCborCodec::encode_to_vec(&data).unwrap();
    assert_eq!(encoded, expected);
}

#[test]
fn test_codec_decode() {
    let data = b"\x6chello world!";
    let expected = "hello world!".to_string();

    let decoded: String = DagCborCodec::decode(&data[..]).unwrap();
    assert_eq!(decoded, expected);

    let decoded_from_slice: String = DagCborCodec::decode_from_slice(data).unwrap();
    assert_eq!(decoded_from_slice, expected);
}

#[test]
fn test_codec_links() {
    let cid = Cid::try_from("bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy").unwrap();
    let data: Ipld = ipld!({"some": {"nested": cid}, "or": [cid, cid], "foo": true});
    let expected = iter::repeat(cid).take(3).collect::<Vec<_>>();

    let mut encoded = Vec::new();
    DagCborCodec::encode(&mut encoded, &data).unwrap();

    let links = DagCborCodec::links(&encoded).unwrap();
    assert_eq!(links.collect::<Vec<_>>(), expected);
}

#[test]
fn test_codec_generic() {
    fn encode_generic<C, T>(value: T) -> Vec<u8>
    where
        C: Codec<T>,
        C::Error: std::fmt::Debug,
    {
        C::encode_to_vec(&value).unwrap()
    }

    let data = "hello world!".to_string();
    let expected = b"\x6chello world!";

    let encoded = encode_generic::<DagCborCodec, _>(data);
    assert_eq!(encoded, expected);
}
