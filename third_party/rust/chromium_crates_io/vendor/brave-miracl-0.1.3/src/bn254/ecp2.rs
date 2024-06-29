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
use crate::bn254::fp2::FP2;
use crate::bn254::rom;
use crate::bn254::fp;
use crate::bn254::fp::FP;
use crate::bn254::dbig::DBIG;

#[derive(Clone)]
pub struct ECP2 {
    x: FP2,
    y: FP2,
    z: FP2,
}

#[cfg(feature = "std")]
impl std::fmt::Debug for ECP2 {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}    

#[cfg(feature = "std")]
impl std::fmt::Display for ECP2 {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}

#[allow(non_snake_case)]
impl ECP2 {
    pub fn new() -> ECP2 {
        ECP2 {
            x: FP2::new(),
            y: FP2::new_int(1),
            z: FP2::new(),
        }
    }
    #[allow(non_snake_case)]
    /* construct this from (x,y) - but set to O if not on curve */
    pub fn new_fp2s(ix: &FP2, iy: &FP2) -> ECP2 {
        let mut E = ECP2::new();
        E.x.copy(&ix);
        E.y.copy(&iy);
        E.z.one();
        E.x.norm();

        let rhs = ECP2::rhs(&E.x);
        let mut y2 = FP2::new_copy(&E.y);
        y2.sqr();
        if !y2.equals(&rhs) {
            E.inf();
        }
        E
    }

    /* construct this from x - but set to O if not on curve */
    pub fn new_fp2(ix: &FP2, s:isize) -> ECP2 {
        let mut E = ECP2::new();
        let mut h = FP::new();
        E.x.copy(&ix);
        E.y.one();
        E.z.one();
        E.x.norm();
        let mut rhs = ECP2::rhs(&E.x);
 	    if rhs.qr(Some(&mut h)) == 1 {
		    rhs.sqrt(Some(&h));
		    if rhs.sign() != s {
			    rhs.neg();
		    }
		    rhs.reduce();
		    E.y.copy(&rhs);
        } else {
            E.inf();
        }
        E
    }

    /* Test this=O? */
    pub fn is_infinity(&self) -> bool {
        self.x.iszilch() && self.z.iszilch()
    }

    /* copy self=P */
    pub fn copy(&mut self, P: &ECP2) {
        self.x.copy(&P.x);
        self.y.copy(&P.y);
        self.z.copy(&P.z);
    }

    /* set self=O */
    pub fn inf(&mut self) {
        self.x.zero();
        self.y.one();
        self.z.zero();
    }

    /* set self=-self */
    pub fn neg(&mut self) {
        self.y.norm();
        self.y.neg();
        self.y.norm();
    }

    /* Conditional move of Q to self dependant on d */
    pub fn cmove(&mut self, Q: &ECP2, d: isize) {
        self.x.cmove(&Q.x, d);
        self.y.cmove(&Q.y, d);
        self.z.cmove(&Q.z, d);
    }

    /* return 1 if b==c, no branching */
    fn teq(b: i32, c: i32) -> isize {
        let mut x = b ^ c;
        x -= 1; // if x=0, x now -1
        ((x >> 31) & 1) as isize
    }

    /* Constant time select from pre-computed table */
    pub fn selector(&mut self, W: &[ECP2], b: i32) {
        let mut MP = ECP2::new();
        let m = b >> 31;
        let mut babs = (b ^ m) - m;

        babs = (babs - 1) / 2;

        self.cmove(&W[0], ECP2::teq(babs, 0)); // conditional move
        self.cmove(&W[1], ECP2::teq(babs, 1));
        self.cmove(&W[2], ECP2::teq(babs, 2));
        self.cmove(&W[3], ECP2::teq(babs, 3));
        self.cmove(&W[4], ECP2::teq(babs, 4));
        self.cmove(&W[5], ECP2::teq(babs, 5));
        self.cmove(&W[6], ECP2::teq(babs, 6));
        self.cmove(&W[7], ECP2::teq(babs, 7));

        MP.copy(self);
        MP.neg();
        self.cmove(&MP, (m & 1) as isize);
    }

