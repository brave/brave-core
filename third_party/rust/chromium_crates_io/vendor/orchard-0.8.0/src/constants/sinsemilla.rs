//! Sinsemilla generators
use super::{OrchardFixedBases, OrchardFixedBasesFull};
use crate::spec::i2lebsp;
use halo2_gadgets::sinsemilla::{CommitDomains, HashDomains};

use group::ff::PrimeField;
use pasta_curves::{arithmetic::CurveAffine, pallas};

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

/// $\ell^\mathsf{Orchard}_\mathsf{Merkle}$
pub(crate) const L_ORCHARD_MERKLE: usize = 255;

/// SWU hash-to-curve personalization for the Merkle CRH generator
pub const MERKLE_CRH_PERSONALIZATION: &str = "z.cash:Orchard-MerkleCRH";

/// Generator used in SinsemillaHashToPoint for note commitment
pub const Q_NOTE_COMMITMENT_M_GENERATOR: ([u8; 32], [u8; 32]) = (
    [
        93, 116, 168, 64, 9, 186, 14, 50, 42, 221, 70, 253, 90, 15, 150, 197, 93, 237, 176, 121,
        180, 242, 159, 247, 13, 205, 251, 86, 160, 7, 128, 23,
    ],
    [
        99, 172, 73, 115, 90, 10, 39, 135, 158, 94, 219, 129, 136, 18, 34, 136, 44, 201, 244, 110,
        217, 194, 190, 78, 131, 112, 198, 138, 147, 88, 160, 50,
    ],
);

/// Generator used in SinsemillaHashToPoint for IVK commitment
pub const Q_COMMIT_IVK_M_GENERATOR: ([u8; 32], [u8; 32]) = (
    [
        242, 130, 15, 121, 146, 47, 203, 107, 50, 162, 40, 81, 36, 204, 27, 66, 250, 65, 162, 90,
        184, 129, 204, 125, 17, 200, 169, 74, 241, 12, 188, 5,
    ],
    [
        190, 222, 173, 207, 206, 229, 90, 190, 241, 165, 109, 201, 29, 53, 196, 70, 75, 5, 222, 32,
        70, 7, 89, 239, 230, 190, 26, 212, 246, 76, 1, 27,
    ],
);

/// Generator used in SinsemillaHashToPoint for Merkle collision-resistant hash
pub const Q_MERKLE_CRH: ([u8; 32], [u8; 32]) = (
    [
        160, 198, 41, 127, 249, 199, 185, 248, 112, 16, 141, 192, 85, 185, 190, 201, 153, 14, 137,
        239, 90, 54, 15, 160, 185, 24, 168, 99, 150, 210, 22, 22,
    ],
    [
        98, 234, 242, 37, 206, 174, 233, 134, 150, 21, 116, 5, 234, 150, 28, 226, 121, 89, 163, 79,
        62, 242, 196, 45, 153, 32, 175, 227, 163, 66, 134, 53,
    ],
);

pub(crate) fn lebs2ip_k(bits: &[bool]) -> u32 {
    assert!(bits.len() == K);
    bits.iter()
        .enumerate()
        .fold(0u32, |acc, (i, b)| acc + if *b { 1 << i } else { 0 })
}

