use super::Vector;
use crate::xxhash3::{primes::PRIME32_1, SliceBackport as _};

#[derive(Copy, Clone)]
pub struct Impl;

impl Vector for Impl {
    #[inline]
    fn round_scramble(&self, acc: &mut [u64; 8], secret_end: &[u8; 64]) {
        let (last, _) = secret_end.bp_as_chunks();
        let last = last.iter().copied().map(u64::from_le_bytes);

        for (acc, secret) in acc.iter_mut().zip(last) {
            *acc ^= *acc >> 47;
            *acc ^= secret;
            *acc = acc.wrapping_mul(PRIME32_1);
        }
    }

    #[inline]
    fn accumulate(&self, acc: &mut [u64; 8], stripe: &[u8; 64], secret: &[u8; 64]) {
        let (stripe, _) = stripe.bp_as_chunks();
        let (secret, _) = secret.bp_as_chunks();

        for i in 0..8 {
            let stripe = u64::from_le_bytes(stripe[i]);
            let secret = u64::from_le_bytes(secret[i]);

            let value = stripe ^ secret;
            acc[i ^ 1] = acc[i ^ 1].wrapping_add(stripe);
            acc[i] = multiply_64_as_32_and_add(value, value >> 32, acc[i]);
        }
    }
}

#[inline]
#[cfg(any(miri, not(target_arch = "aarch64")))]
fn multiply_64_as_32_and_add(lhs: u64, rhs: u64, acc: u64) -> u64 {
    use super::IntoU64;

    let lhs = (lhs as u32).into_u64();
    let rhs = (rhs as u32).into_u64();

    let product = lhs.wrapping_mul(rhs);
    acc.wrapping_add(product)
}

#[inline]
// https://github.com/Cyan4973/xxHash/blob/d5fe4f54c47bc8b8e76c6da9146c32d5c720cd79/xxhash.h#L5595-L5610
// https://github.com/llvm/llvm-project/issues/98481
#[cfg(all(not(miri), target_arch = "aarch64"))]
fn multiply_64_as_32_and_add(lhs: u64, rhs: u64, acc: u64) -> u64 {
    let res;

    // Safety: We only compute using our argument values and do
    // not change memory.
    unsafe {
        core::arch::asm!(
            "umaddl {res}, {lhs:w}, {rhs:w}, {acc}",
            lhs = in(reg) lhs,
            rhs = in(reg) rhs,
            acc = in(reg) acc,
            res = out(reg) res,
            options(pure, nomem, nostack),
        )
    }

    res
}
