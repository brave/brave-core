//! Direct translation of the C code found at
//! [sha1.c](https://github.com/cr-marcstevens/sha1collisiondetection/blob/master/lib/sha1.c).
//!
//! For the original license and source details see the comments in `src/checked.rs`.

#![allow(clippy::many_single_char_names, clippy::too_many_arguments)]

use crate::{
    BLOCK_SIZE,
    {ubc_check::Testt, DetectionState},
};

const K: [u32; 4] = [0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6];

#[inline(always)]
fn mix(w: &mut [u32; 80], t: usize) -> u32 {
    (w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16]).rotate_left(1)
}

#[inline(always)]
fn f1(b: u32, c: u32, d: u32) -> u32 {
    d ^ b & (c ^ d)
}

#[inline(always)]
fn f2(b: u32, c: u32, d: u32) -> u32 {
    b ^ c ^ d
}

#[inline(always)]
fn f3(b: u32, c: u32, d: u32) -> u32 {
    (b & c).wrapping_add(d & (b ^ c))
}

#[inline(always)]
fn f4(b: u32, c: u32, d: u32) -> u32 {
    b ^ c ^ d
}

#[inline(always)]
fn round3_step(a: u32, b: &mut u32, c: u32, d: u32, e: &mut u32, mt: u32) {
    *e = e.wrapping_add(
        a.rotate_left(5)
            .wrapping_add(f3(*b, c, d))
            .wrapping_add(K[2])
            .wrapping_add(mt),
    );
    *b = b.rotate_left(30);
}

#[inline(always)]
fn round4_step(a: u32, b: &mut u32, c: u32, d: u32, e: &mut u32, mt: u32) {
    *e = e.wrapping_add(
        a.rotate_left(5)
            .wrapping_add(f4(*b, c, d))
            .wrapping_add(K[3])
            .wrapping_add(mt),
    );
    *b = b.rotate_left(30);
}

#[inline(always)]
fn round1_step_bw(a: u32, b: &mut u32, c: u32, d: u32, e: &mut u32, mt: u32) {
    *b = b.rotate_right(30);
    *e = e.wrapping_sub(
        a.rotate_left(5)
            .wrapping_add(f1(*b, c, d))
            .wrapping_add(K[0])
            .wrapping_add(mt),
    );
}

#[inline(always)]
fn round2_step_bw(a: u32, b: &mut u32, c: u32, d: u32, e: &mut u32, mt: u32) {
    *b = b.rotate_right(30);
    *e = e.wrapping_sub(
        a.rotate_left(5)
            .wrapping_add(f2(*b, c, d))
            .wrapping_add(K[1])
            .wrapping_add(mt),
    );
}

#[inline(always)]
fn round3_step_bw(a: u32, b: &mut u32, c: u32, d: u32, e: &mut u32, mt: u32) {
    *b = b.rotate_right(30);
    *e = e.wrapping_sub(
        a.rotate_left(5)
            .wrapping_add(f3(*b, c, d))
            .wrapping_add(K[2])
            .wrapping_add(mt),
    );
}

#[inline(always)]
fn round4_step_bw(a: u32, b: &mut u32, c: u32, d: u32, e: &mut u32, mt: u32) {
    *b = b.rotate_right(30);
    *e = e.wrapping_sub(
        a.rotate_left(5)
            .wrapping_add(f4(*b, c, d))
            .wrapping_add(K[3])
            .wrapping_add(mt),
    );
}

#[inline(always)]
fn full_round3_step(a: u32, b: &mut u32, c: u32, d: u32, e: &mut u32, w: &mut [u32; 80], t: usize) {
    w[t] = mix(w, t);
    *e = e.wrapping_add(
        w[t].wrapping_add(a.rotate_left(5))
            .wrapping_add(f3(*b, c, d))
            .wrapping_add(K[2]),
    );
    *b = b.rotate_left(30);
}

