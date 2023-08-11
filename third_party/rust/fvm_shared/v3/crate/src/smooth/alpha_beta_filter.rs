// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use fvm_ipld_encoding::tuple::*;

use crate::bigint::{bigint_ser, BigInt, Integer};
use crate::clock::ChainEpoch;
use crate::math::PRECISION;

#[derive(Default, Serialize_tuple, Deserialize_tuple, Clone, Debug, PartialEq, Eq)]
pub struct FilterEstimate {
    #[serde(with = "bigint_ser")]
    pub position: BigInt,
    #[serde(with = "bigint_ser")]
    pub velocity: BigInt,
}

impl FilterEstimate {
    /// Create a new filter estimate given two Q.0 format ints.
    pub fn new(position: BigInt, velocity: BigInt) -> Self {
        FilterEstimate {
            position: position << PRECISION,
            velocity: velocity << PRECISION,
        }
    }

    /// Returns the Q.0 position estimate of the filter
    pub fn estimate(&self) -> BigInt {
        &self.position >> PRECISION
    }

    /// Extrapolate filter "position" delta epochs in the future.
    pub fn extrapolate(&self, delta: ChainEpoch) -> BigInt {
        let delta_t = BigInt::from(delta) << PRECISION;
        let position = &self.position << PRECISION;
        (&self.velocity * delta_t) + position
    }
}

pub struct AlphaBetaFilter<'a, 'b, 'f> {
    alpha: &'a BigInt,
    beta: &'b BigInt,
    prev_est: &'f FilterEstimate,
}

impl<'a, 'b, 'f> AlphaBetaFilter<'a, 'b, 'f> {
    pub fn load(prev_est: &'f FilterEstimate, alpha: &'a BigInt, beta: &'b BigInt) -> Self {
        Self {
            alpha,
            beta,
            prev_est,
        }
    }

    pub fn next_estimate(&self, obs: &BigInt, epoch_delta: ChainEpoch) -> FilterEstimate {
        let delta_t = BigInt::from(epoch_delta) << PRECISION;
        let delta_x = (&delta_t * &self.prev_est.velocity) >> PRECISION;
        let mut position = delta_x + &self.prev_est.position;

        let obs = obs << PRECISION;
        let residual = obs - &position;
        let revision_x = (self.alpha * &residual) >> PRECISION;
        position += &revision_x;

        let revision_v = residual * self.beta;
        let revision_v = revision_v.div_floor(&delta_t);
        let velocity = revision_v + &self.prev_est.velocity;
        FilterEstimate { position, velocity }
    }
}

#[cfg(test)]
mod tests {
    use super::super::{DEFAULT_ALPHA, DEFAULT_BETA};
    use super::*;

    #[test]
    fn rounding() {
        // Calculations in this mod are under the assumption division is euclidean and not truncated
        let dd: BigInt = BigInt::from(-100);
        let dv: BigInt = BigInt::from(3);
        assert_eq!(dd.div_floor(&dv), BigInt::from(-34));

        let dd: BigInt = BigInt::from(200);
        let dv: BigInt = BigInt::from(3);
        assert_eq!(dd.div_floor(&dv), BigInt::from(66));
    }

    #[test]
    fn rounding_issue() {
        let fe = FilterEstimate {
            position: "12340768897043811082913117521041414330876498465539749838848"
                .parse()
                .unwrap(),
            velocity: "-37396269384748225153347462373739139597454335279104"
                .parse()
                .unwrap(),
        };
        let filter_reward = AlphaBetaFilter::load(&fe, &DEFAULT_ALPHA, &DEFAULT_BETA);
        let next = filter_reward.next_estimate(&36266252337034982540u128.into(), 3);
        assert_eq!(
            next.position.to_string(),
            "12340768782449774548722755900999027209659079673176744001536"
        );
        assert_eq!(
            next.velocity.to_string(),
            "-37396515542149801792802995707072472930787668612438"
        );
    }
}
