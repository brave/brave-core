use super::super::{EccPoint, EccScalarFixed, FixedPoints, FIXED_BASE_WINDOW_SIZE, H, NUM_WINDOWS};

use crate::utilities::{decompose_word, range_check};
use arrayvec::ArrayVec;
use ff::PrimeField;
use halo2_proofs::{
    circuit::{AssignedCell, Layouter, Region, Value},
    plonk::{ConstraintSystem, Constraints, Error, Selector},
    poly::Rotation,
};
use pasta_curves::pallas;

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Config<Fixed: FixedPoints<pallas::Affine>> {
    q_mul_fixed_full: Selector,
    super_config: super::Config<Fixed>,
}

impl<Fixed: FixedPoints<pallas::Affine>> Config<Fixed> {
    pub(crate) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        super_config: super::Config<Fixed>,
    ) -> Self {
        let config = Self {
            q_mul_fixed_full: meta.selector(),
            super_config,
        };

        config.create_gate(meta);

        config
    }

    fn create_gate(&self, meta: &mut ConstraintSystem<pallas::Base>) {
        // Check that each window `k` is within 3 bits
        // https://p.z.cash/halo2-0.1:ecc-fixed-mul-full-word
        meta.create_gate("Full-width fixed-base scalar mul", |meta| {
            let q_mul_fixed_full = meta.query_selector(self.q_mul_fixed_full);
            let window = meta.query_advice(self.super_config.window, Rotation::cur());

            Constraints::with_selector(
                q_mul_fixed_full,
                self.super_config
                    .coords_check(meta, window.clone())
                    .into_iter()
                    // Constrain each window to a 3-bit value:
                    // window * (1 - window) * ... * (7 - window)
                    .chain(Some(("window range check", range_check(window, H)))),
            )
        });
    }

    /// Witnesses the given scalar as `NUM_WINDOWS` 3-bit windows.
    ///
    /// The scalar is allowed to be non-canonical.
    fn witness(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        scalar: Value<pallas::Scalar>,
    ) -> Result<EccScalarFixed, Error> {
        let windows = self.decompose_scalar_fixed::<{ pallas::Scalar::NUM_BITS as usize }>(
            scalar, offset, region,
        )?;

        Ok(EccScalarFixed {
            value: scalar,
            windows: Some(windows),
        })
    }

    /// Witnesses the given scalar as `NUM_WINDOWS` 3-bit windows.
    ///
    /// The scalar is allowed to be non-canonical.
    fn decompose_scalar_fixed<const SCALAR_NUM_BITS: usize>(
        &self,
        scalar: Value<pallas::Scalar>,
        offset: usize,
        region: &mut Region<'_, pallas::Base>,
    ) -> Result<ArrayVec<AssignedCell<pallas::Base, pallas::Base>, NUM_WINDOWS>, Error> {
        // Enable `q_mul_fixed_full` selector
        for idx in 0..NUM_WINDOWS {
            self.q_mul_fixed_full.enable(region, offset + idx)?;
        }

        // Decompose scalar into `k-bit` windows
        let scalar_windows: Value<Vec<u8>> = scalar.map(|scalar| {
            decompose_word::<pallas::Scalar>(&scalar, SCALAR_NUM_BITS, FIXED_BASE_WINDOW_SIZE)
        });

        // Transpose `Value<Vec<u8>>` into `Vec<Value<pallas::Base>>`.
        let scalar_windows = scalar_windows
            .map(|windows| {
                windows
                    .into_iter()
                    .map(|window| pallas::Base::from(window as u64))
            })
            .transpose_vec(NUM_WINDOWS);

        // Store the scalar decomposition
        let mut windows: ArrayVec<AssignedCell<pallas::Base, pallas::Base>, NUM_WINDOWS> =
            ArrayVec::new();
        for (idx, window) in scalar_windows.into_iter().enumerate() {
            let window_cell = region.assign_advice(
                || format!("k[{:?}]", offset + idx),
                self.super_config.window,
                offset + idx,
                || window,
            )?;
            windows.push(window_cell);
        }

        Ok(windows)
    }

    pub fn assign(
        &self,
        mut layouter: impl Layouter<pallas::Base>,
        scalar: &EccScalarFixed,
        base: &<Fixed as FixedPoints<pallas::Affine>>::FullScalar,
    ) -> Result<(EccPoint, EccScalarFixed), Error>
    where
        <Fixed as FixedPoints<pallas::Affine>>::FullScalar:
            super::super::FixedPoint<pallas::Affine>,
    {
        let (scalar, acc, mul_b) = layouter.assign_region(
            || "Full-width fixed-base mul (incomplete addition)",
            |mut region| {
                let offset = 0;

                // Lazily witness the scalar.
                let scalar = match scalar.windows {
                    None => self.witness(&mut region, offset, scalar.value),
                    Some(_) => todo!("unimplemented for halo2_gadgets v0.1.0"),
                }?;

                let (acc, mul_b) = self.super_config.assign_region_inner::<_, NUM_WINDOWS>(
                    &mut region,
                    offset,
                    &(&scalar).into(),
                    base,
                    self.q_mul_fixed_full,
                )?;

                Ok((scalar, acc, mul_b))
            },
        )?;

        // Add to the accumulator and return the final result as `[scalar]B`.
        let result = layouter.assign_region(
            || "Full-width fixed-base mul (last window, complete addition)",
            |mut region| {
                self.super_config.add_config.assign_region(
                    &mul_b.clone().into(),
                    &acc.clone().into(),
                    0,
                    &mut region,
                )
            },
        )?;

        #[cfg(test)]
        // Check that the correct multiple is obtained.
        {
            use super::super::FixedPoint;
            use group::Curve;

            let real_mul = scalar.value.map(|scalar| base.generator() * scalar);
            let result = result.point();

            real_mul
                .zip(result)
                .assert_if_known(|(real_mul, result)| &real_mul.to_affine() == result);
        }

        Ok((result, scalar))
    }
}

