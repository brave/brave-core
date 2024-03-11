// Copyright 2020 Parity Technologies
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

use core::{convert::TryInto, str::FromStr, u64::MAX};
use crunchy::unroll;
use uint::{construct_uint, overflowing, FromDecStrErr};

construct_uint! {
	pub struct U256(4);
}

construct_uint! {
	pub struct U512(8);
}

#[cfg(feature = "std")]
#[test]
fn hash_impl_is_the_same_as_for_a_slice() {
	use core::hash::{Hash, Hasher as _};
	use std::collections::hash_map::DefaultHasher;

	let uint_hash = {
		let mut h = DefaultHasher::new();
		let uint = U256::from(123u64);
		Hash::hash(&uint, &mut h);
		h.finish()
	};
	let slice_hash = {
		let mut h = DefaultHasher::new();
		Hash::hash(&[123u64, 0, 0, 0], &mut h);
		h.finish()
	};
	assert_eq!(uint_hash, slice_hash);
}

// https://github.com/paritytech/parity-common/issues/420
#[test]
fn const_matching_works() {
	const ONE: U256 = U256([1, 0, 0, 0]);
	match U256::zero() {
		ONE => unreachable!(),
		_ => {},
	}
}

#[test]
fn max() {
	let max = U256::MAX;
	assert_eq!(max.0, [0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF]);

	let max = U512::MAX;
	assert_eq!(
		max.0,
		[
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF
		]
	);
}

#[test]
fn one() {
	let one = U256::one();
	assert_eq!(one.0, [1, 0, 0, 0]);

	let one = U512::one();
	assert_eq!(one.0, [1, 0, 0, 0, 0, 0, 0, 0]);

	let any = U256::from(123456789);
	assert_eq!(any * U256::one(), any);

	let any = U512::from(123456789);
	assert_eq!(any * U512::one(), any);
}

#[test]
#[allow(deprecated)]
fn max_value() {
	let max = U256::max_value();
	assert_eq!(max.0, [0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF]);

	let max = U512::max_value();
	assert_eq!(
		max.0,
		[
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF,
			0xFFFFFFFFFFFFFFFF
		]
	);
}

#[test]
fn u128_conversions() {
	let mut a = U256::from(u128::max_value());
	assert_eq!(a.low_u128(), u128::max_value());
	a += 2u128.into();
	assert_eq!(a.low_u128(), 1u128);
	a -= 3u128.into();
	assert_eq!(a.low_u128(), u128::max_value() - 1);
}

#[test]
fn uint256_checked_ops() {
	let z = U256::from(0);
	let a = U256::from(10);
	let b = !U256::from(1);

	assert_eq!(U256::from(10).checked_pow(U256::from(0)), Some(U256::from(1)));
	assert_eq!(U256::from(10).checked_pow(U256::from(1)), Some(U256::from(10)));
	assert_eq!(U256::from(10).checked_pow(U256::from(2)), Some(U256::from(100)));
	assert_eq!(U256::from(10).checked_pow(U256::from(3)), Some(U256::from(1000)));
	assert_eq!(U256::from(10).checked_pow(U256::from(20)), Some(U256::exp10(20)));
	assert_eq!(U256::from(2).checked_pow(U256::from(0x100)), None);
	assert_eq!(U256::MAX.checked_pow(U256::from(2)), None);

	assert_eq!(a.checked_add(b), None);
	assert_eq!(a.checked_add(a), Some(20.into()));

	assert_eq!(a.checked_sub(b), None);
	assert_eq!(a.checked_sub(a), Some(0.into()));

	assert_eq!(a.checked_mul(b), None);
	assert_eq!(a.checked_mul(a), Some(100.into()));

	assert_eq!(a.checked_div(z), None);
	assert_eq!(a.checked_div(a), Some(1.into()));

	assert_eq!(a.checked_rem(z), None);
	assert_eq!(a.checked_rem(a), Some(0.into()));

	assert_eq!(a.checked_neg(), None);
	assert_eq!(z.checked_neg(), Some(z));
}

#[test]
fn uint256_abs_diff() {
	let zero = U256::zero();
	let max = U256::MAX;

	assert_eq!(zero.abs_diff(zero), zero);
	assert_eq!(max.abs_diff(max), zero);
	assert_eq!(zero.abs_diff(max), max);
	assert_eq!(max.abs_diff(zero), max);
}

#[test]
fn uint256_from() {
	let e = U256([10, 0, 0, 0]);

	// test unsigned initialization
	let ua = U256::from(10u8);
	let ub = U256::from(10u16);
	let uc = U256::from(10u32);
	let ud = U256::from(10u64);
	assert_eq!(e, ua);
	assert_eq!(e, ub);
	assert_eq!(e, uc);
	assert_eq!(e, ud);

	// test initialization from bytes
	let va = U256::from(&[10u8][..]);
	assert_eq!(e, va);

	// more tests for initialization from bytes
	assert_eq!(U256([0x1010, 0, 0, 0]), U256::from(&[0x10u8, 0x10][..]));
	assert_eq!(U256([0x12f0, 0, 0, 0]), U256::from(&[0x12u8, 0xf0][..]));
	assert_eq!(U256([0x12f0, 0, 0, 0]), U256::from(&[0, 0x12u8, 0xf0][..]));
	assert_eq!(U256([0x12f0, 0, 0, 0]), U256::from(&[0, 0, 0, 0, 0, 0, 0, 0x12u8, 0xf0][..]));
	assert_eq!(U256([0x12f0, 1, 0, 0]), U256::from(&[1, 0, 0, 0, 0, 0, 0, 0x12u8, 0xf0][..]));
	assert_eq!(
		U256([0x12f0, 1, 0x0910203040506077, 0x8090a0b0c0d0e0f0]),
		U256::from(
			&[
				0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x09, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x77, 0, 0,
				0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0x12u8, 0xf0
			][..]
		)
	);
	assert_eq!(
		U256([0x00192437100019fa, 0x243710, 0, 0]),
		U256::from(&[0x24u8, 0x37, 0x10, 0, 0x19, 0x24, 0x37, 0x10, 0, 0x19, 0xfa][..])
	);

	// test initializtion from string
	let sa = U256::from_str("0a").unwrap();
	let sa2 = U256::from_str("0x0a").unwrap();
	assert_eq!(sa2, sa);
	assert_eq!(e, sa);
	assert_eq!(U256([0, 0, 0, 0]), U256::from_str("").unwrap());
	assert_eq!(U256([0x1, 0, 0, 0]), U256::from_str("1").unwrap());
	assert_eq!(U256([0x101, 0, 0, 0]), U256::from_str("101").unwrap());
	assert_eq!(U256([0x1010, 0, 0, 0]), U256::from_str("1010").unwrap());
	assert_eq!(U256([0x12f0, 0, 0, 0]), U256::from_str("12f0").unwrap());
	assert_eq!(U256([0x12f0, 0, 0, 0]), U256::from_str("0000000012f0").unwrap());
	assert_eq!(U256([0x12f0, 1, 0, 0]), U256::from_str("0100000000000012f0").unwrap());
	assert_eq!(
		U256([0x12f0, 1, 0x0910203040506077, 0x8090a0b0c0d0e0f0]),
		U256::from_str("8090a0b0c0d0e0f00910203040506077000000000000000100000000000012f0").unwrap()
	);

	// This string contains more bits than what fits in a U256.
	assert!(U256::from_str("000000000000000000000000000000000000000000000000000000000000000000").is_err());
	assert!(U256::from_str("100000000000000000000000000000000000000000000000000000000000000000").is_err());
}

