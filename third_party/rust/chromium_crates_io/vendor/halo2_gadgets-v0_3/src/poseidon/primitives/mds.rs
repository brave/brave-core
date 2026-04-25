use ff::FromUniformBytes;

use super::{grain::Grain, Mds};

pub(super) fn generate_mds<F: FromUniformBytes<64> + Ord, const T: usize>(
    grain: &mut Grain<F>,
    mut select: usize,
) -> (Mds<F, T>, Mds<F, T>) {
    let (xs, ys, mds) = loop {
        // Generate two [F; T] arrays of unique field elements.
        let (xs, ys) = loop {
            let mut vals: Vec<_> = (0..2 * T)
                .map(|_| grain.next_field_element_without_rejection())
                .collect();

            // Check that we have unique field elements.
            let mut unique = vals.clone();
            unique.sort_unstable();
            unique.dedup();
            if vals.len() == unique.len() {
                let rhs = vals.split_off(T);
                break (vals, rhs);
            }
        };

        // We need to ensure that the MDS is secure. Instead of checking the MDS against
        // the relevant algorithms directly, we witness a fixed number of MDS matrices
        // that we need to sample from the given Grain state before obtaining a secure
        // matrix. This can be determined out-of-band via the reference implementation in
        // Sage.
        if select != 0 {
            select -= 1;
            continue;
        }

        // Generate a Cauchy matrix, with elements a_ij in the form:
        //     a_ij = 1/(x_i + y_j); x_i + y_j != 0
        //
        // It would be much easier to use the alternate definition:
        //     a_ij = 1/(x_i - y_j); x_i - y_j != 0
        //
        // These are clearly equivalent on `y <- -y`, but it is easier to work with the
        // negative formulation, because ensuring that xs ∪ ys is unique implies that
        // x_i - y_j != 0 by construction (whereas the positive case does not hold). It
        // also makes computation of the matrix inverse simpler below (the theorem used
        // was formulated for the negative definition).
        //
        // However, the Poseidon paper and reference impl use the positive formulation,
        // and we want to rely on the reference impl for MDS security, so we use the same
        // formulation.
        let mut mds = [[F::ZERO; T]; T];
        #[allow(clippy::needless_range_loop)]
        for i in 0..T {
            for j in 0..T {
                let sum = xs[i] + ys[j];
                // We leverage the secure MDS selection counter to also check this.
                assert!(!sum.is_zero_vartime());
                mds[i][j] = sum.invert().unwrap();
            }
        }

        break (xs, ys, mds);
    };

    // Compute the inverse. All square Cauchy matrices have a non-zero determinant and
    // thus are invertible. The inverse for a Cauchy matrix of the form:
    //
    //     a_ij = 1/(x_i - y_j); x_i - y_j != 0
    //
    // has elements b_ij given by:
    //
    //     b_ij = (x_j - y_i) A_j(y_i) B_i(x_j)    (Schechter 1959, Theorem 1)
    //
    // where A_i(x) and B_i(x) are the Lagrange polynomials for xs and ys respectively.
    //
    // We adapt this to the positive Cauchy formulation by negating ys.
    let mut mds_inv = [[F::ZERO; T]; T];
    let l = |xs: &[F], j, x: F| {
        let x_j = xs[j];
        xs.iter().enumerate().fold(F::ONE, |acc, (m, x_m)| {
            if m == j {
                acc
            } else {
                // We hard-code the type, to avoid spurious "cannot infer type" rustc errors.
                let denominator: F = x_j - x_m;

                // We can invert freely; by construction, the elements of xs are distinct.
                let denominator_inverted: F = denominator.invert().unwrap();

                acc * (x - x_m) * denominator_inverted
            }
        })
    };
    let neg_ys: Vec<_> = ys.iter().map(|y| -*y).collect();
    for i in 0..T {
        for j in 0..T {
            mds_inv[i][j] = (xs[j] - neg_ys[i]) * l(&xs, j, neg_ys[i]) * l(&neg_ys, i, xs[j]);
        }
    }

    (mds, mds_inv)
}

#[cfg(test)]
mod tests {
    use group::ff::Field;
    use pasta_curves::Fp;

    use super::{generate_mds, Grain};

    #[test]
    fn poseidon_mds() {
        const T: usize = 3;
        let mut grain = Grain::new(super::super::grain::SboxType::Pow, T as u16, 8, 56);
        let (mds, mds_inv) = generate_mds::<Fp, T>(&mut grain, 0);

        // Verify that MDS * MDS^-1 = I.
        #[allow(clippy::needless_range_loop)]
        for i in 0..T {
            for j in 0..T {
                let expected = if i == j { Fp::ONE } else { Fp::ZERO };
                assert_eq!(
                    (0..T).fold(Fp::ZERO, |acc, k| acc + (mds[i][k] * mds_inv[k][j])),
                    expected
                );
            }
        }
    }
}
