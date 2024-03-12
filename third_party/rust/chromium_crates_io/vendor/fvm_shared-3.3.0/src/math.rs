// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use crate::bigint::{BigInt, ParseBigIntError};

pub const PRECISION: u64 = 128;

/// polyval evaluates a polynomial given by coefficients `p` in Q.128 format
/// at point `x` in Q.128 format. Output is in Q.128.
/// Coefficients should be ordered from the highest order coefficient to the lowest.
pub fn poly_val(poly: &[BigInt], x: &BigInt) -> BigInt {
    let mut res = BigInt::default();

    for coeff in poly {
        res = ((res * x) >> PRECISION) + coeff;
    }
    res
}

pub fn poly_parse(coefs: &[&str]) -> Result<Vec<BigInt>, ParseBigIntError> {
    coefs.iter().map(|c| c.parse()).collect()
}
