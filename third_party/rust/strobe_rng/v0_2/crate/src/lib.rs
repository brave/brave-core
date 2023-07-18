use rand_core::{CryptoRng, RngCore};
use strobe_rs::Strobe;

/// StrobeRng implements the RngCore trait by using STROBE as an entropy pool
pub struct StrobeRng {
  strobe: Strobe,
}

impl From<Strobe> for StrobeRng {
  fn from(strobe: Strobe) -> Self {
    StrobeRng { strobe }
  }
}

impl RngCore for StrobeRng {
  fn next_u32(&mut self) -> u32 {
    rand_core::impls::next_u32_via_fill(self)
  }

  fn next_u64(&mut self) -> u64 {
    rand_core::impls::next_u64_via_fill(self)
  }

  fn fill_bytes(&mut self, dest: &mut [u8]) {
    let dest_len = (dest.len() as u32).to_le_bytes();
    self.strobe.meta_ad(&dest_len, false);
    self.strobe.prf(dest, false);
  }

  fn try_fill_bytes(
    &mut self,
    dest: &mut [u8],
  ) -> Result<(), rand_core::Error> {
    self.fill_bytes(dest);
    Ok(())
  }
}

impl CryptoRng for StrobeRng {}
