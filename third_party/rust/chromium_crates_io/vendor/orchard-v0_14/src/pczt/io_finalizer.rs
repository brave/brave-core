use core::fmt;

use alloc::vec::Vec;

use rand::{CryptoRng, RngCore};

use crate::{
    keys::SpendAuthorizingKey,
    primitives::redpallas,
    value::{ValueCommitTrapdoor, ValueCommitment},
};

use super::SignerError;

impl super::Bundle {
    /// Finalizes the IO for this bundle.
    pub fn finalize_io<R: RngCore + CryptoRng>(
        &mut self,
        sighash: [u8; 32],
        mut rng: R,
    ) -> Result<(), IoFinalizerError> {
        // Compute the transaction binding signing key.
        let rcvs = self
            .actions
            .iter()
            .map(|a| {
                a.rcv
                    .as_ref()
                    .ok_or(IoFinalizerError::MissingValueCommitTrapdoor)
            })
            .collect::<Result<Vec<_>, _>>()?;
        let bsk = rcvs.into_iter().sum::<ValueCommitTrapdoor>().into_bsk();

        // Verify that bsk and bvk are consistent.
        let bvk = (self
            .actions
            .iter()
            .map(|a| a.cv_net())
            .sum::<ValueCommitment>()
            - ValueCommitment::derive(self.value_sum, ValueCommitTrapdoor::zero()))
        .into_bvk();
        if redpallas::VerificationKey::from(&bsk) != bvk {
            return Err(IoFinalizerError::ValueCommitMismatch);
        }
        self.bsk = Some(bsk);

        // Add signatures to dummy spends.
        for action in self.actions.iter_mut() {
            // The `Option::take` ensures we don't have any spend authorizing keys in the
            // PCZT after the IO Finalizer has run.
            if let Some(sk) = action.spend.dummy_sk.take() {
                let ask = SpendAuthorizingKey::from(&sk);
                action
                    .sign(sighash, &ask, &mut rng)
                    .map_err(IoFinalizerError::DummySignature)?;
            }
        }

        Ok(())
    }
}

/// Errors that can occur while finalizing the I/O for a PCZT bundle.
#[derive(Debug)]
#[non_exhaustive]
pub enum IoFinalizerError {
    /// An error occurred while signing a dummy spend.
    DummySignature(SignerError),
    /// The IO Finalizer role requires all `rcv` fields to be set.
    MissingValueCommitTrapdoor,
    /// The `cv_net`, `rcv`, and `value_sum` values within the Orchard bundle are
    /// inconsistent.
    ValueCommitMismatch,
}

impl fmt::Display for IoFinalizerError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            IoFinalizerError::DummySignature(e) => {
                write!(f, "An error occurred while signing a dummy spend: {e}")
            }
            IoFinalizerError::MissingValueCommitTrapdoor => write!(
                f,
                "The IO Finalizer role requires all `rcv` fields to be set"
            ),
            IoFinalizerError::ValueCommitMismatch => write!(
                f,
                "`cv_net`, `rcv`, and `value_sum` within the Orchard bundle are inconsistent."
            ),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for IoFinalizerError {}
