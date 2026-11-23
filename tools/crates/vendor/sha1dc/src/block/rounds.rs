//! SHA-1 block compression, and the recompression that detection needs.
//!
//! Everything here runs whatever backend is in use. [`states_from_60_64`]
//! finishes the states a hardware backend stores,
//! [`recompression_step`] is the detection itself, and [`compression_w`] is
//! the mitigation. The steps themselves are also the scalar backend, in
//! [`super::backend::scalar`].
//!
//! # The compression function
//!
//! Written from [FIPS 180-1]. For each step `t` of 80 the working state
//! `(A, B, C, D, E)` becomes
//!
//! ```text
//! (ROTL(A,5) + f_t(B,C,D) + E + K_t + W_t,  A,  ROTL(B,30),  C,  D)
//! ```
//!
//! Only the first and third words change. The rest move one place. So a step
//! does not move anything: it writes the new word over `E`, rotates `B`, and
//! the caller renames the five words one place round. After five steps the
//! names line up again, which is why the work here is grouped in fives.
//!
//! # Recompression
//!
//! Detection needs more than the digest. Algorithm 1 of the [paper] asks,
//! for each candidate attack, what the *other* message of the pair would
//! hash to. That message differs from this one by a fixed difference, so its
//! state at one step is known too. From there the compression function runs
//! backwards to the input chaining value and forwards to the output one.
//!
//! A step is reversible because only two words change: `ROTL(B,30)` undoes,
//! and the added word subtracts. [`recompression_step`] runs backwards from a
//! stored state to step 0, then forwards to step 80, and adds the two to get
//! the output chaining value the pair would share.
//!
//! Steps 58 and 65 are the two the DV table names. The scalar backend stores
//! the state there on the way past. A hardware backend runs four steps at a
//! time and can only stop between groups of them, so it stores the states at
//! steps 60 and 64, and [`states_from_60_64`] takes them two steps back and
//! one on. A stored state is in the order `(A, B, C, D, E)` of the step it
//! belongs to.
//!
//! [FIPS 180-1]: https://csrc.nist.gov/pubs/fips/180-1/final
//! [paper]: https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-stevens.pdf

use crate::Schedule;
use crate::ubc_check::RecompressFrom;

/// The round constants, one per 20 steps. FIPS 180-1, section 5.
pub(crate) const K: [u32; 4] = [0x5A82_7999, 0x6ED9_EBA1, 0x8F1B_BCDC, 0xCA62_C1D6];

/// `Ch`, the choice function of steps 0 to 19.
#[inline(always)]
pub(crate) fn ch(b: u32, c: u32, d: u32) -> u32 {
    d ^ (b & (c ^ d))
}

/// `Parity`, the XOR of steps 20 to 39 and 60 to 79.
#[inline(always)]
pub(crate) fn parity(b: u32, c: u32, d: u32) -> u32 {
    b ^ c ^ d
}

/// `Maj`, the majority function of steps 40 to 59.
#[inline(always)]
pub(crate) fn maj(b: u32, c: u32, d: u32) -> u32 {
    (b & c) | (d & (b | c))
}

/// One step. Writes over `$e` and rotates `$b`; the caller renames.
macro_rules! step {
    ($f:ident, $k:expr, $a:ident, $b:ident, $c:ident, $d:ident, $e:ident, $w:expr) => {{
        // The four terms that do not depend on `$e` are summed first. `$e`
        // carries the chain from one step to the next, so putting it last
        // keeps three of the four additions off that chain.
        $e = $e.wrapping_add(
            $a.rotate_left(5)
                .wrapping_add($f($b, $c, $d))
                .wrapping_add($k)
                .wrapping_add($w),
        );
        $b = $b.rotate_left(30);
    }};
}

/// One step backwards. Undoes the rotation first, because the word that
/// subtracts is a function of the unrotated value.
macro_rules! unstep {
    ($f:ident, $k:expr, $a:ident, $b:ident, $c:ident, $d:ident, $e:ident, $w:expr) => {{
        $c = $c.rotate_right(30);
        $a = $a.wrapping_sub(
            $b.rotate_left(5)
                .wrapping_add($f($c, $d, $e))
                .wrapping_add($k)
                .wrapping_add($w),
        );
    }};
}

/// Five steps, which is one turn of the names. The words go in and come out
/// under the names they started with.
macro_rules! five {
    ($f:ident, $k:expr, $a:ident, $b:ident, $c:ident, $d:ident, $e:ident, $w:expr, $t:expr) => {{
        step!($f, $k, $a, $b, $c, $d, $e, $w.at($t));
        step!($f, $k, $e, $a, $b, $c, $d, $w.at($t + 1));
        step!($f, $k, $d, $e, $a, $b, $c, $w.at($t + 2));
        step!($f, $k, $c, $d, $e, $a, $b, $w.at($t + 3));
        step!($f, $k, $b, $c, $d, $e, $a, $w.at($t + 4));
    }};
}

