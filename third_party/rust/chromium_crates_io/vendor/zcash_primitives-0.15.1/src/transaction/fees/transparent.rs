//! Types related to computation of fees and change related to the transparent components
//! of a transaction.

use std::convert::Infallible;

use crate::{
    legacy::Script,
    transaction::components::{amount::NonNegativeAmount, transparent::TxOut, OutPoint},
};

#[cfg(feature = "transparent-inputs")]
use crate::transaction::components::transparent::builder::TransparentInputInfo;

/// This trait provides a minimized view of a transparent input suitable for use in
/// fee and change computation.
pub trait InputView: std::fmt::Debug {
    /// The outpoint to which the input refers.
    fn outpoint(&self) -> &OutPoint;
    /// The previous output being spent.
    fn coin(&self) -> &TxOut;
}

#[cfg(feature = "transparent-inputs")]
impl InputView for TransparentInputInfo {
    fn outpoint(&self) -> &OutPoint {
        self.outpoint()
    }

    fn coin(&self) -> &TxOut {
        self.coin()
    }
}

impl InputView for Infallible {
    fn outpoint(&self) -> &OutPoint {
        unreachable!()
    }
    fn coin(&self) -> &TxOut {
        unreachable!()
    }
}

/// This trait provides a minimized view of a transparent output suitable for use in
/// fee and change computation.
pub trait OutputView: std::fmt::Debug {
    /// Returns the value of the output being created.
    fn value(&self) -> NonNegativeAmount;
    /// Returns the script corresponding to the newly created output.
    fn script_pubkey(&self) -> &Script;
}

impl OutputView for TxOut {
    fn value(&self) -> NonNegativeAmount {
        self.value
    }

    fn script_pubkey(&self) -> &Script {
        &self.script_pubkey
    }
}
