//! The `adss` crate defines functionality for performing secret
//! sharing with established security guarantees. We use this framework
//! as it allows for specifying the random coins that are used for
//! establishing the lagrange polynomial coefficients explicitly. A
//! description of the framework is provided in the paper by [Bellare et
//! al.](https://eprint.iacr.org/2020/800).

use star_sharks::Sharks;
use std::convert::{TryFrom, TryInto};
use std::error::Error;
use std::fmt;
use strobe_rs::{SecParam, Strobe};
use zeroize::{Zeroize, ZeroizeOnDrop};

mod strobe_rng;
use strobe_rng::StrobeRng;

// The length of a `AccessStructure`, in bytes.
pub const ACCESS_STRUCTURE_LENGTH: usize = 4;

// The length of a `Share::J`, in bytes.
pub const MAC_LENGTH: usize = 64;

/// The `AccessStructure` struct defines the policy under which shares
/// can be recovered. Currently, this policy is simply whether there are
/// `threshold` number of independent shares.
#[derive(Debug, Clone, PartialEq, Eq, Zeroize, ZeroizeOnDrop)]
pub struct AccessStructure {
  threshold: u32,
}

pub fn store_u32(u: u32, out: &mut Vec<u8>) {
  out.extend(u.to_le_bytes());
}

pub fn load_u32(bytes: &[u8]) -> Option<u32> {
  if bytes.len() != 4 {
    return None;
  }

  let mut bits: [u8; 4] = [0u8; 4];
  bits.copy_from_slice(bytes);
  Some(u32::from_le_bytes(bits))
}

pub fn store_bytes(s: &[u8], out: &mut Vec<u8>) {
  store_u32(s.len() as u32, out);
  out.extend(s);
}

pub fn load_bytes(bytes: &[u8]) -> Option<&[u8]> {
  if bytes.len() < 4 {
    return None;
  }

  let len: u32 = load_u32(&bytes[..4])?;
  if bytes.len() < (4 + len) as usize {
    return None;
  }

  Some(&bytes[4..4 + len as usize])
}

/// An `AccessStructure` defines how a message is to be split among multiple parties
///
/// In particular this determines how many shares will be issued and what threshold of the shares
/// are needed to reconstruct the original `Commune`
impl AccessStructure {
  /// Convert this `AccessStructure` to a byte array.
  pub fn to_bytes(&self) -> [u8; ACCESS_STRUCTURE_LENGTH] {
    self.threshold.to_le_bytes()
  }

  pub fn from_bytes(bytes: &[u8]) -> Option<AccessStructure> {
    let threshold = load_u32(bytes)?;
    Some(AccessStructure { threshold })
  }
}

#[allow(non_snake_case)]
impl From<AccessStructure> for Sharks {
  fn from(A: AccessStructure) -> Sharks {
    Sharks(A.threshold)
  }
}

/// A `Commune` is a unique instance of sharing across multiple parties
///
/// It consists of an access structure defining the parameters of the sharing, a secret message
/// which will be shared, "random coins" which provide strong but possibly non-uniform entropy
/// and an optional STROBE transcript which can include extra data which will be authenticated.
#[cfg_attr(not(feature = "cbindgen"), repr(C))]
#[allow(non_snake_case)]
#[derive(Clone, ZeroizeOnDrop)]
pub struct Commune {
  /// `A` is an `AccessStructure` defining the sharing
  A: AccessStructure,
  /// `M` is the message to be shared
  M: Vec<u8>,
  /// `R` are the "random coins" which may not be uniform
  R: Vec<u8>,
  /// `T` is a `Strobe` transcript which forms optional tags to be authenticated
  T: Option<Strobe>,
}

/// The `Share` struct holds the necessary data that is encoded in a
/// single secret share. A share itself reveals nothing about the
/// encoded secret data until it is grouped with a set of shares that
/// satisfy the policy in the associated `AccessStructure`.
#[allow(non_snake_case)]
#[derive(Clone, Eq, PartialEq, Zeroize)]
pub struct Share {
  A: AccessStructure,
  S: star_sharks::Share,
  /// C is the encrypted message
  C: Vec<u8>,
  /// D is the encrypted randomness
  D: Vec<u8>,
  /// J is a MAC showing knowledge of A, M, R, and T
  J: [u8; MAC_LENGTH],
  T: (),
}

