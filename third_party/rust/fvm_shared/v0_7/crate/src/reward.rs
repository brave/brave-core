use serde_tuple::*;

use crate::bigint::bigint_ser;
use crate::sector::StoragePower;
use crate::smooth::FilterEstimate;

#[derive(Clone, Debug, PartialEq, Serialize_tuple, Deserialize_tuple)]
pub struct ThisEpochRewardReturn {
    // * Removed this_epoch_reward in v2
    pub this_epoch_reward_smoothed: FilterEstimate,
    #[serde(with = "bigint_ser")]
    pub this_epoch_baseline_power: StoragePower,
}
