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

/* ECDH/ECIES/ECDSA API Functions */

use crate::bn254::big;
use crate::bn254::big::BIG;
use crate::bn254::fp;
use crate::bn254::fp::FP;
use crate::bn254::dbig::DBIG;
use crate::bn254::ecp;
use crate::bn254::ecp::ECP;
use crate::bn254::rom;
use crate::rand::RAND;
use crate::hash512::HASH512;
use crate::sha3;
use crate::sha3::SHA3;

pub const INVALID_PUBLIC_KEY: isize = -2;
pub const ERROR: isize = -3;
//pub const INVALID: isize = -4;
pub const EFS: usize = big::MODBYTES as usize;
pub const EGS: usize = big::MODBYTES as usize;

fn rfc7748(r: &mut BIG) {
    let mut lg=0;
    let mut t=BIG::new_int(1);
    let mut c=rom::CURVE_COF_I;
    while c!=1 {
        lg+=1;
        c/=2;
    }
    let n=(8*EGS-lg+1) as usize;
    r.mod2m(n);
    t.shl(n);
    r.add(&t);
    c=r.lastbits(lg as usize);
    r.dec(c);
}

// reverse first n bytes of buff - for little endian
fn reverse(n: usize,buff: &mut [u8]) {
    for i in 0..n/2 { 
        let ch = buff[i]; 
        buff[i] = buff[n - i - 1]; 
        buff[n - i - 1] = ch; 
    }    
}

// dom - domain function
fn dom(ds: &str,ph: bool,cl: usize,domain: &mut [u8]) -> usize {
    let dsb = ds.as_bytes();
    let n=dsb.len();
    for i in 0..n {
        domain[i]=dsb[i];
    }
    if ph {
        domain[n]=1;
    } else { 
        domain[n]=0;
    }
    domain[n+1]=cl as u8;
    return n+2;
}

fn h(s: &[u8],digest: &mut [u8]) -> usize {
    if ecp::AESKEY<=16 { // for ed25519?
        let mut sh=HASH512::new();
        for i in 0..s.len() {
            sh.process(s[i]);
        }
        let hs = sh.hash();
        for i in 0..64 {
            digest[i]=hs[i];
        }
        return 64;
    } else {                       // for ed448?
        let mut sh=SHA3::new(sha3::SHAKE256);
        for i in 0..s.len() {
            sh.process(s[i]);
        }
        sh.shake(digest,2*s.len());
        return 2*s.len();
    }
}

fn h2(ph: bool,ctx: Option<&[u8]>,r: &[u8],q: &[u8],m: &[u8]) -> DBIG {
    let b=q.len();
    let mut cl=0;
    if let Some(sctx) = ctx {
        cl=sctx.len();
    }
    let mut domain: [u8; 64] = [0; 64];
    if ecp::AESKEY<=16 { // Ed25519??
        let mut sh=HASH512::new();
        if ph || cl>0 {                   // if not prehash and no context, omit dom2()
            let dl=dom("Sigbn254 no bn254 collisions",ph,cl,&mut domain);
            for i in 0..dl {
                sh.process(domain[i]);
            }
            if let Some(sctx) = ctx {
                for i in 0..cl {
                    sh.process(sctx[i]);
                }
            }
        }
        for i in 0..b {
            sh.process(r[i]);
        }
        for i in 0..b {
            sh.process(q[i]);
        }
        for i in 0..m.len() {
            sh.process(m[i]);
        }
        let mut h=sh.hash();
        reverse(64,&mut h);
        return DBIG::frombytes(&h);
   } else {                       // for ed448?
        let dl=dom("Sigbn254",ph,cl,&mut domain);
        let mut h: [u8; 128] = [0; 128];
        let mut sh=SHA3::new(sha3::SHAKE256);
        for i in 0..dl {
            sh.process(domain[i]);
        }
        if let Some(sctx) = ctx {
            for i in 0..cl {
                sh.process(sctx[i]);
            }
        }
        for i in 0..b { 
            sh.process(r[i]);
        }
        for i in 0..b {
            sh.process(q[i]);
        }
        for i in 0..m.len() { 
            sh.process(m[i]);
        }

        sh.shake(&mut h,2*b);
        reverse(2*b,&mut h[0..2*b]);
        return DBIG::frombytes(&h[0..2*b]);
    }
}

