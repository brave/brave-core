//! Minimal implementation of (parts of) Strobe.

use core::ops::{Deref, DerefMut};

use keccak;
use zeroize::Zeroize;

/// Strobe R value; security level 128 is hardcoded
const STROBE_R: u8 = 166;

const FLAG_I: u8 = 1;
const FLAG_A: u8 = 1 << 1;
const FLAG_C: u8 = 1 << 2;
const FLAG_T: u8 = 1 << 3;
const FLAG_M: u8 = 1 << 4;
const FLAG_K: u8 = 1 << 5;

fn transmute_state(st: &mut AlignedKeccakState) -> &mut [u64; 25] {
    unsafe { &mut *(st as *mut AlignedKeccakState as *mut [u64; 25]) }
}

/// This is a wrapper around 200-byte buffer that's always 8-byte aligned
/// to make pointers to it safely convertible to pointers to [u64; 25]
/// (since u64 words must be 8-byte aligned)
#[derive(Clone, Zeroize)]
#[zeroize(drop)]
#[repr(align(8))]
struct AlignedKeccakState([u8; 200]);

/// A Strobe context for the 128-bit security level.
///
/// Only `meta-AD`, `AD`, `KEY`, and `PRF` operations are supported.
#[derive(Clone, Zeroize)]
pub struct Strobe128 {
    state: AlignedKeccakState,
    pos: u8,
    pos_begin: u8,
    cur_flags: u8,
}

impl ::core::fmt::Debug for Strobe128 {
    fn fmt(&self, f: &mut ::core::fmt::Formatter<'_>) -> ::core::fmt::Result {
        // Ensure that the Strobe state isn't accidentally logged
        write!(f, "Strobe128: STATE OMITTED")
    }
}

impl Strobe128 {
    pub fn new(protocol_label: &[u8]) -> Strobe128 {
        let initial_state = {
            let mut st = AlignedKeccakState([0u8; 200]);
            st[0..6].copy_from_slice(&[1, STROBE_R + 2, 1, 0, 1, 96]);
            st[6..18].copy_from_slice(b"STROBEv1.0.2");
            keccak::f1600(transmute_state(&mut st));

            st
        };

        let mut strobe = Strobe128 {
            state: initial_state,
            pos: 0,
            pos_begin: 0,
            cur_flags: 0,
        };

        strobe.meta_ad(protocol_label, false);

        strobe
    }

    pub fn meta_ad(&mut self, data: &[u8], more: bool) {
        self.begin_op(FLAG_M | FLAG_A, more);
        self.absorb(data);
    }

    pub fn ad(&mut self, data: &[u8], more: bool) {
        self.begin_op(FLAG_A, more);
        self.absorb(data);
    }

    pub fn prf(&mut self, data: &mut [u8], more: bool) {
        self.begin_op(FLAG_I | FLAG_A | FLAG_C, more);
        self.squeeze(data);
    }

    pub fn key(&mut self, data: &[u8], more: bool) {
        self.begin_op(FLAG_A | FLAG_C, more);
        self.overwrite(data);
    }
}

impl Strobe128 {
    fn run_f(&mut self) {
        self.state[self.pos as usize] ^= self.pos_begin;
        self.state[(self.pos + 1) as usize] ^= 0x04;
        self.state[(STROBE_R + 1) as usize] ^= 0x80;
        keccak::f1600(transmute_state(&mut self.state));
        self.pos = 0;
        self.pos_begin = 0;
    }

    fn absorb(&mut self, data: &[u8]) {
        for byte in data {
            self.state[self.pos as usize] ^= byte;
            self.pos += 1;
            if self.pos == STROBE_R {
                self.run_f();
            }
        }
    }

    fn overwrite(&mut self, data: &[u8]) {
        for byte in data {
            self.state[self.pos as usize] = *byte;
            self.pos += 1;
            if self.pos == STROBE_R {
                self.run_f();
            }
        }
    }

    fn squeeze(&mut self, data: &mut [u8]) {
        for byte in data {
            *byte = self.state[self.pos as usize];
            self.state[self.pos as usize] = 0;
            self.pos += 1;
            if self.pos == STROBE_R {
                self.run_f();
            }
        }
    }

    fn begin_op(&mut self, flags: u8, more: bool) {
        // Check if we're continuing an operation
        if more {
            assert_eq!(
                self.cur_flags, flags,
                "You tried to continue op {:#b} but changed flags to {:#b}",
                self.cur_flags, flags,
            );
            return;
        }

        // Skip adjusting direction information (we just use AD, PRF)
        assert_eq!(
            flags & FLAG_T,
            0u8,
            "You used the T flag, which this implementation doesn't support"
        );

        let old_begin = self.pos_begin;
        self.pos_begin = self.pos + 1;
        self.cur_flags = flags;

        self.absorb(&[old_begin, flags]);

        // Force running F if C or K is set
        let force_f = 0 != (flags & (FLAG_C | FLAG_K));

        if force_f && self.pos != 0 {
            self.run_f();
        }
    }
}

impl Deref for AlignedKeccakState {
    type Target = [u8; 200];

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for AlignedKeccakState {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

#[cfg(test)]
mod tests {
    use strobe_rs::{self, SecParam};

    #[test]
    fn test_conformance() {
        let mut s1 = super::Strobe128::new(b"Conformance Test Protocol");
        let mut s2 = strobe_rs::Strobe::new(b"Conformance Test Protocol", SecParam::B128);

        // meta-AD(b"msg"); AD(msg)

        let msg = [99u8; 1024];

        s1.meta_ad(b"ms", false);
        s1.meta_ad(b"g", true);
        s1.ad(&msg, false);

        s2.meta_ad(b"ms", false);
        s2.meta_ad(b"g", true);
        s2.ad(&msg, false);

        // meta-AD(b"prf"); PRF()

        let mut prf1 = [0u8; 32];
        s1.meta_ad(b"prf", false);
        s1.prf(&mut prf1, false);

        let mut prf2 = [0u8; 32];
        s2.meta_ad(b"prf", false);
        s2.prf(&mut prf2, false);

        assert_eq!(prf1, prf2);

        // meta-AD(b"key"); KEY(prf output)

        s1.meta_ad(b"key", false);
        s1.key(&prf1, false);

        s2.meta_ad(b"key", false);
        s2.key(&prf2, false);

        // meta-AD(b"prf"); PRF()

        let mut prf1 = [0u8; 32];
        s1.meta_ad(b"prf", false);
        s1.prf(&mut prf1, false);

        let mut prf2 = [0u8; 32];
        s2.meta_ad(b"prf", false);
        s2.prf(&mut prf2, false);

        assert_eq!(prf1, prf2);
    }
}
