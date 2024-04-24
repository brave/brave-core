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

use crate::bn254::big::BIG;
use crate::bn254::ecp;
use crate::bn254::ecp::ECP;
use crate::bn254::ecp2::ECP2;
use crate::bn254::fp::FP;
use crate::bn254::fp12;
use crate::bn254::fp12::FP12;
use crate::bn254::fp2::FP2;
use crate::bn254::fp4::FP4;
use crate::bn254::rom;

#[allow(unused_imports)]
use crate::bn254::dbig::DBIG;

#[allow(non_snake_case)]
fn dbl(A: &mut ECP2, aa: &mut FP2, bb: &mut FP2, cc: &mut FP2)  {
    cc.copy(&A.getpx()); //X
    let mut yy = FP2::new_copy(&A.getpy()); //Y
    bb.copy(&A.getpz()); //Z

    aa.copy(&yy); //Y
    aa.mul(&bb); //YZ
    cc.sqr(); //X^2
    yy.sqr(); //Y^2
    bb.sqr(); //Z^2

    aa.dbl();
    aa.neg(); aa.norm();
    aa.mul_ip();
    aa.norm();

    let sb = 3 * rom::CURVE_B_I;
    bb.imul(sb);
    cc.imul(3);
    if ecp::SEXTIC_TWIST == ecp::D_TYPE {
	yy.mul_ip();
	yy.norm();
	cc.mul_ip();
	cc.norm();
    }
    if ecp::SEXTIC_TWIST == ecp::M_TYPE {
	bb.mul_ip();
	bb.norm();
    }
    bb.sub(&yy);
    bb.norm();
    A.dbl();
}

#[allow(non_snake_case)]
fn add(A: &mut ECP2, B: &ECP2, aa: &mut FP2, bb: &mut FP2, cc: &mut FP2) {

    aa.copy(&A.getpx()); // X1
    cc.copy(&A.getpy()); // Y1
    let mut t1 = FP2::new_copy(&A.getpz()); // Z1
    bb.copy(&A.getpz()); // Z1

    t1.mul(&B.getpy()); // T1=Z1.Y2
    bb.mul(&B.getpx()); // T2=Z1.X2

    aa.sub(&bb);
    aa.norm(); // X1=X1-Z1.X2
    cc.sub(&t1);
    cc.norm(); // Y1=Y1-Z1.Y2

    t1.copy(&aa); // T1=X1-Z1.X2
    if ecp::SEXTIC_TWIST == ecp::M_TYPE {
        aa.mul_ip();
        aa.norm();
    }

    t1.mul(&B.getpy()); // T1=(X1-Z1.X2).Y2

    bb.copy(&cc); // T2=Y1-Z1.Y2
    bb.mul(&B.getpx()); // T2=(Y1-Z1.Y2).X2
    bb.sub(&t1);
    bb.norm(); // T2=(Y1-Z1.Y2).X2 - (X1-Z1.X2).Y2

    cc.neg();
    cc.norm(); // Y1=-(Y1-Z1.Y2).Xs

    A.add(B);
}

#[allow(non_snake_case)]
fn linedbl(A: &mut ECP2, qx: &FP, qy: &FP) -> FP12 {
    let mut a = FP4::new();
    let mut b = FP4::new();
    let mut c = FP4::new();
    let mut aa = FP2::new(); 
    let mut bb = FP2::new(); 
    let mut cc = FP2::new(); 

    dbl(A,&mut aa,&mut bb,&mut cc);

    cc.pmul(qx);
    aa.pmul(qy);

    a.copy(&FP4::new_fp2s(&aa, &bb)); // -2YZ.Ys | 3b.Z^2-Y^2 | 3X^2.Xs
    if ecp::SEXTIC_TWIST == ecp::D_TYPE {
        b.copy(&FP4::new_fp2(&cc)); // L(0,1) | L(0,0) | L(1,0)
    }
    if ecp::SEXTIC_TWIST == ecp::M_TYPE {
        c.copy(&FP4::new_fp2(&cc));
        c.times_i();
    }
    let mut res= FP12::new_fp4s(&a, &b, &c);
    res.settype(fp12::SPARSER);
    res
}