#[inline(always)]
fn full_round4_step(a: u32, b: &mut u32, c: u32, d: u32, e: &mut u32, w: &mut [u32; 80], t: usize) {
    w[t] = mix(w, t);
    *e = e.wrapping_add(
        w[t].wrapping_add(a.rotate_left(5))
            .wrapping_add(f4(*b, c, d))
            .wrapping_add(K[3]),
    );
    *b = b.rotate_left(30);
}

#[inline]
fn round2_step4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    w: &[u32; 80],
    t: usize,
) {
    // 1
    *e = e.wrapping_add(
        w[t].wrapping_add(a.rotate_left(5))
            .wrapping_add(f2(*b, *c, *d))
            .wrapping_add(K[1]),
    );
    *b = b.rotate_left(30);

    // 2
    *d = d.wrapping_add(
        w[t + 1]
            .wrapping_add(e.rotate_left(5))
            .wrapping_add(f2(*a, *b, *c))
            .wrapping_add(K[1]),
    );
    *a = a.rotate_left(30);

    // 3
    *c = c.wrapping_add(
        w[t + 2]
            .wrapping_add(d.rotate_left(5))
            .wrapping_add(f2(*e, *a, *b))
            .wrapping_add(K[1]),
    );
    *e = e.rotate_left(30);

    // 4
    *b = b.wrapping_add(
        w[t + 3]
            .wrapping_add(c.rotate_left(5))
            .wrapping_add(f2(*d, *e, *a))
            .wrapping_add(K[1]),
    );
    *d = d.rotate_left(30);
}

#[inline]
fn round3_step4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    w: &[u32; 80],
    t: usize,
) {
    // 1
    *e = e.wrapping_add(
        w[t].wrapping_add(a.rotate_left(5))
            .wrapping_add(f3(*b, *c, *d))
            .wrapping_add(K[2]),
    );
    *b = b.rotate_left(30);

    // 2
    *d = d.wrapping_add(
        w[t + 1]
            .wrapping_add(e.rotate_left(5))
            .wrapping_add(f3(*a, *b, *c))
            .wrapping_add(K[2]),
    );
    *a = a.rotate_left(30);

    // 3
    *c = c.wrapping_add(
        w[t + 2]
            .wrapping_add(d.rotate_left(5))
            .wrapping_add(f3(*e, *a, *b))
            .wrapping_add(K[2]),
    );
    *e = e.rotate_left(30);

    // 4
    *b = b.wrapping_add(
        w[t + 3]
            .wrapping_add(c.rotate_left(5))
            .wrapping_add(f3(*d, *e, *a))
            .wrapping_add(K[2]),
    );
    *d = d.rotate_left(30);
}

#[inline]
fn round4_step4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    w: &[u32; 80],
    t: usize,
) {
    // 1
    *e = e.wrapping_add(
        w[t].wrapping_add(a.rotate_left(5))
            .wrapping_add(f4(*b, *c, *d))
            .wrapping_add(K[3]),
    );
    *b = b.rotate_left(30);

    // 2
    *d = d.wrapping_add(
        w[t + 1]
            .wrapping_add(e.rotate_left(5))
            .wrapping_add(f4(*a, *b, *c))
            .wrapping_add(K[3]),
    );
    *a = a.rotate_left(30);

    // 3
    *c = c.wrapping_add(
        w[t + 2]
            .wrapping_add(d.rotate_left(5))
            .wrapping_add(f4(*e, *a, *b))
            .wrapping_add(K[3]),
    );
    *e = e.rotate_left(30);

    // 4
    *b = b.wrapping_add(
        w[t + 3]
            .wrapping_add(c.rotate_left(5))
            .wrapping_add(f4(*d, *e, *a))
            .wrapping_add(K[3]),
    );
    *d = d.rotate_left(30);
}

#[inline]
fn full_round1_step_load4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    m: &[u32; 16],
    w: &mut [u32; 80],
    t: usize,
) {
    // load
    w[t..t + 4].copy_from_slice(&m[t..t + 4]);
    round1_step4(a, b, c, d, e, w, t);
}

