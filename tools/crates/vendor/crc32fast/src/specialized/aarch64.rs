use core::arch::aarch64 as arch;

#[derive(Clone)]
pub struct State(());

impl State {
    #[cfg(not(feature = "std"))]
    pub fn new() -> Option<Self> {
        if cfg!(target_feature = "crc") {
            // SAFETY: The conditions above ensure that all
            //         required instructions are supported by the CPU.
            Some(Self(()))
        } else {
            None
        }
    }

    #[cfg(feature = "std")]
    pub fn new() -> Option<Self> {
        if std::arch::is_aarch64_feature_detected!("crc") {
            // SAFETY: The condition above ensures that all
            //         required instructions are supported by the CPU.
            Some(Self(()))
        } else {
            None
        }
    }

    pub fn update(&self, state: u32, buf: &[u8]) -> u32 {
        // SAFETY: Calling a `#[target_feature]` function is only unsafe because the CPU might lack
        // that feature. A `State` only exists if `new` found the `crc` feature, and
        // `calculate_pmull` (which also needs `aes`, for `PMULL`) is only called once `has_pmull`
        // found that too.
        //
        // Checking `aes` here rather than storing it from `new` keeps `State` zero-sized, which
        // makes `hash()` of 8 to 48 bytes, which builds a `Hasher` each call, 18-35% faster on an
        // Apple M4 (and `#[inline]` here undoes it). The short inputs go to the `crc32` chains
        // from here, too: a second call through `calculate_pmull`, which can't be inlined into
        // this function, costs them 20-30%.
        unsafe {
            if buf.len() >= MIN_PMULL_BYTES && has_pmull() {
                calculate_pmull(state, buf)
            } else {
                calculate(state, buf)
            }
        }
    }
}

/// Whether the CPU has `PMULL` (part of the `aes` feature).
#[inline]
fn has_pmull() -> bool {
    #[cfg(feature = "std")]
    {
        std::arch::is_aarch64_feature_detected!("aes")
    }
    #[cfg(not(feature = "std"))]
    {
        cfg!(target_feature = "aes")
    }
}

// Bytes per stream in the 3-way loops. Merging the streams appends zero bytes to the earlier CRC
// states, which for a fixed length is a multiplication by a constant: four lookups in a table built
// at compile time. Shorter blocks would help cores with a slow `crc32x` (Apple M1), but cost a
// little on cores where one chain already keeps up (Neoverse N2), so the 3-way loop starts at
// 3 * SHORT = 1.5 KiB, as before.
const LONG: usize = 32768;
const SHORT: usize = 512;

// The 3-way loop advances each chain four words (32 bytes) at a time, so blocks leave no remainder.
const _: () = assert!(LONG % 32 == 0 && SHORT % 32 == 0);

static LONG_SHIFT: [[u32; 256]; 4] = crate::combine::shift_table(LONG as u64);
static SHORT_SHIFT: [[u32; 256]; 4] = crate::combine::shift_table(SHORT as u64);

/// Reads the `i`-th 8-byte word of `b` as a little-endian `u64`, the byte order `crc32x` expects.
#[inline(always)]
fn quad(b: &[u8], i: usize) -> u64 {
    u64::from_le_bytes(b[i * 8..i * 8 + 8].try_into().unwrap())
}

