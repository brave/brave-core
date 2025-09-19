// -*- mode: rust; -*-
//
// This file is part of schnorrkel.
// Copyright (c) 2019 Web 3 Foundation
// See LICENSE for licensing information.
//
// Authors:
// - Jeff Burdges <jeff@web3.foundation>

//! Scalar tooling
//!
//! Elliptic curve utilities not provided by curve25519-dalek,
//! including some not so safe utilities for managing scalars and points.

pub(crate) fn divide_scalar_bytes_by_cofactor(scalar: &mut [u8; 32]) {
    let mut low = 0u8;
    for i in scalar.iter_mut().rev() {
        let r = *i & 0b00000111; // save remainder
        *i >>= 3; // divide by 8
        *i += low;
        low = r << 5;
    }
}

pub(crate) fn multiply_scalar_bytes_by_cofactor(scalar: &mut [u8; 32]) {
    let mut high = 0u8;
    for i in scalar.iter_mut() {
        let r = *i & 0b11100000; // carry bits
        *i <<= 3; // multiply by 8
        *i += high;
        high = r >> 5;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    // use ed25519_dalek::SecretKey;
    use rand::{thread_rng, Rng};

    // TODO: Simple test `RistrettoPoint` is implemented as an `EdwardsPoint`
    // #[test]
    // fn ristretto_point_is_edwards_point() {
    // }

    #[test]
    fn cofactor_adjustment() {
        let mut x: [u8; 32] = thread_rng().gen();
        x[31] &= 0b00011111;
        let mut y = x.clone();
        multiply_scalar_bytes_by_cofactor(&mut y);
        divide_scalar_bytes_by_cofactor(&mut y);
        assert_eq!(x, y);

        let mut x: [u8; 32] = thread_rng().gen();
        x[0] &= 0b11111000;
        let mut y = x.clone();
        divide_scalar_bytes_by_cofactor(&mut y);
        multiply_scalar_bytes_by_cofactor(&mut y);
        assert_eq!(x, y);
    }
}
