//! The `x86`/`x86_64` compressions: [`sha_ni`] on a CPU with SHA-NI, behind
//! the [`ShaNi`] its detection makes, and [`sse2`] on one without, in place
//! of the portable [`scalar`](super::scalar) one.
//!
//! Both build the schedule a group of four words at a time, in the order
//! SHA-NI holds them and the spill lays them out, and share the recurrence
//! that takes it from word 32 on.

#[cfg(target_arch = "x86")]
use core::arch::x86::*;
#[cfg(target_arch = "x86_64")]
use core::arch::x86_64::*;

use crate::Schedule;
use crate::mem::store_u32x4;

mod sha_ni;
#[cfg(target_feature = "sse2")]
pub(super) mod sse2;

pub(super) use sha_ni::ShaNi;

/// SHA-NI holds the four words of a group in the opposite order to the
/// schedule, so every crossing between the two reverses them.
const REVERSE: i32 = 0b00_01_10_11;

/// Writes the four words a group is about to use into the schedule.
///
/// The group is held reversed, so it goes to the mirrored offset rather than
/// through a `pshufd` to put it in order. What that leaves is [`Schedule`]'s
/// backwards layout, which every reader of it already indexes through.
///
/// `I` is a const parameter, so a spill past the end of the schedule is a
/// compile error at the call site rather than a promise in a comment.
#[inline]
#[target_feature(enable = "sse2")]
fn spill<const I: usize>(w: &mut Schedule, msg: __m128i) {
    store_u32x4(w.window_mut::<I, 4>(), msg);
}

/// The top two words of `lo` under the bottom two of `hi`: `palignr` by
/// eight bytes, which needs SSSE3, from SSE2's `shufpd`.
#[inline]
#[target_feature(enable = "sse2")]
fn alignr8(hi: __m128i, lo: __m128i) -> __m128i {
    _mm_castpd_si128(_mm_shuffle_pd(
        _mm_castsi128_pd(lo),
        _mm_castsi128_pd(hi),
        1,
    ))
}

/// Each word rotated left by `L`, where `R` is `32 - L`.
#[inline]
#[target_feature(enable = "sse2")]
fn rol<const L: i32, const R: i32>(x: __m128i) -> __m128i {
    _mm_or_si128(_mm_slli_epi32(x, L), _mm_srli_epi32(x, R))
}

/// Schedule words `4k..4k + 4` for `k` from 8 on, from groups `k - 8`,
/// `k - 7`, `k - 4`, `k - 2` and `k - 1`.
///
/// From step 32 on, the usual recurrence applied twice gives
/// `W[t] = (W[t-6] ^ W[t-16] ^ W[t-28] ^ W[t-32]) <<< 2`, in which no word of
/// a group depends on another, so no `sha1msg2` or fixing up is needed.
#[inline]
#[target_feature(enable = "sse2")]
fn expand_rol2(v8: __m128i, v7: __m128i, v4: __m128i, v2: __m128i, v1: __m128i) -> __m128i {
    let far = _mm_xor_si128(_mm_xor_si128(v8, v7), v4);
    // Words `t - 6` to `t - 3`: the last two of group `k - 2`, the first two
    // of group `k - 1`.
    rol::<2, 30>(_mm_xor_si128(far, alignr8(v2, v1)))
}
