//! SHA-1 `aarch64` backend built on the ARMv8 SHA-1 instructions.
//!
//! Computes a block's digest and spills the expanded message schedule, which
//! the collision detection needs. The spill is the difference from an
//! ordinary SHA-1: `vsha1su0q`/`vsha1su1q` keep the schedule in registers,
//! and the check needs all 80 words in memory.
//!
//! `vsha1cq`, `vsha1pq` and `vsha1mq` each do four rounds, so the work is in
//! groups of four. `vsha1h` carries the fifth working word between them, and
//! `vsha1su0q`/`vsha1su1q` take the schedule two steps at a time, far enough
//! ahead that the rounds do not wait for it.

#[cfg(not(target_arch = "aarch64"))]
compile_error!("the armv8 backend needs an aarch64 target");

use core::arch::aarch64::*;

use crate::Schedule;
use crate::block::rounds::{self, K};
use crate::mem::{load_u8x16, load_u32x4, store_u32x4};
use crate::ubc_check::RecompressFrom;

/// The four schedule words starting at `I`.
///
/// `I` is a const parameter, so a spill past the end of the schedule is a
/// compile error at the call site rather than a promise in a comment.
#[inline]
#[target_feature(enable = "sha2")]
fn spill<const I: usize>(w: &mut Schedule, v: uint32x4_t) {
    store_u32x4(w.window_mut::<I, 4>(), v);
}

/// The four message words starting at byte `I`, in native order.
#[inline]
#[target_feature(enable = "sha2")]
fn msg<const I: usize>(block: &[u8; 64]) -> uint32x4_t {
    const { assert!(I + 16 <= 64, "the load runs past the block") }
    let bytes = load_u8x16(block[I..I + 16].try_into().unwrap());
    vreinterpretq_u32_u8(vrev32q_u8(bytes))
}

/// The four schedule words of steps `T..T + 4`, exclusive-ored with the
/// partner's difference and the round constant added.
///
/// The read side of [`spill`], with `T` const for the same reason: a group
/// past the end is a compile error rather than a promise in a comment.
#[inline]
#[target_feature(enable = "sha2")]
fn wk<const T: usize>(m1: &Schedule, dm: &[u32; 80], k: u32) -> uint32x4_t {
    const { assert!(T + 4 <= 80, "the group runs past the schedule") }
    let spilled = load_u32x4(m1.window::<T, 4>());
    let diff = load_u32x4(dm[T..T + 4].try_into().unwrap());
    vaddq_u32(veorq_u32(spilled, diff), vdupq_n_u32(k))
}

/// The `abcd` half of the state. The fifth word is carried on its own.
#[inline]
#[target_feature(enable = "sha2")]
fn load_abcd(state: &[u32; 5]) -> uint32x4_t {
    load_u32x4(state.first_chunk().unwrap())
}

/// Writes `abcd` back, leaving the fifth word alone.
#[inline]
#[target_feature(enable = "sha2")]
fn store_abcd(state: &mut [u32; 5], v: uint32x4_t) {
    store_u32x4(state.first_chunk_mut().unwrap(), v);
}

