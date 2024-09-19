use std::convert::{TryFrom, TryInto};
use std::io::Cursor;
use std::str::FromStr;

use cid::Cid;
use libipld_core::ipld::Ipld;
use serde::de;
use serde::{Deserialize, Serialize};
use serde_bytes::ByteBuf;
use serde_ipld_dagcbor::{from_reader, from_slice, to_vec};

#[test]
fn test_cid_struct() {
    #[derive(Debug, PartialEq, Deserialize, Serialize)]
    struct MyStruct {
        cid: Cid,
        data: bool,
    }

    let cid = Cid::from_str("bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy").unwrap();
    let cid_encoded = to_vec(&cid).unwrap();
    assert_eq!(
        cid_encoded,
        [
            0xd8, 0x2a, 0x58, 0x25, 0x00, 0x01, 0x55, 0x12, 0x20, 0x2c, 0x26, 0xb4, 0x6b, 0x68,
            0xff, 0xc6, 0x8f, 0xf9, 0x9b, 0x45, 0x3c, 0x1d, 0x30, 0x41, 0x34, 0x13, 0x42, 0x2d,
            0x70, 0x64, 0x83, 0xbf, 0xa0, 0xf9, 0x8a, 0x5e, 0x88, 0x62, 0x66, 0xe7, 0xae,
        ]
    );

    let cid_decoded_as_cid: Cid = from_slice(&cid_encoded).unwrap();
    assert_eq!(cid_decoded_as_cid, cid);

    let cid_decoded_as_ipld: Ipld = from_slice(&cid_encoded).unwrap();
    assert_eq!(cid_decoded_as_ipld, Ipld::Link(cid));

    // Tests with the Type nested in a struct

    let mystruct = MyStruct { cid, data: true };
    let mystruct_encoded = to_vec(&mystruct).unwrap();
    assert_eq!(
        mystruct_encoded,
        [
            0xa2, 0x63, 0x63, 0x69, 0x64, 0xd8, 0x2a, 0x58, 0x25, 0x00, 0x01, 0x55, 0x12, 0x20,
            0x2c, 0x26, 0xb4, 0x6b, 0x68, 0xff, 0xc6, 0x8f, 0xf9, 0x9b, 0x45, 0x3c, 0x1d, 0x30,
            0x41, 0x34, 0x13, 0x42, 0x2d, 0x70, 0x64, 0x83, 0xbf, 0xa0, 0xf9, 0x8a, 0x5e, 0x88,
            0x62, 0x66, 0xe7, 0xae, 0x64, 0x64, 0x61, 0x74, 0x61, 0xf5
        ]
    );

    let mystruct_decoded_as_mystruct: MyStruct = from_slice(&mystruct_encoded).unwrap();
    assert_eq!(mystruct_decoded_as_mystruct, mystruct);

    let mystruct_decoded_as_ipld: Ipld = from_slice(&mystruct_encoded).unwrap();
    let mut expected_map = std::collections::BTreeMap::new();
    expected_map.insert("cid".to_string(), Ipld::Link(cid));
    expected_map.insert("data".to_string(), Ipld::Bool(true));
    assert_eq!(mystruct_decoded_as_ipld, Ipld::Map(expected_map));
}

/// Test that arbitrary bytes are not interpreted as CID.
#[test]
fn test_binary_not_as_cid() {
    // h'affe'
    // 42      # bytes(2)
    //    AFFE # "\xAF\xFE"
    let bytes = [0x42, 0xaf, 0xfe];
    let bytes_as_ipld: Ipld = from_slice(&bytes).unwrap();
    assert_eq!(bytes_as_ipld, Ipld::Bytes(vec![0xaf, 0xfe]));
}

/// Test that CIDs don't decode into byte buffers, lists, etc.
#[test]
fn test_cid_not_as_bytes() {
    let cbor_cid = [
        0xd8, 0x2a, 0x58, 0x25, 0x00, 0x01, 0x55, 0x12, 0x20, 0x2c, 0x26, 0xb4, 0x6b, 0x68, 0xff,
        0xc6, 0x8f, 0xf9, 0x9b, 0x45, 0x3c, 0x1d, 0x30, 0x41, 0x34, 0x13, 0x42, 0x2d, 0x70, 0x64,
        0x83, 0xbf, 0xa0, 0xf9, 0x8a, 0x5e, 0x88, 0x62, 0x66, 0xe7, 0xae,
    ];
    from_slice::<Vec<u8>>(&cbor_cid).expect_err("shouldn't have parsed a tagged CID as a sequence");
    from_slice::<serde_bytes::ByteBuf>(&cbor_cid)
        .expect_err("shouldn't have parsed a tagged CID as a byte array");
    from_slice::<serde_bytes::ByteBuf>(&cbor_cid[2..])
        .expect("should have parsed an untagged CID as a byte array");
}

