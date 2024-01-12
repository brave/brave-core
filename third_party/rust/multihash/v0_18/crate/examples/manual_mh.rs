use multihash::{Code, MultihashDigest};

/// prefix/multihash generating tool to aid when adding new tests
fn prefix_util() {
    use unsigned_varint::encode;
    // change these as needed
    let empty = Code::Sha2_256.wrap(&[]).unwrap().to_bytes();
    let hash = "7c8357577f51d4f0a8d393aa1aaafb28863d9421";

    // encode things
    let len = (hash.len() / 2) as u64; // always hex so len bytes is always half
    let mut buf = encode::u64_buffer();
    let len = encode::u64(len, &mut buf);

    let code_hex = hex::encode(&empty[..1]); // change if longer/shorter prefix
    let len_hex = hex::encode(len);
    println!("prefix hex: code: {}, len: {}", code_hex, len_hex);

    println!("{}{}{}", code_hex, len_hex, hash);
}

fn main() {
    prefix_util()
}
