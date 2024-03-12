//! This module implements the [Goldwasser-Goldreich-Micali
//! PRF](https://crypto.stanford.edu/pbc/notes/crypto/ggm.html), along
//! with extended functionality that allows puncturing inputs from
//! secret keys.

use std::fmt;

use super::{PPRFError, PPRF};
use crate::strobe_rng::StrobeRng;
use bitvec::prelude::*;
use rand::rngs::OsRng;
use rand::Rng;
use strobe_rs::{SecParam, Strobe};

use zeroize::{Zeroize, ZeroizeOnDrop};

#[derive(Clone, Eq, PartialEq)]
struct Prefix {
  bits: BitVec<usize, bitvec::order::Lsb0>,
}

impl Prefix {
  fn new(bits: BitVec<usize, bitvec::order::Lsb0>) -> Self {
    Prefix { bits }
  }

  fn len(&self) -> usize {
    self.bits.len()
  }
}

impl fmt::Debug for Prefix {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    f.debug_struct("Prefix").field("bits", &self.bits).finish()
  }
}

#[derive(Clone, Zeroize, ZeroizeOnDrop)]
struct GGMPseudorandomGenerator {
  key: [u8; 32],
}

impl GGMPseudorandomGenerator {
  fn setup() -> Self {
    let mut t = Strobe::new(b"ggm key gen (ppoprf)", SecParam::B128);
    t.key(&sample_secret(), false);
    let mut rng: StrobeRng = t.into();
    let mut s_key = [0u8; 32];
    rng.fill(&mut s_key);
    GGMPseudorandomGenerator { key: s_key }
  }

  fn eval(&self, input: &[u8], output: &mut [u8]) {
    let mut t = Strobe::new(b"ggm eval (ppoprf)", SecParam::B128);
    t.key(&self.key, false);
    t.ad(input, false);
    let mut rng: StrobeRng = t.into();
    rng.fill(output);
  }
}

#[derive(Clone, Zeroize, ZeroizeOnDrop)]
struct GGMPuncturableKey {
  prgs: Vec<GGMPseudorandomGenerator>,
  #[zeroize(skip)]
  prefixes: Vec<(Prefix, Vec<u8>)>,
  #[zeroize(skip)]
  punctured: Vec<Prefix>,
}

// TODO: remove copies/clones
impl GGMPuncturableKey {
  fn new() -> Self {
    let secret = sample_secret();
    // Setup PRGs and initial tree
    let prg0 = GGMPseudorandomGenerator::setup();
    let mut out0 = vec![0u8; 32];
    prg0.eval(&secret, &mut out0);
    let prg1 = GGMPseudorandomGenerator::setup();
    let mut out1 = vec![0u8; 32];
    prg1.eval(&secret, &mut out1);
    GGMPuncturableKey {
      prgs: vec![prg0, prg1],
      prefixes: vec![
        (Prefix::new(bits![0].to_bitvec()), out0),
        (Prefix::new(bits![1].to_bitvec()), out1),
      ],
      punctured: vec![],
    }
  }

  fn find_prefix(&self, bv: &BitVec) -> Result<(Prefix, Vec<u8>), PPRFError> {
    let key_prefixes = self.prefixes.clone();
    for prefix in key_prefixes {
      let bits = &prefix.0.bits;
      if bv.starts_with(bits) {
        return Ok(prefix);
      }
    }
    Err(PPRFError::NoPrefixFound)
  }

  fn puncture(
    &mut self,
    pfx: &Prefix,
    to_punc: &Prefix,
    new_prefixes: Vec<(Prefix, Vec<u8>)>,
  ) -> Result<(), PPRFError> {
    if self.punctured.iter().any(|p| p.bits == pfx.bits) {
      return Err(PPRFError::AlreadyPunctured);
    }
    if let Some(index) = self.prefixes.iter().position(|p| p.0.bits == pfx.bits)
    {
      self.prefixes.remove(index);
      if !new_prefixes.is_empty() {
        self.prefixes.extend(new_prefixes);
      }
      self.punctured.push(to_punc.clone());
      return Ok(());
    }
    Err(PPRFError::NoPrefixFound)
  }
}