/// Compresses one block, spilling the message schedule into `w`.
///
/// Requires `sha2`, so a caller that cannot prove the feature needs an
/// `unsafe` block.
#[inline]
#[target_feature(enable = "sha2")]
#[expect(
    clippy::too_many_lines,
    reason = "the block compression is one unrolled body"
)]
pub(crate) fn compress_spill(
    state: &mut [u32; 5],
    block: &[u8; 64],
    w: &mut Schedule,
    at_60: &mut [u32; 5],
    at_64: &mut [u32; 5],
) {
    let mut abcd = load_abcd(state);
    let mut e0 = state[4];
    let abcd_in = abcd;
    let e_in = e0;

    let k0 = vdupq_n_u32(K[0]);
    let k1 = vdupq_n_u32(K[1]);
    let k2 = vdupq_n_u32(K[2]);
    let k3 = vdupq_n_u32(K[3]);

    // The block, big-endian, one group of four words at a time.
    let mut msg0 = msg::<0>(block);
    let mut msg1 = msg::<16>(block);
    let mut msg2 = msg::<32>(block);
    let mut msg3 = msg::<48>(block);

    let mut e1;
    let mut tmp0;
    let mut tmp1;

    spill::<0>(w, msg0);
    spill::<4>(w, msg1);
    spill::<8>(w, msg2);
    spill::<12>(w, msg3);

    tmp0 = vaddq_u32(msg0, k0);
    tmp1 = vaddq_u32(msg1, k0);

    // Rounds 0-3
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e0, tmp0);
    tmp0 = vaddq_u32(msg2, k0);
    msg0 = vsha1su0q_u32(msg0, msg1, msg2);

    // Rounds 4-7
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, k0);
    msg0 = vsha1su1q_u32(msg0, msg3);
    spill::<16>(w, msg0);
    msg1 = vsha1su0q_u32(msg1, msg2, msg3);

    // Rounds 8-11
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e0, tmp0);
    tmp0 = vaddq_u32(msg0, k0);
    msg1 = vsha1su1q_u32(msg1, msg0);
    spill::<20>(w, msg1);
    msg2 = vsha1su0q_u32(msg2, msg3, msg0);

    // Rounds 12-15
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg1, k1);
    msg2 = vsha1su1q_u32(msg2, msg1);
    spill::<24>(w, msg2);
    msg3 = vsha1su0q_u32(msg3, msg0, msg1);

    // Rounds 16-19
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1cq_u32(abcd, e0, tmp0);
    tmp0 = vaddq_u32(msg2, k1);
    msg3 = vsha1su1q_u32(msg3, msg2);
    spill::<28>(w, msg3);
    msg0 = vsha1su0q_u32(msg0, msg1, msg2);

    // Rounds 20-23
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, k1);
    msg0 = vsha1su1q_u32(msg0, msg3);
    spill::<32>(w, msg0);
    msg1 = vsha1su0q_u32(msg1, msg2, msg3);

    // Rounds 24-27
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e0, tmp0);
    tmp0 = vaddq_u32(msg0, k1);
    msg1 = vsha1su1q_u32(msg1, msg0);
    spill::<36>(w, msg1);
    msg2 = vsha1su0q_u32(msg2, msg3, msg0);

    // Rounds 28-31
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg1, k1);
    msg2 = vsha1su1q_u32(msg2, msg1);
    spill::<40>(w, msg2);
    msg3 = vsha1su0q_u32(msg3, msg0, msg1);

    // Rounds 32-35
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e0, tmp0);
    tmp0 = vaddq_u32(msg2, k2);
    msg3 = vsha1su1q_u32(msg3, msg2);
    spill::<44>(w, msg3);
    msg0 = vsha1su0q_u32(msg0, msg1, msg2);

    // Rounds 36-39
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, k2);
    msg0 = vsha1su1q_u32(msg0, msg3);
    spill::<48>(w, msg0);
    msg1 = vsha1su0q_u32(msg1, msg2, msg3);

    // Rounds 40-43
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e0, tmp0);
    tmp0 = vaddq_u32(msg0, k2);
    msg1 = vsha1su1q_u32(msg1, msg0);
    spill::<52>(w, msg1);
    msg2 = vsha1su0q_u32(msg2, msg3, msg0);

    // Rounds 44-47
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg1, k2);
    msg2 = vsha1su1q_u32(msg2, msg1);
    spill::<56>(w, msg2);
    msg3 = vsha1su0q_u32(msg3, msg0, msg1);

    // Rounds 48-51
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e0, tmp0);
    tmp0 = vaddq_u32(msg2, k2);
    msg3 = vsha1su1q_u32(msg3, msg2);
    spill::<60>(w, msg3);
    msg0 = vsha1su0q_u32(msg0, msg1, msg2);

    // Rounds 52-55
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, k3);
    msg0 = vsha1su1q_u32(msg0, msg3);
    spill::<64>(w, msg0);
    msg1 = vsha1su0q_u32(msg1, msg2, msg3);

    // Rounds 56-59
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1mq_u32(abcd, e0, tmp0);
    tmp0 = vaddq_u32(msg0, k3);
    msg1 = vsha1su1q_u32(msg1, msg0);
    spill::<68>(w, msg1);
    msg2 = vsha1su0q_u32(msg2, msg3, msg0);

    // The state at step 60, for the check to start from.
    store_abcd(at_60, abcd);
    at_60[4] = e1;

    // Rounds 60-63
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg1, k3);
    msg2 = vsha1su1q_u32(msg2, msg1);
    spill::<72>(w, msg2);
    msg3 = vsha1su0q_u32(msg3, msg0, msg1);

    // And at step 64.
    store_abcd(at_64, abcd);
    at_64[4] = e0;

    // Rounds 64-67
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e0, tmp0);
    tmp0 = vaddq_u32(msg2, k3);
    msg3 = vsha1su1q_u32(msg3, msg2);
    spill::<76>(w, msg3);

    // Rounds 68-71
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);
    tmp1 = vaddq_u32(msg3, k3);

    // Rounds 72-75
    e1 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e0, tmp0);

    // Rounds 76-79
    e0 = vsha1h_u32(vgetq_lane_u32(abcd, 0));
    abcd = vsha1pq_u32(abcd, e1, tmp1);

    abcd = vaddq_u32(abcd_in, abcd);
    e0 = e0.wrapping_add(e_in);

    store_abcd(state, abcd);
    state[4] = e0;
}

