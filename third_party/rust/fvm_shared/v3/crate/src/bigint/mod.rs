// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

pub mod bigint_ser;
pub mod biguint_ser;

pub use num_bigint::*;
pub use num_integer::{self, Integer};
pub use num_traits::Zero;

/// The maximum number of bytes we accept to serialize/deserialize for a single BigInt.
pub const MAX_BIGINT_SIZE: usize = 128;
