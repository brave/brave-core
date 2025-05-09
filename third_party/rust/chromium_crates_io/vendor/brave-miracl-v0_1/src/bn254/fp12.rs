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
use crate::bn254::fp::FP;
use crate::bn254::fp2::FP2;
use crate::bn254::fp4::FP4;
use crate::bn254::rom;

pub const ZERO: usize = 0;
pub const ONE: usize = 1;
pub const SPARSEST: usize = 2;
pub const SPARSER: usize = 3;
pub const SPARSE: usize = 4;
pub const DENSE: usize = 5;

#[derive(Copy, Clone)]
pub struct FP12 {
    a: FP4,
    b: FP4,
    c: FP4,
    stype: usize,
}

#[cfg(feature = "std")]
impl std::fmt::Debug for FP12 {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}    

#[cfg(feature = "std")]
impl std::fmt::Display for FP12 {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}

impl FP12 {
    pub fn new() -> FP12 {
        FP12 {
            a: FP4::new(),
            b: FP4::new(),
            c: FP4::new(),
            stype: ZERO,
        }
    }

    pub fn settype(&mut self, t: usize) {
        self.stype = t;
    }

    pub fn gettype(&self) -> usize {
        self.stype
    }

    pub fn new_int(a: isize) -> FP12 {
        let mut f = FP12::new();
        f.a.copy(&FP4::new_int(a));
        f.b.zero();
        f.c.zero();
        if a == 1 {
            f.stype = ONE;
        } else {
            f.stype = SPARSEST;
        }
        f
    }

    pub fn new_copy(x: &FP12) -> FP12 {
        let mut f = FP12::new();
        f.a.copy(&x.a);
        f.b.copy(&x.b);
        f.c.copy(&x.c);
        f.stype = x.stype;
        f
    }

    pub fn new_fp4s(d: &FP4, e: &FP4, f: &FP4) -> FP12 {
        let mut g = FP12::new();
        g.a.copy(d);
        g.b.copy(e);
        g.c.copy(f);
        g.stype = DENSE;
        g
    }

    pub fn new_fp4(d: &FP4) -> FP12 {
        let mut g = FP12::new();
        g.a.copy(d);
        g.b.zero();
        g.c.zero();
        g.stype = SPARSEST;
        g
    }

    /* reduce components mod Modulus */
    pub fn reduce(&mut self) {
        self.a.reduce();
        self.b.reduce();
        self.c.reduce();
    }

    /* normalise components of w */
    pub fn norm(&mut self) {
        self.a.norm();
        self.b.norm();
        self.c.norm();
    }

    /* test self=0 ? */
    pub fn iszilch(&self) -> bool {
        //self.reduce();
        self.a.iszilch() && self.b.iszilch() && self.c.iszilch()
    }

    /* Conditional move of g to self dependant on d */
    pub fn cmove(&mut self, g: &FP12, d: isize) {
        self.a.cmove(&g.a, d);
        self.b.cmove(&g.b, d);
        self.c.cmove(&g.c, d);
        let mut u = d as usize;
        u = !(u.wrapping_sub(1));
        self.stype ^= (self.stype ^ g.stype) & u;
    }

    /* return 1 if b==c, no branching */
    fn teq(b: i32, c: i32) -> isize {
        let mut x = b ^ c;
        x -= 1; // if x=0, x now -1
        ((x >> 31) & 1) as isize
    }

    /* Constant time select from pre-computed table */
    pub fn selector(&mut self, g: &[FP12], b: i32) {
        let m = b >> 31;
        let mut babs = (b ^ m) - m;

        babs = (babs - 1) / 2;

        self.cmove(&g[0], FP12::teq(babs, 0)); // conditional move
        self.cmove(&g[1], FP12::teq(babs, 1));
        self.cmove(&g[2], FP12::teq(babs, 2));
        self.cmove(&g[3], FP12::teq(babs, 3));
        self.cmove(&g[4], FP12::teq(babs, 4));
        self.cmove(&g[5], FP12::teq(babs, 5));
        self.cmove(&g[6], FP12::teq(babs, 6));
        self.cmove(&g[7], FP12::teq(babs, 7));

        let mut invf = FP12::new_copy(self);
        invf.conj();
        self.cmove(&invf, (m & 1) as isize);
    }

    /* test self=1 ? */
    pub fn isunity(&self) -> bool {
        let one = FP4::new_int(1);
        self.a.equals(&one) && self.b.iszilch() && self.c.iszilch()
    }