#[inline(always)]
fn round1_step4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    w: &[u32; 80],
    t: usize,
) {
    // 1
    *e = e.wrapping_add(
        w[t].wrapping_add(a.rotate_left(5))
            .wrapping_add(f1(*b, *c, *d))
            .wrapping_add(K[0]),
    );
    *b = b.rotate_left(30);

    // 2
    *d = d.wrapping_add(
        w[t + 1]
            .wrapping_add(e.rotate_left(5))
            .wrapping_add(f1(*a, *b, *c))
            .wrapping_add(K[0]),
    );
    *a = a.rotate_left(30);

    // 3
    *c = c.wrapping_add(
        w[t + 2]
            .wrapping_add(d.rotate_left(5))
            .wrapping_add(f1(*e, *a, *b))
            .wrapping_add(K[0]),
    );
    *e = e.rotate_left(30);

    // 4
    *b = b.wrapping_add(
        w[t + 3]
            .wrapping_add(c.rotate_left(5))
            .wrapping_add(f1(*d, *e, *a))
            .wrapping_add(K[0]),
    );
    *d = d.rotate_left(30);
}

#[inline]
fn full_round1_step_expand4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    w: &mut [u32; 80],
    t: usize,
) {
    w[t] = mix(w, t);
    w[t + 1] = mix(w, t + 1);
    w[t + 2] = mix(w, t + 2);
    w[t + 3] = mix(w, t + 3);
    round1_step4(a, b, c, d, e, w, t);
}

#[inline]
fn full_round2_step4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    w: &mut [u32; 80],
    t: usize,
) {
    w[t] = mix(w, t);
    w[t + 1] = mix(w, t + 1);
    w[t + 2] = mix(w, t + 2);
    w[t + 3] = mix(w, t + 3);
    round2_step4(a, b, c, d, e, w, t);
}

#[inline]
fn full_round3_step4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    w: &mut [u32; 80],
    t: usize,
) {
    w[t] = mix(w, t);
    w[t + 1] = mix(w, t + 1);
    w[t + 2] = mix(w, t + 2);
    w[t + 3] = mix(w, t + 3);
    round3_step4(a, b, c, d, e, w, t);
}

#[inline]
fn full_round4_step4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    w: &mut [u32; 80],
    t: usize,
) {
    w[t] = mix(w, t);
    w[t + 1] = mix(w, t + 1);
    w[t + 2] = mix(w, t + 2);
    w[t + 3] = mix(w, t + 3);
    round4_step4(a, b, c, d, e, w, t);
}

#[inline]
fn round1_step_bw4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    m: &[u32; 80],
    t: usize,
) {
    round1_step_bw(*a, b, *c, *d, e, m[t]);
    round1_step_bw(*b, c, *d, *e, a, m[t - 1]);
    round1_step_bw(*c, d, *e, *a, b, m[t - 2]);
    round1_step_bw(*d, e, *a, *b, c, m[t - 3]);
}

#[inline]
fn round2_step_bw4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    m: &[u32; 80],
    t: usize,
) {
    round2_step_bw(*a, b, *c, *d, e, m[t]);
    round2_step_bw(*b, c, *d, *e, a, m[t - 1]);
    round2_step_bw(*c, d, *e, *a, b, m[t - 2]);
    round2_step_bw(*d, e, *a, *b, c, m[t - 3]);
}

#[inline]
fn round3_step_bw4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    m: &[u32; 80],
    t: usize,
) {
    round3_step_bw(*a, b, *c, *d, e, m[t]);
    round3_step_bw(*b, c, *d, *e, a, m[t - 1]);
    round3_step_bw(*c, d, *e, *a, b, m[t - 2]);
    round3_step_bw(*d, e, *a, *b, c, m[t - 3]);
}

