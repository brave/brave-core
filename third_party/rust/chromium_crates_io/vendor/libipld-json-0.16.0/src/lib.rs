//! Json codec.
#![deny(missing_docs)]
#![deny(warnings)]

use core::convert::TryFrom;
use libipld_core::cid::Cid;
use libipld_core::codec::{Codec, Decode, Encode, References};
use libipld_core::error::{Result, UnsupportedCodec};
use libipld_core::ipld::Ipld;
// TODO vmx 2020-05-28: Don't expose the `serde_json` error directly, but wrap it in a custom one
pub use serde_json::Error;
use std::io::{Read, Seek, Write};

mod codec;

/// Json codec.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct DagJsonCodec;

impl Codec for DagJsonCodec {}

impl From<DagJsonCodec> for u64 {
    fn from(_: DagJsonCodec) -> Self {
        0x0129
    }
}

impl TryFrom<u64> for DagJsonCodec {
    type Error = UnsupportedCodec;

    fn try_from(_: u64) -> core::result::Result<Self, Self::Error> {
        Ok(Self)
    }
}

impl Encode<DagJsonCodec> for Ipld {
    fn encode<W: Write>(&self, _: DagJsonCodec, w: &mut W) -> Result<()> {
        Ok(codec::encode(self, w)?)
    }
}

impl Decode<DagJsonCodec> for Ipld {
    fn decode<R: Read + Seek>(_: DagJsonCodec, r: &mut R) -> Result<Self> {
        Ok(codec::decode(r)?)
    }
}

impl References<DagJsonCodec> for Ipld {
    fn references<R: Read + Seek, E: Extend<Cid>>(
        c: DagJsonCodec,
        r: &mut R,
        set: &mut E,
    ) -> Result<()> {
        Ipld::decode(c, r)?.references(set);
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use libipld_core::cid::Cid;
    use libipld_core::multihash::{Code, MultihashDigest};
    use std::collections::BTreeMap;

    #[test]
    fn encode_struct() {
        let digest = Code::Blake3_256.digest(&b"block"[..]);
        let cid = Cid::new_v1(0x55, digest);

        // Create a contact object that looks like:
        // Contact { name: "Hello World", details: CID }
        let mut map = BTreeMap::new();
        map.insert("name".to_string(), Ipld::String("Hello World!".to_string()));
        map.insert("details".to_string(), Ipld::Link(cid));
        let contact = Ipld::Map(map);

        let contact_encoded = DagJsonCodec.encode(&contact).unwrap();
        println!("encoded: {:02x?}", contact_encoded);
        println!(
            "encoded string {}",
            std::str::from_utf8(&contact_encoded).unwrap()
        );

        assert_eq!(
            std::str::from_utf8(&contact_encoded).unwrap(),
            format!(r#"{{"details":{{"/":"{}"}},"name":"Hello World!"}}"#, cid)
        );

        let contact_decoded: Ipld = DagJsonCodec.decode(&contact_encoded).unwrap();
        assert_eq!(contact_decoded, contact);
    }
}
