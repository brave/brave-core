//! Orchard fixed bases.

#[cfg(feature = "circuit")]
use alloc::vec::Vec;

use super::{L_ORCHARD_SCALAR, L_VALUE};

#[cfg(feature = "circuit")]
use halo2_gadgets::ecc::{
    chip::{BaseFieldElem, FixedPoint, FullScalar, ShortScalar},
    FixedPoints,
};

#[cfg(feature = "circuit")]
use pasta_curves::pallas;

/// Precomputed table for the `CommitIvk` commitment randomness base.
pub mod commit_ivk_r;
/// Precomputed table for the `NoteCommit` commitment randomness base.
pub mod note_commit_r;
/// Precomputed table for the nullifier base `K^Orchard`.
pub mod nullifier_k;
/// Precomputed table for the spend authorization base `G^Orchard`.
pub mod spend_auth_g;
/// Precomputed table for the value commitment randomness base.
pub mod value_commit_r;
/// Precomputed table for the value commitment value base.
pub mod value_commit_v;

/// SWU hash-to-curve personalization for the spending key base point and
/// the nullifier base point K^Orchard
pub const ORCHARD_PERSONALIZATION: &str = "z.cash:Orchard";

/// SWU hash-to-curve personalization for the value commitment generator
pub const VALUE_COMMITMENT_PERSONALIZATION: &str = "z.cash:Orchard-cv";

/// SWU hash-to-curve value for the value commitment generator
pub const VALUE_COMMITMENT_V_BYTES: [u8; 1] = *b"v";

/// SWU hash-to-curve value for the value commitment generator
pub const VALUE_COMMITMENT_R_BYTES: [u8; 1] = *b"r";

/// SWU hash-to-curve personalization for the note commitment generator
pub const NOTE_COMMITMENT_PERSONALIZATION: &str = "z.cash:Orchard-NoteCommit";

/// SWU hash-to-curve personalization for the IVK commitment generator
pub const COMMIT_IVK_PERSONALIZATION: &str = "z.cash:Orchard-CommitIvk";

/// Window size for fixed-base scalar multiplication
pub const FIXED_BASE_WINDOW_SIZE: usize = 3;

/// $2^{`FIXED_BASE_WINDOW_SIZE`}$
pub const H: usize = 1 << FIXED_BASE_WINDOW_SIZE;

/// Number of windows for a full-width scalar
pub const NUM_WINDOWS: usize = L_ORCHARD_SCALAR.div_ceil(FIXED_BASE_WINDOW_SIZE);

/// Number of windows for a short signed scalar
pub const NUM_WINDOWS_SHORT: usize = L_VALUE.div_ceil(FIXED_BASE_WINDOW_SIZE);

/// Fixed bases used in scalar mul where the scalar is a base field element.
///
/// The ECC chip's `FixedPoints::Base` associated type must be a single type,
/// so both `NullifierK` and `SpendAuthGBase` are wrapped in this enum.
/// `SpendAuthGBase` reuses the same generator and U/Z tables as
/// `OrchardFixedBasesFull::SpendAuthG` (same 85-window structure over the
/// 255-bit pallas base field), allowing `FixedPointBaseField::mul` to accept
/// an `AssignedCell` directly — no variable-base `NonIdentityPoint` witness
/// needed.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
#[non_exhaustive]
pub enum OrchardBaseFieldBases {
    /// Nullifier key generator $\mathcal{K}^{\mathsf{Orchard}}$, used in
    /// DeriveNullifier.
    NullifierK,
    /// SpendAuthG with full 85-window tables, accepting a base-field scalar via
    /// an `AssignedCell` input (no variable-base `NonIdentityPoint` witness
    /// required). Reuses the same generator and U/Z tables as
    /// `OrchardFixedBasesFull::SpendAuthG`.
    SpendAuthGBase,
}

/// Fixed bases used in scalar mul where the scalar is a short (64-bit) signed
/// value.
///
/// `FixedPoints::ShortScalar` must be a single type, so both `ValueCommitV` and
/// `SpendAuthGShort` are wrapped in this enum. `SpendAuthGShort` uses the same
/// generator as `SpendAuthG` but with 22-window precomputed tables, enabling
/// `FixedPointShort::mul` for short signed scalars.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
#[non_exhaustive]
pub enum OrchardShortScalarBases {
    /// Value commitment generator for Orchard (original short base).
    ValueCommitV,
    /// SpendAuthG with short (22-window) precomputed tables, for multiplication
    /// by short signed scalars via `FixedPointShort::mul`.
    SpendAuthGShort,
}

