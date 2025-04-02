//! Constants required for the ECC chip.

use arrayvec::ArrayVec;
use group::{
    ff::{Field, PrimeField},
    Curve,
};
use halo2_proofs::arithmetic::lagrange_interpolate;
use pasta_curves::{arithmetic::CurveAffine, pallas};

/// Window size for fixed-base scalar multiplication
pub const FIXED_BASE_WINDOW_SIZE: usize = 3;

/// $2^{`FIXED_BASE_WINDOW_SIZE`}$
pub const H: usize = 1 << FIXED_BASE_WINDOW_SIZE;

/// Number of windows for a full-width scalar
pub const NUM_WINDOWS: usize =
    (pallas::Scalar::NUM_BITS as usize + FIXED_BASE_WINDOW_SIZE - 1) / FIXED_BASE_WINDOW_SIZE;

/// Number of windows for a short signed scalar
pub const NUM_WINDOWS_SHORT: usize =
    (L_SCALAR_SHORT + FIXED_BASE_WINDOW_SIZE - 1) / FIXED_BASE_WINDOW_SIZE;

/// $\ell_\mathsf{value}$
/// Number of bits in an unsigned short scalar.
pub(crate) const L_SCALAR_SHORT: usize = 64;

/// The Pallas scalar field modulus is $q = 2^{254} + \mathsf{t_q}$.
/// <https://github.com/zcash/pasta>
pub(crate) const T_Q: u128 = 45560315531506369815346746415080538113;

/// The Pallas base field modulus is $p = 2^{254} + \mathsf{t_p}$.
/// <https://github.com/zcash/pasta>
pub(crate) const T_P: u128 = 45560315531419706090280762371685220353;

/// For each fixed base, we calculate its scalar multiples in three-bit windows.
/// Each window will have $2^3 = 8$ points. The tables are computed as described in
/// [the Halo 2 book](https://zcash.github.io/halo2/design/gadgets/ecc/fixed-base-scalar-mul.html#load-fixed-base).
fn compute_window_table<C: CurveAffine>(base: C, num_windows: usize) -> Vec<[C; H]> {
    let mut window_table: Vec<[C; H]> = Vec::with_capacity(num_windows);

    // Generate window table entries for all windows but the last.
    // For these first `num_windows - 1` windows, we compute the multiple [(k+2)*(2^3)^w]B.
    // Here, w ranges from [0..`num_windows - 1`)
    for w in 0..(num_windows - 1) {
        window_table.push(
            (0..H)
                .map(|k| {
                    // scalar = (k+2)*(8^w)
                    let scalar = C::Scalar::from(k as u64 + 2)
                        * C::Scalar::from(H as u64).pow(&[w as u64, 0, 0, 0]);
                    (base * scalar).to_affine()
                })
                .collect::<ArrayVec<C, H>>()
                .into_inner()
                .unwrap(),
        );
    }

    // Generate window table entries for the last window, w = `num_windows - 1`.
    // For the last window, we compute [k * (2^3)^w - sum]B, where sum is defined
    // as sum = \sum_{j = 0}^{`num_windows - 2`} 2^{3j+1}
    let sum = (0..(num_windows - 1)).fold(C::Scalar::ZERO, |acc, j| {
        acc + C::Scalar::from(2).pow(&[FIXED_BASE_WINDOW_SIZE as u64 * j as u64 + 1, 0, 0, 0])
    });
    window_table.push(
        (0..H)
            .map(|k| {
                // scalar = k * (2^3)^w - sum, where w = `num_windows - 1`
                let scalar = C::Scalar::from(k as u64)
                    * C::Scalar::from(H as u64).pow(&[(num_windows - 1) as u64, 0, 0, 0])
                    - sum;
                (base * scalar).to_affine()
            })
            .collect::<ArrayVec<C, H>>()
            .into_inner()
            .unwrap(),
    );

    window_table
}

