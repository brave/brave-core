use parity_scale_codec::{Decode, Encode};
use parity_scale_codec_derive::{
	Decode as DeriveDecode, DecodeWithMemTracking as DeriveDecodeWithMemTracking,
	Encode as DeriveEncode,
};

#[test]
fn enum_struct_test() {
	#[derive(PartialEq, Debug, Default)]
	struct UncodecType;

	#[derive(PartialEq, Debug)]
	struct UncodecUndefaultType;

	use parity_scale_codec_derive::{
		Decode as DeriveDecode, DecodeWithMemTracking as DeriveDecodeWithMemTracking,
		Encode as DeriveEncode,
	};
	#[derive(PartialEq, Debug, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
	enum Enum<T = UncodecType, S = UncodecUndefaultType> {
		#[codec(skip)]
		A(S),
		B {
			#[codec(skip)]
			_b1: T,
			b2: u32,
		},
		C(#[codec(skip)] T, u32),
	}

	#[derive(PartialEq, Debug, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
	struct StructNamed<T = UncodecType> {
		#[codec(skip)]
		a: T,
		b: u32,
	}

	#[derive(PartialEq, Debug, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
	struct StructUnnamed<T = UncodecType>(#[codec(skip)] T, u32);

	let ea: Enum = Enum::A(UncodecUndefaultType);
	let eb: Enum = Enum::B { _b1: UncodecType, b2: 1 };
	let ec: Enum = Enum::C(UncodecType, 1);
	let sn = StructNamed { a: UncodecType, b: 1 };
	let su = StructUnnamed(UncodecType, 1);

	assert_eq!(ea.encode(), Vec::new());

	let mut eb_encoded: &[u8] = &eb.encode();
	let mut ec_encoded: &[u8] = &ec.encode();
	let mut sn_encoded: &[u8] = &sn.encode();
	let mut su_encoded: &[u8] = &su.encode();

	assert_eq!(Enum::decode(&mut eb_encoded).unwrap(), eb);
	assert_eq!(Enum::decode(&mut ec_encoded).unwrap(), ec);
	assert_eq!(StructNamed::decode(&mut sn_encoded).unwrap(), sn);
	assert_eq!(StructUnnamed::decode(&mut su_encoded).unwrap(), su);
}

#[test]
fn skip_enum_struct_inner_variant() {
	// Make sure the skipping does not generates a warning.
	#![deny(warnings)]

	#[derive(DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
	enum Enum {
		Data {
			some_named: u32,
			#[codec(skip)]
			ignore: Option<u32>,
		},
	}

	let encoded = Enum::Data { some_named: 1, ignore: Some(1) }.encode();
	assert_eq!(vec![0, 1, 0, 0, 0], encoded);
}
