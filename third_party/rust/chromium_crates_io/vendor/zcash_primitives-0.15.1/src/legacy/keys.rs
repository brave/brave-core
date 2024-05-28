//! Transparent key components.

use hdwallet::{
    traits::{Deserialize, Serialize},
    ExtendedPrivKey, ExtendedPubKey, KeyIndex,
};
use secp256k1::PublicKey;
use sha2::{Digest, Sha256};
use subtle::{Choice, ConstantTimeEq};

use zcash_protocol::consensus::{self, NetworkConstants};
use zcash_spec::PrfExpand;
use zip32::AccountId;

use super::TransparentAddress;

/// The scope of a transparent key.
///
/// This type can represent [`zip32`] internal and external scopes, as well as custom scopes that
/// may be used in non-hardened derivation at the `change` level of the BIP 44 key path.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TransparentKeyScope(u32);

impl TransparentKeyScope {
    pub fn custom(i: u32) -> Option<Self> {
        if i < (1 << 31) {
            Some(TransparentKeyScope(i))
        } else {
            None
        }
    }
}

impl From<zip32::Scope> for TransparentKeyScope {
    fn from(value: zip32::Scope) -> Self {
        match value {
            zip32::Scope::External => TransparentKeyScope(0),
            zip32::Scope::Internal => TransparentKeyScope(1),
        }
    }
}

impl From<TransparentKeyScope> for KeyIndex {
    fn from(value: TransparentKeyScope) -> Self {
        KeyIndex::Normal(value.0)
    }
}

/// A child index for a derived transparent address.
///
/// Only NON-hardened derivation is supported.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct NonHardenedChildIndex(u32);

impl ConstantTimeEq for NonHardenedChildIndex {
    fn ct_eq(&self, other: &Self) -> Choice {
        self.0.ct_eq(&other.0)
    }
}

impl NonHardenedChildIndex {
    pub const ZERO: NonHardenedChildIndex = NonHardenedChildIndex(0);

    /// Parses the given ZIP 32 child index.
    ///
    /// Returns `None` if the hardened bit is set.
    pub fn from_index(i: u32) -> Option<Self> {
        if i < (1 << 31) {
            Some(NonHardenedChildIndex(i))
        } else {
            None
        }
    }

    /// Returns the index as a 32-bit integer.
    pub fn index(&self) -> u32 {
        self.0
    }

    pub fn next(&self) -> Option<Self> {
        // overflow cannot happen because self.0 is 31 bits, and the next index is at most 32 bits
        // which in that case would lead from_index to return None.
        Self::from_index(self.0 + 1)
    }
}

impl TryFrom<KeyIndex> for NonHardenedChildIndex {
    type Error = ();

    fn try_from(value: KeyIndex) -> Result<Self, Self::Error> {
        match value {
            KeyIndex::Normal(i) => NonHardenedChildIndex::from_index(i).ok_or(()),
            KeyIndex::Hardened(_) => Err(()),
        }
    }
}

impl From<NonHardenedChildIndex> for KeyIndex {
    fn from(value: NonHardenedChildIndex) -> Self {
        Self::Normal(value.index())
    }
}

/// A [BIP44] private key at the account path level `m/44'/<coin_type>'/<account>'`.
///
/// [BIP44]: https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
#[derive(Clone, Debug)]
pub struct AccountPrivKey(ExtendedPrivKey);

impl AccountPrivKey {
    /// Performs derivation of the extended private key for the BIP44 path:
    /// `m/44'/<coin_type>'/<account>'`.
    ///
    /// This produces the root of the derivation tree for transparent
    /// viewing keys and addresses for the for the provided account.
    pub fn from_seed<P: consensus::Parameters>(
        params: &P,
        seed: &[u8],
        account: AccountId,
    ) -> Result<AccountPrivKey, hdwallet::error::Error> {
        ExtendedPrivKey::with_seed(seed)?
            .derive_private_key(KeyIndex::hardened_from_normalize_index(44)?)?
            .derive_private_key(KeyIndex::hardened_from_normalize_index(params.coin_type())?)?
            .derive_private_key(KeyIndex::hardened_from_normalize_index(account.into())?)
            .map(AccountPrivKey)
    }

    pub fn from_extended_privkey(extprivkey: ExtendedPrivKey) -> Self {
        AccountPrivKey(extprivkey)
    }

    pub fn to_account_pubkey(&self) -> AccountPubKey {
        AccountPubKey(ExtendedPubKey::from_private_key(&self.0))
    }

