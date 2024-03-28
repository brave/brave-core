// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

mod quantize;
pub use quantize::*;

const _ISO_FORMAT: &str = "%FT%X.%.9F";

/// Duration of each tipset epoch.
pub const EPOCH_DURATION_SECONDS: i64 = 30;

/// Epoch number of a chain. This acts as a proxy for time within the VM.
pub type ChainEpoch = i64;

/// Const used within the VM to denote an unset `ChainEpoch`
pub const EPOCH_UNDEFINED: ChainEpoch = -1;