#[allow(non_snake_case)]
fn lineadd(A: &mut ECP2, B: &ECP2, qx: &FP, qy: &FP) -> FP12 {
    let mut a = FP4::new();
    let mut b = FP4::new();
    let mut c = FP4::new();
    let mut aa = FP2::new(); 
    let mut bb = FP2::new(); 
    let mut cc = FP2::new(); 

    add(A,B,&mut aa,&mut bb,&mut cc);

    cc.pmul(qx);
    aa.pmul(qy);

    a.copy(&FP4::new_fp2s(&aa, &bb)); // -2YZ.Ys | 3b.Z^2-Y^2 | 3X^2.Xs
    if ecp::SEXTIC_TWIST == ecp::D_TYPE {
        b.copy(&FP4::new_fp2(&cc)); // L(0,1) | L(0,0) | L(1,0)
    }
    if ecp::SEXTIC_TWIST == ecp::M_TYPE {
        c.copy(&FP4::new_fp2(&cc));
        c.times_i();
    }
    let mut res= FP12::new_fp4s(&a, &b, &c);
    res.settype(fp12::SPARSER);
    res
}

/* prepare ate parameter, n=6u+2 (BN) or n=u (BLS), n3=3*n */
#[allow(non_snake_case)]
fn lbits(n3: &mut BIG,n: &mut BIG) -> usize {
    n.copy(&BIG::new_ints(&rom::CURVE_BNX));
    if ecp::CURVE_PAIRING_TYPE==ecp::BN {
        n.pmul(6);
        if ecp::SIGN_OF_X==ecp::POSITIVEX {
            n.inc(2);
        } else {
            n.dec(2);
        }
    }
    n.norm();
    n3.copy(&n);
    n3.pmul(3);
    n3.norm();
    n3.nbits()
}

/* prepare for multi-pairing */
pub fn initmp() -> [FP12; ecp::ATE_BITS] {
    [FP12::new_int(1); ecp::ATE_BITS]
}

/* basic Miller loop */
pub fn miller(r:&mut [FP12]) -> FP12 {
    let mut res=FP12::new_int(1);
    for i in (1..ecp::ATE_BITS).rev() {
        res.sqr();
        res.ssmul(&r[i]);
	r[i].zero();
    }

    if ecp::SIGN_OF_X==ecp::NEGATIVEX {
        res.conj();
    }
    res.ssmul(&r[0]);
    r[0].zero();
    res
}

fn pack(aa: &FP2,bb: &FP2,cc: &FP2) -> FP4 {
    let mut i=FP2::new_copy(cc);
    i.inverse(None);
    let mut a=FP2::new_copy(aa);
    let mut b=FP2::new_copy(bb);
    a.mul(&i);
    b.mul(&i);
    FP4::new_fp2s(&a,&b)
}

fn unpack(t: &FP4, qx: &FP, qy: &FP) -> FP12 {
    let b:FP4;
    let mut c:FP4;
    let w=FP2::new_fp(qx);

    let mut aa=t.geta();
    let bb=t.getb();
    aa.pmul(qy);
    let a=FP4::new_fp2s(&aa,&bb);

    if ecp::SEXTIC_TWIST==ecp::D_TYPE {
        b=FP4::new_fp2(&w);
	c=FP4::new();
    } else {
        b=FP4::new();
	c=FP4::new_fp2(&w); c.times_i();
    }
    let mut v=FP12::new_fp4s(&a,&b,&c);
    v.settype(fp12::SPARSEST);
    v
}