#[test]
fn uint256_try_into_primitives() {
	macro_rules! try_into_uint_primitive_ok {
		($primitive: ty) => {
			assert_eq!(U256::from(10).try_into() as Result<$primitive, _>, Ok(<$primitive>::from(10u8)));
		};
	}
	try_into_uint_primitive_ok!(u8);
	try_into_uint_primitive_ok!(u16);
	try_into_uint_primitive_ok!(u32);
	try_into_uint_primitive_ok!(usize);
	try_into_uint_primitive_ok!(u64);
	try_into_uint_primitive_ok!(u128);

	macro_rules! try_into_iint_primitive_ok {
		($primitive: ty) => {
			assert_eq!(U256::from(10).try_into() as Result<$primitive, _>, Ok(<$primitive>::from(10i8)));
		};
	}
	try_into_iint_primitive_ok!(i8);
	try_into_iint_primitive_ok!(i16);
	try_into_iint_primitive_ok!(i32);
	try_into_iint_primitive_ok!(isize);
	try_into_iint_primitive_ok!(i64);
	try_into_iint_primitive_ok!(i128);

	macro_rules! try_into_primitive_err {
		($small: ty, $big: ty) => {
			assert_eq!(
				U256::from(<$small>::max_value() as $big + 1).try_into() as Result<$small, _>,
				Err(concat!("integer overflow when casting to ", stringify!($small)))
			);
		};
	}
	try_into_primitive_err!(u8, u16);
	try_into_primitive_err!(u16, u32);
	try_into_primitive_err!(u32, u64);
	try_into_primitive_err!(usize, u128);
	try_into_primitive_err!(u64, u128);
	assert_eq!(U256([0, 0, 1, 0]).try_into() as Result<u128, _>, Err("integer overflow when casting to u128"));
	try_into_primitive_err!(i8, i16);
	try_into_primitive_err!(i16, i32);
	try_into_primitive_err!(i32, i64);
	try_into_primitive_err!(isize, i128);
	try_into_primitive_err!(i64, i128);
	try_into_primitive_err!(i128, u128);
	assert_eq!(U256([0, 0, 1, 0]).try_into() as Result<i128, _>, Err("integer overflow when casting to i128"));
}

#[test]
fn uint256_to() {
	let hex = "8090a0b0c0d0e0f00910203040506077583a2cf8264910e1436bda32571012f0";
	let uint = U256::from_str(hex).unwrap();
	let mut bytes = [0u8; 32];
	uint.to_big_endian(&mut bytes);
	let uint2 = U256::from(&bytes[..]);
	assert_eq!(uint, uint2);
}

#[test]
fn uint256_bits_test() {
	assert_eq!(U256::from(0u64).bits(), 0);
	assert_eq!(U256::from(255u64).bits(), 8);
	assert_eq!(U256::from(256u64).bits(), 9);
	assert_eq!(U256::from(300u64).bits(), 9);
	assert_eq!(U256::from(60000u64).bits(), 16);
	assert_eq!(U256::from(70000u64).bits(), 17);

	//// Try to read the following lines out loud quickly
	let mut shl = U256::from(70000u64);
	shl = shl << 100;
	assert_eq!(shl.bits(), 117);
	shl = shl << 100;
	assert_eq!(shl.bits(), 217);
	shl = shl << 100;
	assert_eq!(shl.bits(), 0);

	//// Bit set check
	//// 01010
	assert!(!U256::from(10u8).bit(0));
	assert!(U256::from(10u8).bit(1));
	assert!(!U256::from(10u8).bit(2));
	assert!(U256::from(10u8).bit(3));
	assert!(!U256::from(10u8).bit(4));

	//// byte check
	assert_eq!(U256::from(10u8).byte(0), 10);
	assert_eq!(U256::from(0xffu64).byte(0), 0xff);
	assert_eq!(U256::from(0xffu64).byte(1), 0);
	assert_eq!(U256::from(0x01ffu64).byte(0), 0xff);
	assert_eq!(U256::from(0x01ffu64).byte(1), 0x1);
	assert_eq!(U256([0u64, 0xfc, 0, 0]).byte(8), 0xfc);
	assert_eq!(U256([0u64, 0, 0, u64::max_value()]).byte(31), 0xff);
	assert_eq!(U256([0u64, 0, 0, (u64::max_value() >> 8) + 1]).byte(31), 0x01);
}

#[test]
fn uint256_comp_test() {
	let small = U256([10u64, 0, 0, 0]);
	let big = U256([0x8C8C3EE70C644118u64, 0x0209E7378231E632, 0, 0]);
	let bigger = U256([0x9C8C3EE70C644118u64, 0x0209E7378231E632, 0, 0]);
	let biggest = U256([0x5C8C3EE70C644118u64, 0x0209E7378231E632, 0, 1]);

	assert!(small < big);
	assert!(big < bigger);
	assert!(bigger < biggest);
	assert!(bigger <= biggest);
	assert!(biggest <= biggest);
	assert!(bigger >= big);
	assert!(bigger >= small);
	assert!(small <= small);
	assert_eq!(small, small);
	assert_eq!(biggest, biggest);
	assert_ne!(big, biggest);
	assert_ne!(big, bigger);
}

#[test]
fn uint256_arithmetic_test() {
	let init = U256::from(0xDEADBEEFDEADBEEFu64);
	let copy = init;

	let add = init + copy;
	assert_eq!(add, U256([0xBD5B7DDFBD5B7DDEu64, 1, 0, 0]));
	// Bitshifts
	let shl = add << 88;
	assert_eq!(shl, U256([0u64, 0xDFBD5B7DDE000000, 0x1BD5B7D, 0]));
	let shr = shl >> 40;
	assert_eq!(shr, U256([0x7DDE000000000000u64, 0x0001BD5B7DDFBD5B, 0, 0]));
	// Increment
	let incr = shr + U256::from(1u64);
	assert_eq!(incr, U256([0x7DDE000000000001u64, 0x0001BD5B7DDFBD5B, 0, 0]));
	// Subtraction
	let sub = overflowing!(incr.overflowing_sub(init));
	assert_eq!(sub, U256([0x9F30411021524112u64, 0x0001BD5B7DDFBD5A, 0, 0]));
	// Multiplication
	let mult = sub * 300u32;
	assert_eq!(mult, U256([0x8C8C3EE70C644118u64, 0x0209E7378231E632, 0, 0]));
	// Division
	assert_eq!(U256::from(105u8) / U256::from(5u8), U256::from(21u8));
	let div = mult / U256::from(300u16);
	assert_eq!(div, U256([0x9F30411021524112u64, 0x0001BD5B7DDFBD5A, 0, 0]));

	let a = U256::from_str("ff000000000000000000000000000000000000000000000000000000000000d1").unwrap();
	let b = U256::from_str("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff2e").unwrap();
	println!("{:x}", a);
	println!("{:x}", b);
	assert_eq!(!a, b);
	assert_eq!(a, !b);
}

