use core::arch::aarch64::*;

use super::Vector;
use crate::xxhash3::{primes::PRIME32_1, SliceBackport as _};

#[derive(Copy, Clone)]
pub struct Impl(());

impl Impl {
    /// # Safety
    ///
    /// You must ensure that the CPU has the NEON feature
    #[inline]
    #[cfg(feature = "std")]
    pub unsafe fn new_unchecked() -> Self {
        Self(())
    }
}

impl Vector for Impl {
    #[inline]
    fn round_scramble(&self, acc: &mut [u64; 8], secret_end: &[u8; 64]) {
        // Safety: Type can only be constructed when NEON feature is present
        unsafe { round_scramble_neon(acc, secret_end) }
    }

    #[inline]
    fn accumulate(&self, acc: &mut [u64; 8], stripe: &[u8; 64], secret: &[u8; 64]) {
        // Safety: Type can only be constructed when NEON feature is present
        unsafe { accumulate_neon(acc, stripe, secret) }
    }
}

/// # Safety
///
/// You must ensure that the CPU has the NEON feature
#[target_feature(enable = "neon")]
#[inline]
unsafe fn round_scramble_neon(acc: &mut [u64; 8], secret_end: &[u8; 64]) {
    let secret_base = secret_end.as_ptr().cast::<u64>();
    let (acc, _) = acc.bp_as_chunks_mut::<2>();

    for (i, acc) in acc.iter_mut().enumerate() {
        // Safety: The caller has ensured we have the NEON
        // feature. We load from and store to references so we
        // know that data is valid. We use unaligned loads /
        // stores. Data manipulation is otherwise done on
        // intermediate values.
        unsafe {
            let mut accv = vld1q_u64(acc.as_ptr());
            let secret = vld1q_u64(secret_base.add(i * 2));

            // tmp[i] = acc[i] >> 47
            let shifted = vshrq_n_u64::<47>(accv);

            // acc[i] ^= tmp[i]
            accv = veorq_u64(accv, shifted);

            // acc[i] ^= secret[i]
            accv = veorq_u64(accv, secret);

            // acc[i] *= PRIME32_1
            accv = xx_vmulq_u32_u64(accv, PRIME32_1 as u32);

            vst1q_u64(acc.as_mut_ptr(), accv);
        }
    }
}

/// We process 4x u64 at a time as that allows us to completely
/// fill a `uint64x2_t` with useful values when performing the
/// multiplication.
///
/// # Safety
///
/// You must ensure that the CPU has the NEON feature
#[target_feature(enable = "neon")]
#[inline]
unsafe fn accumulate_neon(acc: &mut [u64; 8], stripe: &[u8; 64], secret: &[u8; 64]) {
    let (acc2, _) = acc.bp_as_chunks_mut::<4>();
    for (i, acc) in acc2.iter_mut().enumerate() {
        // Safety: The caller has ensured we have the NEON
        // feature. We load from and store to references so we
        // know that data is valid. We use unaligned loads /
        // stores. Data manipulation is otherwise done on
        // intermediate values.
        unsafe {
            let mut accv_0 = vld1q_u64(acc.as_ptr().cast::<u64>());
            let mut accv_1 = vld1q_u64(acc.as_ptr().cast::<u64>().add(2));
            let stripe_0 = vld1q_u64(stripe.as_ptr().cast::<u64>().add(i * 4));
            let stripe_1 = vld1q_u64(stripe.as_ptr().cast::<u64>().add(i * 4 + 2));
            let secret_0 = vld1q_u64(secret.as_ptr().cast::<u64>().add(i * 4));
            let secret_1 = vld1q_u64(secret.as_ptr().cast::<u64>().add(i * 4 + 2));

            // stripe_rot[i ^ 1] = stripe[i];
            let stripe_rot_0 = vextq_u64::<1>(stripe_0, stripe_0);
            let stripe_rot_1 = vextq_u64::<1>(stripe_1, stripe_1);

            // value[i] = stripe[i] ^ secret[i];
            let value_0 = veorq_u64(stripe_0, secret_0);
            let value_1 = veorq_u64(stripe_1, secret_1);

            // sum[i] = value[i] * (value[i] >> 32) + stripe_rot[i]
            //
            // Each vector has 64-bit values, but we treat them as
            // 32-bit and then unzip them. This naturally splits
            // the upper and lower 32 bits.
            let parts_0 = vreinterpretq_u32_u64(value_0);
            let parts_1 = vreinterpretq_u32_u64(value_1);

            let hi = vuzp1q_u32(parts_0, parts_1);
            let lo = vuzp2q_u32(parts_0, parts_1);

            let sum_0 = vmlal_u32(stripe_rot_0, vget_low_u32(hi), vget_low_u32(lo));
            let sum_1 = vmlal_high_u32(stripe_rot_1, hi, lo);

            reordering_barrier(sum_0);
            reordering_barrier(sum_1);

            // acc[i] += sum[i]
            accv_0 = vaddq_u64(accv_0, sum_0);
            accv_1 = vaddq_u64(accv_1, sum_1);

            vst1q_u64(acc.as_mut_ptr().cast::<u64>(), accv_0);
            vst1q_u64(acc.as_mut_ptr().cast::<u64>().add(2), accv_1);
        };
    }
}

