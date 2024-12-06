// Copyright 2021-2023 Protocol Labs
// SPDX-License-Identifier: Apache-2.0, MIT
use serde_tuple::*;

use crate::bigint::bigint_ser;
use crate::sector::StoragePower;
use crate::smooth::FilterEstimate;

#[derive(Clone, Debug, PartialEq, Eq, Serialize_tuple, Deserialize_tuple)]
pub struct ThisEpochRewardReturn {
    // * Removed this_epoch_reward in v2
    pub this_epoch_reward_smoothed: FilterEstimate,
    #[serde(with = "bigint_ser")]
    pub this_epoch_baseline_power: StoragePower,
}