/// Carrier type for the `FixedPoints<pallas::Affine>` impl that wires Orchard's
/// per-slot fixed-base enums into halo2_gadgets's ECC, Sinsemilla, and Merkle
/// chips. Carries no value-level state — the concrete fixed bases live in
/// `OrchardFixedBasesFull`, `OrchardBaseFieldBases`, and `OrchardShortScalarBases`.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct OrchardFixedBases;

impl From<NullifierK> for OrchardBaseFieldBases {
    fn from(_: NullifierK) -> Self {
        Self::NullifierK
    }
}

impl From<ValueCommitV> for OrchardShortScalarBases {
    fn from(_: ValueCommitV) -> Self {
        Self::ValueCommitV
    }
}

/// The Orchard fixed bases used in scalar mul with full-width scalars.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum OrchardFixedBasesFull {
    /// Randomness base for the `CommitIvk` commitment.
    CommitIvkR,
    /// Randomness base for the `NoteCommit` commitment.
    NoteCommitR,
    /// Randomness base for value commitments.
    ValueCommitR,
    /// Spend authorization base `G^Orchard`.
    SpendAuthG,
}

/// NullifierK is used in scalar mul with a base field element.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct NullifierK;

/// ValueCommitV is used in scalar mul with a short signed scalar.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct ValueCommitV;

#[cfg(feature = "circuit")]
impl FixedPoints<pallas::Affine> for OrchardFixedBases {
    type FullScalar = OrchardFixedBasesFull;
    type Base = OrchardBaseFieldBases;
    type ShortScalar = OrchardShortScalarBases;
}

#[cfg(feature = "circuit")]
impl FixedPoint<pallas::Affine> for OrchardFixedBasesFull {
    type FixedScalarKind = FullScalar;

    fn generator(&self) -> pallas::Affine {
        match self {
            Self::CommitIvkR => commit_ivk_r::generator(),
            Self::NoteCommitR => note_commit_r::generator(),
            Self::ValueCommitR => value_commit_r::generator(),
            Self::SpendAuthG => spend_auth_g::generator(),
        }
    }

    fn u(&self) -> Vec<[[u8; 32]; H]> {
        match self {
            Self::CommitIvkR => commit_ivk_r::U.to_vec(),
            Self::NoteCommitR => note_commit_r::U.to_vec(),
            Self::ValueCommitR => value_commit_r::U.to_vec(),
            Self::SpendAuthG => spend_auth_g::U.to_vec(),
        }
    }

    fn z(&self) -> Vec<u64> {
        match self {
            Self::CommitIvkR => commit_ivk_r::Z.to_vec(),
            Self::NoteCommitR => note_commit_r::Z.to_vec(),
            Self::ValueCommitR => value_commit_r::Z.to_vec(),
            Self::SpendAuthG => spend_auth_g::Z.to_vec(),
        }
    }
}

#[cfg(feature = "circuit")]
impl FixedPoint<pallas::Affine> for OrchardBaseFieldBases {
    type FixedScalarKind = BaseFieldElem;

    fn generator(&self) -> pallas::Affine {
        match self {
            Self::NullifierK => nullifier_k::generator(),
            Self::SpendAuthGBase => spend_auth_g::generator(),
        }
    }

    fn u(&self) -> Vec<[[u8; 32]; H]> {
        match self {
            Self::NullifierK => nullifier_k::U.to_vec(),
            // SpendAuthG's full-scalar U/Z tables have the same 85-window
            // structure as the base-field-element variant (pallas::Base and
            // pallas::Scalar are both 255-bit); the precomputed values depend
            // only on the generator and window layout, not the scalar kind.
            Self::SpendAuthGBase => spend_auth_g::U.to_vec(),
        }
    }

    fn z(&self) -> Vec<u64> {
        match self {
            Self::NullifierK => nullifier_k::Z.to_vec(),
            Self::SpendAuthGBase => spend_auth_g::Z.to_vec(),
        }
    }
}

#[cfg(feature = "circuit")]
impl FixedPoint<pallas::Affine> for OrchardShortScalarBases {
    type FixedScalarKind = ShortScalar;

    fn generator(&self) -> pallas::Affine {
        match self {
            Self::ValueCommitV => value_commit_v::generator(),
            Self::SpendAuthGShort => spend_auth_g::generator(),
        }
    }

