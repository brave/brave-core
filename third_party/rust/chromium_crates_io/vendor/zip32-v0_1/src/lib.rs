//! Common types for implementing [ZIP 32] for hierarchical deterministic key management.
//!
//! [ZIP 32]: https://zips.z.cash/zip-0032

#![no_std]
#![deny(missing_docs)]
#![deny(unsafe_code)]
#![deny(rustdoc::broken_intra_doc_links)]

#[cfg(feature = "std")]
extern crate std;

use core::mem;

use memuse::{self, DynamicUsage};
use subtle::{Choice, ConditionallySelectable, ConstantTimeEq};

pub mod fingerprint;

/// A type-safe wrapper for account identifiers.
///
/// Accounts are 31-bit unsigned integers, and are always treated as hardened in
/// derivation paths.
#[derive(Debug, Default, Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct AccountId(u32);

memuse::impl_no_dynamic_usage!(AccountId);

impl TryFrom<u32> for AccountId {
    type Error = TryFromIntError;

    fn try_from(id: u32) -> Result<Self, Self::Error> {
        // Account IDs are always hardened in derivation paths, so they are effectively at
        // most 31 bits.
        if id < (1 << 31) {
            Ok(Self(id))
        } else {
            Err(TryFromIntError(()))
        }
    }
}

impl From<AccountId> for u32 {
    fn from(id: AccountId) -> Self {
        id.0
    }
}

impl From<AccountId> for ChildIndex {
    fn from(id: AccountId) -> Self {
        // Account IDs are always hardened in derivation paths.
        ChildIndex::hardened(id.0)
    }
}

impl ConditionallySelectable for AccountId {
    fn conditional_select(a0: &Self, a1: &Self, c: Choice) -> Self {
        AccountId(u32::conditional_select(&a0.0, &a1.0, c))
    }
}

impl AccountId {
    /// The ID for account zero (the first account).
    pub const ZERO: Self = Self(0);

    /// Returns the next account ID in sequence, or `None` on overflow.
    pub fn next(&self) -> Option<Self> {
        Self::try_from(self.0 + 1).ok()
    }
}

/// The error type returned when a checked integral type conversion fails.
#[derive(Clone, Copy, Debug)]
pub struct TryFromIntError(());

impl core::fmt::Display for TryFromIntError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "out of range integral type conversion attempted")
    }
}

#[cfg(feature = "std")]
impl std::error::Error for TryFromIntError {}

// ZIP 32 structures

/// A child index for a derived key.
///
/// Only hardened derivation is supported.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ChildIndex(u32);

impl ConstantTimeEq for ChildIndex {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0.ct_eq(&other.0)
    }
}

impl ChildIndex {
    /// Parses the given ZIP 32 child index.
    ///
    /// Returns `None` if the hardened bit is not set.
    pub fn from_index(i: u32) -> Option<Self> {
        if i >= (1 << 31) {
            Some(ChildIndex(i))
        } else {
            None
        }
    }

    /// Constructs a hardened `ChildIndex` from the given value.
    ///
    /// # Panics
    ///
    /// Panics if `value >= (1 << 31)`.
    pub const fn hardened(value: u32) -> Self {
        assert!(value < (1 << 31));
        Self(value + (1 << 31))
    }

    /// Returns the index as a 32-bit integer, including the hardened bit.
    pub fn index(&self) -> u32 {
        self.0
    }
}

/// A value that is needed, in addition to a spending key, in order to derive descendant
/// keys and addresses of that key.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ChainCode([u8; 32]);

impl ConstantTimeEq for ChainCode {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0.ct_eq(&other.0)
    }
}

impl ChainCode {
    /// Constructs a `ChainCode` from the given array.
    pub fn new(c: [u8; 32]) -> Self {
        Self(c)
    }

    /// Returns the byte representation of the chain code, as required for
    /// [ZIP 32](https://zips.z.cash/zip-0032) encoding.
    pub fn as_bytes(&self) -> &[u8; 32] {
        &self.0
    }
}

/// The index for a particular diversifier.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct DiversifierIndex([u8; 11]);

impl Default for DiversifierIndex {
    fn default() -> Self {
        DiversifierIndex::new()
    }
}

macro_rules! di_from {
    ($n:ident) => {
        impl From<$n> for DiversifierIndex {
            fn from(j: $n) -> Self {
                let mut j_bytes = [0; 11];
                j_bytes[..mem::size_of::<$n>()].copy_from_slice(&j.to_le_bytes());
                DiversifierIndex(j_bytes)
            }
        }
    };
}
di_from!(u32);
di_from!(u64);
di_from!(usize);

impl From<[u8; 11]> for DiversifierIndex {
    fn from(j_bytes: [u8; 11]) -> Self {
        DiversifierIndex(j_bytes)
    }
}

impl TryFrom<u128> for DiversifierIndex {
    type Error = TryFromIntError;

    fn try_from(value: u128) -> Result<Self, Self::Error> {
        if (value >> 88) == 0 {
            Ok(Self(value.to_le_bytes()[..11].try_into().unwrap()))
        } else {
            Err(TryFromIntError(()))
        }
    }
}

macro_rules! di_try_into {
    ($n:ident) => {
        impl TryFrom<DiversifierIndex> for $n {
            type Error = core::num::TryFromIntError;

            fn try_from(di: DiversifierIndex) -> Result<Self, Self::Error> {
                u128::from(di).try_into()
            }
        }
    };
}
di_try_into!(u32);
di_try_into!(u64);
di_try_into!(usize);

