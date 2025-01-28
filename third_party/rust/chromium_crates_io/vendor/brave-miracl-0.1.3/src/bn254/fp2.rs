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
use crate::bn254::dbig::DBIG;
use crate::bn254::fp;
use crate::bn254::fp::FP;
use crate::bn254::rom;

use crate::rand::RAND;

#[derive(Copy, Clone)]
pub struct FP2 {
    a: FP,
    b: FP,
}

#[cfg(feature = "std")]
impl std::fmt::Debug for FP2 {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}

#[cfg(feature = "std")]
impl std::fmt::Display for FP2 {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}

impl FP2 {
    pub const fn new() -> FP2 {
        FP2 {
            a: FP::new(),
            b: FP::new(),
        }
    }

    pub fn new_int(a: isize) -> FP2 {
        let mut f = FP2::new();
        f.a.copy(&FP::new_int(a));
        f.b.zero();
        f
    }

    pub fn new_ints(a: isize, b: isize) -> FP2 {
        let mut f = FP2::new();
        f.a.copy(&FP::new_int(a));
        f.b.copy(&FP::new_int(b));
        f
    }

    pub fn new_copy(x: &FP2) -> FP2 {
        let mut f = FP2::new();
        f.a.copy(&x.a);
        f.b.copy(&x.b);
        f
    }

    pub fn new_fps(c: &FP, d: &FP) -> FP2 {
        let mut f = FP2::new();
        f.a.copy(c);
        f.b.copy(d);
        f
    }

    pub fn new_bigs(c: &BIG, d: &BIG) -> FP2 {
        let mut f = FP2::new();
        f.a.copy(&FP::new_big(c));
        f.b.copy(&FP::new_big(d));
        f
    }

    pub fn new_fp(c: &FP) -> FP2 {
        let mut f = FP2::new();
        f.a.copy(c);
        f.b.zero();
        f
    }

    pub fn new_big(c: &BIG) -> FP2 {
        let mut f = FP2::new();
        f.a.copy(&FP::new_big(c));
        f.b.zero();
        f
    }

    pub fn new_rand(rng: &mut RAND) -> FP2 {
        FP2::new_fps(&FP::new_rand(rng),&FP::new_rand(rng))
    }

    /* reduce components mod Modulus */
    pub fn reduce(&mut self) {
        self.a.reduce();
        self.b.reduce();
    }

    /* normalise components of w */
    pub fn norm(&mut self) {
        self.a.norm();
        self.b.norm();
    }

    /* test self=0 ? */
    pub fn iszilch(&self) -> bool {
        self.a.iszilch() && self.b.iszilch()
    }

    pub fn islarger(&self) -> isize {
        if self.iszilch() {
            return 0;
        }
        let cmp=self.b.islarger();
        if cmp!=0 {
            return cmp;
        }
        self.a.islarger()
    }

    pub fn tobytes(&self,bf: &mut [u8]) {
        const MB:usize = big::MODBYTES as usize;
        let mut t: [u8; MB] = [0; MB];
        self.b.tobytes(&mut t);
        for i in 0..MB {
            bf[i]=t[i];
        }
        self.a.tobytes(&mut t);
        for i in 0..MB {
            bf[i+MB]=t[i];
        }
    }

    pub fn frombytes(bf: &[u8]) -> FP2 {
        const MB:usize = big::MODBYTES as usize;
        let mut t: [u8; MB] = [0; MB];
        for i in 0..MB {
            t[i]=bf[i];
        }
        let tb=FP::frombytes(&t);
        for i in 0..MB {
            t[i]=bf[i+MB];
        }
        let ta=FP::frombytes(&t);
        FP2::new_fps(&ta,&tb)
    }

    pub fn cmove(&mut self, g: &FP2, d: isize) {
        self.a.cmove(&g.a, d);
        self.b.cmove(&g.b, d);
    }

    /* test self=1 ? */
    pub fn isunity(&self) -> bool {
        let one = FP::new_int(1);
        self.a.equals(&one) && self.b.iszilch()
    }

    /* test self=x */
    pub fn equals(&self, x: &FP2) -> bool {
        self.a.equals(&x.a) && self.b.equals(&x.b)
    }

    /* extract a */
    #[allow(non_snake_case)]
    pub fn getA(&mut self) -> FP {
        self.a
    }

    /* extract b */
    #[allow(non_snake_case)]
    pub fn getB(&mut self) -> FP {
        self.b
    }