    /* Test if P == Q */
    pub fn equals(&self, Q: &ECP2) -> bool {
        let mut a = FP2::new_copy(&self.x);
        let mut b = FP2::new_copy(&Q.x);

        a.mul(&Q.z);
        b.mul(&self.z);
        if !a.equals(&b) {
            return false;
        }
        a.copy(&self.y);
        a.mul(&Q.z);
        b.copy(&Q.y);
        b.mul(&self.z);
        if !a.equals(&b) {
            return false;
        }

        true
    }

    /* set to Affine - (x,y,z) to (x,y) */
    pub fn affine(&mut self) {
        if self.is_infinity() {
            return;
        }
        let one = FP2::new_int(1);
        if self.z.equals(&one) {
            return;
        }
        self.z.inverse(None);

        self.x.mul(&self.z);
        self.x.reduce();
        self.y.mul(&self.z);
        self.y.reduce();
        self.z.copy(&one);
    }

    /* extract affine x as FP2 */
    pub fn getx(&self) -> FP2 {
        let mut W = ECP2::new();
        W.copy(self);
        W.affine();
        FP2::new_copy(&W.x)
    }

    /* extract affine y as FP2 */
    pub fn gety(&self) -> FP2 {
        let mut W = ECP2::new();
        W.copy(self);
        W.affine();
        FP2::new_copy(&W.y)
    }

    /* extract projective x */
    pub fn getpx(&self) -> FP2 {
        FP2::new_copy(&self.x)
    }
    /* extract projective y */
    pub fn getpy(&self) -> FP2 {
        FP2::new_copy(&self.y)
    }
    /* extract projective z */
    pub fn getpz(&self) -> FP2 {
        FP2::new_copy(&self.z)
    }

    /* convert to byte array */
    pub fn tobytes(&self, b: &mut [u8], compress: bool) {
        const MB:usize = 2*(big::MODBYTES as usize);
        let mut t: [u8; MB] = [0; MB];
        let mut alt=false;
        let mut W = ECP2::new();
        W.copy(self);
        W.affine();
        W.x.tobytes(&mut t);

        if (fp::MODBITS-1)%8 <= 4 && ecp::ALLOW_ALT_COMPRESS {
            alt=true;
        }
        if alt {
		    for i in 0..MB {
			    b[i]=t[i]
		    }
            if !compress {
                W.y.tobytes(&mut t);
                for i in 0..MB {
				    b[i+MB]=t[i];
			    }
            } else {
                b[0]|=0x80;
                if W.y.islarger()==1 {
				    b[0]|=0x20;
			    }
            }

	    } else {
		    for i in 0..MB {
			    b[i+1]=t[i];
		    }
            if !compress {
                b[0]=0x04;
                W.y.tobytes(&mut t);
	            for i in 0..MB {
			        b[i+MB+1]=t[i];
			    }
            } else {
                b[0]=0x02;
                if W.y.sign() == 1 {
                    b[0]=0x03;
			    }
            }
	    }
    }

    /* convert from byte array to point */
    pub fn frombytes(b: &[u8]) -> ECP2 {
        const MB:usize = 2*(big::MODBYTES as usize);
        let mut t: [u8; MB] = [0; MB];
        let typ=b[0] as isize;
        let mut alt=false;

        if (fp::MODBITS-1)%8 <= 4 && ecp::ALLOW_ALT_COMPRESS {
            alt=true;
        }

	if alt {
            for i in 0..MB  {
			    t[i]=b[i];
		    }
            t[0]&=0x1f;
            let rx=FP2::frombytes(&t);
            if (b[0]&0x80)==0 {
                for i in 0..MB {
				    t[i]=b[i+MB];
			    }
                let ry=FP2::frombytes(&t);
                ECP2::new_fp2s(&rx,&ry)
            } else {
                let sgn=(b[0]&0x20)>>5;
                let mut P=ECP2::new_fp2(&rx,0);
                let cmp=P.y.islarger();
                if (sgn == 1 && cmp != 1) || (sgn == 0 && cmp == 1) {
				    P.neg();
			    }
                P
            }
        } else {
            for i in 0..MB {
                 t[i]=b[i+1];
            }
            let rx=FP2::frombytes(&t);
            if typ == 0x04 {
		        for i in 0..MB {
				    t[i]=b[i+MB+1];
			    }
		        let ry=FP2::frombytes(&t);
		        ECP2::new_fp2s(&rx,&ry)
            } else {
                ECP2::new_fp2(&rx,typ&1)
            }
        }
    }