    /* test self=x */
    pub fn equals(&self, x: &FP12) -> bool {
        self.a.equals(&x.a) && self.b.equals(&x.b) && self.c.equals(&x.c)
    }

    pub fn geta(&mut self) -> FP4 {
        self.a
        //        let f = FP4::new_copy(&self.a);
        //        return f;
    }

    pub fn getb(&mut self) -> FP4 {
        self.b
        //        let f = FP4::new_copy(&self.b);
        //        return f;
    }

    pub fn getc(&mut self) -> FP4 {
        self.c
        //        let f = FP4::new_copy(&self.c);
        //        return f;
    }

    /* copy self=x */
    pub fn copy(&mut self, x: &FP12) {
        self.a.copy(&x.a);
        self.b.copy(&x.b);
        self.c.copy(&x.c);
        self.stype = x.stype;
    }

    /* set self=1 */
    pub fn one(&mut self) {
        self.a.one();
        self.b.zero();
        self.c.zero();
        self.stype = ONE;
    }

    /* set self=0 */
    pub fn zero(&mut self) {
        self.a.zero();
        self.b.zero();
        self.c.zero();
        self.stype = ZERO;
    }

    /* this=conj(this) */
    pub fn conj(&mut self) {
        self.a.conj();
        self.b.nconj();
        self.c.conj();
    }

    /* Granger-Scott Unitary Squaring */
    pub fn usqr(&mut self) {
        let mut a = FP4::new_copy(&self.a);
        let mut b = FP4::new_copy(&self.c);
        let mut c = FP4::new_copy(&self.b);
        let mut d = FP4::new();

        self.a.sqr();
        d.copy(&self.a);
        d.add(&self.a);
        self.a.add(&d);

        self.a.norm();
        a.nconj();

        a.dbl();
        self.a.add(&a);
        b.sqr();
        b.times_i();

        d.copy(&b);
        d.add(&b);
        b.add(&d);
        b.norm();

        c.sqr();
        d.copy(&c);
        d.add(&c);
        c.add(&d);
        c.norm();

        self.b.conj();
        self.b.dbl();
        self.c.nconj();

        self.c.dbl();
        self.b.add(&b);
        self.c.add(&c);
        self.stype = DENSE;
        self.reduce();
    }

    /* Chung-Hasan SQR2 method from http://cacr.uwaterloo.ca/techreports/2006/cacr2006-24.pdf */
    pub fn sqr(&mut self) {
        if self.stype == ONE {
            return;
        }

        let mut a = FP4::new_copy(&self.a);
        let mut b = FP4::new_copy(&self.b);
        let mut c = FP4::new_copy(&self.c);
        let mut d = FP4::new_copy(&self.a);

        a.sqr();
        b.mul(&self.c);
        b.dbl();
        b.norm();
        c.sqr();
        d.mul(&self.b);
        d.dbl();

        self.c.add(&self.a);
        self.c.add(&self.b);
        self.c.norm();
        self.c.sqr();

        self.a.copy(&a);
        a.add(&b);
        a.norm();
        a.add(&c);
        a.add(&d);
        a.norm();

        a.neg();
        b.times_i();
        c.times_i();

        self.a.add(&b);

        self.b.copy(&c);
        self.b.add(&d);
        self.c.add(&a);
        if self.stype == SPARSER || self.stype == SPARSEST {
            self.stype = SPARSE;
        } else {
            self.stype = DENSE;
        }
        self.norm();
    }

    /* FP12 full multiplication self=self*y */
    pub fn mul(&mut self, y: &FP12) {
        let mut z0 = FP4::new_copy(&self.a);
        let mut z1 = FP4::new();
        let mut z2 = FP4::new_copy(&self.b);
        let mut z3 = FP4::new();
        let mut t0 = FP4::new_copy(&self.a);
        let mut t1 = FP4::new_copy(&y.a);

        z0.mul(&y.a);
        z2.mul(&y.b);

        t0.add(&self.b);
        t1.add(&y.b);

        t0.norm();
        t1.norm();

        z1.copy(&t0);
        z1.mul(&t1);
        t0.copy(&self.b);
        t0.add(&self.c);
        t1.copy(&y.b);
        t1.add(&y.c);

        t0.norm();
        t1.norm();

        z3.copy(&t0);
        z3.mul(&t1);

        t0.copy(&z0);
        t0.neg();
        t1.copy(&z2);
        t1.neg();

        z1.add(&t0);
        self.b.copy(&z1);
        self.b.add(&t1);

        z3.add(&t1);
        z2.add(&t0);

        t0.copy(&self.a);
        t0.add(&self.c);
        t0.norm();
        t1.copy(&y.a);
        t1.add(&y.c);
        t1.norm();
        t0.mul(&t1);
        z2.add(&t0);

        t0.copy(&self.c);
        t0.mul(&y.c);
        t1.copy(&t0);
        t1.neg();

        self.c.copy(&z2);
        self.c.add(&t1);
        z3.add(&t1);
        t0.times_i();
        self.b.add(&t0);
        z3.norm();

        z3.times_i();
        self.a.copy(&z0);
        self.a.add(&z3);
        self.stype = DENSE;
        self.norm();
    }