#[inline]
fn round4_step_bw4(
    a: &mut u32,
    b: &mut u32,
    c: &mut u32,
    d: &mut u32,
    e: &mut u32,
    m: &[u32; 80],
    t: usize,
) {
    round4_step_bw(*a, b, *c, *d, e, m[t]);
    round4_step_bw(*b, c, *d, *e, a, m[t - 1]);
    round4_step_bw(*c, d, *e, *a, b, m[t - 2]);
    round4_step_bw(*d, e, *a, *b, c, m[t - 3]);
}

fn add_assign(left: &mut [u32; 5], right: [u32; 5]) {
    left[0] = left[0].wrapping_add(right[0]);
    left[1] = left[1].wrapping_add(right[1]);
    left[2] = left[2].wrapping_add(right[2]);
    left[3] = left[3].wrapping_add(right[3]);
    left[4] = left[4].wrapping_add(right[4]);
}

fn compression_w(ihv: &mut [u32; 5], w: &[u32; 80]) {
    let [mut a, mut b, mut c, mut d, mut e] = ihv;

    round1_step4(&mut a, &mut b, &mut c, &mut d, &mut e, w, 0);
    round1_step4(&mut b, &mut c, &mut d, &mut e, &mut a, w, 4);
    round1_step4(&mut c, &mut d, &mut e, &mut a, &mut b, w, 8);
    round1_step4(&mut d, &mut e, &mut a, &mut b, &mut c, w, 12);
    round1_step4(&mut e, &mut a, &mut b, &mut c, &mut d, w, 16);

    round2_step4(&mut a, &mut b, &mut c, &mut d, &mut e, w, 20);
    round2_step4(&mut b, &mut c, &mut d, &mut e, &mut a, w, 24);
    round2_step4(&mut c, &mut d, &mut e, &mut a, &mut b, w, 28);
    round2_step4(&mut d, &mut e, &mut a, &mut b, &mut c, w, 32);
    round2_step4(&mut e, &mut a, &mut b, &mut c, &mut d, w, 36);

    round3_step4(&mut a, &mut b, &mut c, &mut d, &mut e, w, 40);
    round3_step4(&mut b, &mut c, &mut d, &mut e, &mut a, w, 44);
    round3_step4(&mut c, &mut d, &mut e, &mut a, &mut b, w, 48);
    round3_step4(&mut d, &mut e, &mut a, &mut b, &mut c, w, 52);
    round3_step4(&mut e, &mut a, &mut b, &mut c, &mut d, w, 56);

    round4_step4(&mut a, &mut b, &mut c, &mut d, &mut e, w, 60);
    round4_step4(&mut b, &mut c, &mut d, &mut e, &mut a, w, 64);
    round4_step4(&mut c, &mut d, &mut e, &mut a, &mut b, w, 68);
    round4_step4(&mut d, &mut e, &mut a, &mut b, &mut c, w, 72);
    round4_step4(&mut e, &mut a, &mut b, &mut c, &mut d, w, 76);

    add_assign(ihv, [a, b, c, d, e]);
}