/// Whether this candidate is the attack it is a candidate for.
///
/// The `aarch64` half of [`Backend::is_attack`], which says why it runs this
/// way round. Both halves run on the SHA-1 instructions, from the partner's
/// state at the nearest group boundary, step 60 or 64: out to step 80, which
/// gives the chaining value an attack would have had to start from, and then
/// in from that value, which must arrive back at the same state.
///
/// [`Backend::is_attack`]: crate::block::Backend::is_attack
#[inline]
#[target_feature(enable = "sha2")]
pub(crate) fn recompress(
    step: RecompressFrom,
    m1: &Schedule,
    dm: &[u32; 80],
    state: &[u32; 5],
    chaining_out: &[u32; 5],
) -> bool {
    let at = rounds::partner_boundary(step, m1, dm, state);

    let mut abcd = load_abcd(&at);
    let mut e = at[4];

    /// Four rounds on group `$g` of the partner schedule. `vsha1h` gives the
    /// fifth word of the group after, from the state before this one.
    macro_rules! group {
        ($f:ident, $k:expr, $g:expr) => {{
            let wk = wk::<{ 4 * $g }>(m1, dm, $k);
            let next = vsha1h_u32(vgetq_lane_u32(abcd, 0));
            abcd = $f(abcd, e, wk);
            e = next;
        }};
    }

    // Out to step 80, from 60 or from 64.
    if let RecompressFrom::Step58 = step {
        group!(vsha1pq_u32, K[3], 15);
    }
    group!(vsha1pq_u32, K[3], 16);
    group!(vsha1pq_u32, K[3], 17);
    group!(vsha1pq_u32, K[3], 18);
    group!(vsha1pq_u32, K[3], 19);

    // The feed-forward adds the input to the state at 80, so the only input
    // that gives this block's output is the output less that state.
    abcd = vsubq_u32(load_abcd(chaining_out), abcd);
    e = chaining_out[4].wrapping_sub(e);

    // In from there, as far as the state the way out started from.
    group!(vsha1cq_u32, K[0], 0);
    group!(vsha1cq_u32, K[0], 1);
    group!(vsha1cq_u32, K[0], 2);
    group!(vsha1cq_u32, K[0], 3);
    group!(vsha1cq_u32, K[0], 4);

    group!(vsha1pq_u32, K[1], 5);
    group!(vsha1pq_u32, K[1], 6);
    group!(vsha1pq_u32, K[1], 7);
    group!(vsha1pq_u32, K[1], 8);
    group!(vsha1pq_u32, K[1], 9);

    group!(vsha1mq_u32, K[2], 10);
    group!(vsha1mq_u32, K[2], 11);
    group!(vsha1mq_u32, K[2], 12);
    group!(vsha1mq_u32, K[2], 13);
    group!(vsha1mq_u32, K[2], 14);

    if let RecompressFrom::Step65 = step {
        group!(vsha1pq_u32, K[3], 15);
    }

    let mut reached = [0u32; 5];
    store_abcd(&mut reached, abcd);
    reached[4] = e;
    crate::block::xor(&reached, &at) == 0
}