/// For each window, we interpolate the $x$-coordinate.
/// Here, we pre-compute and store the coefficients of the interpolation polynomial.
pub fn compute_lagrange_coeffs<C: CurveAffine>(base: C, num_windows: usize) -> Vec<[C::Base; H]> {
    // We are interpolating over the 3-bit window, k \in [0..8)
    let points: Vec<_> = (0..H).map(|i| C::Base::from(i as u64)).collect();

    let window_table = compute_window_table(base, num_windows);

    window_table
        .iter()
        .map(|window_points| {
            let x_window_points: Vec<_> = window_points
                .iter()
                .map(|point| *point.coordinates().unwrap().x())
                .collect();
            lagrange_interpolate(&points, &x_window_points)
                .into_iter()
                .collect::<ArrayVec<C::Base, H>>()
                .into_inner()
                .unwrap()
        })
        .collect()
}

/// For each window, $z$ is a field element such that for each point $(x, y)$ in the window:
/// - $z + y = u^2$ (some square in the field); and
/// - $z - y$ is not a square.
/// If successful, return a vector of `(z: u64, us: [C::Base; H])` for each window.
///
/// This function was used to generate the `z`s and `u`s for the Orchard fixed
/// bases. The outputs of this function have been stored as constants, and it
/// is not called anywhere in this codebase. However, we keep this function here
/// as a utility for those who wish to use it with different parameters.
pub fn find_zs_and_us<C: CurveAffine>(
    base: C,
    num_windows: usize,
) -> Option<Vec<(u64, [C::Base; H])>> {
    // Closure to find z and u's for one window
    let find_z_and_us = |window_points: &[C]| {
        assert_eq!(H, window_points.len());

        let ys: Vec<_> = window_points
            .iter()
            .map(|point| *point.coordinates().unwrap().y())
            .collect();
        (0..(1000 * (1 << (2 * H)))).find_map(|z| {
            ys.iter()
                .map(|&y| {
                    if (-y + C::Base::from(z)).sqrt().is_none().into() {
                        (y + C::Base::from(z)).sqrt().into()
                    } else {
                        None
                    }
                })
                .collect::<Option<ArrayVec<C::Base, H>>>()
                .map(|us| (z, us.into_inner().unwrap()))
        })
    };

    let window_table = compute_window_table(base, num_windows);
    window_table
        .iter()
        .map(|window_points| find_z_and_us(window_points))
        .collect()
}

/// Test that the z-values and u-values satisfy the conditions:
///      1. z + y = u^2,
///      2. z - y is not a square
/// for the y-coordinate of each fixed-base multiple in each window.
#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub fn test_zs_and_us<C: CurveAffine>(base: C, z: &[u64], u: &[[[u8; 32]; H]], num_windows: usize) {
    let window_table = compute_window_table(base, num_windows);

    for ((u, z), window_points) in u.iter().zip(z.iter()).zip(window_table) {
        for (u, point) in u.iter().zip(window_points.iter()) {
            let y = *point.coordinates().unwrap().y();
            let mut u_repr = <C::Base as PrimeField>::Repr::default();
            u_repr.as_mut().copy_from_slice(u);
            let u = C::Base::from_repr(u_repr).unwrap();
            assert_eq!(C::Base::from(*z) + y, u * u); // allow either square root
            assert!(bool::from((C::Base::from(*z) - y).sqrt().is_none()));
        }
    }
}