fn compression_states(
    ihv: &mut [u32; 5],
    m: &[u32; 16],
    w: &mut [u32; 80],
    state_58: &mut [u32; 5],
    state_65: &mut [u32; 5],
) {
    let [mut a, mut b, mut c, mut d, mut e] = ihv;

    full_round1_step_load4(&mut a, &mut b, &mut c, &mut d, &mut e, m, w, 0);
    full_round1_step_load4(&mut b, &mut c, &mut d, &mut e, &mut a, m, w, 4);
    full_round1_step_load4(&mut c, &mut d, &mut e, &mut a, &mut b, m, w, 8);
    full_round1_step_load4(&mut d, &mut e, &mut a, &mut b, &mut c, m, w, 12);

    full_round1_step_expand4(&mut e, &mut a, &mut b, &mut c, &mut d, w, 16);

    full_round2_step4(&mut a, &mut b, &mut c, &mut d, &mut e, w, 20);
    full_round2_step4(&mut b, &mut c, &mut d, &mut e, &mut a, w, 24);
    full_round2_step4(&mut c, &mut d, &mut e, &mut a, &mut b, w, 28);
    full_round2_step4(&mut d, &mut e, &mut a, &mut b, &mut c, w, 32);
    full_round2_step4(&mut e, &mut a, &mut b, &mut c, &mut d, w, 36);

    full_round3_step4(&mut a, &mut b, &mut c, &mut d, &mut e, w, 40);
    full_round3_step4(&mut b, &mut c, &mut d, &mut e, &mut a, w, 44);
    full_round3_step4(&mut c, &mut d, &mut e, &mut a, &mut b, w, 48);
    full_round3_step4(&mut d, &mut e, &mut a, &mut b, &mut c, w, 52);

    full_round3_step(e, &mut a, b, c, &mut d, w, 56);
    full_round3_step(d, &mut e, a, b, &mut c, w, 57);

    // Store state58
    *state_58 = [a, b, c, d, e];

    full_round3_step(c, &mut d, e, a, &mut b, w, 58);
    full_round3_step(b, &mut c, d, e, &mut a, w, 59);

    full_round4_step4(&mut a, &mut b, &mut c, &mut d, &mut e, w, 60);
    full_round4_step(b, &mut c, d, e, &mut a, w, 64);

    // Store state65
    *state_65 = [a, b, c, d, e];

    full_round4_step(a, &mut b, c, d, &mut e, w, 65);
    full_round4_step(e, &mut a, b, c, &mut d, w, 66);
    full_round4_step(d, &mut e, a, b, &mut c, w, 67);

    full_round4_step4(&mut c, &mut d, &mut e, &mut a, &mut b, w, 68);
    full_round4_step4(&mut d, &mut e, &mut a, &mut b, &mut c, w, 72);
    full_round4_step4(&mut e, &mut a, &mut b, &mut c, &mut d, w, 76);

    add_assign(ihv, [a, b, c, d, e]);
}

fn recompress_fast_58(
    ihvin: &mut [u32; 5],
    ihvout: &mut [u32; 5],
    me2: &[u32; 80],
    state: &[u32; 5],
) {
    let [mut a, mut b, mut c, mut d, mut e] = state;

    round3_step_bw(d, &mut e, a, b, &mut c, me2[57]);
    round3_step_bw(e, &mut a, b, c, &mut d, me2[56]);

    round3_step_bw4(&mut a, &mut b, &mut c, &mut d, &mut e, me2, 55);
    round3_step_bw4(&mut e, &mut a, &mut b, &mut c, &mut d, me2, 51);
    round3_step_bw4(&mut d, &mut e, &mut a, &mut b, &mut c, me2, 47);
    round3_step_bw4(&mut c, &mut d, &mut e, &mut a, &mut b, me2, 43);

    round2_step_bw4(&mut b, &mut c, &mut d, &mut e, &mut a, me2, 39);
    round2_step_bw4(&mut a, &mut b, &mut c, &mut d, &mut e, me2, 35);
    round2_step_bw4(&mut e, &mut a, &mut b, &mut c, &mut d, me2, 31);
    round2_step_bw4(&mut d, &mut e, &mut a, &mut b, &mut c, me2, 27);
    round2_step_bw4(&mut c, &mut d, &mut e, &mut a, &mut b, me2, 23);

    round1_step_bw4(&mut b, &mut c, &mut d, &mut e, &mut a, me2, 19);
    round1_step_bw4(&mut a, &mut b, &mut c, &mut d, &mut e, me2, 15);
    round1_step_bw4(&mut e, &mut a, &mut b, &mut c, &mut d, me2, 11);
    round1_step_bw4(&mut d, &mut e, &mut a, &mut b, &mut c, me2, 7);
    round1_step_bw4(&mut c, &mut d, &mut e, &mut a, &mut b, me2, 3);

    *ihvin = [a, b, c, d, e];
    [a, b, c, d, e] = *state;

    round3_step(c, &mut d, e, a, &mut b, me2[58]);
    round3_step(b, &mut c, d, e, &mut a, me2[59]);

    round4_step4(&mut a, &mut b, &mut c, &mut d, &mut e, me2, 60);
    round4_step4(&mut b, &mut c, &mut d, &mut e, &mut a, me2, 64);
    round4_step4(&mut c, &mut d, &mut e, &mut a, &mut b, me2, 68);
    round4_step4(&mut d, &mut e, &mut a, &mut b, &mut c, me2, 72);
    round4_step4(&mut e, &mut a, &mut b, &mut c, &mut d, me2, 76);

    ihvout[0] = ihvin[0].wrapping_add(a);
    ihvout[1] = ihvin[1].wrapping_add(b);
    ihvout[2] = ihvin[2].wrapping_add(c);
    ihvout[3] = ihvin[3].wrapping_add(d);
    ihvout[4] = ihvin[4].wrapping_add(e);
}

