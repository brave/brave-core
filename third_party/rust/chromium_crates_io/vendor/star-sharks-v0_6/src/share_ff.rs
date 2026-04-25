use alloc::vec::*;
use core::convert::TryFrom;
use core::convert::TryInto;

#[cfg(feature = "fuzzing")]
use arbitrary::{Arbitrary, Unstructured};

#[cfg(feature = "zeroize_memory")]
use zeroize::Zeroize;

use crate::ff::*;

pub const FIELD_ELEMENT_LEN: usize = 24;

#[cfg_attr(feature = "fuzzing", derive(Arbitrary))]
#[cfg_attr(feature = "zeroize_memory", derive(Zeroize))]
#[derive(PrimeField)]
// 2^128 + 12451 (https://eprint.iacr.org/2011/326)
#[PrimeFieldModulus = "340282366920938463463374607431768223907"]
#[PrimeFieldGenerator = "3"]
#[PrimeFieldReprEndianness = "little"]
pub struct Fp([u64; 3]);

impl From<Fp> for Vec<u8> {
  fn from(s: Fp) -> Vec<u8> {
    s.to_repr().as_ref().to_vec()
  }
}

impl From<Fp> for Vec<u64> {
  fn from(s: Fp) -> Vec<u64> {
    s.0.to_vec()
  }
}

// Finds the [root of the Lagrange polynomial](https://en.wikipedia.org/wiki/Shamir%27s_Secret_Sharing#Computationally_efficient_approach).
// The expected `shares` argument format is the same as the output by the `get_evaluator´ function.
// Where each (key, value) pair corresponds to one share, where the key is the `x` and the value is a vector of `y`,
// where each element corresponds to one of the secret's byte chunks.
pub fn interpolate(shares: &[Share]) -> Result<Vec<u8>, &'static str> {
  if shares.is_empty() {
    return Err("Need at least one share to interpolate");
  }
  let res: Vec<Vec<u8>> = (0..shares[0].y.len())
    .map(|s| {
      let e: Fp = shares
        .iter()
        .map(|s_i| {
          let f: Fp = shares
            .iter()
            .filter(|s_j| s_j.x != s_i.x)
            .map(|s_j| s_j.x * (s_j.x - s_i.x).invert().unwrap())
            .fold(Fp::ONE, |acc, x| acc * x); // take product of all fractions
          f * s_i.y[s]
        })
        .fold(Fp::ZERO, |acc, x| acc + x); // take sum of all field elements
      Vec::from(e) // turn into byte vector
    })
    .collect();
  Ok(
    res
      .iter()
      .fold(Vec::new(), |acc, r| [acc, r.to_vec()].concat()),
  )
}

// Generates `k` polynomial coefficients, being the last one `s` and the
// others randomly generated in the field.
// Coefficient degrees go from higher to lower in the returned vector
// order.
pub fn random_polynomial<R: rand::Rng>(s: Fp, k: u32, rng: &mut R) -> Vec<Fp> {
  let k = k as usize;
  let mut poly = Vec::with_capacity(k);
  for _ in 1..k {
    poly.push(Fp::random(&mut *rng));
  }
  poly.push(s);

  poly
}

// Returns an iterator over the points of the `polys` polynomials passed as argument.
// Each item of the iterator is a tuple `(x, [f_1(x), f_2(x)..])` where eaxh `f_i` is the result for the ith polynomial.
// Each polynomial corresponds to one byte chunk of the original secret.
pub fn get_evaluator(polys: Vec<Vec<Fp>>) -> Evaluator {
  Evaluator { polys, x: Fp::ZERO }
}

#[derive(Debug)]
pub struct Evaluator {
  polys: Vec<Vec<Fp>>,
  x: Fp,
}

impl Evaluator {
  fn evaluate(&self, x: Fp) -> Share {
    Share {
      x,
      y: self
        .polys
        .iter()
        .map(|p| p.iter().fold(Fp::ZERO, |acc, c| acc * x + c))
        .collect(),
    }
  }

  pub fn gen<R: rand::Rng>(&self, rng: &mut R) -> Share {
    let rand = Fp::random(rng);
    self.evaluate(rand)
  }
}