    /* convert this to hex string */
    #[cfg(feature = "std")]
    pub fn tostring(&self) -> String {
        let mut W = ECP2::new();
        W.copy(self);
        W.affine();
        if W.is_infinity() {
            String::from("infinity")
        } else {
            format!("({},{})", W.x.tostring(), W.y.tostring())
        }
    }

    /* Calculate RHS of twisted curve equation x^3+B/i */
    pub fn rhs(x: &FP2) -> FP2 {
        let mut r = FP2::new_copy(x);
        r.sqr();
        let mut b = FP2::new_big(&BIG::new_ints(&rom::CURVE_B));
        if ecp::SEXTIC_TWIST == ecp::D_TYPE {
            b.div_ip();
        }
        if ecp::SEXTIC_TWIST == ecp::M_TYPE {
            b.norm();
            b.mul_ip();
            b.norm();
        }

        r.mul(x);
        r.add(&b);

        r.reduce();
        r
    }

    /* self+=self */
    pub fn dbl(&mut self) -> isize {
        let mut iy = FP2::new_copy(&self.y);
        if ecp::SEXTIC_TWIST == ecp::D_TYPE {
            iy.mul_ip();
            iy.norm();
        }

        let mut t0 = FP2::new_copy(&self.y); //***** Change
        t0.sqr();
        if ecp::SEXTIC_TWIST == ecp::D_TYPE {
            t0.mul_ip();
        }
        let mut t1 = FP2::new_copy(&iy);
        t1.mul(&self.z);
        let mut t2 = FP2::new_copy(&self.z);
        t2.sqr();

        self.z.copy(&t0);
        self.z.add(&t0);
        self.z.norm();
        self.z.dbl();
        self.z.dbl();
        self.z.norm();

        t2.imul(3 * rom::CURVE_B_I);
        if ecp::SEXTIC_TWIST == ecp::M_TYPE {
            t2.mul_ip();
            t2.norm();
        }
        let mut x3 = FP2::new_copy(&t2);
        x3.mul(&self.z);

        let mut y3 = FP2::new_copy(&t0);

        y3.add(&t2);
        y3.norm();
        self.z.mul(&t1);
        t1.copy(&t2);
        t1.add(&t2);
        t2.add(&t1);
        t2.norm();
        t0.sub(&t2);
        t0.norm(); //y^2-9bz^2
        y3.mul(&t0);
        y3.add(&x3); //(y^2+3z*2)(y^2-9z^2)+3b.z^2.8y^2
        t1.copy(&self.x);
        t1.mul(&iy); //
        self.x.copy(&t0);
        self.x.norm();
        self.x.mul(&t1);
        self.x.dbl(); //(y^2-9bz^2)xy2

        self.x.norm();
        self.y.copy(&y3);
        self.y.norm();

        1
    }