#[test]
fn uint256_simple_mul() {
	let a = U256::from_str("10000000000000000").unwrap();
	let b = U256::from_str("10000000000000000").unwrap();

	let c = U256::from_str("100000000000000000000000000000000").unwrap();
	println!("Multiplying");
	let result = a.overflowing_mul(b);
	println!("Got result");
	assert_eq!(result, (c, false))
}

#[test]
fn uint256_extreme_bitshift_test() {
	//// Shifting a u64 by 64 bits gives an undefined value, so make sure that
	//// we're doing the Right Thing here
	let init = U256::from(0xDEADBEEFDEADBEEFu64);

	assert_eq!(init << 64, U256([0, 0xDEADBEEFDEADBEEF, 0, 0]));
	let add = (init << 64) + init;
	assert_eq!(add, U256([0xDEADBEEFDEADBEEF, 0xDEADBEEFDEADBEEF, 0, 0]));
	assert_eq!(add >> 0, U256([0xDEADBEEFDEADBEEF, 0xDEADBEEFDEADBEEF, 0, 0]));
	assert_eq!(add << 0, U256([0xDEADBEEFDEADBEEF, 0xDEADBEEFDEADBEEF, 0, 0]));
	assert_eq!(add >> 64, U256([0xDEADBEEFDEADBEEF, 0, 0, 0]));
	assert_eq!(add << 64, U256([0, 0xDEADBEEFDEADBEEF, 0xDEADBEEFDEADBEEF, 0]));
}

#[test]
fn uint256_exp10() {
	assert_eq!(U256::exp10(0), U256::from(1u64));
	println!("\none: {:?}", U256::from(1u64));
	println!("ten: {:?}", U256::from(10u64));
	assert_eq!(U256::from(2u64) * U256::from(10u64), U256::from(20u64));
	assert_eq!(U256::exp10(1), U256::from(10u64));
	assert_eq!(U256::exp10(2), U256::from(100u64));
	assert_eq!(U256::exp10(5), U256::from(100000u64));
}

#[test]
fn uint256_mul32() {
	assert_eq!(U256::from(0u64) * 2u32, U256::from(0u64));
	assert_eq!(U256::from(1u64) * 2u32, U256::from(2u64));
	assert_eq!(U256::from(10u64) * 2u32, U256::from(20u64));
	assert_eq!(U256::from(10u64) * 5u32, U256::from(50u64));
	assert_eq!(U256::from(1000u64) * 50u32, U256::from(50000u64));
}

#[test]
fn uint256_pow() {
	assert_eq!(U256::from(10).pow(U256::from(0)), U256::from(1));
	assert_eq!(U256::from(10).pow(U256::from(1)), U256::from(10));
	assert_eq!(U256::from(10).pow(U256::from(2)), U256::from(100));
	assert_eq!(U256::from(10).pow(U256::from(3)), U256::from(1000));
	assert_eq!(U256::from(10).pow(U256::from(20)), U256::exp10(20));
}

#[test]
#[should_panic]
fn uint256_pow_overflow_panic() {
	U256::from(2).pow(U256::from(0x100));
}

#[test]
fn should_format_and_debug_correctly() {
	let test = |x: usize, hex: &'static str, display: &'static str| {
		assert_eq!(format!("{}", U256::from(x)), display);
		// TODO: proper impl for Debug so we get this to pass:  assert_eq!(format!("{:?}", U256::from(x)), format!("0x{}", hex));
		assert_eq!(format!("{:?}", U256::from(x)), display);
		assert_eq!(format!("{:x}", U256::from(x)), hex);
		assert_eq!(format!("{:#x}", U256::from(x)), format!("0x{}", hex));
	};

	test(0x1, "1", "1");
	test(0xf, "f", "15");
	test(0x10, "10", "16");
	test(0xff, "ff", "255");
	test(0x100, "100", "256");
	test(0xfff, "fff", "4095");
	test(0x1000, "1000", "4096");
}

#[test]
pub fn display_u256() {
	let expected = "115792089237316195423570985008687907853269984665640564039457584007913129639935";
	let value = U256::MAX;
	assert_eq!(format!("{}", value), expected);
	assert_eq!(format!("{:?}", value), expected);
}

#[test]
pub fn display_u512() {
	let expected = "13407807929942597099574024998205846127479365820592393377723561443721764030073546976801874298166903427690031858186486050853753882811946569946433649006084095";
	let value = U512::MAX;
	assert_eq!(format!("{}", value), expected);
	assert_eq!(format!("{:?}", value), expected);
}

#[test]
fn uint256_overflowing_pow() {
	assert_eq!(
		U256::from(2).overflowing_pow(U256::from(0xff)),
		(U256::from_str("8000000000000000000000000000000000000000000000000000000000000000").unwrap(), false)
	);
	assert_eq!(U256::from(2).overflowing_pow(U256::from(0x100)), (U256::zero(), true));
}

#[test]
fn uint256_mul1() {
	assert_eq!(U256::from(1u64) * U256::from(10u64), U256::from(10u64));
}

#[test]
fn uint256_mul2() {
	let a = U512::from_str("10000000000000000fffffffffffffffe").unwrap();
	let b = U512::from_str("ffffffffffffffffffffffffffffffff").unwrap();

	assert_eq!(a * b, U512::from_str("10000000000000000fffffffffffffffcffffffffffffffff0000000000000002").unwrap());
}

#[test]
fn uint256_overflowing_mul() {
	assert_eq!(
		U256::from_str("100000000000000000000000000000000")
			.unwrap()
			.overflowing_mul(U256::from_str("100000000000000000000000000000000").unwrap()),
		(U256::zero(), true)
	);
}

#[test]
fn uint512_mul() {
	assert_eq!(
		U512::from_str("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").unwrap()
		*
		U512::from_str("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").unwrap(),
		U512::from_str("3fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000000000000000000000000000000000000000000000000001").unwrap()
	);
}

#[test]
fn uint256_mul_overflow() {
	assert_eq!(
		U256::from_str("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff")
			.unwrap()
			.overflowing_mul(
				U256::from_str("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").unwrap()
			),
		(U256::from_str("1").unwrap(), true)
	);
}

