//! Types related to implementing a [`FeeRule`] provides [ZIP 317] fee calculation.
//!
//! [`FeeRule`]: crate::transaction::fees::FeeRule
//! [ZIP 317]: https//zips.z.cash/zip-0317
use core::cmp::max;

use crate::{
    consensus::{self, BlockHeight},
    legacy::TransparentAddress,
    transaction::{
        components::{
            amount::{BalanceError, NonNegativeAmount},
            transparent::OutPoint,
        },
        fees::transparent,
    },
};

/// The standard [ZIP 317] marginal fee.
///
/// [ZIP 317]: https//zips.z.cash/zip-0317
pub const MARGINAL_FEE: NonNegativeAmount = NonNegativeAmount::const_from_u64(5_000);

/// The minimum number of logical actions that must be paid according to [ZIP 317].
///
/// [ZIP 317]: https//zips.z.cash/zip-0317
pub const GRACE_ACTIONS: usize = 2;

/// The standard size of a P2PKH input, in bytes, according to [ZIP 317].
///
/// [ZIP 317]: https//zips.z.cash/zip-0317
pub const P2PKH_STANDARD_INPUT_SIZE: usize = 150;

/// The standard size of a P2PKH output, in bytes, according to [ZIP 317].
///
/// [ZIP 317]: https//zips.z.cash/zip-0317
pub const P2PKH_STANDARD_OUTPUT_SIZE: usize = 34;

/// The minimum conventional fee computed from the standard [ZIP 317] constants. Equivalent to
/// `MARGINAL_FEE * GRACE_ACTIONS`.
///
/// [ZIP 317]: https//zips.z.cash/zip-0317
pub const MINIMUM_FEE: NonNegativeAmount = NonNegativeAmount::const_from_u64(10_000);

/// A [`FeeRule`] implementation that implements the [ZIP 317] fee rule.
///
/// This fee rule supports Orchard, Sapling, and (P2PKH only) transparent inputs.
/// Returns an error if a coin containing a non-p2pkh script is provided as an input.
/// This fee rule may slightly overestimate fees in case where the user is attempting to spend more than ~150 transparent inputs.
///
/// [`FeeRule`]: crate::transaction::fees::FeeRule
/// [ZIP 317]: https//zips.z.cash/zip-0317
#[derive(Clone, Debug)]
pub struct FeeRule {
    marginal_fee: NonNegativeAmount,
    grace_actions: usize,
    p2pkh_standard_input_size: usize,
    p2pkh_standard_output_size: usize,
}

impl FeeRule {
    /// Construct a new FeeRule using the standard [ZIP 317] constants.
    ///
    /// [ZIP 317]: https//zips.z.cash/zip-0317
    pub fn standard() -> Self {
        Self {
            marginal_fee: MARGINAL_FEE,
            grace_actions: GRACE_ACTIONS,
            p2pkh_standard_input_size: P2PKH_STANDARD_INPUT_SIZE,
            p2pkh_standard_output_size: P2PKH_STANDARD_OUTPUT_SIZE,
        }
    }

    /// Construct a new FeeRule instance with the specified parameter values.
    ///
    /// Returns `None` if either `p2pkh_standard_input_size` or `p2pkh_standard_output_size` are
    /// zero.
    pub fn non_standard(
        marginal_fee: NonNegativeAmount,
        grace_actions: usize,
        p2pkh_standard_input_size: usize,
        p2pkh_standard_output_size: usize,
    ) -> Option<Self> {
        if p2pkh_standard_input_size == 0 || p2pkh_standard_output_size == 0 {
            None
        } else {
            Some(Self {
                marginal_fee,
                grace_actions,
                p2pkh_standard_input_size,
                p2pkh_standard_output_size,
            })
        }
    }

    /// Returns the ZIP 317 marginal fee.
    pub fn marginal_fee(&self) -> NonNegativeAmount {
        self.marginal_fee
    }
    /// Returns the ZIP 317 number of grace actions
    pub fn grace_actions(&self) -> usize {
        self.grace_actions
    }
    /// Returns the ZIP 317 standard P2PKH input size
    pub fn p2pkh_standard_input_size(&self) -> usize {
        self.p2pkh_standard_input_size
    }
    /// Returns the ZIP 317 standard P2PKH output size
    pub fn p2pkh_standard_output_size(&self) -> usize {
        self.p2pkh_standard_output_size
    }
}

/// Errors that can occur in ZIP 317 fee computation
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum FeeError {
    /// An overflow or underflow of amount computation occurred.
    Balance(BalanceError),
    /// Transparent inputs provided to the fee calculation included coins that do not pay to
    /// standard P2pkh scripts.
    NonP2pkhInputs(Vec<OutPoint>),
}

impl From<BalanceError> for FeeError {
    fn from(err: BalanceError) -> Self {
        FeeError::Balance(err)
    }
}

impl std::fmt::Display for FeeError {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match &self {
            FeeError::Balance(e) => write!(
                f,
                "A balance calculation violated amount validity bounds: {}.",
                e
            ),
            FeeError::NonP2pkhInputs(_) => write!(f, "Only P2PKH inputs are supported."),
        }
    }
}

impl super::FeeRule for FeeRule {
    type Error = FeeError;

    fn fee_required<P: consensus::Parameters>(
        &self,
        _params: &P,
        _target_height: BlockHeight,
        transparent_inputs: &[impl transparent::InputView],
        transparent_outputs: &[impl transparent::OutputView],
        sapling_input_count: usize,
        sapling_output_count: usize,
        orchard_action_count: usize,
    ) -> Result<NonNegativeAmount, Self::Error> {
        let non_p2pkh_inputs: Vec<_> = transparent_inputs
            .iter()
            .filter_map(|t_in| match t_in.coin().script_pubkey.address() {
                Some(TransparentAddress::PublicKeyHash(_)) => None,
                _ => Some(t_in.outpoint()),
            })
            .cloned()
            .collect();

        if !non_p2pkh_inputs.is_empty() {
            return Err(FeeError::NonP2pkhInputs(non_p2pkh_inputs));
        }

        let t_in_total_size = transparent_inputs.len() * 150;
        let t_out_total_size = transparent_outputs.len() * 34;

        let ceildiv = |num: usize, den: usize| (num + den - 1) / den;

        let logical_actions = max(
            ceildiv(t_in_total_size, self.p2pkh_standard_input_size),
            ceildiv(t_out_total_size, self.p2pkh_standard_output_size),
        ) + max(sapling_input_count, sapling_output_count)
            + orchard_action_count;

        (self.marginal_fee * max(self.grace_actions, logical_actions))
            .ok_or_else(|| BalanceError::Overflow.into())
    }
}
