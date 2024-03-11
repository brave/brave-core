//! Software implementation of the Poly1305 state machine.

// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
//
// This code originates from the rust-crypto project:
// <https://github.com/DaGenix/rust-crypto>
//
// ...and was originally a port of Andrew Moons poly1305-donna
// https://github.com/floodyberry/poly1305-donna

use universal_hash::{
    consts::{U1, U16},
    crypto_common::{BlockSizeUser, ParBlocksSizeUser},
    UhfBackend, UniversalHash,
};

use crate::{Block, Key, Tag};

#[derive(Clone, Default)]
pub(crate) struct State {
    r: [u32; 5],
    h: [u32; 5],
    pad: [u32; 4],
}

impl State {
    /// Initialize Poly1305 [`State`] with the given key
    pub(crate) fn new(key: &Key) -> State {
        let mut poly = State::default();

        // r &= 0xffffffc0ffffffc0ffffffc0fffffff
        poly.r[0] = (u32::from_le_bytes(key[0..4].try_into().unwrap())) & 0x3ff_ffff;
        poly.r[1] = (u32::from_le_bytes(key[3..7].try_into().unwrap()) >> 2) & 0x3ff_ff03;
        poly.r[2] = (u32::from_le_bytes(key[6..10].try_into().unwrap()) >> 4) & 0x3ff_c0ff;
        poly.r[3] = (u32::from_le_bytes(key[9..13].try_into().unwrap()) >> 6) & 0x3f0_3fff;
        poly.r[4] = (u32::from_le_bytes(key[12..16].try_into().unwrap()) >> 8) & 0x00f_ffff;

        poly.pad[0] = u32::from_le_bytes(key[16..20].try_into().unwrap());
        poly.pad[1] = u32::from_le_bytes(key[20..24].try_into().unwrap());
        poly.pad[2] = u32::from_le_bytes(key[24..28].try_into().unwrap());
        poly.pad[3] = u32::from_le_bytes(key[28..32].try_into().unwrap());

        poly
    }

    /// Compute a Poly1305 block
    pub(crate) fn compute_block(&mut self, block: &Block, partial: bool) {
        let hibit = if partial { 0 } else { 1 << 24 };

        let r0 = self.r[0];
        let r1 = self.r[1];
        let r2 = self.r[2];
        let r3 = self.r[3];
        let r4 = self.r[4];

        let s1 = r1 * 5;
        let s2 = r2 * 5;
        let s3 = r3 * 5;
        let s4 = r4 * 5;

        let mut h0 = self.h[0];
        let mut h1 = self.h[1];
        let mut h2 = self.h[2];
        let mut h3 = self.h[3];
        let mut h4 = self.h[4];

        // h += m
        h0 += (u32::from_le_bytes(block[0..4].try_into().unwrap())) & 0x3ff_ffff;
        h1 += (u32::from_le_bytes(block[3..7].try_into().unwrap()) >> 2) & 0x3ff_ffff;
        h2 += (u32::from_le_bytes(block[6..10].try_into().unwrap()) >> 4) & 0x3ff_ffff;
        h3 += (u32::from_le_bytes(block[9..13].try_into().unwrap()) >> 6) & 0x3ff_ffff;
        h4 += (u32::from_le_bytes(block[12..16].try_into().unwrap()) >> 8) | hibit;

        // h *= r
        let d0 = (u64::from(h0) * u64::from(r0))
            + (u64::from(h1) * u64::from(s4))
            + (u64::from(h2) * u64::from(s3))
            + (u64::from(h3) * u64::from(s2))
            + (u64::from(h4) * u64::from(s1));

        let mut d1 = (u64::from(h0) * u64::from(r1))
            + (u64::from(h1) * u64::from(r0))
            + (u64::from(h2) * u64::from(s4))
            + (u64::from(h3) * u64::from(s3))
            + (u64::from(h4) * u64::from(s2));

        let mut d2 = (u64::from(h0) * u64::from(r2))
            + (u64::from(h1) * u64::from(r1))
            + (u64::from(h2) * u64::from(r0))
            + (u64::from(h3) * u64::from(s4))
            + (u64::from(h4) * u64::from(s3));

        let mut d3 = (u64::from(h0) * u64::from(r3))
            + (u64::from(h1) * u64::from(r2))
            + (u64::from(h2) * u64::from(r1))
            + (u64::from(h3) * u64::from(r0))
            + (u64::from(h4) * u64::from(s4));

        let mut d4 = (u64::from(h0) * u64::from(r4))
            + (u64::from(h1) * u64::from(r3))
            + (u64::from(h2) * u64::from(r2))
            + (u64::from(h3) * u64::from(r1))
            + (u64::from(h4) * u64::from(r0));

        // (partial) h %= p
        let mut c: u32;
        c = (d0 >> 26) as u32;
        h0 = d0 as u32 & 0x3ff_ffff;
        d1 += u64::from(c);

        c = (d1 >> 26) as u32;
        h1 = d1 as u32 & 0x3ff_ffff;
        d2 += u64::from(c);

        c = (d2 >> 26) as u32;
        h2 = d2 as u32 & 0x3ff_ffff;
        d3 += u64::from(c);

        c = (d3 >> 26) as u32;
        h3 = d3 as u32 & 0x3ff_ffff;
        d4 += u64::from(c);

        c = (d4 >> 26) as u32;
        h4 = d4 as u32 & 0x3ff_ffff;
        h0 += c * 5;

        c = h0 >> 26;
        h0 &= 0x3ff_ffff;
        h1 += c;

        self.h[0] = h0;
        self.h[1] = h1;
        self.h[2] = h2;
        self.h[3] = h3;
        self.h[4] = h4;
    }

