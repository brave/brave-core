//! The unavoidable-bitconditions check: a cheap filter for whether a block
//! could be part of a collision attack.
//!
//! # What it does
//!
//! Every SHA-1 collision attack practical with current cryptanalysis is
//! believed to use one of 32 known *disturbance vectors* (DVs), as selected by
//! Stevens and Shumow (2017). Each DV makes certain bits of the expanded
//! message relate in a fixed way. [`ubc_check`] tests those relations and
//! returns a mask. A set bit means that the DV is still possible for this
//! block.
//!
//! A zero mask rejects all 32 DV attack classes. About 95% of blocks give
//! one. A non-zero mask sends the block to the recompression check in
//! [`crate::block`], which is much slower but gives a definite answer.
//! This check is a filter, not a decision. A false positive costs time.
//! Assuming the paper's Conjecture 3 (that an attack following a DV must use
//! its prescribed local collisions over message steps 35 to 64) a false
//! negative is impossible: this filter never rejects a block that the
//! recompression check would flag.
//!
//! # Why it looks the way it does
//!
//! Every check has the shape
//!
//! ```text
//! mask &= <one bit condition> | !(<the DVs that condition rules out>);
//! ```
//!
//! It tests one bit of `w[i]` against one bit of `w[j]` and clears the DVs
//! that the result rules out. A check only clears bits, so the order of the
//! checks does not matter.
//!
//! For one DV the published checks are one basis of a linear space, not a
//! fixed list. Any basis with the same span gives the same mask, so
//! `codegen/` is free to pick a different one. It solves once per target,
//! because each wants a different basis: a check costs one statement in
//! [`scalar`] but a quarter of a load pair in [`neon`] and [`sse2`], and an
//! eighth in [`avx2`]. Every target therefore gets its own module, holding
//! its own whole check.
//!
//! Inside a module the checks are in two parts. The prefix runs on every
//! block. What it costs depends on the core: next to nothing where the core
//! overlaps it with the compression, as Zen 4 and the ARM cores measured do,
//! but most of detection's cost on a Sapphire Rapids Xeon, which does not.
//! The tail runs the rest one at a time behind guards, because by then the
//! mask is sparse and a guard that skips a check is worth more than the check
//! costs. Where the split falls is what each target solves for.
//!
//! # Provenance
//!
//! Both tables are the paper's. The conditions are its Table 4 and Table 5,
//! in `codegen/src/ubc.rs`, which `codegen/src/solve.rs` then rebases. The DV
//! table is not stored at all: [`SHA1_DVS`] is built from the two families of
//! section 4.4 and the local-collision sum of section 4.3, so the only data
//! here is one 16-word seed per family.
//!
//! Marc Stevens and Dan Shumow also wrote
//! [sha1collisiondetection](https://github.com/cr-marcstevens/sha1collisiondetection),
//! the implementation that accompanies the paper. It is licensed under MIT.
//!
//! The tests compare this module against that original. `codegen/` has it as
//! a git submodule, pinned to a commit, runs its `lib/ubc_check.c` to read
//! its rules off, checks them against it, and writes them to `upstream.rs`,
//! so the tests need no C. Every form must give the rules' mask on a million
//! random schedules, and next to a block upstream keeps each DV on;
//! `dv_table_matches_upstream` compares the derived DV table entry for entry.

use crate::Schedule;

const DV_I_43_0_BIT: u32 = 1 << 0;
const DV_I_44_0_BIT: u32 = 1 << 1;
const DV_I_45_0_BIT: u32 = 1 << 2;
const DV_I_46_0_BIT: u32 = 1 << 3;
const DV_I_46_2_BIT: u32 = 1 << 4;
const DV_I_47_0_BIT: u32 = 1 << 5;
const DV_I_47_2_BIT: u32 = 1 << 6;
const DV_I_48_0_BIT: u32 = 1 << 7;
const DV_I_48_2_BIT: u32 = 1 << 8;
const DV_I_49_0_BIT: u32 = 1 << 9;
const DV_I_49_2_BIT: u32 = 1 << 10;
const DV_I_50_0_BIT: u32 = 1 << 11;
const DV_I_50_2_BIT: u32 = 1 << 12;
const DV_I_51_0_BIT: u32 = 1 << 13;
const DV_I_51_2_BIT: u32 = 1 << 14;
const DV_I_52_0_BIT: u32 = 1 << 15;
const DV_II_45_0_BIT: u32 = 1 << 16;
const DV_II_46_0_BIT: u32 = 1 << 17;
const DV_II_46_2_BIT: u32 = 1 << 18;
const DV_II_47_0_BIT: u32 = 1 << 19;
const DV_II_48_0_BIT: u32 = 1 << 20;
const DV_II_49_0_BIT: u32 = 1 << 21;
const DV_II_49_2_BIT: u32 = 1 << 22;
const DV_II_50_0_BIT: u32 = 1 << 23;
const DV_II_50_2_BIT: u32 = 1 << 24;
const DV_II_51_0_BIT: u32 = 1 << 25;
const DV_II_51_2_BIT: u32 = 1 << 26;
const DV_II_52_0_BIT: u32 = 1 << 27;
const DV_II_53_0_BIT: u32 = 1 << 28;
const DV_II_54_0_BIT: u32 = 1 << 29;
const DV_II_55_0_BIT: u32 = 1 << 30;
const DV_II_56_0_BIT: u32 = 1 << 31;