#[test]
#[should_panic]
#[allow(unused_must_use)]
fn uint256_mul_overflow_panic() {
	U256::from_str("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").unwrap() *
		U256::from_str("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").unwrap();
}

#[test]
fn uint256_sub_overflow() {
	assert_eq!(
		U256::from_str("0").unwrap().overflowing_sub(U256::from_str("1").unwrap()),
		(U256::from_str("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").unwrap(), true)
	);
}

#[test]
fn uint256_neg_overflow() {
	assert_eq!(U256::from_str("0").unwrap().overflowing_neg(), (U256::from_str("0").unwrap(), false));
	assert_eq!(
		U256::from_str("1").unwrap().overflowing_neg(),
		(U256::from_str("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").unwrap(), true)
	);
	assert_eq!(
		U256::from_str("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff")
			.unwrap()
			.overflowing_neg(),
		(U256::from_str("1").unwrap(), true)
	);
	assert_eq!(
		U256::from_str("8000000000000000000000000000000000000000000000000000000000000000")
			.unwrap()
			.overflowing_neg(),
		(U256::from_str("8000000000000000000000000000000000000000000000000000000000000000").unwrap(), true)
	);
	assert_eq!(
		U256::from_str("ffffffffffffffff0000000000000000ffffffffffffffff0000000000000000")
			.unwrap()
			.overflowing_neg(),
		(U256::from_str("0000000000000000ffffffffffffffff00000000000000010000000000000000").unwrap(), true)
	);
	assert_eq!(
		U256::from_str("0000000000000000ffffffffffffffff0000000000000000ffffffffffffffff")
			.unwrap()
			.overflowing_neg(),
		(U256::from_str("ffffffffffffffff0000000000000000ffffffffffffffff0000000000000001").unwrap(), true)
	);
}

#[test]
#[should_panic]
#[allow(unused_must_use)]
fn uint256_sub_overflow_panic() {
	U256::from_str("0").unwrap() - U256::from_str("1").unwrap();
}

#[test]
fn uint256_shl() {
	assert_eq!(
		U256::from_str("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").unwrap() << 4,
		U256::from_str("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0").unwrap()
	);
}

#[test]
fn uint256_shl_words() {
	assert_eq!(
		U256::from_str("0000000000000001ffffffffffffffffffffffffffffffffffffffffffffffff").unwrap() << 64,
		U256::from_str("ffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000").unwrap()
	);
	assert_eq!(
		U256::from_str("0000000000000000ffffffffffffffffffffffffffffffffffffffffffffffff").unwrap() << 64,
		U256::from_str("ffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000").unwrap()
	);
}

#[test]
fn uint256_mul() {
	assert_eq!(
		U256::from_str("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff").unwrap() *
			U256::from_str("2").unwrap(),
		U256::from_str("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe").unwrap()
	);
}

#[test]
fn uint256_div() {
	assert_eq!(U256::from(10u64) / U256::from(1u64), U256::from(10u64));
	assert_eq!(U256::from(10u64) / U256::from(2u64), U256::from(5u64));
	assert_eq!(U256::from(10u64) / U256::from(3u64), U256::from(3u64));
}

#[test]
fn uint256_rem() {
	assert_eq!(U256::from(10u64) % U256::from(1u64), U256::from(0u64));
	assert_eq!(U256::from(10u64) % U256::from(3u64), U256::from(1u64));
}

#[test]
fn uint256_from_dec_str() {
	assert_eq!(U256::from_dec_str("10").unwrap(), U256::from(10u64));
	assert_eq!(U256::from_dec_str("1024").unwrap(), U256::from(1024u64));
	assert_eq!(
		U256::from_dec_str("115792089237316195423570985008687907853269984665640564039457584007913129639936"),
		Err(FromDecStrErr::InvalidLength)
	);
	assert_eq!(U256::from_dec_str("0x11"), Err(FromDecStrErr::InvalidCharacter));
}

#[test]
fn display_uint() {
	let s = U256::from_dec_str("12345678987654321023456789").unwrap();
	assert_eq!(format!("{}", s), "12345678987654321023456789");
	assert_eq!(format!("{:x}", s), "a364c995584f929f39615");
	assert_eq!(format!("{:X}", s), "A364C995584F929F39615");
	assert_eq!(format!("{:032}", s), "00000012345678987654321023456789");
	assert_eq!(format!("{:032x}", s), "00000000000a364c995584f929f39615");
	assert_eq!(format!("{:032X}", s), "00000000000A364C995584F929F39615");
	assert_eq!(format!("{:#032x}", s), "0x000000000a364c995584f929f39615");
	assert_eq!(format!("{:#032X}", s), "0x000000000A364C995584F929F39615");
}

#[test]
fn display_uint_zero() {
	let s = U256::from(0);
	assert_eq!(format!("{}", s), "0");
	assert_eq!(format!("{:x}", s), "0");
	assert_eq!(format!("{:X}", s), "0");
	assert_eq!(format!("{:032x}", s), "00000000000000000000000000000000");
	assert_eq!(format!("{:032X}", s), "00000000000000000000000000000000");
	assert_eq!(format!("{:#032x}", s), "0x000000000000000000000000000000");
	assert_eq!(format!("{:#032X}", s), "0x000000000000000000000000000000");
}

#[test]
fn u512_multi_adds() {
	let (result, _) = U512([0, 0, 0, 0, 0, 0, 0, 0]).overflowing_add(U512([0, 0, 0, 0, 0, 0, 0, 0]));
	assert_eq!(result, U512([0, 0, 0, 0, 0, 0, 0, 0]));

	let (result, _) = U512([1, 0, 0, 0, 0, 0, 0, 1]).overflowing_add(U512([1, 0, 0, 0, 0, 0, 0, 1]));
	assert_eq!(result, U512([2, 0, 0, 0, 0, 0, 0, 2]));

	let (result, _) = U512([0, 0, 0, 0, 0, 0, 0, 1]).overflowing_add(U512([0, 0, 0, 0, 0, 0, 0, 1]));
	assert_eq!(result, U512([0, 0, 0, 0, 0, 0, 0, 2]));

	let (result, _) = U512([0, 0, 0, 0, 0, 0, 2, 1]).overflowing_add(U512([0, 0, 0, 0, 0, 0, 3, 1]));
	assert_eq!(result, U512([0, 0, 0, 0, 0, 0, 5, 2]));

	let (result, _) = U512([1, 2, 3, 4, 5, 6, 7, 8]).overflowing_add(U512([9, 10, 11, 12, 13, 14, 15, 16]));
	assert_eq!(result, U512([10, 12, 14, 16, 18, 20, 22, 24]));

	let (_, overflow) = U512([0, 0, 0, 0, 0, 0, 2, 1]).overflowing_add(U512([0, 0, 0, 0, 0, 0, 3, 1]));
	assert!(!overflow);

	let (_, overflow) =
		U512([MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX]).overflowing_add(U512([MAX, MAX, MAX, MAX, MAX, MAX, MAX, MAX]));
	assert!(overflow);

	let (_, overflow) = U512([0, 0, 0, 0, 0, 0, 0, MAX]).overflowing_add(U512([0, 0, 0, 0, 0, 0, 0, MAX]));
	assert!(overflow);

	let (_, overflow) = U512([0, 0, 0, 0, 0, 0, 0, MAX]).overflowing_add(U512([0, 0, 0, 0, 0, 0, 0, 0]));
	assert!(!overflow);
}