impl From<DiversifierIndex> for u128 {
    fn from(di: DiversifierIndex) -> Self {
        let mut u128_bytes = [0u8; 16];
        u128_bytes[0..11].copy_from_slice(&di.0[..]);
        u128::from_le_bytes(u128_bytes)
    }
}

impl DiversifierIndex {
    /// Constructs the zero index.
    pub fn new() -> Self {
        DiversifierIndex([0; 11])
    }

    /// Returns the raw bytes of the diversifier index.
    pub fn as_bytes(&self) -> &[u8; 11] {
        &self.0
    }

    /// Increments this index, failing on overflow.
    pub fn increment(&mut self) -> Result<(), DiversifierIndexOverflowError> {
        for k in 0..11 {
            self.0[k] = self.0[k].wrapping_add(1);
            if self.0[k] != 0 {
                // No overflow
                return Ok(());
            }
        }
        // Overflow
        Err(DiversifierIndexOverflowError)
    }
}

/// The error type returned when a [`DiversifierIndex`] increment fails.
#[derive(Clone, Copy, Debug)]
pub struct DiversifierIndexOverflowError;

impl core::fmt::Display for DiversifierIndexOverflowError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "DiversifierIndex increment overflowed")
    }
}

#[cfg(feature = "std")]
impl std::error::Error for DiversifierIndexOverflowError {}

/// The scope of a viewing key or address.
///
/// A "scope" narrows the visibility or usage to a level below "full".
///
/// Consistent usage of `Scope` enables the user to provide consistent views over a wallet
/// to other people. For example, a user can give an external incoming viewing key to a
/// merchant terminal, enabling it to only detect "real" transactions from customers and
/// not internal transactions from the wallet.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub enum Scope {
    /// A scope used for wallet-external operations, namely deriving addresses to give to
    /// other users in order to receive funds.
    External,
    /// A scope used for wallet-internal operations, such as creating change notes,
    /// auto-shielding, and note management.
    Internal,
}

memuse::impl_no_dynamic_usage!(Scope);

#[cfg(test)]
mod tests {
    use super::{AccountId, DiversifierIndex};

    use assert_matches::assert_matches;

    #[test]
    fn account_id_next() {
        let zero = AccountId::ZERO;
        assert_eq!(zero.next(), AccountId::try_from(1).ok());

        let max_id = AccountId::try_from((1 << 31) - 1).unwrap();
        assert_eq!(max_id.next(), None);
    }

    #[test]
    fn diversifier_index_to_u32() {
        let two = DiversifierIndex([
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        ]);
        assert_eq!(u32::try_from(two), Ok(2));
        assert_eq!(DiversifierIndex::from(2_u32), two);

        let max_u32 = DiversifierIndex([
            0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        ]);
        assert_eq!(u32::try_from(max_u32), Ok(u32::MAX));
        assert_eq!(DiversifierIndex::from(u32::MAX), max_u32);

        let too_big = DiversifierIndex([
            0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        ]);
        assert_matches!(u32::try_from(too_big), Err(_));
    }

    #[test]
    fn diversifier_index_to_u64() {
        let two = DiversifierIndex([
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        ]);
        assert_eq!(u64::try_from(two), Ok(2));
        assert_eq!(DiversifierIndex::from(2_u64), two);

        let max_u64 = DiversifierIndex([
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
        ]);
        assert_eq!(u64::try_from(max_u64), Ok(u64::MAX));
        assert_eq!(DiversifierIndex::from(u64::MAX), max_u64);

        let too_big = DiversifierIndex([
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        ]);
        assert_matches!(u64::try_from(too_big), Err(_));
    }

    #[test]
    fn diversifier_index_to_u128() {
        let two = DiversifierIndex([
            0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        ]);
        assert_eq!(u128::from(two), 2);
        assert_eq!(DiversifierIndex::try_from(2_u128).unwrap(), two);

        let max_di = DiversifierIndex([
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        ]);
        assert_eq!(u128::try_from(max_di), Ok(0x00ff_ffff_ffff_ffff_ffff_ffff));
        assert_eq!(
            DiversifierIndex::try_from(0x00ff_ffff_ffff_ffff_ffff_ffff_u128).unwrap(),
            max_di,
        );

        assert!(DiversifierIndex::try_from(0x0100_0000_0000_0000_0000_0000_u128).is_err());
    }

    #[test]
    fn diversifier_index_increment() {
        let mut di = DiversifierIndex::new();
        assert_eq!(di, 0_u32.into());

        assert_matches!(di.increment(), Ok(_));
        assert_eq!(di, 1_u32.into());

        assert_matches!(di.increment(), Ok(_));
        assert_eq!(di, 2_u32.into());

        let mut di = DiversifierIndex::from(0xff_u32);
        assert_eq!(di, 0x00ff_u32.into());

        assert_matches!(di.increment(), Ok(_));
        assert_eq!(di, 0x0100_u32.into());

        assert_matches!(di.increment(), Ok(_));
        assert_eq!(di, 0x0101_u32.into());

        let mut di = DiversifierIndex::try_from(0x00ff_ffff_ffff_ffff_ffff_fffe_u128).unwrap();
        assert_eq!(u128::from(di), 0x00ff_ffff_ffff_ffff_ffff_fffe_u128);

        assert_matches!(di.increment(), Ok(_));
        assert_eq!(u128::from(di), 0x00ff_ffff_ffff_ffff_ffff_ffff_u128);

        assert_matches!(di.increment(), Err(_));
    }
}
