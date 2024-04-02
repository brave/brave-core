use super::{add, EccPoint, NonIdentityEccPoint, ScalarVar, T_Q};
use crate::{
    sinsemilla::primitives as sinsemilla,
    utilities::{bool_check, lookup_range_check::LookupRangeCheckConfig, ternary},
};
use std::{
    convert::TryInto,
    ops::{Deref, Range},
};

use ff::{Field, PrimeField};
use halo2_proofs::{
    circuit::{AssignedCell, Layouter, Region, Value},
    plonk::{Advice, Assigned, Column, ConstraintSystem, Constraints, Error, Selector},
    poly::Rotation,
};
use uint::construct_uint;

use pasta_curves::pallas;

mod complete;
pub(super) mod incomplete;
mod overflow;

/// Number of bits for which complete addition needs to be used in variable-base
/// scalar multiplication
const NUM_COMPLETE_BITS: usize = 3;

// Bits used in incomplete addition. k_{254} to k_{4} inclusive
const INCOMPLETE_LEN: usize = pallas::Scalar::NUM_BITS as usize - 1 - NUM_COMPLETE_BITS;

// Bits k_{254} to k_{4} inclusive are used in incomplete addition.
// The `hi` half is k_{254} to k_{130} inclusive (length 125 bits).
// (It is a coincidence that k_{130} matches the boundary of the
// overflow check described in [the book](https://zcash.github.io/halo2/design/gadgets/ecc/var-base-scalar-mul.html#overflow-check).)
const INCOMPLETE_HI_RANGE: Range<usize> = 0..INCOMPLETE_HI_LEN;
const INCOMPLETE_HI_LEN: usize = INCOMPLETE_LEN / 2;

// Bits k_{254} to k_{4} inclusive are used in incomplete addition.
// The `lo` half is k_{129} to k_{4} inclusive (length 126 bits).
const INCOMPLETE_LO_RANGE: Range<usize> = INCOMPLETE_HI_LEN..INCOMPLETE_LEN;
const INCOMPLETE_LO_LEN: usize = INCOMPLETE_LEN - INCOMPLETE_HI_LEN;

// Bits k_{3} to k_{1} inclusive are used in complete addition.
// Bit k_{0} is handled separately.
const COMPLETE_RANGE: Range<usize> = INCOMPLETE_LEN..(INCOMPLETE_LEN + NUM_COMPLETE_BITS);

#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct Config {
    // Selector used to check switching logic on LSB
    q_mul_lsb: Selector,
    // Configuration used in complete addition
    add_config: add::Config,
    // Configuration used for `hi` bits of the scalar
    hi_config: incomplete::Config<INCOMPLETE_HI_LEN>,
    // Configuration used for `lo` bits of the scalar
    lo_config: incomplete::Config<INCOMPLETE_LO_LEN>,
    // Configuration used for complete addition part of double-and-add algorithm
    complete_config: complete::Config,
    // Configuration used to check for overflow
    overflow_config: overflow::Config,
}