#[test]
fn u256_multi_adds() {
	let (result, _) = U256([0, 0, 0, 0]).overflowing_add(U256([0, 0, 0, 0]));
	assert_eq!(result, U256([0, 0, 0, 0]));

	let (result, _) = U256([0, 0, 0, 1]).overflowing_add(U256([0, 0, 0, 1]));
	assert_eq!(result, U256([0, 0, 0, 2]));

	let (result, overflow) = U256([0, 0, 2, 1]).overflowing_add(U256([0, 0, 3, 1]));
	assert_eq!(result, U256([0, 0, 5, 2]));
	assert!(!overflow);

	let (_, overflow) = U256([MAX, MAX, MAX, MAX]).overflowing_add(U256([MAX, MAX, MAX, MAX]));
	assert!(overflow);

	let (_, overflow) = U256([0, 0, 0, MAX]).overflowing_add(U256([0, 0, 0, MAX]));
	assert!(overflow);
}

#[test]
fn u256_multi_subs() {
	let (result, _) = U256([0, 0, 0, 0]).overflowing_sub(U256([0, 0, 0, 0]));
	assert_eq!(result, U256([0, 0, 0, 0]));

	let (result, _) = U256([0, 0, 0, 1]).overflowing_sub(U256([0, 0, 0, 1]));
	assert_eq!(result, U256([0, 0, 0, 0]));

	let (_, overflow) = U256([0, 0, 2, 1]).overflowing_sub(U256([0, 0, 3, 1]));
	assert!(overflow);

	let (result, overflow) = U256([MAX, MAX, MAX, MAX]).overflowing_sub(U256([MAX / 2, MAX / 2, MAX / 2, MAX / 2]));

	assert!(!overflow);
	assert_eq!(U256([MAX / 2 + 1, MAX / 2 + 1, MAX / 2 + 1, MAX / 2 + 1]), result);

	let (result, overflow) = U256([0, 0, 0, 1]).overflowing_sub(U256([0, 0, 1, 0]));
	assert!(!overflow);
	assert_eq!(U256([0, 0, MAX, 0]), result);

	let (result, overflow) = U256([0, 0, 0, 1]).overflowing_sub(U256([1, 0, 0, 0]));
	assert!(!overflow);
	assert_eq!(U256([MAX, MAX, MAX, 0]), result);
}

#[test]
fn u512_multi_subs() {
	let (result, _) = U512([0, 0, 0, 0, 0, 0, 0, 0]).overflowing_sub(U512([0, 0, 0, 0, 0, 0, 0, 0]));
	assert_eq!(result, U512([0, 0, 0, 0, 0, 0, 0, 0]));

	let (result, _) = U512([10, 9, 8, 7, 6, 5, 4, 3]).overflowing_sub(U512([9, 8, 7, 6, 5, 4, 3, 2]));
	assert_eq!(result, U512([1, 1, 1, 1, 1, 1, 1, 1]));

	let (_, overflow) = U512([10, 9, 8, 7, 6, 5, 4, 3]).overflowing_sub(U512([9, 8, 7, 6, 5, 4, 3, 2]));
	assert!(!overflow);

	let (_, overflow) = U512([9, 8, 7, 6, 5, 4, 3, 2]).overflowing_sub(U512([10, 9, 8, 7, 6, 5, 4, 3]));
	assert!(overflow);
}

#[test]
fn u256_multi_carry_all() {
	let (result, _) = U256([MAX, 0, 0, 0]).overflowing_mul(U256([MAX, 0, 0, 0]));
	assert_eq!(U256([1, MAX - 1, 0, 0]), result);

	let (result, _) = U256([0, MAX, 0, 0]).overflowing_mul(U256([MAX, 0, 0, 0]));
	assert_eq!(U256([0, 1, MAX - 1, 0]), result);

	let (result, _) = U256([MAX, MAX, 0, 0]).overflowing_mul(U256([MAX, 0, 0, 0]));
	assert_eq!(U256([1, MAX, MAX - 1, 0]), result);

	let (result, _) = U256([MAX, 0, 0, 0]).overflowing_mul(U256([MAX, MAX, 0, 0]));
	assert_eq!(U256([1, MAX, MAX - 1, 0]), result);

	let (result, _) = U256([MAX, MAX, 0, 0]).overflowing_mul(U256([MAX, MAX, 0, 0]));
	assert_eq!(U256([1, 0, MAX - 1, MAX]), result);

	let (result, _) = U256([MAX, 0, 0, 0]).overflowing_mul(U256([MAX, MAX, MAX, 0]));
	assert_eq!(U256([1, MAX, MAX, MAX - 1]), result);

	let (result, _) = U256([MAX, MAX, MAX, 0]).overflowing_mul(U256([MAX, 0, 0, 0]));
	assert_eq!(U256([1, MAX, MAX, MAX - 1]), result);

	let (result, _) = U256([MAX, 0, 0, 0]).overflowing_mul(U256([MAX, MAX, MAX, MAX]));
	assert_eq!(U256([1, MAX, MAX, MAX]), result);

	let (result, _) = U256([MAX, MAX, MAX, MAX]).overflowing_mul(U256([MAX, 0, 0, 0]));
	assert_eq!(U256([1, MAX, MAX, MAX]), result);

	let (result, _) = U256([MAX, MAX, MAX, 0]).overflowing_mul(U256([MAX, MAX, 0, 0]));
	assert_eq!(U256([1, 0, MAX, MAX - 1]), result);

	let (result, _) = U256([MAX, MAX, 0, 0]).overflowing_mul(U256([MAX, MAX, MAX, 0]));
	assert_eq!(U256([1, 0, MAX, MAX - 1]), result);

	let (result, _) = U256([MAX, MAX, MAX, MAX]).overflowing_mul(U256([MAX, MAX, 0, 0]));
	assert_eq!(U256([1, 0, MAX, MAX]), result);

	let (result, _) = U256([MAX, MAX, 0, 0]).overflowing_mul(U256([MAX, MAX, MAX, MAX]));
	assert_eq!(U256([1, 0, MAX, MAX]), result);

	let (result, _) = U256([MAX, MAX, MAX, 0]).overflowing_mul(U256([MAX, MAX, MAX, 0]));
	assert_eq!(U256([1, 0, 0, MAX - 1]), result);

	let (result, _) = U256([MAX, MAX, MAX, 0]).overflowing_mul(U256([MAX, MAX, MAX, MAX]));
	assert_eq!(U256([1, 0, 0, MAX]), result);

	let (result, _) = U256([MAX, MAX, MAX, MAX]).overflowing_mul(U256([MAX, MAX, MAX, 0]));
	assert_eq!(U256([1, 0, 0, MAX]), result);

	let (result, _) = U256([0, 0, 0, MAX]).overflowing_mul(U256([0, 0, 0, MAX]));
	assert_eq!(U256([0, 0, 0, 0]), result);

	let (result, _) = U256([1, 0, 0, 0]).overflowing_mul(U256([0, 0, 0, MAX]));
	assert_eq!(U256([0, 0, 0, MAX]), result);

	let (result, _) = U256([MAX, MAX, MAX, MAX]).overflowing_mul(U256([MAX, MAX, MAX, MAX]));
	assert_eq!(U256([1, 0, 0, 0]), result);
}

