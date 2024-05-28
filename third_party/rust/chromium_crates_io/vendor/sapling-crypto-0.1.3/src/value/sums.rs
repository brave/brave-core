use core::fmt::{self, Debug};
use core::iter::Sum;
use core::ops::{Add, AddAssign, Sub, SubAssign};

use group::GroupEncoding;
use redjubjub::Binding;

use super::{NoteValue, ValueCommitTrapdoor, ValueCommitment};
use crate::constants::VALUE_COMMITMENT_VALUE_GENERATOR;

/// A value operation overflowed.
#[derive(Debug)]
pub struct OverflowError;

impl fmt::Display for OverflowError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "Sapling value operation overflowed")
    }
}

impl std::error::Error for OverflowError {}

/// A sum of Sapling note values.
///
/// [Zcash Protocol Spec § 4.13: Balance and Binding Signature (Sapling)][saplingbalance]
/// constrains the range of this type to between `[-(r_J - 1)/2..(r_J - 1)/2]` in the
/// abstract protocol, and `[−38913406623490299131842..104805176454780817500623]` in the
/// concrete Zcash protocol. We represent it as an `i128`, which has a range large enough
/// to handle Zcash transactions while small enough to ensure the abstract protocol bounds
/// are not breached.
///
/// [saplingbalance]: https://zips.z.cash/protocol/protocol.pdf#saplingbalance
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct ValueSum(i128);

impl ValueSum {
    /// Initializes a sum of `NoteValue`s to zero.
    pub fn zero() -> Self {
        ValueSum(0)
    }
}

impl Add<NoteValue> for ValueSum {
    type Output = Option<ValueSum>;

    #[allow(clippy::suspicious_arithmetic_impl)]
    fn add(self, rhs: NoteValue) -> Self::Output {
        self.0.checked_add(rhs.0.into()).map(ValueSum)
    }
}

impl Sub<NoteValue> for ValueSum {
    type Output = Option<ValueSum>;

    #[allow(clippy::suspicious_arithmetic_impl)]
    fn sub(self, rhs: NoteValue) -> Self::Output {
        self.0.checked_sub(rhs.0.into()).map(ValueSum)
    }
}

impl<'a> Sum<&'a NoteValue> for Result<ValueSum, OverflowError> {
    fn sum<I: Iterator<Item = &'a NoteValue>>(iter: I) -> Self {
        iter.fold(Ok(ValueSum(0)), |acc, v| (acc? + *v).ok_or(OverflowError))
    }
}

impl Sum<NoteValue> for Result<ValueSum, OverflowError> {
    fn sum<I: Iterator<Item = NoteValue>>(iter: I) -> Self {
        iter.fold(Ok(ValueSum(0)), |acc, v| (acc? + v).ok_or(OverflowError))
    }
}

impl TryFrom<ValueSum> for i64 {
    type Error = OverflowError;

    fn try_from(v: ValueSum) -> Result<i64, Self::Error> {
        i64::try_from(v.0).map_err(|_| OverflowError)
    }
}

/// A sum of Sapling value commitment blinding factors.
#[derive(Clone, Copy, Debug)]
pub struct TrapdoorSum(jubjub::Scalar);

impl TrapdoorSum {
    /// Initializes a sum of `ValueCommitTrapdoor`s to zero.
    pub fn zero() -> Self {
        TrapdoorSum(jubjub::Scalar::zero())
    }

    /// Transform this trapdoor sum into the corresponding RedJubjub private key.
    ///
    /// This is public for access by `zcash_proofs`.
    pub fn into_bsk(self) -> redjubjub::SigningKey<Binding> {
        redjubjub::SigningKey::try_from(self.0.to_bytes())
            .expect("valid scalars are valid signing keys")
    }
}

impl Add<&ValueCommitTrapdoor> for ValueCommitTrapdoor {
    type Output = TrapdoorSum;

    fn add(self, rhs: &Self) -> Self::Output {
        TrapdoorSum(self.0 + rhs.0)
    }
}

impl Add<&ValueCommitTrapdoor> for TrapdoorSum {
    type Output = TrapdoorSum;

    fn add(self, rhs: &ValueCommitTrapdoor) -> Self::Output {
        TrapdoorSum(self.0 + rhs.0)
    }
}

