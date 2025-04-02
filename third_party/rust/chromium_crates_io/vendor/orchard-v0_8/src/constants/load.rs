use crate::constants::{self, compute_lagrange_coeffs, H, NUM_WINDOWS, NUM_WINDOWS_SHORT};
use group::ff::PrimeField;
use pasta_curves::pallas;

#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum OrchardFixedBasesFull {
    CommitIvkR,
    NoteCommitR,
    ValueCommitR,
    SpendAuthG,
}

impl OrchardFixedBasesFull {
    pub fn generator(&self) -> pallas::Affine {
        match self {
            OrchardFixedBasesFull::CommitIvkR => super::commit_ivk_r::generator(),
            OrchardFixedBasesFull::NoteCommitR => super::note_commit_r::generator(),
            OrchardFixedBasesFull::ValueCommitR => super::value_commit_r::generator(),
            OrchardFixedBasesFull::SpendAuthG => super::spend_auth_g::generator(),
        }
    }

    pub fn u(&self) -> U {
        match self {
            OrchardFixedBasesFull::CommitIvkR => super::commit_ivk_r::U.into(),
            OrchardFixedBasesFull::NoteCommitR => super::note_commit_r::U.into(),
            OrchardFixedBasesFull::ValueCommitR => super::value_commit_r::U.into(),
            OrchardFixedBasesFull::SpendAuthG => super::spend_auth_g::U.into(),
        }
    }
}

/// A fixed base to be used in scalar multiplication with a full-width scalar.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct OrchardFixedBase {
    pub generator: pallas::Affine,
    pub lagrange_coeffs: LagrangeCoeffs,
    pub z: Z,
    pub u: U,
}

impl From<OrchardFixedBasesFull> for OrchardFixedBase {
    fn from(base: OrchardFixedBasesFull) -> Self {
        let (generator, z, u) = match base {
            OrchardFixedBasesFull::CommitIvkR => (
                super::commit_ivk_r::generator(),
                super::commit_ivk_r::Z.into(),
                super::commit_ivk_r::U.into(),
            ),
            OrchardFixedBasesFull::NoteCommitR => (
                super::note_commit_r::generator(),
                super::note_commit_r::Z.into(),
                super::note_commit_r::U.into(),
            ),
            OrchardFixedBasesFull::ValueCommitR => (
                super::value_commit_r::generator(),
                super::value_commit_r::Z.into(),
                super::value_commit_r::U.into(),
            ),
            OrchardFixedBasesFull::SpendAuthG => (
                super::spend_auth_g::generator(),
                super::spend_auth_g::Z.into(),
                super::spend_auth_g::U.into(),
            ),
        };

        Self {
            generator,
            lagrange_coeffs: compute_lagrange_coeffs(generator, NUM_WINDOWS).into(),
            z,
            u,
        }
    }
}

/// A fixed base to be used in scalar multiplication with a base field element.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct ValueCommitV {
    pub generator: pallas::Affine,
    pub lagrange_coeffs_short: LagrangeCoeffsShort,
    pub z_short: ZShort,
    pub u_short: UShort,
}

impl ValueCommitV {
    pub fn get() -> Self {
        let generator = super::value_commit_v::generator();
        Self {
            generator,
            lagrange_coeffs_short: compute_lagrange_coeffs(generator, NUM_WINDOWS_SHORT).into(),
            z_short: super::value_commit_v::Z_SHORT.into(),
            u_short: super::value_commit_v::U_SHORT.into(),
        }
    }
}

/// A fixed base to be used in scalar multiplication with a short signed exponent.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct NullifierK;

impl From<NullifierK> for OrchardFixedBase {
    fn from(_nullifier_k: NullifierK) -> Self {
        let (generator, z, u) = (
            super::nullifier_k::generator(),
            super::nullifier_k::Z.into(),
            super::nullifier_k::U.into(),
        );
        Self {
            generator,
            lagrange_coeffs: compute_lagrange_coeffs(generator, NUM_WINDOWS).into(),
            z,
            u,
        }
    }
}

impl NullifierK {
    pub fn generator(&self) -> pallas::Affine {
        super::nullifier_k::generator()
    }

