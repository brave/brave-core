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
use crate::bn254::fp::FP;
use crate::bn254::fp;
use crate::bn254::rom;

#[derive(Clone)]
pub struct ECP {
    x: FP,
    y: FP,
    z: FP,
}

#[cfg(feature = "std")]
impl std::fmt::Debug for ECP {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}    

#[cfg(feature = "std")]
impl std::fmt::Display for ECP {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}

pub const WEIERSTRASS: usize = 0;
pub const EDWARDS: usize = 1;
pub const MONTGOMERY: usize = 2;
pub const NOT: usize = 0;
pub const BN: usize = 1;
pub const BLS12: usize = 2;
pub const BLS24: usize = 3;
pub const BLS48: usize = 4;
pub const D_TYPE: usize = 0;
pub const M_TYPE: usize = 1;
pub const POSITIVEX: usize = 0;
pub const NEGATIVEX: usize = 1;

pub const CURVETYPE:usize=WEIERSTRASS;
pub const CURVE_A:isize=0;
pub const CURVE_PAIRING_TYPE:usize=BN;
pub const SEXTIC_TWIST:usize=D_TYPE;
pub const SIGN_OF_X:usize=NEGATIVEX;
pub const ATE_BITS:usize=66;
pub const G2_TABLE:usize=71;
pub const HTC_ISO:usize=0;
pub const HTC_ISO_G2:usize=0;

pub const ALLOW_ALT_COMPRESS:bool=false;
pub const HASH_TYPE:usize=32;
pub const AESKEY:usize=16;

#[allow(non_snake_case)]
impl ECP {
    pub fn pnew() -> ECP {
        ECP {
            x: FP::new(),
            y: FP::new_int(1),
            z: FP::new(),
        }
    }

    pub fn new() -> ECP {
        let mut E = ECP::pnew();
        if CURVETYPE == EDWARDS {
            E.z.one();
        }
        E
    }

    /* set (x,y) from two BIGs */
    pub fn new_bigs(ix: &BIG, iy: &BIG) -> ECP {
        let mut E = ECP::new();
        E.x.bcopy(ix);
        E.y.bcopy(iy);
        E.z.one();
        E.x.norm();
        let rhs = ECP::rhs(&E.x);
        if CURVETYPE == MONTGOMERY {
            if rhs.qr(None) != 1 {
                E.inf();
            }
        } else {
            let mut y2 = FP::new_copy(&E.y);
            y2.sqr();
            if !y2.equals(&rhs) {
                E.inf();
            }
        }
        E
    }

    /* set (x,y) from BIG and a bit */
    pub fn new_bigint(ix: &BIG, s: isize) -> ECP {
        let mut E = ECP::new();
        let mut hint = FP::new();
        E.x.bcopy(ix);
        E.x.norm();
        E.z.one();

        let rhs = ECP::rhs(&E.x);

        if rhs.qr(Some(&mut hint)) == 1 {
            let mut ny = rhs.sqrt(Some(&hint));
            if ny.sign() != s {
                ny.neg(); ny.norm()
            }
            E.y.copy(&ny);
        } else {
            E.inf()
        }
        E
    }

    #[allow(non_snake_case)]
    /* set from x - calculate y from curve equation */
    pub fn new_big(ix: &BIG) -> ECP {
        let mut E = ECP::new();
        let mut hint = FP::new();
        E.x.bcopy(ix);
        E.x.norm();
        E.z.one();
        let rhs = ECP::rhs(&E.x);
        if rhs.qr(Some(&mut hint)) == 1 {
            if CURVETYPE != MONTGOMERY {
                E.y.copy(&rhs.sqrt(Some(&hint)))
            }
        } else {
            E.inf();
        }
        E
    }

    /* set this=O */
    pub fn inf(&mut self) {
        self.x.zero();
        if CURVETYPE != MONTGOMERY {
            self.y.one();
        }
        if CURVETYPE != EDWARDS {
            self.z.zero();
        } else {
            self.z.one()
        }
    }

    /* Calculate RHS of curve equation */
    fn rhs(x: &FP) -> FP {
        let mut r = FP::new_copy(x);
        r.sqr();

        if CURVETYPE == WEIERSTRASS {
            // x^3+Ax+B
            let b = FP::new_big(&BIG::new_ints(&rom::CURVE_B));
            r.mul(x);
            if CURVE_A == -3 {
                let mut cx = FP::new_copy(x);
                cx.imul(3);
                cx.neg();
                cx.norm();
                r.add(&cx);
            }
            r.add(&b);
        }
        if CURVETYPE == EDWARDS {
            // (Ax^2-1)/(Bx^2-1)
            let mut b = FP::new_big(&BIG::new_ints(&rom::CURVE_B));
            let one = FP::new_int(1);
            b.mul(&r);
            b.sub(&one);
            b.norm();
            if CURVE_A == -1 {
                r.neg()
            }
            r.sub(&one);
            r.norm();
            b.inverse(None);
            r.mul(&b);
        }
        if CURVETYPE == MONTGOMERY {
            // x^3+Ax^2+x
            let mut x3 = FP::new();
            x3.copy(&r);
            x3.mul(x);
            r.imul(CURVE_A);
            r.add(&x3);
            r.add(&x);
        }
        r.reduce();
        r
    }

    /* test for O point-at-infinity */
    pub fn is_infinity(&self) -> bool {
        if CURVETYPE == EDWARDS {
            return self.x.iszilch() && self.y.equals(&self.z);
        }
        if CURVETYPE == WEIERSTRASS {
            return self.x.iszilch() && self.z.iszilch();
        }
        if CURVETYPE == MONTGOMERY {
            return self.z.iszilch();
        }
        true
    }

    /* Conditional swap of P and Q dependant on d */
    pub fn cswap(&mut self, Q: &mut ECP, d: isize) {
        self.x.cswap(&mut Q.x, d);
        if CURVETYPE != MONTGOMERY {
            self.y.cswap(&mut Q.y, d)
        }
        self.z.cswap(&mut Q.z, d);
    }

    /* Conditional move of Q to P dependant on d */
    pub fn cmove(&mut self, Q: &ECP, d: isize) {
        self.x.cmove(&Q.x, d);
        if CURVETYPE != MONTGOMERY {
            self.y.cmove(&Q.y, d)
        }
        self.z.cmove(&Q.z, d);
    }

    /* return 1 if b==c, no branching */
    fn teq(b: i32, c: i32) -> isize {
        let mut x = b ^ c;
        x -= 1; // if x=0, x now -1
        ((x >> 31) & 1) as isize
    }

    /* this=P */
    pub fn copy(&mut self, P: &ECP) {
        self.x.copy(&P.x);
        if CURVETYPE != MONTGOMERY {
            self.y.copy(&P.y)
        }
        self.z.copy(&P.z);
    }