impl AddAssign<&ValueCommitTrapdoor> for TrapdoorSum {
    fn add_assign(&mut self, rhs: &ValueCommitTrapdoor) {
        self.0 += rhs.0;
    }
}

impl Sub<&ValueCommitTrapdoor> for ValueCommitTrapdoor {
    type Output = TrapdoorSum;

    fn sub(self, rhs: &Self) -> Self::Output {
        TrapdoorSum(self.0 - rhs.0)
    }
}

impl Sub<TrapdoorSum> for TrapdoorSum {
    type Output = TrapdoorSum;

    fn sub(self, rhs: Self) -> Self::Output {
        TrapdoorSum(self.0 - rhs.0)
    }
}

impl SubAssign<&ValueCommitTrapdoor> for TrapdoorSum {
    fn sub_assign(&mut self, rhs: &ValueCommitTrapdoor) {
        self.0 -= rhs.0;
    }
}

impl<'a> Sum<&'a ValueCommitTrapdoor> for TrapdoorSum {
    fn sum<I: Iterator<Item = &'a ValueCommitTrapdoor>>(iter: I) -> Self {
        iter.fold(TrapdoorSum::zero(), |acc, cv| acc + cv)
    }
}

/// A sum of Sapling value commitments.
#[derive(Clone, Copy, Debug)]
pub struct CommitmentSum(jubjub::ExtendedPoint);

impl CommitmentSum {
    /// Initializes a sum of `ValueCommitment`s to zero.
    pub fn zero() -> Self {
        CommitmentSum(jubjub::ExtendedPoint::identity())
    }

    /// Transform this value commitment sum into the corresponding RedJubjub public key.
    ///
    /// This is public for access by `zcash_proofs`.
    pub fn into_bvk<V: Into<i64>>(self, value_balance: V) -> redjubjub::VerificationKey<Binding> {
        let value: i64 = value_balance.into();

        // Compute the absolute value.
        let abs_value = match value.checked_abs() {
            Some(v) => u64::try_from(v).expect("v is non-negative"),
            None => 1u64 << 63,
        };

        // Construct the field representation of the signed value.
        let value_balance = if value.is_negative() {
            -jubjub::Scalar::from(abs_value)
        } else {
            jubjub::Scalar::from(abs_value)
        };

        // Subtract `value_balance` from the sum to get the final bvk.
        let bvk = self.0 - VALUE_COMMITMENT_VALUE_GENERATOR * value_balance;

        redjubjub::VerificationKey::try_from(bvk.to_bytes())
            .expect("valid points are valid verification keys")
    }
}

impl Add<&ValueCommitment> for ValueCommitment {
    type Output = CommitmentSum;

    fn add(self, rhs: &Self) -> Self::Output {
        CommitmentSum(self.0 + rhs.0)
    }
}

impl Add<&ValueCommitment> for CommitmentSum {
    type Output = CommitmentSum;

    fn add(self, rhs: &ValueCommitment) -> Self::Output {
        CommitmentSum(self.0 + rhs.0)
    }
}

impl AddAssign<&ValueCommitment> for CommitmentSum {
    fn add_assign(&mut self, rhs: &ValueCommitment) {
        self.0 += rhs.0;
    }
}

impl Sub<&ValueCommitment> for ValueCommitment {
    type Output = CommitmentSum;

    fn sub(self, rhs: &Self) -> Self::Output {
        CommitmentSum(self.0 - rhs.0)
    }
}

impl SubAssign<&ValueCommitment> for CommitmentSum {
    fn sub_assign(&mut self, rhs: &ValueCommitment) {
        self.0 -= rhs.0;
    }
}

impl Sub<CommitmentSum> for CommitmentSum {
    type Output = CommitmentSum;

    fn sub(self, rhs: Self) -> Self::Output {
        CommitmentSum(self.0 - rhs.0)
    }
}

impl Sum<ValueCommitment> for CommitmentSum {
    fn sum<I: Iterator<Item = ValueCommitment>>(iter: I) -> Self {
        iter.fold(CommitmentSum::zero(), |acc, cv| acc + &cv)
    }
}

impl<'a> Sum<&'a ValueCommitment> for CommitmentSum {
    fn sum<I: Iterator<Item = &'a ValueCommitment>>(iter: I) -> Self {
        iter.fold(CommitmentSum::zero(), |acc, cv| acc + cv)
    }
}
