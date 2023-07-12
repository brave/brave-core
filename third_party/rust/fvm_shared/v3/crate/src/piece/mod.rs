// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

#[cfg(feature = "proofs")]
pub mod zero;

use cid::Cid;
use serde::{Deserialize, Serialize};
use serde_tuple::*;
#[cfg(feature = "proofs")]
pub use zero::zero_piece_commitment;

/// Size of a piece in bytes.
#[derive(PartialEq, Debug, Eq, Clone, Copy)]
pub struct UnpaddedPieceSize(pub u64);

impl UnpaddedPieceSize {
    /// Converts unpadded piece size into padded piece size.
    pub fn padded(self) -> PaddedPieceSize {
        PaddedPieceSize(self.0 + (self.0 / 127))
    }

    /// Validates piece size.
    pub fn validate(self) -> Result<(), &'static str> {
        if self.0 < 127 {
            return Err("minimum piece size is 127 bytes");
        }

        // is 127 * 2^n
        if self.0 >> self.0.trailing_zeros() != 127 {
            return Err("unpadded piece size must be a power of 2 multiple of 127");
        }

        Ok(())
    }
}

/// Size of a piece in bytes with padding.
#[derive(PartialEq, Debug, Eq, Clone, Copy, Serialize, Deserialize)]
#[serde(transparent)]
pub struct PaddedPieceSize(pub u64);

impl PaddedPieceSize {
    /// Converts padded piece size into an unpadded piece size.
    pub fn unpadded(self) -> UnpaddedPieceSize {
        UnpaddedPieceSize(self.0 - (self.0 / 128))
    }

    /// Validates piece size.
    pub fn validate(self) -> Result<(), &'static str> {
        if self.0 < 128 {
            return Err("minimum piece size is 128 bytes");
        }

        if self.0.count_ones() != 1 {
            return Err("padded piece size must be a power of 2");
        }

        Ok(())
    }
}

/// Piece information for part or a whole file.
#[derive(Serialize_tuple, Deserialize_tuple, PartialEq, Eq, Clone, Debug)]
pub struct PieceInfo {
    /// Size in nodes. For BLS12-381 (capacity 254 bits), must be >= 16. (16 * 8 = 128).
    pub size: PaddedPieceSize,
    /// Content identifier for piece.
    pub cid: Cid,
}

#[cfg(feature = "proofs")]
use std::convert::TryFrom;

#[cfg(feature = "proofs")]
impl TryFrom<&PieceInfo> for filecoin_proofs_api::PieceInfo {
    type Error = &'static str;

    fn try_from(p: &PieceInfo) -> Result<Self, Self::Error> {
        Ok(Self {
            commitment: crate::commcid::cid_to_piece_commitment_v1(&p.cid)?,
            size: p.size.unpadded().into(),
        })
    }
}

#[cfg(feature = "proofs")]
impl From<UnpaddedPieceSize> for filecoin_proofs_api::UnpaddedBytesAmount {
    fn from(p: UnpaddedPieceSize) -> Self {
        Self(p.0)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round_trip_piece_size() {
        let p_piece = PaddedPieceSize(0b10000000);
        p_piece.validate().unwrap();
        let up_piece = p_piece.unpadded();
        up_piece.validate().unwrap();
        assert_eq!(&up_piece, &UnpaddedPieceSize(127));
        assert_eq!(&p_piece, &up_piece.padded());
    }
    #[test]
    fn invalid_piece_checks() {
        let p = PaddedPieceSize(127);
        assert_eq!(p.validate(), Err("minimum piece size is 128 bytes"));
        let p = UnpaddedPieceSize(126);
        assert_eq!(p.validate(), Err("minimum piece size is 127 bytes"));
        let p = PaddedPieceSize(0b10000001);
        assert_eq!(p.validate(), Err("padded piece size must be a power of 2"));
        assert_eq!(UnpaddedPieceSize(0b1111111000).validate(), Ok(()));
        assert_eq!(
            UnpaddedPieceSize(0b1110111000).validate(),
            Err("unpadded piece size must be a power of 2 multiple of 127")
        );
    }
}