#[allow(non_snake_case)]
pub fn precomp(T: &mut [FP4],GV: &ECP2) {
    let mut n = BIG::new();
    let mut n3 = BIG::new();
    let nb=lbits(&mut n3,&mut n);
    let mut f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));
    let mut aa = FP2::new(); 
    let mut bb = FP2::new(); 
    let mut cc = FP2::new(); 

    let mut P=ECP2::new(); P.copy(GV);

    if (ecp::CURVE_PAIRING_TYPE==ecp::BN) && (ecp::SEXTIC_TWIST==ecp::M_TYPE) {
        f.inverse(None);
        f.norm();
    }

    let mut A = ECP2::new();

    A.copy(&P);
    let mut NP = ECP2::new();
    NP.copy(&P);
    NP.neg();
    let mut j=0;

    for i in (1..nb-1).rev() {
        dbl(&mut A,&mut aa,&mut bb,&mut cc);
	T[j].copy(&pack(&aa,&bb,&cc)); j+=1;
	let bt=n3.bit(i)-n.bit(i);
	if bt==1 {
	    add(&mut A,&P,&mut aa,&mut bb,&mut cc);
	    T[j].copy(&pack(&aa,&bb,&cc)); j+=1;
	}
	if bt==-1 {
	    add(&mut A,&NP,&mut aa,&mut bb,&mut cc);
	    T[j].copy(&pack(&aa,&bb,&cc)); j+=1;
	}
    }
/* R-ate fixup required for BN curves */
    if ecp::CURVE_PAIRING_TYPE==ecp::BN {
        if ecp::SIGN_OF_X==ecp::NEGATIVEX {
            A.neg();
        }
        let mut K = ECP2::new();
        K.copy(&P);
        K.frob(&f);
        add(&mut A,&K,&mut aa,&mut bb,&mut cc);
        T[j].copy(&pack(&aa,&bb,&cc)); j+=1;
        K.frob(&f);
        K.neg();
        add(&mut A,&K,&mut aa,&mut bb,&mut cc);
        T[j].copy(&pack(&aa,&bb,&cc)); 
    } 
}

/* Accumulate another set of line functions for n-pairing, assuming precomputation on G2 */
#[allow(non_snake_case)]
pub fn another_pc(r:&mut [FP12],T: &[FP4],QV: &ECP) {
    let mut n = BIG::new();
    let mut n3 = BIG::new();

    if QV.is_infinity() {
        return;
    }

    let nb=lbits(&mut n3,&mut n);

    let mut Q = ECP::new();
    Q.copy(QV);
    Q.affine();
    let qx = FP::new_copy(&Q.getpx());
    let qy = FP::new_copy(&Q.getpy());

    let mut j=0;
    for i in (1..nb-1).rev() {
        let mut lv=unpack(&T[j],&qx,&qy); j+=1;
	let bt=n3.bit(i)-n.bit(i);
	if bt==1 {
	    let lv2=unpack(&T[j],&qx,&qy); j+=1;
	    lv.smul(&lv2);
	}
	if bt==-1 {
	    let lv2=unpack(&T[j],&qx,&qy); j+=1;
	    lv.smul(&lv2);
	}
        r[i].ssmul(&lv);
    }
    if ecp::CURVE_PAIRING_TYPE==ecp::BN {
        let mut lv=unpack(&T[j],&qx,&qy); j+=1;  
	let lv2=unpack(&T[j],&qx,&qy); 
	lv.smul(&lv2);
	r[0].ssmul(&lv);
    }
}

/* Accumulate another set of line functions for n-pairing */
#[allow(non_snake_case)]
pub fn another(r:&mut [FP12],P1: &ECP2,Q1: &ECP) {
    let mut f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));
    let mut n = BIG::new();
    let mut n3 = BIG::new();
    let mut K = ECP2::new();

    if Q1.is_infinity() {
        return;
    }
// P is needed in affine form for line function, Q for (Qx,Qy) extraction
    let mut P = ECP2::new();
    P.copy(P1);
    P.affine();
    let mut Q = ECP::new();
    Q.copy(Q1);
    Q.affine();

    if (ecp::CURVE_PAIRING_TYPE==ecp::BN) && (ecp::SEXTIC_TWIST==ecp::M_TYPE) {
        f.inverse(None);
        f.norm();
    }

    let qx = FP::new_copy(&Q.getpx());
    let qy = FP::new_copy(&Q.getpy());
    let mut A = ECP2::new();

    A.copy(&P);
    let mut NP = ECP2::new();
    NP.copy(&P);
    NP.neg();

    let nb=lbits(&mut n3,&mut n);

    for i in (1..nb-1).rev() {
        let mut lv=linedbl(&mut A,&qx,&qy);

	let bt=n3.bit(i)-n.bit(i);
        if bt==1 {
            let lv2=lineadd(&mut A,&P,&qx,&qy);
            lv.smul(&lv2);
        }
        if bt==-1 {
            let lv2=lineadd(&mut A,&NP,&qx,&qy);
            lv.smul(&lv2);
        }
        r[i].ssmul(&lv);
    }

