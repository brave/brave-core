// Copyright 2021-2023 Protocol Labs
// Copyright 2019-2022 ChainSafe Systems
// SPDX-License-Identifier: Apache-2.0, MIT

use super::alpha_beta_filter::*;
use crate::bigint::{BigInt, Integer};
use crate::clock::ChainEpoch;
use crate::math::{poly_parse, poly_val, PRECISION};

lazy_static! {
    pub static ref NUM: Vec<BigInt> = poly_parse(&[
        "261417938209272870992496419296200268025",
        "7266615505142943436908456158054846846897",
        "32458783941900493142649393804518050491988",
        "17078670566130897220338060387082146864806",
        "-35150353308172866634071793531642638290419",
        "-20351202052858059355702509232125230498980",
        "-1563932590352680681114104005183375350999",
    ])
    .unwrap();
    pub static ref DENOM: Vec<BigInt> = poly_parse(&[
        "49928077726659937662124949977867279384",
        "2508163877009111928787629628566491583994",
        "21757751789594546643737445330202599887121",
        "53400635271583923415775576342898617051826",
        "41248834748603606604000911015235164348839",
        "9015227820322455780436733526367238305537",
        "340282366920938463463374607431768211456",
    ])
    .unwrap();
    pub static ref DEFAULT_ALPHA: BigInt = "314760000000000000000000000000000000".parse().unwrap();
    pub static ref DEFAULT_BETA: BigInt = "96640100000000000000000000000000".parse().unwrap();
    pub static ref LN_2: BigInt = "235865763225513294137944142764154484399".parse().unwrap();
    pub static ref EPSILON: BigInt = "302231454903657293676544".parse().unwrap();
}

fn get_bit_len(z: &BigInt) -> u64 {
    z.bits()
}

/// Extrapolate the CumSumRatio given two filters.
pub fn extrapolated_cum_sum_of_ratio(
    delta: ChainEpoch,
    relative_start: ChainEpoch,
    est_num: &FilterEstimate,
    est_denom: &FilterEstimate,
) -> BigInt {
    let delta_t = BigInt::from(delta) << PRECISION;
    let t0 = BigInt::from(relative_start) << PRECISION;

    let pos_1 = &est_num.position;
    let pos_2 = &est_denom.position;
    let velo_1 = &est_num.velocity;
    let velo_2 = &est_denom.velocity;

    let squared_velo_2 = (velo_2 * velo_2) >> PRECISION;

    if squared_velo_2 > *EPSILON {
        let mut x2a = ((velo_2 * t0) >> PRECISION) + pos_2;
        let mut x2b = ((velo_2 * &delta_t) >> PRECISION) + &x2a;
        x2a = ln(&x2a);
        x2b = ln(&x2b);

        let m1 = ((&x2b - &x2a) * pos_1 * velo_2) >> PRECISION;

        let m2_l = (&x2a - &x2b) * pos_2;
        let m2_r = velo_2 * &delta_t;
        let m2: BigInt = ((m2_l + m2_r) * velo_1) >> PRECISION;

        return (m2 + m1).div_floor(&squared_velo_2);
    }

    let half_delta = &delta_t >> 1;
    let mut x1m: BigInt = velo_1 * (t0 + half_delta);
    x1m = (x1m >> PRECISION) + pos_1;

    (x1m * delta_t).div_floor(pos_2)
}

/// The natural log of Q.128 x.
pub fn ln(z: &BigInt) -> BigInt {
    let k: i64 = get_bit_len(z) as i64 - 1 - PRECISION as i64;

    let x: BigInt = if k > 0 { z >> k } else { z << k.abs() };

    (BigInt::from(k) * &*LN_2) + ln_between_one_and_two(x)
}

/// The natural log of x, specified in Q.128 format
/// Should only use with 1 <= x <= 2
/// Output is in Q.128 format.
fn ln_between_one_and_two(x: BigInt) -> BigInt {
    let num = poly_val(&NUM, &x) << PRECISION;
    let denom = poly_val(&DENOM, &x);
    num.div_floor(&denom)
}