    /* FP12 full multiplication w=w*y */
    /* Supports sparse multiplicands */
    /* Usually w is denser than y */
    pub fn ssmul(&mut self, y: &FP12) {
        if self.stype == ONE {
            self.copy(&y);
            return;
        }
        if y.stype == ONE {
            return;
        }
        if y.stype >= SPARSE {
            let mut z0 = FP4::new_copy(&self.a);
            let mut z1 = FP4::new();
            let mut z2 = FP4::new();
            let mut z3 = FP4::new();
            z0.mul(&y.a);

            if ecp::SEXTIC_TWIST == ecp::M_TYPE {
                if y.stype == SPARSE || self.stype == SPARSE {
                    let mut ga = FP2::new();
                    let mut gb = FP2::new();

                    gb.copy(&self.b.getb());
                    gb.mul(&y.b.getb());
                    ga.zero();
                    if y.stype != SPARSE {
                        ga.copy(&self.b.getb());
                        ga.mul(&y.b.geta());
                    }
                    if self.stype != SPARSE {
                        ga.copy(&self.b.geta());
                        ga.mul(&y.b.getb());
                    }
                    z2.set_fp2s(&ga, &gb);
                    z2.times_i();
                } else {
                    z2.copy(&self.b);
                    z2.mul(&y.b);
                }
            } else {
                z2.copy(&self.b);
                z2.mul(&y.b);
            }
            let mut t0 = FP4::new_copy(&self.a);
            let mut t1 = FP4::new_copy(&y.a);
            t0.add(&self.b);
            t0.norm();
            t1.add(&y.b);
            t1.norm();

            z1.copy(&t0);
            z1.mul(&t1);
            t0.copy(&self.b);
            t0.add(&self.c);
            t0.norm();
            t1.copy(&y.b);
            t1.add(&y.c);
            t1.norm();

            z3.copy(&t0);
            z3.mul(&t1);

            t0.copy(&z0);
            t0.neg();
            t1.copy(&z2);
            t1.neg();

            z1.add(&t0);
            self.b.copy(&z1);
            self.b.add(&t1);

            z3.add(&t1);
            z2.add(&t0);

            t0.copy(&self.a);
            t0.add(&self.c);
            t0.norm();
            t1.copy(&y.a);
            t1.add(&y.c);
            t1.norm();

            t0.mul(&t1);
            z2.add(&t0);

            if ecp::SEXTIC_TWIST == ecp::D_TYPE {
                if y.stype == SPARSE || self.stype == SPARSE {
                    let mut ga = FP2::new();
                    let mut gb = FP2::new();

                    ga.copy(&self.c.geta());
                    ga.mul(&y.c.geta());
                    gb.zero();
                    if y.stype != SPARSE {
                        gb.copy(&self.c.geta());
                        gb.mul(&y.c.getb());
                    }
                    if self.stype != SPARSE {
                        gb.copy(&self.c.getb());
                        gb.mul(&y.c.geta());
                    }
                    t0.set_fp2s(&ga, &gb);
                } else {
                    t0.copy(&self.c);
                    t0.mul(&y.c);
                }
            } else {
                t0.copy(&self.c);
                t0.mul(&y.c);
            }
            t1.copy(&t0);
            t1.neg();

            self.c.copy(&z2);
            self.c.add(&t1);
            z3.add(&t1);
            t0.times_i();
            self.b.add(&t0);
            z3.norm();
            z3.times_i();
            self.a.copy(&z0);
            self.a.add(&z3);
        } else {
            if self.stype == SPARSER || self.stype == SPARSEST {
                self.smul(&y);
                return;
            }
            if ecp::SEXTIC_TWIST == ecp::D_TYPE {
                // dense by sparser - 13m
                let mut z0 = FP4::new_copy(&self.a);
                let mut z2 = FP4::new_copy(&self.b);
                let mut z3 = FP4::new_copy(&self.b);
                let mut t0 = FP4::new();
                let mut t1 = FP4::new_copy(&y.a);

                z0.mul(&y.a);
                if y.stype == SPARSEST {
                    z2.qmul(&y.b.geta().getA());
                } else {
                    z2.pmul(&y.b.geta());
                }
                self.b.add(&self.a);
                t1.padd(&y.b.geta());

                t1.norm();
                self.b.norm();
                self.b.mul(&t1);
                z3.add(&self.c);
                z3.norm();

                if y.stype == SPARSEST {
                    z3.qmul(&y.b.geta().getA());
                } else {
                    z3.pmul(&y.b.geta());
                }
                t0.copy(&z0);
                t0.neg();
                t1.copy(&z2);
                t1.neg();

                self.b.add(&t0);

                self.b.add(&t1);
                z3.add(&t1);
                z2.add(&t0);

                t0.copy(&self.a);
                t0.add(&self.c);
                t0.norm();
                z3.norm();
                t0.mul(&y.a);
                self.c.copy(&z2);
                self.c.add(&t0);

                z3.times_i();
                self.a.copy(&z0);
                self.a.add(&z3);
            }
            if ecp::SEXTIC_TWIST == ecp::M_TYPE {
                let mut z0 = FP4::new_copy(&self.a);
                let mut z1 = FP4::new();
                let mut z2 = FP4::new();
                let mut z3 = FP4::new();
                let mut t0 = FP4::new_copy(&self.a);
                let mut t1 = FP4::new();

                z0.mul(&y.a);
                t0.add(&self.b);
                t0.norm();

                z1.copy(&t0);
                z1.mul(&y.a);
                t0.copy(&self.b);
                t0.add(&self.c);
                t0.norm();

                z3.copy(&t0);
                if y.stype == SPARSEST {
                    z3.qmul(&y.c.getb().getA());
                } else {
                    z3.pmul(&y.c.getb());
                }
                z3.times_i();

                t0.copy(&z0);
                t0.neg();
                z1.add(&t0);
                self.b.copy(&z1);
                z2.copy(&t0);

                t0.copy(&self.a);
                t0.add(&self.c);
                t0.norm();
                t1.copy(&y.a);
                t1.add(&y.c);
                t1.norm();

                t0.mul(&t1);
                z2.add(&t0);
                t0.copy(&self.c);

                if y.stype == SPARSEST {
                    t0.qmul(&y.c.getb().getA());
                } else {
                    t0.pmul(&y.c.getb());
                }
                t0.times_i();
                t1.copy(&t0);
                t1.neg();

                self.c.copy(&z2);
                self.c.add(&t1);
                z3.add(&t1);
                t0.times_i();
                self.b.add(&t0);
                z3.norm();
                z3.times_i();
                self.a.copy(&z0);
                self.a.add(&z3);
            }
        }
        self.stype = DENSE;
        self.norm();
    }