/// Disturbance Vector (DV).
#[derive(Clone, Copy)]
pub(crate) struct Info {
    /// Which stored intermediate state this DV's recompression starts from.
    ///
    /// `testt` in the C original.
    pub(crate) recompress_from: RecompressFrom,
    /// This DV's bit in the mask [`ubc_check`] returns.
    ///
    /// `maskb` in the C original.
    pub(crate) mask_bit: i32,
    /// The expanded message block XOR-difference defined by the DV.
    pub(crate) dm: [u32; 80],
}

/// The compression step a DV's recompression starts from.
///
/// Detection runs the compression again from a stored intermediate state and
/// not from the start. The DV selects which state applies. The C original
/// calls this `testt` and uses these step numbers as its values.
#[derive(Copy, Clone)]
#[repr(u32)]
pub(crate) enum RecompressFrom {
    Step58 = 58,
    Step65 = 65,
}

/// Which family of disturbance vector a DV belongs to.
#[derive(Clone, Copy, PartialEq)]
enum DvType {
    I,
    II,
}

/// The two base message differences, one per DV family, as their first 16
/// words. [`expand`] derives the rest.
///
/// Each seed is the difference for the *highest* K in its family. The others
/// are the same sequence at a later offset, so one seed covers all sixteen.
const TYPE_I_SEED: [u32; 16] = [
    0x04000010, 0xe8000000, 0x0800000c, 0x18000000, 0xb800000a, 0xc8000010, 0x2c000010, 0xf4000014,
    0xb4000008, 0x08000000, 0x9800000c, 0xd8000010, 0x08000010, 0xb8000010, 0x98000000, 0x60000000,
];
const TYPE_II_SEED: [u32; 16] = [
    0x2600001a, 0x00000010, 0x0400001c, 0xcc000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x3c000004,
    0xbc00001a, 0x20000010, 0x2400001c, 0xec000014, 0x0c000002, 0xc0000010, 0xb400001c, 0x2c000004,
];

/// The K each seed above is the difference for.
const TYPE_I_K: u32 = 52;
const TYPE_II_K: u32 = 56;

/// Runs SHA-1's message expansion over a seed.
///
/// A message difference obeys the same recurrence as the schedule, so only
/// the first 16 words are data. 96 words cover every offset in a family.
const fn expand(seed: [u32; 16]) -> [u32; 96] {
    let mut w = [0u32; 96];
    let mut t = 0;
    while t < 16 {
        w[t] = seed[t];
        t += 1;
    }
    while t < 96 {
        w[t] = (w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16]).rotate_left(1);
        t += 1;
    }
    w
}

const TYPE_I_DM: [u32; 96] = expand(TYPE_I_SEED);
const TYPE_II_DM: [u32; 96] = expand(TYPE_II_SEED);

/// The message difference for one DV. This is the sequence of its family, read
/// from the offset that puts the disturbance at step `k` and rotated to bit
/// `b`.
const fn message_difference(family: DvType, k: u32, b: u32) -> [u32; 80] {
    let (base, base_k) = match family {
        DvType::I => (&TYPE_I_DM, TYPE_I_K),
        DvType::II => (&TYPE_II_DM, TYPE_II_K),
    };
    let offset = (base_k - k) as usize;

    let mut dm = [0u32; 80];
    let mut i = 0;
    while i < 80 {
        dm[i] = base[i + offset].rotate_left(b);
        i += 1;
    }
    dm
}

