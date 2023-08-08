use std::collections::HashMap;
use std::convert::{TryFrom, TryInto};
use std::str::FromStr;

use cid::{Cid, CidGeneric, Error, Version};
use multibase::Base;
use multihash::{derive::Multihash, Code, MultihashDigest};

const RAW: u64 = 0x55;
const DAG_PB: u64 = 0x70;

#[test]
fn basic_marshalling() {
    let h = Code::Sha2_256.digest(b"beep boop");

    let cid = Cid::new_v1(DAG_PB, h);

    let data = cid.to_bytes();
    let out = Cid::try_from(data.clone()).unwrap();
    assert_eq!(cid, out);

    let out2 = data.try_into().unwrap();
    assert_eq!(cid, out2);

    let s = cid.to_string();
    let out3 = Cid::try_from(&s[..]).unwrap();
    assert_eq!(cid, out3);

    let out4 = (&s[..]).try_into().unwrap();
    assert_eq!(cid, out4);
}

#[test]
fn empty_string() {
    assert!(matches!(Cid::try_from(""), Err(Error::InputTooShort)))
}

#[test]
fn v0_handling() {
    let old = "QmdfTbBqBPQ7VNxZEYEj14VmRuZBkqFbiwReogJgS1zR1n";
    let cid = Cid::try_from(old).unwrap();

    assert_eq!(cid.version(), Version::V0);
    assert_eq!(cid.to_string(), old);
}

#[test]
fn from_str() {
    let cid: Cid = "QmdfTbBqBPQ7VNxZEYEj14VmRuZBkqFbiwReogJgS1zR1n"
        .parse()
        .unwrap();
    assert_eq!(cid.version(), Version::V0);

    let bad = "QmdfTbBqBPQ7VNxZEYEj14VmRuZBkqFbiwReogJgS1zIII".parse::<Cid>();
    assert!(matches!(bad, Err(Error::ParsingError)));
}

#[test]
fn v0_error() {
    let bad = "QmdfTbBqBPQ7VNxZEYEj14VmRuZBkqFbiwReogJgS1zIII";
    assert!(matches!(Cid::try_from(bad), Err(Error::ParsingError)));
}

#[test]
fn from() {
    let the_hash = "QmdfTbBqBPQ7VNxZEYEj14VmRuZBkqFbiwReogJgS1zR1n";

    let cases = vec![
        format!("/ipfs/{:}", &the_hash),
        format!("https://ipfs.io/ipfs/{:}", &the_hash),
        format!("http://localhost:8080/ipfs/{:}", &the_hash),
    ];

    for case in cases {
        let cid = Cid::try_from(case).unwrap();
        assert_eq!(cid.version(), Version::V0);
        assert_eq!(cid.to_string(), the_hash);
    }
}

#[test]
fn test_hash() {
    let data: Vec<u8> = vec![1, 2, 3];
    let hash = Code::Sha2_256.digest(&data);
    let mut map = HashMap::new();
    let cid = Cid::new_v0(hash).unwrap();
    map.insert(cid, data.clone());
    assert_eq!(&data, map.get(&cid).unwrap());
}

#[test]
fn test_base32() {
    let cid = Cid::from_str("bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy").unwrap();
    assert_eq!(cid.version(), Version::V1);
    assert_eq!(cid.codec(), RAW);
    assert_eq!(cid.hash(), &Code::Sha2_256.digest(b"foo"));
}

#[test]
fn to_string() {
    let expected_cid = "bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy";
    let cid = Cid::new_v1(RAW, Code::Sha2_256.digest(b"foo"));
    assert_eq!(cid.to_string(), expected_cid);
}

#[test]
fn to_string_of_base32() {
    let expected_cid = "bafkreibme22gw2h7y2h7tg2fhqotaqjucnbc24deqo72b6mkl2egezxhvy";
    let cid = Cid::new_v1(RAW, Code::Sha2_256.digest(b"foo"));
    assert_eq!(
        cid.to_string_of_base(Base::Base32Lower).unwrap(),
        expected_cid
    );
}

#[test]
fn to_string_of_base64() {
    let expected_cid = "mAVUSICwmtGto/8aP+ZtFPB0wQTQTQi1wZIO/oPmKXohiZueu";
    let cid = Cid::new_v1(RAW, Code::Sha2_256.digest(b"foo"));
    assert_eq!(cid.to_string_of_base(Base::Base64).unwrap(), expected_cid);
}

#[test]
fn to_string_of_base58_v0() {
    let expected_cid = "QmRJzsvyCQyizr73Gmms8ZRtvNxmgqumxc2KUp71dfEmoj";
    let cid = Cid::new_v0(Code::Sha2_256.digest(b"foo")).unwrap();
    assert_eq!(
        cid.to_string_of_base(Base::Base58Btc).unwrap(),
        expected_cid
    );
}

#[test]
fn to_string_of_base_v0_error() {
    let cid = Cid::new_v0(Code::Sha2_256.digest(b"foo")).unwrap();
    assert!(matches!(
        cid.to_string_of_base(Base::Base16Upper),
        Err(Error::InvalidCidV0Base)
    ));
}

#[test]
fn explicit_v0_is_disallowed() {
    use std::io::Cursor;
    assert!(matches!(
        Cid::read_bytes(Cursor::new([
            0x00, 0x70, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12
        ])),
        Err(Error::InvalidExplicitCidV0)
    ));
}

fn a_function_that_takes_a_generic_cid<const S: usize>(cid: &CidGeneric<S>) -> String {
    cid.to_string()
}

// This test is about having something implemented that used the default size of `Cid`. So the code
// is using `Cid` instead of `Cid<SomeSize>`. The code will still work with other sizes.
#[test]
fn method_can_take_differently_sized_cids() {
    #[derive(Clone, Copy, Debug, Eq, PartialEq, Multihash)]
    #[mh(alloc_size = 128)]
    enum Code128 {
        #[mh(code = 0x12, hasher = multihash::Sha2_256)]
        Sha2_256,
    }

    let cid_default = Cid::new_v1(RAW, Code::Sha2_256.digest(b"foo"));
    let cid_128 = CidGeneric::<128>::new_v1(RAW, Code128::Sha2_256.digest(b"foo"));

    assert_eq!(
        a_function_that_takes_a_generic_cid(&cid_default),
        a_function_that_takes_a_generic_cid(&cid_128)
    );
}

#[test]
fn test_into_v1() {
    let cid = Cid::from_str("QmTPcW343HGMdoxarwvHHoPhkbo5GfNYjnZkyW5DBtpvLe").unwrap();
    let cid_v1 = cid.into_v1().unwrap();
    assert_eq!(cid_v1.version(), Version::V1);
    assert_eq!(
        cid_v1.to_string(),
        "bafybeiclbsxcvqpfliqcejqz5ghpvw4r7vktjkyk3ruvjvdmam5azct2v4"
    );

    let cid = Cid::from_str("bafyreibjo4xmgaevkgud7mbifn3dzp4v4lyaui4yvqp3f2bqwtxcjrdqg4").unwrap();
    let cid_v1 = cid.into_v1().unwrap();
    assert_eq!(cid_v1.version(), Version::V1);
    assert_eq!(cid_v1, cid);
}