/// Five steps backwards: the same five in the other order.
macro_rules! unfive {
    ($f:ident, $k:expr, $a:ident, $b:ident, $c:ident, $d:ident, $e:ident, $w:expr, $t:expr) => {{
        unstep!($f, $k, $a, $b, $c, $d, $e, $w.at($t + 4));
        unstep!($f, $k, $b, $c, $d, $e, $a, $w.at($t + 3));
        unstep!($f, $k, $c, $d, $e, $a, $b, $w.at($t + 2));
        unstep!($f, $k, $d, $e, $a, $b, $c, $w.at($t + 1));
        unstep!($f, $k, $e, $a, $b, $c, $d, $w.at($t));
    }};
}

/// A source of schedule words for the rounds above: an array for the
/// compression, the block itself for its first sixteen, [`Xor`] for the
/// recompression.
///
/// Every index is a constant at the call, so `at` is a constant-offset load
/// with no bounds check. [`Schedule`] knows where the spill actually put the
/// word; nothing here has to.
pub(crate) trait Words {
    fn at(&self, t: usize) -> u32;
}

impl Words for Schedule {
    #[inline(always)]
    fn at(&self, t: usize) -> u32 {
        self[t]
    }
}

impl Words for [u32; 16] {
    #[inline(always)]
    fn at(&self, t: usize) -> u32 {
        self[t]
    }
}

/// The partner schedule of a candidate attack, as the two arrays it is the
/// XOR of rather than the array it would be.
///
/// The recompression reads every word exactly once, so storing them buys
/// nothing — and building the array is dearer than it looks. The compiler
/// vectorises the XOR, then has to move all eighty words back to the integer
/// registers the steps use: a hundred `fmov`/`mov.s` on `aarch64`, 134
/// `movd`/`movq` on `x86_64`. Forming each word where its step runs is 8% to
/// 16% faster per candidate.
struct Xor<'a> {
    m1: &'a Schedule,
    dm: &'a [u32; 80],
}

impl Words for Xor<'_> {
    #[inline(always)]
    fn at(&self, t: usize) -> u32 {
        // Only `m1` is a spill; the difference comes from the table, which
        // is in step order on every target.
        self.m1[t] ^ self.dm[t]
    }
}

/// One step, expanding the schedule word it needs first.
///
/// The word is kept for the step rather than read back from `w`, and the
/// expansion sits next to a chain of steps that cannot start early, so the
/// processor fills one with the other. Expanding the whole schedule up front
/// instead measures 14% slower.
///
/// The indices are constants, so the bounds are settled at compile time. A
/// loop over `16..80` costs four bounds checks a word, worth another 25%.
macro_rules! step_expand {
    ($f:ident, $k:expr, $a:ident, $b:ident, $c:ident, $d:ident, $e:ident, $w:expr, $t:expr) => {{
        let word = ($w[$t - 3] ^ $w[$t - 8] ^ $w[$t - 14] ^ $w[$t - 16]).rotate_left(1);
        $w[$t] = word;
        step!($f, $k, $a, $b, $c, $d, $e, word);
    }};
}

/// One step over a word of the block itself, which is stored as it is used.
/// Copying the block into `w` first and reading it back costs 16 loads a
/// block, worth about 4%.
macro_rules! step_load {
    ($f:ident, $k:expr, $a:ident, $b:ident, $c:ident, $d:ident, $e:ident, $w:expr, $m:expr, $t:expr) => {{
        let word = $m[$t];
        $w[$t] = word;
        step!($f, $k, $a, $b, $c, $d, $e, word);
    }};
}

/// Five steps over the block itself, which is one turn of the names.
macro_rules! five_load {
    ($f:ident, $k:expr, $a:ident, $b:ident, $c:ident, $d:ident, $e:ident, $w:expr, $m:expr, $t:expr) => {{
        step_load!($f, $k, $a, $b, $c, $d, $e, $w, $m, $t);
        step_load!($f, $k, $e, $a, $b, $c, $d, $w, $m, $t + 1);
        step_load!($f, $k, $d, $e, $a, $b, $c, $w, $m, $t + 2);
        step_load!($f, $k, $c, $d, $e, $a, $b, $w, $m, $t + 3);
        step_load!($f, $k, $b, $c, $d, $e, $a, $w, $m, $t + 4);
    }};
}