/// The DVs to check. Each row gives the family, the step K of the disturbance,
/// and the bit B in that step. The last column is the stored state that its
/// recompression starts from.
///
/// The C original writes out all 80 words of every difference. That is 500
/// lines of hex that a reader cannot verify. The two seeds above give the same
/// values, and `dv_table_matches_upstream` compares the result against them.
const DVS: [(DvType, u32, u32, RecompressFrom); 32] = [
    (DvType::I, 43, 0, RecompressFrom::Step58),
    (DvType::I, 44, 0, RecompressFrom::Step58),
    (DvType::I, 45, 0, RecompressFrom::Step58),
    (DvType::I, 46, 0, RecompressFrom::Step58),
    (DvType::I, 46, 2, RecompressFrom::Step58),
    (DvType::I, 47, 0, RecompressFrom::Step58),
    (DvType::I, 47, 2, RecompressFrom::Step58),
    (DvType::I, 48, 0, RecompressFrom::Step58),
    (DvType::I, 48, 2, RecompressFrom::Step58),
    (DvType::I, 49, 0, RecompressFrom::Step58),
    (DvType::I, 49, 2, RecompressFrom::Step58),
    (DvType::I, 50, 0, RecompressFrom::Step65),
    (DvType::I, 50, 2, RecompressFrom::Step65),
    (DvType::I, 51, 0, RecompressFrom::Step65),
    (DvType::I, 51, 2, RecompressFrom::Step65),
    (DvType::I, 52, 0, RecompressFrom::Step65),
    (DvType::II, 45, 0, RecompressFrom::Step58),
    (DvType::II, 46, 0, RecompressFrom::Step58),
    (DvType::II, 46, 2, RecompressFrom::Step58),
    (DvType::II, 47, 0, RecompressFrom::Step58),
    (DvType::II, 48, 0, RecompressFrom::Step58),
    (DvType::II, 49, 0, RecompressFrom::Step58),
    (DvType::II, 49, 2, RecompressFrom::Step58),
    (DvType::II, 50, 0, RecompressFrom::Step65),
    (DvType::II, 50, 2, RecompressFrom::Step65),
    (DvType::II, 51, 0, RecompressFrom::Step65),
    (DvType::II, 51, 2, RecompressFrom::Step65),
    (DvType::II, 52, 0, RecompressFrom::Step65),
    (DvType::II, 53, 0, RecompressFrom::Step65),
    (DvType::II, 54, 0, RecompressFrom::Step65),
    (DvType::II, 55, 0, RecompressFrom::Step65),
    (DvType::II, 56, 0, RecompressFrom::Step65),
];

/// The list of SHA-1 Disturbance Vectors (DV) to check.
pub(crate) const SHA1_DVS: [Info; 32] = build_dvs();

/// The candidate bits whose DVs recompress from step 58.
///
/// State recovery on a hardware backend reaches step 65 first and step 58
/// only with seven further steps, so it asks this whether the extra steps
/// are wanted.
pub(crate) const STEP58_MASK: u32 = build_step58_mask();

const fn build_step58_mask() -> u32 {
    let mut mask = 0;
    let mut i = 0;
    while i < DVS.len() {
        if DVS[i].3 as u32 == RecompressFrom::Step58 as u32 {
            mask |= 1 << i;
        }
        i += 1;
    }
    mask
}

const fn build_dvs() -> [Info; 32] {
    let mut out = [Info {
        recompress_from: RecompressFrom::Step58,
        mask_bit: 0,
        dm: [0u32; 80],
    }; 32];

    let mut i = 0;
    while i < out.len() {
        let (family, k, b, recompress_from) = DVS[i];
        out[i] = Info {
            recompress_from,
            // Each DV uses the bit at its own position in this table.
            mask_bit: i as i32,
            dm: message_difference(family, k, b),
        };
        i += 1;
    }
    out
}

#[cfg(all(test, feature = "std"))]
mod upstream;

mod scalar;

#[cfg(all(target_arch = "aarch64", target_feature = "neon"))]
mod neon;

#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    target_feature = "sse2"
))]
mod sse2;

#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    target_feature = "sse2",
    any(feature = "std", target_feature = "avx2")
))]
mod avx2;

/// Whether this CPU has AVX2, which no target guarantees.
#[cfg(all(
    any(target_arch = "x86", target_arch = "x86_64"),
    target_feature = "sse2",
    any(feature = "std", target_feature = "avx2")
))]
#[inline(always)]
fn has_avx2() -> bool {
    #[cfg(feature = "std")]
    {
        std::arch::is_x86_feature_detected!("avx2")
    }
    #[cfg(not(feature = "std"))]
    {
        cfg!(target_feature = "avx2")
    }
}