impl Config {
    pub(super) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        add_config: add::Config,
        lookup_config: LookupRangeCheckConfig<pallas::Base, { sinsemilla::K }>,
        advices: [Column<Advice>; 10],
    ) -> Self {
        let hi_config = incomplete::Config::configure(
            meta, advices[9], advices[3], advices[0], advices[1], advices[4], advices[5],
        );
        let lo_config = incomplete::Config::configure(
            meta, advices[6], advices[7], advices[0], advices[1], advices[8], advices[2],
        );
        let complete_config = complete::Config::configure(meta, advices[9], add_config);
        let overflow_config =
            overflow::Config::configure(meta, lookup_config, advices[6..9].try_into().unwrap());

        let config = Self {
            q_mul_lsb: meta.selector(),
            add_config,
            hi_config,
            lo_config,
            complete_config,
            overflow_config,
        };

        config.create_gate(meta);

        assert_eq!(
            config.hi_config.double_and_add.x_p, config.lo_config.double_and_add.x_p,
            "x_p is shared across hi and lo halves."
        );
        assert_eq!(
            config.hi_config.y_p, config.lo_config.y_p,
            "y_p is shared across hi and lo halves."
        );

        // For both hi_config and lo_config:
        // z and lambda1 are assigned on the same row as the add_config output.
        // Therefore, z and lambda1 must not overlap with add_config.x_qr, add_config.y_qr.
        let add_config_outputs = config.add_config.output_columns();
        {
            assert!(
                !add_config_outputs.contains(&config.hi_config.z),
                "incomplete config z cannot overlap with complete addition columns."
            );
            assert!(
                !add_config_outputs.contains(&config.hi_config.double_and_add.lambda_1),
                "incomplete config lambda1 cannot overlap with complete addition columns."
            );
        }
        {
            assert!(
                !add_config_outputs.contains(&config.lo_config.z),
                "incomplete config z cannot overlap with complete addition columns."
            );
            assert!(
                !add_config_outputs.contains(&config.lo_config.double_and_add.lambda_1),
                "incomplete config lambda1 cannot overlap with complete addition columns."
            );
        }

        config
    }

    fn create_gate(&self, meta: &mut ConstraintSystem<pallas::Base>) {
        // If `lsb` is 0, (x, y) = (x_p, -y_p). If `lsb` is 1, (x, y) = (0,0).
        // https://p.z.cash/halo2-0.1:ecc-var-mul-lsb-gate?partial
        meta.create_gate("LSB check", |meta| {
            let q_mul_lsb = meta.query_selector(self.q_mul_lsb);

            let z_1 = meta.query_advice(self.complete_config.z_complete, Rotation::cur());
            let z_0 = meta.query_advice(self.complete_config.z_complete, Rotation::next());
            let x_p = meta.query_advice(self.add_config.x_p, Rotation::cur());
            let y_p = meta.query_advice(self.add_config.y_p, Rotation::cur());
            let base_x = meta.query_advice(self.add_config.x_p, Rotation::next());
            let base_y = meta.query_advice(self.add_config.y_p, Rotation::next());

            //    z_0 = 2 * z_1 + k_0
            // => k_0 = z_0 - 2 * z_1
            let lsb = z_0 - z_1 * pallas::Base::from(2);

            let bool_check = bool_check(lsb.clone());

            // `lsb` = 0 => (x_p, y_p) = (x, -y)
            // `lsb` = 1 => (x_p, y_p) = (0,0)
            let lsb_x = ternary(lsb.clone(), x_p.clone(), x_p - base_x);
            let lsb_y = ternary(lsb, y_p.clone(), y_p + base_y);

            Constraints::with_selector(
                q_mul_lsb,
                [
                    ("bool_check", bool_check),
                    ("lsb_x", lsb_x),
                    ("lsb_y", lsb_y),
                ],
            )
        });
    }

    pub(super) fn assign(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        alpha: AssignedCell<pallas::Base, pallas::Base>,
        base: &NonIdentityEccPoint,
    ) -> Result<(EccPoint, ScalarVar), Error> {
        let (result, zs): (EccPoint, Vec<Z<pallas::Base>>) = layouter.assign_region(
            || "variable-base scalar mul",
            |mut region| {
                let offset = 0;

                // Case `base` into an `EccPoint` for later use.
                let base_point: EccPoint = base.clone().into();

                // Decompose `k = alpha + t_q` bitwise (big-endian bit order).
                let bits = decompose_for_scalar_mul(alpha.value());

                // Define ranges for each part of the algorithm.
                let bits_incomplete_hi = &bits[INCOMPLETE_HI_RANGE];
                let bits_incomplete_lo = &bits[INCOMPLETE_LO_RANGE];
                let lsb = bits[pallas::Scalar::NUM_BITS as usize - 1];

                // Initialize the accumulator `acc = [2]base` using complete addition.
                let acc =
                    self.add_config
                        .assign_region(&base_point, &base_point, offset, &mut region)?;

                // Increase the offset by 1 after complete addition.
                let offset = offset + 1;

                // Initialize the running sum for scalar decomposition to zero.
                //
                // `incomplete::Config::double_and_add` will copy this cell directly into
                // itself. This is fine because we are just assigning the same value to
                // the same cell twice, and then applying an equality constraint between
                // the cell and itself (which the permutation argument treats as a no-op).
                let z_init = Z(region.assign_advice_from_constant(
                    || "z_init = 0",
                    self.hi_config.z,
                    offset,
                    pallas::Base::zero(),
                )?);

                // Double-and-add (incomplete addition) for the `hi` half of the scalar decomposition
                let (x_a, y_a, zs_incomplete_hi) = self.hi_config.double_and_add(
                    &mut region,
                    offset,
                    base,
                    bits_incomplete_hi,
                    (X(acc.x), Y(acc.y), z_init.clone()),
                )?;

                // Double-and-add (incomplete addition) for the `lo` half of the scalar decomposition
                let z = zs_incomplete_hi.last().expect("should not be empty");
                let (x_a, y_a, zs_incomplete_lo) = self.lo_config.double_and_add(
                    &mut region,
                    offset,
                    base,
                    bits_incomplete_lo,
                    (x_a, y_a, z.clone()),
                )?;

                // Move from incomplete addition to complete addition.
                // Inside incomplete::double_and_add, the offset was increased once after initialization
                // of the running sum.
                // Then, the final assignment of double-and-add was made on row + offset + 1.
                // Outside of incomplete addition, we must account for these offset increases by adding
                // 2 to the incomplete addition length.
                assert!(INCOMPLETE_LO_RANGE.len() >= INCOMPLETE_HI_RANGE.len());
                let offset = offset + INCOMPLETE_LO_RANGE.len() + 2;

                // Complete addition
                let (acc, zs_complete) = {
                    let z = zs_incomplete_lo.last().expect("should not be empty");
                    // Bits used in complete addition. k_{3} to k_{1} inclusive
                    // The LSB k_{0} is handled separately.
                    let bits_complete = &bits[COMPLETE_RANGE];
                    self.complete_config.assign_region(
                        &mut region,
                        offset,
                        bits_complete,
                        &base_point,
                        x_a,
                        y_a,
                        z.clone(),
                    )?
                };

                // Each iteration of the complete addition uses two rows.
                let offset = offset + COMPLETE_RANGE.len() * 2;

                // Process the least significant bit
                let z_1 = zs_complete.last().unwrap().clone();
                let (result, z_0) = self.process_lsb(&mut region, offset, base, acc, z_1, lsb)?;

                #[cfg(test)]
                // Check that the correct multiple is obtained.
                {
                    use group::Curve;

                    let base = base.point();
                    let alpha = alpha
                        .value()
                        .map(|alpha| pallas::Scalar::from_repr(alpha.to_repr()).unwrap());
                    let real_mul = base.zip(alpha).map(|(base, alpha)| base * alpha);
                    let result = result.point();

                    real_mul
                        .zip(result)
                        .assert_if_known(|(real_mul, result)| &real_mul.to_affine() == result);
                }

                let zs = {
                    let mut zs = std::iter::empty()
                        .chain(Some(z_init))
                        .chain(zs_incomplete_hi.into_iter())
                        .chain(zs_incomplete_lo.into_iter())
                        .chain(zs_complete.into_iter())
                        .chain(Some(z_0))
                        .collect::<Vec<_>>();
                    assert_eq!(zs.len(), pallas::Scalar::NUM_BITS as usize + 1);

                    // This reverses zs to give us [z_0, z_1, ..., z_{254}, z_{255}].
                    zs.reverse();
                    zs
                };

                Ok((result, zs))
            },
        )?;

        self.overflow_config.overflow_check(
            layouter.namespace(|| "overflow check"),
            alpha.clone(),
            &zs,
        )?;

        Ok((result, ScalarVar::BaseFieldElem(alpha)))
    }

    /// Processes the final scalar bit `k_0`.
    ///
    /// Assumptions for this sub-region:
    /// - `acc_x` and `acc_y` are assigned in row `offset` by the previous complete
    ///   addition. They will be copied into themselves.
    /// - `z_1 is assigned in row `offset` by the mul::complete region assignment. We only
    ///   use its value here.
    ///
    /// `x_p` and `y_p` are assigned here, and then copied into themselves by the complete
    /// addition subregion.
    ///
    /// ```text
    /// | x_p  | y_p  | acc_x | acc_y | complete addition  | z_1 | q_mul_lsb = 1
    /// |base_x|base_y| res_x | res_y |   |   |    |   |   | z_0 |
    /// ```
    ///
    /// [Specification](https://p.z.cash/halo2-0.1:ecc-var-mul-lsb-gate?partial).
    fn process_lsb(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        base: &NonIdentityEccPoint,
        acc: EccPoint,
        z_1: Z<pallas::Base>,
        lsb: Value<bool>,
    ) -> Result<(EccPoint, Z<pallas::Base>), Error> {
        // Enforce switching logic on LSB using a custom gate
        self.q_mul_lsb.enable(region, offset)?;

        // z_1 has been assigned at (z_complete, offset).
        // Assign z_0 = 2⋅z_1 + k_0
        let z_0 = {
            let z_0_val = z_1.value().zip(lsb).map(|(z_1, lsb)| {
                let lsb = pallas::Base::from(lsb as u64);
                z_1 * pallas::Base::from(2) + lsb
            });
            let z_0_cell = region.assign_advice(
                || "z_0",
                self.complete_config.z_complete,
                offset + 1,
                || z_0_val,
            )?;

            Z(z_0_cell)
        };

        // Copy in `base_x`, `base_y` to use in the LSB gate
        base.x()
            .copy_advice(|| "copy base_x", region, self.add_config.x_p, offset + 1)?;
        base.y()
            .copy_advice(|| "copy base_y", region, self.add_config.y_p, offset + 1)?;

        // If `lsb` is 0, return `Acc + (-P)`. If `lsb` is 1, simply return `Acc + 0`.
        let x = lsb.and_then(|lsb| {
            if !lsb {
                base.x.value().cloned()
            } else {
                Value::known(Assigned::Zero)
            }
        });

        let y = lsb.and_then(|lsb| {
            if !lsb {
                -base.y.value()
            } else {
                Value::known(Assigned::Zero)
            }
        });

        let x_cell = region.assign_advice(|| "x", self.add_config.x_p, offset, || x)?;
        let y_cell = region.assign_advice(|| "y", self.add_config.y_p, offset, || y)?;

        let p = EccPoint::from_coordinates_unchecked(x_cell, y_cell);

        // Return the result of the final complete addition as `[scalar]B`
        let result = self.add_config.assign_region(&p, &acc, offset, region)?;

        Ok((result, z_0))
    }
}