    fn u(&self) -> Vec<[[u8; 32]; H]> {
        match self {
            Self::ValueCommitV => value_commit_v::U_SHORT.to_vec(),
            Self::SpendAuthGShort => spend_auth_g::U_SHORT.to_vec(),
        }
    }

    fn z(&self) -> Vec<u64> {
        match self {
            Self::ValueCommitV => value_commit_v::Z_SHORT.to_vec(),
            Self::SpendAuthGShort => spend_auth_g::Z_SHORT.to_vec(),
        }
    }
}

#[cfg(all(test, feature = "circuit"))]
mod tests {
    use super::*;

    /// Ensures that `OrchardBaseFieldBases::SpendAuthGBase` routes to the
    /// correct generator and tables via the `FixedPoint` trait. The U/Z data
    /// is identical to `OrchardFixedBasesFull::SpendAuthG` (same generator,
    /// same 85-window structure); this test makes the dispatch wiring explicit.
    #[test]
    fn spend_auth_g_base_field_routes_correctly() {
        let full = OrchardFixedBasesFull::SpendAuthG;
        let base = OrchardBaseFieldBases::SpendAuthGBase;

        assert_eq!(
            full.generator(),
            base.generator(),
            "SpendAuthGBase must share the SpendAuthG generator"
        );
        assert_eq!(
            full.u(),
            base.u(),
            "SpendAuthGBase U tables must match SpendAuthG full-scalar U tables"
        );
        assert_eq!(
            full.z(),
            base.z(),
            "SpendAuthGBase Z tables must match SpendAuthG full-scalar Z tables"
        );
    }

    /// Ensures that `OrchardBaseFieldBases::NullifierK` still routes to the
    /// NullifierK generator and tables (regression guard for the enum refactor).
    #[test]
    fn nullifier_k_base_field_routes_correctly() {
        let base = OrchardBaseFieldBases::NullifierK;

        assert_eq!(
            base.generator(),
            nullifier_k::generator(),
            "OrchardBaseFieldBases::NullifierK must use the NullifierK generator"
        );
        assert_eq!(
            base.u(),
            nullifier_k::U.to_vec(),
            "OrchardBaseFieldBases::NullifierK U tables must match"
        );
        assert_eq!(
            base.z(),
            nullifier_k::Z.to_vec(),
            "OrchardBaseFieldBases::NullifierK Z tables must match"
        );
    }

    #[test]
    fn nullifier_k_converts_to_base_field_enum() {
        assert_eq!(
            OrchardBaseFieldBases::from(NullifierK),
            OrchardBaseFieldBases::NullifierK
        );
    }

    /// Ensures that `OrchardShortScalarBases::SpendAuthGShort` routes to the
    /// SpendAuthG generator and the 22-window short tables.
    #[test]
    fn spend_auth_g_short_routes_correctly() {
        let short = OrchardShortScalarBases::SpendAuthGShort;
        let full = OrchardFixedBasesFull::SpendAuthG;

        assert_eq!(
            short.generator(),
            full.generator(),
            "SpendAuthGShort must share the SpendAuthG generator"
        );
        assert_eq!(
            short.u().len(),
            NUM_WINDOWS_SHORT,
            "SpendAuthGShort U table must have NUM_WINDOWS_SHORT entries"
        );
        assert_eq!(
            short.z().len(),
            NUM_WINDOWS_SHORT,
            "SpendAuthGShort Z table must have NUM_WINDOWS_SHORT entries"
        );
        // The first NUM_WINDOWS_SHORT - 1 windows are identical: halo2_gadgets'
        // `find_zs_and_us` builds them from (base, w) only, with no dependence
        // on num_windows. Only the last window depends on num_windows (through
        // its offset), so it must differ between the 85-window and 22-window
        // tables.
        let short_u = short.u();
        let full_u = full.u();
        let short_z = short.z();
        let full_z = full.z();

        assert_eq!(
            short_u[..NUM_WINDOWS_SHORT - 1],
            full_u[..NUM_WINDOWS_SHORT - 1],
            "SpendAuthGShort's first NUM_WINDOWS_SHORT - 1 U windows must equal SpendAuthG's"
        );
        assert_eq!(
            short_z[..NUM_WINDOWS_SHORT - 1],
            full_z[..NUM_WINDOWS_SHORT - 1],
            "SpendAuthGShort's first NUM_WINDOWS_SHORT - 1 Z windows must equal SpendAuthG's"
        );
        assert_ne!(
            short_u[NUM_WINDOWS_SHORT - 1],
            full_u[NUM_WINDOWS_SHORT - 1],
            "SpendAuthGShort's final U window must differ from the full-scalar table"
        );
        assert_ne!(
            short_z[NUM_WINDOWS_SHORT - 1],
            full_z[NUM_WINDOWS_SHORT - 1],
            "SpendAuthGShort's final Z window must differ from the full-scalar table"
        );
    }