fn getr(ph: bool,b: usize,digest: &[u8],ctx: Option<&[u8]>,m: &[u8]) -> DBIG {
    let mut cl=0;
    if let Some(sctx) = ctx {
        cl=sctx.len();
    }
    let mut domain: [u8; 64] = [0; 64];

    if ecp::AESKEY<=16 { // Ed25519??
        let mut sh=HASH512::new();
        if ph || cl>0 {                   // if not prehash and no context, omit dom2()
            let dl=dom("Sigbn254 no bn254 collisions",ph,cl,&mut domain);
            for i in 0..dl {
                sh.process(domain[i]);
            }
            if let Some(sctx) = ctx {
                for i in 0..cl {
                    sh.process(sctx[i]);
                }
            }
        }
        for i in b..2*b {
            sh.process(digest[i]);
        }
        for i in 0..m.len() {
            sh.process(m[i]);
        }
        let mut h=sh.hash();
        reverse(64,&mut h);
        return DBIG::frombytes(&h);
    } else {                       // for ed448?
        let dl=dom("Sigbn254",ph,cl,&mut domain);
        let mut h: [u8; 128] = [0; 128];
        let mut sh=SHA3::new(sha3::SHAKE256);
        for i in 0..dl {
            sh.process(domain[i]);
        }
        if let Some(sctx) = ctx {
            for i in 0..cl {
                sh.process(sctx[i]);
            }
        }
        for i in b..2*b {
            sh.process(digest[i]);
        }
        for i in 0..m.len() { 
            sh.process(m[i]);
        }
        sh.shake(&mut h,2*b);
        reverse(2*b,&mut h[0..2*b]);
        return DBIG::frombytes(&h[0..2*b]);
    }
}

// encode integer (little endian)
fn encode_int(x: &BIG,w: &mut [u8]) -> usize {
    let mut index=0;
    if 8*EFS==fp::MODBITS {
        index=1; // extra byte needed for compression
    }
    let b=EFS+index;

    w[0]=0;
    x.tobytearray(w,index);
    reverse(b,w);
    return b;
}

// encode point
 #[allow(non_snake_case)]
fn encode(P: &ECP,w: &mut [u8]) {
    let mut index=0;
    if 8*EFS==fp::MODBITS {
        index=1; // extra byte needed for compression 
    }
    let b=EFS+index;

    let x=P.getx();
    let y=P.gety();
    encode_int(&y,w);
    w[b-1]|=(x.parity()<<7) as u8;
}

// get sign
fn getsign(x: &[u8]) -> isize{
    let mut index=0;
    if 8*EFS==fp::MODBITS {
        index=1; // extra byte needed for compression
    }
    let b=EFS+index;

    if (x[b-1]&0x80)!=0 {
        return 1;
    } else { 
        return 0;
    }
}

// decode integer (little endian)
fn decode_int(strip_sign: bool,ei: &[u8]) -> BIG {
    let mut index=0;
    if 8*EFS==fp::MODBITS {
        index=1; // extra byte needed for compression
    }
    let b=EFS+index;

    let mut r: [u8; EFS+1] = [0; EFS+1];

    for i in 0..b {
        r[i]=ei[i];
    }
    reverse(b,&mut r);

    if strip_sign {
        r[0]&=0x7f;
    }
    return BIG::frombytearray(&r,index);
}

// decode compressed point
fn decode(w: &[u8]) -> ECP {
    let sign=getsign(w); // lsb of x
    let y=decode_int(true,w);
    let one = FP::new_int(1);
    let mut hint=FP::new();
    let mut x=FP::new_big(&y); x.sqr(); 
    let mut d=FP::new_copy(&x); 
    x.sub(&one);
    x.norm();
    let mut t = FP::new_big(&BIG::new_ints(&rom::CURVE_B));
    d.mul(&t);
    if ecp::CURVE_A==1 {
        d.sub(&one);
    }
    if ecp::CURVE_A==-1 {
        d.add(&one);
    }
    d.norm();
// inverse square root trick for sqrt(x/d)
    t.copy(&x);
    t.sqr();
    x.mul(&t);
    x.mul(&d);
    if x.qr(Some(&mut hint))!=1 {
        return ECP::new();
    }
    d.copy(&x.sqrt(Some(&hint)));
    x.inverse(Some(&hint));
    x.mul(&d);
    x.mul(&t);
    x.reduce();
    if x.redc().parity()!=sign {
        x.neg();
    }
    x.norm();
    return ECP::new_bigs(&x.redc(),&y);
}