/// The sequence of K bits in little-endian order representing an integer
/// up to `2^K` - 1.
pub(crate) fn i2lebsp_k(int: usize) -> [bool; K] {
    assert!(int < (1 << K));
    i2lebsp(int as u64)
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum OrchardHashDomains {
    NoteCommit,
    CommitIvk,
    MerkleCrh,
}

#[allow(non_snake_case)]
impl HashDomains<pallas::Affine> for OrchardHashDomains {
    fn Q(&self) -> pallas::Affine {
        match self {
            OrchardHashDomains::CommitIvk => pallas::Affine::from_xy(
                pallas::Base::from_repr(Q_COMMIT_IVK_M_GENERATOR.0).unwrap(),
                pallas::Base::from_repr(Q_COMMIT_IVK_M_GENERATOR.1).unwrap(),
            )
            .unwrap(),
            OrchardHashDomains::NoteCommit => pallas::Affine::from_xy(
                pallas::Base::from_repr(Q_NOTE_COMMITMENT_M_GENERATOR.0).unwrap(),
                pallas::Base::from_repr(Q_NOTE_COMMITMENT_M_GENERATOR.1).unwrap(),
            )
            .unwrap(),
            OrchardHashDomains::MerkleCrh => pallas::Affine::from_xy(
                pallas::Base::from_repr(Q_MERKLE_CRH.0).unwrap(),
                pallas::Base::from_repr(Q_MERKLE_CRH.1).unwrap(),
            )
            .unwrap(),
        }
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum OrchardCommitDomains {
    NoteCommit,
    CommitIvk,
}

impl CommitDomains<pallas::Affine, OrchardFixedBases, OrchardHashDomains> for OrchardCommitDomains {
    fn r(&self) -> OrchardFixedBasesFull {
        match self {
            Self::NoteCommit => OrchardFixedBasesFull::NoteCommitR,
            Self::CommitIvk => OrchardFixedBasesFull::CommitIvkR,
        }
    }

    fn hash_domain(&self) -> OrchardHashDomains {
        match self {
            Self::NoteCommit => OrchardHashDomains::NoteCommit,
            Self::CommitIvk => OrchardHashDomains::CommitIvk,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::constants::{
        fixed_bases::{COMMIT_IVK_PERSONALIZATION, NOTE_COMMITMENT_PERSONALIZATION},
        sinsemilla::MERKLE_CRH_PERSONALIZATION,
    };
    use group::{ff::PrimeField, Curve};
    use halo2_gadgets::sinsemilla::primitives::{CommitDomain, HashDomain};
    use halo2_proofs::arithmetic::CurveAffine;
    use halo2_proofs::pasta::pallas;
    use rand::{self, rngs::OsRng, Rng};

    #[test]
    // Nodes in the Merkle tree are Pallas base field elements.
    fn l_orchard_merkle() {
        assert_eq!(super::L_ORCHARD_MERKLE, pallas::Base::NUM_BITS as usize);
    }

    #[test]
    fn lebs2ip_k_round_trip() {
        let mut rng = OsRng;
        {
            let int = rng.gen_range(0..(1 << K));
            assert_eq!(lebs2ip_k(&i2lebsp_k(int)) as usize, int);
        }

        assert_eq!(lebs2ip_k(&i2lebsp_k(0)) as usize, 0);
        assert_eq!(lebs2ip_k(&i2lebsp_k((1 << K) - 1)) as usize, (1 << K) - 1);
    }

    #[test]
    fn i2lebsp_k_round_trip() {
        {
            let bitstring = [0; K].map(|_| rand::random());
            assert_eq!(i2lebsp_k(lebs2ip_k(&bitstring) as usize), bitstring);
        }

        {
            let bitstring = [false; K];
            assert_eq!(i2lebsp_k(lebs2ip_k(&bitstring) as usize), bitstring);
        }

        {
            let bitstring = [true; K];
            assert_eq!(i2lebsp_k(lebs2ip_k(&bitstring) as usize), bitstring);
        }
    }

    #[test]
    fn q_note_commitment_m() {
        let domain = CommitDomain::new(NOTE_COMMITMENT_PERSONALIZATION);
        let point = domain.Q();
        let coords = point.to_affine().coordinates().unwrap();

        assert_eq!(
            *coords.x(),
            pallas::Base::from_repr(Q_NOTE_COMMITMENT_M_GENERATOR.0).unwrap()
        );
        assert_eq!(
            *coords.y(),
            pallas::Base::from_repr(Q_NOTE_COMMITMENT_M_GENERATOR.1).unwrap()
        );
    }

    #[test]
    fn q_commit_ivk_m() {
        let domain = CommitDomain::new(COMMIT_IVK_PERSONALIZATION);
        let point = domain.Q();
        let coords = point.to_affine().coordinates().unwrap();

        assert_eq!(
            *coords.x(),
            pallas::Base::from_repr(Q_COMMIT_IVK_M_GENERATOR.0).unwrap()
        );
        assert_eq!(
            *coords.y(),
            pallas::Base::from_repr(Q_COMMIT_IVK_M_GENERATOR.1).unwrap()
        );
    }

    #[test]
    fn q_merkle_crh() {
        let domain = HashDomain::new(MERKLE_CRH_PERSONALIZATION);
        let point = domain.Q();
        let coords = point.to_affine().coordinates().unwrap();

        assert_eq!(
            *coords.x(),
            pallas::Base::from_repr(Q_MERKLE_CRH.0).unwrap()
        );
        assert_eq!(
            *coords.y(),
            pallas::Base::from_repr(Q_MERKLE_CRH.1).unwrap()
        );
    }

    #[test]
    fn inv_two_pow_k() {
        let two_pow_k = pallas::Base::from(1u64 << K);
        let inv_two_pow_k = pallas::Base::from_repr(INV_TWO_POW_K).unwrap();

        assert_eq!(two_pow_k * inv_two_pow_k, pallas::Base::one());
    }
}