/// Five expanding steps, which is one turn of the names.
macro_rules! five_expand {
    ($f:ident, $k:expr, $a:ident, $b:ident, $c:ident, $d:ident, $e:ident, $w:expr, $t:expr) => {{
        step_expand!($f, $k, $a, $b, $c, $d, $e, $w, $t);
        step_expand!($f, $k, $e, $a, $b, $c, $d, $w, $t + 1);
        step_expand!($f, $k, $d, $e, $a, $b, $c, $w, $t + 2);
        step_expand!($f, $k, $c, $d, $e, $a, $b, $w, $t + 3);
        step_expand!($f, $k, $b, $c, $d, $e, $a, $w, $t + 4);
    }};
}

#[allow(
    unused_imports,
    reason = "only the `x86` compression without SHA-NI uses it outside this file"
)]
pub(crate) use five;
/// The round primitives are shared with [`super::backend::scalar`] and the
/// `x86` one without SHA-NI, which run them over a whole block.
pub(crate) use {five_expand, five_load, step, step_expand, step_load};

#[inline(always)]
pub(crate) fn add(left: &mut [u32; 5], right: [u32; 5]) {
    for (l, r) in left.iter_mut().zip(right) {
        *l = l.wrapping_add(r);
    }
}

/// All 80 steps over an expanded schedule, added into `ihv`.
pub(crate) fn compression_w(ihv: &mut [u32; 5], w: &Schedule) {
    let [mut a, mut b, mut c, mut d, mut e] = *ihv;

    five!(ch, K[0], a, b, c, d, e, w, 0);
    five!(ch, K[0], a, b, c, d, e, w, 5);
    five!(ch, K[0], a, b, c, d, e, w, 10);
    five!(ch, K[0], a, b, c, d, e, w, 15);

    five!(parity, K[1], a, b, c, d, e, w, 20);
    five!(parity, K[1], a, b, c, d, e, w, 25);
    five!(parity, K[1], a, b, c, d, e, w, 30);
    five!(parity, K[1], a, b, c, d, e, w, 35);

    five!(maj, K[2], a, b, c, d, e, w, 40);
    five!(maj, K[2], a, b, c, d, e, w, 45);
    five!(maj, K[2], a, b, c, d, e, w, 50);
    five!(maj, K[2], a, b, c, d, e, w, 55);

    five!(parity, K[3], a, b, c, d, e, w, 60);
    five!(parity, K[3], a, b, c, d, e, w, 65);
    five!(parity, K[3], a, b, c, d, e, w, 70);
    five!(parity, K[3], a, b, c, d, e, w, 75);

    add(ihv, [a, b, c, d, e]);
}

/// The stored states from a hardware backend's own at steps 60 and 64, which
/// come in `state_58` and `state_65`: one step on to 65, two back to 58.
#[allow(dead_code, reason = "a target with no SHA-1 instructions never asks")]
pub(crate) fn states_from_60_64(
    w: &Schedule,
    need_58: bool,
    state_58: &mut [u32; 5],
    state_65: &mut [u32; 5],
) {
    let [a, mut b, c, d, mut e] = *state_65;
    step!(parity, K[3], a, b, c, d, e, w[64]);
    *state_65 = [e, a, b, c, d];

    if need_58 {
        let [mut a, mut b, mut c, mut d, e] = *state_58;
        unstep!(maj, K[2], a, b, c, d, e, w[59]);
        unstep!(maj, K[2], b, c, d, e, a, w[58]);
        *state_58 = [c, d, e, a, b];
    }
}

/// The state before step `t`, from a plain run of the compression over an
/// expanded schedule, for the tests.
///
/// A loop with the five words moved by hand rather than the renaming macros
/// above, so that a test comparing against it does not share their faults.
#[cfg(all(test, feature = "std"))]
pub(crate) fn state_at(ihv: &[u32; 5], w: &Schedule, t: usize) -> [u32; 5] {
    let [mut a, mut b, mut c, mut d, mut e] = *ihv;
    for s in 0..t {
        let f = match s / 20 {
            0 => ch(b, c, d),
            2 => maj(b, c, d),
            _ => parity(b, c, d),
        };
        let next = a
            .rotate_left(5)
            .wrapping_add(f)
            .wrapping_add(e)
            .wrapping_add(K[s / 20])
            .wrapping_add(w[s]);
        (a, b, c, d, e) = (next, a, b.rotate_left(30), c, d);
    }
    [a, b, c, d, e]
}