#[test]
fn u256_multi_muls() {
	let (result, _) = U256([0, 0, 0, 0]).overflowing_mul(U256([0, 0, 0, 0]));
	assert_eq!(U256([0, 0, 0, 0]), result);

	let (result, _) = U256([1, 0, 0, 0]).overflowing_mul(U256([1, 0, 0, 0]));
	assert_eq!(U256([1, 0, 0, 0]), result);

	let (result, _) = U256([5, 0, 0, 0]).overflowing_mul(U256([5, 0, 0, 0]));
	assert_eq!(U256([25, 0, 0, 0]), result);

	let (result, _) = U256([0, 5, 0, 0]).overflowing_mul(U256([0, 5, 0, 0]));
	assert_eq!(U256([0, 0, 25, 0]), result);

	let (result, _) = U256([0, 0, 0, 1]).overflowing_mul(U256([1, 0, 0, 0]));
	assert_eq!(U256([0, 0, 0, 1]), result);

	let (result, _) = U256([0, 0, 0, 5]).overflowing_mul(U256([2, 0, 0, 0]));
	assert_eq!(U256([0, 0, 0, 10]), result);

	let (result, _) = U256([0, 0, 1, 0]).overflowing_mul(U256([0, 5, 0, 0]));
	assert_eq!(U256([0, 0, 0, 5]), result);

	let (result, _) = U256([0, 0, 8, 0]).overflowing_mul(U256([0, 0, 7, 0]));
	assert_eq!(U256([0, 0, 0, 0]), result);

	let (result, _) = U256([2, 0, 0, 0]).overflowing_mul(U256([0, 5, 0, 0]));
	assert_eq!(U256([0, 10, 0, 0]), result);

	let (result, _) = U256([1, 0, 0, 0]).overflowing_mul(U256([0, 0, 0, MAX]));
	assert_eq!(U256([0, 0, 0, MAX]), result);
}

#[test]
fn u256_multi_muls_overflow() {
	let (_, overflow) = U256([1, 0, 0, 0]).overflowing_mul(U256([0, 0, 0, 0]));
	assert!(!overflow);

	let (_, overflow) = U256([1, 0, 0, 0]).overflowing_mul(U256([0, 0, 0, MAX]));
	assert!(!overflow);

	let (_, overflow) = U256([0, 1, 0, 0]).overflowing_mul(U256([0, 0, 0, MAX]));
	assert!(overflow);

	let (_, overflow) = U256([0, 1, 0, 0]).overflowing_mul(U256([0, 1, 0, 0]));
	assert!(!overflow);

	let (_, overflow) = U256([0, 1, 0, MAX]).overflowing_mul(U256([0, 1, 0, MAX]));
	assert!(overflow);

	let (_, overflow) = U256([0, MAX, 0, 0]).overflowing_mul(U256([0, MAX, 0, 0]));
	assert!(!overflow);

	let (_, overflow) = U256([1, 0, 0, 0]).overflowing_mul(U256([10, 0, 0, 0]));
	assert!(!overflow);

	let (_, overflow) = U256([2, 0, 0, 0]).overflowing_mul(U256([0, 0, 0, MAX / 2]));
	assert!(!overflow);

	let (_, overflow) = U256([0, 0, 8, 0]).overflowing_mul(U256([0, 0, 7, 0]));
	assert!(overflow);
}

#[test]
fn u512_div() {
	let fuzz_data = [
		0x38, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0x7, 0x0, 0x0, 0x0, 0x0, 0xc1,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xfe, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	];
	let a = U512::from_little_endian(&fuzz_data[..64]);
	let b = U512::from_little_endian(&fuzz_data[64..]);
	let (x, y) = (a / b, a % b);
	let (q, r) = a.div_mod(b);
	assert_eq!((x, y), (q, r));
}

#[test]
fn big_endian() {
	let source = U256([1, 0, 0, 0]);
	let mut target = vec![0u8; 32];

	assert_eq!(source, U256::from(1));

	source.to_big_endian(&mut target);
	assert_eq!(
		vec![
			0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8,
			0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 1u8
		],
		target
	);

	let source = U256([512, 0, 0, 0]);
	let mut target = vec![0u8; 32];

	source.to_big_endian(&mut target);
	assert_eq!(
		vec![
			0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8,
			0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 2u8, 0u8
		],
		target
	);

	let source = U256([0, 512, 0, 0]);
	let mut target = vec![0u8; 32];

	source.to_big_endian(&mut target);
	assert_eq!(
		vec![
			0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8,
			0u8, 2u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8, 0u8
		],
		target
	);

	let source = U256::from_str("0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f20").unwrap();
	source.to_big_endian(&mut target);
	assert_eq!(
		vec![
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
			0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
		],
		target
	);
}

