// Copyright (C) 2020-2021 Parity Technologies (UK) Ltd.
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

#[test]
#[cfg(feature = "derive")]
fn derive_no_bound_ui() {
	let t = trybuild::TestCases::new();
	t.compile_fail("tests/decode_with_mem_tracking_ui/*.rs");
	t.pass("tests/decode_with_mem_tracking_ui/pass/*.rs");
}
