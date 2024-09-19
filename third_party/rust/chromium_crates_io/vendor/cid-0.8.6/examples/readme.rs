use cid::multihash::{Code, MultihashDigest};
use cid::Cid;
use std::convert::TryFrom;

const RAW: u64 = 0x55;

fn main() {
    let h = Code::Sha2_256.digest(b"beep boop");

    let cid = Cid::new_v1(RAW, h);

    let data = cid.to_bytes();
    let out = Cid::try_from(data).unwrap();

    assert_eq!(cid, out);

    let cid_string = cid.to_string();
    assert_eq!(
        cid_string,
        "bafkreieq5jui4j25lacwomsqgjeswwl3y5zcdrresptwgmfylxo2depppq"
    );
    println!("{}", cid_string);
}