// Implement `Iterator` for `Evaluator`.
// The `Iterator` trait only requires a method to be defined for the `next` element.
impl Iterator for Evaluator {
  type Item = Share;

  fn next(&mut self) -> Option<Share> {
    self.x += Fp::ONE;
    Some(self.evaluate(self.x))
  }
}

/// A share used to reconstruct the secret. Can be serialized to and from a byte array.
#[derive(Debug, Clone, Eq, PartialEq)]
#[cfg_attr(feature = "zeroize_memory", derive(Zeroize))]
pub struct Share {
  pub x: Fp,
  pub y: Vec<Fp>,
}

/// Obtains a byte vector from a `Share` instance
impl From<&Share> for Vec<u8> {
  fn from(s: &Share) -> Vec<u8> {
    let mut bytes = Vec::with_capacity((s.y.len() + 1) * FIELD_ELEMENT_LEN);
    let repr = s.x.to_repr();
    let x_coord = repr.as_ref().to_vec();
    let y_coords = s
      .y
      .iter()
      .map(|p| p.to_repr().as_ref().to_vec())
      .fold(Vec::new(), |acc, r| [acc, r.to_vec()].concat());
    bytes.extend(x_coord);
    bytes.extend(y_coords);
    bytes
  }
}

/// Obtains a `Share` instance from a byte slice
impl TryFrom<&[u8]> for Share {
  type Error = &'static str;

  fn try_from(s: &[u8]) -> Result<Share, Self::Error> {
    if s.len() < FIELD_ELEMENT_LEN {
      return Err(
        "A Share must have enough bytes to represent a field element",
      );
    }
    let xr = FpRepr(
      s[..FIELD_ELEMENT_LEN]
        .try_into()
        // Slice into array only fails if the lengths don't match.
        // The length we pass is fixed, so this will not panic based
        // on the input data from the caller and unwrap is safe.
        .expect("byte slice should be the right size for an x coordinate"),
    );
    let x = Option::from(Fp::from_repr(xr))
      .ok_or("Failed to create field element from x representation")?;

    let y_bytes = &s[FIELD_ELEMENT_LEN..];
    let y_count = y_bytes.len() / FIELD_ELEMENT_LEN;
    let mut y = Vec::with_capacity(y_count);
    for i in 0..y_count {
      let fr = FpRepr(
        y_bytes[i * FIELD_ELEMENT_LEN..(i + 1) * FIELD_ELEMENT_LEN]
          .try_into()
          .expect("byte slice should be the right size for a y coordinate"),
      );
      let f = Option::from(Fp::from_repr(fr))
        .ok_or("Failed to create field element from y representation")?;
      y.push(f);
    }
    Ok(Share { x, y })
  }
}

/// Generate a Share from arbitrary input data.
///
/// The derived `Arbitary` trait impl just splats data directly
/// into the `Fp` struct without checking that it's a valid field
/// element, which violates the invariants of the type and leads
/// to false panic reports from the fuzzer.
///
/// Implement the trait directly to ensure invalid values are
/// not passed on to further code.
#[cfg(feature = "fuzzing")]
impl<'a> Arbitrary<'a> for Share {
  fn arbitrary(u: &mut Unstructured<'a>) -> arbitrary::Result<Self> {
    let count = u.arbitrary_len::<Fp>()?;
    Share::try_from(u.bytes(count * FIELD_ELEMENT_LEN)?)
      .map_err(|_| arbitrary::Error::IncorrectFormat)
  }
}

#[cfg(test)]
mod tests {
  use super::{get_evaluator, interpolate};
  use super::{Fp, Share, FIELD_ELEMENT_LEN};
  use crate::ff::Field;
  use alloc::{vec, vec::Vec};
  use core::convert::TryFrom;
  use rand_chacha::rand_core::SeedableRng;

  fn fp_one() -> Fp {
    Fp::ONE
  }

  fn fp_two() -> Fp {
    fp_one().double()
  }

  fn fp_three() -> Fp {
    fp_two() + fp_one()
  }

  #[test]
  fn field_addition() {
    let x = fp_one();
    let y = fp_two();
    let z = fp_three();
    assert_eq!(x + y, z);
  }