/// The dispatch, written once. `$pick` is a macro applied to the form the
/// cascade picks: [`ubc_check`] passes one that runs it, the test that
/// reports which form ran passes one that names it.
macro_rules! dispatch {
    ($scalar_only:expr, $pick:ident) => {{
        #[cfg(all(target_arch = "aarch64", target_feature = "neon"))]
        if !$scalar_only {
            return $pick!(neon);
        }

        #[cfg(all(
            any(target_arch = "x86", target_arch = "x86_64"),
            target_feature = "sse2"
        ))]
        if !$scalar_only {
            #[cfg(any(feature = "std", target_feature = "avx2"))]
            if has_avx2() {
                return $pick!(avx2);
            }
            return $pick!(sse2);
        }

        // On a target with neither there is nothing to turn off.
        #[cfg(not(any(
            all(target_arch = "aarch64", target_feature = "neon"),
            all(
                any(target_arch = "x86", target_arch = "x86_64"),
                target_feature = "sse2"
            )
        )))]
        let _ = $scalar_only;

        $pick!(scalar)
    }};
}

/// Checks the unavoidable bitconditions of every listed DV against an expanded
/// message block. Returns a mask. A set bit marks a DV that met all of its
/// conditions and still needs the recompression check.
///
/// `scalar_only` keeps to [`scalar`] on a machine that has a vector unit.
/// Tests and benchmarks use it to reach that path.
#[inline]
pub(crate) fn ubc_check(w: &Schedule, scalar_only: bool) -> u32 {
    macro_rules! run {
        (neon) => {{
            // SAFETY: the cfg guarantees `neon`. All reads stay in `w`.
            unsafe { neon::check(w) }
        }};
        (avx2) => {{
            // SAFETY: just detected. All reads stay in `w`.
            unsafe { avx2::check(w) }
        }};
        (sse2) => {{
            // SAFETY: the cfg guarantees `sse2`. All reads stay in `w`.
            unsafe { sse2::check(w) }
        }};
        (scalar) => {
            scalar::check(w)
        };
    }

    dispatch!(scalar_only, run)
}

// Every test here needs `std`: for its collections, or for `quickcheck`.
#[cfg(all(test, feature = "std"))]
mod tests {
    use super::*;
    use quickcheck::{Arbitrary, Gen, QuickCheck, TestResult};
    use std::collections::BTreeMap;
    use std::string::String;
    use std::vec::Vec;

    /// Names the form that disagrees with [`scalar::check`] on `w`, if one
    /// does. Only the forms that this build has are run.
    fn diverging_form(w: &Schedule) -> Option<&'static str> {
        // A build with no vector form at all never reads this.
        #[allow(unused_variables)]
        let want = scalar::check(w);

        #[cfg(all(target_arch = "aarch64", target_feature = "neon"))]
        // SAFETY: the cfg guarantees `neon`.
        if unsafe { neon::check(w) } != want {
            return Some("neon");
        }