// target_feature is necessary to allow rustc to inline the crc32* wrappers
#[target_feature(enable = "crc")]
unsafe fn calculate(crc: u32, data: &[u8]) -> u32 {
    // Split the input into unaligned leading bytes, 8-byte-aligned words, and trailing bytes.
    let head = data.as_ptr().align_offset(8).min(data.len());
    let (pre_quad, rest) = data.split_at(head);
    let (mut quads, post_quad) = rest.split_at(rest.len() / 8 * 8);

    // Unaligned leading bytes, then switch to the internal (bit-inverted) state.
    let mut c0 = !crc_bytes(crc, pre_quad);

    // A single `crc32d` chain is latency-bound: each instruction depends on the previous result.
    // While enough words remain, advance three independent chains over three consecutive blocks of
    // `n` bytes. B and C start from a zero state, so appending them to A is linear: shift A's state
    // past B's bytes and XOR in B's state, then the same for C.
    for &(n, shift) in &[(LONG, &LONG_SHIFT), (SHORT, &SHORT_SHIFT)] {
        while quads.len() >= 3 * n {
            let (a, rest) = quads.split_at(n);
            let (b, rest) = rest.split_at(n);
            let (c, rest) = rest.split_at(n);
            quads = rest;

            let mut c1 = 0;
            let mut c2 = 0;
            for ((qa, qb), qc) in a
                .chunks_exact(32)
                .zip(b.chunks_exact(32))
                .zip(c.chunks_exact(32))
            {
                for k in 0..4 {
                    c0 = arch::__crc32d(c0, quad(qa, k));
                    c1 = arch::__crc32d(c1, quad(qb, k));
                    c2 = arch::__crc32d(c2, quad(qc, k));
                }
            }
            c0 = crate::combine::shift(shift, c0) ^ c1;
            c0 = crate::combine::shift(shift, c0) ^ c2;
        }
    }

    // Remaining words, then unaligned trailing bytes.
    let r = crc_quads(!c0, quads);
    crc_bytes(r, post_quad)
}

/// CRCs a byte slice whose length is a multiple of 8, a word at a time.
#[target_feature(enable = "crc")]
unsafe fn crc_quads(crc: u32, quads: &[u8]) -> u32 {
    let mut c = !crc;

    // unrolling increases performance by a lot
    let mut quad_iter = quads.chunks_exact(64);
    for chunk in &mut quad_iter {
        c = arch::__crc32d(c, quad(chunk, 0));
        c = arch::__crc32d(c, quad(chunk, 1));
        c = arch::__crc32d(c, quad(chunk, 2));
        c = arch::__crc32d(c, quad(chunk, 3));
        c = arch::__crc32d(c, quad(chunk, 4));
        c = arch::__crc32d(c, quad(chunk, 5));
        c = arch::__crc32d(c, quad(chunk, 6));
        c = arch::__crc32d(c, quad(chunk, 7));
    }
    c = quad_iter
        .remainder()
        .chunks_exact(8)
        .fold(c, |acc, q| arch::__crc32d(acc, quad(q, 0)));

    !c
}

#[target_feature(enable = "crc")]
unsafe fn crc_bytes(crc: u32, bytes: &[u8]) -> u32 {
    let c = bytes.iter().fold(!crc, |acc, &b| arch::__crc32b(acc, b));
    !c
}

use crate::combine::fold_keys;

const FOLD_128: [u64; 2] = fold_keys(128);
const FOLD_256: [u64; 2] = fold_keys(256);
const FOLD_512: [u64; 2] = fold_keys(512);
const FOLD_1024: [u64; 2] = fold_keys(1024);
const FOLD_2048: [u64; 2] = fold_keys(2048);

/// Below this, the `crc32` chains are faster than folding.
const MIN_PMULL_BYTES: usize = 64;

/// Folds `data` with as many 128-bit streams as pay for their setup and collapse at its length:
/// each fold of a stream waits for the last, so more streams keep more `PMULL`s in flight. The
/// crossovers are an Apple M4's. `data` holds at least `MIN_PMULL_BYTES`.
#[target_feature(enable = "crc", enable = "aes")]
unsafe fn calculate_pmull(crc: u32, data: &[u8]) -> u32 {
    debug_assert!(data.len() >= MIN_PMULL_BYTES);
    match data.len() {
        0..=255 => fold_streams::<4>(crc, data, FOLD_512),
        256..=4095 => fold_streams::<8>(crc, data, FOLD_1024),
        _ => fold_streams::<16>(crc, data, FOLD_2048),
    }
}