    /* this=-this */
    pub fn neg(&mut self) {
        if CURVETYPE == WEIERSTRASS {
            self.y.neg();
            self.y.norm();
        }
        if CURVETYPE == EDWARDS {
            self.x.neg();
            self.x.norm();
        }
    }
    /* multiply x coordinate */
    pub fn mulx(&mut self, c: &mut FP) {
        self.x.mul(c);
    }

    /* Constant time select from pre-computed table */
    fn selector(&mut self, W: &[ECP], b: i32) {
        // unsure about &[& syntax. An array of pointers I hope..
        let mut MP = ECP::new();
        let m = b >> 31;
        let mut babs = (b ^ m) - m;

        babs = (babs - 1) / 2;

        self.cmove(&W[0], ECP::teq(babs, 0)); // conditional move
        self.cmove(&W[1], ECP::teq(babs, 1));
        self.cmove(&W[2], ECP::teq(babs, 2));
        self.cmove(&W[3], ECP::teq(babs, 3));
        self.cmove(&W[4], ECP::teq(babs, 4));
        self.cmove(&W[5], ECP::teq(babs, 5));
        self.cmove(&W[6], ECP::teq(babs, 6));
        self.cmove(&W[7], ECP::teq(babs, 7));

        MP.copy(self);
        MP.neg();
        self.cmove(&MP, (m & 1) as isize);
    }

    /* Test P == Q */
    pub fn equals(&self, Q: &ECP) -> bool {
        let mut a = FP::new();
        let mut b = FP::new();
        a.copy(&self.x);
        a.mul(&Q.z);
        b.copy(&Q.x);
        b.mul(&self.z);
        if !a.equals(&b) {
            return false;
        }
        if CURVETYPE != MONTGOMERY {
            a.copy(&self.y);
            a.mul(&Q.z);
            b.copy(&Q.y);
            b.mul(&self.z);
            if !a.equals(&b) {
                return false;
            }
        }
        true
    }

    /* set to affine - from (x,y,z) to (x,y) */
    pub fn affine(&mut self) {
        if self.is_infinity() {
            return;
        }
        let one = FP::new_int(1);
        if self.z.equals(&one) {
            return;
        }
        self.z.inverse(None);

        self.x.mul(&self.z);
        self.x.reduce();
        if CURVETYPE != MONTGOMERY {
            self.y.mul(&self.z);
            self.y.reduce();
        }
        self.z.copy(&one);
    }

    /* extract x as a BIG */
    pub fn getx(&self) -> BIG {
        let mut W = ECP::new();
        W.copy(self);
        W.affine();
        W.x.redc()
    }

    /* extract y as a BIG */
    pub fn gety(&self) -> BIG {
        let mut W = ECP::new();
        W.copy(self);
        W.affine();
        W.y.redc()
    }

    /* get sign of Y */
    pub fn gets(&self) -> isize {
        let mut W = ECP::new();
        W.copy(self);
        W.affine();
        W.y.sign()
    }

    /* extract x as an FP */
    pub fn getpx(&self) -> FP {
        FP::new_copy(&self.x)
    }
    /* extract y as an FP */
    pub fn getpy(&self) -> FP {
        FP::new_copy(&self.y)
    }

    /* extract z as an FP */
    pub fn getpz(&self) -> FP {
        FP::new_copy(&self.z)
    }

    /* convert to byte array */
    pub fn tobytes(&self, b: &mut [u8], compress: bool) {
        const MB:usize = big::MODBYTES as usize;
        let mut t: [u8; MB] = [0; MB];
        let mut alt=false;
        let mut W = ECP::new();
        W.copy(self);
        W.affine();
        W.x.redc().tobytes(&mut t);

        if CURVETYPE == MONTGOMERY {
            for i in 0..MB {
                b[i] = t[i]
            }
            return;
        }

        if (fp::MODBITS-1)%8 <= 4 && ALLOW_ALT_COMPRESS {
            alt=true;
        }

        if alt {
            for i in 0..MB {
			    b[i]=t[i];
		    }
            if compress {
                b[0]|=0x80;
                if W.y.islarger()==1 {
				    b[0]|=0x20;
			    }
            } else {
                W.y.redc().tobytes(&mut t);
                for i in 0..MB {
				    b[i+MB]=t[i];
			    }
		    }		
        } else {
            for i in 0..MB {
                b[i + 1] = t[i];
            }
            if compress {
                b[0] = 0x02;
                if W.y.sign() == 1 {
                    b[0] = 0x03;
                }
                return;
            }
            b[0] = 0x04;
            W.y.redc().tobytes(&mut t);
            for i in 0..MB {
                b[i + MB + 1] = t[i];
            }
        }
    }

    /* convert from byte array to point */
    pub fn frombytes(b: &[u8]) -> ECP {
        const MB:usize = big::MODBYTES as usize;
        let mut t: [u8; MB] = [0; MB];
        let mut alt=false;
        let p = BIG::new_ints(&rom::MODULUS);

        if CURVETYPE == MONTGOMERY {
            for i in 0..MB {
                t[i] = b[i];
            }
            let px = BIG::frombytes(&t);
            if BIG::comp(&px, &p) >= 0 {
                return ECP::new();
            }
            return ECP::new_big(&px);
        }

        if (fp::MODBITS-1)%8 <= 4 && ALLOW_ALT_COMPRESS {
            alt=true;
        }

        if alt {
            for i in 0..MB {
			    t[i]=b[i];
		    }
            t[0]&=0x1f;
            let px=BIG::frombytes(&t);
            if (b[0]&0x80)==0 {
                for i in 0 ..MB {
				    t[i]=b[i+MB];
			    }
                let py=BIG::frombytes(&t);
                return ECP::new_bigs(&px, &py);
            } else {
                let sgn=(b[0]&0x20)>>5;
                let mut P=ECP::new_bigint(&px,0);
                let cmp=P.y.islarger();
                if (sgn == 1 && cmp != 1) || (sgn == 0 && cmp == 1) {
				    P.neg();
			    }
                return P;
            }
        } else {
            for i in 0..MB {
                t[i] = b[i + 1];
            }
            let px = BIG::frombytes(&t);
            if BIG::comp(&px, &p) >= 0 {
                return ECP::new();
            }
            if b[0] == 0x04 {
                for i in 0..MB {
                    t[i] = b[i + MB + 1];
                }
                let py = BIG::frombytes(&t);
                if BIG::comp(&py, &p) >= 0 {
                    return ECP::new();
                }
                return ECP::new_bigs(&px, &py);
            }
            if b[0] == 0x02 || b[0] == 0x03 {
                return ECP::new_bigint(&px, (b[0] & 1) as isize);
            }
        }

        ECP::new()
    }