/// The chaining values the partner of this block would give, from its state
/// at step 58 or at step 65.
///
/// The partner schedule is `m1` XORed with `dm`, formed a word at a time as
/// the steps run. `ihvin` gets the input chaining value, reached by running
/// backwards, and `ihvout` the output one, which is `ihvin` plus the state
/// at step 80.
///
/// Both directions start from the same stored state and meet only in that
/// final addition, so they are two independent chains and the steps below
/// alternate between them. Each one is a serial dependency a couple of
/// cycles deep, and a processor with spare width can hold both at once: left
/// to itself it already overlaps a third to two thirds of the shorter chain,
/// and taking the rest is worth about a seventh of this function.
///
/// The backward chain is the longer one either way — 58 steps against 22
/// from step 58, and 65 against 15 from step 65 — so it runs on alone once
/// the forward chain has finished.
pub(crate) fn recompression_step(
    step: RecompressFrom,
    ihvin: &mut [u32; 5],
    ihvout: &mut [u32; 5],
    m1: &Schedule,
    dm: &[u32; 80],
    state: &[u32; 5],
) {
    let me2 = &Xor { m1, dm };
    // `b*` walks back towards step 0, `f*` on towards step 80.
    let [mut b0, mut b1, mut b2, mut b3, mut b4] = *state;
    let [mut f0, mut f1, mut f2, mut f3, mut f4] = *state;

    match step {
        RecompressFrom::Step58 => {
            // The steps that are not part of a whole turn, which leave each
            // chain's names three places round.
            unstep!(maj, K[2], b0, b1, b2, b3, b4, me2.at(57));
            unstep!(maj, K[2], b1, b2, b3, b4, b0, me2.at(56));
            unstep!(maj, K[2], b2, b3, b4, b0, b1, me2.at(55));
            step!(maj, K[2], f0, f1, f2, f3, f4, me2.at(58));
            step!(maj, K[2], f4, f0, f1, f2, f3, me2.at(59));

            unfive!(maj, K[2], b3, b4, b0, b1, b2, me2, 50);
            five!(parity, K[3], f3, f4, f0, f1, f2, me2, 60);
            unfive!(maj, K[2], b3, b4, b0, b1, b2, me2, 45);
            five!(parity, K[3], f3, f4, f0, f1, f2, me2, 65);
            unfive!(maj, K[2], b3, b4, b0, b1, b2, me2, 40);
            five!(parity, K[3], f3, f4, f0, f1, f2, me2, 70);
            unfive!(parity, K[1], b3, b4, b0, b1, b2, me2, 35);
            five!(parity, K[3], f3, f4, f0, f1, f2, me2, 75);

            // The forward chain is done; the rest of the way back is alone.
            unfive!(parity, K[1], b3, b4, b0, b1, b2, me2, 30);
            unfive!(parity, K[1], b3, b4, b0, b1, b2, me2, 25);
            unfive!(parity, K[1], b3, b4, b0, b1, b2, me2, 20);
            unfive!(ch, K[0], b3, b4, b0, b1, b2, me2, 15);
            unfive!(ch, K[0], b3, b4, b0, b1, b2, me2, 10);
            unfive!(ch, K[0], b3, b4, b0, b1, b2, me2, 5);
            unfive!(ch, K[0], b3, b4, b0, b1, b2, me2, 0);

            *ihvin = [b3, b4, b0, b1, b2];
            *ihvout = *ihvin;
            add(ihvout, [f3, f4, f0, f1, f2]);
        }
        RecompressFrom::Step65 => {
            unfive!(parity, K[3], b0, b1, b2, b3, b4, me2, 60);
            five!(parity, K[3], f0, f1, f2, f3, f4, me2, 65);
            unfive!(maj, K[2], b0, b1, b2, b3, b4, me2, 55);
            five!(parity, K[3], f0, f1, f2, f3, f4, me2, 70);
            unfive!(maj, K[2], b0, b1, b2, b3, b4, me2, 50);
            five!(parity, K[3], f0, f1, f2, f3, f4, me2, 75);

            unfive!(maj, K[2], b0, b1, b2, b3, b4, me2, 45);
            unfive!(maj, K[2], b0, b1, b2, b3, b4, me2, 40);
            unfive!(parity, K[1], b0, b1, b2, b3, b4, me2, 35);
            unfive!(parity, K[1], b0, b1, b2, b3, b4, me2, 30);
            unfive!(parity, K[1], b0, b1, b2, b3, b4, me2, 25);
            unfive!(parity, K[1], b0, b1, b2, b3, b4, me2, 20);
            unfive!(ch, K[0], b0, b1, b2, b3, b4, me2, 15);
            unfive!(ch, K[0], b0, b1, b2, b3, b4, me2, 10);
            unfive!(ch, K[0], b0, b1, b2, b3, b4, me2, 5);
            unfive!(ch, K[0], b0, b1, b2, b3, b4, me2, 0);

            *ihvin = [b0, b1, b2, b3, b4];
            *ihvout = *ihvin;
            add(ihvout, [f0, f1, f2, f3, f4]);
        }
    }
}

