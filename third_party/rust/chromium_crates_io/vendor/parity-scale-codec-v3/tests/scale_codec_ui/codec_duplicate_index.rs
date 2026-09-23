#[derive(::parity_scale_codec::Decode, ::parity_scale_codec::Encode)]
#[codec(crate = ::parity_scale_codec)]
enum T {
	A = 3,
	#[codec(index = 3)]
	B,
}

#[derive(::parity_scale_codec::Decode, ::parity_scale_codec::Encode)]
#[codec(crate = ::parity_scale_codec)]
enum T1 {
	A,
	#[codec(index = 0)]
	B,
}

fn main() {}
