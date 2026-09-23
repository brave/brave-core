// Copyright 2017, 2018 Parity Technologies
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

use parity_scale_codec::{Decode, Encode, HasCompact};
use parity_scale_codec_derive::{Decode as DeriveDecode, Encode as DeriveEncode};

#[test]
fn ensure_derive_macro_derives_bounds_correctly() {
	#[derive(DeriveEncode, DeriveDecode)]
	pub struct Header<Number> {
		#[codec(compact)]
		pub number: Number,
	}

	trait _IsEncodeDecode: Encode + Decode {}

	impl<Number: HasCompact> _IsEncodeDecode for Header<Number> {}
}
