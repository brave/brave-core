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
use crate::bn254::fp::FP;
use crate::bn254::ecp;
use crate::bn254::ecp::ECP;
use crate::bn254::dbig::DBIG;
use crate::bn254::ecp2::ECP2;
//use crate::bn254::fp4::FP4;
use crate::bn254::pair;
use crate::bn254::rom;
use crate::hmac;

/* Boneh-Lynn-Shacham signature 128-bit API Functions */

/* Loosely (for now) following https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-bls-signature-02 */

// Minimal-signature-size variant

pub const BFS: usize = big::MODBYTES as usize;
pub const BGS: usize = big::MODBYTES as usize;
pub const BLS_OK: isize = 0;
pub const BLS_FAIL: isize = -1;

// NOTE this must be accessed in unsafe mode.
// But it is just written to once at start-up, so actually safe.

// Best not to use precomp if stack memory limited - i.e. embedded use
// Uncomment to use precomp
// static mut G2_TAB: [FP4; ecp::G2_TABLE] = [FP4::new(); ecp::G2_TABLE];

fn ceil(a: usize,b: usize) -> usize {
    (a-1)/b+1
}

/* output u \in F_p */
fn hash_to_field(hash: usize,hlen: usize ,u: &mut [FP], dst: &[u8],m: &[u8],ctr: usize) {
    let q = BIG::new_ints(&rom::MODULUS);
    let nbq=q.nbits();
    let el = ceil(nbq+ecp::AESKEY*8,8);

    let mut okm: [u8;256]=[0;256];
    let mut fd: [u8;128]=[0;128];

    hmac::xmd_expand(hash,hlen,&mut okm,el*ctr,&dst,&m);
    for i in 0..ctr {
        for j in 0..el {
            fd[j]=okm[el*i+j];
        }
        u[i]=FP::new_big(&DBIG::frombytes(&fd[0 .. el]).ctdmod(&q,8*el-nbq));
    }
}

/* hash a message to an ECP point, using SHA2, random oracle method */
#[allow(non_snake_case)]
pub fn bls_hash_to_point(m: &[u8]) -> ECP {
    let dst= "BLS_SIG_BN254G1_XMD:SHA-256_SVDW_RO_NUL_";

    let mut u: [FP; 2] = [
        FP::new(),
        FP::new(),
    ];
    hash_to_field(hmac::MC_SHA2,ecp::HASH_TYPE,&mut u,dst.as_bytes(),m,2);

    let mut P=ECP::map2point(&u[0]);
    let P1=ECP::map2point(&u[1]);

    P.add(&P1);
    P.cfp();
    P.affine();
    P
}

pub fn init() -> isize {
// Uncomment to use precomp
//    let g = ECP2::generator();
//    if g.is_infinity() {
//        return BLS_FAIL;
//    }
//    unsafe {
//        pair::precomp(&mut G2_TAB, &g);
//    }
    BLS_OK
}

/* generate key pair, private key s, public key w */
pub fn key_pair_generate(ikm: &[u8], s: &mut [u8], w: &mut [u8]) -> isize {
    let r = BIG::new_ints(&rom::CURVE_ORDER);  
    let nbr=r.nbits();
    let el = ceil(3*ceil(nbr,8),2);
    let g = ECP2::generator();
    let mut len: [u8; 2] = [0; 2];
    hmac::inttobytes(el,&mut len);
    
    let salt="BLS-SIG-KEYGEN-SALT-";

    let mut prk: [u8;64]=[0;64];
    let mut okm: [u8;128]=[0;128];
    let mut aikm: [u8;65]=[0;65];
    let likm=ikm.len();
    for i in 0..likm {
        aikm[i]=ikm[i];
    }
    aikm[likm]=0;

    let hlen=ecp::HASH_TYPE;

    hmac::hkdf_extract(hmac::MC_SHA2,hlen,&mut prk,Some(&salt.as_bytes()),&aikm[0 .. likm+1]);
    hmac::hkdf_expand(hmac::MC_SHA2,hlen,&mut okm,el,&prk[0 .. hlen],&len);

    let mut dx = DBIG::frombytes(&okm[0 .. el]);
    let sc = dx.ctdmod(&r,8*el-nbr);
    sc.tobytes(s);
// SkToPk
    pair::g2mul(&g, &sc).tobytes(w,true);  // true for public key compression
    BLS_OK
}

/* Sign message m using private key s to produce signature sig */

pub fn core_sign(sig: &mut [u8], m: &[u8], s: &[u8]) -> isize {
    let d = bls_hash_to_point(m);
    let sc = BIG::frombytes(&s);
    pair::g1mul(&d, &sc).tobytes(sig, true);
    BLS_OK
}

/* Verify signature given message m, the signature sig, and the public key w */

pub fn core_verify(sig: &[u8], m: &[u8], w: &[u8]) -> isize {
    let hm = bls_hash_to_point(m);
    let mut d = ECP::frombytes(&sig);

    if !pair::g1member(&d) {
        return BLS_FAIL;
    }
    d.neg();

    let pk = ECP2::frombytes(&w);
    if !pair::g2member(&pk) {
        return BLS_FAIL;
    }

// Uncomment to use precomp
    // Use new multi-pairing mechanism
    //let mut r = pair::initmp();
    //    pair::another(&mut r,&g,&d);

    //unsafe {
    //    pair::another_pc(&mut r, &G2_TAB, &d);
    //}
    //pair::another(&mut r, &pk, &hm);
    //let mut v = pair::miller(&mut r);

    //.. or else do not use precomp!
        let g = ECP2::generator();
        let mut v = pair::ate2(&g, &d, &pk, &hm);

    v = pair::fexp(&v);
    if v.isunity() {
        return BLS_OK;
    }
    BLS_FAIL
}