/// Test whether a binary CID could be serialized if it isn't prefixed by tag 42. It should fail.
#[test]
fn test_cid_bytes_without_tag() {
    let cbor_cid = [
        0xd8, 0x2a, 0x58, 0x25, 0x00, 0x01, 0x55, 0x12, 0x20, 0x2c, 0x26, 0xb4, 0x6b, 0x68, 0xff,
        0xc6, 0x8f, 0xf9, 0x9b, 0x45, 0x3c, 0x1d, 0x30, 0x41, 0x34, 0x13, 0x42, 0x2d, 0x70, 0x64,
        0x83, 0xbf, 0xa0, 0xf9, 0x8a, 0x5e, 0x88, 0x62, 0x66, 0xe7, 0xae,
    ];
    let decoded_cbor_cid: Cid = from_slice(&cbor_cid).unwrap();
    assert_eq!(decoded_cbor_cid.to_bytes(), &cbor_cid[5..]);

    // The CID without the tag 42 prefix
    let cbor_bytes = &cbor_cid[2..];
    from_slice::<Cid>(cbor_bytes).expect_err("should have failed to decode bytes as cid");
}

/// This test shows how a kinded enum could be implemented.
#[test]
fn test_cid_in_kinded_enum() {
    #[derive(Debug, PartialEq)]
    pub enum Kinded {
        Bytes(ByteBuf),
        Link(Cid),
    }

    let cbor_cid = [
        0xd8, 0x2a, 0x58, 0x25, 0x00, 0x01, 0x55, 0x12, 0x20, 0x2c, 0x26, 0xb4, 0x6b, 0x68, 0xff,
        0xc6, 0x8f, 0xf9, 0x9b, 0x45, 0x3c, 0x1d, 0x30, 0x41, 0x34, 0x13, 0x42, 0x2d, 0x70, 0x64,
        0x83, 0xbf, 0xa0, 0xf9, 0x8a, 0x5e, 0x88, 0x62, 0x66, 0xe7, 0xae,
    ];

    impl TryFrom<Ipld> for Kinded {
        type Error = ();

        fn try_from(ipld: Ipld) -> Result<Self, Self::Error> {
            match ipld {
                Ipld::Bytes(bytes) => Ok(Self::Bytes(ByteBuf::from(bytes))),
                Ipld::Link(cid) => Ok(Self::Link(cid)),
                _ => Err(()),
            }
        }
    }

    impl<'de> de::Deserialize<'de> for Kinded {
        fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: de::Deserializer<'de>,
        {
            Ipld::deserialize(deserializer).and_then(|ipld| {
                ipld.try_into()
                    .map_err(|_| de::Error::custom("No matching enum variant found"))
            })
        }
    }

    let decoded_cid: Kinded = from_slice(&cbor_cid).unwrap();
    let cid = Cid::try_from(&cbor_cid[5..]).unwrap();
    assert_eq!(decoded_cid, Kinded::Link(cid));

    // The CID without the tag 42 prefix
    let cbor_bytes = &cbor_cid[2..];
    let decoded_bytes: Kinded = from_slice(cbor_bytes).unwrap();
    // The CBOR decoded bytes don't contain the prefix with the bytes type identifier and the
    // length.
    let bytes = cbor_bytes[2..].to_vec();
    assert_eq!(decoded_bytes, Kinded::Bytes(ByteBuf::from(bytes)));

    // Check that random bytes cannot be deserialized.
    let random_bytes = &cbor_cid[10..];
    let decoded_random_bytes: Result<Kinded, _> = from_slice(random_bytes);
    assert!(decoded_random_bytes.is_err());
}

