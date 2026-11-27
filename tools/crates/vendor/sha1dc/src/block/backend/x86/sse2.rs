//! SHA-1 on an `x86` CPU without SHA-NI: the portable rounds, with the
//! schedule built four words at a time.
//!
//! Expanded a word at a time, as [`scalar`](super::super::scalar) does, the
//! schedule costs four loads, three XORs, a rotation and a store a word, as
//! much again as the rounds. Here it is built a group of four at a time:
//! words 16 to 31 by the usual recurrence, fixed up within a group as
//! `sha1msg2` would, and the rest by [`expand_rol2`](super::expand_rol2).
//! Each group is computed between two turns of the rounds, well before they
//! read it: built all up front, the rounds would wait.

// The safety of [`compress_spill`] rests on this.
#[cfg(not(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    target_feature = "sse2"
)))]
compile_error!("the sse2 backend needs an x86 or x86_64 target with SSE2");

#[cfg(target_arch = "x86")]
use core::arch::x86::*;
#[cfg(target_arch = "x86_64")]
use core::arch::x86_64::*;

use super::{REVERSE, alignr8, expand_rol2, rol, spill};
use crate::Schedule;
use crate::block::rounds::{K, Words, add, ch, five, maj, parity, step};
use crate::mem::load_u32x4;

/// Words `4k..4k + 4` of the block, reversed, as a group holds them.
#[inline]
#[target_feature(enable = "sse2")]
fn group<const K: usize>(m: &[u32; 16]) -> __m128i {
    const { assert!(K < 4, "the block has four groups") }
    let words = load_u32x4(m[4 * K..4 * K + 4].try_into().unwrap());
    _mm_shuffle_epi32(words, REVERSE)
}

/// Group `k` for `k` from 4 to 7, from the four before, as `sha1msg1` and
/// `sha1msg2` would compute it.
#[inline]
#[target_feature(enable = "sse2")]
fn expand16(a: __m128i, b: __m128i, c: __m128i, d: __m128i) -> __m128i {
    // `sha1msg1`: each word with the one two after it.
    let x = _mm_xor_si128(_mm_xor_si128(a, alignr8(a, b)), c);
    // `sha1msg2`: the top three words take the word below them from `d`, and
    // the bottom one the top one, just computed.
    let r = rol::<1, 31>(_mm_xor_si128(x, _mm_slli_si128(d, 4)));
    _mm_xor_si128(r, rol::<1, 31>(_mm_srli_si128(r, 12)))
}

/// Expands `m` into `w`, runs all 80 steps, and stores the two states that
/// recompression starts from, as [`scalar`](super::super::scalar) does.
pub(crate) fn compress_spill(
    ihv: &mut [u32; 5],
    m: &[u32; 16],
    w: &mut Schedule,
    state_58: &mut [u32; 5],
    state_65: &mut [u32; 5],
) {
    // SAFETY: the `compile_error!` at the top of the file means the target
    // has SSE2, and so every CPU it runs on.
    unsafe { compress_spill_sse2(ihv, m, w, state_58, state_65) }
}