    /* convert to hex string */
    #[cfg(feature = "std")]
    pub fn tostring(&self) -> String {
        let mut W = ECP::new();
        W.copy(self);
        W.affine();
        if W.is_infinity() {
            return String::from("infinity");
        }
        if CURVETYPE == MONTGOMERY {
            return format!("({})", W.x.redc().tostring());
        } else {
            return format!("({},{})", W.x.redc().tostring(), W.y.redc().tostring());
        };
    }

    /* this*=2 */
    pub fn dbl(&mut self) {
        if CURVETYPE == WEIERSTRASS {
            if CURVE_A == 0 {
                let mut t0 = FP::new_copy(&self.y);
                t0.sqr();
                let mut t1 = FP::new_copy(&self.y);
                t1.mul(&self.z);
                let mut t2 = FP::new_copy(&self.z);
                t2.sqr();

                self.z.copy(&t0);
                self.z.add(&t0);
                self.z.norm();
                self.z.dbl();
                self.z.dbl();
                self.z.norm();
                t2.imul(3 * rom::CURVE_B_I);

                let mut x3 = FP::new_copy(&t2);
                x3.mul(&self.z);

                let mut y3 = FP::new_copy(&t0);
                y3.add(&t2);
                y3.norm();
                self.z.mul(&t1);
                t1.copy(&t2);
                t1.add(&t2);
                t2.add(&t1);
                t0.sub(&t2);
                t0.norm();
                y3.mul(&t0);
                y3.add(&x3);
                t1.copy(&self.x);
                t1.mul(&self.y);
                self.x.copy(&t0);
                self.x.norm();
                self.x.mul(&t1);
                self.x.dbl();
                self.x.norm();
                self.y.copy(&y3);
                self.y.norm();
            } else {
                let mut t0 = FP::new_copy(&self.x);
                let mut t1 = FP::new_copy(&self.y);
                let mut t2 = FP::new_copy(&self.z);
                let mut t3 = FP::new_copy(&self.x);
                let mut z3 = FP::new_copy(&self.z);
                let mut y3 = FP::new();
                let mut x3 = FP::new();
                let mut b = FP::new();

                if rom::CURVE_B_I == 0 {
                    b.copy(&FP::new_big(&BIG::new_ints(&rom::CURVE_B)));
                }

                t0.sqr(); //1    x^2
                t1.sqr(); //2    y^2
                t2.sqr(); //3

                t3.mul(&self.y); //4
                t3.dbl();
                t3.norm(); //5
                z3.mul(&self.x); //6
                z3.dbl();
                z3.norm(); //7
                y3.copy(&t2);

                if rom::CURVE_B_I == 0 {
                    y3.mul(&b); //8
                } else {
                    y3.imul(rom::CURVE_B_I);
                }

                y3.sub(&z3); //9  ***
                x3.copy(&y3);
                x3.add(&y3);
                x3.norm(); //10

                y3.add(&x3); //11
                x3.copy(&t1);
                x3.sub(&y3);
                x3.norm(); //12
                y3.add(&t1);
                y3.norm(); //13
                y3.mul(&x3); //14
                x3.mul(&t3); //15
                t3.copy(&t2);
                t3.add(&t2); //16
                t2.add(&t3); //17

                if rom::CURVE_B_I == 0 {
                    z3.mul(&b); //18
                } else {
                    z3.imul(rom::CURVE_B_I);
                }

                z3.sub(&t2); //19
                z3.sub(&t0);
                z3.norm(); //20  ***
                t3.copy(&z3);
                t3.add(&z3); //21

                z3.add(&t3);
                z3.norm(); //22
                t3.copy(&t0);
                t3.add(&t0); //23
                t0.add(&t3); //24
                t0.sub(&t2);
                t0.norm(); //25

                t0.mul(&z3); //26
                y3.add(&t0); //27
                t0.copy(&self.y);
                t0.mul(&self.z); //28
                t0.dbl();
                t0.norm(); //29
                z3.mul(&t0); //30
                x3.sub(&z3); //31
                t0.dbl();
                t0.norm(); //32
                t1.dbl();
                t1.norm(); //33
                z3.copy(&t0);
                z3.mul(&t1); //34

                self.x.copy(&x3);
                self.x.norm();
                self.y.copy(&y3);
                self.y.norm();
                self.z.copy(&z3);
                self.z.norm();
            }
        }
        if CURVETYPE == EDWARDS {
            let mut c = FP::new_copy(&self.x);
            let mut d = FP::new_copy(&self.y);
            let mut h = FP::new_copy(&self.z);
            let mut j = FP::new();

            self.x.mul(&self.y);
            self.x.dbl();
            self.x.norm();
            c.sqr();
            d.sqr();
            if CURVE_A == -1 {
                c.neg()
            }
            self.y.copy(&c);
            self.y.add(&d);
            self.y.norm();
            h.sqr();
            h.dbl();
            self.z.copy(&self.y);
            j.copy(&self.y);
            j.sub(&h);
            j.norm();
            self.x.mul(&j);
            c.sub(&d);
            c.norm();
            self.y.mul(&c);
            self.z.mul(&j);
        }
        if CURVETYPE == MONTGOMERY {
            let mut a = FP::new_copy(&self.x);
            let mut b = FP::new_copy(&self.x);
            let mut aa = FP::new();
            let mut bb = FP::new();
            let mut c = FP::new();

            a.add(&self.z);
            a.norm();
            aa.copy(&a);
            aa.sqr();
            b.sub(&self.z);
            b.norm();
            bb.copy(&b);
            bb.sqr();
            c.copy(&aa);
            c.sub(&bb);
            c.norm();

            self.x.copy(&aa);
            self.x.mul(&bb);

            a.copy(&c);
            a.imul((CURVE_A + 2) / 4);

            bb.add(&a);
            bb.norm();
            self.z.copy(&bb);
            self.z.mul(&c);
        }
    }

