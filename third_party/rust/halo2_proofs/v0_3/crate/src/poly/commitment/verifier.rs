use group::{
    ff::{BatchInvert, Field},
    Curve,
};

use super::super::Error;
use super::{Params, MSM};
use crate::transcript::{EncodedChallenge, TranscriptRead};

use crate::arithmetic::{best_multiexp, CurveAffine};

/// A guard returned by the verifier
#[derive(Debug, Clone)]
pub struct Guard<'a, C: CurveAffine, E: EncodedChallenge<C>> {
    msm: MSM<'a, C>,
    neg_c: C::Scalar,
    u: Vec<C::Scalar>,
    u_packed: Vec<E>,
}

/// An accumulator instance consisting of an evaluation claim and a proof.
#[derive(Debug, Clone)]
pub struct Accumulator<C: CurveAffine, E: EncodedChallenge<C>> {
    /// The claimed output of the linear-time polycommit opening protocol
    pub g: C,

    /// A vector of challenges u_0, ..., u_{k - 1} sampled by the verifier, to
    /// be used in computing G'_0.
    pub u_packed: Vec<E>,
}

impl<'a, C: CurveAffine, E: EncodedChallenge<C>> Guard<'a, C, E> {
    /// Lets caller supply the challenges and obtain an MSM with updated
    /// scalars and points.
    pub fn use_challenges(mut self) -> MSM<'a, C> {
        let s = compute_s(&self.u, self.neg_c);
        self.msm.add_to_g_scalars(&s);

        self.msm
    }

    /// Lets caller supply the purported G point and simply appends
    /// [-c] G to return an updated MSM.
    pub fn use_g(mut self, g: C) -> (MSM<'a, C>, Accumulator<C, E>) {
        self.msm.append_term(self.neg_c, g);

        let accumulator = Accumulator {
            g,
            u_packed: self.u_packed,
        };

        (self.msm, accumulator)
    }

    /// Computes G = ⟨s, params.g⟩
    pub fn compute_g(&self) -> C {
        let s = compute_s(&self.u, C::Scalar::ONE);

        best_multiexp(&s, &self.msm.params.g).to_affine()
    }
}

/// Checks to see if the proof represented within `transcript` is valid, and a
/// point `x` that the polynomial commitment `P` opens purportedly to the value
/// `v`. The provided `msm` should evaluate to the commitment `P` being opened.
pub fn verify_proof<'a, C: CurveAffine, E: EncodedChallenge<C>, T: TranscriptRead<C, E>>(
    params: &'a Params<C>,
    mut msm: MSM<'a, C>,
    transcript: &mut T,
    x: C::Scalar,
    v: C::Scalar,
) -> Result<Guard<'a, C, E>, Error> {
    let k = params.k as usize;

    // P' = P - [v] G_0 + [ξ] S
    msm.add_constant_term(-v); // add [-v] G_0
    let s_poly_commitment = transcript.read_point().map_err(|_| Error::OpeningError)?;
    let xi = *transcript.squeeze_challenge_scalar::<()>();
    msm.append_term(xi, s_poly_commitment);

    let z = *transcript.squeeze_challenge_scalar::<()>();

    let mut rounds = vec![];
    for _ in 0..k {
        // Read L and R from the proof and write them to the transcript
        let l = transcript.read_point().map_err(|_| Error::OpeningError)?;
        let r = transcript.read_point().map_err(|_| Error::OpeningError)?;

        let u_j_packed = transcript.squeeze_challenge();
        let u_j = *u_j_packed.as_challenge_scalar::<()>();

        rounds.push((l, r, u_j, /* to be inverted */ u_j, u_j_packed));
    }

    rounds
        .iter_mut()
        .map(|&mut (_, _, _, ref mut u_j, _)| u_j)
        .batch_invert();

    // This is the left-hand side of the verifier equation.
    // P' + \sum([u_j^{-1}] L_j) + \sum([u_j] R_j)
    let mut u = Vec::with_capacity(k);
    let mut u_packed: Vec<E> = Vec::with_capacity(k);
    for (l, r, u_j, u_j_inv, u_j_packed) in rounds {
        msm.append_term(u_j_inv, l);
        msm.append_term(u_j, r);

        u.push(u_j);
        u_packed.push(u_j_packed);
    }

    // Our goal is to check that the left hand side of the verifier
    // equation
    //     P' + \sum([u_j^{-1}] L_j) + \sum([u_j] R_j)
    // equals (given b = \mathbf{b}_0, and the prover's values c, f),
    // the right-hand side
    //   = [c] (G'_0 + [b * z] U) + [f] W
    // Subtracting the right-hand side from both sides we get
    //   P' + \sum([u_j^{-1}] L_j) + \sum([u_j] R_j)
    //   + [-c] G'_0 + [-cbz] U + [-f] W
    //   = 0
    //
    // Note that the guard returned from this function does not include
    // the [-c]G'_0 term.

    let c = transcript.read_scalar().map_err(|_| Error::SamplingError)?;
    let neg_c = -c;
    let f = transcript.read_scalar().map_err(|_| Error::SamplingError)?;
    let b = compute_b(x, &u);

    msm.add_to_u_scalar(neg_c * &b * &z);
    msm.add_to_w_scalar(-f);

    let guard = Guard {
        msm,
        neg_c,
        u,
        u_packed,
    };

    Ok(guard)
}

/// Computes $\prod\limits_{i=0}^{k-1} (1 + u_{k - 1 - i} x^{2^i})$.
fn compute_b<F: Field>(x: F, u: &[F]) -> F {
    let mut tmp = F::ONE;
    let mut cur = x;
    for u_j in u.iter().rev() {
        tmp *= F::ONE + &(*u_j * &cur);
        cur *= cur;
    }
    tmp
}

/// Computes the coefficients of $g(X) = \prod\limits_{i=0}^{k-1} (1 + u_{k - 1 - i} X^{2^i})$.
fn compute_s<F: Field>(u: &[F], init: F) -> Vec<F> {
    assert!(!u.is_empty());
    let mut v = vec![F::ZERO; 1 << u.len()];
    v[0] = init;

    for (len, u_j) in u.iter().rev().enumerate().map(|(i, u_j)| (1 << i, u_j)) {
        let (left, right) = v.split_at_mut(len);
        let right = &mut right[0..len];
        right.copy_from_slice(left);
        for v in right {
            *v *= u_j;
        }
    }

    v
}
