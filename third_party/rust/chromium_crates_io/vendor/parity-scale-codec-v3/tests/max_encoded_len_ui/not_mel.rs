use parity_scale_codec::{Encode, MaxEncodedLen};

#[derive(Encode)]
struct NotMel;

#[derive(Encode, MaxEncodedLen)]
struct Generic<T> {
	t: T,
}

fn main() {
	let _ = Generic::<NotMel>::max_encoded_len();
}