#[derive(Clone, Debug)]
// `x`-coordinate of the accumulator.
struct X<F: Field>(AssignedCell<Assigned<F>, F>);
impl<F: Field> Deref for X<F> {
    type Target = AssignedCell<Assigned<F>, F>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

#[derive(Clone, Debug)]
// `y`-coordinate of the accumulator.
struct Y<F: Field>(AssignedCell<Assigned<F>, F>);
impl<F: Field> Deref for Y<F> {
    type Target = AssignedCell<Assigned<F>, F>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

#[derive(Clone, Debug)]
// Cumulative sum `z` used to decompose the scalar.
struct Z<F: Field>(AssignedCell<F, F>);
impl<F: Field> Deref for Z<F> {
    type Target = AssignedCell<F, F>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

// https://p.z.cash/halo2-0.1:ecc-var-mul-witness-scalar?partial
#[allow(clippy::assign_op_pattern)]
#[allow(clippy::ptr_offset_with_cast)]
fn decompose_for_scalar_mul(scalar: Value<&pallas::Base>) -> Vec<Value<bool>> {
    construct_uint! {
        struct U256(4);
    }

    let bitstring = scalar.map(|scalar| {
        // We use `k = scalar + t_q` in the double-and-add algorithm, where
        // the scalar field `F_q = 2^254 + t_q`.
        // Note that the addition `scalar + t_q` is not reduced.
        //
        let scalar = U256::from_little_endian(&scalar.to_repr());
        let t_q = U256::from_little_endian(&T_Q.to_le_bytes());
        let k = scalar + t_q;

        // Little-endian bit representation of `k`.
        let bitstring = {
            let mut le_bytes = [0u8; 32];
            k.to_little_endian(&mut le_bytes);
            le_bytes
                .into_iter()
                .flat_map(|byte| (0..8).map(move |shift| (byte >> shift) % 2 == 1))
        };

        // Take the first 255 bits.
        bitstring
            .take(pallas::Scalar::NUM_BITS as usize)
            .collect::<Vec<_>>()
    });

    // Transpose.
    let mut bitstring = bitstring.transpose_vec(pallas::Scalar::NUM_BITS as usize);
    // Reverse to get the big-endian bit representation.
    bitstring.reverse();
    bitstring
}

#[cfg(test)]
pub mod tests {
    use group::{
        ff::{Field, PrimeField},
        Curve,
    };
    use halo2_proofs::{
        circuit::{Chip, Layouter, Value},
        plonk::Error,
    };
    use pasta_curves::pallas;
    use rand::rngs::OsRng;

