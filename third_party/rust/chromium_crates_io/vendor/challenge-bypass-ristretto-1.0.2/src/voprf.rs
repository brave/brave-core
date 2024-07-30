//! An implementation of a verifiable oblivious pseudorandom function

#[cfg(not(feature = "merlin"))]
pub use crate::dleq::*;
#[cfg(feature = "merlin")]
pub use crate::dleq_merlin::*;

pub use crate::oprf::*;