/// Test that Lagrange interpolation coefficients reproduce the correct x-coordinate
/// for each fixed-base multiple in each window.
#[cfg(any(test, feature = "test-dependencies"))]
#[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
pub fn test_lagrange_coeffs<C: CurveAffine>(base: C, num_windows: usize) {
    /// Evaluate y = f(x) given the coefficients of f(x)
    fn evaluate<C: CurveAffine>(x: u8, coeffs: &[C::Base]) -> C::Base {
        let x = C::Base::from(x as u64);
        coeffs
            .iter()
            .rev()
            .cloned()
            .reduce(|acc, coeff| acc * x + coeff)
            .unwrap_or(C::Base::ZERO)
    }

    let lagrange_coeffs = compute_lagrange_coeffs(base, num_windows);

    // Check first 84 windows, i.e. `k_0, k_1, ..., k_83`
    for (idx, coeffs) in lagrange_coeffs[0..(num_windows - 1)].iter().enumerate() {
        // Test each three-bit chunk in this window.
        for bits in 0..(H as u8) {
            {
                // Interpolate the x-coordinate using this window's coefficients
                let interpolated_x = evaluate::<C>(bits, coeffs);

                // Compute the actual x-coordinate of the multiple [(k+2)*(8^w)]B.
                let point = base
                    * C::Scalar::from(bits as u64 + 2)
                    * C::Scalar::from(H as u64).pow(&[idx as u64, 0, 0, 0]);
                let x = *point.to_affine().coordinates().unwrap().x();

                // Check that the interpolated x-coordinate matches the actual one.
                assert_eq!(x, interpolated_x);
            }
        }
    }

    // Check last window.
    for bits in 0..(H as u8) {
        // Interpolate the x-coordinate using the last window's coefficients
        let interpolated_x = evaluate::<C>(bits, &lagrange_coeffs[num_windows - 1]);

        // Compute the actual x-coordinate of the multiple [k * (8^84) - offset]B,
        // where offset = \sum_{j = 0}^{83} 2^{3j+1}
        let offset = (0..(num_windows - 1)).fold(C::Scalar::ZERO, |acc, w| {
            acc + C::Scalar::from(2).pow(&[FIXED_BASE_WINDOW_SIZE as u64 * w as u64 + 1, 0, 0, 0])
        });
        let scalar = C::Scalar::from(bits as u64)
            * C::Scalar::from(H as u64).pow(&[(num_windows - 1) as u64, 0, 0, 0])
            - offset;
        let point = base * scalar;
        let x = *point.to_affine().coordinates().unwrap().x();

        // Check that the interpolated x-coordinate matches the actual one.
        assert_eq!(x, interpolated_x);
    }
}

#[cfg(test)]
mod tests {
    use ff::FromUniformBytes;
    use group::{ff::Field, Curve, Group};
    use pasta_curves::{arithmetic::CurveAffine, pallas};
    use proptest::prelude::*;

    use super::{compute_window_table, find_zs_and_us, test_lagrange_coeffs, H, NUM_WINDOWS};

    prop_compose! {
        /// Generate an arbitrary Pallas point.
        pub fn arb_point()(bytes in prop::array::uniform32(0u8..)) -> pallas::Point {
            // Instead of rejecting out-of-range bytes, let's reduce them.
            let mut buf = [0; 64];
            buf[..32].copy_from_slice(&bytes);
            let scalar = pallas::Scalar::from_uniform_bytes(&buf);
            pallas::Point::generator() * scalar
        }
    }

    proptest! {
        #[test]
        fn lagrange_coeffs(
            base in arb_point(),
        ) {
            test_lagrange_coeffs(base.to_affine(), NUM_WINDOWS);
        }
    }

    #[test]
    fn zs_and_us() {
        let base = pallas::Point::random(rand::rngs::OsRng);
        let (z, u): (Vec<u64>, Vec<[pallas::Base; H]>) =
            find_zs_and_us(base.to_affine(), NUM_WINDOWS)
                .unwrap()
                .into_iter()
                .unzip();
        let window_table = compute_window_table(base.to_affine(), NUM_WINDOWS);

        for ((u, z), window_points) in u.iter().zip(z.iter()).zip(window_table) {
            for (u, point) in u.iter().zip(window_points.iter()) {
                let y = *point.coordinates().unwrap().y();
                assert_eq!(pallas::Base::from(*z) + y, u * u); // allow either square root
                assert!(bool::from((pallas::Base::from(*z) - y).sqrt().is_none()));
            }
        }
    }
}
