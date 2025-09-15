use parity_scale_codec::Encode;
use parity_scale_codec_derive::Encode as DeriveEncode;

#[test]
fn size_hint_for_struct() {
	#[derive(DeriveEncode)]
	struct Struct<A, B, C> {
		pub a: A,
		pub b: B,
		#[codec(skip)]
		pub c: C,
	}

	let v = Struct::<String, Vec<i32>, u32> { a: String::from("foo"), b: vec![1, 2, 3], c: 0 };
	assert_eq!(v.size_hint(), 23);
}

#[test]
fn size_hint_for_tuple_struct() {
	#[derive(DeriveEncode)]
	struct Tuple(String, Vec<i32>, #[codec(skip)] u32);

	let v = Tuple(String::from("foo"), vec![1, 2, 3], 0);
	assert_eq!(v.size_hint(), 23);
}

#[test]
fn size_hint_for_unit_struct() {
	#[derive(DeriveEncode)]
	struct Unit;

	let v = Unit;
	assert_eq!(v.size_hint(), 0);
}

#[test]
fn size_hint_for_simple_enum() {
	#[derive(DeriveEncode)]
	enum EnumType {
		#[codec(index = 15)]
		A,
		B(u32, u64),
		C {
			a: u32,
			b: u64,
		},
	}

	let v = EnumType::A;
	assert_eq!(v.size_hint(), 1);

	let v = EnumType::B(1, 2);
	assert_eq!(v.size_hint(), 13);

	let v = EnumType::C { a: 0, b: 0 };
	assert_eq!(v.size_hint(), 13);
}

#[test]
fn size_hint_for_enum_with_discriminant() {
	#[derive(DeriveEncode)]
	enum EnumWithDiscriminant {
		A = 1,
		B = 15,
		C = 255,
	}

	let discriminant = core::mem::size_of::<u8>();

	let v = EnumWithDiscriminant::A;
	assert_eq!(v.size_hint(), discriminant);

	let v = EnumWithDiscriminant::B;
	assert_eq!(v.size_hint(), discriminant);

	let v = EnumWithDiscriminant::C;
	assert_eq!(v.size_hint(), discriminant);
}
