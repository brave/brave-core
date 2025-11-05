#[derive(::parity_scale_codec::Decode, ::parity_scale_codec::Encode)]
#[codec(crate = ::parity_scale_codec)]
enum T {
	A = 1,
	B,
}

#[derive(::parity_scale_codec::Decode, ::parity_scale_codec::Encode)]
#[codec(crate = ::parity_scale_codec)]
enum T2 {
	#[codec(index = 1)]
	A,
	B,
}

fn main() {}