#[test]
fn u256_multi_muls2() {
	let (result, _) = U256([0, 0, 0, 0]).overflowing_mul(U256([0, 0, 0, 0]));
	assert_eq!(U256([0, 0, 0, 0]), result);

	let (result, _) = U256([1, 0, 0, 0]).overflowing_mul(U256([1, 0, 0, 0]));
	assert_eq!(U256([1, 0, 0, 0]), result);

	let (result, _) = U256([5, 0, 0, 0]).overflowing_mul(U256([5, 0, 0, 0]));
	assert_eq!(U256([25, 0, 0, 0]), result);

	let (result, _) = U256([0, 5, 0, 0]).overflowing_mul(U256([0, 5, 0, 0]));
	assert_eq!(U256([0, 0, 25, 0]), result);

	let (result, _) = U256([0, 0, 0, 1]).overflowing_mul(U256([1, 0, 0, 0]));
	assert_eq!(U256([0, 0, 0, 1]), result);

	let (result, _) = U256([0, 0, 0, 5]).overflowing_mul(U256([2, 0, 0, 0]));
	assert_eq!(U256([0, 0, 0, 10]), result);

	let (result, _) = U256([0, 0, 1, 0]).overflowing_mul(U256([0, 5, 0, 0]));
	assert_eq!(U256([0, 0, 0, 5]), result);

	let (result, _) = U256([0, 0, 8, 0]).overflowing_mul(U256([0, 0, 7, 0]));
	assert_eq!(U256([0, 0, 0, 0]), result);

	let (result, _) = U256([2, 0, 0, 0]).overflowing_mul(U256([0, 5, 0, 0]));
	assert_eq!(U256([0, 10, 0, 0]), result);

	let (result, _) = U256([1, 0, 0, 0]).overflowing_mul(U256([0, 0, 0, u64::max_value()]));
	assert_eq!(U256([0, 0, 0, u64::max_value()]), result);

	let x1: U256 = "0000000000000000000000000000000000000000000000000000012365124623".into();
	let x2sqr_right: U256 = "000000000000000000000000000000000000000000014baeef72e0378e2328c9".into();
	let x1sqr = x1 * x1;
	assert_eq!(x2sqr_right, x1sqr);

	let x1cube = x1sqr * x1;
	let x1cube_right: U256 = "0000000000000000000000000000000001798acde139361466f712813717897b".into();
	assert_eq!(x1cube_right, x1cube);

	let x1quad = x1cube * x1;
	let x1quad_right: U256 = "000000000000000000000001adbdd6bd6ff027485484b97f8a6a4c7129756dd1".into();
	assert_eq!(x1quad_right, x1quad);

	let x1penta = x1quad * x1;
	let x1penta_right: U256 = "00000000000001e92875ac24be246e1c57e0507e8c46cc8d233b77f6f4c72993".into();
	assert_eq!(x1penta_right, x1penta);

	let x1septima = x1penta * x1;
	let x1septima_right: U256 = "00022cca1da3f6e5722b7d3cc5bbfb486465ebc5a708dd293042f932d7eee119".into();
	assert_eq!(x1septima_right, x1septima);
}

#[test]
fn example() {
	let mut val: U256 = 1023.into();
	for _ in 0..200 {
		val = val * U256::from(2)
	}
	assert_eq!(&format!("{}", val), "1643897619276947051879427220465009342380213662639797070513307648");
}

#[test]
fn little_endian() {
	let number: U256 = "00022cca1da3f6e5722b7d3cc5bbfb486465ebc5a708dd293042f932d7eee119".into();
	let expected = [
		0x19, 0xe1, 0xee, 0xd7, 0x32, 0xf9, 0x42, 0x30, 0x29, 0xdd, 0x08, 0xa7, 0xc5, 0xeb, 0x65, 0x64, 0x48, 0xfb,
		0xbb, 0xc5, 0x3c, 0x7d, 0x2b, 0x72, 0xe5, 0xf6, 0xa3, 0x1d, 0xca, 0x2c, 0x02, 0x00,
	];
	let mut result = [0u8; 32];
	number.to_little_endian(&mut result);
	assert_eq!(expected, result);
}

#[test]
fn slice_roundtrip() {
	let raw = [
		1u8, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103,
		107, 109, 113, 127,
	];

	let u256: U256 = (&raw[..]).into();

	let mut new_raw = [0u8; 32];

	u256.to_big_endian(&mut new_raw);

	assert_eq!(&raw, &new_raw);
}

#[test]
fn slice_roundtrip_le() {
	let raw = [
		1u8, 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103,
		107, 109, 113, 127,
	];

	let u256 = U256::from_little_endian(&raw[..]);

	let mut new_raw = [0u8; 32];

	u256.to_little_endian(&mut new_raw);

	assert_eq!(&raw, &new_raw);
}

#[test]
fn slice_roundtrip_le2() {
	let raw = [
		2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107,
		109, 113, 127,
	];

	let u256 = U256::from_little_endian(&raw[..]);

	let mut new_raw = [0u8; 32];

	u256.to_little_endian(&mut new_raw);

	assert_eq!(&raw, &new_raw[..31]);
}

#[test]
fn from_little_endian() {
	let source: [u8; 32] =
		[1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];

	let number = U256::from_little_endian(&source[..]);

	assert_eq!(U256::from(1), number);
}

#[test]
fn from_big_endian() {
	let source: [u8; 32] =
		[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];

	let number = U256::from_big_endian(&source[..]);

	assert_eq!(U256::from(1), number);

	let number = U256::from_big_endian(&[]);
	assert_eq!(U256::zero(), number);

	let number = U256::from_big_endian(&[1]);
	assert_eq!(U256::from(1), number);
}

#[test]
fn into_fixed_array() {
	let expected: [u8; 32] =
		[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1];
	let ary: [u8; 32] = U256::from(1).into();
	assert_eq!(ary, expected);
}

#[test]
fn test_u256_from_fixed_array() {
	let ary = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 123];
	let num: U256 = ary.into();
	assert_eq!(num, U256::from(core::u64::MAX) + 1 + 123);

	let a_ref: &U256 = &ary.into();
	assert_eq!(a_ref, &(U256::from(core::u64::MAX) + 1 + 123));
}

#[test]
fn test_from_ref_to_fixed_array() {
	let ary: &[u8; 32] =
		&[1, 0, 1, 2, 1, 0, 1, 2, 3, 0, 3, 4, 3, 0, 3, 4, 5, 0, 5, 6, 5, 0, 5, 6, 7, 0, 7, 8, 7, 0, 7, 8];
	let big: U256 = ary.into();
	// the numbers are each row of 8 bytes reversed and cast to u64
	assert_eq!(big, U256([504410889324070664, 360293493601469702, 216176097878868740, 72058702156267778u64]));
}

#[test]
fn test_u512_from_fixed_array() {
	let ary = [
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 123,
	];
	let num: U512 = ary.into();
	assert_eq!(num, U512::from(123));

	let a_ref: &U512 = &ary.into();
	assert_eq!(a_ref, &U512::from(123));
}

#[test]
fn leading_zeros() {
	assert_eq!(U256::from("000000000000000000000001adbdd6bd6ff027485484b97f8a6a4c7129756dd1").leading_zeros(), 95);
	assert_eq!(U256::from("f00000000000000000000001adbdd6bd6ff027485484b97f8a6a4c7129756dd1").leading_zeros(), 0);
	assert_eq!(U256::from("0000000000000000000000000000000000000000000000000000000000000001").leading_zeros(), 255);
	assert_eq!(U256::from("0000000000000000000000000000000000000000000000000000000000000000").leading_zeros(), 256);
}

#[test]
fn issue_507_roundtrip() {
	let mut b32 = <[u8; 32]>::default();
	let a = U256::from(10);
	a.to_little_endian(&mut b32);
	let b = U256::from_little_endian(&b32[..]);
	assert_eq!(a, b);
}

