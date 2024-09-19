// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use super::ChainEpoch;

/// Constant defining the [QuantSpec] which performs no quantization.
pub const NO_QUANTIZATION: QuantSpec = QuantSpec { unit: 1, offset: 0 };

/// A spec for epoch quantization.
#[derive(Copy, Clone)]
pub struct QuantSpec {
    /// The unit of quantization
    pub unit: ChainEpoch,
    /// The offset from zero from which to base the modulus
    pub offset: ChainEpoch,
}

impl QuantSpec {
    /// Rounds `epoch` to the nearest exact multiple of the quantization unit offset by
    /// `offset % unit`, rounding up.
    ///
    /// This function is equivalent to `unit * ceil(epoch - (offset % unit) / unit) + (offsetSeed % unit)`
    /// with the variables/operations over real numbers instead of ints.
    ///
    /// Precondition: `unit >= 0`
    pub fn quantize_up(&self, epoch: ChainEpoch) -> ChainEpoch {
        let offset = self.offset % self.unit;

        let remainder = (epoch - offset) % self.unit;
        let quotient = (epoch - offset) / self.unit;

        // Don't round if epoch falls on a quantization epoch
        if remainder == 0
        // Negative truncating division rounds up
        || epoch - offset < 0
        {
            self.unit * quotient + offset
        } else {
            self.unit * (quotient + 1) + offset
        }
    }

    pub fn quantize_down(&self, epoch: ChainEpoch) -> ChainEpoch {
        let next = self.quantize_up(epoch);
        // QuantizeDown == QuantizeUp iff epoch is a fixed point of QuantizeUp
        if epoch == next {
            next
        } else {
            next - self.unit
        }
    }
}
