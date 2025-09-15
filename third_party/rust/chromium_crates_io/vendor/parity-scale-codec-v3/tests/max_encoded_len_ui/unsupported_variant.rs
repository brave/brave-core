use parity_scale_codec::{Encode, MaxEncodedLen};

#[derive(Encode)]
struct NotMel;

#[derive(Encode, MaxEncodedLen)]
enum UnsupportedVariant {
	NotMel(NotMel),
}

fn main() {}