    /// Ensures that `OrchardShortScalarBases::ValueCommitV` still routes to
    /// the ValueCommitV generator and tables (regression guard for the enum change).
    #[test]
    fn value_commit_v_short_routes_correctly() {
        let short = OrchardShortScalarBases::ValueCommitV;

        assert_eq!(
            short.generator(),
            value_commit_v::generator(),
            "OrchardShortScalarBases::ValueCommitV must use the ValueCommitV generator"
        );
        assert_eq!(
            short.u(),
            value_commit_v::U_SHORT.to_vec(),
            "OrchardShortScalarBases::ValueCommitV U tables must match"
        );
        assert_eq!(
            short.z(),
            value_commit_v::Z_SHORT.to_vec(),
            "OrchardShortScalarBases::ValueCommitV Z tables must match"
        );
    }

    #[test]
    fn value_commit_v_converts_to_short_scalar_enum() {
        assert_eq!(
            OrchardShortScalarBases::from(ValueCommitV),
            OrchardShortScalarBases::ValueCommitV
        );
    }

    /// Selects which newly supported SpendAuthG fixed base the shared circuit
    /// should exercise.
    #[derive(Copy, Clone, Debug)]
    enum SpendAuthGCase {
        /// Multiply `SpendAuthGBase` by a witnessed base-field scalar via
        /// `FixedPointBaseField::mul`.
        BaseField,
        /// Multiply `SpendAuthGShort` by a witnessed signed short scalar via
        /// `FixedPointShort::mul`.
        Short,
    }