/* R-ate fixup required for BN curves */
    if ecp::CURVE_PAIRING_TYPE==ecp::BN {
        if ecp::SIGN_OF_X==ecp::NEGATIVEX {
            A.neg();
        }
        K.copy(&P);
        K.frob(&f);
        let mut lv=lineadd(&mut A,&K,&qx,&qy);
        K.frob(&f);
        K.neg();
        let lv2=lineadd(&mut A,&K,&qx,&qy);
        lv.smul(&lv2);
	r[0].ssmul(&lv);
    } 
}

#[allow(non_snake_case)]
/* Optimal R-ate pairing */
pub fn ate(P1: &ECP2, Q1: &ECP) -> FP12 {
    let mut f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));
    let mut n = BIG::new();
    let mut n3 = BIG::new();
    let mut K = ECP2::new();

    if Q1.is_infinity() {
        return FP12::new_int(1);
    }

    if (ecp::CURVE_PAIRING_TYPE == ecp::BN) && (ecp::SEXTIC_TWIST == ecp::M_TYPE) {
        f.inverse(None);
        f.norm();
    } 
    let mut P = ECP2::new();
    P.copy(P1);
    P.affine();
    let mut Q = ECP::new();
    Q.copy(Q1);
    Q.affine();

    let qx = FP::new_copy(&Q.getpx());
    let qy = FP::new_copy(&Q.getpy());

    let mut A = ECP2::new();
    let mut r = FP12::new_int(1);

    A.copy(&P);
    let mut NP = ECP2::new();
    NP.copy(&P);
    NP.neg();

    let nb=lbits(&mut n3,&mut n);

    for i in (1..nb - 1).rev() {
        r.sqr();
        let mut lv = linedbl(&mut A, &qx, &qy);
        let bt = n3.bit(i) - n.bit(i);
        if bt == 1 {
            let lv2 = lineadd(&mut A, &P, &qx, &qy);
            lv.smul(&lv2);
        }
        if bt == -1 {
            let lv2 = lineadd(&mut A, &NP, &qx, &qy);
            lv.smul(&lv2);
        }
        r.ssmul(&lv);
    }

    if ecp::SIGN_OF_X == ecp::NEGATIVEX {
        r.conj();
    }

    /* R-ate fixup required for BN curves */

    if ecp::CURVE_PAIRING_TYPE == ecp::BN {
        if ecp::SIGN_OF_X == ecp::NEGATIVEX {
            A.neg();
        }

        K.copy(&P);
        K.frob(&f);

        let mut lv = lineadd(&mut A, &K, &qx, &qy);
        K.frob(&f);
        K.neg();
        let lv2 = lineadd(&mut A, &K, &qx, &qy);
	lv.smul(&lv2);
        r.ssmul(&lv);
    }

    r
}

