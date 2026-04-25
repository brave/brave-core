// Copyright (C) Parity Technologies (UK) Ltd.
// SPDX-License-Identifier: Apache-2.0

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// 	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use core::fmt::Debug;
use parity_scale_codec::{
	alloc::{
		collections::{BTreeMap, BTreeSet, LinkedList, VecDeque},
		rc::Rc,
	},
	DecodeWithMemLimit, DecodeWithMemTracking, Encode, Error, MemTrackingInput,
};
use parity_scale_codec_derive::{
	Decode as DeriveDecode, DecodeWithMemTracking as DeriveDecodeWithMemTracking,
	Encode as DeriveEncode,
};

const ARRAY: [u8; 1000] = [11; 1000];

#[derive(DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking, PartialEq, Debug)]
#[allow(clippy::large_enum_variant)]
enum TestEnum {
	Empty,
	Array([u8; 1000]),
}

#[derive(DeriveEncode, DeriveDecode, DeriveDecodeWithMemTracking, PartialEq, Debug)]
struct ComplexStruct {
	test_enum: TestEnum,
	boxed_test_enum: Box<TestEnum>,
	box_field: Box<u32>,
	vec: Vec<u8>,
}

fn decode_object<T>(obj: T, mem_limit: usize, expected_used_mem: usize) -> Result<T, Error>
where
	T: Encode + DecodeWithMemTracking + DecodeWithMemLimit + PartialEq + Debug,
{
	let encoded_bytes = obj.encode();

	let decoded_obj = T::decode_with_mem_limit(&mut &encoded_bytes[..], mem_limit)?;
	assert_eq!(&decoded_obj, &obj);

	let raw_input = &mut &encoded_bytes[..];
	let mut input = MemTrackingInput::new(raw_input, mem_limit);
	let decoded_obj = T::decode(&mut input)?;
	assert_eq!(&decoded_obj, &obj);
	assert_eq!(input.used_mem(), expected_used_mem);
	if expected_used_mem > 0 {
		let raw_input = &mut &encoded_bytes[..];
		let mut input = MemTrackingInput::new(raw_input, expected_used_mem);
		assert!(T::decode(&mut input).is_err());
	}

	Ok(decoded_obj)
}

#[test]
fn decode_simple_objects_works() {
	// Test simple objects
	assert!(decode_object(ARRAY, usize::MAX, 0).is_ok());
	assert!(decode_object(Some(ARRAY), usize::MAX, 0).is_ok());
	assert!(decode_object((ARRAY, ARRAY), usize::MAX, 0).is_ok());
	assert!(decode_object(1u8, usize::MAX, 0).is_ok());
	assert!(decode_object(1u32, usize::MAX, 0).is_ok());
	assert!(decode_object(1f64, usize::MAX, 0).is_ok());

	// Test heap objects
	assert!(decode_object(Box::new(ARRAY), usize::MAX, 1000).is_ok());
	#[cfg(target_has_atomic = "ptr")]
	{
		use parity_scale_codec::alloc::sync::Arc;
		assert!(decode_object(Arc::new(ARRAY), usize::MAX, 1000).is_ok());
	}
	assert!(decode_object(Rc::new(ARRAY), usize::MAX, 1000).is_ok());
	// Simple collections
	assert!(decode_object(vec![ARRAY; 3], usize::MAX, 3000).is_ok());
	assert!(decode_object(VecDeque::from(vec![ARRAY; 5]), usize::MAX, 5000).is_ok());
	assert!(decode_object(String::from("test"), usize::MAX, 4).is_ok());
	#[cfg(feature = "bytes")]
	assert!(decode_object(bytes::Bytes::from(&ARRAY[..]), usize::MAX, 1000).is_ok());
	// Complex Collections
	assert!(decode_object(BTreeMap::<u8, u8>::from([(1, 2), (2, 3)]), usize::MAX, 40).is_ok());
	assert!(decode_object(
		BTreeMap::from([
			("key1".to_string(), "value1".to_string()),
			("key2".to_string(), "value2".to_string()),
		]),
		usize::MAX,
		564,
	)
	.is_ok());
	assert!(decode_object(BTreeSet::<u8>::from([1, 2, 3, 4, 5]), usize::MAX, 24).is_ok());
	assert!(decode_object(LinkedList::<u8>::from([1, 2, 3, 4, 5]), usize::MAX, 120).is_ok());
}

#[test]
fn decode_complex_objects_works() {
	assert!(decode_object(vec![vec![vec![vec![vec![1u8]]]]], usize::MAX, 97).is_ok());
	assert!(decode_object(Box::new(Rc::new(vec![String::from("test")])), usize::MAX, 60).is_ok());
}

#[test]
fn decode_complex_derived_struct_works() {
	assert!(decode_object(
		ComplexStruct {
			test_enum: TestEnum::Array([0; 1000]),
			boxed_test_enum: Box::new(TestEnum::Empty),
			box_field: Box::new(1),
			vec: vec![1; 10],
		},
		usize::MAX,
		1015
	)
	.is_ok());
}

#[test]
fn mem_limit_exceeded_is_triggered() {
	// Test simple heap object
	assert_eq!(
		decode_object(Box::new(ARRAY), 999, 999).unwrap_err().to_string(),
		"Heap memory limit exceeded while decoding"
	);

	// Test complex derived struct
	assert_eq!(
		decode_object(
			ComplexStruct {
				test_enum: TestEnum::Array([0; 1000]),
				boxed_test_enum: Box::new(TestEnum::Empty),
				box_field: Box::new(1),
				vec: vec![1; 10],
			},
			1014,
			1014
		)
		.unwrap_err()
		.to_string(),
		"Could not decode `ComplexStruct::vec`:\n\tHeap memory limit exceeded while decoding\n"
	);
}