/// `N` streams of 16 bytes each are folded forward by `16 * N` bytes (`k`) with carry-less
/// multiplies until fewer than that remain, then halved (each fold of a half into the other a
/// level of latency, not a fold of every stream in turn) down to one, absorbing what remains of
/// the input on the way. What is left is 16 bytes whose CRC (from a zero state), followed by the
/// last 0 to 15 bytes, is the CRC of the whole. `data` holds at least `16 * N` bytes, and `N` is
/// 4, 8 or 16.
#[target_feature(enable = "crc", enable = "aes")]
#[inline]
unsafe fn fold_streams<const N: usize>(crc: u32, mut data: &[u8], k: [u64; 2]) -> u32 {
    let mut v = [arch::vdupq_n_u64(0); N];
    for x in v.iter_mut() {
        *x = load(&mut data);
    }
    // The CRC state (inverted, the internal form) goes into the first 4 bytes.
    v[0] = arch::veorq_u64(
        v[0],
        arch::vsetq_lane_u64::<0>(!crc as u64, arch::vdupq_n_u64(0)),
    );

    let k = keys(k);
    while data.len() >= 16 * N {
        for x in v.iter_mut() {
            *x = fold(*x, load(&mut data), k);
        }
    }

    // Fewer than `16 * N` bytes remain. Halve `2h` streams to `h` by folding each of the first
    // half `h` blocks forward into its partner, then let the `h` take `16 * h` more bytes if
    // there are that many: fewer than `16 * h` remain after each level. The levels are written
    // out: as a loop, the streams no longer stay in registers.
    macro_rules! halve {
        ($h:expr, $keys:expr) => {
            if N >= 2 * $h {
                let k = keys($keys);
                for i in 0..$h {
                    v[i] = fold(v[i], v[i + $h], k);
                }
                if data.len() >= 16 * $h {
                    for x in v[..$h].iter_mut() {
                        *x = fold(*x, load(&mut data), k);
                    }
                }
            }
        };
    }
    halve!(8, FOLD_1024);
    halve!(4, FOLD_512);
    halve!(2, FOLD_256);
    halve!(1, FOLD_128);

    let c = arch::__crc32d(0, arch::vgetq_lane_u64::<0>(v[0]));
    let c = arch::__crc32d(c, arch::vgetq_lane_u64::<1>(v[0]));
    !crc_tail(c, data)
}

/// Advances the internal (inverted) state `c` over the last 0 to 15 bytes: at most one each of
/// `crc32x`, `crc32w`, `crc32h` and `crc32b`, rather than a `crc32b` for each.
#[target_feature(enable = "crc")]
#[inline]
unsafe fn crc_tail(mut c: u32, mut data: &[u8]) -> u32 {
    // Often there's none: without this, the 4-stream kernel runs 64 bytes 28% slower.
    if data.is_empty() {
        return c;
    }
    if data.len() >= 8 {
        c = arch::__crc32d(c, quad(data, 0));
        data = &data[8..];
    }
    if data.len() >= 4 {
        c = arch::__crc32w(c, u32::from_le_bytes(data[..4].try_into().unwrap()));
        data = &data[4..];
    }
    if data.len() >= 2 {
        c = arch::__crc32h(c, u16::from_le_bytes(data[..2].try_into().unwrap()));
        data = &data[2..];
    }
    if let Some(&b) = data.first() {
        c = arch::__crc32b(c, b);
    }
    c
}

/// The two fold constants as a vector, low lane first.
#[inline(always)]
unsafe fn keys(k: [u64; 2]) -> arch::uint64x2_t {
    arch::vld1q_u64(k.as_ptr())
}

/// Folds `a` forward by the distance `k` is for and adds it to `b`: `a.lo * k.lo + a.hi * k.hi +
/// b`, carry-less.
#[target_feature(enable = "aes")]
#[inline]
unsafe fn fold(a: arch::uint64x2_t, b: arch::uint64x2_t, k: arch::uint64x2_t) -> arch::uint64x2_t {
    let lo = arch::vmull_p64(arch::vgetq_lane_u64::<0>(a), arch::vgetq_lane_u64::<0>(k));
    let hi = arch::vmull_high_p64(
        arch::vreinterpretq_p64_u64(a),
        arch::vreinterpretq_p64_u64(k),
    );
    arch::veorq_u64(
        b,
        arch::veorq_u64(
            arch::vreinterpretq_u64_p128(lo),
            arch::vreinterpretq_u64_p128(hi),
        ),
    )
}

