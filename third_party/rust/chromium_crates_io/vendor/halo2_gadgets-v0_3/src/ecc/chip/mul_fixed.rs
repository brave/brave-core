use super::{
    add, add_incomplete, EccBaseFieldElemFixed, EccScalarFixed, EccScalarFixedShort, FixedPoint,
    NonIdentityEccPoint, FIXED_BASE_WINDOW_SIZE, H,
};
use crate::utilities::decompose_running_sum::RunningSumConfig;

use std::marker::PhantomData;

use group::{
    ff::{Field, PrimeField, PrimeFieldBits},
    Curve,
};
use halo2_proofs::{
    circuit::{AssignedCell, Region, Value},
    plonk::{
        Advice, Column, ConstraintSystem, Constraints, Error, Expression, Fixed, Selector,
        VirtualCells,
    },
    poly::Rotation,
};
use lazy_static::lazy_static;
use pasta_curves::{arithmetic::CurveAffine, pallas};

pub mod base_field_elem;
pub mod full_width;
pub mod short;

lazy_static! {
    static ref TWO_SCALAR: pallas::Scalar = pallas::Scalar::from(2);
    // H = 2^3 (3-bit window)
    static ref H_SCALAR: pallas::Scalar = pallas::Scalar::from(H as u64);
    static ref H_BASE: pallas::Base = pallas::Base::from(H as u64);
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct Config<FixedPoints: super::FixedPoints<pallas::Affine>> {
    running_sum_config: RunningSumConfig<pallas::Base, FIXED_BASE_WINDOW_SIZE>,
    // The fixed Lagrange interpolation coefficients for `x_p`.
    lagrange_coeffs: [Column<Fixed>; H],
    // The fixed `z` for each window such that `y + z = u^2`.
    fixed_z: Column<Fixed>,
    // Decomposition of an `n-1`-bit scalar into `k`-bit windows:
    // a = a_0 + 2^k(a_1) + 2^{2k}(a_2) + ... + 2^{(n-1)k}(a_{n-1})
    window: Column<Advice>,
    // y-coordinate of accumulator (only used in the final row).
    u: Column<Advice>,
    // Configuration for `add`
    add_config: add::Config,
    // Configuration for `add_incomplete`
    add_incomplete_config: add_incomplete::Config,
    _marker: PhantomData<FixedPoints>,
}

impl<FixedPoints: super::FixedPoints<pallas::Affine>> Config<FixedPoints> {
    #[allow(clippy::too_many_arguments)]
    pub(super) fn configure(
        meta: &mut ConstraintSystem<pallas::Base>,
        lagrange_coeffs: [Column<Fixed>; H],
        window: Column<Advice>,
        u: Column<Advice>,
        add_config: add::Config,
        add_incomplete_config: add_incomplete::Config,
    ) -> Self {
        meta.enable_equality(window);
        meta.enable_equality(u);

        let q_running_sum = meta.selector();
        let running_sum_config = RunningSumConfig::configure(meta, q_running_sum, window);

        let config = Self {
            running_sum_config,
            lagrange_coeffs,
            fixed_z: meta.fixed_column(),
            window,
            u,
            add_config,
            add_incomplete_config,
            _marker: PhantomData,
        };

        // Check relationships between `add_config` and `add_incomplete_config`.
        assert_eq!(
            config.add_config.x_p, config.add_incomplete_config.x_p,
            "add and add_incomplete are used internally in mul_fixed."
        );
        assert_eq!(
            config.add_config.y_p, config.add_incomplete_config.y_p,
            "add and add_incomplete are used internally in mul_fixed."
        );
        for advice in [config.window, config.u].iter() {
            assert_ne!(
                *advice, config.add_config.x_qr,
                "Do not overlap with output columns of add."
            );
            assert_ne!(
                *advice, config.add_config.y_qr,
                "Do not overlap with output columns of add."
            );
        }

        config.running_sum_coords_gate(meta);

        config
    }

    /// Check that each window in the running sum decomposition uses the correct y_p
    /// and interpolated x_p.
    ///
    /// This gate is used both in the mul_fixed::base_field_elem and mul_fixed::short
    /// helpers, which decompose the scalar using a running sum.
    ///
    /// This gate is not used in the mul_fixed::full_width helper, since the full-width
    /// scalar is witnessed directly as three-bit windows instead of being decomposed
    /// via a running sum.
    fn running_sum_coords_gate(&self, meta: &mut ConstraintSystem<pallas::Base>) {
        meta.create_gate("Running sum coordinates check", |meta| {
            let q_mul_fixed_running_sum =
                meta.query_selector(self.running_sum_config.q_range_check());

            let z_cur = meta.query_advice(self.window, Rotation::cur());
            let z_next = meta.query_advice(self.window, Rotation::next());

            //    z_{i+1} = (z_i - a_i) / 2^3
            // => a_i = z_i - z_{i+1} * 2^3
            let word = z_cur - z_next * pallas::Base::from(H as u64);

            Constraints::with_selector(q_mul_fixed_running_sum, self.coords_check(meta, word))
        });
    }

    /// [Specification](https://p.z.cash/halo2-0.1:ecc-fixed-mul-coordinates).
    #[allow(clippy::op_ref)]
    fn coords_check(
        &self,
        meta: &mut VirtualCells<'_, pallas::Base>,
        window: Expression<pallas::Base>,
    ) -> Vec<(&'static str, Expression<pallas::Base>)> {
        let y_p = meta.query_advice(self.add_config.y_p, Rotation::cur());
        let x_p = meta.query_advice(self.add_config.x_p, Rotation::cur());
        let z = meta.query_fixed(self.fixed_z);
        let u = meta.query_advice(self.u, Rotation::cur());

        let window_pow: Vec<Expression<pallas::Base>> = (0..H)
            .map(|pow| {
                (0..pow).fold(Expression::Constant(pallas::Base::one()), |acc, _| {
                    acc * window.clone()
                })
            })
            .collect();

        let interpolated_x = window_pow.iter().zip(self.lagrange_coeffs.iter()).fold(
            Expression::Constant(pallas::Base::zero()),
            |acc, (window_pow, coeff)| acc + (window_pow.clone() * meta.query_fixed(*coeff)),
        );

        // Check interpolation of x-coordinate
        let x_check = interpolated_x - x_p.clone();
        // Check that `y + z = u^2`, where `z` is fixed and `u`, `y` are witnessed
        let y_check = u.square() - y_p.clone() - z;
        // Check that (x, y) is on the curve
        let on_curve =
            y_p.square() - x_p.clone().square() * x_p - Expression::Constant(pallas::Affine::b());

        vec![
            ("check x", x_check),
            ("check y", y_check),
            ("on-curve", on_curve),
        ]
    }

    #[allow(clippy::type_complexity)]
    fn assign_region_inner<F: FixedPoint<pallas::Affine>, const NUM_WINDOWS: usize>(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        scalar: &ScalarFixed,
        base: &F,
        coords_check_toggle: Selector,
    ) -> Result<(NonIdentityEccPoint, NonIdentityEccPoint), Error> {
        // Assign fixed columns for given fixed base
        self.assign_fixed_constants::<F, NUM_WINDOWS>(region, offset, base, coords_check_toggle)?;

        // Initialize accumulator
        let acc = self.initialize_accumulator::<F, NUM_WINDOWS>(region, offset, base, scalar)?;

        // Process all windows excluding least and most significant windows
        let acc = self.add_incomplete::<F, NUM_WINDOWS>(region, offset, acc, base, scalar)?;

        // Process most significant window
        let mul_b = self.process_msb::<F, NUM_WINDOWS>(region, offset, base, scalar)?;

        Ok((acc, mul_b))
    }

    /// [Specification](https://p.z.cash/halo2-0.1:ecc-fixed-mul-load-base).
    fn assign_fixed_constants<F: FixedPoint<pallas::Affine>, const NUM_WINDOWS: usize>(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        base: &F,
        coords_check_toggle: Selector,
    ) -> Result<(), Error> {
        let mut constants = None;
        let build_constants = || {
            let lagrange_coeffs = base.lagrange_coeffs();
            assert_eq!(lagrange_coeffs.len(), NUM_WINDOWS);

            let z = base.z();
            assert_eq!(z.len(), NUM_WINDOWS);

            (lagrange_coeffs, z)
        };

        // Assign fixed columns for given fixed base
        for window in 0..NUM_WINDOWS {
            coords_check_toggle.enable(region, window + offset)?;

            // Assign x-coordinate Lagrange interpolation coefficients
            for k in 0..H {
                region.assign_fixed(
                    || {
                        format!(
                            "Lagrange interpolation coeff for window: {:?}, k: {:?}",
                            window, k
                        )
                    },
                    self.lagrange_coeffs[k],
                    window + offset,
                    || {
                        if constants.as_ref().is_none() {
                            constants = Some(build_constants());
                        }
                        let lagrange_coeffs = &constants.as_ref().unwrap().0;
                        Value::known(lagrange_coeffs[window][k])
                    },
                )?;
            }

            // Assign z-values for each window
            region.assign_fixed(
                || format!("z-value for window: {:?}", window),
                self.fixed_z,
                window + offset,
                || {
                    let z = &constants.as_ref().unwrap().1;
                    Value::known(pallas::Base::from(z[window]))
                },
            )?;
        }

        Ok(())
    }

    /// Assigns the values used to process a window.
    fn process_window<F: FixedPoint<pallas::Affine>, const NUM_WINDOWS: usize>(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        w: usize,
        k_usize: Value<usize>,
        window_scalar: Value<pallas::Scalar>,
        base: &F,
    ) -> Result<NonIdentityEccPoint, Error> {
        let base_value = base.generator();
        let base_u = base.u();
        assert_eq!(base_u.len(), NUM_WINDOWS);

        // Compute [window_scalar]B
        let mul_b = {
            let mul_b = window_scalar.map(|scalar| base_value * scalar);
            let mul_b = mul_b.map(|mul_b| mul_b.to_affine().coordinates().unwrap());

            let x = mul_b.map(|mul_b| {
                let x = *mul_b.x();
                assert!(x != pallas::Base::zero());
                x.into()
            });
            let x = region.assign_advice(
                || format!("mul_b_x, window {}", w),
                self.add_config.x_p,
                offset + w,
                || x,
            )?;

            let y = mul_b.map(|mul_b| {
                let y = *mul_b.y();
                assert!(y != pallas::Base::zero());
                y.into()
            });
            let y = region.assign_advice(
                || format!("mul_b_y, window {}", w),
                self.add_config.y_p,
                offset + w,
                || y,
            )?;

            NonIdentityEccPoint::from_coordinates_unchecked(x, y)
        };

        // Assign u = (y_p + z_w).sqrt()
        let u_val = k_usize.map(|k| pallas::Base::from_repr(base_u[w][k]).unwrap());
        region.assign_advice(|| "u", self.u, offset + w, || u_val)?;

        Ok(mul_b)
    }

    fn initialize_accumulator<F: FixedPoint<pallas::Affine>, const NUM_WINDOWS: usize>(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        base: &F,
        scalar: &ScalarFixed,
    ) -> Result<NonIdentityEccPoint, Error> {
        // Recall that the message at each window `w` is represented as
        // `m_w = [(k_w + 2) ⋅ 8^w]B`.
        // When `w = 0`, we have `m_0 = [(k_0 + 2)]B`.
        let w = 0;
        let k0 = scalar.windows_field()[0];
        let k0_usize = scalar.windows_usize()[0];
        self.process_lower_bits::<_, NUM_WINDOWS>(region, offset, w, k0, k0_usize, base)
    }

    fn add_incomplete<F: FixedPoint<pallas::Affine>, const NUM_WINDOWS: usize>(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        mut acc: NonIdentityEccPoint,
        base: &F,
        scalar: &ScalarFixed,
    ) -> Result<NonIdentityEccPoint, Error> {
        let scalar_windows_field = scalar.windows_field();
        let scalar_windows_usize = scalar.windows_usize();
        assert_eq!(scalar_windows_field.len(), NUM_WINDOWS);

        for (w, (k, k_usize)) in scalar_windows_field
            .into_iter()
            .zip(scalar_windows_usize)
            .enumerate()
            // The MSB is processed separately.
            .take(NUM_WINDOWS - 1)
            // Skip k_0 (already processed).
            .skip(1)
        {
            // Compute [(k_w + 2) ⋅ 8^w]B
            //
            // This assigns the coordinates of the returned point into the input cells for
            // the incomplete addition gate, which will then copy them into themselves.
            let mul_b =
                self.process_lower_bits::<_, NUM_WINDOWS>(region, offset, w, k, k_usize, base)?;

            // Add to the accumulator.
            //
            // After the first loop, the accumulator will already be in the input cells
            // for the incomplete addition gate, and will be copied into themselves.
            acc = self
                .add_incomplete_config
                .assign_region(&mul_b, &acc, offset + w, region)?;
        }
        Ok(acc)
    }

    /// Assigns the values used to process a window that does not contain the MSB.
    fn process_lower_bits<F: FixedPoint<pallas::Affine>, const NUM_WINDOWS: usize>(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        w: usize,
        k: Value<pallas::Scalar>,
        k_usize: Value<usize>,
        base: &F,
    ) -> Result<NonIdentityEccPoint, Error> {
        // `scalar = [(k_w + 2) ⋅ 8^w]
        let scalar = k.map(|k| (k + *TWO_SCALAR) * (*H_SCALAR).pow(&[w as u64, 0, 0, 0]));

        self.process_window::<_, NUM_WINDOWS>(region, offset, w, k_usize, scalar, base)
    }

    /// Assigns the values used to process the window containing the MSB.
    fn process_msb<F: FixedPoint<pallas::Affine>, const NUM_WINDOWS: usize>(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        base: &F,
        scalar: &ScalarFixed,
    ) -> Result<NonIdentityEccPoint, Error> {
        let k_usize = scalar.windows_usize()[NUM_WINDOWS - 1];

        // offset_acc = \sum_{j = 0}^{NUM_WINDOWS - 2} 2^{FIXED_BASE_WINDOW_SIZE*j + 1}
        let offset_acc = (0..(NUM_WINDOWS - 1)).fold(pallas::Scalar::zero(), |acc, w| {
            acc + (*TWO_SCALAR).pow(&[FIXED_BASE_WINDOW_SIZE as u64 * w as u64 + 1, 0, 0, 0])
        });

        // `scalar = [k * 8^(NUM_WINDOWS - 1) - offset_acc]`.
        let scalar = scalar.windows_field()[scalar.windows_field().len() - 1]
            .map(|k| k * (*H_SCALAR).pow(&[(NUM_WINDOWS - 1) as u64, 0, 0, 0]) - offset_acc);

        self.process_window::<_, NUM_WINDOWS>(
            region,
            offset,
            NUM_WINDOWS - 1,
            k_usize,
            scalar,
            base,
        )
    }
}

enum ScalarFixed {
    FullWidth(EccScalarFixed),
    Short(EccScalarFixedShort),
    BaseFieldElem(EccBaseFieldElemFixed),
}

impl From<&EccScalarFixed> for ScalarFixed {
    fn from(scalar_fixed: &EccScalarFixed) -> Self {
        Self::FullWidth(scalar_fixed.clone())
    }
}

impl From<&EccScalarFixedShort> for ScalarFixed {
    fn from(scalar_fixed: &EccScalarFixedShort) -> Self {
        Self::Short(scalar_fixed.clone())
    }
}

impl From<&EccBaseFieldElemFixed> for ScalarFixed {
    fn from(base_field_elem: &EccBaseFieldElemFixed) -> Self {
        Self::BaseFieldElem(base_field_elem.clone())
    }
}

impl ScalarFixed {
    /// The scalar decomposition was done in the base field. For computation
    /// outside the circuit, we now convert them back into the scalar field.
    ///
    /// This function does not require that the base field fits inside the scalar field,
    /// because the window size fits into either field.
    fn windows_field(&self) -> Vec<Value<pallas::Scalar>> {
        let running_sum_to_windows = |zs: Vec<AssignedCell<pallas::Base, pallas::Base>>| {
            (0..(zs.len() - 1))
                .map(|idx| {
                    let z_cur = zs[idx].value();
                    let z_next = zs[idx + 1].value();
                    let word = z_cur - z_next * Value::known(*H_BASE);
                    // This assumes that the endianness of the encodings of pallas::Base
                    // and pallas::Scalar are the same. They happen to be, but we need to
                    // be careful if this is generalised.
                    word.map(|word| pallas::Scalar::from_repr(word.to_repr()).unwrap())
                })
                .collect::<Vec<_>>()
        };
        match self {
            Self::BaseFieldElem(scalar) => running_sum_to_windows(scalar.running_sum.to_vec()),
            Self::Short(scalar) => running_sum_to_windows(
                scalar
                    .running_sum
                    .as_ref()
                    .expect("EccScalarFixedShort has been constrained")
                    .to_vec(),
            ),
            Self::FullWidth(scalar) => scalar
                .windows
                .as_ref()
                .expect("EccScalarFixed has been witnessed")
                .iter()
                .map(|bits| {
                    // This assumes that the endianness of the encodings of pallas::Base
                    // and pallas::Scalar are the same. They happen to be, but we need to
                    // be careful if this is generalised.
                    bits.value()
                        .map(|value| pallas::Scalar::from_repr(value.to_repr()).unwrap())
                })
                .collect::<Vec<_>>(),
        }
    }

    /// The scalar decomposition is guaranteed to be in three-bit windows, so we construct
    /// `usize` indices from the lowest three bits of each window field element for
    /// convenient indexing into `u`-values.
    fn windows_usize(&self) -> Vec<Value<usize>> {
        self.windows_field()
            .iter()
            .map(|window| {
                window.map(|window| {
                    window
                        .to_le_bits()
                        .iter()
                        .by_vals()
                        .take(FIXED_BASE_WINDOW_SIZE)
                        .rev()
                        .fold(0, |acc, b| 2 * acc + usize::from(b))
                })
            })
            .collect::<Vec<_>>()
    }
}