#[derive(Clone, Zeroize, ZeroizeOnDrop)]
pub struct GGM {
  inp_len: usize,
  key: GGMPuncturableKey,
}

impl GGM {
  fn bit_eval(&self, bits: &BitVec, prg_inp: &[u8], output: &mut [u8]) {
    let mut eval = prg_inp.to_vec();
    for bit in bits {
      let prg: &GGMPseudorandomGenerator = if *bit {
        &self.key.prgs[1]
      } else {
        &self.key.prgs[0]
      };
      prg.eval(&eval.clone(), &mut eval);
    }
    output.copy_from_slice(&eval);
  }

  fn partial_eval(
    &self,
    input_bits: &mut BitVec,
    output: &mut [u8],
  ) -> Result<(), PPRFError> {
    let res = self.key.find_prefix(input_bits);
    if let Ok(pfx) = res {
      let tail = pfx.1;
      let (_, right) = input_bits.split_at(pfx.0.bits.len());
      self.bit_eval(&right.to_bitvec(), &tail, output);
      return Ok(());
    }
    Err(PPRFError::NoPrefixFound)
  }
}

impl PPRF for GGM {
  fn setup() -> Self {
    GGM {
      inp_len: 1,
      key: GGMPuncturableKey::new(),
    }
  }

  fn eval(&self, input: &[u8], output: &mut [u8]) -> Result<(), PPRFError> {
    if input.len() != self.inp_len {
      return Err(PPRFError::BadInputLength {
        actual: input.len(),
        expected: self.inp_len,
      });
    }
    let mut input_bits =
      bvcast_u8_to_usize(&BitVec::<_, Lsb0>::from_slice(input));
    self.partial_eval(&mut input_bits, output)
  }

  // Disable clippy lint false positive on `iter_bv` with Rust 1.70.0.
  #[allow(clippy::redundant_clone)]
  fn puncture(&mut self, input: &[u8]) -> Result<(), PPRFError> {
    if input.len() != self.inp_len {
      return Err(PPRFError::BadInputLength {
        actual: input.len(),
        expected: self.inp_len,
      });
    }
    let bv = bvcast_u8_to_usize(&BitVec::<_, Lsb0>::from_slice(input));
    let pfx = self.key.find_prefix(&bv)?;
    let pfx_len = pfx.0.len();

    // If the prefix is smaller than the current input, then we
    // need to recompute some parts of the tree. Otherwise we
    // just remove the prefix entirely.
    let mut new_pfxs: Vec<(Prefix, Vec<u8>)> = Vec::new();
    if pfx_len != bv.len() {
      let mut iter_bv = bv.clone();
      for i in (0..bv.len()).rev() {
        if let Some((last, rest)) = iter_bv.clone().split_last() {
          let mut cbv = iter_bv.clone();
          cbv.set(i, !*last);
          let mut out = vec![0u8; 32];
          let (_, split) = cbv.split_at(pfx_len);
          self.bit_eval(&split.to_bitvec(), &pfx.1, &mut out);
          new_pfxs.push((Prefix::new(cbv), out));
          if rest.len() == pfx_len {
            // we don't want to recompute any further
            break;
          }
          iter_bv = rest.to_bitvec();
        } else {
          return Err(PPRFError::UnexpectedEndOfBv);
        }
      }
    }

    self.key.puncture(&pfx.0, &Prefix::new(bv), new_pfxs)
  }
}

fn sample_secret() -> Vec<u8> {
  let mut out = vec![0u8; 32];
  OsRng.fill(out.as_mut_slice());
  out
}

fn bvcast_u8_to_usize(
  bv_u8: &BitVec<u8, bitvec::order::Lsb0>,
) -> BitVec<usize, bitvec::order::Lsb0> {
  let mut bv_us = BitVec::with_capacity(bv_u8.len());
  for i in 0..bv_u8.len() {
    bv_us.push(bv_u8[i]);
  }
  bv_us
}

#[cfg(test)]
mod tests {
  use super::*;

  #[test]
  fn eval() -> Result<(), PPRFError> {
    let ggm = GGM::setup();
    let x0 = [8u8];
    let x1 = [7u8];
    let mut out = [0u8; 32];
    ggm.eval(&x0, &mut out)?;
    ggm.eval(&x1, &mut out)?;
    Ok(())
  }