/// The partner's state at the step a hardware backend can start from: 60
/// for a DV stored at step 58, 64 for one stored at 65.
///
/// The SHA-1 instructions run four steps at a time, so they start and stop
/// only between groups of four. The stored state is the partner's too, and
/// the steps between it and the nearest boundary are taken here: two forward
/// from 58, one back from 65. A state is in the order `(A, B, C, D, E)` of the
/// step it belongs to.
#[allow(dead_code, reason = "a target with no SHA-1 instructions never asks")]
pub(crate) fn partner_boundary(
    step: RecompressFrom,
    m1: &Schedule,
    dm: &[u32; 80],
    state: &[u32; 5],
) -> [u32; 5] {
    let me2 = &Xor { m1, dm };

    let [mut a, mut b, mut c, mut d, mut e] = *state;
    match step {
        RecompressFrom::Step58 => {
            step!(maj, K[2], a, b, c, d, e, me2.at(58));
            step!(maj, K[2], e, a, b, c, d, me2.at(59));
            [d, e, a, b, c]
        }
        RecompressFrom::Step65 => {
            unstep!(parity, K[3], a, b, c, d, e, me2.at(64));
            [b, c, d, e, a]
        }
    }
}

/// The recompression is the compression itself, run out from a state partway
/// through rather than from the ends, so it can be checked against the
/// compression without a second implementation of it and without a collision
/// to hand.
///
/// `quickcheck` needs `std`, so a `no_std` build skips this.
#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;
    use crate::ubc_check::SHA1_DVS;
    use quickcheck::QuickCheck;

    /// Running the compression over the partner schedule, from the chaining
    /// value the recompression recovered, must land on the one it reports.
    ///
    /// That pins both directions at once. The way back settles `ihvin`, and a
    /// wrong one starts this compression somewhere else; the way out settles
    /// what is added to it, and a wrong one lands somewhere else. Either way
    /// the two disagree.
    ///
    /// Any state at all will do, and none of this has to come from a real
    /// block: the steps are invertible, so whatever `state` is, it is the
    /// state some message reaches.
    #[test]
    fn the_recompression_agrees_with_the_compression() {
        fn prop(w: [u32; 80], state: [u32; 5]) -> bool {
            let m1 = Schedule::from_words(w);
            SHA1_DVS.iter().all(|dv| {
                let (mut ihvin, mut ihvout) = ([0u32; 5], [0u32; 5]);
                recompression_step(
                    dv.recompress_from,
                    &mut ihvin,
                    &mut ihvout,
                    &m1,
                    &dv.dm,
                    &state,
                );

                let mut me2 = Schedule::zeroed();
                for t in 0..80 {
                    me2[t] = m1[t] ^ dv.dm[t];
                }

                let mut replayed = ihvin;
                compression_w(&mut replayed, &me2);
                replayed == ihvout
            })
        }
        QuickCheck::new()
            .tests(500)
            .quickcheck(prop as fn([u32; 80], [u32; 5]) -> bool);
    }

    /// [`partner_boundary`] must be the state the partner's compression
    /// reaches at the boundary, from the chaining value the way back finds.
    #[test]
    fn the_partner_boundary_is_on_the_partners_way() {
        fn prop(w: [u32; 80], state: [u32; 5]) -> bool {
            let m1 = Schedule::from_words(w);
            SHA1_DVS.iter().all(|dv| {
                let (mut ihvin, mut ihvout) = ([0u32; 5], [0u32; 5]);
                recompression_step(
                    dv.recompress_from,
                    &mut ihvin,
                    &mut ihvout,
                    &m1,
                    &dv.dm,
                    &state,
                );

                let mut me2 = Schedule::zeroed();
                for t in 0..80 {
                    me2[t] = m1[t] ^ dv.dm[t];
                }
                let at = match dv.recompress_from {
                    RecompressFrom::Step58 => 60,
                    RecompressFrom::Step65 => 64,
                };
                partner_boundary(dv.recompress_from, &m1, &dv.dm, &state)
                    == state_at(&ihvin, &me2, at)
            })
        }
        QuickCheck::new()
            .tests(500)
            .quickcheck(prop as fn([u32; 80], [u32; 5]) -> bool);
    }
}
