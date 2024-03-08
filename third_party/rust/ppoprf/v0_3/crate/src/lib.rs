//! This module defines the combined functionality for producing a
//! puncturable partially oblivious pseudorandom function (PPOPRF)
//! protocol. The protocol combines the PPOPRF of [Tyagi et
//! al.](https://eprint.iacr.org/2021/864.pdf) with the classic GGM
//! puncturable PRF.
//!
//! The result is a POPRF that can provide forward-security guarantees
//! related to the pseudorandomness of client-side outputs, by allowing
//! the puncturing of metadata tags from the server secret key. Such
//! guarantees hold when clients reveal POPRF outputs for a metadata tag
//! `t`, after `t` has been punctured from the secret key. This
//! functionality is used to provide forward-secure randomness to
//! clients in the STAR protocol.

pub mod ggm;
pub mod ppoprf;

mod strobe_rng;

use derive_more::{Display, Error};

#[derive(Debug, Error, Display)]
pub enum PPRFError {
  #[display(fmt = "Specified tag ({md}) is not a valid metadata tag")]
  BadTag { md: u8 },
  #[display(fmt = "No prefix found")]
  NoPrefixFound,
  #[display(fmt = "Tag already punctured")]
  AlreadyPunctured,
  #[display(
    fmt = "Input length ({actual}) does not match input param ({expected})"
  )]
  BadInputLength { actual: usize, expected: usize },
  #[display(fmt = "Unexpected end of bv")]
  UnexpectedEndOfBv,
  #[display(fmt = "Bincode serialization error: {_0}")]
  Bincode(bincode::Error),
  #[display(fmt = "Serialized data exceeds size limit")]
  SerializedDataTooBig,
  #[display(fmt = "Bad compressed ristretto point encoding")]
  BadPointEncoding,
}

pub trait PPRF {
  fn setup() -> Self;
  fn eval(&self, input: &[u8], output: &mut [u8]) -> Result<(), PPRFError>;
  fn puncture(&mut self, input: &[u8]) -> Result<(), PPRFError>;
}
