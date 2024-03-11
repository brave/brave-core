use super::super::{
    commitment::{self, Blind, Params},
    Coeff, Polynomial,
};
use super::{
    construct_intermediate_sets, ChallengeX1, ChallengeX2, ChallengeX3, ChallengeX4, ProverQuery,
    Query,
};

use crate::arithmetic::{eval_polynomial, kate_division, CurveAffine};
use crate::transcript::{EncodedChallenge, TranscriptWrite};

use ff::Field;
use group::Curve;
use rand_core::RngCore;
use std::io;
use std::marker::PhantomData;

/// Create a multi-opening proof
pub fn create_proof<
    'a,
    I,
    C: CurveAffine,
    E: EncodedChallenge<C>,
    R: RngCore,
    T: TranscriptWrite<C, E>,
>(
    params: &Params<C>,
    mut rng: R,
    transcript: &mut T,
    queries: I,
) -> io::Result<()>
where
    I: IntoIterator<Item = ProverQuery<'a, C>> + Clone,
{
    let x_1: ChallengeX1<_> = transcript.squeeze_challenge_scalar();
    let x_2: ChallengeX2<_> = transcript.squeeze_challenge_scalar();

    let (poly_map, point_sets) = construct_intermediate_sets(queries);

    // Collapse openings at same point sets together into single openings using
    // x_1 challenge.
    let mut q_polys: Vec<Option<Polynomial<C::Scalar, Coeff>>> = vec![None; point_sets.len()];
    let mut q_blinds = vec![Blind(C::Scalar::ZERO); point_sets.len()];

    {
        let mut accumulate =
            |set_idx: usize, new_poly: &Polynomial<C::Scalar, Coeff>, blind: Blind<C::Scalar>| {
                if let Some(poly) = &q_polys[set_idx] {
                    q_polys[set_idx] = Some(poly.clone() * *x_1 + new_poly);
                } else {
                    q_polys[set_idx] = Some(new_poly.clone());
                }
                q_blinds[set_idx] *= *x_1;
                q_blinds[set_idx] += blind;
            };

        for commitment_data in poly_map.into_iter() {
            accumulate(
                commitment_data.set_index,        // set_idx,
                commitment_data.commitment.poly,  // poly,
                commitment_data.commitment.blind, // blind,
            );
        }
    }

    let q_prime_poly = point_sets
        .iter()
        .zip(q_polys.iter())
        .fold(None, |q_prime_poly, (points, poly)| {
            let mut poly = points
                .iter()
                .fold(poly.clone().unwrap().values, |poly, point| {
                    kate_division(&poly, *point)
                });
            poly.resize(params.n as usize, C::Scalar::ZERO);
            let poly = Polynomial {
                values: poly,
                _marker: PhantomData,
            };

            if q_prime_poly.is_none() {
                Some(poly)
            } else {
                q_prime_poly.map(|q_prime_poly| q_prime_poly * *x_2 + &poly)
            }
        })
        .unwrap();

    let q_prime_blind = Blind(C::Scalar::random(&mut rng));
    let q_prime_commitment = params.commit(&q_prime_poly, q_prime_blind).to_affine();

    transcript.write_point(q_prime_commitment)?;

    let x_3: ChallengeX3<_> = transcript.squeeze_challenge_scalar();

    // Prover sends u_i for all i, which correspond to the evaluation
    // of each Q polynomial commitment at x_3.
    for q_i_poly in &q_polys {
        transcript.write_scalar(eval_polynomial(q_i_poly.as_ref().unwrap(), *x_3))?;
    }

    let x_4: ChallengeX4<_> = transcript.squeeze_challenge_scalar();

    let (p_poly, p_poly_blind) = q_polys.into_iter().zip(q_blinds.into_iter()).fold(
        (q_prime_poly, q_prime_blind),
        |(q_prime_poly, q_prime_blind), (poly, blind)| {
            (
                q_prime_poly * *x_4 + &poly.unwrap(),
                Blind((q_prime_blind.0 * &(*x_4)) + &blind.0),
            )
        },
    );

    commitment::create_proof(params, rng, transcript, &p_poly, p_poly_blind, *x_3)
}

#[doc(hidden)]
#[derive(Copy, Clone)]
pub struct PolynomialPointer<'a, C: CurveAffine> {
    poly: &'a Polynomial<C::Scalar, Coeff>,
    blind: commitment::Blind<C::Scalar>,
}

impl<'a, C: CurveAffine> PartialEq for PolynomialPointer<'a, C> {
    fn eq(&self, other: &Self) -> bool {
        std::ptr::eq(self.poly, other.poly)
    }
}

impl<'a, C: CurveAffine> Query<C::Scalar> for ProverQuery<'a, C> {
    type Commitment = PolynomialPointer<'a, C>;
    type Eval = ();

    fn get_point(&self) -> C::Scalar {
        self.point
    }
    fn get_eval(&self) {}
    fn get_commitment(&self) -> Self::Commitment {
        PolynomialPointer {
            poly: self.poly,
            blind: self.blind,
        }
    }
}