  #[test]
  fn puncture_fail_eval() -> Result<(), PPRFError> {
    let mut ggm = GGM::setup();
    let x0 = [8u8];
    let mut out = [0u8; 32];
    ggm.eval(&x0, &mut out)?;
    ggm.puncture(&x0)?;
    // next step should error out
    assert!(matches!(
      ggm.eval(&x0, &mut out),
      Err(PPRFError::NoPrefixFound)
    ));
    Ok(())
  }

  #[test]
  fn mult_puncture_fail_eval() -> Result<(), PPRFError> {
    let mut ggm = GGM::setup();
    let x0 = [0u8];
    let x1 = [1u8];
    ggm.puncture(&x0)?;
    ggm.puncture(&x1)?;
    // next step should error out
    assert!(matches!(
      ggm.eval(&x0, &mut [0u8; 32]),
      Err(PPRFError::NoPrefixFound)
    ));
    Ok(())
  }

  #[test]
  fn puncture_eval_consistent() -> Result<(), PPRFError> {
    let mut ggm = GGM::setup();
    let inputs = [[2u8], [4u8], [8u8], [16u8], [32u8], [64u8], [128u8]];
    let x0 = [0u8];
    let mut outputs_b4 = vec![vec![0u8; 1]; inputs.len()];
    let mut outputs_after = vec![vec![0u8; 1]; inputs.len()];
    for (i, x) in inputs.iter().enumerate() {
      let mut out = vec![0u8; 32];
      ggm.eval(x, &mut out)?;
      outputs_b4[i] = out;
    }
    ggm.puncture(&x0)?;
    for (i, x) in inputs.iter().enumerate() {
      let mut out = vec![0u8; 32];
      ggm.eval(x, &mut out)?;
      outputs_after[i] = out;
    }
    for (i, o) in outputs_b4.iter().enumerate() {
      assert_eq!(o, &outputs_after[i]);
    }
    Ok(())
  }

  #[test]
  fn multiple_puncture() -> Result<(), PPRFError> {
    let mut ggm = GGM::setup();
    let inputs = [[2u8], [4u8], [8u8], [16u8], [32u8], [64u8], [128u8]];
    let mut outputs_b4 = vec![vec![0u8; 1]; inputs.len()];
    let mut outputs_after = vec![vec![0u8; 1]; inputs.len()];
    for (i, x) in inputs.iter().enumerate() {
      let mut out = vec![0u8; 32];
      ggm.eval(x, &mut out)?;
      outputs_b4[i] = out;
    }
    let x0 = [0u8];
    let x1 = [1u8];
    ggm.puncture(&x0)?;
    for (i, x) in inputs.iter().enumerate() {
      let mut out = vec![0u8; 32];
      ggm.eval(x, &mut out)?;
      outputs_after[i] = out;
    }
    for (i, o) in outputs_b4.iter().enumerate() {
      assert_eq!(o, &outputs_after[i]);
    }
    ggm.puncture(&x1)?;
    for (i, x) in inputs.iter().enumerate() {
      let mut out = vec![0u8; 32];
      ggm.eval(x, &mut out)?;
      outputs_after[i] = out;
    }
    for (i, o) in outputs_b4.iter().enumerate() {
      assert_eq!(o, &outputs_after[i]);
    }
    Ok(())
  }

  #[test]
  fn puncture_all() -> Result<(), PPRFError> {
    let mut inputs = Vec::new();
    for i in 0..255 {
      inputs.push(vec![i as u8]);
    }
    let mut ggm = GGM::setup();
    for x in &inputs {
      ggm.puncture(x)?;
    }
    Ok(())
  }

  #[test]
  fn casting() {
    let bv_0 = bits![0].to_bitvec();
    let bv_1 = bvcast_u8_to_usize(&BitVec::<_, Lsb0>::from_slice(&[4]));
    assert_eq!(bv_0.len(), 1);
    assert_eq!(bv_1.len(), 8);
    assert!(bv_1.starts_with(&bv_0));
  }
}
