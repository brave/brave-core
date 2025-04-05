//! Implementation of Sinsemilla outside the circuit.

use group::{Curve, Wnaf};
use halo2_proofs::arithmetic::{CurveAffine, CurveExt};
use pasta_curves::pallas;
use subtle::CtOption;

mod addition;
use self::addition::IncompletePoint;
mod sinsemilla_s;
pub use sinsemilla_s::SINSEMILLA_S;

/// Number of bits of each message piece in $\mathsf{SinsemillaHashToPoint}$
pub const K: usize = 10;

/// $\frac{1}{2^K}$
pub const INV_TWO_POW_K: [u8; 32] = [
    1, 0, 192, 196, 160, 229, 70, 82, 221, 165, 74, 202, 85, 7, 62, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 240, 63,
];

/// The largest integer such that $2^c \leq (r_P - 1) / 2$, where $r_P$ is the order
/// of Pallas.
pub const C: usize = 253;

// Sinsemilla Q generators

/// SWU hash-to-curve personalization for Sinsemilla $Q$ generators.
pub const Q_PERSONALIZATION: &str = "z.cash:SinsemillaQ";

// Sinsemilla S generators

/// SWU hash-to-curve personalization for Sinsemilla $S$ generators.
pub const S_PERSONALIZATION: &str = "z.cash:SinsemillaS";

pub(crate) fn lebs2ip_k(bits: &[bool]) -> u32 {
    assert!(bits.len() == K);
    bits.iter()
        .enumerate()
        .fold(0u32, |acc, (i, b)| acc + if *b { 1 << i } else { 0 })
}

/// Coordinate extractor for Pallas.
///
/// Defined in [Zcash Protocol Spec § 5.4.9.7: Coordinate Extractor for Pallas][concreteextractorpallas].
///
/// [concreteextractorpallas]: https://zips.z.cash/protocol/nu5.pdf#concreteextractorpallas
fn extract_p_bottom(point: CtOption<pallas::Point>) -> CtOption<pallas::Base> {
    point.map(|p| {
        p.to_affine()
            .coordinates()
            .map(|c| *c.x())
            .unwrap_or_else(pallas::Base::zero)
    })
}

/// Pads the given iterator (which MUST have length $\leq K * C$) with zero-bits to a
/// multiple of $K$ bits.
struct Pad<I: Iterator<Item = bool>> {
    /// The iterator we are padding.
    inner: I,
    /// The measured length of the inner iterator.
    ///
    /// This starts as a lower bound, and will be accurate once `padding_left.is_some()`.
    len: usize,
    /// The amount of padding that remains to be emitted.
    padding_left: Option<usize>,
}

impl<I: Iterator<Item = bool>> Pad<I> {
    fn new(inner: I) -> Self {
        Pad {
            inner,
            len: 0,
            padding_left: None,
        }
    }
}

impl<I: Iterator<Item = bool>> Iterator for Pad<I> {
    type Item = bool;

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            // If we have identified the required padding, the inner iterator has ended,
            // and we will never poll it again.
            if let Some(n) = self.padding_left.as_mut() {
                if *n == 0 {
                    // Either we already emitted all necessary padding, or there was no
                    // padding required.
                    break None;
                } else {
                    // Emit the next padding bit.
                    *n -= 1;
                    break Some(false);
                }
            } else if let Some(ret) = self.inner.next() {
                // We haven't reached the end of the inner iterator yet.
                self.len += 1;
                assert!(self.len <= K * C);
                break Some(ret);
            } else {
                // Inner iterator just ended, so we now know its length.
                let rem = self.len % K;
                if rem > 0 {
                    // The inner iterator requires padding in the range [1,K).
                    self.padding_left = Some(K - rem);
                } else {
                    // No padding required.
                    self.padding_left = Some(0);
                }
            }
        }
    }
}

/// A domain in which $\mathsf{SinsemillaHashToPoint}$ and $\mathsf{SinsemillaHash}$ can
/// be used.
#[derive(Debug, Clone)]
#[allow(non_snake_case)]
pub struct HashDomain {
    Q: pallas::Point,
}

impl HashDomain {
    /// Constructs a new `HashDomain` with a specific prefix string.
    pub fn new(domain: &str) -> Self {
        HashDomain {
            Q: pallas::Point::hash_to_curve(Q_PERSONALIZATION)(domain.as_bytes()),
        }
    }

    /// $\mathsf{SinsemillaHashToPoint}$ from [§ 5.4.1.9][concretesinsemillahash].
    ///
    /// [concretesinsemillahash]: https://zips.z.cash/protocol/nu5.pdf#concretesinsemillahash
    pub fn hash_to_point(&self, msg: impl Iterator<Item = bool>) -> CtOption<pallas::Point> {
        self.hash_to_point_inner(msg).into()
    }

    #[allow(non_snake_case)]
    fn hash_to_point_inner(&self, msg: impl Iterator<Item = bool>) -> IncompletePoint {
        let padded: Vec<_> = Pad::new(msg).collect();

        padded
            .chunks(K)
            .fold(IncompletePoint::from(self.Q), |acc, chunk| {
                let (S_x, S_y) = SINSEMILLA_S[lebs2ip_k(chunk) as usize];
                let S_chunk = pallas::Affine::from_xy(S_x, S_y).unwrap();
                (acc + S_chunk) + acc
            })
    }