  #[test]
  fn field_mult() {
    let x = fp_three();
    let y = fp_one();
    let z = fp_three();
    assert_eq!(Vec::from(x * y) as Vec<u8>, Vec::from(z) as Vec<u8>);
  }

  #[test]
  fn random_polynomial() {
    let mut rng = rand_chacha::ChaCha8Rng::from_seed([0x90; 32]);
    let poly = super::random_polynomial(fp_one(), 3, &mut rng);
    assert_eq!(poly.len(), 3);
    assert_eq!(poly[2], fp_one());
  }

  #[test]
  fn evaluation() {
    let iter =
      get_evaluator(vec![vec![fp_three(), fp_two(), fp_three() + fp_two()]]);
    let values: Vec<(Fp, Vec<Fp>)> = iter.take(2).map(|s| (s.x, s.y)).collect();
    assert_eq!(
      values,
      vec![
        (fp_one(), vec![Fp([12451u64, 18446744073709427106, 0])]),
        (fp_two(), vec![Fp([12451u64, 18446744073709290145, 0])])
      ]
    );
  }

  #[test]
  fn interpolation() {
    let mut rng = rand_chacha::ChaCha8Rng::from_seed([0x90; 32]);
    let poly = super::random_polynomial(fp_one(), 5, &mut rng);
    let iter = get_evaluator(vec![poly]);
    let shares: Vec<Share> = iter.take(5).collect();
    let root = interpolate(&shares).unwrap();
    let mut chk = vec![0u8; FIELD_ELEMENT_LEN];
    chk[0] = 1u8;
    assert_eq!(root, chk);
  }

  #[test]
  fn vec_from_share() {
    let share = Share {
      x: fp_one(),
      y: vec![fp_two(), fp_three()],
    };
    let bytes = Vec::from(&share);
    let chk_bytes = test_bytes();
    assert_eq!(bytes, chk_bytes);
  }

  #[test]
  fn share_from_u8_slice() {
    let share = Share::try_from(&test_bytes()[..]).unwrap();
    assert_eq!(share.x, fp_one());
    assert_eq!(share.y, vec![fp_two(), fp_three()]);
  }

  #[test]
  fn share_from_u8_slice_without_y() {
    let share = Share::try_from(&test_bytes()[..FIELD_ELEMENT_LEN]).unwrap();
    assert_eq!(share.x, fp_one());
    assert_eq!(share.y, vec![]);
  }

  #[test]
  fn share_from_u8_slice_partial_y() {
    let share =
      Share::try_from(&test_bytes()[..FIELD_ELEMENT_LEN + 20]).unwrap();
    assert_eq!(share.x, fp_one());
    assert_eq!(share.y, vec![]);
    let share =
      Share::try_from(&test_bytes()[..FIELD_ELEMENT_LEN * 2 + 12]).unwrap();
    assert_eq!(share.x, fp_one());
    assert_eq!(share.y, vec![fp_two()]);
  }

  #[test]
  fn share_from_short_u8_slice() {
    let bytes = test_bytes();
    assert!(Share::try_from(&bytes[0..FIELD_ELEMENT_LEN - 1]).is_err());
    assert!(Share::try_from(&bytes[0..1]).is_err());
  }

  fn test_bytes() -> Vec<u8> {
    let suffix = vec![0u8; FIELD_ELEMENT_LEN - 1];
    let mut bytes = vec![1u8; 1];
    bytes.extend(suffix.clone()); // x coord
    bytes.extend(vec![2u8; 1]);
    bytes.extend(suffix.clone()); // y coord #1
    bytes.extend(vec![3u8; 1]);
    bytes.extend(suffix); // y coord #2
    bytes
  }

  #[test]
  fn bad_share_bytes() {
    let bytes: Vec<u8> = vec![
      10u8, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 39, 10, 0, 0, 0, 0, 0, 0,
      10, 0, 0, 0, 0, 0,
    ];
    let _ = Share::try_from(bytes.as_slice());
  }

  #[test]
  fn element_length() {
    assert_eq!(FIELD_ELEMENT_LEN, core::mem::size_of::<Fp>());
  }
}