    /* Special case of multiplication arises from special form of ATE pairing line function */
    pub fn smul(&mut self, y: &FP12) {
        if ecp::SEXTIC_TWIST == ecp::D_TYPE {
            let mut w1 = FP2::new_copy(&self.a.geta());
            let mut w2 = FP2::new_copy(&self.a.getb());
            //let mut w3=FP2::new_copy(&self.b.geta());

            let mut w3: FP2;

            w1.mul(&y.a.geta());
            w2.mul(&y.a.getb());

            if y.stype == SPARSEST || self.stype == SPARSEST {
                if y.stype == SPARSEST && self.stype == SPARSEST {
                    let mut t = FP::new_copy(&self.b.geta().getA());
                    t.mul(&y.b.geta().getA());
                    w3 = FP2::new_fp(&t);
                } else if y.stype != SPARSEST {
                    w3 = FP2::new_copy(&y.b.geta());
                    w3.pmul(&self.b.geta().getA());
                } else {
                    w3 = FP2::new_copy(&self.b.geta());
                    w3.pmul(&y.b.geta().getA());
                }
            } else {
                w3 = FP2::new_copy(&self.b.geta());
                w3.mul(&y.b.geta());
            }
            let mut ta = FP2::new_copy(&self.a.geta());
            let mut tb = FP2::new_copy(&y.a.geta());
            ta.add(&self.a.getb());
            ta.norm();
            tb.add(&y.a.getb());
            tb.norm();
            let mut tc = FP2::new_copy(&ta);
            tc.mul(&tb);
            let mut t = FP2::new_copy(&w1);
            t.add(&w2);
            t.neg();
            tc.add(&t);

            ta.copy(&self.a.geta());
            ta.add(&self.b.geta());
            ta.norm();
            tb.copy(&y.a.geta());
            tb.add(&y.b.geta());
            tb.norm();
            let mut td = FP2::new_copy(&ta);
            td.mul(&tb);
            t.copy(&w1);
            t.add(&w3);
            t.neg();
            td.add(&t);

            ta.copy(&self.a.getb());
            ta.add(&self.b.geta());
            ta.norm();
            tb.copy(&y.a.getb());
            tb.add(&y.b.geta());
            tb.norm();
            let mut te = FP2::new_copy(&ta);
            te.mul(&tb);
            t.copy(&w2);
            t.add(&w3);
            t.neg();
            te.add(&t);

            w2.mul_ip();
            w1.add(&w2);

            self.a.set_fp2s(&w1, &tc);
            self.b.set_fp2s(&td, &te);
            self.c.set_fp2(&w3);

            self.a.norm();
            self.b.norm();
        } else {
            let mut w1 = FP2::new_copy(&self.a.geta());
            let mut w2 = FP2::new_copy(&self.a.getb());
            //           let mut w3=FP2::new_copy(&self.c.getb());
            let mut w3: FP2;

            w1.mul(&y.a.geta());
            w2.mul(&y.a.getb());

            if y.stype == SPARSEST || self.stype == SPARSEST {
                if y.stype == SPARSEST && self.stype == SPARSEST {
                    let mut t = FP::new_copy(&self.c.getb().getA());
                    t.mul(&y.c.getb().getA());
                    w3 = FP2::new_fp(&t);
                } else if y.stype != SPARSEST {
                    w3 = FP2::new_copy(&y.c.getb());
                    w3.pmul(&self.c.getb().getA());
                } else {
                    w3 = FP2::new_copy(&self.c.getb());
                    w3.pmul(&y.c.getb().getA());
                }
            } else {
                w3 = FP2::new_copy(&self.c.getb());
                w3.mul(&y.c.getb());
            }
            let mut ta = FP2::new_copy(&self.a.geta());
            let mut tb = FP2::new_copy(&y.a.geta());
            ta.add(&self.a.getb());
            ta.norm();
            tb.add(&y.a.getb());
            tb.norm();
            let mut tc = FP2::new_copy(&ta);
            tc.mul(&tb);
            let mut t = FP2::new_copy(&w1);
            t.add(&w2);
            t.neg();
            tc.add(&t);

            ta.copy(&self.a.geta());
            ta.add(&self.c.getb());
            ta.norm();
            tb.copy(&y.a.geta());
            tb.add(&y.c.getb());
            tb.norm();
            let mut td = FP2::new_copy(&ta);
            td.mul(&tb);
            t.copy(&w1);
            t.add(&w3);
            t.neg();
            td.add(&t);

            ta.copy(&self.a.getb());
            ta.add(&self.c.getb());
            ta.norm();
            tb.copy(&y.a.getb());
            tb.add(&y.c.getb());
            tb.norm();
            let mut te = FP2::new_copy(&ta);
            te.mul(&tb);
            t.copy(&w2);
            t.add(&w3);
            t.neg();
            te.add(&t);

            w2.mul_ip();
            w1.add(&w2);
            self.a.set_fp2s(&w1, &tc);

            w3.mul_ip();
            w3.norm();
            self.b.set_fp2h(&w3);

            te.norm();
            te.mul_ip();
            self.c.set_fp2s(&te, &td);

            self.a.norm();
            self.c.norm();
        }
        self.stype = SPARSE;
    }

