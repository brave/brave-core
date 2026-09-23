// SPDX-License-Identifier: MIT

//! Provides the internal nuts and bolts that enable bech32 encoding/decoding.

pub mod checksum;
pub mod decode;
pub mod encode;
pub mod gf32;
pub mod hrp;
pub mod iter;
pub mod segwit;

use checksum::{Checksum, PackedNull};

/// The "null checksum" used on bech32 strings for which we want to do no checksum checking.
#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum NoChecksum {}

/// The bech32 checksum algorithm, defined in [BIP-173].
///
/// [BIP-173]: <https://github.com/bitcoin/bips/blob/master/bip-0173.mediawiki>
#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum Bech32 {}

/// The bech32m checksum algorithm, defined in [BIP-350].
///
/// [BIP-350]: <https://github.com/bitcoin/bips/blob/master/bip-0350.mediawiki>
#[derive(Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum Bech32m {}

impl Checksum for NoChecksum {
    type MidstateRepr = PackedNull;
    const CODE_LENGTH: usize = usize::MAX;
    const CHECKSUM_LENGTH: usize = 0;
    const GENERATOR_SH: [PackedNull; 5] = [PackedNull; 5];
    const TARGET_RESIDUE: PackedNull = PackedNull;
}

// Bech32[m] generator coefficients, copied from Bitcoin Core src/bech32.cpp
const GEN: [u32; 5] = [0x3b6a_57b2, 0x2650_8e6d, 0x1ea1_19fa, 0x3d42_33dd, 0x2a14_62b3];

impl Checksum for Bech32 {
    type MidstateRepr = u32;
    const CODE_LENGTH: usize = 1023;
    const CHECKSUM_LENGTH: usize = 6;
    const GENERATOR_SH: [u32; 5] = GEN;
    const TARGET_RESIDUE: u32 = 1;
}
// Same as Bech32 except TARGET_RESIDUE is different
impl Checksum for Bech32m {
    type MidstateRepr = u32;
    const CODE_LENGTH: usize = 1023;
    const CHECKSUM_LENGTH: usize = 6;
    const GENERATOR_SH: [u32; 5] = GEN;
    const TARGET_RESIDUE: u32 = 0x2bc830a3;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn bech32_sanity() { Bech32::sanity_check(); }

    #[test]
    fn bech32m_sanity() { Bech32m::sanity_check(); }
}