    /* self+=Q */
    pub fn add(&mut self, Q: &ECP) {
        if CURVETYPE == WEIERSTRASS {
            if CURVE_A == 0 {
                let b = 3 * rom::CURVE_B_I;
                let mut t0 = FP::new_copy(&self.x);
                t0.mul(&Q.x);
                let mut t1 = FP::new_copy(&self.y);
                t1.mul(&Q.y);
                let mut t2 = FP::new_copy(&self.z);
                t2.mul(&Q.z);
                let mut t3 = FP::new_copy(&self.x);
                t3.add(&self.y);
                t3.norm();
                let mut t4 = FP::new_copy(&Q.x);
                t4.add(&Q.y);
                t4.norm();
                t3.mul(&t4);
                t4.copy(&t0);
                t4.add(&t1);

                t3.sub(&t4);
                t3.norm();
                t4.copy(&self.y);
                t4.add(&self.z);
                t4.norm();
                let mut x3 = FP::new_copy(&Q.y);
                x3.add(&Q.z);
                x3.norm();

                t4.mul(&x3);
                x3.copy(&t1);
                x3.add(&t2);

                t4.sub(&x3);
                t4.norm();
                x3.copy(&self.x);
                x3.add(&self.z);
                x3.norm();
                let mut y3 = FP::new_copy(&Q.x);
                y3.add(&Q.z);
                y3.norm();
                x3.mul(&y3);
                y3.copy(&t0);
                y3.add(&t2);
                y3.rsub(&x3);
                y3.norm();
                x3.copy(&t0);
                x3.add(&t0);
                t0.add(&x3);
                t0.norm();
                t2.imul(b);

                let mut z3 = FP::new_copy(&t1);
                z3.add(&t2);
                z3.norm();
                t1.sub(&t2);
                t1.norm();
                y3.imul(b);

                x3.copy(&y3);
                x3.mul(&t4);
                t2.copy(&t3);
                t2.mul(&t1);
                x3.rsub(&t2);
                y3.mul(&t0);
                t1.mul(&z3);
                y3.add(&t1);
                t0.mul(&t3);
                z3.mul(&t4);
                z3.add(&t0);

                self.x.copy(&x3);
                self.x.norm();
                self.y.copy(&y3);
                self.y.norm();
                self.z.copy(&z3);
                self.z.norm();
            } else {
                let mut t0 = FP::new_copy(&self.x);
                let mut t1 = FP::new_copy(&self.y);
                let mut t2 = FP::new_copy(&self.z);
                let mut t3 = FP::new_copy(&self.x);
                let mut t4 = FP::new_copy(&Q.x);
                let mut z3 = FP::new();
                let mut y3 = FP::new_copy(&Q.x);
                let mut x3 = FP::new_copy(&Q.y);
                let mut b = FP::new();

                if rom::CURVE_B_I == 0 {
                    b.copy(&FP::new_big(&BIG::new_ints(&rom::CURVE_B)));
                }

                t0.mul(&Q.x); //1
                t1.mul(&Q.y); //2
                t2.mul(&Q.z); //3

                t3.add(&self.y);
                t3.norm(); //4
                t4.add(&Q.y);
                t4.norm(); //5
                t3.mul(&t4); //6
                t4.copy(&t0);
                t4.add(&t1); //7
                t3.sub(&t4);
                t3.norm(); //8
                t4.copy(&self.y);
                t4.add(&self.z);
                t4.norm(); //9
                x3.add(&Q.z);
                x3.norm(); //10
                t4.mul(&x3); //11
                x3.copy(&t1);
                x3.add(&t2); //12

                t4.sub(&x3);
                t4.norm(); //13
                x3.copy(&self.x);
                x3.add(&self.z);
                x3.norm(); //14
                y3.add(&Q.z);
                y3.norm(); //15

                x3.mul(&y3); //16
                y3.copy(&t0);
                y3.add(&t2); //17

                y3.rsub(&x3);
                y3.norm(); //18
                z3.copy(&t2);

                if rom::CURVE_B_I == 0 {
                    z3.mul(&b); //18
                } else {
                    z3.imul(rom::CURVE_B_I);
                }

                x3.copy(&y3);
                x3.sub(&z3);
                x3.norm(); //20
                z3.copy(&x3);
                z3.add(&x3); //21

                x3.add(&z3); //22
                z3.copy(&t1);
                z3.sub(&x3);
                z3.norm(); //23
                x3.add(&t1);
                x3.norm(); //24

                if rom::CURVE_B_I == 0 {
                    y3.mul(&b); //18
                } else {
                    y3.imul(rom::CURVE_B_I);
                }

                t1.copy(&t2);
                t1.add(&t2); //t1.norm();//26
                t2.add(&t1); //27

                y3.sub(&t2); //28

                y3.sub(&t0);
                y3.norm(); //29
                t1.copy(&y3);
                t1.add(&y3); //30
                y3.add(&t1);
                y3.norm(); //31

                t1.copy(&t0);
                t1.add(&t0); //32
                t0.add(&t1); //33
                t0.sub(&t2);
                t0.norm(); //34
                t1.copy(&t4);
                t1.mul(&y3); //35
                t2.copy(&t0);
                t2.mul(&y3); //36
                y3.copy(&x3);
                y3.mul(&z3); //37
                y3.add(&t2); //y3.norm();//38
                x3.mul(&t3); //39
                x3.sub(&t1); //40
                z3.mul(&t4); //41
                t1.copy(&t3);
                t1.mul(&t0); //42
                z3.add(&t1);
                self.x.copy(&x3);
                self.x.norm();
                self.y.copy(&y3);
                self.y.norm();
                self.z.copy(&z3);
                self.z.norm();
            }
        }
        if CURVETYPE == EDWARDS {
            let bb = FP::new_big(&BIG::new_ints(&rom::CURVE_B));
            let mut a = FP::new_copy(&self.z);
            let mut b = FP::new();
            let mut c = FP::new_copy(&self.x);
            let mut d = FP::new_copy(&self.y);
            let mut e = FP::new();
            let mut f = FP::new();
            let mut g = FP::new();

            a.mul(&Q.z);
            b.copy(&a);
            b.sqr();
            c.mul(&Q.x);
            d.mul(&Q.y);

            e.copy(&c);
            e.mul(&d);
            e.mul(&bb);
            f.copy(&b);
            f.sub(&e);
            g.copy(&b);
            g.add(&e);

            if CURVE_A == 1 {
                e.copy(&d);
                e.sub(&c);
            }
            c.add(&d);

            b.copy(&self.x);
            b.add(&self.y);
            d.copy(&Q.x);
            d.add(&Q.y);
            b.norm();
            d.norm();
            b.mul(&d);
            b.sub(&c);
            b.norm();
            f.norm();
            b.mul(&f);
            self.x.copy(&a);
            self.x.mul(&b);
            g.norm();
            if CURVE_A == 1 {
                e.norm();
                c.copy(&e);
                c.mul(&g);
            }
            if CURVE_A == -1 {
                c.norm();
                c.mul(&g);
            }
            self.y.copy(&a);
            self.y.mul(&c);
            self.z.copy(&f);
            self.z.mul(&g);
        }
    }