    /* self=1/self */
    pub fn inverse(&mut self) {
        let mut f0 = FP4::new_copy(&self.a);
        let mut f1 = FP4::new_copy(&self.b);
        let mut f2 = FP4::new_copy(&self.a);
        let mut f3 = FP4::new();

        self.norm();
        f0.sqr();
        f1.mul(&self.c);
        f1.times_i();
        f0.sub(&f1);
        f0.norm();

        f1.copy(&self.c);
        f1.sqr();
        f1.times_i();
        f2.mul(&self.b);
        f1.sub(&f2);
        f1.norm();

        f2.copy(&self.b);
        f2.sqr();
        f3.copy(&self.a);
        f3.mul(&self.c);
        f2.sub(&f3);
        f2.norm();

        f3.copy(&self.b);
        f3.mul(&f2);
        f3.times_i();
        self.a.mul(&f0);
        f3.add(&self.a);
        self.c.mul(&f1);
        self.c.times_i();

        f3.add(&self.c);
        f3.norm();
        f3.inverse(None);
        self.a.copy(&f0);
        self.a.mul(&f3);
        self.b.copy(&f1);
        self.b.mul(&f3);
        self.c.copy(&f2);
        self.c.mul(&f3);
        self.stype = DENSE;
    }

    /* self=self^p using Frobenius */
    pub fn frob(&mut self, f: &FP2) {
        let mut f2 = FP2::new_copy(f);
        let mut f3 = FP2::new_copy(f);

        f2.sqr();
        f3.mul(&f2);

        self.a.frob(&f3);
        self.b.frob(&f3);
        self.c.frob(&f3);

        self.b.pmul(f);
        self.c.pmul(&f2);
        self.stype = DENSE;
    }