    /// $\mathsf{SinsemillaHash}$ from [§ 5.4.1.9][concretesinsemillahash].
    ///
    /// [concretesinsemillahash]: https://zips.z.cash/protocol/nu5.pdf#concretesinsemillahash
    ///
    /// # Panics
    ///
    /// This panics if the message length is greater than [`K`] * [`C`]
    pub fn hash(&self, msg: impl Iterator<Item = bool>) -> CtOption<pallas::Base> {
        extract_p_bottom(self.hash_to_point(msg))
    }

    /// Constructs a new `HashDomain` from a given `Q`.
    ///
    /// This is only for testing use.
    #[cfg(test)]
    #[allow(non_snake_case)]
    pub(crate) fn from_Q(Q: pallas::Point) -> Self {
        HashDomain { Q }
    }

    /// Returns the Sinsemilla $Q$ constant for this domain.
    #[cfg(any(test, feature = "test-dependencies"))]
    #[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
    #[allow(non_snake_case)]
    pub fn Q(&self) -> pallas::Point {
        self.Q
    }
}

/// A domain in which $\mathsf{SinsemillaCommit}$ and $\mathsf{SinsemillaShortCommit}$ can
/// be used.
#[derive(Debug)]
#[allow(non_snake_case)]
pub struct CommitDomain {
    M: HashDomain,
    R: pallas::Point,
}

impl CommitDomain {
    /// Constructs a new `CommitDomain` with a specific prefix string.
    pub fn new(domain: &str) -> Self {
        let m_prefix = format!("{}-M", domain);
        let r_prefix = format!("{}-r", domain);
        let hasher_r = pallas::Point::hash_to_curve(&r_prefix);
        CommitDomain {
            M: HashDomain::new(&m_prefix),
            R: hasher_r(&[]),
        }
    }

    /// $\mathsf{SinsemillaCommit}$ from [§ 5.4.8.4][concretesinsemillacommit].
    ///
    /// [concretesinsemillacommit]: https://zips.z.cash/protocol/nu5.pdf#concretesinsemillacommit
    #[allow(non_snake_case)]
    pub fn commit(
        &self,
        msg: impl Iterator<Item = bool>,
        r: &pallas::Scalar,
    ) -> CtOption<pallas::Point> {
        // We use complete addition for the blinding factor.
        CtOption::<pallas::Point>::from(self.M.hash_to_point_inner(msg))
            .map(|p| p + Wnaf::new().scalar(r).base(self.R))
    }

    /// $\mathsf{SinsemillaShortCommit}$ from [§ 5.4.8.4][concretesinsemillacommit].
    ///
    /// [concretesinsemillacommit]: https://zips.z.cash/protocol/nu5.pdf#concretesinsemillacommit
    pub fn short_commit(
        &self,
        msg: impl Iterator<Item = bool>,
        r: &pallas::Scalar,
    ) -> CtOption<pallas::Base> {
        extract_p_bottom(self.commit(msg, r))
    }

    /// Returns the Sinsemilla $R$ constant for this domain.
    #[cfg(any(test, feature = "test-dependencies"))]
    #[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
    #[allow(non_snake_case)]
    pub fn R(&self) -> pallas::Point {
        self.R
    }

    /// Returns the Sinsemilla $Q$ constant for this domain.
    #[cfg(any(test, feature = "test-dependencies"))]
    #[cfg_attr(docsrs, doc(cfg(feature = "test-dependencies")))]
    #[allow(non_snake_case)]
    pub fn Q(&self) -> pallas::Point {
        self.M.Q
    }
}

#[cfg(test)]
mod tests {
    use super::{Pad, K};
    use pasta_curves::{arithmetic::CurveExt, pallas};

    #[test]
    fn pad() {
        assert_eq!(
            Pad::new([].iter().cloned()).collect::<Vec<_>>(),
            vec![false; 0]
        );
        assert_eq!(
            Pad::new([true].iter().cloned()).collect::<Vec<_>>(),
            vec![true, false, false, false, false, false, false, false, false, false]
        );
        assert_eq!(
            Pad::new([true, true].iter().cloned()).collect::<Vec<_>>(),
            vec![true, true, false, false, false, false, false, false, false, false]
        );
        assert_eq!(
            Pad::new([true, true, true].iter().cloned()).collect::<Vec<_>>(),
            vec![true, true, true, false, false, false, false, false, false, false]
        );
        assert_eq!(
            Pad::new(
                [true, true, false, true, false, true, false, true, false, true]
                    .iter()
                    .cloned()
            )
            .collect::<Vec<_>>(),
            vec![true, true, false, true, false, true, false, true, false, true]
        );
        assert_eq!(
            Pad::new(
                [true, true, false, true, false, true, false, true, false, true, true]
                    .iter()
                    .cloned()
            )
            .collect::<Vec<_>>(),
            vec![
                true, true, false, true, false, true, false, true, false, true, true, false, false,
                false, false, false, false, false, false, false
            ]
        );
    }

    #[test]
    fn sinsemilla_s() {
        use super::sinsemilla_s::SINSEMILLA_S;
        use group::Curve;
        use pasta_curves::arithmetic::CurveAffine;

        let hasher = pallas::Point::hash_to_curve(super::S_PERSONALIZATION);

        for j in 0..(1u32 << K) {
            let computed = {
                let point = hasher(&j.to_le_bytes()).to_affine().coordinates().unwrap();
                (*point.x(), *point.y())
            };
            let actual = SINSEMILLA_S[j as usize];
            assert_eq!(computed, actual);
        }
    }
}