    /* self+=Q - return 0 for add, 1 for double, -1 for O */
    pub fn add(&mut self, Q: &ECP2) -> isize {
        let b = 3 * rom::CURVE_B_I;
        let mut t0 = FP2::new_copy(&self.x);
        t0.mul(&Q.x); // x.Q.x
        let mut t1 = FP2::new_copy(&self.y);
        t1.mul(&Q.y); // y.Q.y

        let mut t2 = FP2::new_copy(&self.z);
        t2.mul(&Q.z);
        let mut t3 = FP2::new_copy(&self.x);
        t3.add(&self.y);
        t3.norm(); //t3=X1+Y1
        let mut t4 = FP2::new_copy(&Q.x);
        t4.add(&Q.y);
        t4.norm(); //t4=X2+Y2
        t3.mul(&t4); //t3=(X1+Y1)(X2+Y2)
        t4.copy(&t0);
        t4.add(&t1); //t4=X1.X2+Y1.Y2

        t3.sub(&t4);
        t3.norm();
        if ecp::SEXTIC_TWIST == ecp::D_TYPE {
            t3.mul_ip();
            t3.norm(); //t3=(X1+Y1)(X2+Y2)-(X1.X2+Y1.Y2) = X1.Y2+X2.Y1
        }
        t4.copy(&self.y);
        t4.add(&self.z);
        t4.norm(); //t4=Y1+Z1
        let mut x3 = FP2::new_copy(&Q.y);
        x3.add(&Q.z);
        x3.norm(); //x3=Y2+Z2

        t4.mul(&x3); //t4=(Y1+Z1)(Y2+Z2)
        x3.copy(&t1); //
        x3.add(&t2); //X3=Y1.Y2+Z1.Z2

        t4.sub(&x3);
        t4.norm();
        if ecp::SEXTIC_TWIST == ecp::D_TYPE {
            t4.mul_ip();
            t4.norm(); //t4=(Y1+Z1)(Y2+Z2) - (Y1.Y2+Z1.Z2) = Y1.Z2+Y2.Z1
        }
        x3.copy(&self.x);
        x3.add(&self.z);
        x3.norm(); // x3=X1+Z1
        let mut y3 = FP2::new_copy(&Q.x);
        y3.add(&Q.z);
        y3.norm(); // y3=X2+Z2
        x3.mul(&y3); // x3=(X1+Z1)(X2+Z2)
        y3.copy(&t0);
        y3.add(&t2); // y3=X1.X2+Z1+Z2
        y3.rsub(&x3);
        y3.norm(); // y3=(X1+Z1)(X2+Z2) - (X1.X2+Z1.Z2) = X1.Z2+X2.Z1

        if ecp::SEXTIC_TWIST == ecp::D_TYPE {
            t0.mul_ip();
            t0.norm(); // x.Q.x
            t1.mul_ip();
            t1.norm(); // y.Q.y
        }
        x3.copy(&t0);
        x3.add(&t0);
        t0.add(&x3);
        t0.norm();
        t2.imul(b);
        if ecp::SEXTIC_TWIST == ecp::M_TYPE {
            t2.mul_ip();
            t2.norm();
        }
        let mut z3 = FP2::new_copy(&t1);
        z3.add(&t2);
        z3.norm();
        t1.sub(&t2);
        t1.norm();
        y3.imul(b);
        if ecp::SEXTIC_TWIST == ecp::M_TYPE {
            y3.mul_ip();
            y3.norm();
        }
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

        0
    }

    /* set this-=Q */
    pub fn sub(&mut self, Q: &ECP2) -> isize {
        let mut NQ = ECP2::new();
        NQ.copy(Q);
        NQ.neg();
        self.add(&NQ)
    }

    /* set this*=q, where q is Modulus, using Frobenius */
    pub fn frob(&mut self, x: &FP2) {
        let mut x2 = FP2::new_copy(x);
        x2.sqr();
        self.x.conj();
        self.y.conj();
        self.z.conj();
        self.z.reduce();
        self.x.mul(&x2);
        self.y.mul(&x2);
        self.y.mul(x);
    }

    /* self*=e */
    pub fn mul(&self, e: &BIG) -> ECP2 {
        /* fixed size windows */
        let mut mt = BIG::new();
        let mut t = BIG::new();
        let mut P = ECP2::new();
        let mut Q = ECP2::new();
        let mut C = ECP2::new();

        if self.is_infinity() {
            return P;
        }

        let mut W: [ECP2; 8] = [
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
        ];

        const CT: usize = 1 + (big::NLEN * (big::BASEBITS as usize) + 3) / 4;
        let mut w: [i8; CT] = [0; CT];

        /* precompute table */
        Q.copy(&self);
        Q.dbl();

        W[0].copy(&self);

        for i in 1..8 {
            C.copy(&W[i - 1]);
            W[i].copy(&C);
            W[i].add(&Q);
        }

        /* make exponent odd - add 2P if even, P if odd */
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

        let nb = 1 + (t.nbits() + 3) / 4;

        /* convert exponent to signed 4-bit window */
        for i in 0..nb {
            w[i] = (t.lastbits(5) - 16) as i8;
            t.dec(w[i] as isize);
            t.norm();
            t.fshr(4);
        }
        w[nb] = (t.lastbits(5)) as i8;

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
        P.sub(&C);
        P
    }