#[cfg(test)]
pub mod tests {
    use group::{ff::Field, Curve};
    use halo2_proofs::{
        circuit::{Layouter, Value},
        plonk::Error,
    };
    use pasta_curves::pallas;
    use rand::rngs::OsRng;

    use crate::ecc::{
        chip::{EccChip, FixedPoint as _, H},
        tests::{FullWidth, TestFixedBases},
        FixedPoint, NonIdentityPoint, Point, ScalarFixed,
    };

    pub(crate) fn test_mul_fixed(
        chip: EccChip<TestFixedBases>,
        mut layouter: impl Layouter<pallas::Base>,
    ) -> Result<(), Error> {
        let test_base = FullWidth::from_pallas_generator();
        test_single_base(
            chip.clone(),
            layouter.namespace(|| "full_width"),
            FixedPoint::from_inner(chip, test_base.clone()),
            test_base.generator(),
        )?;

        Ok(())
    }

    #[allow(clippy::op_ref)]
    fn test_single_base(
        chip: EccChip<TestFixedBases>,
        mut layouter: impl Layouter<pallas::Base>,
        base: FixedPoint<pallas::Affine, EccChip<TestFixedBases>>,
        base_val: pallas::Affine,
    ) -> Result<(), Error> {
        fn constrain_equal_non_id(
            chip: EccChip<TestFixedBases>,
            mut layouter: impl Layouter<pallas::Base>,
            base_val: pallas::Affine,
            scalar_val: pallas::Scalar,
            result: Point<pallas::Affine, EccChip<TestFixedBases>>,
        ) -> Result<(), Error> {
            let expected = NonIdentityPoint::new(
                chip,
                layouter.namespace(|| "expected point"),
                Value::known((base_val * scalar_val).to_affine()),
            )?;
            result.constrain_equal(layouter.namespace(|| "constrain result"), &expected)
        }

        // [a]B
        {
            let scalar_fixed = pallas::Scalar::random(OsRng);
            let by = ScalarFixed::new(
                chip.clone(),
                layouter.namespace(|| "random a"),
                Value::known(scalar_fixed),
            )?;

            let (result, _) = base.mul(layouter.namespace(|| "random [a]B"), by)?;
            constrain_equal_non_id(
                chip.clone(),
                layouter.namespace(|| "random [a]B"),
                base_val,
                scalar_fixed,
                result,
            )?;
        }

        // There is a single canonical sequence of window values for which a doubling occurs on the last step:
        // 1333333333333333333333333333333333333333333333333333333333333333333333333333333333334 in octal.
        // (There is another *non-canonical* sequence
        // 5333333333333333333333333333333333333333332711161673731021062440252244051273333333333 in octal.)
        {
            const LAST_DOUBLING: &str = "1333333333333333333333333333333333333333333333333333333333333333333333333333333333334";
            let h = pallas::Scalar::from(H as u64);
            let scalar_fixed = LAST_DOUBLING
                .chars()
                .fold(pallas::Scalar::zero(), |acc, c| {
                    acc * &h + &pallas::Scalar::from(c.to_digit(8).unwrap() as u64)
                });
            let by = ScalarFixed::new(
                chip.clone(),
                layouter.namespace(|| LAST_DOUBLING),
                Value::known(scalar_fixed),
            )?;
            let (result, _) = base.mul(layouter.namespace(|| "mul with double"), by)?;

            constrain_equal_non_id(
                chip.clone(),
                layouter.namespace(|| "mul with double"),
                base_val,
                scalar_fixed,
                result,
            )?;
        }

        // [0]B should return (0,0) since it uses complete addition
        // on the last step.
        {
            let scalar_fixed = pallas::Scalar::zero();
            let zero = ScalarFixed::new(
                chip.clone(),
                layouter.namespace(|| "0"),
                Value::known(scalar_fixed),
            )?;
            let (result, _) = base.mul(layouter.namespace(|| "mul by zero"), zero)?;
            result
                .inner()
                .is_identity()
                .assert_if_known(|is_identity| *is_identity);
        }

        // [-1]B is the largest scalar field element.
        {
            let scalar_fixed = -pallas::Scalar::one();
            let neg_1 = ScalarFixed::new(
                chip.clone(),
                layouter.namespace(|| "-1"),
                Value::known(scalar_fixed),
            )?;
            let (result, _) = base.mul(layouter.namespace(|| "mul by -1"), neg_1)?;
            constrain_equal_non_id(
                chip,
                layouter.namespace(|| "mul by -1"),
                base_val,
                scalar_fixed,
                result,
            )?;
        }

        Ok(())
    }
}