    /// Derives the BIP44 private spending key for the child path
    /// `m/44'/<coin_type>'/<account>'/<scope>/<child_index>`.
    pub fn derive_secret_key(
        &self,
        scope: TransparentKeyScope,
        child_index: NonHardenedChildIndex,
    ) -> Result<secp256k1::SecretKey, hdwallet::error::Error> {
        self.0
            .derive_private_key(scope.into())?
            .derive_private_key(child_index.into())
            .map(|k| k.private_key)
    }

    /// Derives the BIP44 private spending key for the external (incoming payment) child path
    /// `m/44'/<coin_type>'/<account>'/0/<child_index>`.
    pub fn derive_external_secret_key(
        &self,
        child_index: NonHardenedChildIndex,
    ) -> Result<secp256k1::SecretKey, hdwallet::error::Error> {
        self.derive_secret_key(zip32::Scope::External.into(), child_index)
    }

    /// Derives the BIP44 private spending key for the internal (change) child path
    /// `m/44'/<coin_type>'/<account>'/1/<child_index>`.
    pub fn derive_internal_secret_key(
        &self,
        child_index: NonHardenedChildIndex,
    ) -> Result<secp256k1::SecretKey, hdwallet::error::Error> {
        self.derive_secret_key(zip32::Scope::Internal.into(), child_index)
    }

    /// Returns the `AccountPrivKey` serialized using the encoding for a
    /// [BIP 32](https://en.bitcoin.it/wiki/BIP_0032) ExtendedPrivKey
    pub fn to_bytes(&self) -> Vec<u8> {
        self.0.serialize()
    }

    /// Decodes the `AccountPrivKey` from the encoding specified for a
    /// [BIP 32](https://en.bitcoin.it/wiki/BIP_0032) ExtendedPrivKey
    pub fn from_bytes(b: &[u8]) -> Option<Self> {
        ExtendedPrivKey::deserialize(b)
            .map(AccountPrivKey::from_extended_privkey)
            .ok()
    }
}

/// A [BIP44] public key at the account path level `m/44'/<coin_type>'/<account>'`.
///
/// This provides the necessary derivation capability for the transparent component of a unified
/// full viewing key.
///
/// [BIP44]: https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
#[derive(Clone, Debug)]
pub struct AccountPubKey(ExtendedPubKey);

impl AccountPubKey {
    /// Derives the BIP44 public key at the external "change level" path
    /// `m/44'/<coin_type>'/<account>'/0`.
    pub fn derive_external_ivk(&self) -> Result<ExternalIvk, hdwallet::error::Error> {
        self.0
            .derive_public_key(KeyIndex::Normal(0))
            .map(ExternalIvk)
    }

    /// Derives the BIP44 public key at the internal "change level" path
    /// `m/44'/<coin_type>'/<account>'/1`.
    pub fn derive_internal_ivk(&self) -> Result<InternalIvk, hdwallet::error::Error> {
        self.0
            .derive_public_key(KeyIndex::Normal(1))
            .map(InternalIvk)
    }

    /// Derives the internal ovk and external ovk corresponding to this
    /// transparent fvk. As specified in [ZIP 316][transparent-ovk].
    ///
    /// [transparent-ovk]: https://zips.z.cash/zip-0316#deriving-internal-keys
    pub fn ovks_for_shielding(&self) -> (InternalOvk, ExternalOvk) {
        let i_ovk = PrfExpand::TRANSPARENT_ZIP316_OVK
            .with(&self.0.chain_code, &self.0.public_key.serialize());
        let ovk_external = ExternalOvk(i_ovk[..32].try_into().unwrap());
        let ovk_internal = InternalOvk(i_ovk[32..].try_into().unwrap());

        (ovk_internal, ovk_external)
    }

    /// Derives the internal ovk corresponding to this transparent fvk.
    pub fn internal_ovk(&self) -> InternalOvk {
        self.ovks_for_shielding().0
    }

    /// Derives the external ovk corresponding to this transparent fvk.
    pub fn external_ovk(&self) -> ExternalOvk {
        self.ovks_for_shielding().1
    }

    pub fn serialize(&self) -> Vec<u8> {
        let mut buf = self.0.chain_code.clone();
        buf.extend(self.0.public_key.serialize().to_vec());
        buf
    }

    pub fn deserialize(data: &[u8; 65]) -> Result<Self, hdwallet::error::Error> {
        let chain_code = data[..32].to_vec();
        let public_key = PublicKey::from_slice(&data[32..])?;
        Ok(AccountPubKey(ExtendedPubKey {
            public_key,
            chain_code,
        }))
    }
}

