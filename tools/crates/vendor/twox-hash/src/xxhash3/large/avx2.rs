use core::arch::x86_64::*;

use super::{scalar, Vector};

#[derive(Copy, Clone)]
pub struct Impl(());

impl Impl {
    /// # Safety
    ///
    /// You must ensure that the CPU has the AVX2 feature
    #[inline]
    #[cfg(feature = "std")]
    pub unsafe fn new_unchecked() -> Impl {
        Impl(())
    }
}

impl Vector for Impl {
    #[inline]
    fn round_scramble(&self, acc: &mut [u64; 8], secret_end: &[u8; 64]) {
        // Safety: Type can only be constructed when AVX2 feature is present
        unsafe { round_scramble_avx2(acc, secret_end) }
    }

    #[inline]
    fn accumulate(&self, acc: &mut [u64; 8], stripe: &[u8; 64], secret: &[u8; 64]) {
        // Safety: Type can only be constructed when AVX2 feature is present
        unsafe { accumulate_avx2(acc, stripe, secret) }
    }
}

/// # Safety
///
/// You must ensure that the CPU has the AVX2 feature
#[inline]
#[target_feature(enable = "avx2")]
unsafe fn round_scramble_avx2(acc: &mut [u64; 8], secret_end: &[u8; 64]) {
    // The scalar implementation is autovectorized nicely enough
    scalar::Impl.round_scramble(acc, secret_end)
}

/// # Safety
///
/// You must ensure that the CPU has the AVX2 feature
#[inline]
#[target_feature(enable = "avx2")]
unsafe fn accumulate_avx2(acc: &mut [u64; 8], stripe: &[u8; 64], secret: &[u8; 64]) {
    let acc = acc.as_mut_ptr().cast::<__m256i>();
    let stripe = stripe.as_ptr().cast::<__m256i>();
    let secret = secret.as_ptr().cast::<__m256i>();

    // Safety: The caller has ensured we have the AVX2
    // feature. We load from and store to references so we
    // know that data is valid. We use unaligned loads /
    // stores. Data manipulation is otherwise done on
    // intermediate values.
    unsafe {
        for i in 0..2 {
            // [align-acc]: The C code aligns the accumulator to avoid
            // the unaligned load and store here, but that doesn't
            // seem to be a big performance loss.
            let mut acc_0 = _mm256_loadu_si256(acc.add(i));
            let stripe_0 = _mm256_loadu_si256(stripe.add(i));
            let secret_0 = _mm256_loadu_si256(secret.add(i));

            // let value[i] = stripe[i] ^ secret[i];
            let value_0 = _mm256_xor_si256(stripe_0, secret_0);

            // stripe_swap[i] = stripe[i ^ 1]
            let stripe_swap_0 = _mm256_shuffle_epi32::<0b01_00_11_10>(stripe_0);

            // acc[i] += stripe_swap[i]
            acc_0 = _mm256_add_epi64(acc_0, stripe_swap_0);

            // value_shift[i] = value[i] >> 32
            let value_shift_0 = _mm256_srli_epi64::<32>(value_0);

            // product[i] = lower_32_bit(value[i]) * lower_32_bit(value_shift[i])
            let product_0 = _mm256_mul_epu32(value_0, value_shift_0);

            // acc[i] += product[i]
            acc_0 = _mm256_add_epi64(acc_0, product_0);

            _mm256_storeu_si256(acc.add(i), acc_0);
        }
    }
}
