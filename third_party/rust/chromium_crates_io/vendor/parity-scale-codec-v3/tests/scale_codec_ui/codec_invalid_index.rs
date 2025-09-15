#[derive(::parity_scale_codec::Decode, ::parity_scale_codec::Encode)]
#[codec(crate = ::parity_scale_codec)]
enum T {
	A = 3,
	#[codec(index = 524)]
	B,
}

fn main() {}
