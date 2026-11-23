use core::simd::prelude::*;
use core::slice;

use super::fallback;

#[must_use]
#[inline]
pub fn contains_null_or_utf8_4_byte_char_header(value: &[u8]) -> bool {
    let mut remainder = value;

    macro_rules! process {
        ($simd:ty) => {
            let (array_chunks, array_remainder) = as_chunks_simd::<{ <$simd>::LEN }>(remainder);
            remainder = array_remainder;

            let zero = <$simd>::splat(0x00);
            let mask = <$simd>::splat(0b1111_1000);
            let header = <$simd>::splat(0b1111_0000);

            for &array in array_chunks {
                let simd = <$simd>::from_array(array);

                if simd.simd_eq(zero).any() || (simd & mask).simd_eq(header).any() {
                    return true;
                }
            }
        };
    }

    process!(u8x64);
    process!(u8x32);
    process!(u8x16);
    process!(u8x8);
    process!(u8x4);
    process!(u8x2);

    fallback::contains_null_or_utf8_4_byte_char_header(remainder)
}

#[must_use]
#[inline]
pub fn contains_utf8_4_byte_char_header(value: &[u8]) -> bool {
    let mut remainder = value;

    macro_rules! process {
        ($simd:ty) => {
            let (array_chunks, array_remainder) = as_chunks_simd::<{ <$simd>::LEN }>(remainder);
            remainder = array_remainder;

            let mask = <$simd>::splat(0b1111_1000);
            let header = <$simd>::splat(0b1111_0000);

            for &array in array_chunks {
                let simd = <$simd>::from_array(array);

                if (simd & mask).simd_eq(header).any() {
                    return true;
                }
            }
        };
    }

    process!(u8x64);
    process!(u8x32);
    process!(u8x16);
    process!(u8x8);
    process!(u8x4);
    process!(u8x2);

    fallback::contains_utf8_4_byte_char_header(remainder)
}

#[must_use]
#[inline]
fn as_chunks_simd<const N: usize>(slice: &[u8]) -> (&[[u8; N]], &[u8]) {
    debug_assert_ne!(N, 0, "N must be greater than 0");
    let len_rounded_down = slice.len() / N * N;
    // SAFETY: The rounded-down value is always the same or smaller than the
    // original length, and thus must be in-bounds of the slice.
    let (multiple_of_n, remainder) = unsafe { slice.split_at_unchecked(len_rounded_down) };
    // SAFETY: multiple_of_n has a length that is a multiple of N.
    (unsafe { as_arrays::<N>(multiple_of_n) }, remainder)
}

/// # Safety
///
/// `slice` must have a length that is a multiple of `N`.
#[must_use]
#[inline]
unsafe fn as_arrays<const N: usize>(slice: &[u8]) -> &[[u8; N]] {
    debug_assert_ne!(N, 0, "N must be greater than 0");
    debug_assert_eq!(slice.len() % N, 0, "slice length must be a multiple of N");
    let len = slice.len() / N;
    // SAFETY: This is effectively a cast from &[u8] to &[[u8; N]].
    unsafe { slice::from_raw_parts(slice.as_ptr().cast::<[u8; N]>(), len) }
}