    /* trace function */
    pub fn trace(&mut self) -> FP4 {
        let mut t = FP4::new();
        t.copy(&self.a);
        t.imul(3);
        t.reduce();
        t
    }

    /* convert from byte array to FP12 */
    pub fn frombytes(w: &[u8]) -> FP12 {
        const MB:usize = 4*(big::MODBYTES as usize);
        let mut t: [u8; MB] = [0; MB];
	    for i in 0..MB {
		    t[i]=w[i];
	    }
        let c=FP4::frombytes(&t);
	    for i in 0..MB {
		    t[i]=w[i+MB];
	    }
        let b=FP4::frombytes(&t);
	    for i in 0..MB {
		    t[i]=w[i+2*MB];
	    }
        let a=FP4::frombytes(&t);
	    FP12::new_fp4s(&a,&b,&c)
    }

    /* convert this to byte array */
    pub fn tobytes(&mut self, w: &mut [u8]) {
        const MB:usize = 4*(big::MODBYTES as usize);
        let mut t: [u8; MB] = [0; MB];

        self.c.tobytes(&mut t);
	    for i in 0..MB { 
		    w[i]=t[i];
	    }
        self.b.tobytes(&mut t);
	    for i in 0..MB {
		    w[i+MB]=t[i];
	    }
        self.a.tobytes(&mut t);
	    for i in 0..MB {
		    w[i+2*MB]=t[i];
	    }
    }

    /* output to hex string */
    #[cfg(feature = "std")]
    pub fn tostring(&self) -> String {
        format!(
            "[{},{},{}]",
            self.a.tostring(),
            self.b.tostring(),
            self.c.tostring()
        )
    }

/* Note this is simple square and multiply, so not side-channel safe */
/* But fast for final exponentiation where exponent is not a secret */
/* return this^e */
    pub fn pow(&self, e: &BIG) -> FP12 {
        let mut r = FP12::new_copy(self);
        r.norm();
        let mut e1 = BIG::new_copy(e);
        e1.norm();
        let mut e3 = BIG::new_copy(&e1);
        e3.pmul(3);
        e3.norm();
        let mut w = FP12::new_copy(&r);
        if e3.iszilch() {
            w.one();
            return w;
        }

        let nb = e3.nbits();
        for i in (1..nb - 1).rev() {
            w.usqr();
            let bt = e3.bit(i) - e1.bit(i);
            if bt == 1 {
                w.mul(&r);
            }
            if bt == -1 {
                r.conj();
                w.mul(&r);
                r.conj();
            }
        }

        w.reduce();
        w
    }

    /* constant time powering by small integer of max length bts */
    pub fn pinpow(&mut self, e: i32, bts: i32) {
        let mut r: [FP12; 2] = [FP12::new_int(1), FP12::new_copy(self)];
        let mut t = FP12::new();

        for i in (0..bts).rev() {
            let b: usize = ((e >> i) & 1) as usize;
            t.copy(&r[b]);
            r[1 - b].mul(&t);
            r[b].usqr();
        }
        self.copy(&r[0]);
    }