#[allow(non_snake_case)]
/* Optimal R-ate double pairing e(P,Q).e(R,S) */
pub fn ate2(P1: &ECP2, Q1: &ECP, R1: &ECP2, S1: &ECP) -> FP12 {
    let mut f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));
    let mut n = BIG::new();
    let mut n3 = BIG::new();
    let mut K = ECP2::new();

    if Q1.is_infinity() {
        return ate(R1,S1);
    }
    if S1.is_infinity() {
        return ate(P1,Q1);
    }

    if (ecp::CURVE_PAIRING_TYPE == ecp::BN) && (ecp::SEXTIC_TWIST == ecp::M_TYPE) {
        f.inverse(None);
        f.norm();
    } 

    let mut P = ECP2::new();
    P.copy(P1);
    P.affine();
    let mut Q = ECP::new();
    Q.copy(Q1);
    Q.affine();
    let mut R = ECP2::new();
    R.copy(R1);
    R.affine();
    let mut S = ECP::new();
    S.copy(S1);
    S.affine();

    let qx = FP::new_copy(&Q.getpx());
    let qy = FP::new_copy(&Q.getpy());

    let sx = FP::new_copy(&S.getpx());
    let sy = FP::new_copy(&S.getpy());

    let mut A = ECP2::new();
    let mut B = ECP2::new();
    let mut r = FP12::new_int(1);

    A.copy(&P);
    B.copy(&R);

    let mut NP = ECP2::new();
    NP.copy(&P);
    NP.neg();
    let mut NR = ECP2::new();
    NR.copy(&R);
    NR.neg();

    let nb=lbits(&mut n3,&mut n);

    for i in (1..nb - 1).rev() {
        r.sqr();
        let mut lv = linedbl(&mut A, &qx, &qy);
        let lv2 = linedbl(&mut B, &sx, &sy);
	lv.smul(&lv2);
        r.ssmul(&lv);
        let bt = n3.bit(i) - n.bit(i);
        if bt == 1 {
            lv = lineadd(&mut A, &P, &qx, &qy);
            let lv2 = lineadd(&mut B, &R, &sx, &sy);
	    lv.smul(&lv2);
            r.ssmul(&lv);
        }
        if bt == -1 {
            lv = lineadd(&mut A, &NP, &qx, &qy);
            let lv2 = lineadd(&mut B, &NR, &sx, &sy);
	    lv.smul(&lv2);
            r.ssmul(&lv);
        }
    }

    if ecp::SIGN_OF_X == ecp::NEGATIVEX {
        r.conj();
    }

    /* R-ate fixup */
    if ecp::CURVE_PAIRING_TYPE == ecp::BN {
        if ecp::SIGN_OF_X == ecp::NEGATIVEX {
            A.neg();
            B.neg();
        }
        K.copy(&P);
        K.frob(&f);

        let mut lv = lineadd(&mut A, &K, &qx, &qy);
        K.frob(&f);
        K.neg();
        let mut lv2 = lineadd(&mut A, &K, &qx, &qy);
	lv.smul(&lv2);
        r.ssmul(&lv);

        K.copy(&R);
        K.frob(&f);

        lv = lineadd(&mut B, &K, &sx, &sy);
        K.frob(&f);
        K.neg();
        lv2 = lineadd(&mut B, &K, &sx, &sy);
	lv.smul(&lv2);
        r.ssmul(&lv);

    }

    r
}

/* final exponentiation - keep separate for multi-pairings and to avoid thrashing stack */
pub fn fexp(m: &FP12) -> FP12 {
    let f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));
    let x = BIG::new_ints(&rom::CURVE_BNX);
    let mut r = FP12::new_copy(m);

    /* Easy part of final exp */
    let mut lv = FP12::new_copy(&r);
    lv.inverse();
    r.conj();

    r.mul(&lv);
    lv.copy(&r);
    r.frob(&f);
    r.frob(&f);
    r.mul(&lv);

    /* Hard part of final exp */
    if ecp::CURVE_PAIRING_TYPE == ecp::BN {
        lv.copy(&r);
        lv.frob(&f);
        let mut x0 = FP12::new_copy(&lv);
        x0.frob(&f);
        lv.mul(&r);
        x0.mul(&lv);
        x0.frob(&f);
        let mut x1 = FP12::new_copy(&r);
        x1.conj();
        let mut x4 = r.pow(&x);
        if ecp::SIGN_OF_X == ecp::POSITIVEX {
            x4.conj();
        }

        let mut x3 = FP12::new_copy(&x4);
        x3.frob(&f);

        let mut x2 = x4.pow(&x);
        if ecp::SIGN_OF_X == ecp::POSITIVEX {
            x2.conj();
        }
        let mut x5 = FP12::new_copy(&x2);
        x5.conj();
        lv = x2.pow(&x);
        if ecp::SIGN_OF_X == ecp::POSITIVEX {
            lv.conj();
        }
        x2.frob(&f);
        r.copy(&x2);
        r.conj();

        x4.mul(&r);
        x2.frob(&f);

        r.copy(&lv);
        r.frob(&f);
        lv.mul(&r);

        lv.usqr();
        lv.mul(&x4);
        lv.mul(&x5);
        r.copy(&x3);
        r.mul(&x5);
        r.mul(&lv);
        lv.mul(&x2);
        r.usqr();
        r.mul(&lv);
        r.usqr();
        lv.copy(&r);
        lv.mul(&x1);
        r.mul(&x0);
        lv.usqr();
        r.mul(&lv);
        r.reduce();
    } else {

// See https://eprint.iacr.org/2020/875.pdf
        let mut y1 = FP12::new_copy(&r);
        y1.usqr();
        y1.mul(&r); // y1=r^3

        let mut y0 = FP12::new_copy(&r.pow(&x));
        if ecp::SIGN_OF_X == ecp::NEGATIVEX {
            y0.conj();
        }
        let mut t0 = FP12::new_copy(&r); t0.conj();
        r.copy(&y0);
        r.mul(&t0);

        y0.copy(&r.pow(&x));
        if ecp::SIGN_OF_X == ecp::NEGATIVEX {
            y0.conj();
        }
        t0.copy(&r); t0.conj();
        r.copy(&y0);
        r.mul(&t0);

// ^(x+p)
        y0.copy(&r.pow(&x));
        if ecp::SIGN_OF_X == ecp::NEGATIVEX {
            y0.conj();
        }
        t0.copy(&r);
        t0.frob(&f);
        r.copy(&y0);
        r.mul(&t0);

// ^(x^2+p^2-1)
        y0.copy(&r.pow(&x));
        y0.copy(&y0.pow(&x));
        t0.copy(&r);
        t0.frob(&f); t0.frob(&f);
        y0.mul(&t0);
        t0.copy(&r); t0.conj();
        r.copy(&y0);
        r.mul(&t0);

        r.mul(&y1);
        r.reduce();
    }
    r
}