    /* Differential Add for Montgomery curves. this+=Q where W is this-Q and is affine. */
    pub fn dadd(&mut self, Q: &ECP, W: &ECP) {
        let mut a = FP::new_copy(&self.x);
        let mut b = FP::new_copy(&self.x);
        let mut c = FP::new_copy(&Q.x);
        let mut d = FP::new_copy(&Q.x);
        let mut da = FP::new();
        let mut cb = FP::new();

        a.add(&self.z);
        b.sub(&self.z);

        c.add(&Q.z);
        d.sub(&Q.z);

        a.norm();
        d.norm();

        da.copy(&d);
        da.mul(&a);

        c.norm();
        b.norm();

        cb.copy(&c);
        cb.mul(&b);

        a.copy(&da);
        a.add(&cb);
        a.norm();
        a.sqr();
        b.copy(&da);
        b.sub(&cb);
        b.norm();
        b.sqr();

        self.x.copy(&a);
        self.z.copy(&W.x);
        self.z.mul(&b);
    }

    /* self-=Q */
    pub fn sub(&mut self, Q: &ECP) {
        let mut NQ = ECP::new();
        NQ.copy(Q);
        NQ.neg();
        self.add(&NQ);
    }

    /* constant time multiply by small integer of length bts - use ladder */
    pub fn pinmul(&self, e: i32, bts: i32) -> ECP {
        if CURVETYPE == MONTGOMERY {
            self.mul(&BIG::new_int(e as isize))
        } else {
            let mut P = ECP::new();
            let mut R0 = ECP::new();
            let mut R1 = ECP::new();
            R1.copy(&self);

            for i in (0..bts).rev() {
                let b = ((e >> i) & 1) as isize;
                P.copy(&R1);
                P.add(&R0);
                R0.cswap(&mut R1, b);
                R1.copy(&P);
                R0.dbl();
                R0.cswap(&mut R1, b);
            }
            P.copy(&R0);
            P
        }
    }

// Point multiplication, multiplies a point P by a scalar e
// This code has no inherent awareness of the order of the curve, or the order of the point.
// The order of the curve will be h.r, where h is a cofactor, and r is a large prime
// Typically P will be of order r (but not always), and typically e will be less than r (but not always)
// A problem can arise if a secret e is a few bits less than r, as the leading zeros in e will leak via a timing attack
// The secret e may however be greater than r (see RFC7748 which combines elimination of a small cofactor h with the point multiplication, using an e>r)
// Our solution is to use as a multiplier an e, whose length in bits is that of the logical OR of e and r, hence allowing e>r while forcing inclusion of leading zeros if e<r. 
// The point multiplication methods used will process leading zeros correctly.

// So this function leaks information about the length of e...
    pub fn mul(&self, e: &BIG) -> ECP {
        return self.clmul(e,e);
    }

// .. but this one does not (typically set maxe=r)
// Set P=e*P 
    pub fn clmul(&self, e: &BIG, maxe: &BIG) -> ECP {
        if e.iszilch() || self.is_infinity() {
            return ECP::new();
        }
        let mut P = ECP::new();
        let mut cm = BIG::new_copy(e); cm.or(maxe);
        let max=cm.nbits();
        
        if CURVETYPE == MONTGOMERY {
            /* use Ladder */
            let mut D = ECP::new();
            let mut R0 = ECP::new();
            R0.copy(&self);
            let mut R1 = ECP::new();
            R1.copy(&self);
            R1.dbl();
            D.copy(&self); D.affine();
            let nb = max;

            for i in (0..nb - 1).rev() {
                let b = e.bit(i);
                P.copy(&R1);
                P.dadd(&R0, &D);
                R0.cswap(&mut R1, b);
                R1.copy(&P);
                R0.dbl();
                R0.cswap(&mut R1, b);
            }
            P.copy(&R0)
        } else {
            // fixed size windows
            let mut mt = BIG::new();
            let mut t = BIG::new();
            let mut Q = ECP::new();
            let mut C = ECP::new();

            let mut W: [ECP; 8] = [
                ECP::new(),
                ECP::new(),
                ECP::new(),
                ECP::new(),
                ECP::new(),
                ECP::new(),
                ECP::new(),
                ECP::new(),
            ];

            const CT: usize = 1 + (big::NLEN * (big::BASEBITS as usize) + 3) / 4;
            let mut w: [i8; CT] = [0; CT];

            Q.copy(&self);
            Q.dbl();

            W[0].copy(&self);

            for i in 1..8 {
                C.copy(&W[i - 1]);
                W[i].copy(&C);
                W[i].add(&Q);
            }

            // make exponent odd - add 2P if even, P if odd
            t.copy(&e);
            let s = t.parity();
            t.inc(1);
            t.norm();
            let ns = t.parity();
            mt.copy(&t);
            mt.inc(1);
            mt.norm();
            t.cmove(&mt, s);
            Q.cmove(&self, ns);
            C.copy(&Q);

            let nb = 1 + (max + 3) / 4;

            // convert exponent to signed 4-bit window
            for i in 0..nb {
                w[i] = (t.lastbits(5) - 16) as i8;
                t.dec(w[i] as isize);
                t.norm();
                t.fshr(4);
            }
            w[nb] = t.lastbits(5) as i8;

            //P.copy(&W[((w[nb] as usize) - 1) / 2]);

            P.selector(&W, w[nb] as i32);
            for i in (0..nb).rev() {
                Q.selector(&W, w[i] as i32);
                P.dbl();
                P.dbl();
                P.dbl();
                P.dbl();
                P.add(&Q);
            }
            P.sub(&C); /* apply correction */
        }
        P
    }

// Generic multi-multiplication, fixed 4-bit window, P=Sigma e_i*X_i
    pub fn muln(n: usize, X: &[ECP], e: &[BIG]) -> ECP {
        let mut B: [ECP; 16] = [
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
        ];    
        let mut mt = BIG::new();
        let mut t = BIG::new();
        let mut P = ECP::new();
        let mut S = ECP::new();
        let mut R = ECP::new();

        mt.copy(&e[0]); mt.norm();
        for i in 1..n { // find biggest
            t.copy(&e[i]); t.norm();
            let k=BIG::comp(&t,&mt);
            mt.cmove(&t,(k+1)/2);
        }
        let nb=(mt.nbits()+3)/4;
        for i in (0..nb).rev() { // Pippenger's algorithm
            for j in 0..16 {
                B[j].inf();
            }
            for j in 0..n { 
                mt.copy(&e[j]); mt.norm();
                mt.shr((i*4) as usize);
                let k=mt.lastbits(4) as usize;
                B[k].add(&X[j]);
            }
            R.inf(); S.inf();
            for j in (1..16).rev() {
                R.add(&B[j]);
                S.add(&R);
            }
            for _ in 0..4 {
                P.dbl();
            }
            P.add(&S);
        }
        P
    }

    /* Return e.this+f.Q */

