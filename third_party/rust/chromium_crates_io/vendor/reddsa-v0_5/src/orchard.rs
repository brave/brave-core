//! Signature types for the Orchard protocol.

#[cfg(feature = "alloc")]
use alloc::vec::Vec;
#[cfg(feature = "alloc")]
use core::borrow::Borrow;

use group::GroupEncoding;
#[cfg(feature = "alloc")]
use group::{ff::PrimeField, Group};
use pasta_curves::pallas;

use crate::{private, SigType};

#[cfg(feature = "alloc")]
use crate::scalar_mul::{LookupTable5, NonAdjacentForm, VartimeMultiscalarMul};

#[cfg(test)]
mod tests;

/// The byte-encoding of the basepoint for the Orchard `SpendAuthSig` on the [Pallas curve][pallasandvesta].
///
/// [pallasandvesta]: https://zips.z.cash/protocol/nu5.pdf#pallasandvesta
// Reproducible by pallas::Point::hash_to_curve("z.cash:Orchard")(b"G").to_bytes()
const ORCHARD_SPENDAUTHSIG_BASEPOINT_BYTES: [u8; 32] = [
    99, 201, 117, 184, 132, 114, 26, 141, 12, 161, 112, 123, 227, 12, 127, 12, 95, 68, 95, 62, 124,
    24, 141, 59, 6, 214, 241, 40, 179, 35, 85, 183,
];

/// The byte-encoding of the basepoint for the Orchard `BindingSig` on the Pallas curve.
// Reproducible by pallas::Point::hash_to_curve("z.cash:Orchard-cv")(b"r").to_bytes()
const ORCHARD_BINDINGSIG_BASEPOINT_BYTES: [u8; 32] = [
    145, 90, 60, 136, 104, 198, 195, 14, 47, 128, 144, 238, 69, 215, 110, 64, 72, 32, 141, 234, 91,
    35, 102, 79, 187, 9, 164, 15, 85, 68, 244, 7,
];

/// A type variable corresponding to Zcash's `OrchardSpendAuthSig`.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum SpendAuth {}
// This should not exist, but is necessary to use zeroize::DefaultIsZeroes.
impl Default for SpendAuth {
    fn default() -> Self {
        unimplemented!()
    }
}
impl SigType for SpendAuth {}
impl super::SpendAuth for SpendAuth {}

/// A type variable corresponding to Zcash's `OrchardBindingSig`.
#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum Binding {}
// This should not exist, but is necessary to use zeroize::DefaultIsZeroes.
impl Default for Binding {
    fn default() -> Self {
        unimplemented!()
    }
}
impl SigType for Binding {}
impl super::Binding for Binding {}

impl private::SealedScalar for pallas::Scalar {
    fn from_bytes_wide(bytes: &[u8; 64]) -> Self {
        <pallas::Scalar as group::ff::FromUniformBytes<64>>::from_uniform_bytes(bytes)
    }
    fn from_raw(val: [u64; 4]) -> Self {
        pallas::Scalar::from_raw(val)
    }
}
impl private::Sealed<SpendAuth> for SpendAuth {
    const H_STAR_PERSONALIZATION: &'static [u8; 16] = b"Zcash_RedPallasH";
    type Point = pallas::Point;
    type Scalar = pallas::Scalar;

    fn basepoint() -> pallas::Point {
        pallas::Point::from_bytes(&ORCHARD_SPENDAUTHSIG_BASEPOINT_BYTES).unwrap()
    }
}
impl private::Sealed<Binding> for Binding {
    const H_STAR_PERSONALIZATION: &'static [u8; 16] = b"Zcash_RedPallasH";
    type Point = pallas::Point;
    type Scalar = pallas::Scalar;

    fn basepoint() -> pallas::Point {
        pallas::Point::from_bytes(&ORCHARD_BINDINGSIG_BASEPOINT_BYTES).unwrap()
    }
}

#[cfg(feature = "alloc")]
impl NonAdjacentForm for pallas::Scalar {
    fn inner_to_bytes(&self) -> [u8; 32] {
        self.to_repr()
    }

    /// The NAF length for Pallas is 255 since Pallas' order is about 2<sup>254</sup> +
    /// 2<sup>125.1</sup>.
    fn naf_length() -> usize {
        255
    }
}

#[cfg(feature = "alloc")]
impl<'a> From<&'a pallas::Point> for LookupTable5<pallas::Point> {
    #[allow(non_snake_case)]
    fn from(A: &'a pallas::Point) -> Self {
        let mut Ai = [*A; 8];
        let A2 = A.double();
        for i in 0..7 {
            Ai[i + 1] = A2 + Ai[i];
        }
        // Now Ai = [A, 3A, 5A, 7A, 9A, 11A, 13A, 15A]
        LookupTable5(Ai)
    }
}

#[cfg(feature = "alloc")]
impl VartimeMultiscalarMul for pallas::Point {
    type Scalar = pallas::Scalar;
    type Point = pallas::Point;

    #[allow(non_snake_case)]
    fn optional_multiscalar_mul<I, J>(scalars: I, points: J) -> Option<pallas::Point>
    where
        I: IntoIterator,
        I::Item: Borrow<Self::Scalar>,
        J: IntoIterator<Item = Option<pallas::Point>>,
    {
        let nafs: Vec<_> = scalars
            .into_iter()
            .map(|c| c.borrow().non_adjacent_form(5))
            .collect();

        let lookup_tables = points
            .into_iter()
            .map(|P_opt| P_opt.map(|P| LookupTable5::<pallas::Point>::from(&P)))
            .collect::<Option<Vec<_>>>()?;

        let mut r = pallas::Point::identity();
        let naf_size = Self::Scalar::naf_length();

        for i in (0..naf_size).rev() {
            let mut t = r.double();

            for (naf, lookup_table) in nafs.iter().zip(lookup_tables.iter()) {
                #[allow(clippy::comparison_chain)]
                if naf[i] > 0 {
                    t += lookup_table.select(naf[i] as usize);
                } else if naf[i] < 0 {
                    t -= lookup_table.select(-naf[i] as usize);
                }
            }

            r = t;
        }

        Some(r)
    }
}