#[allow(non_snake_case)]
fn key_pair_regenerate(d: &[u8], q: &mut [u8]) {
    let mut index=0;
    if 8*EFS==fp::MODBITS {
        index=1; // extra byte needed for compression
    }
    let b=EFS+index;
    let mut G=ECP::generator();

    let mut digest: [u8; 128] = [0; 128];
    h(d,&mut digest);

// reverse bytes for little endian
    reverse(b,&mut digest);
    let mut s=BIG::frombytearray(&digest,index);
    rfc7748(&mut s);
    G.copy(&G.mul(&s));
    encode(&G,q);
}

/* Calculate a public/private EC GF(p) key pair w,s where W=s.G mod EC(p),
 * where s is the secret key and W is the public key
 * and G is fixed generator.
 * If RNG is NULL then the private key is provided externally in s
 * otherwise it is generated randomly internally */
#[allow(non_snake_case)]
pub fn key_pair_generate(rng: Option<&mut RAND>, d: &mut [u8], q: &mut [u8]) -> isize {
    let res = 0;
    let mut index=0;
    if 8*EFS==fp::MODBITS {
        index=1; // extra byte needed for compression
    }
    let b=EFS+index;
    let mut G=ECP::generator();

    if let Some(srng) = rng {
        for i in 0..b {
            d[i]=srng.getbyte();
        }
    } 
    let mut digest: [u8; 128] = [0; 128];
    h(d,&mut digest);

// reverse bytes for little endian
    reverse(b,&mut digest);
    let mut s=BIG::frombytearray(&digest,index);
    rfc7748(&mut s);
    G.copy(&G.mul(&s));
    encode(&G,q);
    return res;
}

// Generate a signature using key pair (d,q) on message m
// Set ph=true if message has already been pre-hashed
// if ph=false, then context should be NULL for ed25519. However RFC8032 mode ed25519ctx is supported by supplying a non-NULL or non-empty context
#[allow(non_snake_case)]
pub fn signature(ph: bool,d: &[u8], ctx: Option<&[u8]>,m: &[u8], sig: &mut [u8]) ->isize {
    let mut digest: [u8; 128] = [0; 128];
    let mut q: [u8; EFS+1] = [0; EFS+1];  // public key
    h(d,&mut digest); // hash of private key
    let mut res = 0;
    let mut index=0;
    if 8*EFS==fp::MODBITS {
        index=1; // extra byte needed for compression
    }
    let b=EFS+index;
    let r = BIG::new_ints(&rom::CURVE_ORDER);

    key_pair_regenerate(d,&mut q);
    let qs=&q[0..b];

    if d.len()!=qs.len() || d.len()!=b {
        res=INVALID_PUBLIC_KEY;
    }
    if res==0 {
        let mut dr=getr(ph,b,&digest,ctx,m);
        let sr=dr.dmod(&r);
        let R=ECP::generator().mul(&sr);
        encode(&R,&mut sig[0..b]);
        reverse(b,&mut digest);
        let mut s=BIG::frombytearray(&digest,index);
        rfc7748(&mut s);
        dr=h2(ph,ctx,sig,&qs,m);
        let sd=dr.dmod(&r);
        encode_int(&BIG::modadd(&sr,&BIG::modmul(&s,&sd,&r),&r),&mut sig[b..2*b]);
    }
    return res;
}

// verify a signature using public key q
// same context (if any) as used for signature
#[allow(non_snake_case)]
pub fn verify(ph: bool, q: &[u8], ctx: Option<&[u8]>,m: &[u8], sig: &[u8]) ->bool {

    let mut index=0;
    if 8*EFS==fp::MODBITS {
        index=1; // extra byte needed for compression
    }
    let b=EFS+index;

    let mut lg=0;
    let mut c=rom::CURVE_COF_I;
    while c!=1 {
        lg+=1;
        c/=2;
    }
    let r = BIG::new_ints(&rom::CURVE_ORDER);
    let mut R=decode(&sig[0..b]);
 
    if R.is_infinity() {
        return false;
    }

    let t=decode_int(false,&sig[b..2*b]);

    if BIG::comp(&t,&r)>=0 {
        return false;
    }   
    let mut du=h2(ph,ctx,&sig,q,m);
    let su=du.dmod(&r);

    let mut G=ECP::generator();
    let mut QD=decode(&q); 
    if QD.is_infinity() {
        return false;
    }
    QD.neg();
    for _ in 0..lg { // use cofactor 2^c
        G.dbl(); QD.dbl(); R.dbl();
    }

    if !G.mul2(&t,&QD,&su).equals(&R) {
        return false;
    }
    return true;
}
