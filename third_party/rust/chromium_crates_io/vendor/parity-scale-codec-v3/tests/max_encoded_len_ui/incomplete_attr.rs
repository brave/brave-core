use parity_scale_codec::{Encode, MaxEncodedLen};

#[derive(Encode, MaxEncodedLen)]
#[codec(crate)]
struct Example;

fn main() {
	let _ = Example::max_encoded_len();
}