    use crate::{
        ecc::{
            chip::{EccChip, EccPoint},
            tests::TestFixedBases,
            EccInstructions, NonIdentityPoint, Point, ScalarVar,
        },
        utilities::UtilitiesInstructions,
    };

    pub(crate) fn test_mul(
        chip: EccChip<TestFixedBases>,
        mut layouter: impl Layouter<pallas::Base>,
        p: &NonIdentityPoint<pallas::Affine, EccChip<TestFixedBases>>,
        p_val: pallas::Affine,
    ) -> Result<(), Error> {
        let column = chip.config().advices[0];

        fn constrain_equal_non_id<
            EccChip: EccInstructions<pallas::Affine, Point = EccPoint> + Clone + Eq + std::fmt::Debug,
        >(
            chip: EccChip,
            mut layouter: impl Layouter<pallas::Base>,
            base_val: pallas::Affine,
            scalar_val: pallas::Base,
            result: Point<pallas::Affine, EccChip>,
        ) -> Result<(), Error> {
            // Move scalar from base field into scalar field (which always fits
            // for Pallas).
            let scalar = pallas::Scalar::from_repr(scalar_val.to_repr()).unwrap();
            let expected = NonIdentityPoint::new(
                chip,
                layouter.namespace(|| "expected point"),
                Value::known((base_val * scalar).to_affine()),
            )?;
            result.constrain_equal(layouter.namespace(|| "constrain result"), &expected)
        }

        // [a]B
        {
            let scalar_val = pallas::Base::random(OsRng);
            let (result, _) = {
                let scalar = chip.load_private(
                    layouter.namespace(|| "random scalar"),
                    column,
                    Value::known(scalar_val),
                )?;
                let scalar = ScalarVar::from_base(
                    chip.clone(),
                    layouter.namespace(|| "ScalarVar from_base"),
                    &scalar,
                )?;
                p.mul(layouter.namespace(|| "random [a]B"), scalar)?
            };
            constrain_equal_non_id(
                chip.clone(),
                layouter.namespace(|| "random [a]B"),
                p_val,
                scalar_val,
                result,
            )?;
        }

        // [0]B should return (0,0) since variable-base scalar multiplication
        // uses complete addition for the final bits of the scalar.
        {
            let scalar_val = pallas::Base::zero();
            let (result, _) = {
                let scalar = chip.load_private(
                    layouter.namespace(|| "zero"),
                    column,
                    Value::known(scalar_val),
                )?;
                let scalar = ScalarVar::from_base(
                    chip.clone(),
                    layouter.namespace(|| "ScalarVar from_base"),
                    &scalar,
                )?;
                p.mul(layouter.namespace(|| "[0]B"), scalar)?
            };
            result
                .inner()
                .is_identity()
                .assert_if_known(|is_identity| *is_identity);
        }

        // [-1]B (the largest possible base field element)
        {
            let scalar_val = -pallas::Base::one();
            let (result, _) = {
                let scalar = chip.load_private(
                    layouter.namespace(|| "-1"),
                    column,
                    Value::known(scalar_val),
                )?;
                let scalar = ScalarVar::from_base(
                    chip.clone(),
                    layouter.namespace(|| "ScalarVar from_base"),
                    &scalar,
                )?;
                p.mul(layouter.namespace(|| "[-1]B"), scalar)?
            };
            constrain_equal_non_id(
                chip,
                layouter.namespace(|| "[-1]B"),
                p_val,
                scalar_val,
                result,
            )?;
        }

        Ok(())
    }
}