/// Derives the P2PKH transparent address corresponding to the given pubkey.
#[deprecated(note = "This function will be removed from the public API in an upcoming refactor.")]
pub fn pubkey_to_address(pubkey: &secp256k1::PublicKey) -> TransparentAddress {
    TransparentAddress::PublicKeyHash(
        *ripemd::Ripemd160::digest(Sha256::digest(pubkey.serialize())).as_ref(),
    )
}

pub(crate) mod private {
    use hdwallet::ExtendedPubKey;
    pub trait SealedChangeLevelKey {
        fn extended_pubkey(&self) -> &ExtendedPubKey;
        fn from_extended_pubkey(key: ExtendedPubKey) -> Self;
    }
}

/// Trait representing a transparent "incoming viewing key".
///
/// Unlike the Sapling and Orchard shielded protocols (which have viewing keys built into
/// their key trees and bound to specific spending keys), the transparent protocol has no
/// "viewing key" concept. Transparent viewing keys are instead emulated by making two
/// observations:
///
/// - [BIP32] hierarchical derivation is structured as a tree.
/// - The [BIP44] key paths use non-hardened derivation below the account level.
///
/// A transparent viewing key for an account is thus defined as the root of a specific
/// non-hardened subtree underneath the account's path.
///
/// [BIP32]: https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki
/// [BIP44]: https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
pub trait IncomingViewingKey: private::SealedChangeLevelKey + std::marker::Sized {
    /// Derives a transparent address at the provided child index.
    #[allow(deprecated)]
    fn derive_address(
        &self,
        child_index: NonHardenedChildIndex,
    ) -> Result<TransparentAddress, hdwallet::error::Error> {
        let child_key = self
            .extended_pubkey()
            .derive_public_key(child_index.into())?;
        Ok(pubkey_to_address(&child_key.public_key))
    }

    /// Searches the space of child indexes for an index that will
    /// generate a valid transparent address, and returns the resulting
    /// address and the index at which it was generated.
    fn default_address(&self) -> (TransparentAddress, NonHardenedChildIndex) {
        let mut child_index = NonHardenedChildIndex::ZERO;
        loop {
            match self.derive_address(child_index) {
                Ok(addr) => {
                    return (addr, child_index);
                }
                Err(_) => {
                    child_index = child_index.next().unwrap_or_else(|| {
                        panic!("Exhausted child index space attempting to find a default address.");
                    });
                }
            }
        }
    }

    fn serialize(&self) -> Vec<u8> {
        let extpubkey = self.extended_pubkey();
        let mut buf = extpubkey.chain_code.clone();
        buf.extend(extpubkey.public_key.serialize().to_vec());
        buf
    }

    fn deserialize(data: &[u8; 65]) -> Result<Self, hdwallet::error::Error> {
        let chain_code = data[..32].to_vec();
        let public_key = PublicKey::from_slice(&data[32..])?;
        Ok(Self::from_extended_pubkey(ExtendedPubKey {
            public_key,
            chain_code,
        }))
    }
}

/// An incoming viewing key at the [BIP44] "external" path
/// `m/44'/<coin_type>'/<account>'/0`.
///
/// This allows derivation of child addresses that may be provided to external parties.
///
/// [BIP44]: https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
#[derive(Clone, Debug)]
pub struct ExternalIvk(ExtendedPubKey);

impl private::SealedChangeLevelKey for ExternalIvk {
    fn extended_pubkey(&self) -> &ExtendedPubKey {
        &self.0
    }

    fn from_extended_pubkey(key: ExtendedPubKey) -> Self {
        ExternalIvk(key)
    }
}

impl IncomingViewingKey for ExternalIvk {}

/// An incoming viewing key at the [BIP44] "internal" path
/// `m/44'/<coin_type>'/<account>'/1`.
///
/// This allows derivation of change addresses for use within the wallet, but which should
/// not be shared with external parties.
///
/// [BIP44]: https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki
#[derive(Clone, Debug)]
pub struct InternalIvk(ExtendedPubKey);

impl private::SealedChangeLevelKey for InternalIvk {
    fn extended_pubkey(&self) -> &ExtendedPubKey {
        &self.0
    }

    fn from_extended_pubkey(key: ExtendedPubKey) -> Self {
        InternalIvk(key)
    }
}

impl IncomingViewingKey for InternalIvk {}

/// Internal outgoing viewing key used for autoshielding.
pub struct InternalOvk([u8; 32]);

impl InternalOvk {
    pub fn as_bytes(&self) -> [u8; 32] {
        self.0
    }
}

/// External outgoing viewing key used by `zcashd` for transparent-to-shielded spends to
/// external receivers.
pub struct ExternalOvk([u8; 32]);