    pub fn mul2(&self, e: &BIG, Q: &ECP, f: &BIG) -> ECP {
        let mut te = BIG::new();
        let mut tf = BIG::new();
        let mut mt = BIG::new();
        let mut S = ECP::new();
        let mut T = ECP::new();
        let mut C = ECP::new();

        let mut W: [ECP; 8] = [
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
            ECP::new(),
        ];

        const CT: usize = 1 + (big::NLEN * (big::BASEBITS as usize) + 1) / 2;
        let mut w: [i8; CT] = [0; CT];

        te.copy(e);
        tf.copy(f);

        // precompute table

        W[1].copy(&self);
        W[1].sub(Q);
        W[2].copy(&self);
        W[2].add(Q);
        S.copy(&Q);
        S.dbl();
        C.copy(&W[1]);
        W[0].copy(&C);
        W[0].sub(&S); // copy to C is stupid Rust thing..
        C.copy(&W[2]);
        W[3].copy(&C);
        W[3].add(&S);
        T.copy(&self);
        T.dbl();
        C.copy(&W[1]);
        W[5].copy(&C);
        W[5].add(&T);
        C.copy(&W[2]);
        W[6].copy(&C);
        W[6].add(&T);
        C.copy(&W[5]);
        W[4].copy(&C);
        W[4].sub(&S);
        C.copy(&W[6]);
        W[7].copy(&C);
        W[7].add(&S);

        // if multiplier is odd, add 2, else add 1 to multiplier, and add 2P or P to correction

        let mut s = te.parity();
        te.inc(1);
        te.norm();
        let mut ns = te.parity();
        mt.copy(&te);
        mt.inc(1);
        mt.norm();
        te.cmove(&mt, s);
        T.cmove(&self, ns);
        C.copy(&T);

        s = tf.parity();
        tf.inc(1);
        tf.norm();
        ns = tf.parity();
        mt.copy(&tf);
        mt.inc(1);
        mt.norm();
        tf.cmove(&mt, s);
        S.cmove(&Q, ns);
        C.add(&S);

        mt.copy(&te);
        mt.add(&tf);
        mt.norm();
        let nb = 1 + (mt.nbits() + 1) / 2;

        // convert exponent to signed 2-bit window
        for i in 0..nb {
            let a = te.lastbits(3) - 4;
            te.dec(a);
            te.norm();
            te.fshr(2);
            let b = tf.lastbits(3) - 4;
            tf.dec(b);
            tf.norm();
            tf.fshr(2);
            w[i] = (4 * a + b) as i8;
        }
        w[nb] = (4 * te.lastbits(3) + tf.lastbits(3)) as i8;
        //S.copy(&W[((w[nb] as usize) - 1) / 2]);
        S.selector(&W, w[nb] as i32);

        for i in (0..nb).rev() {
            T.selector(&W, w[i] as i32);
            S.dbl();
            S.dbl();
            S.add(&T);
        }
        S.sub(&C); /* apply correction */
        S
    }

