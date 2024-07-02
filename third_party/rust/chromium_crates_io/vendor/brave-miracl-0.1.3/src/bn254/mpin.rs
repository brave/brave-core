/*
 * Copyright (c) 2012-2020 MIRACL UK Ltd.
 *
 * This file is part of MIRACL Core
 * (see https://github.com/miracl/core).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

use crate::bn254::big;
use crate::bn254::big::BIG;
use crate::bn254::ecp;
use crate::bn254::ecp::ECP;
use crate::bn254::ecp2::ECP2;
use crate::bn254::fp12::FP12;
use crate::bn254::pair;
use crate::bn254::rom;
use crate::bn254::fp::FP;
use crate::bn254::dbig::DBIG;

use crate::hmac;
use crate::rand::RAND;

/* MPIN 128-bit API Functions */

/* Configure mode of operation */

pub const EFS: usize = big::MODBYTES as usize;
pub const EGS: usize = big::MODBYTES as usize;
pub const BAD_PARAMS: isize = -11;
pub const INVALID_POINT: isize = -14;
pub const WRONG_ORDER: isize = -18;
pub const BAD_PIN: isize = -19;
pub const SHA256: usize = 32;
pub const SHA384: usize = 48;
pub const SHA512: usize = 64;

/* Configure your PIN here */

pub const MAXPIN: i32 = 10000; /* PIN less than this */
pub const PBLEN: i32 = 14; /* Number of bits in PIN */

fn ceil(a: usize,b: usize) -> usize {
    (a-1)/b+1
}

#[allow(non_snake_case)]
pub fn encode_to_curve(dst: &[u8],id: &[u8],hcid: &mut [u8]) {
    let q = BIG::new_ints(&rom::MODULUS);
    let k=q.nbits();
    let r = BIG::new_ints(&rom::CURVE_ORDER);
    let m=r.nbits();
    let el=ceil(k+ceil(m,2),8);
    let mut okm: [u8;512]=[0;512];
    hmac::xmd_expand(hmac::MC_SHA2,ecp::HASH_TYPE,&mut okm,el,&dst,&id);
    let mut fd: [u8;256]=[0;256];
    for j in 0..el {
        fd[j]=okm[j];
    }
	let mut dx=DBIG::frombytes(&fd[0..el]);
    let u=FP::new_big(&dx.dmod(&q));
    let mut P=ECP::map2point(&u);
    P.cfp();
    P.affine();
    P.tobytes(hcid,false);
}

/* create random secret S */
pub fn random_generate(rng: &mut RAND, s: &mut [u8]) -> isize {
    let r = BIG::new_ints(&rom::CURVE_ORDER);
    let sc = BIG::randtrunc(&r, 16 * ecp::AESKEY, rng);
    sc.tobytes(s);
    0
}

/* Extract PIN from TOKEN for identity CID */
#[allow(non_snake_case)]
pub fn extract_pin(cid: &[u8], pin: i32, token: &mut [u8]) -> isize {
    let mut P = ECP::frombytes(&token);
    if P.is_infinity() {
        return INVALID_POINT;
    }
    let mut R = ECP::frombytes(&cid);
    if R.is_infinity() {
        return INVALID_POINT;
    }

    R = R.pinmul(pin%MAXPIN, PBLEN);
    P.sub(&R);
    P.tobytes(token, false);
    0
}

/* Implement step 2 on client side of MPin protocol */
#[allow(non_snake_case)]
pub fn client_2(x: &[u8], y: &[u8], sec: &mut [u8]) -> isize {
    let r = BIG::new_ints(&rom::CURVE_ORDER);
    let mut P = ECP::frombytes(sec);
    if P.is_infinity() {
        return INVALID_POINT;
    }

    let mut px = BIG::frombytes(x);
    let py = BIG::frombytes(y);
    px.add(&py);
    px.rmod(&r);

    P = pair::g1mul(&P, &px);
    P.neg();
    P.tobytes(sec, false);
    0
}

/* Client secret CST=S*H(CID) where CID is client ID and S is master secret */
#[allow(non_snake_case)]
pub fn get_client_secret(s: &mut [u8], idhtc: &[u8], cst: &mut [u8]) -> isize {
    let sx=BIG::frombytes(s);
    let P=ECP::frombytes(idhtc);
    if P.is_infinity() {
        return INVALID_POINT;
    }
    pair::g1mul(&P, &sx).tobytes(cst, false);
    0
}

/* Implement step 1 on client side of MPin protocol */
#[allow(non_snake_case)]
pub fn client_1(
    cid: &[u8],
    rng: Option<&mut RAND>,
    x: &mut [u8],
    pin: usize,
    token: &[u8],
    sec: &mut [u8],
    xid: &mut [u8]
) -> isize {
    let r = BIG::new_ints(&rom::CURVE_ORDER);
    let sx: BIG;

    if let Some(rd) = rng {
        sx = BIG::randtrunc(&r, 16 * ecp::AESKEY, rd);
        sx.tobytes(x);
    } else {
        sx = BIG::frombytes(x);
    }
    let mut P=ECP::frombytes(cid);
    if P.is_infinity() {
        return INVALID_POINT;
    }

    let mut T = ECP::frombytes(&token);
    if T.is_infinity() {
        return INVALID_POINT;
    }

    let W = P.pinmul((pin as i32) % MAXPIN, PBLEN);
    T.add(&W);

    P = pair::g1mul(&P, &sx);
    P.tobytes(xid, false);

    T.tobytes(sec, false);
    0
}


/* Extract Server Secret SST=S*Q where Q is fixed generator in G2 and S is master secret */
#[allow(non_snake_case)]
pub fn get_server_secret(s: &[u8], sst: &mut [u8]) -> isize {
    let mut Q = ECP2::generator();
    let sc = BIG::frombytes(s);
    Q = pair::g2mul(&Q, &sc);
    Q.tobytes(sst,false);
    0
}

/* Implement step 2 of MPin protocol on server side */
#[allow(non_snake_case)]
pub fn server(
    hid: &[u8],
    y: &[u8],
    sst: &[u8],
    xid: &[u8],
    msec: &[u8],
) -> isize {
    let Q = ECP2::generator();
    let sQ = ECP2::frombytes(&sst);
    if sQ.is_infinity() {
        return INVALID_POINT;
    }
    let mut R = ECP::frombytes(&xid);
    if R.is_infinity() {
        return INVALID_POINT;
    }

    let sy = BIG::frombytes(&y);
    let mut P = ECP::frombytes(&hid);
    if P.is_infinity() {
        return INVALID_POINT;
    }

    P = pair::g1mul(&P, &sy);
    P.add(&R);
    R = ECP::frombytes(&msec);
    if R.is_infinity() {
        return INVALID_POINT;
    }

    let mut g: FP12;
    g = pair::ate2(&Q, &R, &sQ, &P);
    g = pair::fexp(&g);

    if !g.isunity() {
        return BAD_PIN;
    }
    0
}