/// This test shows how a kinded enum could be implemented, when bytes as well as a CID are wrapped
/// in a newtype struct.
#[test]
fn test_cid_in_kinded_enum_with_newtype() {
    #[derive(Debug, Deserialize, PartialEq)]
    pub struct Foo(#[serde(with = "serde_bytes")] Vec<u8>);

    #[derive(Debug, PartialEq)]
    pub enum Kinded {
        MyBytes(Foo),
        Link(Cid),
    }

    let cbor_cid = [
        0xd8, 0x2a, 0x58, 0x25, 0x00, 0x01, 0x55, 0x12, 0x20, 0x2c, 0x26, 0xb4, 0x6b, 0x68, 0xff,
        0xc6, 0x8f, 0xf9, 0x9b, 0x45, 0x3c, 0x1d, 0x30, 0x41, 0x34, 0x13, 0x42, 0x2d, 0x70, 0x64,
        0x83, 0xbf, 0xa0, 0xf9, 0x8a, 0x5e, 0x88, 0x62, 0x66, 0xe7, 0xae,
    ];

    impl TryFrom<Ipld> for Kinded {
        type Error = ();

        fn try_from(ipld: Ipld) -> Result<Self, Self::Error> {
            match ipld {
                Ipld::Bytes(bytes) => Ok(Self::MyBytes(Foo(bytes))),
                Ipld::Link(cid) => Ok(Self::Link(cid)),
                _ => Err(()),
            }
        }
    }

    impl<'de> de::Deserialize<'de> for Kinded {
        fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
        where
            D: de::Deserializer<'de>,
        {
            Ipld::deserialize(deserializer).and_then(|ipld| {
                ipld.try_into()
                    .map_err(|_| de::Error::custom("No matching enum variant found"))
            })
        }
    }

    let decoded_cid: Kinded = from_slice(&cbor_cid).unwrap();
    // The actual CID is without the CBOR tag 42, the byte identifier and the data length.
    let cid = Cid::try_from(&cbor_cid[5..]).unwrap();
    assert_eq!(decoded_cid, Kinded::Link(cid));

    // The CID without the tag 42 prefix
    let cbor_bytes = &cbor_cid[2..];
    let decoded_bytes: Kinded = from_slice(cbor_bytes).unwrap();
    // The CBOR decoded bytes don't contain the prefix with the bytes type identifier and the
    // length.
    let bytes = cbor_bytes[2..].to_vec();
    assert_eq!(decoded_bytes, Kinded::MyBytes(Foo(bytes)));

    // Check that random bytes cannot be deserialized.
    let random_bytes = &cbor_cid[10..];
    let decoded_random_bytes: Result<Kinded, _> = from_slice(random_bytes);
    assert!(decoded_random_bytes.is_err());
}

#[test]
fn test_cid_empty_errors() {
    // Tag 42 with zero bytes
    let cbor_empty_cid = [0xd8, 0x2a, 0x40];

    let decoded: Result<Cid, _> = from_slice(&cbor_empty_cid);
    assert!(decoded.is_err());
}

#[test]
fn test_cid_non_minimally_encoded() {
    let cid = Cid::from_str("bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy").unwrap();
    let cid_encoded = to_vec(&cid).unwrap();

    let decoded: Cid = from_slice(&cid_encoded).unwrap();
    assert_eq!(decoded, cid);

    // Strip off the CBOR tag.
    let without_tag = &cid_encoded[2..];

    let tag_2_bytes_encoded = [&[0xd9, 0x00, 0x2a], without_tag].concat();
    let tag_2_bytes_decoded: Cid = from_slice(&tag_2_bytes_encoded).unwrap();
    assert_eq!(tag_2_bytes_decoded, cid);

    let tag_4_bytes_encoded = [&[0xda, 0x00, 0x00, 0x00, 0x2a], without_tag].concat();
    let tag_4_bytes_decoded: Cid = from_slice(&tag_4_bytes_encoded).unwrap();
    assert_eq!(tag_4_bytes_decoded, cid);

    let tag_8_bytes_encoded = [
        &[0xdb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a],
        without_tag,
    ]
    .concat();
    let tag_8_bytes_decoded: Cid = from_slice(&tag_8_bytes_encoded).unwrap();
    assert_eq!(tag_8_bytes_decoded, cid);
}

#[test]
fn test_cid_decode_from_reader() {
    let cid_encoded = [
        0xd8, 0x2a, 0x49, 0x00, 0x01, 0xce, 0x01, 0x9b, 0x01, 0x02, 0x63, 0xc8,
    ];
    println!("vmx: cid: {:?}", cid_encoded);
    let cid_decoded: Cid = from_reader(Cursor::new(&cid_encoded)).unwrap();
    println!("vmx: cid: {:?}", cid_decoded);
    assert_eq!(&cid_encoded[4..], &cid_decoded.to_bytes());
}