fn recompress_fast_65(
    ihvin: &mut [u32; 5],
    ihvout: &mut [u32; 5],
    me2: &[u32; 80],
    state: &[u32; 5],
) {
    let [mut a, mut b, mut c, mut d, mut e] = state;

    round4_step_bw(b, &mut c, d, e, &mut a, me2[64]);
    round4_step_bw4(&mut c, &mut d, &mut e, &mut a, &mut b, me2, 63);

    round3_step_bw4(&mut b, &mut c, &mut d, &mut e, &mut a, me2, 59);
    round3_step_bw4(&mut a, &mut b, &mut c, &mut d, &mut e, me2, 55);
    round3_step_bw4(&mut e, &mut a, &mut b, &mut c, &mut d, me2, 51);
    round3_step_bw4(&mut d, &mut e, &mut a, &mut b, &mut c, me2, 47);
    round3_step_bw4(&mut c, &mut d, &mut e, &mut a, &mut b, me2, 43);

    round2_step_bw4(&mut b, &mut c, &mut d, &mut e, &mut a, me2, 39);
    round2_step_bw4(&mut a, &mut b, &mut c, &mut d, &mut e, me2, 35);
    round2_step_bw4(&mut e, &mut a, &mut b, &mut c, &mut d, me2, 31);
    round2_step_bw4(&mut d, &mut e, &mut a, &mut b, &mut c, me2, 27);
    round2_step_bw4(&mut c, &mut d, &mut e, &mut a, &mut b, me2, 23);

    round1_step_bw4(&mut b, &mut c, &mut d, &mut e, &mut a, me2, 19);
    round1_step_bw4(&mut a, &mut b, &mut c, &mut d, &mut e, me2, 15);
    round1_step_bw4(&mut e, &mut a, &mut b, &mut c, &mut d, me2, 11);
    round1_step_bw4(&mut d, &mut e, &mut a, &mut b, &mut c, me2, 7);
    round1_step_bw4(&mut c, &mut d, &mut e, &mut a, &mut b, me2, 3);

    *ihvin = [a, b, c, d, e];
    [a, b, c, d, e] = *state;

    round4_step(a, &mut b, c, d, &mut e, me2[65]);
    round4_step(e, &mut a, b, c, &mut d, me2[66]);
    round4_step(d, &mut e, a, b, &mut c, me2[67]);

    round4_step4(&mut c, &mut d, &mut e, &mut a, &mut b, me2, 68);
    round4_step4(&mut d, &mut e, &mut a, &mut b, &mut c, me2, 72);
    round4_step4(&mut e, &mut a, &mut b, &mut c, &mut d, me2, 76);

    ihvout[0] = ihvin[0].wrapping_add(a);
    ihvout[1] = ihvin[1].wrapping_add(b);
    ihvout[2] = ihvin[2].wrapping_add(c);
    ihvout[3] = ihvin[3].wrapping_add(d);
    ihvout[4] = ihvin[4].wrapping_add(e);
}

fn recompression_step(
    step: Testt,
    ihvin: &mut [u32; 5],
    ihvout: &mut [u32; 5],
    me2: &[u32; 80],
    state: &[u32; 5],
) {
    match step {
        Testt::T58 => {
            recompress_fast_58(ihvin, ihvout, me2, state);
        }
        Testt::T65 => {
            recompress_fast_65(ihvin, ihvout, me2, state);
        }
    }
}

