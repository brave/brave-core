use parity_scale_codec::{Compact, Decode, DecodeWithMemTracking, Encode, HasCompact};
use parity_scale_codec_derive::{
	CompactAs as DeriveCompactAs, Decode as DeriveDecode,
	DecodeWithMemTracking as DeriveDecodeWithMemTracking, Encode as DeriveEncode,
};
use serde_derive::{Deserialize, Serialize};

#[derive(Debug, PartialEq, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
struct S {
	x: u32,
}

#[cfg_attr(feature = "std", derive(Serialize, Deserialize))]
#[derive(
	Debug,
	PartialEq,
	Eq,
	Clone,
	Copy,
	DeriveEncode,
	DeriveDecode,
	DeriveDecodeWithMemTracking,
	DeriveCompactAs,
)]
struct SSkip {
	#[codec(skip)]
	s1: u32,
	x: u32,
	#[codec(skip)]
	s2: u32,
}

#[derive(Debug, PartialEq, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
struct Sc {
	#[codec(compact)]
	x: u32,
}

#[derive(Debug, PartialEq, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
struct Sh<T: HasCompact>
where
	<T as HasCompact>::Type: DecodeWithMemTracking,
{
	#[codec(encoded_as = "<T as HasCompact>::Type")]
	x: T,
}

#[cfg_attr(feature = "std", derive(Serialize, Deserialize))]
#[derive(
	Debug,
	PartialEq,
	Eq,
	Clone,
	Copy,
	DeriveEncode,
	DeriveDecode,
	DeriveDecodeWithMemTracking,
	DeriveCompactAs,
)]
struct U(u32);

#[cfg_attr(feature = "std", derive(Serialize, Deserialize))]
#[derive(
	Debug,
	PartialEq,
	Eq,
	Clone,
	Copy,
	DeriveEncode,
	DeriveDecode,
	DeriveDecodeWithMemTracking,
	DeriveCompactAs,
)]
struct U2 {
	a: u64,
}

#[cfg_attr(feature = "std", derive(Serialize, Deserialize))]
#[derive(
	Debug,
	PartialEq,
	Eq,
	Clone,
	Copy,
	DeriveEncode,
	DeriveDecode,
	DeriveDecodeWithMemTracking,
	DeriveCompactAs,
)]
struct USkip(#[codec(skip)] u32, u32, #[codec(skip)] u32);

#[derive(Debug, PartialEq, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
struct Uc(#[codec(compact)] u32);

#[derive(Debug, PartialEq, Clone, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
struct Ucas(#[codec(compact)] U);

#[derive(Debug, PartialEq, Clone, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
struct USkipcas(#[codec(compact)] USkip);

#[derive(Debug, PartialEq, Clone, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
struct SSkipcas(#[codec(compact)] SSkip);

#[derive(Debug, PartialEq, DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking)]
struct Uh<T: HasCompact>(#[codec(encoded_as = "<T as HasCompact>::Type")] T)
where
	<T as HasCompact>::Type: DecodeWithMemTracking;

#[test]
fn test_encoding() {
	let x = 3u32;
	let s = S { x };
	let s_skip = SSkip { x, s1: Default::default(), s2: Default::default() };
	let sc = Sc { x };
	let sh = Sh { x };
	let u = U(x);
	let u_skip = USkip(Default::default(), x, Default::default());
	let uc = Uc(x);
	let ucom = Compact(u);
	let ucas = Ucas(u);
	let u_skip_cas = USkipcas(u_skip);
	let s_skip_cas = SSkipcas(s_skip);
	let uh = Uh(x);

	let mut s_encoded: &[u8] = &[3, 0, 0, 0];
	let mut s_skip_encoded: &[u8] = &[3, 0, 0, 0];
	let mut sc_encoded: &[u8] = &[12];
	let mut sh_encoded: &[u8] = &[12];
	let mut u_encoded: &[u8] = &[3, 0, 0, 0];
	let mut u_skip_encoded: &[u8] = &[3, 0, 0, 0];
	let mut uc_encoded: &[u8] = &[12];
	let mut ucom_encoded: &[u8] = &[12];
	let mut ucas_encoded: &[u8] = &[12];
	let mut u_skip_cas_encoded: &[u8] = &[12];
	let mut s_skip_cas_encoded: &[u8] = &[12];
	let mut uh_encoded: &[u8] = &[12];

	assert_eq!(s.encode(), s_encoded);
	assert_eq!(s_skip.encode(), s_skip_encoded);
	assert_eq!(sc.encode(), sc_encoded);
	assert_eq!(sh.encode(), sh_encoded);
	assert_eq!(u.encode(), u_encoded);
	assert_eq!(u_skip.encode(), u_skip_encoded);
	assert_eq!(uc.encode(), uc_encoded);
	assert_eq!(ucom.encode(), ucom_encoded);
	assert_eq!(ucas.encode(), ucas_encoded);
	assert_eq!(u_skip_cas.encode(), u_skip_cas_encoded);
	assert_eq!(s_skip_cas.encode(), s_skip_cas_encoded);
	assert_eq!(uh.encode(), uh_encoded);

	assert_eq!(s, S::decode(&mut s_encoded).unwrap());
	assert_eq!(s_skip, SSkip::decode(&mut s_skip_encoded).unwrap());
	assert_eq!(sc, Sc::decode(&mut sc_encoded).unwrap());
	assert_eq!(sh, Sh::decode(&mut sh_encoded).unwrap());
	assert_eq!(u, U::decode(&mut u_encoded).unwrap());
	assert_eq!(u_skip, USkip::decode(&mut u_skip_encoded).unwrap());
	assert_eq!(uc, Uc::decode(&mut uc_encoded).unwrap());
	assert_eq!(ucom, <Compact::<U>>::decode(&mut ucom_encoded).unwrap());
	assert_eq!(ucas, Ucas::decode(&mut ucas_encoded).unwrap());
	assert_eq!(u_skip_cas, USkipcas::decode(&mut u_skip_cas_encoded).unwrap());
	assert_eq!(s_skip_cas, SSkipcas::decode(&mut s_skip_cas_encoded).unwrap());
	assert_eq!(uh, Uh::decode(&mut uh_encoded).unwrap());
}