    /// Finalize output producing a [`Tag`]
    pub(crate) fn finalize_mut(&mut self) -> Tag {
        // fully carry h
        let mut h0 = self.h[0];
        let mut h1 = self.h[1];
        let mut h2 = self.h[2];
        let mut h3 = self.h[3];
        let mut h4 = self.h[4];

        let mut c: u32;
        c = h1 >> 26;
        h1 &= 0x3ff_ffff;
        h2 += c;

        c = h2 >> 26;
        h2 &= 0x3ff_ffff;
        h3 += c;

        c = h3 >> 26;
        h3 &= 0x3ff_ffff;
        h4 += c;

        c = h4 >> 26;
        h4 &= 0x3ff_ffff;
        h0 += c * 5;

        c = h0 >> 26;
        h0 &= 0x3ff_ffff;
        h1 += c;

        // compute h + -p
        let mut g0 = h0.wrapping_add(5);
        c = g0 >> 26;
        g0 &= 0x3ff_ffff;

        let mut g1 = h1.wrapping_add(c);
        c = g1 >> 26;
        g1 &= 0x3ff_ffff;

        let mut g2 = h2.wrapping_add(c);
        c = g2 >> 26;
        g2 &= 0x3ff_ffff;

        let mut g3 = h3.wrapping_add(c);
        c = g3 >> 26;
        g3 &= 0x3ff_ffff;

        let mut g4 = h4.wrapping_add(c).wrapping_sub(1 << 26);

        // select h if h < p, or h + -p if h >= p
        let mut mask = (g4 >> (32 - 1)).wrapping_sub(1);
        g0 &= mask;
        g1 &= mask;
        g2 &= mask;
        g3 &= mask;
        g4 &= mask;
        mask = !mask;
        h0 = (h0 & mask) | g0;
        h1 = (h1 & mask) | g1;
        h2 = (h2 & mask) | g2;
        h3 = (h3 & mask) | g3;
        h4 = (h4 & mask) | g4;

        // h = h % (2^128)
        h0 |= h1 << 26;
        h1 = (h1 >> 6) | (h2 << 20);
        h2 = (h2 >> 12) | (h3 << 14);
        h3 = (h3 >> 18) | (h4 << 8);

        // h = mac = (h + pad) % (2^128)
        let mut f: u64;
        f = u64::from(h0) + u64::from(self.pad[0]);
        h0 = f as u32;

        f = u64::from(h1) + u64::from(self.pad[1]) + (f >> 32);
        h1 = f as u32;

        f = u64::from(h2) + u64::from(self.pad[2]) + (f >> 32);
        h2 = f as u32;

        f = u64::from(h3) + u64::from(self.pad[3]) + (f >> 32);
        h3 = f as u32;

        let mut tag = Block::default();
        tag[0..4].copy_from_slice(&h0.to_le_bytes());
        tag[4..8].copy_from_slice(&h1.to_le_bytes());
        tag[8..12].copy_from_slice(&h2.to_le_bytes());
        tag[12..16].copy_from_slice(&h3.to_le_bytes());

        tag
    }
}

#[cfg(feature = "zeroize")]
impl Drop for State {
    fn drop(&mut self) {
        use zeroize::Zeroize;
        self.r.zeroize();
        self.h.zeroize();
        self.pad.zeroize();
    }
}

impl BlockSizeUser for State {
    type BlockSize = U16;
}

impl ParBlocksSizeUser for State {
    type ParBlocksSize = U1;
}

impl UhfBackend for State {
    fn proc_block(&mut self, block: &Block) {
        self.compute_block(block, false);
    }
}

impl UniversalHash for State {
    fn update_with_backend(
        &mut self,
        f: impl universal_hash::UhfClosure<BlockSize = Self::BlockSize>,
    ) {
        f.call(self);
    }

    /// Finalize output producing a [`Tag`]
    fn finalize(mut self) -> Tag {
        self.finalize_mut()
    }
}