#[test]
fn trailing_zeros() {
	assert_eq!(U256::from("1adbdd6bd6ff027485484b97f8a6a4c7129756dd100000000000000000000000").trailing_zeros(), 92);
	assert_eq!(U256::from("1adbdd6bd6ff027485484b97f8a6a4c7129756dd10000000000000000000000f").trailing_zeros(), 0);
	assert_eq!(U256::from("8000000000000000000000000000000000000000000000000000000000000000").trailing_zeros(), 255);
	assert_eq!(U256::from("0000000000000000000000000000000000000000000000000000000000000000").trailing_zeros(), 256);
}

#[test]
fn bit_assign() {
	fn check(a: U256, b: U256) {
		// and
		{
			let mut x = a;
			x &= b;
			assert_eq!(x, a & b);
		}
		// or
		{
			let mut x = a;
			x |= b;
			assert_eq!(x, a | b);
		}
		// xor
		{
			let mut x = a;
			x ^= b;
			assert_eq!(x, a ^ b);
		}
		// shr
		{
			let mut x = a;
			x >>= b;
			assert_eq!(x, a >> b);
		}
		// shl
		{
			let mut x = a;
			x <<= b;
			assert_eq!(x, a << b);
		}
	}

	check(U256::from(9), U256::from(999999));
	check(U256::from(0), U256::from(0));
	check(U256::from(23432), U256::from(u32::MAX));
	check(U256::MAX, U256::zero());
}

#[cfg(feature = "quickcheck")]
pub mod laws {
	use super::construct_uint;
	macro_rules! uint_laws {
		($mod_name:ident, $uint_ty:ident) => {
			mod $mod_name {
				use quickcheck::{TestResult, quickcheck};
				use super::$uint_ty;

				quickcheck! {
					fn associative_add(x: $uint_ty, y: $uint_ty, z: $uint_ty) -> TestResult {
						if x.overflowing_add(y).1 || y.overflowing_add(z).1 || (x + y).overflowing_add(z).1 {
							return TestResult::discard();
						}

						TestResult::from_bool(
							(x + y) + z == x + (y + z)
						)
					}
				}

				quickcheck! {
					fn associative_mul(x: $uint_ty, y: $uint_ty, z: $uint_ty) -> TestResult {
						if x.overflowing_mul(y).1 || y.overflowing_mul(z).1 || (x * y).overflowing_mul(z).1 {
							return TestResult::discard();
						}

						TestResult::from_bool(
							(x * y) * z == x * (y * z)
						)
					}
				}

				quickcheck! {
					fn commutative_add(x: $uint_ty, y: $uint_ty) -> TestResult {
						if x.overflowing_add(y).1 {
							return TestResult::discard();
						}

						TestResult::from_bool(
							x + y == y + x
						)
					}
				}

				quickcheck! {
					fn commutative_mul(x: $uint_ty, y: $uint_ty) -> TestResult {
						if x.overflowing_mul(y).1 {
							return TestResult::discard();
						}

						TestResult::from_bool(
							x * y == y * x
						)
					}
				}

				quickcheck! {
					fn identity_add(x: $uint_ty) -> bool {
						x + $uint_ty::zero() == x
					}
				}

				quickcheck! {
					fn identity_mul(x: $uint_ty) -> bool {
						x * $uint_ty::one() == x
					}
				}

				quickcheck! {
					fn identity_div(x: $uint_ty) -> bool {
						x / $uint_ty::one() == x
					}
				}

				quickcheck! {
					fn absorbing_rem(x: $uint_ty) -> bool {
						x % $uint_ty::one() == $uint_ty::zero()
					}
				}

				quickcheck! {
					fn absorbing_sub(x: $uint_ty) -> bool {
						x - x == $uint_ty::zero()
					}
				}

				quickcheck! {
					fn absorbing_mul(x: $uint_ty) -> bool {
						x * $uint_ty::zero() == $uint_ty::zero()
					}
				}

				quickcheck! {
					fn distributive_mul_over_add(x: $uint_ty, y: $uint_ty, z: $uint_ty) -> TestResult {
						if y.overflowing_add(z).1 || x.overflowing_mul(y + z).1 || x.overflowing_add(y).1 || (x + y).overflowing_mul(z).1 {
							return TestResult::discard();
						}

						TestResult::from_bool(
							(x * (y + z) == (x * y + x * z)) && (((x + y) * z) == (x * z + y * z))
						)
					}
				}

				quickcheck! {
					fn isqrt(x: $uint_ty) -> TestResult {
						let s = x.integer_sqrt();
						let higher = s + 1;
						if let Some(y) = higher.checked_mul(higher) {
							TestResult::from_bool(
								(s * s <= x) && (y > x)
							)
						} else {
							TestResult::from_bool(
								s * s <= x
							)
						}
					}
				}

				quickcheck! {
					fn pow_mul(x: $uint_ty) -> TestResult {
						if x.overflowing_pow($uint_ty::from(2)).1 || x.overflowing_pow($uint_ty::from(3)).1 {
							// On overflow `checked_pow` should return `None`.
							assert_eq!(x.checked_pow($uint_ty::from(2)), None);
							assert_eq!(x.checked_pow($uint_ty::from(3)), None);

							return TestResult::discard();
						}

						TestResult::from_bool(
							x.pow($uint_ty::from(2)) == x * x && x.pow($uint_ty::from(3)) == x * x * x
						)
					}
				}

				quickcheck! {
					fn add_increases(x: $uint_ty, y: $uint_ty) -> TestResult {
						if y.is_zero() || x.overflowing_add(y).1 {
							return TestResult::discard();
						}

						TestResult::from_bool(
							x + y > x
						)
					}
				}

				quickcheck! {
					fn mul_increases(x: $uint_ty, y: $uint_ty) -> TestResult {
						if y.is_zero() || x.overflowing_mul(y).1 {
							return TestResult::discard();
						}

						TestResult::from_bool(
							x * y >= x
						)
					}
				}

				quickcheck! {
					fn div_decreases_dividend(x: $uint_ty, y: $uint_ty) -> TestResult {
						if y.is_zero() {
							return TestResult::discard();
						}

						TestResult::from_bool(
							x / y <= x
						)
					}
				}

				quickcheck! {
					fn rem_decreases_divisor(x: $uint_ty, y: $uint_ty) -> TestResult {
						if y.is_zero() {
							return TestResult::discard();
						}

						TestResult::from_bool(
							x % y < y
						)
					}
				}
			}
		}
	}

	construct_uint! {
		pub struct U64(1);
	}
	construct_uint! {
		pub struct U256(4);
	}
	construct_uint! {
		pub struct U512(8);
	}
	construct_uint! {
		pub struct U1024(16);
	}

	uint_laws!(u64, U64);
	uint_laws!(u256, U256);
	uint_laws!(u512, U512);
	uint_laws!(u1024, U1024);
}
