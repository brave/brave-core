use libipld::cbor::DagCborCodec;
use libipld::codec::{Decode, Encode};
use libipld::ipld::Ipld;
use libipld::{ipld, DagCbor};
use std::io::Cursor;

#[derive(Clone, Debug, Default, PartialEq, DagCbor)]
struct RenameFields {
    #[ipld(rename = "hashAlg")]
    hash_alg: String,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let data = RenameFields {
        hash_alg: "murmur3".to_string(),
    };
    let mut bytes = Vec::new();
    data.encode(DagCborCodec, &mut bytes)?;
    let ipld: Ipld = Decode::decode(DagCborCodec, &mut Cursor::new(bytes.as_slice()))?;
    let expect = ipld!({
        "hashAlg": "murmur3",
    });
    assert_eq!(ipld, expect);
    let data2 = RenameFields::decode(DagCborCodec, &mut Cursor::new(bytes.as_slice()))?;
    assert_eq!(data, data2);
    Ok(())
}