/// Loads the next 16 bytes of `a`, little-endian lanes, and advances `a` past them.
#[inline(always)]
unsafe fn load(a: &mut &[u8]) -> arch::uint64x2_t {
    // Indexing `a[..16]` first makes a miscounted length panic instead of reading out of bounds;
    // the callers' loop conditions imply it, so the check compiles away.
    let r = arch::vreinterpretq_u64_u8(arch::vld1q_u8(a[..16].as_ptr()));
    *a = &a[16..];
    r
}

#[cfg(test)]
mod test {
    /// Every implementation this CPU supports, to test each: the `crc32` chains, and `State`, which
    /// folds with `PMULL` when the CPU has it.
    fn kernels() -> Vec<fn(u32, &[u8]) -> u32> {
        assert!(super::State::new().is_some(), "not supported");
        vec![
            // SAFETY: `State::new` found the `crc` feature.
            |c, d| unsafe { super::calculate(c, d) },
            |c, d| super::State::new().unwrap().update(c, d),
        ]
    }

    /// Set by CI, on runners whose CPU features it knows, to the name of the kernel detection
    /// should pick: "none" (no `crc` feature), "crc" (the `crc32` chains only) or "pmull" (folding
    /// with `PMULL` for longer inputs, too).
    const EXPECT: &str = "CRC32FAST_EXPECT";

    #[test]
    fn picks_the_expected_kernel() {
        // Unset or "" on runners whose CPU CI does not pin down.
        let want = std::env::var(EXPECT).unwrap_or_default();
        if want.is_empty() {
            return;
        }
        let got = match super::State::new() {
            None => "none",
            Some(_) if super::has_pmull() => "pmull",
            Some(_) => "crc",
        };
        assert_eq!(got, want, "{EXPECT}");
    }

    quickcheck::quickcheck! {
        fn check_against_baseline(init: u32, chunks: Vec<(Vec<u8>, usize)>) -> bool {
            kernels().iter().all(|kernel| {
                let mut baseline = init;
                let mut aarch64 = init;
                for (chunk, offset) in &chunks {
                    // simulate random alignments by offsetting the slice by up to 15 bytes
                    let chunk = &chunk[(offset & 0xF).min(chunk.len())..];
                    baseline = crate::baseline::update_fast_16(baseline, chunk);
                    aarch64 = kernel(aarch64, chunk);
                }
                aarch64 == baseline
            })
        }
    }

    // Exercises the 3-way interleaved and the folding paths across the sizes and alignments where
    // they switch.
    #[test]
    fn check_large_inputs_against_baseline() {
        let mut data = vec![0u8; 8200];
        let mut s: u32 = 0x1234_5678;
        for byte in data.iter_mut() {
            s = s.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
            *byte = (s >> 24) as u8;
        }

        for kernel in kernels() {
            for &len in &[
                0usize, 1, 7, 8, 15, 16, 63, 64, 65, 79, 80, 127, 128, 129, 255, 256, 257, 271,
                272, 383, 384, 385, 1535, 1536, 1537, 4095, 4096, 4097, 4111, 4112, 4351, 4352,
                4353, 8192, 8199,
            ] {
                for &offset in &[0usize, 1, 3, 7, 8, 15] {
                    if offset + len > data.len() {
                        continue;
                    }
                    let slice = &data[offset..offset + len];
                    let baseline = crate::baseline::update_fast_16(0, slice);
                    let specialized = kernel(0, slice);

                    assert_eq!(
                        specialized, baseline,
                        "mismatch for len={len} offset={offset}",
                    );
                }
            }
        }
    }
}
