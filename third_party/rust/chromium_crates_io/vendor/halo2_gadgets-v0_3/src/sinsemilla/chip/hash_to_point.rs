use super::super::{CommitDomains, HashDomains, SinsemillaInstructions};
use super::{NonIdentityEccPoint, SinsemillaChip};
use crate::{
    ecc::FixedPoints,
    sinsemilla::primitives::{self as sinsemilla, lebs2ip_k, INV_TWO_POW_K, SINSEMILLA_S},
};

use ff::Field;
use halo2_proofs::{
    circuit::{AssignedCell, Chip, Region, Value},
    plonk::{Assigned, Error},
};

use group::ff::{PrimeField, PrimeFieldBits};
use pasta_curves::{arithmetic::CurveAffine, pallas};

use std::ops::Deref;

impl<Hash, Commit, Fixed> SinsemillaChip<Hash, Commit, Fixed>
where
    Hash: HashDomains<pallas::Affine>,
    Fixed: FixedPoints<pallas::Affine>,
    Commit: CommitDomains<pallas::Affine, Fixed, Hash>,
{
    /// [Specification](https://p.z.cash/halo2-0.1:sinsemilla-constraints?partial).
    #[allow(non_snake_case)]
    #[allow(clippy::type_complexity)]
    pub(super) fn hash_message(
        &self,
        region: &mut Region<'_, pallas::Base>,
        Q: pallas::Affine,
        message: &<Self as SinsemillaInstructions<
            pallas::Affine,
            { sinsemilla::K },
            { sinsemilla::C },
        >>::Message,
    ) -> Result<
        (
            NonIdentityEccPoint,
            Vec<Vec<AssignedCell<pallas::Base, pallas::Base>>>,
        ),
        Error,
    > {
        let config = self.config().clone();
        let mut offset = 0;

        // Get the `x`- and `y`-coordinates of the starting `Q` base.
        let x_q = *Q.coordinates().unwrap().x();
        let y_q = *Q.coordinates().unwrap().y();

        // Constrain the initial x_a, lambda_1, lambda_2, x_p using the q_sinsemilla4
        // selector.
        let mut y_a: Y<pallas::Base> = {
            // Enable `q_sinsemilla4` on the first row.
            config.q_sinsemilla4.enable(region, offset)?;
            region.assign_fixed(
                || "fixed y_q",
                config.fixed_y_q,
                offset,
                || Value::known(y_q),
            )?;

            Value::known(y_q.into()).into()
        };

        // Constrain the initial x_q to equal the x-coordinate of the domain's `Q`.
        let mut x_a: X<pallas::Base> = {
            let x_a = region.assign_advice_from_constant(
                || "fixed x_q",
                config.double_and_add.x_a,
                offset,
                x_q.into(),
            )?;

            x_a.into()
        };

        let mut zs_sum: Vec<Vec<AssignedCell<pallas::Base, pallas::Base>>> = Vec::new();

        // Hash each piece in the message.
        for (idx, piece) in message.iter().enumerate() {
            let final_piece = idx == message.len() - 1;

            // The value of the accumulator after this piece is processed.
            let (x, y, zs) = self.hash_piece(region, offset, piece, x_a, y_a, final_piece)?;

            // Since each message word takes one row to process, we increase
            // the offset by `piece.num_words` on each iteration.
            offset += piece.num_words();

            // Update the accumulator to the latest value.
            x_a = x;
            y_a = y;
            zs_sum.push(zs);
        }

        // Assign the final y_a.
        let y_a = {
            // Assign the final y_a.
            let y_a_cell =
                region.assign_advice(|| "y_a", config.double_and_add.lambda_1, offset, || y_a.0)?;

            // Assign lambda_2 and x_p zero values since they are queried
            // in the gate. (The actual values do not matter since they are
            // multiplied by zero.)
            {
                region.assign_advice(
                    || "dummy lambda2",
                    config.double_and_add.lambda_2,
                    offset,
                    || Value::known(pallas::Base::zero()),
                )?;
                region.assign_advice(
                    || "dummy x_p",
                    config.double_and_add.x_p,
                    offset,
                    || Value::known(pallas::Base::zero()),
                )?;
            }

            y_a_cell
        };

        #[cfg(test)]
        #[allow(non_snake_case)]
        // Check equivalence to result from primitives::sinsemilla::hash_to_point
        {
            use crate::sinsemilla::primitives::{K, S_PERSONALIZATION};

            use group::{prime::PrimeCurveAffine, Curve};
            use pasta_curves::arithmetic::CurveExt;

            let field_elems: Value<Vec<_>> = message
                .iter()
                .map(|piece| piece.field_elem().map(|elem| (elem, piece.num_words())))
                .collect();

            field_elems
                .zip(x_a.value().zip(y_a.value()))
                .assert_if_known(|(field_elems, (x_a, y_a))| {
                    // Get message as a bitstring.
                    let bitstring: Vec<bool> = field_elems
                        .iter()
                        .flat_map(|(elem, num_words)| {
                            elem.to_le_bits().into_iter().take(K * num_words)
                        })
                        .collect();

                    let hasher_S = pallas::Point::hash_to_curve(S_PERSONALIZATION);
                    let S = |chunk: &[bool]| hasher_S(&lebs2ip_k(chunk).to_le_bytes());

                    // We can use complete addition here because it differs from
                    // incomplete addition with negligible probability.
                    let expected_point = bitstring
                        .chunks(K)
                        .fold(Q.to_curve(), |acc, chunk| (acc + S(chunk)) + acc);
                    let actual_point =
                        pallas::Affine::from_xy(x_a.evaluate(), y_a.evaluate()).unwrap();
                    expected_point.to_affine() == actual_point
                });
        }

        x_a.value()
            .zip(y_a.value())
            .error_if_known_and(|(x_a, y_a)| x_a.is_zero_vartime() || y_a.is_zero_vartime())?;
        Ok((
            NonIdentityEccPoint::from_coordinates_unchecked(x_a.0, y_a),
            zs_sum,
        ))
    }

    #[allow(clippy::type_complexity)]
    /// Hashes a message piece containing `piece.length` number of `K`-bit words.
    ///
    /// To avoid a duplicate assignment, the accumulator x-coordinate provided
    /// by the caller is not copied. This only works because `hash_piece()` is
    /// an internal API. Before this call to `hash_piece()`, x_a MUST have been
    /// already assigned within this region at the correct offset.
    fn hash_piece(
        &self,
        region: &mut Region<'_, pallas::Base>,
        offset: usize,
        piece: &<Self as SinsemillaInstructions<
            pallas::Affine,
            { sinsemilla::K },
            { sinsemilla::C },
        >>::MessagePiece,
        mut x_a: X<pallas::Base>,
        mut y_a: Y<pallas::Base>,
        final_piece: bool,
    ) -> Result<
        (
            X<pallas::Base>,
            Y<pallas::Base>,
            Vec<AssignedCell<pallas::Base, pallas::Base>>,
        ),
        Error,
    > {
        let config = self.config().clone();

        // Selector assignments
        {
            // Enable `q_sinsemilla1` selector on every row.
            for row in 0..piece.num_words() {
                config.q_sinsemilla1.enable(region, offset + row)?;
            }

            // Set `q_sinsemilla2` fixed column to 1 on every row but the last.
            for row in 0..(piece.num_words() - 1) {
                region.assign_fixed(
                    || "q_s2 = 1",
                    config.q_sinsemilla2,
                    offset + row,
                    || Value::known(pallas::Base::one()),
                )?;
            }

            // Set `q_sinsemilla2` fixed column to 0 on the last row if this is
            // not the final piece, or to 2 on the last row of the final piece.
            region.assign_fixed(
                || {
                    if final_piece {
                        "q_s2 for final piece"
                    } else {
                        "q_s2 between pieces"
                    }
                },
                config.q_sinsemilla2,
                offset + piece.num_words() - 1,
                || {
                    Value::known(if final_piece {
                        pallas::Base::from(2)
                    } else {
                        pallas::Base::zero()
                    })
                },
            )?;
        }

        // Message piece as K * piece.length bitstring
        let bitstring: Value<Vec<bool>> = piece.field_elem().map(|value| {
            value
                .to_le_bits()
                .into_iter()
                .take(sinsemilla::K * piece.num_words())
                .collect()
        });

        let words: Value<Vec<u32>> = bitstring.map(|bitstring| {
            bitstring
                .chunks_exact(sinsemilla::K)
                .map(lebs2ip_k)
                .collect()
        });

        // Get (x_p, y_p) for each word.
        let generators: Value<Vec<(pallas::Base, pallas::Base)>> = words.clone().map(|words| {
            words
                .iter()
                .map(|word| SINSEMILLA_S[*word as usize])
                .collect()
        });

        // Convert `words` from `Value<Vec<u32>>` to `Vec<Value<u32>>`
        let words = words.transpose_vec(piece.num_words());

        // Decompose message piece into `K`-bit pieces with a running sum `z`.
        let zs = {
            let mut zs = Vec::with_capacity(piece.num_words() + 1);

            // Copy message and initialize running sum `z` to decompose message in-circuit
            let initial_z = piece.cell_value().copy_advice(
                || "z_0 (copy of message piece)",
                region,
                config.bits,
                offset,
            )?;
            zs.push(initial_z);

            // Assign cumulative sum such that for 0 <= i < n,
            //          z_i = 2^K * z_{i + 1} + m_{i + 1}
            // => z_{i + 1} = (z_i - m_{i + 1}) / 2^K
            //
            // For a message piece m = m_1 + 2^K m_2 + ... + 2^{K(n-1)} m_n}, initialize z_0 = m.
            // We end up with z_n = 0. (z_n is not directly encoded as a cell value;
            // it is implicitly taken as 0 by adjusting the definition of m_{i+1}.)
            let mut z = piece.field_elem();
            let inv_2_k = Value::known(pallas::Base::from_repr(INV_TWO_POW_K).unwrap());

            // We do not assign the final z_n as it is constrained to be zero.
            for (idx, word) in words[0..(words.len() - 1)].iter().enumerate() {
                let word = word.map(|word| pallas::Base::from(word as u64));
                // z_{i + 1} = (z_i - m_{i + 1}) / 2^K
                z = (z - word) * inv_2_k;
                let cell = region.assign_advice(
                    || format!("z_{:?}", idx + 1),
                    config.bits,
                    offset + idx + 1,
                    || z,
                )?;
                zs.push(cell)
            }

            zs
        };

        // The accumulator x-coordinate provided by the caller MUST have been assigned
        // within this region.

        let generators = generators.transpose_vec(piece.num_words());

        for (row, gen) in generators.iter().enumerate() {
            let x_p = gen.map(|gen| gen.0);
            let y_p = gen.map(|gen| gen.1);

            // Assign `x_p`
            region.assign_advice(|| "x_p", config.double_and_add.x_p, offset + row, || x_p)?;

            // Compute and assign `lambda_1`
            let lambda_1 = {
                let lambda_1 = (y_a.0 - y_p) * (x_a.value() - x_p).invert();

                // Assign lambda_1
                region.assign_advice(
                    || "lambda_1",
                    config.double_and_add.lambda_1,
                    offset + row,
                    || lambda_1,
                )?;

                lambda_1
            };

            // Compute `x_r`
            let x_r = lambda_1.square() - x_a.value() - x_p;

            // Compute and assign `lambda_2`
            let lambda_2 = {
                let lambda_2 =
                    y_a.0 * pallas::Base::from(2) * (x_a.value() - x_r).invert() - lambda_1;

                region.assign_advice(
                    || "lambda_2",
                    config.double_and_add.lambda_2,
                    offset + row,
                    || lambda_2,
                )?;

                lambda_2
            };

            // Compute and assign `x_a` for the next row.
            let x_a_new: X<pallas::Base> = {
                let x_a_new = lambda_2.square() - x_a.value() - x_r;

                let x_a_cell = region.assign_advice(
                    || "x_a",
                    config.double_and_add.x_a,
                    offset + row + 1,
                    || x_a_new,
                )?;

                x_a_cell.into()
            };

            // Compute y_a for the next row.
            let y_a_new: Y<pallas::Base> =
                (lambda_2 * (x_a.value() - x_a_new.value()) - y_a.0).into();

            // Update the mutable `x_a`, `y_a` variables.
            x_a = x_a_new;
            y_a = y_a_new;
        }

        Ok((x_a, y_a, zs))
    }
}

/// The x-coordinate of the accumulator in a Sinsemilla hash instance.
struct X<F: Field>(AssignedCell<Assigned<F>, F>);

impl<F: Field> From<AssignedCell<Assigned<F>, F>> for X<F> {
    fn from(cell_value: AssignedCell<Assigned<F>, F>) -> Self {
        X(cell_value)
    }
}

impl<F: Field> Deref for X<F> {
    type Target = AssignedCell<Assigned<F>, F>;

    fn deref(&self) -> &AssignedCell<Assigned<F>, F> {
        &self.0
    }
}

/// The y-coordinate of the accumulator in a Sinsemilla hash instance.
///
/// This is never actually witnessed until the last round, since it
/// can be derived from other variables. Thus it only exists as a field
/// element, not a `CellValue`.
struct Y<F: Field>(Value<Assigned<F>>);

impl<F: Field> From<Value<Assigned<F>>> for Y<F> {
    fn from(value: Value<Assigned<F>>) -> Self {
        Y(value)
    }
}

impl<F: Field> Deref for Y<F> {
    type Target = Value<Assigned<F>>;

    fn deref(&self) -> &Value<Assigned<F>> {
        &self.0
    }
}