#[inline]
#[target_feature(enable = "sse2")]
fn compress_spill_sse2(
    ihv: &mut [u32; 5],
    m: &[u32; 16],
    w: &mut Schedule,
    state_58: &mut [u32; 5],
    state_65: &mut [u32; 5],
) {
    let [mut a, mut b, mut c, mut d, mut e] = *ihv;

    let v0 = group::<0>(m);
    let v1 = group::<1>(m);
    let v2 = group::<2>(m);
    let v3 = group::<3>(m);
    spill::<0>(w, v0);
    spill::<4>(w, v1);
    spill::<8>(w, v2);
    spill::<12>(w, v3);
    let v4 = expand16(v0, v1, v2, v3);
    spill::<16>(w, v4);
    let v5 = expand16(v1, v2, v3, v4);
    spill::<20>(w, v5);

    // The first sixteen words are the block itself, read where it already is.
    five!(ch, K[0], a, b, c, d, e, m, 0);
    let v6 = expand16(v2, v3, v4, v5);
    spill::<24>(w, v6);
    five!(ch, K[0], a, b, c, d, e, m, 5);
    let v7 = expand16(v3, v4, v5, v6);
    spill::<28>(w, v7);
    five!(ch, K[0], a, b, c, d, e, m, 10);
    let v8 = expand_rol2(v0, v1, v4, v6, v7);
    spill::<32>(w, v8);
    step!(ch, K[0], a, b, c, d, e, m[15]);
    step!(ch, K[0], e, a, b, c, d, w[16]);
    step!(ch, K[0], d, e, a, b, c, w[17]);
    step!(ch, K[0], c, d, e, a, b, w[18]);
    step!(ch, K[0], b, c, d, e, a, w[19]);

    let v9 = expand_rol2(v1, v2, v5, v7, v8);
    spill::<36>(w, v9);
    five!(parity, K[1], a, b, c, d, e, w, 20);
    let v10 = expand_rol2(v2, v3, v6, v8, v9);
    spill::<40>(w, v10);
    five!(parity, K[1], a, b, c, d, e, w, 25);
    let v11 = expand_rol2(v3, v4, v7, v9, v10);
    spill::<44>(w, v11);
    five!(parity, K[1], a, b, c, d, e, w, 30);
    let v12 = expand_rol2(v4, v5, v8, v10, v11);
    spill::<48>(w, v12);
    five!(parity, K[1], a, b, c, d, e, w, 35);

    let v13 = expand_rol2(v5, v6, v9, v11, v12);
    spill::<52>(w, v13);
    five!(maj, K[2], a, b, c, d, e, w, 40);
    let v14 = expand_rol2(v6, v7, v10, v12, v13);
    spill::<56>(w, v14);
    five!(maj, K[2], a, b, c, d, e, w, 45);
    let v15 = expand_rol2(v7, v8, v11, v13, v14);
    spill::<60>(w, v15);
    five!(maj, K[2], a, b, c, d, e, w, 50);
    let v16 = expand_rol2(v8, v9, v12, v14, v15);
    spill::<64>(w, v16);

    // Step 58 falls three into a turn of the names, so that turn is written
    // out a step at a time. Step 65 falls on a turn.
    step!(maj, K[2], a, b, c, d, e, w[55]);
    step!(maj, K[2], e, a, b, c, d, w[56]);
    step!(maj, K[2], d, e, a, b, c, w[57]);
    *state_58 = [c, d, e, a, b];
    step!(maj, K[2], c, d, e, a, b, w[58]);
    step!(maj, K[2], b, c, d, e, a, w[59]);

    let v17 = expand_rol2(v9, v10, v13, v15, v16);
    spill::<68>(w, v17);
    five!(parity, K[3], a, b, c, d, e, w, 60);
    *state_65 = [a, b, c, d, e];
    let v18 = expand_rol2(v10, v11, v14, v16, v17);
    spill::<72>(w, v18);
    five!(parity, K[3], a, b, c, d, e, w, 65);
    let v19 = expand_rol2(v11, v12, v15, v17, v18);
    spill::<76>(w, v19);
    five!(parity, K[3], a, b, c, d, e, w, 70);
    five!(parity, K[3], a, b, c, d, e, w, 75);

    add(ihv, [a, b, c, d, e]);
}

#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;
    use crate::block::backend::scalar;
    use quickcheck::QuickCheck;

    /// The same state, schedule and stored states as the portable
    /// compression, for any block and starting state.
    #[test]
    fn matches_the_portable_compression() {
        fn prop(m: [u32; 16], ihv: [u32; 5]) -> bool {
            let (mut state, mut w) = (ihv, Schedule::zeroed());
            let (mut s58, mut s65) = ([0u32; 5], [0u32; 5]);
            compress_spill(&mut state, &m, &mut w, &mut s58, &mut s65);

            let (mut want, mut want_w) = (ihv, Schedule::zeroed());
            let (mut want_58, mut want_65) = ([0u32; 5], [0u32; 5]);
            scalar::compress_spill(&mut want, &m, &mut want_w, &mut want_58, &mut want_65);

            state == want && w.words() == want_w.words() && s58 == want_58 && s65 == want_65
        }
        QuickCheck::new()
            .tests(1_000)
            .quickcheck(prop as fn([u32; 16], [u32; 5]) -> bool);
    }
}
