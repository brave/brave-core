//! This module contains implementations for the two finite fields of the Pallas
//! and Vesta curves.

mod fp;
mod fq;

pub use fp::*;
pub use fq::*;

/// Converts 64-bit little-endian limbs to 32-bit little endian limbs.
#[cfg(feature = "gpu")]
fn u64_to_u32(limbs: &[u64]) -> alloc::vec::Vec<u32> {
    limbs
        .iter()
        .flat_map(|limb| [(limb & 0xFFFF_FFFF) as u32, (limb >> 32) as u32].into_iter())
        .collect()
}

#[cfg(feature = "gpu")]
#[test]
fn test_u64_to_u32() {
    use rand::{RngCore, SeedableRng};
    use rand_xorshift::XorShiftRng;

    let mut rng = XorShiftRng::from_seed([0; 16]);
    let u64_limbs: alloc::vec::Vec<u64> = (0..6).map(|_| rng.next_u64()).collect();
    let u32_limbs = crate::fields::u64_to_u32(&u64_limbs);

    let u64_le_bytes: alloc::vec::Vec<u8> = u64_limbs
        .iter()
        .flat_map(|limb| limb.to_le_bytes())
        .collect();
    let u32_le_bytes: alloc::vec::Vec<u8> = u32_limbs
        .iter()
        .flat_map(|limb| limb.to_le_bytes())
        .collect();

    assert_eq!(u64_le_bytes, u32_le_bytes);
}
