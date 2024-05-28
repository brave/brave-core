use super::{
    keys::{DiversifiedTransmissionKey, Diversifier},
    note::{Note, Rseed},
    value::NoteValue,
};

/// A Sapling payment address.
///
/// # Invariants
///
/// - `diversifier` is guaranteed to be valid for Sapling (only 50% of diversifiers are).
/// - `pk_d` is guaranteed to be prime-order (i.e. in the prime-order subgroup of Jubjub,
///  and not the identity).
#[derive(Clone, Copy, Debug)]
pub struct PaymentAddress {
    pk_d: DiversifiedTransmissionKey,
    diversifier: Diversifier,
}

impl PartialEq for PaymentAddress {
    fn eq(&self, other: &Self) -> bool {
        self.pk_d == other.pk_d && self.diversifier == other.diversifier
    }
}

impl Eq for PaymentAddress {}

impl PaymentAddress {
    /// Constructs a PaymentAddress from a diversifier and a Jubjub point.
    ///
    /// Returns None if `diversifier` is not valid for Sapling, or `pk_d` is the identity.
    /// Note that we cannot verify in this constructor that `pk_d` is derived from
    /// `diversifier`, so addresses for which these values have no known relationship
    /// (and therefore no-one can receive funds at them) can still be constructed.
    pub fn from_parts(diversifier: Diversifier, pk_d: DiversifiedTransmissionKey) -> Option<Self> {
        // Check that the diversifier is valid
        diversifier.g_d()?;

        Self::from_parts_unchecked(diversifier, pk_d)
    }

    /// Constructs a PaymentAddress from a diversifier and a Jubjub point.
    ///
    /// Returns None if `pk_d` is the identity. The caller must check that `diversifier`
    /// is valid for Sapling.
    pub(crate) fn from_parts_unchecked(
        diversifier: Diversifier,
        pk_d: DiversifiedTransmissionKey,
    ) -> Option<Self> {
        if pk_d.is_identity() {
            None
        } else {
            Some(PaymentAddress { pk_d, diversifier })
        }
    }

    /// Parses a PaymentAddress from bytes.
    pub fn from_bytes(bytes: &[u8; 43]) -> Option<Self> {
        let diversifier = {
            let mut tmp = [0; 11];
            tmp.copy_from_slice(&bytes[0..11]);
            Diversifier(tmp)
        };

        let pk_d = DiversifiedTransmissionKey::from_bytes(bytes[11..43].try_into().unwrap());
        if pk_d.is_some().into() {
            // The remaining invariants are checked here.
            PaymentAddress::from_parts(diversifier, pk_d.unwrap())
        } else {
            None
        }
    }

    /// Returns the byte encoding of this `PaymentAddress`.
    pub fn to_bytes(&self) -> [u8; 43] {
        let mut bytes = [0; 43];
        bytes[0..11].copy_from_slice(&self.diversifier.0);
        bytes[11..].copy_from_slice(&self.pk_d.to_bytes());
        bytes
    }

    /// Returns the [`Diversifier`] for this `PaymentAddress`.
    pub fn diversifier(&self) -> &Diversifier {
        &self.diversifier
    }

    /// Returns `pk_d` for this `PaymentAddress`.
    pub fn pk_d(&self) -> &DiversifiedTransmissionKey {
        &self.pk_d
    }

    pub(crate) fn g_d(&self) -> jubjub::SubgroupPoint {
        self.diversifier.g_d().expect("checked at construction")
    }

    pub fn create_note(&self, value: NoteValue, rseed: Rseed) -> Note {
        Note::from_parts(*self, value, rseed)
    }
}

#[cfg(any(test, feature = "test-dependencies"))]
pub(super) mod testing {
    use proptest::prelude::*;

    use super::{
        super::keys::{testing::arb_incoming_viewing_key, Diversifier, SaplingIvk},
        PaymentAddress,
    };

    pub fn arb_payment_address() -> impl Strategy<Value = PaymentAddress> {
        arb_incoming_viewing_key().prop_flat_map(|ivk: SaplingIvk| {
            any::<[u8; 11]>().prop_filter_map(
                "Sampled diversifier must generate a valid Sapling payment address.",
                move |d| ivk.to_payment_address(Diversifier(d)),
            )
        })
    }
}