    /// Shared MockProver harness for both SpendAuthG fixed-base routes.
    ///
    /// Factoring this out keeps `configure`, the 10-bit lookup-table load, and
    /// the MockProver wiring common between the per-base tests below. Only the
    /// case-specific `mul` invocation and native-arithmetic cross-check differ.
    fn run_spend_auth_g_fixed_base_mul_e2e(case: SpendAuthGCase) {
        use group::{ff::PrimeField, Curve};
        use halo2_gadgets::{
            ecc::{
                chip::{CircuitVersion, EccChip, EccConfig},
                FixedPointBaseField, FixedPointShort, NonIdentityPoint, ScalarFixedShort,
            },
            utilities::{
                lookup_range_check::{LookupRangeCheck, LookupRangeCheckConfig},
                UtilitiesInstructions,
            },
        };
        use halo2_proofs::{
            circuit::{AssignedCell, Layouter, SimpleFloorPlanner, Value},
            dev::MockProver,
            plonk::{Circuit, ConstraintSystem, Error, TableColumn},
        };

        #[derive(Clone, Debug)]
        struct MyConfig {
            ecc: EccConfig<OrchardFixedBases>,
            table_idx: TableColumn,
        }

        struct MyCircuit {
            case: SpendAuthGCase,
        }

        impl UtilitiesInstructions<pallas::Base> for MyCircuit {
            type Var = AssignedCell<pallas::Base, pallas::Base>;
        }

        impl Circuit<pallas::Base> for MyCircuit {
            type Config = MyConfig;
            type FloorPlanner = SimpleFloorPlanner;

            fn without_witnesses(&self) -> Self {
                MyCircuit { case: self.case }
            }

            fn configure(meta: &mut ConstraintSystem<pallas::Base>) -> Self::Config {
                let advices = [
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                    meta.advice_column(),
                ];
                for advice in advices.iter() {
                    meta.enable_equality(*advice);
                }
                let lagrange_coeffs = [
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                    meta.fixed_column(),
                ];
                let constants = meta.fixed_column();
                meta.enable_constant(constants);
                let table_idx = meta.lookup_table_column();
                let range_check = LookupRangeCheckConfig::configure(meta, advices[9], table_idx);

                let ecc = EccChip::<OrchardFixedBases>::configure(
                    meta,
                    advices,
                    lagrange_coeffs,
                    range_check,
                );

                MyConfig { ecc, table_idx }
            }

            fn synthesize(
                &self,
                config: Self::Config,
                mut layouter: impl Layouter<pallas::Base>,
            ) -> Result<(), Error> {
                let chip = EccChip::construct(config.ecc.clone(), CircuitVersion::AnchoredBase);

                // The fixed-base ECC gadgets expect this 10-bit lookup table to
                // be populated before any windowed multiplication can succeed.
                layouter.assign_table(
                    || "10-bit range-check table",
                    |mut table| {
                        for index in 0..(1 << 10) {
                            table.assign_cell(
                                || "range-check value",
                                config.table_idx,
                                index,
                                || Value::known(pallas::Base::from(index as u64)),
                            )?;
                        }
                        Ok(())
                    },
                )?;

                match self.case {
                    SpendAuthGCase::BaseField => {
                        // Exercise the new base-field entrypoint end-to-end by
                        // multiplying SpendAuthGBase by a witnessed base-field
                        // scalar.
                        let base = FixedPointBaseField::from_inner(
                            chip.clone(),
                            OrchardBaseFieldBases::SpendAuthGBase,
                        );
                        let scalar_val = pallas::Base::from(7u64);
                        let scalar_cell = chip.load_private(
                            layouter.namespace(|| "SpendAuthGBase scalar"),
                            config.ecc.advices[0],
                            Value::known(scalar_val),
                        )?;
                        let result =
                            base.mul(layouter.namespace(|| "SpendAuthGBase mul"), scalar_cell)?;

                        // Check the gadget result against native curve
                        // arithmetic on the same generator and scalar.
                        let scalar = pallas::Scalar::from_repr(scalar_val.to_repr()).unwrap();
                        let expected = NonIdentityPoint::new(
                            chip,
                            layouter.namespace(|| "SpendAuthGBase expected"),
                            Value::known((spend_auth_g::generator() * scalar).to_affine()),
                        )?;
                        result.constrain_equal(
                            layouter.namespace(|| "SpendAuthGBase constrain"),
                            &expected,
                        )?;
                    }
                    SpendAuthGCase::Short => {
                        // Exercise the short-scalar entrypoint end-to-end by
                        // constructing a signed short witness and multiplying
                        // it.
                        let base = FixedPointShort::from_inner(
                            chip.clone(),
                            OrchardShortScalarBases::SpendAuthGShort,
                        );
                        let magnitude = chip.load_private(
                            layouter.namespace(|| "SpendAuthGShort magnitude"),
                            config.ecc.advices[0],
                            Value::known(pallas::Base::from(42u64)),
                        )?;
                        let sign = chip.load_private(
                            layouter.namespace(|| "SpendAuthGShort sign"),
                            config.ecc.advices[0],
                            Value::known(pallas::Base::one()),
                        )?;
                        let scalar = ScalarFixedShort::new(
                            chip.clone(),
                            layouter.namespace(|| "SpendAuthGShort scalar"),
                            (magnitude, sign),
                        )?;
                        let (result, _) =
                            base.mul(layouter.namespace(|| "SpendAuthGShort mul"), scalar)?;

                        // As above, compare the gadget output with the native
                        // SpendAuthG generator multiplied by the equivalent
                        // scalar.
                        let expected = NonIdentityPoint::new(
                            chip,
                            layouter.namespace(|| "SpendAuthGShort expected"),
                            Value::known(
                                (spend_auth_g::generator() * pallas::Scalar::from(42u64))
                                    .to_affine(),
                            ),
                        )?;
                        result.constrain_equal(
                            layouter.namespace(|| "SpendAuthGShort constrain"),
                            &expected,
                        )?;
                    }
                }

                Ok(())
            }
        }

        // The circuit only passes if the selected fixed-base route produces the
        // expected curve point under MockProver.
        let prover = MockProver::<pallas::Base>::run(11, &MyCircuit { case }, vec![]).unwrap();
        assert_eq!(prover.verify(), Ok(()));
    }

    #[test]
    fn spend_auth_g_base_field_mul_e2e() {
        run_spend_auth_g_fixed_base_mul_e2e(SpendAuthGCase::BaseField);
    }

    #[test]
    fn spend_auth_g_short_mul_e2e() {
        run_spend_auth_g_fixed_base_mul_e2e(SpendAuthGCase::Short);
    }
}