impl fmt::Debug for Share {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    f.debug_struct("Share").field("S", &self.S).finish()
  }
}

impl Share {
  pub fn to_bytes(&self) -> Vec<u8> {
    let mut out: Vec<u8> = Vec::new();

    // A: AccessStructure
    out.extend(self.A.to_bytes());

    // S: star_sharks::Share
    store_bytes(&Vec::from(&self.S), &mut out);

    // C: Vec<u8>
    store_bytes(&self.C, &mut out);

    // D: Vec<u8>
    store_bytes(&self.D, &mut out);

    // J: [u8; 64]
    out.extend(self.J);

    out
  }

  pub fn from_bytes(bytes: &[u8]) -> Option<Share> {
    let mut slice = bytes;

    // A: AccessStructure
    let a = AccessStructure::from_bytes(&slice[..ACCESS_STRUCTURE_LENGTH])?;
    slice = &slice[ACCESS_STRUCTURE_LENGTH..];

    // S: star_sharks::Share
    let sb = load_bytes(slice)?;
    slice = &slice[4 + sb.len()..];

    // C: Vec<u8>
    let c = load_bytes(slice)?;
    slice = &slice[4 + c.len()..];

    // D: Vec<u8>
    let d = load_bytes(slice)?;
    slice = &slice[4 + d.len()..];

    // J: [u8; 64]
    let j: [u8; MAC_LENGTH] = slice.try_into().ok()?;

    Some(Share {
      A: a,
      S: star_sharks::Share::try_from(sb).ok()?,
      C: c.to_vec(),
      D: d.to_vec(),
      J: j,
      T: (),
    })
  }
}

#[allow(non_snake_case)]
impl Commune {
  pub fn new(
    threshold: u32,
    message: Vec<u8>,
    randomness: Vec<u8>,
    transcript: Option<Strobe>,
  ) -> Self {
    Commune {
      A: AccessStructure { threshold },
      M: message,
      R: randomness,
      T: transcript,
    }
  }

  /// The `share` function samples a single secret share for the
  /// specified `message`.
  pub fn share(self) -> Result<Share, Box<dyn Error>> {
    // H4κ = (A, M, R, T)
    let mut transcript = self
      .T
      .clone()
      .unwrap_or_else(|| Strobe::new(b"adss", SecParam::B128));
    transcript.ad(&self.A.to_bytes(), false);
    transcript.ad(&self.M, false);
    transcript.key(&self.R, false);

    // J is a MAC which authenticates A, M, R, and T
    let mut J = [0u8; 64];
    transcript.send_mac(&mut J, false);

    // K is the derived key used to encrypt the message and our "random coins"
    let mut K = [0u8; 16];
    transcript.prf(&mut K, false);

    // L is the randomness to be fed to secret sharing polynomial generation
    let mut L: StrobeRng = transcript.into();

    let mut key = Strobe::new(b"adss encrypt", SecParam::B128);
    key.key(&K, false);

    // C is the encrypted message
    let mut C: Vec<u8> = vec![0; self.M.len()];
    C.copy_from_slice(&self.M);
    key.send_enc(&mut C, false);

    // D is the encrypted randomness
    let mut D: Vec<u8> = vec![0; self.R.len()];
    D.copy_from_slice(&self.R);
    key.send_enc(&mut D, false);

    // Generate a random share
    let mut K_vec: Vec<u8> = K.to_vec();
    K_vec.extend(vec![0u8; 16]);
    let polys = Sharks::from(self.A.clone()).dealer_rng(&K_vec, &mut L)?;
    let S = polys.gen(&mut rand::rngs::OsRng);
    Ok(Share {
      A: self.A.clone(),
      S,
      C,
      D,
      J,
      T: (),
    })
  }

  pub fn get_message(&self) -> Vec<u8> {
    self.M.clone()
  }

