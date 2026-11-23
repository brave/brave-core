//! SHA-1 scalar backend, for a CPU with no SHA-1 instructions.
//!
//! The counterpart of [`sha_ni`](super::x86::sha_ni) and
//! [`armv8`](super::armv8). Those two get the schedule for free from the
//! instructions and have to spill it; here it is expanded a word at a time
//! into the round that needs it, so the spill costs one store. On `x86` with
//! SSE2, [`sse2`](super::x86::sse2) builds it four words at a time instead.
//!
//! The steps come from [`rounds`](super::super::rounds), which also runs them
//! backwards for the detection.

use crate::Schedule;
use crate::block::rounds::{
    K, add, ch, five_expand, five_load, maj, parity, step, step_expand, step_load,
};

/// Expands `m` into `w`, runs all 80 steps, and stores the two states that
/// recompression starts from.
///
/// On `x86` with SSE2 this one only serves [`Backend::scalar`], so it stays
/// out of the block loop rather than growing it.
///
/// [`Backend::scalar`]: super::Backend::scalar
#[cfg_attr(
    all(
        any(target_arch = "x86", target_arch = "x86_64"),
        target_feature = "sse2"
    ),
    inline(never)
)]
pub(crate) fn compress_spill(
    ihv: &mut [u32; 5],
    m: &[u32; 16],
    w: &mut Schedule,
    state_58: &mut [u32; 5],
    state_65: &mut [u32; 5],
) {
    let [mut a, mut b, mut c, mut d, mut e] = *ihv;

    // The first sixteen words are the block itself; the rest are expanded.
    five_load!(ch, K[0], a, b, c, d, e, w, m, 0);
    five_load!(ch, K[0], a, b, c, d, e, w, m, 5);
    five_load!(ch, K[0], a, b, c, d, e, w, m, 10);
    step_load!(ch, K[0], a, b, c, d, e, w, m, 15);
    step_expand!(ch, K[0], e, a, b, c, d, w, 16);
    step_expand!(ch, K[0], d, e, a, b, c, w, 17);
    step_expand!(ch, K[0], c, d, e, a, b, w, 18);
    step_expand!(ch, K[0], b, c, d, e, a, w, 19);

    five_expand!(parity, K[1], a, b, c, d, e, w, 20);
    five_expand!(parity, K[1], a, b, c, d, e, w, 25);
    five_expand!(parity, K[1], a, b, c, d, e, w, 30);
    five_expand!(parity, K[1], a, b, c, d, e, w, 35);

    five_expand!(maj, K[2], a, b, c, d, e, w, 40);
    five_expand!(maj, K[2], a, b, c, d, e, w, 45);
    five_expand!(maj, K[2], a, b, c, d, e, w, 50);

    // Step 58 falls three into a turn of the names, so that turn is written
    // out a step at a time. Step 65 falls on a turn, so the names are already
    // in order there.
    step_expand!(maj, K[2], a, b, c, d, e, w, 55);
    step_expand!(maj, K[2], e, a, b, c, d, w, 56);
    step_expand!(maj, K[2], d, e, a, b, c, w, 57);
    *state_58 = [c, d, e, a, b];
    step_expand!(maj, K[2], c, d, e, a, b, w, 58);
    step_expand!(maj, K[2], b, c, d, e, a, w, 59);

    five_expand!(parity, K[3], a, b, c, d, e, w, 60);
    *state_65 = [a, b, c, d, e];

    five_expand!(parity, K[3], a, b, c, d, e, w, 65);
    five_expand!(parity, K[3], a, b, c, d, e, w, 70);
    five_expand!(parity, K[3], a, b, c, d, e, w, 75);

    add(ihv, [a, b, c, d, e]);
}