#[inline(always)]
fn xor(a: &[u32; 5], b: &[u32; 5]) -> u32 {
    a[0] ^ b[0] | a[1] ^ b[1] | a[2] ^ b[2] | a[3] ^ b[3] | a[4] ^ b[4]
}

#[inline]
pub(super) fn compress(
    state: &mut [u32; 5],
    ctx: &mut DetectionState,
    blocks: &[[u8; BLOCK_SIZE]],
) {
    let mut block_u32 = [0u32; BLOCK_SIZE / 4];

    for block in blocks.iter() {
        ctx.ihv1.copy_from_slice(&*state);

        for (o, chunk) in block_u32.iter_mut().zip(block.chunks_exact(4)) {
            *o = u32::from_be_bytes(chunk.try_into().unwrap());
        }

        let DetectionState {
            m1,
            state_58,
            state_65,
            ..
        } = ctx;

        compression_states(state, &block_u32, m1, state_58, state_65);

        let ubc_mask = if ctx.ubc_check {
            crate::ubc_check::ubc_check(&ctx.m1)
        } else {
            0xFFFFFFFF
        };

        if ubc_mask != 0 {
            let mut ihvtmp = [0u32; 5];
            for dv_type in &crate::ubc_check::SHA1_DVS {
                if ubc_mask & (1 << dv_type.maskb) != 0 {
                    for ((m2, m1), dm) in
                        ctx.m2.iter_mut().zip(ctx.m1.iter()).zip(dv_type.dm.iter())
                    {
                        *m2 = m1 ^ dm;
                    }
                    let DetectionState {
                        ihv2,
                        m2,
                        state_58,
                        state_65,
                        ..
                    } = ctx;

                    recompression_step(
                        dv_type.testt,
                        ihv2,
                        &mut ihvtmp,
                        m2,
                        match dv_type.testt {
                            Testt::T58 => state_58,
                            Testt::T65 => state_65,
                        },
                    );

                    // to verify SHA-1 collision detection code with collisions for reduced-step SHA-1
                    if (0 == xor(&ihvtmp, &*state))
                        || (ctx.reduced_round_collision && 0 == xor(&ctx.ihv1, &ctx.ihv2))
                    {
                        ctx.found_collision = true;

                        if ctx.safe_hash {
                            compression_w(state, &ctx.m1);
                            compression_w(state, &ctx.m1);
                        }
                        break;
                    }
                }
            }
        }
    }
}

const SHA1_PADDING: [u8; 64] = [
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0,
];

#[inline]
pub(super) fn finalize(
    state: &mut [u32; 5],
    total: u64,
    last_block: &[u8],
    ctx: &mut DetectionState,
) {
    let mut total = total + last_block.len() as u64;
    let last = last_block.len();
    let needs_two_blocks = last >= 56;

    let mut buffer = [0u8; BLOCK_SIZE];
    buffer[..last].copy_from_slice(last_block);
    let left = BLOCK_SIZE - last;

    if needs_two_blocks {
        let padn = 120 - last;
        let (pad0, pad1) = SHA1_PADDING[..padn].split_at(left);
        buffer[last..].copy_from_slice(pad0);
        compress(state, ctx, &[buffer]);
        buffer[..pad1.len()].copy_from_slice(pad1);
    } else {
        let padn = 56 - last;
        buffer[last..56].copy_from_slice(&SHA1_PADDING[..padn]);
    }

    total <<= 3;

    buffer[56] = (total >> 56) as u8;
    buffer[57] = (total >> 48) as u8;
    buffer[58] = (total >> 40) as u8;
    buffer[59] = (total >> 32) as u8;
    buffer[60] = (total >> 24) as u8;
    buffer[61] = (total >> 16) as u8;
    buffer[62] = (total >> 8) as u8;
    buffer[63] = total as u8;

    compress(state, ctx, &[buffer]);
}
