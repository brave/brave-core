/// Serde untagged (`#[serde(untagged)]`) and internaly tagged enums (`#[serde(tag = "tag")]`) are
/// not supported by CIDs. Here examples are provided on how to implement similar behaviour. This
/// file also contains an example for a kinded enum.
use std::convert::{TryFrom, TryInto};

use ipld_core::{cid::Cid, ipld::Ipld};
use serde::{de, Deserialize};
use serde_bytes::ByteBuf;
use serde_derive::Deserialize;
use serde_ipld_dagcbor::from_slice;

/// The CID `bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy` encoded as CBOR
/// 42(h'00015512202C26B46B68FFC68FF99B453C1D30413413422D706483BFA0F98A5E886266E7AE')
const CBOR_CID_FIXTURE: [u8; 41] = [
    0xd8, 0x2a, 0x58, 0x25, 0x00, 0x01, 0x55, 0x12, 0x20, 0x2c, 0x26, 0xb4, 0x6b, 0x68, 0xff, 0xc6,
    0x8f, 0xf9, 0x9b, 0x45, 0x3c, 0x1d, 0x30, 0x41, 0x34, 0x13, 0x42, 0x2d, 0x70, 0x64, 0x83, 0xbf,
    0xa0, 0xf9, 0x8a, 0x5e, 0x88, 0x62, 0x66, 0xe7, 0xae,
];

/// This enum shows how an internally tagged enum could be implemented.
#[derive(Debug, PartialEq)]
enum CidInInternallyTaggedEnum {
    MyCid { cid: Cid },
}

// This manual deserializer implementation works as if you would derive `Deserialize` and add
// `#[serde(tag = "type")]` to the `CidInternallyTaggedEnum` enum.
impl<'de> de::Deserialize<'de> for CidInInternallyTaggedEnum {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        #[derive(Deserialize)]
        struct Tagged {
            r#type: String,
            cid: Cid,
        }

        let Tagged { r#type, cid } = Deserialize::deserialize(deserializer)?;
        if r#type == "MyCid" {
            Ok(CidInInternallyTaggedEnum::MyCid { cid })
        } else {
            Err(de::Error::custom("No matching enum variant found"))
        }
    }
}

/// This enum shows how an untagged enum could be implemented.
#[derive(Debug, PartialEq)]
enum CidInUntaggedEnum {
    MyCid(Cid),
}

// This manual deserializer implementation works as if you would derive `Deserialize` and add
// `#[serde(untagged)]`.
impl<'de> de::Deserialize<'de> for CidInUntaggedEnum {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: de::Deserializer<'de>,
    {
        Cid::deserialize(deserializer)
            .map(CidInUntaggedEnum::MyCid)
            .map_err(|_| de::Error::custom("No matching enum variant found"))
    }
}

/// This enum shows how a kinded enum could be implemented.
#[derive(Debug, PartialEq)]
pub enum Kinded {
    Bytes(ByteBuf),
    Link(Cid),
}

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

pub fn main() {
    let cid: Cid = from_slice(&CBOR_CID_FIXTURE).unwrap();

    // {"type": "MyCid", "cid": 42(h'00015512202C26B46B68FFC68FF99B453C1D30413413422D706483BFA0F98A5E886266E7AE')}
    let cbor_internally_tagged_enum = [
        &[
            0xa2, 0x64, 0x74, 0x79, 0x70, 0x65, 0x65, 0x4d, 0x79, 0x43, 0x69, 0x64, 0x63, 0x63,
            0x69, 0x64,
        ],
        &CBOR_CID_FIXTURE[..],
    ]
    .concat();
    assert_eq!(
        from_slice::<CidInInternallyTaggedEnum>(&cbor_internally_tagged_enum).unwrap(),
        CidInInternallyTaggedEnum::MyCid { cid }
    );

    assert_eq!(
        from_slice::<CidInUntaggedEnum>(&CBOR_CID_FIXTURE).unwrap(),
        CidInUntaggedEnum::MyCid(cid)
    );

    assert_eq!(
        from_slice::<Kinded>(&CBOR_CID_FIXTURE).unwrap(),
        Kinded::Link(cid)
    );

    // The CID without the tag 42 prefix, so that it decodes as just bytes.
    let cbor_bytes = &CBOR_CID_FIXTURE[2..];
    let decoded_bytes: Kinded = from_slice(cbor_bytes).unwrap();
    // The CBOR decoded bytes don't contain the prefix with the bytes type identifier and the
    // length.
    let bytes = cbor_bytes[2..].to_vec();
    assert_eq!(decoded_bytes, Kinded::Bytes(ByteBuf::from(bytes)));
}

// Make it possible to run this example as test.
#[test]
fn test_main() {
    main()
}
