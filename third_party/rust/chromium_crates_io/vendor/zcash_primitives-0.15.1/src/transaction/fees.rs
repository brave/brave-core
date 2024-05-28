//! Abstractions and types related to fee calculations.

use crate::{
    consensus::{self, BlockHeight},
    transaction::components::amount::NonNegativeAmount,
};

pub mod fixed;
pub mod transparent;
pub mod zip317;

#[cfg(zcash_unstable = "zfuture")]
pub mod tze;

/// A trait that represents the ability to compute the fees that must be paid
/// by a transaction having a specified set of inputs and outputs.
pub trait FeeRule {
    type Error;

    /// Computes the total fee required for a transaction given the provided inputs and outputs.
    ///
    /// Implementations of this method should compute the fee amount given exactly the inputs and
    /// outputs specified, and should NOT compute speculative fees given any additional change
    /// outputs that may need to be created in order for inputs and outputs to balance.
    #[allow(clippy::too_many_arguments)]
    fn fee_required<P: consensus::Parameters>(
        &self,
        params: &P,
        target_height: BlockHeight,
        transparent_inputs: &[impl transparent::InputView],
        transparent_outputs: &[impl transparent::OutputView],
        sapling_input_count: usize,
        sapling_output_count: usize,
        orchard_action_count: usize,
    ) -> Result<NonNegativeAmount, Self::Error>;
}

/// A trait that represents the ability to compute the fees that must be paid by a transaction
/// having a specified set of inputs and outputs, for use when experimenting with the TZE feature.
#[cfg(zcash_unstable = "zfuture")]
pub trait FutureFeeRule: FeeRule {
    /// Computes the total fee required for a transaction given the provided inputs and outputs.
    ///
    /// Implementations of this method should compute the fee amount given exactly the inputs and
    /// outputs specified, and should NOT compute speculative fees given any additional change
    /// outputs that may need to be created in order for inputs and outputs to balance.
    #[allow(clippy::too_many_arguments)]
    fn fee_required_zfuture<P: consensus::Parameters>(
        &self,
        params: &P,
        target_height: BlockHeight,
        transparent_inputs: &[impl transparent::InputView],
        transparent_outputs: &[impl transparent::OutputView],
        sapling_input_count: usize,
        sapling_output_count: usize,
        orchard_action_count: usize,
        tze_inputs: &[impl tze::InputView],
        tze_outputs: &[impl tze::OutputView],
    ) -> Result<NonNegativeAmount, Self::Error>;
}

/// An enumeration of the standard fee rules supported by the wallet.
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum StandardFeeRule {
    #[deprecated(
        note = "Using this fee rule violates ZIP 317, and might cause transactions built with it to fail. Use `StandardFeeRule::Zip317` instead."
    )]
    PreZip313,
    #[deprecated(
        note = "Using this fee rule violates ZIP 317, and might cause transactions built with it to fail. Use `StandardFeeRule::Zip317` instead."
    )]
    Zip313,
    Zip317,
}

impl FeeRule for StandardFeeRule {
    type Error = zip317::FeeError;

    fn fee_required<P: consensus::Parameters>(
        &self,
        params: &P,
        target_height: BlockHeight,
        transparent_inputs: &[impl transparent::InputView],
        transparent_outputs: &[impl transparent::OutputView],
        sapling_input_count: usize,
        sapling_output_count: usize,
        orchard_action_count: usize,
    ) -> Result<NonNegativeAmount, Self::Error> {
        #[allow(deprecated)]
        match self {
            Self::PreZip313 => Ok(zip317::MINIMUM_FEE),
            Self::Zip313 => Ok(NonNegativeAmount::const_from_u64(1000)),
            Self::Zip317 => zip317::FeeRule::standard().fee_required(
                params,
                target_height,
                transparent_inputs,
                transparent_outputs,
                sapling_input_count,
                sapling_output_count,
                orchard_action_count,
            ),
        }
    }
}