impl ExternalOvk {
    pub fn as_bytes(&self) -> [u8; 32] {
        self.0
    }
}

#[cfg(test)]
mod tests {
    use hdwallet::KeyIndex;
    use subtle::ConstantTimeEq;

    use super::AccountPubKey;
    use super::NonHardenedChildIndex;

    #[test]
    fn check_ovk_test_vectors() {
        struct TestVector {
            c: [u8; 32],
            pk: [u8; 33],
            external_ovk: [u8; 32],
            internal_ovk: [u8; 32],
        }

        // From https://github.com/zcash-hackworks/zcash-test-vectors/blob/master/zip_0316.py
        let test_vectors = vec![
            TestVector {
                c: [
                    0x5d, 0x7a, 0x8f, 0x73, 0x9a, 0x2d, 0x9e, 0x94, 0x5b, 0x0c, 0xe1, 0x52, 0xa8,
                    0x04, 0x9e, 0x29, 0x4c, 0x4d, 0x6e, 0x66, 0xb1, 0x64, 0x93, 0x9d, 0xaf, 0xfa,
                    0x2e, 0xf6, 0xee, 0x69, 0x21, 0x48,
                ],
                pk: [
                    0x02, 0x16, 0x88, 0x4f, 0x1d, 0xbc, 0x92, 0x90, 0x89, 0xa4, 0x17, 0x6e, 0x84,
                    0x0b, 0xb5, 0x81, 0xc8, 0x0e, 0x16, 0xe9, 0xb1, 0xab, 0xd6, 0x54, 0xe6, 0x2c,
                    0x8b, 0x0b, 0x95, 0x70, 0x20, 0xb7, 0x48,
                ],
                external_ovk: [
                    0xdc, 0xe7, 0xfb, 0x7f, 0x20, 0xeb, 0x77, 0x64, 0xd5, 0x12, 0x4f, 0xbd, 0x23,
                    0xc4, 0xd7, 0xca, 0x8c, 0x32, 0x19, 0xec, 0x1d, 0xb3, 0xff, 0x1e, 0x08, 0x13,
                    0x50, 0xad, 0x03, 0x9b, 0x40, 0x79,
                ],
                internal_ovk: [
                    0x4d, 0x46, 0xc7, 0x14, 0xed, 0xda, 0xd9, 0x4a, 0x40, 0xac, 0x21, 0x28, 0x6a,
                    0xff, 0x32, 0x7d, 0x7e, 0xbf, 0x11, 0x9e, 0x86, 0x85, 0x10, 0x9b, 0x44, 0xe8,
                    0x02, 0x83, 0xd8, 0xc8, 0xa4, 0x00,
                ],
            },
            TestVector {
                c: [
                    0xbf, 0x69, 0xb8, 0x25, 0x0c, 0x18, 0xef, 0x41, 0x29, 0x4c, 0xa9, 0x79, 0x93,
                    0xdb, 0x54, 0x6c, 0x1f, 0xe0, 0x1f, 0x7e, 0x9c, 0x8e, 0x36, 0xd6, 0xa5, 0xe2,
                    0x9d, 0x4e, 0x30, 0xa7, 0x35, 0x94,
                ],
                pk: [
                    0x03, 0x72, 0x73, 0xb6, 0x57, 0xd9, 0x71, 0xa4, 0x5e, 0x72, 0x24, 0x0c, 0x7a,
                    0xaa, 0xa7, 0xd0, 0x68, 0x5d, 0x06, 0xd7, 0x99, 0x9b, 0x0a, 0x19, 0xc4, 0xce,
                    0xa3, 0x27, 0x88, 0xa6, 0xab, 0x51, 0x3d,
                ],
                external_ovk: [
                    0x8d, 0x31, 0x53, 0x7b, 0x38, 0x8f, 0x40, 0x23, 0xe6, 0x48, 0x70, 0x8b, 0xfb,
                    0xde, 0x2b, 0xa1, 0xff, 0x1a, 0x4e, 0xe1, 0x12, 0xea, 0x67, 0x0a, 0xd1, 0x67,
                    0x44, 0xf4, 0x58, 0x3e, 0x95, 0x52,
                ],
                internal_ovk: [
                    0x16, 0x77, 0x49, 0x00, 0x76, 0x9d, 0x9c, 0x03, 0xbe, 0x06, 0x32, 0x45, 0xcf,
                    0x1c, 0x22, 0x44, 0xa9, 0x2e, 0x48, 0x51, 0x01, 0x54, 0x73, 0x61, 0x3f, 0xbf,
                    0x38, 0xd2, 0x42, 0xd7, 0x54, 0xf6,
                ],
            },
            TestVector {
                c: [
                    0x3d, 0xc1, 0x66, 0xd5, 0x6a, 0x1d, 0x62, 0xf5, 0xa8, 0xd7, 0x55, 0x1d, 0xb5,
                    0xfd, 0x93, 0x13, 0xe8, 0xc7, 0x20, 0x3d, 0x99, 0x6a, 0xf7, 0xd4, 0x77, 0x08,
                    0x37, 0x56, 0xd5, 0x9a, 0xf8, 0x0d,
                ],
                pk: [
                    0x03, 0xec, 0x05, 0xbb, 0x7f, 0x06, 0x5e, 0x25, 0x6f, 0xf4, 0x54, 0xf8, 0xa8,
                    0xdf, 0x6f, 0x2f, 0x9b, 0x8a, 0x8c, 0x95, 0x08, 0xca, 0xac, 0xfe, 0xe9, 0x52,
                    0x1c, 0xbe, 0x68, 0x9d, 0xd1, 0x12, 0x0f,
                ],
                external_ovk: [
                    0xdb, 0x97, 0x52, 0x0e, 0x2f, 0xe3, 0x68, 0xad, 0x50, 0x2d, 0xef, 0xf8, 0x42,
                    0xf0, 0xc0, 0xee, 0x5d, 0x20, 0x3b, 0x48, 0x33, 0x7a, 0x0f, 0xff, 0x75, 0xbe,
                    0x24, 0x52, 0x59, 0x77, 0xf3, 0x7e,
                ],
                internal_ovk: [
                    0xbc, 0x4a, 0xcb, 0x5f, 0x52, 0xb8, 0xae, 0x21, 0xe3, 0x32, 0xb1, 0x7c, 0x29,
                    0x63, 0x1f, 0x68, 0xe9, 0x68, 0x2a, 0x46, 0xc4, 0xa7, 0xab, 0xc8, 0xed, 0xf9,
                    0x0d, 0x37, 0xae, 0xea, 0xd3, 0x6c,
                ],
            },
            TestVector {
                c: [
                    0x49, 0x5c, 0x22, 0x2f, 0x7f, 0xba, 0x1e, 0x31, 0xde, 0xfa, 0x3d, 0x5a, 0x57,
                    0xef, 0xc2, 0xe1, 0xe9, 0xb0, 0x1a, 0x03, 0x55, 0x87, 0xd5, 0xfb, 0x1a, 0x38,
                    0xe0, 0x1d, 0x94, 0x90, 0x3d, 0x3c,
                ],
                pk: [
                    0x02, 0x81, 0x8f, 0x50, 0xce, 0x47, 0x10, 0xf4, 0xeb, 0x11, 0xe7, 0x43, 0xe6,
                    0x40, 0x85, 0x44, 0xaa, 0x3c, 0x12, 0x3c, 0x7f, 0x07, 0xe2, 0xaa, 0xbb, 0x91,
                    0xaf, 0xc4, 0xec, 0x48, 0x78, 0x8d, 0xe9,
                ],
                external_ovk: [
                    0xb8, 0xa3, 0x6d, 0x62, 0xa6, 0x3f, 0x69, 0x36, 0x7b, 0xe3, 0xf4, 0xbe, 0xd4,
                    0x20, 0x26, 0x4a, 0xdb, 0x63, 0x7b, 0xbb, 0x47, 0x0e, 0x1f, 0x56, 0xe0, 0x33,
                    0x8b, 0x38, 0xe2, 0xa6, 0x90, 0x97,
                ],
                internal_ovk: [
                    0x4f, 0xf6, 0xfa, 0xf2, 0x06, 0x63, 0x1e, 0xcb, 0x01, 0xf9, 0x57, 0x30, 0xf7,
                    0xe5, 0x5b, 0xfc, 0xff, 0x8b, 0x02, 0xa3, 0x14, 0x88, 0x5a, 0x6d, 0x24, 0x8e,
                    0x6e, 0xbe, 0xb7, 0x4d, 0x3e, 0x50,
                ],
            },
            TestVector {
                c: [
                    0xa7, 0xaf, 0x9d, 0xb6, 0x99, 0x0e, 0xd8, 0x3d, 0xd6, 0x4a, 0xf3, 0x59, 0x7c,
                    0x04, 0x32, 0x3e, 0xa5, 0x1b, 0x00, 0x52, 0xad, 0x80, 0x84, 0xa8, 0xb9, 0xda,
                    0x94, 0x8d, 0x32, 0x0d, 0xad, 0xd6,
                ],
                pk: [
                    0x02, 0xae, 0x36, 0xb6, 0x1a, 0x3d, 0x10, 0xf1, 0xaa, 0x75, 0x2a, 0xb1, 0xdc,
                    0x16, 0xe3, 0xe4, 0x9b, 0x6a, 0xc0, 0xd2, 0xae, 0x19, 0x07, 0xd2, 0xe6, 0x94,
                    0x25, 0xec, 0x12, 0xc9, 0x3a, 0xae, 0xbc,
                ],
                external_ovk: [
                    0xda, 0x6f, 0x47, 0x0f, 0x42, 0x5b, 0x3d, 0x27, 0xf4, 0x28, 0x6e, 0xf0, 0x3b,
                    0x7e, 0x87, 0x01, 0x7c, 0x20, 0xa7, 0x10, 0xb3, 0xff, 0xb9, 0xc1, 0xb6, 0x6c,
                    0x71, 0x60, 0x92, 0xe3, 0xd9, 0xbc,
                ],
                internal_ovk: [
                    0x09, 0xb5, 0x4f, 0x75, 0xcb, 0x70, 0x32, 0x67, 0x1d, 0xc6, 0x8a, 0xaa, 0x07,
                    0x30, 0x5f, 0x38, 0xcd, 0xbc, 0x87, 0x9e, 0xe1, 0x5b, 0xec, 0x04, 0x71, 0x3c,
                    0x24, 0xdc, 0xe3, 0xca, 0x70, 0x26,
                ],
            },
            TestVector {
                c: [
                    0xe0, 0x0c, 0x7a, 0x1d, 0x48, 0xaf, 0x04, 0x68, 0x27, 0x59, 0x1e, 0x97, 0x33,
                    0xa9, 0x7f, 0xa6, 0xb6, 0x79, 0xf3, 0xdc, 0x60, 0x1d, 0x00, 0x82, 0x85, 0xed,
                    0xcb, 0xda, 0xe6, 0x9c, 0xe8, 0xfc,
                ],
                pk: [
                    0x02, 0x49, 0x26, 0x53, 0x80, 0xd2, 0xb0, 0x2e, 0x0a, 0x1d, 0x98, 0x8f, 0x3d,
                    0xe3, 0x45, 0x8b, 0x6e, 0x00, 0x29, 0x1d, 0xb0, 0xe6, 0x2e, 0x17, 0x47, 0x91,
                    0xd0, 0x09, 0x29, 0x9f, 0x61, 0xfe, 0xc4,
                ],
                external_ovk: [
                    0x60, 0xa7, 0xa0, 0x8e, 0xef, 0xa2, 0x4e, 0x75, 0xcc, 0xbb, 0x29, 0xdc, 0x84,
                    0x94, 0x67, 0x2d, 0x73, 0x0f, 0xb3, 0x88, 0x7c, 0xb2, 0x6e, 0xf5, 0x1c, 0x6a,
                    0x1a, 0x78, 0xe8, 0x8a, 0x78, 0x39,
                ],
                internal_ovk: [
                    0x3b, 0xab, 0x40, 0x98, 0x08, 0x10, 0x8b, 0xa9, 0xe5, 0xa1, 0xbb, 0x6a, 0x42,
                    0x24, 0x59, 0x9d, 0x62, 0xcc, 0xee, 0x63, 0xff, 0x2f, 0x38, 0x15, 0x4c, 0x7f,
                    0xb0, 0xc9, 0xa9, 0xa5, 0x79, 0x0f,
                ],
            },
            TestVector {
                c: [
                    0xe2, 0x88, 0x53, 0x15, 0xeb, 0x46, 0x71, 0x09, 0x8b, 0x79, 0x53, 0x5e, 0x79,
                    0x0f, 0xe5, 0x3e, 0x29, 0xfe, 0xf2, 0xb3, 0x76, 0x66, 0x97, 0xac, 0x32, 0xb4,
                    0xf4, 0x73, 0xf4, 0x68, 0xa0, 0x08,
                ],
                pk: [
                    0x03, 0x9a, 0x0e, 0x46, 0x39, 0xb4, 0x69, 0x1f, 0x02, 0x7c, 0x0d, 0xb7, 0xfe,
                    0xf1, 0xbb, 0x5e, 0xf9, 0x0a, 0xcd, 0xb7, 0x08, 0x62, 0x6d, 0x2e, 0x1f, 0x3e,
                    0x38, 0x3e, 0xe7, 0x5b, 0x31, 0xcf, 0x57,
                ],
                external_ovk: [
                    0xbb, 0x47, 0x87, 0x2c, 0x25, 0x09, 0xbf, 0x3c, 0x72, 0xde, 0xdf, 0x4f, 0xc1,
                    0x77, 0x0f, 0x91, 0x93, 0xe2, 0xc1, 0x90, 0xd7, 0xaa, 0x8e, 0x9e, 0x88, 0x1a,
                    0xd2, 0xf1, 0x73, 0x48, 0x4e, 0xf2,
                ],
                internal_ovk: [
                    0x5f, 0x36, 0xdf, 0xa3, 0x6c, 0xa7, 0x65, 0x74, 0x50, 0x29, 0x4e, 0xaa, 0xdd,
                    0xad, 0x78, 0xaf, 0xf2, 0xb3, 0xdc, 0x38, 0x5a, 0x57, 0x73, 0x5a, 0xc0, 0x0d,
                    0x3d, 0x9a, 0x29, 0x2b, 0x8c, 0x77,
                ],
            },
            TestVector {
                c: [
                    0xed, 0x94, 0x94, 0xc6, 0xac, 0x89, 0x3c, 0x49, 0x72, 0x38, 0x33, 0xec, 0x89,
                    0x26, 0xc1, 0x03, 0x95, 0x86, 0xa7, 0xaf, 0xcf, 0x4a, 0x0d, 0x9c, 0x73, 0x1e,
                    0x98, 0x5d, 0x99, 0x58, 0x9c, 0x8b,
                ],
                pk: [
                    0x03, 0xbb, 0xf4, 0x49, 0x82, 0xf1, 0xba, 0x3a, 0x2b, 0x9d, 0xd3, 0xc1, 0x77,
                    0x4d, 0x71, 0xce, 0x33, 0x60, 0x59, 0x9b, 0x07, 0xf2, 0x11, 0xc8, 0x16, 0xb8,
                    0xc4, 0x3b, 0x98, 0x42, 0x23, 0x09, 0x24,
                ],
                external_ovk: [
                    0xed, 0xe8, 0xfb, 0x11, 0x37, 0x9b, 0x15, 0xae, 0xc4, 0xfa, 0x4e, 0xc5, 0x12,
                    0x4c, 0x95, 0x00, 0xad, 0xf4, 0x0e, 0xb6, 0xf7, 0xca, 0xa5, 0xe9, 0xce, 0x80,
                    0xf6, 0xbd, 0x9e, 0x73, 0xd0, 0xe7,
                ],
                internal_ovk: [
                    0x25, 0x0b, 0x4d, 0xfc, 0x34, 0xdd, 0x57, 0x76, 0x74, 0x51, 0x57, 0xf3, 0x82,
                    0xce, 0x6d, 0xe4, 0xf6, 0xfe, 0x22, 0xd7, 0x98, 0x02, 0xf3, 0x9f, 0xe1, 0x34,
                    0x77, 0x8b, 0x79, 0x40, 0x42, 0xd3,
                ],
            },
            TestVector {
                c: [
                    0x92, 0x47, 0x69, 0x30, 0xd0, 0x69, 0x89, 0x6c, 0xff, 0x30, 0xeb, 0x41, 0x4f,
                    0x72, 0x7b, 0x89, 0xe0, 0x01, 0xaf, 0xa2, 0xfb, 0x8d, 0xc3, 0x43, 0x6d, 0x75,
                    0xa4, 0xa6, 0xf2, 0x65, 0x72, 0x50,
                ],
                pk: [
                    0x03, 0xff, 0x63, 0xc7, 0x89, 0x25, 0x1c, 0x10, 0x43, 0xc6, 0xf9, 0x6c, 0x66,
                    0xbf, 0x5b, 0x0f, 0x61, 0xc9, 0xd6, 0x5f, 0xef, 0x5a, 0xaf, 0x42, 0x84, 0xa6,
                    0xa5, 0x69, 0x94, 0x94, 0x1c, 0x05, 0xfa,
                ],
                external_ovk: [
                    0xb3, 0x11, 0x52, 0x06, 0x42, 0x71, 0x01, 0x01, 0xbb, 0xc8, 0x1b, 0xbe, 0x92,
                    0x85, 0x1f, 0x9e, 0x65, 0x36, 0x22, 0x3e, 0xd6, 0xe6, 0xa1, 0x28, 0x59, 0x06,
                    0x62, 0x1e, 0xfa, 0xe6, 0x41, 0x10,
                ],
                internal_ovk: [
                    0xf4, 0x46, 0xc0, 0xc1, 0x74, 0x1c, 0x94, 0x42, 0x56, 0x8e, 0x12, 0xf0, 0x55,
                    0xef, 0xd5, 0x0c, 0x1e, 0xfe, 0x4d, 0x71, 0x53, 0x3d, 0x97, 0x6b, 0x08, 0xe9,
                    0x94, 0x41, 0x44, 0x49, 0xc4, 0xac,
                ],
            },
            TestVector {
                c: [
                    0x7d, 0x41, 0x7a, 0xdb, 0x3d, 0x15, 0xcc, 0x54, 0xdc, 0xb1, 0xfc, 0xe4, 0x67,
                    0x50, 0x0c, 0x6b, 0x8f, 0xb8, 0x6b, 0x12, 0xb5, 0x6d, 0xa9, 0xc3, 0x82, 0x85,
                    0x7d, 0xee, 0xcc, 0x40, 0xa9, 0x8d,
                ],
                pk: [
                    0x02, 0xbf, 0x39, 0x20, 0xce, 0x2e, 0x9e, 0x95, 0xb0, 0xee, 0xce, 0x13, 0x0a,
                    0x50, 0xba, 0x7d, 0xcc, 0x6f, 0x26, 0x51, 0x2a, 0x9f, 0xc7, 0xb8, 0x04, 0xaf,
                    0xf0, 0x89, 0xf5, 0x0c, 0xbc, 0xff, 0xf7,
                ],
                external_ovk: [
                    0xae, 0x63, 0x84, 0xf8, 0x07, 0x72, 0x1c, 0x5f, 0x46, 0xc8, 0xaa, 0x83, 0x3b,
                    0x66, 0x9b, 0x01, 0xc4, 0x22, 0x7c, 0x00, 0x18, 0xcb, 0x27, 0x29, 0xa9, 0x79,
                    0x91, 0x01, 0xea, 0xb8, 0x5a, 0xb9,
                ],
                internal_ovk: [
                    0xef, 0x70, 0x8e, 0xb8, 0x26, 0xd8, 0xbf, 0xcd, 0x7f, 0xaa, 0x4f, 0x90, 0xdf,
                    0x46, 0x1d, 0xed, 0x08, 0xd1, 0x6e, 0x19, 0x1b, 0x4e, 0x51, 0xb8, 0xa3, 0xa9,
                    0x1c, 0x02, 0x0b, 0x32, 0xcc, 0x07,
                ],
            },
        ];

        for tv in test_vectors {
            let mut key_bytes = [0u8; 65];
            key_bytes[..32].copy_from_slice(&tv.c);
            key_bytes[32..].copy_from_slice(&tv.pk);
            let account_key = AccountPubKey::deserialize(&key_bytes).unwrap();

            let (internal, external) = account_key.ovks_for_shielding();

            assert_eq!(tv.internal_ovk, internal.as_bytes());
            assert_eq!(tv.external_ovk, external.as_bytes());
        }
    }

