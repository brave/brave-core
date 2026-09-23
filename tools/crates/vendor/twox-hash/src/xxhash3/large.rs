use super::{
    assert_input_range, avalanche, primes::*, stripes_with_tail, Halves, Secret, SliceBackport as _,
};

#[cfg(feature = "xxhash3_128")]
use super::X128;

use crate::{IntoU128, IntoU64};

// This module is not `cfg`-gated because it is used by some of the
// SIMD implementations.
pub mod scalar;

#[cfg(all(target_arch = "aarch64", feature = "std"))]
pub mod neon;

#[cfg(all(target_arch = "x86_64", feature = "std"))]
pub mod avx2;

#[cfg(all(target_arch = "x86_64", feature = "std"))]
pub mod sse2;

macro_rules! dispatch {
    (
        fn $fn_name:ident<$($gen:ident),*>($($arg_name:ident : $arg_ty:ty),*) $(-> $ret_ty:ty)?
        [$($wheres:tt)*]
    ) => {
        #[inline]
        fn do_scalar<$($gen),*>($($arg_name : $arg_ty),*) $(-> $ret_ty)?
        where
            $($wheres)*
        {
            $fn_name($crate::xxhash3::large::scalar::Impl, $($arg_name),*)
        }

        /// # Safety
        ///
        /// You must ensure that the CPU has the NEON feature
        #[inline]
        #[target_feature(enable = "neon")]
        #[cfg(all(target_arch = "aarch64", feature = "std"))]
        unsafe fn do_neon<$($gen),*>($($arg_name : $arg_ty),*) $(-> $ret_ty)?
        where
            $($wheres)*
        {
            // Safety: The caller has ensured we have the NEON feature
            unsafe {
                $fn_name($crate::xxhash3::large::neon::Impl::new_unchecked(), $($arg_name),*)
            }
        }

        /// # Safety
        ///
        /// You must ensure that the CPU has the AVX2 feature
        #[inline]
        #[target_feature(enable = "avx2")]
        #[cfg(all(target_arch = "x86_64", feature = "std"))]
        unsafe fn do_avx2<$($gen),*>($($arg_name : $arg_ty),*) $(-> $ret_ty)?
        where
            $($wheres)*
        {
            // Safety: The caller has ensured we have the AVX2 feature
            unsafe {
                $fn_name($crate::xxhash3::large::avx2::Impl::new_unchecked(), $($arg_name),*)
            }
        }

        /// # Safety
        ///
        /// You must ensure that the CPU has the SSE2 feature
        #[inline]
        #[target_feature(enable = "sse2")]
        #[cfg(all(target_arch = "x86_64", feature = "std"))]
        unsafe fn do_sse2<$($gen),*>($($arg_name : $arg_ty),*) $(-> $ret_ty)?
        where
            $($wheres)*
        {
            // Safety: The caller has ensured we have the SSE2 feature
            unsafe {
                $fn_name($crate::xxhash3::large::sse2::Impl::new_unchecked(), $($arg_name),*)
            }
        }

        // Now we invoke the right function

        #[cfg(_internal_xxhash3_force_neon)]
        return unsafe { do_neon($($arg_name),*) };

        #[cfg(_internal_xxhash3_force_avx2)]
        return unsafe { do_avx2($($arg_name),*) };

        #[cfg(_internal_xxhash3_force_sse2)]
        return unsafe { do_sse2($($arg_name),*) };

        #[cfg(_internal_xxhash3_force_scalar)]
        return do_scalar($($arg_name),*);

        // This code can be unreachable if one of the `*_force_*` cfgs
        // are set above, but that's the point.
        #[allow(unreachable_code)]
        {
            #[cfg(all(target_arch = "aarch64", feature = "std"))]
            {
                if std::arch::is_aarch64_feature_detected!("neon") {
                    // Safety: We just ensured we have the NEON feature
                    return unsafe { do_neon($($arg_name),*) };
                }
            }

            #[cfg(all(target_arch = "x86_64", feature = "std"))]
            {
                if is_x86_feature_detected!("avx2") {
                    // Safety: We just ensured we have the AVX2 feature
                    return unsafe { do_avx2($($arg_name),*) };
                } else if is_x86_feature_detected!("sse2") {
                    // Safety: We just ensured we have the SSE2 feature
                    return unsafe { do_sse2($($arg_name),*) };
                }
            }
            do_scalar($($arg_name),*)
        }
    };
}
pub(crate) use dispatch;

pub trait Vector: Copy {
    fn round_scramble(&self, acc: &mut [u64; 8], secret_end: &[u8; 64]);

    fn accumulate(&self, acc: &mut [u64; 8], stripe: &[u8; 64], secret: &[u8; 64]);
}

#[rustfmt::skip]
pub const INITIAL_ACCUMULATORS: [u64; 8] = [
    PRIME32_3, PRIME64_1, PRIME64_2, PRIME64_3,
    PRIME64_4, PRIME32_2, PRIME64_5, PRIME32_1,
];

pub struct Algorithm<V>(pub V);