#[allow(non_snake_case)]
/* GLV method */
fn glv(ee: &BIG) -> [BIG; 2] {
    let mut u: [BIG; 2] = [BIG::new(), BIG::new()];
    let q = BIG::new_ints(&rom::CURVE_ORDER);
    if ecp::CURVE_PAIRING_TYPE == ecp::BN {
/* */
        let mut t = BIG::new();
        let mut v: [BIG; 2] = [BIG::new(), BIG::new()];

        for i in 0..2 {
            t.copy(&BIG::new_ints(&rom::CURVE_W[i])); // why not just t=new BIG(ROM.CURVE_W[i]);
            let mut d: DBIG = BIG::mul(&t, &ee);
            v[i].copy(&d.ctdiv(&q,t.nbits()));
        }
        u[0].copy(&ee);
        for i in 0..2 {
            for j in 0..2 {
                t = BIG::new_ints(&rom::CURVE_SB[j][i]);
                t = BIG::modmul(&mut v[j], &mut t, &q);
                u[i].add(&q);
                u[i].sub(&t);
                u[i].ctmod(&q,1);
            }
        }
/* */
    } else {
        let x = BIG::new_ints(&rom::CURVE_BNX);
        let x2 = BIG::smul(&x, &x);
        let bd=q.nbits()-x2.nbits();
        u[0].copy(&ee);
        u[0].ctmod(&x2,bd);
        u[1].copy(&ee);
        u[1].ctdiv(&x2,bd);
        u[1].rsub(&q);
    }
    u
}

#[allow(non_snake_case)]
/* Galbraith & Scott Method */
pub fn gs(ee: &BIG) -> [BIG; 4] {
    let mut u: [BIG; 4] = [BIG::new(), BIG::new(), BIG::new(), BIG::new()];
    let q = BIG::new_ints(&rom::CURVE_ORDER);
    if ecp::CURVE_PAIRING_TYPE == ecp::BN {
/* */
        let mut t = BIG::new();

        let mut v: [BIG; 4] = [BIG::new(), BIG::new(), BIG::new(), BIG::new()];
        for i in 0..4 {
            t.copy(&BIG::new_ints(&rom::CURVE_WB[i]));
            let mut d: DBIG = BIG::mul(&t, &ee);
            v[i].copy(&d.ctdiv(&q,t.nbits()));
        }
        u[0].copy(&ee);
        for i in 0..4 {
            for j in 0..4 {
                t = BIG::new_ints(&rom::CURVE_BB[j][i]);
                t = BIG::modmul(&mut v[j], &mut t, &q);
                u[i].add(&q);
                u[i].sub(&t);
                u[i].ctmod(&q,1);
            }
        }
/* */
    } else {
        let x = BIG::new_ints(&rom::CURVE_BNX);
        let bd=q.nbits()-x.nbits();  // fixed

        let mut w = BIG::new_copy(&ee);
        for i in 0..3 {
            u[i].copy(&w);
            u[i].ctmod(&x,bd);
            w.ctdiv(&x,bd);
        }
        u[3].copy(&w);
        if ecp::SIGN_OF_X == ecp::NEGATIVEX {
            let mut t = BIG::new();
            t.copy(&BIG::modneg(&u[1], &q));
            u[1].copy(&t);
            t.copy(&BIG::modneg(&u[3], &q));
            u[3].copy(&t);
        }
    }
    u
}