    pub fn u(&self) -> U {
        super::nullifier_k::U.into()
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
// 8 coefficients per window
pub struct WindowLagrangeCoeffs(pub Box<[pallas::Base; H]>);

impl From<&[pallas::Base; H]> for WindowLagrangeCoeffs {
    fn from(array: &[pallas::Base; H]) -> Self {
        Self(Box::new(*array))
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
// 85 windows per base (with the exception of ValueCommitV)
pub struct LagrangeCoeffs(pub Box<[WindowLagrangeCoeffs; constants::NUM_WINDOWS]>);

impl From<Vec<WindowLagrangeCoeffs>> for LagrangeCoeffs {
    fn from(windows: Vec<WindowLagrangeCoeffs>) -> Self {
        Self(windows.into_boxed_slice().try_into().unwrap())
    }
}

impl From<Vec<[pallas::Base; H]>> for LagrangeCoeffs {
    fn from(arrays: Vec<[pallas::Base; H]>) -> Self {
        let windows: Vec<WindowLagrangeCoeffs> = arrays.iter().map(|array| array.into()).collect();
        windows.into()
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
// 22 windows for ValueCommitV
pub struct LagrangeCoeffsShort(pub Box<[WindowLagrangeCoeffs; NUM_WINDOWS_SHORT]>);

impl From<Vec<WindowLagrangeCoeffs>> for LagrangeCoeffsShort {
    fn from(windows: Vec<WindowLagrangeCoeffs>) -> Self {
        Self(windows.into_boxed_slice().try_into().unwrap())
    }
}

impl From<Vec<[pallas::Base; H]>> for LagrangeCoeffsShort {
    fn from(arrays: Vec<[pallas::Base; H]>) -> Self {
        let windows: Vec<WindowLagrangeCoeffs> = arrays.iter().map(|array| array.into()).collect();
        windows.into()
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
// 85 Z's per base (with the exception of ValueCommitV)
pub struct Z(pub Box<[pallas::Base; NUM_WINDOWS]>);

impl From<[u64; NUM_WINDOWS]> for Z {
    fn from(zs: [u64; NUM_WINDOWS]) -> Self {
        Self(
            zs.iter()
                .map(|z| pallas::Base::from(*z))
                .collect::<Vec<_>>()
                .into_boxed_slice()
                .try_into()
                .unwrap(),
        )
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
// 22 Z's for ValueCommitV
pub struct ZShort(pub Box<[pallas::Base; NUM_WINDOWS_SHORT]>);

impl From<[u64; NUM_WINDOWS_SHORT]> for ZShort {
    fn from(zs: [u64; NUM_WINDOWS_SHORT]) -> Self {
        Self(
            zs.iter()
                .map(|z| pallas::Base::from(*z))
                .collect::<Vec<_>>()
                .into_boxed_slice()
                .try_into()
                .unwrap(),
        )
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
// 8 u's per window
pub struct WindowUs(pub Box<[pallas::Base; H]>);

impl From<&[[u8; 32]; H]> for WindowUs {
    fn from(window_us: &[[u8; 32]; H]) -> Self {
        Self(
            window_us
                .iter()
                .map(|u| pallas::Base::from_repr(*u).unwrap())
                .collect::<Vec<_>>()
                .into_boxed_slice()
                .try_into()
                .unwrap(),
        )
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
// 85 windows per base (with the exception of ValueCommitV)
pub struct U(pub Box<[WindowUs; NUM_WINDOWS]>);

impl From<Vec<WindowUs>> for U {
    fn from(windows: Vec<WindowUs>) -> Self {
        Self(windows.into_boxed_slice().try_into().unwrap())
    }
}

impl From<[[[u8; 32]; H]; NUM_WINDOWS]> for U {
    fn from(window_us: [[[u8; 32]; H]; NUM_WINDOWS]) -> Self {
        let windows: Vec<WindowUs> = window_us.iter().map(|us| us.into()).collect();
        windows.into()
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
// 22 windows for ValueCommitV
pub struct UShort(pub Box<[WindowUs; NUM_WINDOWS_SHORT]>);

impl From<Vec<WindowUs>> for UShort {
    fn from(windows: Vec<WindowUs>) -> Self {
        Self(windows.into_boxed_slice().try_into().unwrap())
    }
}

impl From<[[[u8; 32]; H]; NUM_WINDOWS_SHORT]> for UShort {
    fn from(window_us: [[[u8; 32]; H]; NUM_WINDOWS_SHORT]) -> Self {
        let windows: Vec<WindowUs> = window_us.iter().map(|us| us.into()).collect();
        windows.into()
    }
}