    #[test]
    fn nonhardened_indexes_accepted() {
        assert_eq!(0, NonHardenedChildIndex::from_index(0).unwrap().index());
        assert_eq!(
            0x7fffffff,
            NonHardenedChildIndex::from_index(0x7fffffff)
                .unwrap()
                .index()
        );
    }

    #[test]
    fn hardened_indexes_rejected() {
        assert!(NonHardenedChildIndex::from_index(0x80000000).is_none());
        assert!(NonHardenedChildIndex::from_index(0xffffffff).is_none());
    }

    #[test]
    fn nonhardened_index_next() {
        assert_eq!(1, NonHardenedChildIndex::ZERO.next().unwrap().index());
        assert!(NonHardenedChildIndex::from_index(0x7fffffff)
            .unwrap()
            .next()
            .is_none());
    }

    #[test]
    fn nonhardened_index_ct_eq() {
        assert!(check(
            NonHardenedChildIndex::ZERO,
            NonHardenedChildIndex::ZERO
        ));
        assert!(!check(
            NonHardenedChildIndex::ZERO,
            NonHardenedChildIndex::ZERO.next().unwrap()
        ));

        fn check<T: ConstantTimeEq>(v1: T, v2: T) -> bool {
            v1.ct_eq(&v2).into()
        }
    }

    #[test]
    fn nonhardened_index_tryfrom_keyindex() {
        let nh: NonHardenedChildIndex = KeyIndex::Normal(0).try_into().unwrap();
        assert_eq!(nh.index(), 0);

        assert!(NonHardenedChildIndex::try_from(KeyIndex::Hardened(0)).is_err());
    }
}