// There is no `vmulq_u64` (multiply 64-bit by 64-bit, keeping the
// lower 64 bits of the result) operation, so we have to make our
// own out of 32-bit operations . We can simplify by realizing
// that we are always multiplying by a 32-bit number.
//
// The basic algorithm is traditional long multiplication. `[]`
// denotes groups of 32 bits.
//
//         [AAAA][BBBB]
// x             [CCCC]
// --------------------
//         [BCBC][BCBC]
// + [ACAC][ACAC]
// --------------------
//         [ACBC][BCBC] // 64-bit truncation occurs
//
// This can be written in NEON as a vectorwise wrapping
// multiplication of the high-order chunk of the input (`A`)
// against the constant and then a multiply-widen-and-accumulate
// of the low-order chunk of the input and the constant:
//
// 1. High-order, vectorwise
//
//         [AAAA][BBBB]
// x       [CCCC][0000]
// --------------------
//         [ACAC][0000]
//
// 2. Low-order, widening
//
//               [BBBB]
// x             [CCCC] // widening
// --------------------
//         [BCBC][BCBC]
//
// 3. Accumulation
//
//         [ACAC][0000]
// +       [BCBC][BCBC] // vectorwise
// --------------------
//         [ACBC][BCBC]
//
// Thankfully, NEON has a single multiply-widen-and-accumulate
// operation.
#[inline]
pub fn xx_vmulq_u32_u64(input: uint64x2_t, og_factor: u32) -> uint64x2_t {
    // Safety: We only compute using our argument values and do
    // not change memory.
    unsafe {
        let input_as_u32 = vreinterpretq_u32_u64(input);
        let factor = vmov_n_u32(og_factor);
        let factor_striped = vmovq_n_u64(u64::from(og_factor) << 32);
        let factor_striped = vreinterpretq_u32_u64(factor_striped);

        let high_shifted_as_32 = vmulq_u32(input_as_u32, factor_striped);
        let high_shifted = vreinterpretq_u64_u32(high_shifted_as_32);

        let input_lo = vmovn_u64(input);
        vmlal_u32(high_shifted, input_lo, factor)
    }
}

/// # Safety
///
/// You must ensure that the CPU has the NEON feature
//
// https://github.com/Cyan4973/xxHash/blob/d5fe4f54c47bc8b8e76c6da9146c32d5c720cd79/xxhash.h#L5312-L5323
#[inline]
#[target_feature(enable = "neon")]
unsafe fn reordering_barrier(r: uint64x2_t) {
    // Safety: The caller has ensured we have the NEON feature. We
    // aren't doing anything with the argument, so we shouldn't be
    // able to cause unsafety!
    unsafe {
        core::arch::asm!(
            "/* {r:v} */",
            r = in(vreg) r,
            options(nomem, nostack),
        )
    }
}
