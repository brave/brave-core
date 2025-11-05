// Copyright 2017-2018 Parity Technologies
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use core::mem::size_of;

// Constants from rust's source:
// https://doc.rust-lang.org/src/alloc/collections/btree/node.rs.html#43-45
const B: usize = 6;
const CAPACITY: usize = 2 * B - 1;
const MIN_LEN_AFTER_SPLIT: usize = B - 1;

/// Estimate the mem size of a btree.
pub fn mem_size_of_btree<T>(len: u32) -> usize {
	if len == 0 {
		return 0;
	}

	// We try to estimate the size of the `InternalNode` struct from:
	// https://doc.rust-lang.org/src/alloc/collections/btree/node.rs.html#97
	// A btree `LeafNode` has 2*B - 1 (K,V) pairs and (usize, u16, u16) overhead.
	let leaf_node_size = size_of::<(usize, u16, u16, [T; CAPACITY])>();
	// An `InternalNode` additionally has 2*B `usize` overhead.
	let internal_node_size = leaf_node_size + size_of::<[usize; 2 * B]>();
	// A node can contain between B - 1 and 2*B - 1 elements. We assume 2/3 occupancy.
	let num_nodes = (len as usize).saturating_div((CAPACITY + MIN_LEN_AFTER_SPLIT) * 2 / 3);

	// If the tree has only one node, it's a leaf node.
	if num_nodes == 0 {
		return leaf_node_size;
	}
	num_nodes.saturating_mul(internal_node_size)
}

#[cfg(test)]
#[cfg(not(miri))]
#[rustversion::nightly]
mod test {
	use super::*;
	use crate::alloc::{
		collections::{BTreeMap, BTreeSet},
		sync::{Arc, Mutex},
	};
	use core::{
		alloc::{AllocError, Allocator, Layout},
		ptr::NonNull,
	};

	#[derive(Clone)]
	struct MockAllocator {
		total: Arc<Mutex<usize>>,
	}

	unsafe impl Allocator for MockAllocator {
		fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
			let ptr = std::alloc::System.allocate(layout);
			if ptr.is_ok() {
				*self.total.lock().unwrap() += layout.size();
			}
			ptr
		}

		unsafe fn deallocate(&self, ptr: NonNull<u8>, layout: Layout) {
			*self.total.lock().unwrap() -= layout.size();
			std::alloc::System.deallocate(ptr, layout)
		}
	}

	fn check_btree_size(expected_size: usize, actual_size: Arc<Mutex<usize>>) {
		// Check that the margin of error is at most 25%.
		assert!(*actual_size.lock().unwrap() as f64 * 0.75 <= expected_size as f64);
		assert!(*actual_size.lock().unwrap() as f64 * 1.25 >= expected_size as f64);
	}

	#[test]
	fn mem_size_of_btree_works() {
		let map_allocator = MockAllocator { total: Arc::new(Mutex::new(0)) };
		let map_actual_size = map_allocator.total.clone();
		let mut map = BTreeMap::<u32, u32, MockAllocator>::new_in(map_allocator);

		let set_allocator = MockAllocator { total: Arc::new(Mutex::new(0)) };
		let set_actual_size = set_allocator.total.clone();
		let mut set = BTreeSet::<u128, MockAllocator>::new_in(set_allocator);

		for i in 0..1000000 {
			map.insert(i, 0);
			set.insert(i as u128);

			// For small number of elements, the differences between the expected size and
			// the actual size can be higher.
			if i > 100 {
				let map_expected_size = mem_size_of_btree::<(u32, u32)>(map.len() as u32);
				check_btree_size(map_expected_size, map_actual_size.clone());

				let set_expected_size = mem_size_of_btree::<u128>(set.len() as u32);
				check_btree_size(set_expected_size, set_actual_size.clone());
			}
		}
	}
}