    /* extract a */
    pub fn geta(&mut self) -> BIG {
        self.a.redc()
    }

    /* extract b */
    pub fn getb(&mut self) -> BIG {
        self.b.redc()
    }

    /* copy self=x */
    pub fn copy(&mut self, x: &FP2) {
        self.a.copy(&x.a);
        self.b.copy(&x.b);
    }

    pub fn set_fp(&mut self, x: &FP) {
        self.a.copy(x);
        self.b.zero();
    }

    /* set self=0 */
    pub fn zero(&mut self) {
        self.a.zero();
        self.b.zero();
    }

    /* set self=1 */
    pub fn one(&mut self) {
        self.a.one();
        self.b.zero();
    }

    pub fn sign(&self) -> isize {
        let mut p1=self.a.sign();
        let mut p2=self.b.sign();
        if fp::BIG_ENDIAN_SIGN {
            let u=self.b.iszilch() as isize;
	        p2^=(p1^p2)&u;
	        p2
        } else {
            let u=self.a.iszilch() as isize;
	        p1^=(p1^p2)&u;
	        p1
        }
    }

    /* negate self mod Modulus */
    pub fn neg(&mut self) {
        let mut m = FP::new_copy(&self.a);
        let mut t = FP::new();

        m.add(&self.b);
        m.neg();
        t.copy(&m);
        t.add(&self.b);
        self.b.copy(&m);
        self.b.add(&self.a);
        self.a.copy(&t);
    }

    /* set to a-ib */
    pub fn conj(&mut self) {
        self.b.neg();
        self.b.norm();
    }

    /* self+=a */
    pub fn add(&mut self, x: &FP2) {
        self.a.add(&x.a);
        self.b.add(&x.b);
    }

    pub fn dbl(&mut self) {
        self.a.dbl();
        self.b.dbl();
    }

    /* self-=a */
    pub fn sub(&mut self, x: &FP2) {
        let mut m = FP2::new_copy(x);
        m.neg();
        self.add(&m);
    }

    /* self=a-self */
    pub fn rsub(&mut self, x: &FP2) {
        self.neg();
        self.add(x);
    }

    /* self*=s, where s is an FP */
    pub fn pmul(&mut self, s: &FP) {
        self.a.mul(s);
        self.b.mul(s);
    }

    /* self*=i, where i is an int */
    pub fn imul(&mut self, c: isize) {
        self.a.imul(c);
        self.b.imul(c);
    }

    /* self*=self */
    pub fn sqr(&mut self) {
        let mut w1 = FP::new_copy(&self.a);
        let mut w3 = FP::new_copy(&self.a);
        let mut mb = FP::new_copy(&self.b);

        w1.add(&self.b);

        w3.add(&self.a);
        w3.norm();
        self.b.mul(&w3);

        mb.neg();
        self.a.add(&mb);

        w1.norm();
        self.a.norm();

        self.a.mul(&w1);
    }

    /* this*=y */
    pub fn mul(&mut self, y: &FP2) {
        if ((self.a.xes + self.b.xes) as i64) * ((y.a.xes + y.b.xes) as i64) > fp::FEXCESS as i64 {
            if self.a.xes > 1 {
                self.a.reduce()
            }
            if self.b.xes > 1 {
                self.b.reduce()
            }
        }

        let p = BIG::new_ints(&rom::MODULUS);
        let mut pr = DBIG::new();

        pr.ucopy(&p);

        let mut c = BIG::new_copy(&(self.a.x));
        let mut d = BIG::new_copy(&(y.a.x));

        let mut a = BIG::mul(&self.a.x, &y.a.x);
        let mut b = BIG::mul(&self.b.x, &y.b.x);

        c.add(&self.b.x);
        c.norm();
        d.add(&y.b.x);
        d.norm();

        let mut e = BIG::mul(&c, &d);
        let mut f = DBIG::new_copy(&a);
        f.add(&b);
        b.rsub(&pr);

        a.add(&b);
        a.norm();
        e.sub(&f);
        e.norm();

        self.a.x.copy(&FP::modulo(&mut a));
        self.a.xes = 3;
        self.b.x.copy(&FP::modulo(&mut e));
        self.b.xes = 2;
    }
/*
    pub fn pow(&mut self, e: &BIG) {
        let mut w = FP2::new_copy(self);
        let mut z = BIG::new_copy(&e);
        let mut r = FP2::new_int(1);
        loop {
            let bt = z.parity();
            z.fshr(1);
            if bt == 1 {
                r.mul(&mut w)
            };
            if z.iszilch() {
                break;
            }
            w.sqr();
        }
        r.reduce();
        self.copy(&r);
    }*/