impl<V> Algorithm<V>
where
    V: Vector,
{
    #[inline]
    pub fn oneshot<F>(&self, secret: &Secret, input: &[u8], finalize: F) -> F::Output
    where
        F: super::Finalize,
    {
        assert_input_range!(241.., input.len());
        let mut acc = INITIAL_ACCUMULATORS;

        let stripes_per_block = (secret.len() - 64) / 8;
        let block_size = 64 * stripes_per_block;

        let mut blocks = input.chunks_exact(block_size);

        let last_block = if blocks.remainder().is_empty() {
            // Safety: We know that `input` is non-empty, which means
            // that either there will be a remainder or one or more
            // full blocks. That info isn't flowing to the optimizer,
            // so we use `unwrap_unchecked`.
            unsafe { blocks.next_back().unwrap_unchecked() }
        } else {
            blocks.remainder()
        };

        self.rounds(&mut acc, blocks, secret);

        let len = input.len();

        let last_stripe = input.last_chunk().unwrap();
        finalize.large(self.0, acc, last_block, last_stripe, secret, len)
    }

    #[inline]
    fn rounds<'a>(
        &self,
        acc: &mut [u64; 8],
        blocks: impl IntoIterator<Item = &'a [u8]>,
        secret: &Secret,
    ) {
        for block in blocks {
            let (stripes, _) = block.bp_as_chunks();

            self.round(acc, stripes, secret);
        }
    }

    #[inline]
    fn round(&self, acc: &mut [u64; 8], stripes: &[[u8; 64]], secret: &Secret) {
        let secret_end = secret.last_stripe();

        self.round_accumulate(acc, stripes, secret);
        self.0.round_scramble(acc, secret_end);
    }

    #[inline]
    fn round_accumulate(&self, acc: &mut [u64; 8], stripes: &[[u8; 64]], secret: &Secret) {
        let secrets = (0..stripes.len()).map(|i| {
            // Safety: The number of stripes is determined by the
            // block size, which is determined by the secret size.
            unsafe { secret.stripe(i) }
        });

        for (stripe, secret) in stripes.iter().zip(secrets) {
            self.0.accumulate(acc, stripe, secret);
        }
    }

    #[inline(always)]
    #[cfg(feature = "xxhash3_64")]
    pub fn finalize_64(
        &self,
        mut acc: [u64; 8],
        last_block: &[u8],
        last_stripe: &[u8; 64],
        secret: &Secret,
        len: usize,
    ) -> u64 {
        debug_assert!(!last_block.is_empty());
        self.last_round(&mut acc, last_block, last_stripe, secret);

        let low = len.into_u64().wrapping_mul(PRIME64_1);
        self.final_merge(&acc, low, secret.final_secret())
    }

    #[inline]
    #[cfg(feature = "xxhash3_128")]
    pub fn finalize_128(
        &self,
        mut acc: [u64; 8],
        last_block: &[u8],
        last_stripe: &[u8; 64],
        secret: &Secret,
        len: usize,
    ) -> u128 {
        debug_assert!(!last_block.is_empty());
        self.last_round(&mut acc, last_block, last_stripe, secret);

        let len = len.into_u64();

        let low = len.wrapping_mul(PRIME64_1);
        let low = self.final_merge(&acc, low, secret.final_secret());

        let high = !len.wrapping_mul(PRIME64_2);
        let high = self.final_merge(&acc, high, secret.for_128().final_secret());

        X128 { low, high }.into()
    }

    #[inline]
    fn last_round(
        &self,
        acc: &mut [u64; 8],
        block: &[u8],
        last_stripe: &[u8; 64],
        secret: &Secret,
    ) {
        // Accumulation steps are run for the stripes in the last block,
        // except for the last stripe (whether it is full or not)
        let (stripes, _) = stripes_with_tail(block);

        let secrets = (0..stripes.len()).map(|i| {
            // Safety: The number of stripes is determined by the
            // block size, which is determined by the secret size.
            unsafe { secret.stripe(i) }
        });

        for (stripe, secret) in stripes.iter().zip(secrets) {
            self.0.accumulate(acc, stripe, secret);
        }

        let last_stripe_secret = secret.last_stripe_secret_better_name();
        self.0.accumulate(acc, last_stripe, last_stripe_secret);
    }

    #[inline]
    fn final_merge(&self, acc: &[u64; 8], init_value: u64, secret: &[u8; 64]) -> u64 {
        let (secrets, _) = secret.bp_as_chunks();
        let mut result = init_value;
        for i in 0..4 {
            // 64-bit by 64-bit multiplication to 128-bit full result
            let mul_result = {
                let sa = u64::from_le_bytes(secrets[i * 2]);
                let sb = u64::from_le_bytes(secrets[i * 2 + 1]);

                let a = (acc[i * 2] ^ sa).into_u128();
                let b = (acc[i * 2 + 1] ^ sb).into_u128();
                a.wrapping_mul(b)
            };
            result = result.wrapping_add(mul_result.lower_half() ^ mul_result.upper_half());
        }
        avalanche(result)
    }
}
