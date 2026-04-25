use core::arch::x86_64::*;

use super::{scalar, Vector};

#[derive(Copy, Clone)]
pub struct Impl(());

impl Impl {
    /// # Safety
    ///
    /// You must ensure that the CPU has the SSE2 feature
    #[inline]
    #[cfg(feature = "std")]
    pub unsafe fn new_unchecked() -> Impl {
        Impl(())
    }
}

impl Vector for Impl {
    #[inline]
    fn round_scramble(&self, acc: &mut [u64; 8], secret_end: &[u8; 64]) {
        // Safety: Type can only be constructed when SSE2 feature is present
        unsafe { round_scramble_sse2(acc, secret_end) }
    }

    #[inline]
    fn accumulate(&self, acc: &mut [u64; 8], stripe: &[u8; 64], secret: &[u8; 64]) {
        // Safety: Type can only be constructed when SSE2 feature is present
        unsafe { accumulate_sse2(acc, stripe, secret) }
    }
}

/// # Safety
///
/// You must ensure that the CPU has the SSE2 feature
#[inline]
#[target_feature(enable = "sse2")]
unsafe fn round_scramble_sse2(acc: &mut [u64; 8], secret_end: &[u8; 64]) {
    // The scalar implementation is autovectorized nicely enough
    scalar::Impl.round_scramble(acc, secret_end)
}

/// # Safety
///
/// You must ensure that the CPU has the SSE2 feature
#[inline]
#[target_feature(enable = "sse2")]
unsafe fn accumulate_sse2(acc: &mut [u64; 8], stripe: &[u8; 64], secret: &[u8; 64]) {
    let acc = acc.as_mut_ptr().cast::<__m128i>();
    let stripe = stripe.as_ptr().cast::<__m128i>();
    let secret = secret.as_ptr().cast::<__m128i>();

    // Safety: The caller has ensured we have the SSE2
    // feature. We load from and store to references so we
    // know that data is valid. We use unaligned loads /
    // stores. Data manipulation is otherwise done on
    // intermediate values.
    unsafe {
        for i in 0..4 {
            // See [align-acc].
            let mut acc_0 = _mm_loadu_si128(acc.add(i));
            let stripe_0 = _mm_loadu_si128(stripe.add(i));
            let secret_0 = _mm_loadu_si128(secret.add(i));

            // let value[i] = stripe[i] ^ secret[i];
            let value_0 = _mm_xor_si128(stripe_0, secret_0);

            // stripe_swap[i] = stripe[i ^ 1]
            let stripe_swap_0 = _mm_shuffle_epi32::<0b01_00_11_10>(stripe_0);

            // acc[i] += stripe_swap[i]
            acc_0 = _mm_add_epi64(acc_0, stripe_swap_0);

            // value_shift[i] = value[i] >> 32
            let value_shift_0 = _mm_srli_epi64::<32>(value_0);

            // product[i] = lower_32_bit(value[i]) * lower_32_bit(value_shift[i])
            let product_0 = _mm_mul_epu32(value_0, value_shift_0);

            // acc[i] += product[i]
            acc_0 = _mm_add_epi64(acc_0, product_0);

            _mm_storeu_si128(acc.add(i), acc_0);
        }
    }
}