    #[allow(non_snake_case)]
    pub fn cfp(&mut self)  {
        let mut X = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));
        if ecp::SEXTIC_TWIST == ecp::M_TYPE {
            X.inverse(None);
            X.norm();
        }
        let x = BIG::new_ints(&rom::CURVE_BNX);
    // Faster Hashing to G2 - Fuentes-Castaneda, Knapp and Rodriguez-Henriquez
    // Q -> xQ + F(3xQ) + F(F(xQ)) + F(F(F(Q))).
        if ecp::CURVE_PAIRING_TYPE == ecp::BN {
            let mut T = self.mul(&x);
            if ecp::SIGN_OF_X == ecp::NEGATIVEX {
                T.neg();
            }
            let mut K = ECP2::new();
            K.copy(&T);
            K.dbl();
            K.add(&T);

            K.frob(&X);
            self.frob(&X);
            self.frob(&X);
            self.frob(&X);
            self.add(&T);
            self.add(&K);
            T.frob(&X);
            T.frob(&X);
            self.add(&T);
        }
    // Efficient hash maps to G2 on BLS curves - Budroni, Pintore
    // Q -> x2Q -xQ -Q +F(xQ -Q) +F(F(2Q))
        if ecp::CURVE_PAIRING_TYPE > ecp::BN {
            let mut xQ = self.mul(&x);
            let mut x2Q = xQ.mul(&x);

            if ecp::SIGN_OF_X == ecp::NEGATIVEX {
                xQ.neg();
            }
            x2Q.sub(&xQ);
            x2Q.sub(&self);

            xQ.sub(&self);
            xQ.frob(&X);

            self.dbl();
            self.frob(&X);
            self.frob(&X);

            self.add(&x2Q);
            self.add(&xQ);
        }
    }


    /* P=u0.Q0+u1*Q1+u2*Q2+u3*Q3 */
    // Bos & Costello https://eprint.iacr.org/2013/458.pdf
    // Faz-Hernandez & Longa & Sanchez  https://eprint.iacr.org/2013/158.pdf
    // Side channel attack secure

    pub fn mul4(Q: &[ECP2], u: &[BIG]) -> ECP2 {
        let mut W = ECP2::new();
        let mut P = ECP2::new();

        let mut T: [ECP2; 8] = [
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
            ECP2::new(),
        ];

        let mut mt = BIG::new();

        let mut t: [BIG; 4] = [
            BIG::new_copy(&u[0]),
            BIG::new_copy(&u[1]),
            BIG::new_copy(&u[2]),
            BIG::new_copy(&u[3]),
        ];

        const CT: usize = 1 + big::NLEN * (big::BASEBITS as usize);
        let mut w: [i8; CT] = [0; CT];
        let mut s: [i8; CT] = [0; CT];

        for i in 0..4 {
            t[i].norm();
        }

        T[0].copy(&Q[0]);
        W.copy(&T[0]);
        T[1].copy(&W);
        T[1].add(&Q[1]); // Q[0]+Q[1]
        T[2].copy(&W);
        T[2].add(&Q[2]);
        W.copy(&T[1]); // Q[0]+Q[2]
        T[3].copy(&W);
        T[3].add(&Q[2]);
        W.copy(&T[0]); // Q[0]+Q[1]+Q[2]
        T[4].copy(&W);
        T[4].add(&Q[3]);
        W.copy(&T[1]); // Q[0]+Q[3]
        T[5].copy(&W);
        T[5].add(&Q[3]);
        W.copy(&T[2]); // Q[0]+Q[1]+Q[3]
        T[6].copy(&W);
        T[6].add(&Q[3]);
        W.copy(&T[3]); // Q[0]+Q[2]+Q[3]
        T[7].copy(&W);
        T[7].add(&Q[3]); // Q[0]+Q[1]+Q[2]+Q[3]

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
        P.selector(&T, (2 * w[nb - 1] + 1) as i32);
        for i in (0..nb - 1).rev() {
            P.dbl();
            W.selector(&T, (2 * w[i] + s[i]) as i32);
            P.add(&W);
        }

        // apply correction
        W.copy(&P);
        W.sub(&Q[0]);
        P.cmove(&W, pb);

        P
    }