#[allow(non_snake_case)]
/* Multiply P by e in group G1 */
pub fn g1mul(P: &ECP, e: &BIG) -> ECP {
    let mut R = ECP::new();
    let q = BIG::new_ints(&rom::CURVE_ORDER);
    let mut ee= BIG::new_copy(e);
    ee.rmod(&q);
    if rom::USE_GLV {
        R.copy(P);
        let mut Q = ECP::new();
        Q.copy(P);
        Q.affine();

        let mut cru = FP::new_big(&BIG::new_ints(&rom::CRU));
        let mut u = glv(&ee);
        Q.mulx(&mut cru);

        let mut np = u[0].nbits();
        let mut t: BIG = BIG::modneg(&u[0], &q);
        let mut nn = t.nbits();
        if nn < np {
            u[0].copy(&t);
            R.neg();
        }

        np = u[1].nbits();
        t = BIG::modneg(&u[1], &q);
        nn = t.nbits();
        if nn < np {
            u[1].copy(&t);
            Q.neg();
        }
        u[0].norm();
        u[1].norm();
        R = R.mul2(&u[0], &Q, &u[1]);
    } else {
        R = P.clmul(&ee,&q);
    }
    R
}

#[allow(non_snake_case)]
/* Multiply P by e in group G2 */
pub fn g2mul(P: &ECP2, e: &BIG) -> ECP2 {
    let mut R = ECP2::new();
    let q = BIG::new_ints(&rom::CURVE_ORDER);
    let mut ee= BIG::new_copy(e);
    ee.rmod(&q);
    if rom::USE_GS_G2 {
        let mut Q: [ECP2; 4] = [ECP2::new(), ECP2::new(), ECP2::new(), ECP2::new()];
        let mut f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));
 
        let mut u = gs(&ee);
        let mut T = ECP2::new();

        if ecp::SEXTIC_TWIST == ecp::M_TYPE {
            f.inverse(None);
            f.norm();
        }

        let mut t = BIG::new();
        Q[0].copy(&P);
        for i in 1..4 {
            T.copy(&Q[i - 1]);
            Q[i].copy(&T);
            Q[i].frob(&f);
        }
        for i in 0..4 {
            let np = u[i].nbits();
            t.copy(&BIG::modneg(&u[i], &q));
            let nn = t.nbits();
            if nn < np {
                u[i].copy(&t);
                Q[i].neg();
            }
            u[i].norm();
        }

        R.copy(&ECP2::mul4(&Q, &u));
    } else {
        R.copy(&P.mul(&ee));
    }
    R
}

/* f=f^e */
/* Note that this method requires a lot of RAM! Better to use compressed XTR method, see FP4.java */
pub fn gtpow(d: &FP12, e: &BIG) -> FP12 {
    let mut r = FP12::new();
    let q = BIG::new_ints(&rom::CURVE_ORDER);
    let mut ee= BIG::new_copy(e);
    ee.rmod(&q);
    if rom::USE_GS_GT {
        let mut g: [FP12; 4] = [FP12::new(), FP12::new(), FP12::new(), FP12::new()];
        let f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));

        let mut t = BIG::new();
        let mut u = gs(&ee);
        let mut w = FP12::new();

        g[0].copy(&d);
        for i in 1..4 {
            w.copy(&g[i - 1]);
            g[i].copy(&w);
            g[i].frob(&f);
        }
        for i in 0..4 {
            let np = u[i].nbits();
            t.copy(&BIG::modneg(&u[i], &q));
            let nn = t.nbits();
            if nn < np {
                u[i].copy(&t);
                g[i].conj();
            }
            u[i].norm();
        }
        r.copy(&FP12::pow4(&g, &u));
    } else {
        r.copy(&d.pow(&ee));
    }
    r
}