  fn verify(&self, J: &mut [u8]) -> Result<(), Box<dyn Error>> {
    let mut transcript = self
      .clone()
      .T
      .clone()
      .unwrap_or_else(|| Strobe::new(b"adss", SecParam::B128));
    transcript.ad(&self.A.to_bytes(), false);
    transcript.ad(&self.M, false);
    transcript.key(&self.R, false);

    transcript
      .recv_mac(J)
      .map_err(|_| "Mac validation failed".into())
  }
}

#[allow(non_snake_case)]
/// The `recover` function attempts to recover a secret shared value
/// from a set of shares that meet the threshold requirements.
pub fn recover<'a, T>(shares: T) -> Result<Commune, Box<dyn Error>>
where
  T: IntoIterator<Item = &'a Share>,
  T::IntoIter: Iterator<Item = &'a Share>,
{
  let mut shares = shares.into_iter().peekable();
  let s = &(*shares.peek().ok_or("no shares passed")?).clone();
  let shares: Vec<star_sharks::Share> = shares.cloned().map(|s| s.S).collect();
  let key = Sharks::from(s.A.clone()).recover(&shares)?;
  let K = key[..16].to_vec();

  let mut key = Strobe::new(b"adss encrypt", SecParam::B128);
  key.key(&K, false);

  // M is the message
  let mut M: Vec<u8> = vec![0; s.C.len()];
  M.copy_from_slice(&s.C);
  key.recv_enc(&mut M, false);

  // R are the "random coins"
  let mut R: Vec<u8> = vec![0; s.D.len()];
  R.copy_from_slice(&s.D);
  key.recv_enc(&mut R, false);

  let c = Commune {
    A: s.A.clone(),
    M,
    R,
    T: None,
  };

  c.verify(&mut s.J.clone())?;
  Ok(c)
}

#[cfg(test)]
mod tests {
  use core::iter;

  use crate::*;

  #[test]
  fn serialization_u32() {
    for &i in &[0, 10, 100, u32::MAX] {
      let mut out: Vec<u8> = Vec::new();
      store_u32(i, &mut out);
      assert_eq!(load_u32(out.as_slice()), Some(i));
    }
  }

  #[test]
  fn serialization_empty_bytes() {
    let mut out: Vec<u8> = Vec::new();
    store_bytes(Vec::new().as_slice(), &mut out);
    assert_eq!(load_bytes(out.as_slice()), Some(&[] as &[u8]));
  }

  #[test]
  fn serialization_bytes() {
    let mut out: Vec<u8> = Vec::new();
    let bytes: &[u8] = &[0, 1, 10, 100];
    store_bytes(bytes, &mut out);
    assert_eq!(load_bytes(out.as_slice()), Some(bytes));
  }

  #[test]
  fn serialization_access_structure() {
    for i in &[0, 10, 100, u32::MAX] {
      let a = AccessStructure { threshold: *i };
      let s: &[u8] = &a.to_bytes()[..];
      assert_eq!(AccessStructure::from_bytes(s), Some(a));
    }
  }

  #[test]
  fn serialization_share() {
    let c = Commune {
      A: AccessStructure { threshold: 50 },
      M: vec![1, 2, 3, 4],
      R: vec![5, 6, 7, 8],
      T: None,
    };

    for share in iter::repeat_with(|| c.clone().share()).take(150) {
      let share = share.unwrap();
      let s = share.to_bytes();
      assert_eq!(Share::from_bytes(s.as_slice()), Some(share));

      // Test shares that are erroneously truncated
      assert_eq!(Share::from_bytes(&s[..s.len() - 7]), None);
    }
  }

  #[test]
  fn it_works() {
    let c = Commune {
      A: AccessStructure { threshold: 50 },
      M: vec![1, 2, 3, 4],
      R: vec![5, 6, 7, 8],
      T: None,
    };

    let shares: Vec<Share> = iter::repeat_with(|| c.clone().share().unwrap())
      .take(150)
      .collect();

    let recovered = recover(&shares).unwrap();

    assert_eq!(c.M, recovered.M);
  }
}