/* Hunt and Peck a BIG to a curve point */
    #[allow(non_snake_case)]
    pub fn hap2point(h: &BIG) -> ECP2 {
        let mut Q: ECP2;
        let one = BIG::new_int(1);
        let mut x =BIG::new_copy(&h);
        loop {
            let X = FP2::new_bigs(&one, &x);
            Q = ECP2::new_fp2(&X,0);
            if !Q.is_infinity() {
                break;
            }
            x.inc(1);
            x.norm();
        }
        Q
    }

/* Constant time Map to Point */
    #[allow(unreachable_code)]
    #[allow(non_snake_case)]
    pub fn map2point(H: &FP2) -> ECP2 {
        let mut T=FP2::new_copy(H); /**/
        let sgn=T.sign(); /**/
        if ecp::HTC_ISO_G2 == 0 {
    // Shallue and van de Woestijne
/* */
            let mut NY=FP2::new_int(1);
            let mut Z=FP::new_int(fp::RIADZG2A);
            let mut X1=FP2::new_fp(&Z);
            let mut X3=FP2::new_copy(&X1);
            let mut A=ECP2::rhs(&X1);
            let mut W=FP2::new_copy(&A);

            if fp::RIADZG2A==-1 && fp::RIADZG2B==0 && ecp::SEXTIC_TWIST == ecp::M_TYPE && rom::CURVE_B_I==4 {
                W.copy(&FP2::new_ints(2,1));
            } else {
                W.sqrt(None);
            }
            let s = FP::new_big(&BIG::new_ints(&rom::SQRTM3));
            Z.mul(&s);

            T.sqr();
            let mut Y=FP2::new_copy(&A); Y.mul(&T);
            T.copy(&NY); T.add(&Y); T.norm();
            Y.rsub(&NY); Y.norm();
            NY.copy(&T); NY.mul(&Y); 
        
            NY.pmul(&Z);
            NY.inverse(None);

            W.pmul(&Z);
            if W.sign()==1 {
                W.neg();
                W.norm();
            }
            W.pmul(&Z);
            W.mul(&H); W.mul(&Y); W.mul(&NY);

            X1.neg(); X1.norm(); X1.div2();
            let mut X2=FP2::new_copy(&X1);
            X1.sub(&W); X1.norm();
            X2.add(&W); X2.norm();
            A.dbl(); A.dbl(); A.norm();
            T.sqr(); T.mul(&NY); T.sqr();
            A.mul(&T);
            X3.add(&A); X3.norm();

            Y.copy(&ECP2::rhs(&X2));
            X3.cmove(&X2,Y.qr(None));
            Y.copy(&ECP2::rhs(&X1));
            X3.cmove(&X1,Y.qr(None));
            Y.copy(&ECP2::rhs(&X3));
            Y.sqrt(None);

            let ne=Y.sign()^sgn;
            W.copy(&Y); W.neg(); W.norm();
            Y.cmove(&W,ne);

            return ECP2::new_fp2s(&X3,&Y);
/* */
        } else {
/* CAHCZS
            let NY=FP2::new_int(1);
            let Ad=FP2::new_bigs(&BIG::new_ints(&rom::CURVE_ADR),&BIG::new_ints(&rom::CURVE_ADI));
            let Bd=FP2::new_bigs(&BIG::new_ints(&rom::CURVE_BDR),&BIG::new_ints(&rom::CURVE_BDI));  
            let ZZ=FP2::new_ints(fp::RIADZG2A,fp::RIADZG2B);
            let mut hint=FP::new();

            T.sqr();
            T.mul(&ZZ);
            let mut W=FP2::new_copy(&T);
            W.add(&NY); W.norm();

            W.mul(&T);
            let mut D=FP2::new_copy(&Ad);
            D.mul(&W);
    
            W.add(&NY); W.norm();
            W.mul(&Bd);
            W.neg(); W.norm();

            let mut X2=FP2::new_copy(&W);
            let mut X3=FP2::new_copy(&T);
            X3.mul(&X2);

            let mut GX1=FP2::new_copy(&X2); GX1.sqr();
            let mut D2=FP2::new_copy(&D); D2.sqr();

            W.copy(&Ad); W.mul(&D2); GX1.add(&W); GX1.norm(); GX1.mul(&X2); D2.mul(&D); W.copy(&Bd); W.mul(&D2); GX1.add(&W); GX1.norm(); // x^3+Ax+b

            W.copy(&GX1); W.mul(&D);
            let qr=W.qr(Some(&mut hint));
            D.copy(&W); D.inverse(Some(&hint));
            D.mul(&GX1);
            X2.mul(&D);
            X3.mul(&D);
            T.mul(&H);
            D2.copy(&D); D2.sqr();

            D.copy(&D2); D.mul(&T);
            T.copy(&W); T.mul(&ZZ);

            let mut s=FP::new_big(&BIG::new_ints(&rom::CURVE_HTPC2));
            s.mul(&hint);

            X2.cmove(&X3,1-qr);
            W.cmove(&T,1-qr);
            D2.cmove(&D,1-qr);
            hint.cmove(&s,1-qr);

            let mut Y=FP2::new_copy(&W); Y.sqrt(Some(&hint));
            Y.mul(&D2);

            let ne=Y.sign()^sgn;
            W.copy(&Y); W.neg(); W.norm();
            Y.cmove(&W,ne);

            let mut k=0;
            let isox=ecp::HTC_ISO_G2;
            let isoy=3*(isox-1)/2;

        // xnum
            let mut xnum=FP2::new_bigs(&BIG::new_ints(&rom::PCR[k]),&BIG::new_ints(&rom::PCI[k])); k+=1;
            for _ in 0..isox {
                xnum.mul(&X2);
                xnum.add(&FP2::new_bigs(&BIG::new_ints(&rom::PCR[k]),&BIG::new_ints(&rom::PCI[k]))); k+=1;
                xnum.norm();
            }
        //xden
            let mut xden=FP2::new_copy(&X2);
            xden.add(&FP2::new_bigs(&BIG::new_ints(&rom::PCR[k]),&BIG::new_ints(&rom::PCI[k]))); k+=1;
            xden.norm();
            for _ in 0..isox-2 {
                xden.mul(&X2);
                xden.add(&FP2::new_bigs(&BIG::new_ints(&rom::PCR[k]),&BIG::new_ints(&rom::PCI[k]))); k+=1;
                xden.norm();                
            }
        //ynum
            let mut ynum=FP2::new_bigs(&BIG::new_ints(&rom::PCR[k]),&BIG::new_ints(&rom::PCI[k])); k+=1;            
            for _ in 0..isoy {
                ynum.mul(&X2);
                ynum.add(&FP2::new_bigs(&BIG::new_ints(&rom::PCR[k]),&BIG::new_ints(&rom::PCI[k]))); k+=1;
                ynum.norm();
            }
        //yden
            let mut yden=FP2::new_copy(&X2);
            yden.add(&FP2::new_bigs(&BIG::new_ints(&rom::PCR[k]),&BIG::new_ints(&rom::PCI[k]))); k+=1;
            yden.norm(); 
            for _ in 0..isoy-1 {
                yden.mul(&X2);
                yden.add(&FP2::new_bigs(&BIG::new_ints(&rom::PCR[k]),&BIG::new_ints(&rom::PCI[k]))); k+=1;
                yden.norm();
            }
            ynum.mul(&Y);

            let mut Q=ECP2::new();
            T.copy(&xnum); T.mul(&yden);
            Q.x.copy(&T);
            T.copy(&ynum); T.mul(&xden);
            Q.y.copy(&T);
            T.copy(&xden); T.mul(&yden);
            Q.z.copy(&T);
            return Q;
CAHCZF */

        }
        ECP2::new()
    }

/* Map byte string to curve point */
    #[allow(non_snake_case)]
    pub fn mapit(h: &[u8]) -> ECP2 {
        let q = BIG::new_ints(&rom::MODULUS);
        let mut dx = DBIG::frombytes(h);
        let x=dx.dmod(&q);
        let mut P=ECP2::hap2point(&x);
        P.cfp();
        P
    }

    pub fn generator() -> ECP2 {
        ECP2::new_fp2s(
            &FP2::new_bigs(
                &BIG::new_ints(&rom::CURVE_PXA),
                &BIG::new_ints(&rom::CURVE_PXB),
            ),
            &FP2::new_bigs(
                &BIG::new_ints(&rom::CURVE_PYA),
                &BIG::new_ints(&rom::CURVE_PYB),
            ),
        )
    }
}
