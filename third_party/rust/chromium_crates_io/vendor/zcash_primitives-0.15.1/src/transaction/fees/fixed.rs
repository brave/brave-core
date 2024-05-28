use crate::{
    consensus::{self, BlockHeight},
    transaction::components::amount::NonNegativeAmount,
    transaction::fees::{transparent, zip317},
};

#[cfg(zcash_unstable = "zfuture")]
use crate::transaction::fees::tze;

/// A fee rule that always returns a fixed fee, irrespective of the structure of
/// the transaction being constructed.
#[derive(Clone, Copy, Debug)]
pub struct FeeRule {
    fixed_fee: NonNegativeAmount,
}

impl FeeRule {
    /// Creates a new nonstandard fixed fee rule with the specified fixed fee.
    pub fn non_standard(fixed_fee: NonNegativeAmount) -> Self {
        Self { fixed_fee }
    }

    /// Creates a new fixed fee rule with the minimum possible [ZIP 317] fee,
    /// i.e. 10000 zatoshis.
    ///
    /// Note that using a fixed fee is not compliant with [ZIP 317]; consider
    /// using [`zcash_primitives::transaction::fees::zip317::FeeRule::standard()`]
    /// instead.
    ///
    /// [`zcash_primitives::transaction::fees::zip317::FeeRule::standard()`]: crate::transaction::fees::zip317::FeeRule::standard
    /// [ZIP 317]: https://zips.z.cash/zip-0317
    #[deprecated(
        since = "0.12.0",
        note = "To calculate the ZIP 317 fee, use `transaction::fees::zip317::FeeRule::standard()`. For a fixed fee, use the `non_standard` constructor."
    )]
    pub fn standard() -> Self {
        Self {
            fixed_fee: zip317::MINIMUM_FEE,
        }
    }

    /// Returns the fixed fee amount which which this rule was configured.
    pub fn fixed_fee(&self) -> NonNegativeAmount {
        self.fixed_fee
    }
}

impl super::FeeRule for FeeRule {
    type Error = std::convert::Infallible;

    fn fee_required<P: consensus::Parameters>(
        &self,
        _params: &P,
        _target_height: BlockHeight,
        _transparent_inputs: &[impl transparent::InputView],
        _transparent_outputs: &[impl transparent::OutputView],
        _sapling_input_count: usize,
        _sapling_output_count: usize,
        _orchard_action_count: usize,
    ) -> Result<NonNegativeAmount, Self::Error> {
        Ok(self.fixed_fee)
    }
}

#[cfg(zcash_unstable = "zfuture")]
impl super::FutureFeeRule for FeeRule {
    fn fee_required_zfuture<P: consensus::Parameters>(
        &self,
        _params: &P,
        _target_height: BlockHeight,
        _transparent_inputs: &[impl transparent::InputView],
        _transparent_outputs: &[impl transparent::OutputView],
        _sapling_input_count: usize,
        _sapling_output_count: usize,
        _orchard_action_count: usize,
        _tze_inputs: &[impl tze::InputView],
        _tze_outputs: &[impl tze::OutputView],
    ) -> Result<NonNegativeAmount, Self::Error> {
        Ok(self.fixed_fee)
    }
}