    pub fn qr(&mut self,h:Option<&mut FP>) -> isize {
        let mut c=FP2::new_copy(self);
        c.conj();
        c.mul(self);
        c.getA().qr(h)
    }

    /* sqrt(a+ib) = sqrt(a+sqrt(a*a-n*b*b)/2)+ib/(2*sqrt(a+sqrt(a*a-n*b*b)/2)) */
    pub fn sqrt(&mut self,h:Option<&FP>) {
        if self.iszilch() {
            return;
        }
        let mut w1 = FP::new_copy(&self.b);
        let mut w2 = FP::new_copy(&self.a);
        let mut w3 = FP::new_copy(&self.a);
        let mut w4 = FP::new();
        let mut hint = FP::new();

        w1.sqr();
        w2.sqr();
        w1.add(&w2); w1.norm();

        w2.copy(&w1.sqrt(h));
        w1.copy(&w2);

        w2.copy(&self.a);
        w2.add(&w1);
        w2.norm();
        w2.div2();

        w1.copy(&self.b); w1.div2();
        let qr=w2.qr(Some(&mut hint));

// tweak hint
        w3.copy(&hint); w3.neg(); w3.norm();
        w4.copy(&w2); w4.neg(); w4.norm();

        w2.cmove(&w4,1-qr);
        hint.cmove(&w3,1-qr);

        self.a.copy(&w2.sqrt(Some(&hint)));
        w3.copy(&w2); w3.inverse(Some(&hint));
        w3.mul(&self.a);
        self.b.copy(&w3); self.b.mul(&w1);
        w4.copy(&self.a);

        self.a.cmove(&self.b,1-qr);
        self.b.cmove(&w4,1-qr);

/*
        self.a.copy(&w2.sqrt(Some(&hint)));
        w3.copy(&w2); w3.inverse(Some(&hint));
        w3.mul(&self.a);
        self.b.copy(&w3); self.b.mul(&w1);

        hint.neg(); hint.norm();
        w2.neg(); w2.norm();

        w4.copy(&w2.sqrt(Some(&hint)));
        w3.copy(&w2); w3.inverse(Some(&hint));
        w3.mul(&w4);
        w3.mul(&w1);

        self.a.cmove(&w3,1-qr);
        self.b.cmove(&w4,1-qr);
*/
        let sgn=self.sign();
        let mut nr=FP2::new_copy(&self);
        nr.neg(); nr.norm();
        self.cmove(&nr,sgn);
    }

    /* output to hex string */
    #[cfg(feature = "std")]
    pub fn tostring(&self) -> String {
        format!("[{},{}]", self.a.tostring(), self.b.tostring())
    }

    /* self=1/self */
    pub fn inverse(&mut self,h:Option<&FP>) {
        self.norm();
        let mut w1 = FP::new_copy(&self.a);
        let mut w2 = FP::new_copy(&self.b);

        w1.sqr();
        w2.sqr();
        w1.add(&w2);
        w1.inverse(h);
        self.a.mul(&w1);
        w1.neg();
        w1.norm();
        self.b.mul(&w1);
    }

    /* self/=2 */
    pub fn div2(&mut self) {
        self.a.div2();
        self.b.div2();
    }

    /* self*=sqrt(-1) */
    pub fn times_i(&mut self) {
        let z = FP::new_copy(&self.a);
        self.a.copy(&self.b);
        self.a.neg();
        self.b.copy(&z);
    }

    /* w*=(1+sqrt(-1)) */
    /* where X*2-(1+sqrt(-1)) is irreducible for FP4, assumes p=3 mod 8 */
    pub fn mul_ip(&mut self) {
        let mut t = FP2::new_copy(self);
        let mut i = fp::QNRI;
        self.times_i();
        while i > 0 {
            t.dbl();
            t.norm();
            i -= 1;
        }
        self.add(&t);
        if fp::TOWER == fp::POSITOWER {
            self.norm();
            self.neg();
        }
    }

    /* w/=(1+sqrt(-1)) */
    pub fn div_ip(&mut self) {
        let mut z = FP2::new_ints(1 << fp::QNRI, 1);
        z.inverse(None);
        self.norm();
        self.mul(&z);
        if fp::TOWER == fp::POSITOWER {
            self.neg();
            self.norm();
        }
    }
}