        #[cfg(all(
            any(target_arch = "x86", target_arch = "x86_64"),
            target_feature = "sse2"
        ))]
        // SAFETY: the cfg guarantees `sse2`.
        if unsafe { sse2::check(w) } != want {
            return Some("sse2");
        }

        #[cfg(all(
            any(target_arch = "x86", target_arch = "x86_64"),
            target_feature = "sse2",
            any(feature = "std", target_feature = "avx2")
        ))]
        if has_avx2() {
            // SAFETY: just detected.
            if unsafe { avx2::check(w) } != want {
                return Some("avx2");
            }
        }

        None
    }

    /// Names the form the dispatch picks. It runs the same cascade, so it
    /// cannot name one [`ubc_check`] would not have run.
    #[cfg(feature = "std")]
    fn selected_form() -> &'static str {
        macro_rules! name {
            ($form:ident) => {
                stringify!($form)
            };
        }

        dispatch!(false, name)
    }

    /// `SHA1DC_EXPECT_UBC_CHECK` lets a job say which form it is there to
    /// cover. `avx2` is the only one chosen at run time, and `sse2` is never
    /// chosen where `avx2` exists, so without this either can go uncovered
    /// while the job looks the same.
    #[cfg(feature = "std")]
    #[test]
    fn the_expected_implementation_was_selected() {
        let expected = std::env::var("SHA1DC_EXPECT_UBC_CHECK").unwrap_or_default();
        let expected = expected.trim();
        if expected.is_empty() {
            return; // a job that does not pin one leaves it empty
        }
        assert_eq!(
            selected_form(),
            expected,
            "this job was meant to exercise a different implementation of `ubc_check`"
        );
    }

    /// Bit `k` of a schedule: bit `k % 32` of step `k / 32`.
    fn bit(w: &Schedule, k: usize) -> u32 {
        w[k / 32] >> (k % 32) & 1
    }

    fn flip(w: &Schedule, bits: &[usize]) -> Schedule {
        let mut v = w.clone();
        for &k in bits {
            v[k / 32] ^= 1 << (k % 32);
        }
        v
    }

    /// Upstream's mask: a DV survives exactly where all its rules hold.
    ///
    /// `upstream.rs` holds the rules, which `codegen/` read off upstream's C
    /// and checked against it before writing them. Upstream tested its own
    /// generated check the same way: against a plain per-DV list of
    /// conditions, over many random schedules.
    fn upstream_mask(w: &Schedule) -> u32 {
        upstream::RULES.iter().fold(!0, |mask, &(dv, u, v, c)| {
            if bit(w, u.into()) ^ bit(w, v.into()) == u32::from(c) {
                mask
            } else {
                mask & !(1 << dv)
            }
        })
    }

    /// What gives a different mask from upstream on `w`, if anything: a form
    /// this build has, or [`ubc_check`] either way `scalar_only` can be set.
    fn diverges_from_upstream(w: &Schedule) -> Option<String> {
        let want = upstream_mask(w);
        let got = scalar::check(w);
        if got != want {
            return Some(std::format!(
                "scalar gives {got:#010x}, upstream {want:#010x}"
            ));
        }
        if let Some(form) = diverging_form(w) {
            return Some(std::format!(
                "{form} differs from upstream, which gives {want:#010x}"
            ));
        }
        for (scalar_only, path) in [
            (false, "the dispatched form"),
            (true, "the scalar-only path"),
        ] {
            let got = ubc_check(w, scalar_only);
            if got != want {
                return Some(std::format!(
                    "{path} gives {got:#010x}, upstream {want:#010x}"
                ));
            }
        }
        None
    }

    fn assert_matches_upstream(w: &Schedule) {
        if let Some(what) = diverges_from_upstream(w) {
            panic!("{what}");
        }
    }

    /// The tied groups of `dv`: its rules that share their first bit tie
    /// their second bits to it.
    fn groups(dv: usize) -> Vec<Vec<usize>> {
        let mut groups: BTreeMap<usize, Vec<usize>> = BTreeMap::new();
        for &(d, u, v, _) in &upstream::RULES {
            if usize::from(d) == dv {
                let u = usize::from(u);
                groups
                    .entry(u)
                    .or_insert_with(|| std::vec![u])
                    .push(v.into());
            }
        }
        groups.into_values().collect()
    }

    /// A schedule that upstream keeps `dv` on: arbitrary words, then each bit
    /// a rule of `dv` ties set from the bit it is tied to.
    fn kept_block(dv: usize, g: &mut Gen) -> Schedule {
        let mut w = Schedule::zeroed();
        for t in 0..crate::SCHEDULE_LEN {
            w[t] = u32::arbitrary(g);
        }
        for &(d, u, v, c) in &upstream::RULES {
            let v = usize::from(v);
            if usize::from(d) == dv && bit(&w, u.into()) ^ bit(&w, v) != u32::from(c) {
                w[v / 32] ^= 1 << (v % 32);
            }
        }
        w
    }

    /// Every part of `group` on its own: each nonempty proper subset, once
    /// per pair of complements, since flipping either is the same to every
    /// rule.
    fn parts(group: &[usize]) -> Vec<Vec<usize>> {
        (1..1u32 << (group.len() - 1))
            .map(|set| {
                let mut part = std::vec![group[0]];
                part.extend(
                    (1..group.len())
                        .filter(|&k| set >> (k - 1) & 1 == 0)
                        .map(|k| group[k]),
                );
                part
            })
            .collect()
    }

    /// FNV-1a over the little-endian bytes of `dm`, as `codegen/` hashes
    /// upstream's.
    fn dm_hash(dm: &[u32; 80]) -> u64 {
        let mut hash = 0xcbf2_9ce4_8422_2325u64;
        for byte in dm.iter().flat_map(|w| w.to_le_bytes()) {
            hash ^= u64::from(byte);
            hash = hash.wrapping_mul(0x100_0000_01b3);
        }
        hash
    }

    /// The derived DV table is upstream's, entry for entry. A wrong seed or
    /// recurrence in `expand` shows up here as a wrong difference.
    #[test]
    fn dv_table_matches_upstream() {
        for (bit, (ours, theirs)) in SHA1_DVS.iter().zip(&upstream::DVS).enumerate() {
            let &(dv_type, dv_k, dv_b, testt, maski, maskb, dm) = theirs;
            let (family, k, b, _) = DVS[bit];
            let family = match family {
                DvType::I => 1,
                DvType::II => 2,
            };
            assert_eq!(
                (family, k as i32, b as i32),
                (dv_type, dv_k, dv_b),
                "type, K and B of DV {bit}"
            );
            assert_eq!(ours.mask_bit, maskb, "mask bit of DV {bit}");
            assert_eq!(maski, 0, "mask word of DV {bit}");
            assert_eq!(
                ours.recompress_from as i32, testt,
                "recompression step of DV {bit}"
            );
            assert_eq!(dm_hash(&ours.dm), dm, "message difference of DV {bit}");
        }
    }

    /// Every form gives upstream's mask on random schedules, drawn the way
    /// upstream's own test draws them: expanded from random messages, fresh
    /// on every run. A failure is shrunk to a small message.
    ///
    /// `SHA1DC_UPSTREAM_SCHEDULES` runs more, for a long local run;
    /// upstream's test ran 2^24.
    #[test]
    fn every_form_matches_upstream() {
        fn prop(m: [u32; 16]) -> TestResult {
            match diverges_from_upstream(&Schedule::expand(&m)) {
                None => TestResult::passed(),
                Some(what) => TestResult::error(what),
            }
        }
        let n = std::env::var("SHA1DC_UPSTREAM_SCHEDULES")
            .map(|n| {
                n.parse()
                    .expect("SHA1DC_UPSTREAM_SCHEDULES must be a number")
            })
            .unwrap_or(1_000_000);
        // `max_tests` caps the cases tried, and defaults to far fewer.
        QuickCheck::new()
            .tests(n)
            .max_tests(n)
            .quickcheck(prop as fn([u32; 16]) -> TestResult);
    }

    /// Every form gives upstream's mask next to a block that keeps each DV:
    /// with each bit flipped, with each tied group flipped whole, which keeps
    /// the DV, and with each part of each group flipped, which rules it out.
    ///
    /// A random schedule keeps a given DV between once in 128 and once in
    /// about 50,000 tries, so the stream above reaches the rarest DVs only a
    /// few dozen times. These reach every one, and every one of its
    /// conditions from both sides.
    #[test]
    fn every_form_matches_upstream_next_to_every_dv() {
        let mut g = Gen::new(100);
        for dv in 0..32 {
            let base = kept_block(dv, &mut g);
            let groups = groups(dv);
            assert!(!groups.is_empty(), "DV {dv} has no rules");
            assert_ne!(upstream_mask(&base) >> dv & 1, 0, "the block keeps DV {dv}");

            assert_matches_upstream(&base);
            for k in 0..crate::SCHEDULE_LEN * 32 {
                assert_matches_upstream(&flip(&base, &[k]));
            }
            for group in &groups {
                let whole = flip(&base, group);
                assert_ne!(
                    upstream_mask(&whole) >> dv & 1,
                    0,
                    "a whole group of DV {dv}"
                );
                assert_matches_upstream(&whole);
                for part in parts(group) {
                    let part = flip(&base, &part);
                    assert_eq!(
                        upstream_mask(&part) >> dv & 1,
                        0,
                        "a part of a group of DV {dv}"
                    );
                    assert_matches_upstream(&part);
                }
            }
        }
    }

    /// A property test on words no message could produce.
    mod properties {
        use super::*;
        use quickcheck::QuickCheck;

        /// The forms share no code, so they must agree on any words at all,
        /// not only on a real expansion. An expansion correlates its words,
        /// which can hide a form that reads the wrong one; the stream above
        /// draws only expansions.
        #[test]
        fn forms_agree_on_arbitrary_words() {
            fn prop(w: [u32; 80]) -> bool {
                diverging_form(&Schedule::from_words(w)).is_none()
            }
            QuickCheck::new()
                .tests(2_000)
                .quickcheck(prop as fn([u32; 80]) -> bool);
        }
    }
}