    pub fn compow(&mut self, e: &BIG, r: &BIG) -> FP4 {
        let f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));
        let q = BIG::new_ints(&rom::MODULUS);

        let mut g1 = FP12::new_copy(self);
        let mut g2 = FP12::new_copy(self);

        let mut m = BIG::new_copy(&q);
        m.rmod(&r);

        let mut a = BIG::new_copy(&e);
        a.rmod(&m);

        let mut b = BIG::new_copy(&e);
        b.div(&m);

        let mut c = g1.trace();

        if b.iszilch() {
            c = c.xtr_pow(&a);
            return c;
        }

        g2.frob(&f);
        let cp = g2.trace();
        g1.conj();
        g2.mul(&g1);
        let cpm1 = g2.trace();
        g2.mul(&g1);
        let cpm2 = g2.trace();

        c.xtr_pow2(&cp, &cpm1, &cpm2, &a, &b)
    }

    /* p=q0^u0.q1^u1.q2^u2.q3^u3 */
    // Bos & Costello https://eprint.iacr.org/2013/458.pdf
    // Faz-Hernandez & Longa & Sanchez  https://eprint.iacr.org/2013/158.pdf
    // Side channel attack secure
    pub fn pow4(q: &[FP12], u: &[BIG]) -> FP12 {
        let mut g: [FP12; 8] = [
            FP12::new(),
            FP12::new(),
            FP12::new(),
            FP12::new(),
            FP12::new(),
            FP12::new(),
            FP12::new(),
            FP12::new(),
        ];

        let mut r = FP12::new();
        let mut p = FP12::new();
        const CT: usize = 1 + big::NLEN * (big::BASEBITS as usize);
        let mut w: [i8; CT] = [0; CT];
        let mut s: [i8; CT] = [0; CT];

        let mut mt = BIG::new();
        let mut t: [BIG; 4] = [
            BIG::new_copy(&u[0]),
            BIG::new_copy(&u[1]),
            BIG::new_copy(&u[2]),
            BIG::new_copy(&u[3]),
        ];

        for i in 0..4 {
            t[i].norm();
        }

        // precomputation
        g[0].copy(&q[0]);
        r.copy(&g[0]);
        g[1].copy(&r);
        g[1].mul(&q[1]); // q[0].q[1]
        g[2].copy(&r);
        g[2].mul(&q[2]);
        r.copy(&g[1]); // q[0].q[2]
        g[3].copy(&r);
        g[3].mul(&q[2]);
        r.copy(&g[0]); // q[0].q[1].q[2]
        g[4].copy(&r);
        g[4].mul(&q[3]);
        r.copy(&g[1]); // q[0].q[3]
        g[5].copy(&r);
        g[5].mul(&q[3]);
        r.copy(&g[2]); // q[0].q[1].q[3]
        g[6].copy(&r);
        g[6].mul(&q[3]);
        r.copy(&g[3]); // q[0].q[2].q[3]
        g[7].copy(&r);
        g[7].mul(&q[3]); // q[0].q[1].q[2].q[3]

        // Make it odd
        let pb = 1 - t[0].parity();
        t[0].inc(pb);
        t[0].norm();

        // Number of bits
        mt.zero();
        for i in 0..4 {
            mt.or(&t[i]);
        }

        let nb = 1 + mt.nbits();

        // Sign pivot
        s[nb - 1] = 1;
        for i in 0..nb - 1 {
            t[0].fshr(1);
            s[i] = (2 * t[0].parity() - 1) as i8;
            //println!("s={}",s[i]);
        }

        // Recoded exponent
        for i in 0..nb {
            w[i] = 0;
            let mut k = 1;
            for j in 1..4 {
                let bt = s[i] * (t[j].parity() as i8);
                t[j].fshr(1);
                t[j].dec((bt >> 1) as isize);
                t[j].norm();
                w[i] += bt * (k as i8);
                k *= 2;
            }
        }

        // Main loop
        p.selector(&g, (2 * w[nb - 1] + 1) as i32);
        for i in (0..nb - 1).rev() {
            p.usqr();
            r.selector(&g, (2 * w[i] + s[i]) as i32);
            p.mul(&r);
        }

        // apply correction
        r.copy(&q[0]);
        r.conj();
        r.mul(&p);
        p.cmove(&r, pb);
        p.reduce();
        p
    }
}