    pub fn cfp(&mut self) {
        let cf = rom::CURVE_COF_I;
        if cf == 1 {
            return;
        }
        if cf == 4 {
            self.dbl();
            self.dbl();
            return;
        }
        if cf == 8 {
            self.dbl();
            self.dbl();
            self.dbl();
            return;
        }
        let c = BIG::new_ints(&rom::CURVE_COF);
        let P = self.mul(&c);
        self.copy(&P);
    }

/* Hunt and Peck a BIG to a curve point */
    #[allow(non_snake_case)]
    pub fn hap2point(h: &BIG) -> ECP {
        let mut P: ECP;
        let mut x =BIG::new_copy(&h);
        loop {
            if CURVETYPE != MONTGOMERY {
                P = ECP::new_bigint(&x, 0);
            } else {
                P = ECP::new_big(&x);
            }
            x.inc(1);
            x.norm();
            if !P.is_infinity() {
                break;
            }
        }
        P
    }

/* Constant time Map to Point */
    #[allow(non_snake_case)]
    pub fn map2point(h: &FP) -> ECP {
        let mut P = ECP::new();

        if CURVETYPE == MONTGOMERY {
        // Elligator 2
            let mut X1=FP::new();
            let mut X2=FP::new();
            let mut t =FP::new_copy(h);
            let mut w =FP::new();
            let one=FP::new_int(1);
            let A=FP::new_int(CURVE_A);
            let mut N =FP::new();
            let mut D =FP::new();
            let mut hint =FP::new();

            t.sqr();   // t^2

            if fp::PM1D2 == 2 {
                t.dbl();    // 2t^2
            }
            if fp::PM1D2 == 1 {
                t.neg();    // -t^2
            }
            if fp::PM1D2 > 2 {
                t.imul(fp::QNRI as isize);   // precomputed QNR
            }

            t.norm();
            D.copy(&t); D.add(&one); D.norm();  // Denominator D=1+z.t^2

            X1.copy(&A);
            X1.neg(); X1.norm();                // X1=-A/D
            X2.copy(&X1);
            X2.mul(&t);                         // X2=-At/D

            w.copy(&X1); w.sqr(); N.copy(&w); N.mul(&X1);
            w.mul(&A); w.mul(&D); N.add(&w); 
            t.copy(&D); t.sqr();
            t.mul(&X1);
            N.add(&t); N.norm();        // Numerator=x^3+ADx^2+D^2x

            t.copy(&N); t.mul(&D);      // N*D
            let qres=t.qr(Some(&mut hint)); // only exp
            w.copy(&t); w.inverse(Some(&hint));
            D.copy(&w); D.mul(&N);          // 1/D
            X1.mul(&D);                     // get X1
            X2.mul(&D);                     // get X2
            X1.cmove(&X2,1-qres);

            let a=X1.redc();
            P.copy(&ECP::new_big(&a));

        }
        if CURVETYPE == EDWARDS {
// Elligator 2 - map to Montgomery, place point, map back
            let mut X1=FP::new();
            let mut X2=FP::new();
            let mut t=FP::new_copy(h);
            let mut w=FP::new();
            let one=FP::new_int(1);
            let mut A=FP::new();
            let mut w1=FP::new();
            let mut w2=FP::new();
            let mut B = FP::new_big(&BIG::new_ints(&rom::CURVE_B));
            let mut Y=FP::new();
            let mut K=FP::new();
            let mut D=FP::new();
            let mut hint=FP::new();
            //let mut Y3=FP::new();
            let rfc: isize;

            if fp::MODTYPE != fp::GENERALISED_MERSENNE {
                A.copy(&B);
                if CURVE_A==1 {
                    A.add(&one);        // A=B+1
                    B.sub(&one);        // B=B-1
                } else {
                    A.sub(&one);        // A=B-1
                    B.add(&one);        // B=B+1
                }
                A.norm(); B.norm();

                A.div2();               // (A+B)/2 = J/K
                B.div2();               // (B-A)/2
                B.div2();               // (B-A)/4 = -1/K

                K.copy(&B);
                K.neg(); K.norm();
 
                K.invsqrt(&mut w2,&mut w1);  // return K, sqrt(1/K) - could be precalculated!
                K.copy(&w2);
                rfc=fp::RIADZ;
                if rfc==1 {
                    A.mul(&K);
                    K.mul(&w1);
                } else {
                    B.sqr();
                }
            } else {
                rfc=1;
                A.copy(&FP::new_int(156326));
            }
// Map to this Montgomery curve X^2=X^3+AX^2+BX
            t.sqr();        // t^2
            let mut qnr=0;
            if fp::PM1D2 == 2 {
                t.dbl();
                qnr=2;
            }
            if fp::PM1D2 == 1 {
                t.neg();
                qnr = -1;
            }
            if fp::PM1D2 > 2 {
                t.imul(fp::QNRI as isize);  // precomputed QNR
                qnr=fp::QNRI as isize;
            }
            t.norm();

            D.copy(&t); D.add(&one); D.norm();  // Denominator=(1+z.u^2)
            X1.copy(&A);
            X1.neg(); X1.norm();                // X1=-(J/K).inv(1+z.u^2)
            X2.copy(&X1); X2.mul(&t);           // X2=X1*z*u^2

// Figure out RHS of Montgomery curve in rational form gx1/d^3

            w.copy(&X1); w.sqr(); w1.copy(&w); w1.mul(&X1);
            w.mul(&A); w.mul(&D); w1.add(&w);
            w2.copy(&D); w2.sqr();

            if rfc==0 {
                w.copy(&X1); w.mul(&B);
                w2.mul(&w);
                w1.add(&w2);        // w1=X1^3+ADX1^2+BD^2X1
            } else {
                w2.mul(&X1);
                w1.add(&w2);        // w1=X1^3+ADX1^2+D^2X1  
            }
            w1.norm();

            B.copy(&w1); B.mul(&D);   // gx1=num/den^3 - is_qr num*den (same as num/den, same as num/den^3)
            let qres=B.qr(Some(&mut hint));   // Exponentiation
            w.copy(&B); w.inverse(Some(&hint));
            D.copy(&w); D.mul(&w1);             // 1/D
            X1.mul(&D);                         // get X1
            X2.mul(&D);                         // get X2
            D.sqr();

            w1.copy(&B); w1.imul(qnr);
            w.copy(&FP::new_big(&BIG::new_ints(&rom::CURVE_HTPC)));
            w.mul(&hint);
            w2.copy(&D); w2.mul(&h);

            X1.cmove(&X2,1-qres);
            B.cmove(&w1,1-qres);
            hint.cmove(&w,1-qres);
            D.cmove(&w2,1-qres);

            Y.copy(&B.sqrt(Some(&hint)));
            Y.mul(&D);
/*
            Y.copy(&B.sqrt(Some(&hint)));       // sqrt(num*den)
            Y.mul(&D);                          // sqrt(num/den^3)

            B.imul(qnr);                        // now for gx2 = Z.u^2.gx1
            w.copy(&FP::new_big(&BIG::new_ints(&rom::CURVE_HTPC))); // qnr^C3 
            hint.mul(&w);                       // modify hint for gx2

            Y3.copy(&B.sqrt(Some(&hint)));      // second candidate
            D.mul(&h);
            Y3.mul(&D);

            X1.cmove(&X2,1-qres);               // pick correct one
            Y.cmove(&Y3,1-qres);
*/
// correct sign of Y
            w.copy(&Y); w.neg(); w.norm();
            Y.cmove(&w,qres^Y.sign());

            if rfc==0 {
                X1.mul(&K);
                Y.mul(&K);
            }

            if fp::MODTYPE == fp::GENERALISED_MERSENNE {
            // GOLDILOCKS isogeny
				t.copy(&X1); t.sqr();
				w.copy(&t); w.add(&one); w.norm();
				t.sub(&one); t.norm();
				w1.copy(&t); w1.mul(&Y);
				w1.dbl(); X2.copy(&w1); X2.add(&w1); X2.norm();
				t.sqr();
				Y.sqr(); Y.dbl(); Y.dbl(); Y.norm();
				B.copy(&t); B.add(&Y); B.norm();

				w2.copy(&Y); w2.sub(&t); w2.norm();
				w2.mul(&X1);
				t.mul(&X1);
				Y.div2();
				w1.copy(&Y); w1.mul(&w);
				w1.rsub(&t); w1.norm();
 
				t.copy(&X2); t.mul(&w1);    // output in projective to avoid inversion
				P.x.copy(&t);
				t.copy(&w2); t.mul(&B);
				P.y.copy(&t);
				t.copy(&w1); t.mul(&B);
				P.z.copy(&t);

				return P;
            } else {
                w1.copy(&X1); w1.add(&one); w1.norm();  // s+1
                w2.copy(&X1); w2.sub(&one); w2.norm();  // s-1
                t.copy(&w1); t.mul(&Y);  
                X1.mul(&w1);
            
                if rfc==1 {
                    X1.mul(&K);
                }
                Y.mul(&w2);     // output in projective to avoid inversion
                P.x.copy(&X1);
                P.y.copy(&Y);
                P.z.copy(&t);

                return P               
            }
        }
        if CURVETYPE==WEIERSTRASS {
        // SSWU or SVDW method
            let mut A=FP::new();
            let mut B=FP::new();
            let mut X1=FP::new();
            let mut X2=FP::new();
            let mut X3=FP::new();
            let one=FP::new_int(1);
            let mut Y=FP::new();
            let mut D=FP::new();
            let mut t=FP::new_copy(h);
            let mut w=FP::new();
            let mut D2=FP::new();
            let mut hint=FP::new();
            let mut GX1=FP::new();
            //let mut Y3=FP::new();

            let sgn=t.sign();

            if CURVE_A != 0 || HTC_ISO != 0
            { // Map to point on isogenous curve
                if HTC_ISO != 0 {
/* CAHCZS
                    A.copy(&FP::new_big(&BIG::new_ints(&rom::CURVE_AD)));
                    B.copy(&FP::new_big(&BIG::new_ints(&rom::CURVE_BD)));
CAHCZF */
                } else {
                    A.copy(&FP::new_int(CURVE_A));
                    B.copy(&FP::new_big(&BIG::new_ints(&rom::CURVE_B)));
                }
                // SSWU Method
                t.sqr();
                t.imul(fp::RIADZ);      // Z from hash-to-point draft standard
                w.copy(&t); w.add(&one); w.norm();

                w.mul(&t); D.copy(&A);
                D.mul(&w);
            
                w.add(&one); w.norm();
                w.mul(&B);
                w.neg(); w.norm();

                X2.copy(&w); 
                X3.copy(&t); X3.mul(&X2);

// x^3+Ad^2x+Bd^3
                GX1.copy(&X2); GX1.sqr(); D2.copy(&D);
                D2.sqr(); w.copy(&A); w.mul(&D2); GX1.add(&w); GX1.norm(); GX1.mul(&X2); D2.mul(&D); w.copy(&B); w.mul(&D2); GX1.add(&w); GX1.norm();

                w.copy(&GX1); w.mul(&D);
                let qr=w.qr(Some(&mut hint));
                D.copy(&w); D.inverse(Some(&hint));
                D.mul(&GX1);
                X2.mul(&D);
                X3.mul(&D);
                t.mul(h);
                D2.copy(&D); D2.sqr();

                D.copy(&D2); D.mul(&t);
                t.copy(&w); t.imul(fp::RIADZ);
                X1.copy(&FP::new_big(&BIG::new_ints(&rom::CURVE_HTPC)));
                X1.mul(&hint);

                X2.cmove(&X3,1-qr);
                D2.cmove(&D,1-qr);
                w.cmove(&t,1-qr);
                hint.cmove(&X1,1-qr);

                Y.copy(&w.sqrt(Some(&hint)));
                Y.mul(&D2);
/*
                Y.copy(&w.sqrt(Some(&hint)));
                Y.mul(&D2);

                D2.mul(&t);
                w.imul(fp::RIADZ);

                X1.copy(&FP::new_big(&BIG::new_ints(&rom::CURVE_HTPC)));
                hint.mul(&X1);
                
                Y3.copy(&w.sqrt(Some(&hint)));
                Y3.mul(&D2);

                X2.cmove(&X3,1-qr);
                Y.cmove(&Y3,1-qr);
*/
                let ne=Y.sign()^sgn;
                w.copy(&Y); w.neg(); w.norm();
                Y.cmove(&w,ne);

                if HTC_ISO != 0 {
 
/* CAHCZS
                    let mut k=0;
                    let isox=HTC_ISO;
                    let isoy=3*(isox-1)/2;
                // xnum
                    let mut xnum=FP::new_big(&BIG::new_ints(&rom::PC[k])); k+=1;
                    for _ in 0..isox {
                        xnum.mul(&X2);
                        w.copy(&FP::new_big(&BIG::new_ints(&rom::PC[k]))); k+=1;
                        xnum.add(&w); xnum.norm();
                    }
                // xden
                    let mut xden=FP::new_copy(&X2);
                    w.copy(&FP::new_big(&BIG::new_ints(&rom::PC[k]))); k+=1;
                    xden.add(&w); xden.norm();
                    for _ in 0..isox-2 {
                        xden.mul(&X2);
                        w.copy(&FP::new_big(&BIG::new_ints(&rom::PC[k]))); k+=1;
                        xden.add(&w); xden.norm();
                    }
                // ynum
                    let mut ynum=FP::new_big(&BIG::new_ints(&rom::PC[k])); k+=1;
                    for _ in 0..isoy {
                        ynum.mul(&X2);
                        w.copy(&FP::new_big(&BIG::new_ints(&rom::PC[k]))); k+=1;
                        ynum.add(&w); ynum.norm();
                    }
                // yden
                    let mut yden=FP::new_copy(&X2);
                    w.copy(&FP::new_big(&BIG::new_ints(&rom::PC[k]))); k+=1;
                    yden.add(&w); yden.norm();
                    for _ in 0..isoy-1 {
                        yden.mul(&X2);
                        w.copy(&FP::new_big(&BIG::new_ints(&rom::PC[k]))); k+=1;
                        yden.add(&w); yden.norm();
                    }  
                    ynum.mul(&Y);
                    w.copy(&xnum); w.mul(&yden);
                    P.x.copy(&w);
                    w.copy(&ynum); w.mul(&xden);
                    P.y.copy(&w);
                    w.copy(&xden); w.mul(&yden);
                    P.z.copy(&w);
                    return P;
CAHCZF */
                } else {
                    let x=X2.redc();
                    let y=Y.redc();
                    P.copy(&ECP::new_bigs(&x,&y));
                    return P;
                }
            } else {
// Shallue and van de Woestijne
// SQRTM3 not available, so preprocess this out
/* */
                let Z=fp::RIADZ;
                X1.copy(&FP::new_int(Z));
                X3.copy(&X1);
                A.copy(&ECP::rhs(&X1));
                B.copy(&FP::new_big(&BIG::new_ints(&rom::SQRTM3)));
                B.imul(Z);

                t.sqr();
                Y.copy(&A); Y.mul(&t);
                t.copy(&one); t.add(&Y); t.norm();
                Y.rsub(&one); Y.norm();
                D.copy(&t); D.mul(&Y); 
                D.mul(&B);
                
                w.copy(&A);
                FP::tpo(&mut D,&mut w);

                w.mul(&B);
                if w.sign()==1 {
                    w.neg();
                    w.norm();
                }
                w.mul(&B);
                w.mul(&h); w.mul(&Y); w.mul(&D);

                X1.neg(); X1.norm(); X1.div2();
                X2.copy(&X1);
                X1.sub(&w); X1.norm();
                X2.add(&w); X2.norm();
                A.dbl(); A.dbl(); A.norm();
                t.sqr(); t.mul(&D); t.sqr();
                A.mul(&t);
                X3.add(&A); X3.norm();

                let mut rhs=ECP::rhs(&X2);
                X3.cmove(&X2,rhs.qr(None));
                rhs.copy(&ECP::rhs(&X1));
                X3.cmove(&X1,rhs.qr(None));
                rhs.copy(&ECP::rhs(&X3));
                Y.copy(&rhs.sqrt(None));

                let ne=Y.sign()^sgn;
                w.copy(&Y); w.neg(); w.norm();
                Y.cmove(&w,ne);

                let x=X3.redc();
                let y=Y.redc();
                P.copy(&ECP::new_bigs(&x,&y));
                return P;
/* */
            }
        }
        P
    }

/* Map byte string to curve point */
    #[allow(non_snake_case)]
    pub fn mapit(h: &[u8]) -> ECP {
        let q = BIG::new_ints(&rom::MODULUS);
        let mut dx = DBIG::frombytes(h);
        let x=dx.dmod(&q);
        let mut P=ECP::hap2point(&x);
        P.cfp();
        P
    }

    pub fn generator() -> ECP {
        let G: ECP;
        let gx = BIG::new_ints(&rom::CURVE_GX);
        if CURVETYPE != MONTGOMERY {
            let gy = BIG::new_ints(&rom::CURVE_GY);
            G = ECP::new_bigs(&gx, &gy);
        } else {
            G = ECP::new_big(&gx);
        }
        G
    }
}
