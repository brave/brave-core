use subtle::CtOption;

use crate::{
    keys::{DiversifiedTransmissionKey, Diversifier},
    spec::{diversify_hash, NonIdentityPallasPoint},
};

/// A shielded payment address.
///
/// # Examples
///
/// ```
/// use orchard::keys::{SpendingKey, FullViewingKey, Scope};
///
/// let sk = SpendingKey::from_bytes([7; 32]).unwrap();
/// let address = FullViewingKey::from(&sk).address_at(0u32, Scope::External);
/// ```
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Address {
    d: Diversifier,
    pk_d: DiversifiedTransmissionKey,
}

impl Address {
    pub(crate) fn from_parts(d: Diversifier, pk_d: DiversifiedTransmissionKey) -> Self {
        // We assume here that pk_d is correctly-derived from d. We ensure this for
        // internal APIs. For parsing from raw byte encodings, we assume that users aren't
        // modifying internals of encoded address formats. If they do, that can result in
        // lost funds, but we can't defend against that from here.
        Address { d, pk_d }
    }

    /// Returns the [`Diversifier`] for this `Address`.
    pub fn diversifier(&self) -> Diversifier {
        self.d
    }

    pub(crate) fn g_d(&self) -> NonIdentityPallasPoint {
        diversify_hash(self.d.as_array())
    }

    pub(crate) fn pk_d(&self) -> &DiversifiedTransmissionKey {
        &self.pk_d
    }

    /// Serializes this address to its "raw" encoding as specified in [Zcash Protocol Spec § 5.6.4.2: Orchard Raw Payment Addresses][orchardpaymentaddrencoding]
    ///
    /// [orchardpaymentaddrencoding]: https://zips.z.cash/protocol/protocol.pdf#orchardpaymentaddrencoding
    pub fn to_raw_address_bytes(&self) -> [u8; 43] {
        let mut result = [0u8; 43];
        result[..11].copy_from_slice(self.d.as_array());
        result[11..].copy_from_slice(&self.pk_d.to_bytes());
        result
    }

    /// Parse an address from its "raw" encoding as specified in [Zcash Protocol Spec § 5.6.4.2: Orchard Raw Payment Addresses][orchardpaymentaddrencoding]
    ///
    /// [orchardpaymentaddrencoding]: https://zips.z.cash/protocol/protocol.pdf#orchardpaymentaddrencoding
    pub fn from_raw_address_bytes(bytes: &[u8; 43]) -> CtOption<Self> {
        DiversifiedTransmissionKey::from_bytes(bytes[11..].try_into().unwrap()).map(|pk_d| {
            let d = Diversifier::from_bytes(bytes[..11].try_into().unwrap());
            Self::from_parts(d, pk_d)
        })
    }
}

/// Generators for property testing.
#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub mod testing {
    use proptest::prelude::*;

    use crate::keys::{
        testing::{arb_diversifier_index, arb_spending_key},
        FullViewingKey, Scope,
    };

    use super::Address;

    prop_compose! {
        /// Generates an arbitrary payment address.
        pub(crate) fn arb_address()(sk in arb_spending_key(), j in arb_diversifier_index()) -> Address {
            let fvk = FullViewingKey::from(&sk);
            fvk.address_at(j, Scope::External)
        }
    }
}