/* test G1 group membership */
#[allow(non_snake_case)]
pub fn g1member(P: &ECP) -> bool {
    //let q = BIG::new_ints(&rom::CURVE_ORDER);
    if P.is_infinity() {
        return false;
    }
    if ecp::CURVE_PAIRING_TYPE != ecp::BN {
        let x = BIG::new_ints(&rom::CURVE_BNX);
        let mut cru = FP::new_big(&BIG::new_ints(&rom::CRU));
        let mut W=ECP::new(); W.copy(P); W.mulx(&mut cru);
        let mut T=P.mul(&x); 
        if P.equals(&T) {return false;}    // P is of low order   	
        
        T=T.mul(&x); T.neg();
        if !W.equals(&T) {
            return false;
        }

// Not needed
//        W.add(P); T.mulx(&mut cru); W.add(&T);
//        if !W.is_infinity() {
//            return false;
//        }        
/*
        let W=P.mul(&q);
        if !W.is_infinity() {
            return false;
        } */
    }
    true
}

/* test G2 group membership */
#[allow(non_snake_case)]
pub fn g2member(P: &ECP2) -> bool {
    if P.is_infinity() {
        return false;
    }
    let mut f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));    
    if ecp::SEXTIC_TWIST == ecp::M_TYPE {
        f.inverse(None);
        f.norm();
    }
    let x = BIG::new_ints(&rom::CURVE_BNX);

    let mut W=ECP2::new();
    let mut T=P.mul(&x);
    if ecp::SIGN_OF_X == ecp::NEGATIVEX {
        T.neg();
    }
    if ecp::CURVE_PAIRING_TYPE == ecp::BN {
//https://eprint.iacr.org/2022/348.pdf
        W.copy(&T);
        W.frob(&f);
        T.add(P);
        T.add(&W);
        W.frob(&f);
        T.add(&W);
        W.frob(&f);
        W.dbl();
    } else {
//https://eprint.iacr.org/2021/1130
        W.copy(P); W.frob(&f);
    }

    if !W.equals(&T) {
        return false;
    }
    true
}

/* Check that m is in cyclotomic sub-group */
/* Check that m!=1, conj(m)*m==1, and m.m^{p^4}=m^{p^2} */
pub fn gtcyclotomic(m: &FP12) -> bool {
    if m.isunity() {
        return false;
    }
    let mut r = FP12::new_copy(&m);
    r.conj();
    r.mul(&m);
    if !r.isunity() {
        return false;
    }
    let f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));
    r.copy(&m); r.frob(&f); r.frob(&f);
    let mut w = FP12::new_copy(&r); w.frob(&f); w.frob(&f);
    w.mul(&m);
    if !w.equals(&r) {
        return false;
    }
    return true;
}

/* test for full GT group membership */
pub fn gtmember(m: &FP12) -> bool {
    if !gtcyclotomic(m) {
        return false;
    }
    let f = FP2::new_bigs(&BIG::new_ints(&rom::FRA), &BIG::new_ints(&rom::FRB));    
    let x = BIG::new_ints(&rom::CURVE_BNX);

    let mut r=FP12::new(); 
    let mut t=m.pow(&x);
    if ecp::SIGN_OF_X == ecp::NEGATIVEX {
        t.conj();
    }
    if ecp::CURVE_PAIRING_TYPE == ecp::BN {
//https://eprint.iacr.org/2022/348.pdf
        r.copy(&t);
        r.frob(&f);
        t.mul(m);
        t.mul(&r);
        r.frob(&f);
        t.mul(&r);
        r.frob(&f);
        r.usqr();
    } else {
//https://eprint.iacr.org/2021/1130
        r.copy(m); r.frob(&f);
    }

    if !r.equals(&t) {
        return false;
    }

    true
}
