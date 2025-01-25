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

use crate::arch;
use crate::arch::Chunk;

use crate::arch::DChunk;

use crate::rand::RAND;
use crate::bn254::dbig::DBIG;

pub const MODBYTES: usize = 32;
pub const BASEBITS: usize = 56;

pub const NLEN: usize = 1 + ((8 * MODBYTES - 1) / BASEBITS);
pub const DNLEN: usize = 2 * NLEN;
pub const BMASK: Chunk = (1 << BASEBITS) - 1;
pub const HBITS: usize = BASEBITS / 2;
pub const HMASK: Chunk = (1 << HBITS) - 1;
pub const NEXCESS: isize = 1 << ((arch::CHUNK) - BASEBITS - 1);
pub const BIGBITS: usize = MODBYTES * 8;

#[derive(Copy)]
pub struct BIG {
    pub w: [Chunk; NLEN],
}

impl Clone for BIG {
    fn clone(&self) -> BIG {
        *self
    }
}

#[cfg(feature = "std")]
impl std::fmt::Debug for BIG {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}

#[cfg(feature = "std")]
impl std::fmt::Display for BIG {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}

impl BIG {
    pub const fn new() -> BIG {
        BIG { w: [0; NLEN] }
    }

    pub fn new_int(x: isize) -> BIG {
        let mut s = BIG::new();
        s.w[0] = x as Chunk;
        s
    }

    pub fn new_ints(a: &[Chunk]) -> BIG {
        let mut s = BIG::new();
        for i in 0..NLEN {
            s.w[i] = a[i]
        }
        s
    }

    pub fn new_copy(y: &BIG) -> BIG {
        let mut s = BIG::new();
        for i in 0..NLEN {
            s.w[i] = y.w[i]
        }
        s
    }

    pub fn new_big(y: &BIG) -> BIG {
        let mut s = BIG::new();
        for i in 0..NLEN {
            s.w[i] = y.w[i]
        }
        s
    }

    pub fn new_dcopy(y: &DBIG) -> BIG {
        let mut s = BIG::new();
        for i in 0..NLEN {
            s.w[i] = y.w[i]
        }
        s
    }

    pub fn get(&self, i: usize) -> Chunk {
        self.w[i]
    }

    pub fn set(&mut self, i: usize, x: Chunk) {
        self.w[i] = x;
    }

    pub fn xortop(&mut self, x: Chunk) {
        self.w[NLEN - 1] ^= x;
    }

    pub fn ortop(&mut self, x: Chunk) {
        self.w[NLEN - 1] |= x;
    }

    /* test for zero */
    pub fn iszilch(&self) -> bool {
        let mut d = 0 as Chunk;
        for i in 0..NLEN {
            d |= self.w[i];
        }
        (1 & ((d-1)>>BASEBITS)) != 0
    }

    /* set to zero */
    pub fn zero(&mut self) {
        for i in 0..NLEN {
            self.w[i] = 0
        }
    }

    /* Test for equal to one */
    pub fn isunity(&self) -> bool {
        let mut d = 0 as Chunk;
        for i in 1..NLEN {
            d |= self.w[i];
        }
        (1 & ((d-1)>>BASEBITS) & (((self.w[0]^1)-1)>>BASEBITS)) != 0
    }

    /* set to one */
    pub fn one(&mut self) {
        self.w[0] = 1;
        for i in 1..NLEN {
            self.w[i] = 0;
        }
    }

    /* Copy from another BIG */
    pub fn copy(&mut self, x: &BIG) {
        for i in 0..NLEN {
            self.w[i] = x.w[i]
        }
    }

    pub fn dcopy(&mut self, x: &DBIG) {
        for i in 0..NLEN {
            self.w[i] = x.w[i]
        }
    }

    /* Get top and bottom half of =x*y+c+r */
    pub fn muladd(a: Chunk, b: Chunk, c: Chunk, r: Chunk) -> (Chunk, Chunk) {
        let prod: DChunk = (a as DChunk) * (b as DChunk) + (c as DChunk) + (r as DChunk);
        let bot = (prod & (BMASK as DChunk)) as Chunk;
        let top = (prod >> BASEBITS) as Chunk;
        (top, bot)
    }

    /* normalise BIG - force all digits < 2^BASEBITS */
    pub fn norm(&mut self) -> Chunk {
        let mut carry = self.w[0]>>BASEBITS;
        self.w[0] &= BMASK;
        for i in 1..NLEN - 1 {
            let d = self.w[i] + carry;
            self.w[i] = d & BMASK;
            carry = d >> BASEBITS;
        }
        self.w[NLEN - 1] += carry;
        (self.w[NLEN - 1] >> ((8 * MODBYTES) % BASEBITS)) as Chunk
    }

    /* Conditional swap of two bigs depending on d using XOR - no branches */
    pub fn cswap(&mut self, b: &mut BIG, d: isize) -> Chunk {
        let c = -d as Chunk;
        let mut w=0 as Chunk;
        let r=self.w[0]^b.w[1];
        let mut ra=r.wrapping_add(r); ra >>= 1;
        for i in 0..NLEN {
            let mut t = c & (self.w[i] ^ b.w[i]);
            t^=r;
            let mut e=self.w[i]^t; w^=e;
            self.w[i]=e^ra; 
            e=b.w[i]^t;  w^=e;
            b.w[i]=e^ra;
        }
        return w;
    }

    pub fn cmove(&mut self, g: &BIG, d: isize)  -> Chunk {
        let b = -d as Chunk;
        let mut w=0 as Chunk;
        let r=self.w[0]^g.w[1];
        let mut ra=r.wrapping_add(r); ra >>= 1;
        for i in 0..NLEN {
            let mut t = b & (self.w[i] ^ g.w[i]);
            t^=r;
            let e=self.w[i]^t; w^=e;
            self.w[i]=e^ra; 
        }
        return w;
    }

    /* Shift right by less than a word */
    pub fn fshr(&mut self, k: usize) -> isize {
        let n = k;
        let w = self.w[0] & ((1 << n) - 1); /* shifted out part */
        for i in 0..NLEN - 1 {
            self.w[i] = (self.w[i] >> k) | ((self.w[i + 1] << (BASEBITS - n)) & BMASK);
        }
        self.w[NLEN - 1] >>= k;
        w as isize
    }

    /* general shift right */
    pub fn shr(&mut self, k: usize) {
        let n = k % BASEBITS;
        let m = k / BASEBITS;
        for i in 0..NLEN - m - 1 {
            self.w[i] = (self.w[m + i] >> n) | ((self.w[m + i + 1] << (BASEBITS - n)) & BMASK)
        }
        self.w[NLEN - m - 1] = self.w[NLEN - 1] >> n;
        for i in NLEN - m..NLEN {
            self.w[i] = 0
        }
    }

    /* Shift right by less than a word */
    pub fn fshl(&mut self, k: usize) -> isize {
        let n = k;
        self.w[NLEN - 1] = (self.w[NLEN - 1] << n) | (self.w[NLEN - 2] >> (BASEBITS - n));
        for i in (1..NLEN - 1).rev() {
            self.w[i] = ((self.w[i] << k) & BMASK) | (self.w[i - 1] >> (BASEBITS - n));
        }
        self.w[0] = (self.w[0] << n) & BMASK;
        (self.w[NLEN - 1] >> ((8 * MODBYTES) % BASEBITS)) as isize /* return excess - only used in ff.c */
    }

    /* general shift left */
    pub fn shl(&mut self, k: usize) {
        let n = k % BASEBITS;
        let m = k / BASEBITS;

        self.w[NLEN - 1] = self.w[NLEN - 1 - m] << n;
        if NLEN >= m + 2 {
            self.w[NLEN - 1] |= self.w[NLEN - m - 2] >> (BASEBITS - n)
        }
        for i in (m + 1..NLEN - 1).rev() {
            self.w[i] = ((self.w[i - m] << n) & BMASK) | (self.w[i - m - 1] >> (BASEBITS - n));
        }
        self.w[m] = (self.w[0] << n) & BMASK;
        for i in 0..m {
            self.w[i] = 0
        }
    }

    /* return number of bits */
    pub fn nbits(&self) -> usize {
        let mut k = NLEN - 1;
        let mut s = BIG::new_copy(&self);
        s.norm();
        while (k as isize) >= 0 && s.w[k] == 0 {
            k = k.wrapping_sub(1)
        }
        if (k as isize) < 0 {
            return 0;
        }
        let mut bts = BASEBITS * k;
        let mut c = s.w[k];
        while c != 0 {
            c /= 2;
            bts += 1;
        }
        bts
    }

    /* Convert to Hex String */
    #[cfg(feature = "std")]
    pub fn tostring(&self) -> String {
        let mut s = String::new();
        let mut len = self.nbits();

        if len % 4 == 0 {
            len /= 4;
        } else {
            len /= 4;
            len += 1;
        }
        let mb = (MODBYTES * 2) as usize;
        if len < mb {
            len = mb
        }

        for i in (0..len).rev() {
            let mut b = BIG::new_copy(&self);
            b.shr(i * 4);
            s = s + &format!("{:X}", b.w[0] & 15);
        }
        s
    }

    #[cfg(feature = "std")]
    pub fn fromstring(val: String) -> BIG {
        let mut res = BIG::new();
        let len = val.len();
        let op = &val[0..1];
        let n = u8::from_str_radix(op, 16).unwrap();
        res.w[0] += n as Chunk;
        for i in 1..len {
            res.shl(4);
            let op = &val[i..i+1];
            let n = u8::from_str_radix(op, 16).unwrap();
            res.w[0] += n as Chunk;
        }
        res
    }

    pub fn add(&mut self, r: &BIG) {
        for i in 0..NLEN {
            self.w[i] += r.w[i]
        }
    }

    pub fn or(&mut self, r: &BIG) {
        for i in 0..NLEN {
            self.w[i] |= r.w[i]
        }
    }

    pub fn dbl(&mut self) {
        for i in 0..NLEN {
            self.w[i] += self.w[i]
        }
    }

    /* return this+x */
    pub fn plus(&self, x: &BIG) -> BIG {
        let mut s = BIG::new();
        for i in 0..NLEN {
            s.w[i] = self.w[i] + x.w[i];
        }
        s.norm();
        s
    }

    pub fn inc(&mut self, x: isize) {
        self.norm();
        self.w[0] += x as Chunk;
    }

    /* return self-x */
    pub fn minus(&self, x: &BIG) -> BIG {
        let mut d = BIG::new();
        for i in 0..NLEN {
            d.w[i] = self.w[i] - x.w[i];
        }
        d
    }

    /* self-=x */
    pub fn sub(&mut self, x: &BIG) {
        for i in 0..NLEN {
            self.w[i] -= x.w[i];
        }
    }

    /* reverse subtract this=x-this */

    pub fn rsub(&mut self, x: &BIG) {
        for i in 0..NLEN {
            self.w[i] = x.w[i] - self.w[i]
        }
    }

    /* self-=x, where x is int */
    pub fn dec(&mut self, x: isize) {
        self.norm();
        self.w[0] -= x as Chunk;
    }

    /* self*=x, where x is small int<NEXCESS */
    pub fn imul(&mut self, c: isize) {
        for i in 0..NLEN {
            self.w[i] *= c as Chunk;
        }
    }

    /* convert this BIG to byte array */
    pub fn tobytearray(&self, b: &mut [u8], n: usize) {
        let mut c = BIG::new_copy(self);
        c.norm();

        for i in (0..(MODBYTES as usize)).rev() {
            b[i + n] = (c.w[0] & 0xff) as u8;
            c.fshr(8);
        }
    }

    /* convert from byte array to BIG */
    pub fn frombytearray(b: &[u8], n: usize) -> BIG {
        let mut m = BIG::new();
        for i in 0..(MODBYTES as usize) {
            m.fshl(8);
            m.w[0] += b[i + n] as Chunk;
        }
        m
    }

    pub fn tobytes(&self, b: &mut [u8]) {
        self.tobytearray(b, 0)
    }

    pub fn frombytes(b: &[u8]) -> BIG {
       BIG::frombytearray(b, 0)
    }

    /* self*=x, where x is >NEXCESS */
    pub fn pmul(&mut self, c: isize) -> Chunk {
        let mut carry = 0 as Chunk;
        for i in 0..NLEN {
            let ak = self.w[i];
            let tuple = BIG::muladd(ak, c as Chunk, carry, 0 as Chunk);
            carry = tuple.0;
            self.w[i] = tuple.1;
        }
        carry
    }

    /* self*=c and catch overflow in DBIG */
    pub fn pxmul(&mut self, c: isize) -> DBIG {
        let mut m = DBIG::new();
        let mut carry = 0 as Chunk;
        for j in 0..NLEN {
            let tuple = BIG::muladd(self.w[j], c as Chunk, carry, m.w[j]);
            carry = tuple.0;
            m.w[j] = tuple.1;
        }
        m.w[NLEN] = carry;
        m
    }

    /* divide by 3 */
    pub fn div3(&mut self) -> Chunk {
        let mut carry = 0 as Chunk;
        self.norm();
        let base = 1 << BASEBITS;
        for i in (0..NLEN).rev() {
            let ak = carry * base + self.w[i];
            self.w[i] = ak / 3;
            carry = ak % 3;
        }
        carry
    }

    /* return a*b where result fits in a BIG */
    pub fn smul(a: &BIG, b: &BIG) -> BIG {
        let mut c = BIG::new();
        for i in 0..NLEN {
            let mut carry = 0 as Chunk;
            for j in 0..NLEN {
                if i + j < NLEN {
                    let tuple = BIG::muladd(a.w[i], b.w[j], carry, c.w[i + j]);
                    carry = tuple.0;
                    c.w[i + j] = tuple.1;
                }
            }
        }
        c
    }

    /* Compare a and b, return 0 if a==b, -1 if a<b, +1 if a>b. Inputs must be normalised */
    pub fn comp(a: &BIG, b: &BIG) -> isize {
        let mut gt = 0 as Chunk;
        let mut eq = 1 as Chunk;
        for i in (0..NLEN).rev() {
 		    gt |= ((b.w[i]-a.w[i]) >> BASEBITS) & eq;
		    eq &= ((b.w[i]^a.w[i])-1) >> BASEBITS;
        }
        (gt+gt+eq-1) as isize
    }

    /* set x = x mod 2^m */
    pub fn mod2m(&mut self, m: usize) {
        let wd = m / BASEBITS;
        let bt = m % BASEBITS;
        let msk = (1 << bt) - 1;
        self.w[wd] &= msk;
        for i in wd + 1..NLEN {
            self.w[i] = 0
        }
    }

    /* Arazi and Qi inversion mod 256 */
    pub fn invmod256(a: isize) -> isize {
        let mut t1: isize = 0;
        let mut c = (a >> 1) & 1;
        t1 += c;
        t1 &= 1;
        t1 = 2 - t1;
        t1 <<= 1;
        let mut u = t1 + 1;

        // i=2
        let mut b = a & 3;
        t1 = u * b;
        t1 >>= 2;
        c = (a >> 2) & 3;
        let mut t2 = (u * c) & 3;
        t1 += t2;
        t1 *= u;
        t1 &= 3;
        t1 = 4 - t1;
        t1 <<= 2;
        u += t1;

        // i=4
        b = a & 15;
        t1 = u * b;
        t1 >>= 4;
        c = (a >> 4) & 15;
        t2 = (u * c) & 15;
        t1 += t2;
        t1 *= u;
        t1 &= 15;
        t1 = 16 - t1;
        t1 <<= 4;
        u += t1;

        u
    }

    /* return parity */
    pub fn parity(&self) -> isize {
        (self.w[0] % 2) as isize
    }

    /* return n-th bit */
    pub fn bit(&self, n: usize) -> isize {
        return ((self.w[n / (BASEBITS as usize)] & (1 << (n % BASEBITS))) >> (n%BASEBITS)) as isize;


 //       if (self.w[n / (BASEBITS as usize)] & (1 << (n % BASEBITS))) > 0 {
//            1
//        } else {
//            0
//        }
    }

    /* return n last bits */
    pub fn lastbits(&mut self, n: usize) -> isize {
        let msk = ((1 << n) - 1) as Chunk;
        self.norm();
        (self.w[0] & msk) as isize
    }

    /* a=1/a mod 2^256. This is very fast! */
    pub fn invmod2m(&mut self) {
        let mut u = BIG::new();
        let mut b = BIG::new();
        let mut c = BIG::new();

        u.inc(BIG::invmod256(self.lastbits(8)));

        let mut i = 8;
        while i < BIGBITS {
            u.norm();
            b.copy(self);
            b.mod2m(i);
            let mut t1 = BIG::smul(&u, &b);
            t1.shr(i);
            c.copy(self);
            c.shr(i);
            c.mod2m(i);

            let mut t2 = BIG::smul(&u, &c);
            t2.mod2m(i);
            t1.add(&t2);
            t1.norm();
            b = BIG::smul(&t1, &u);
            t1.copy(&b);
            t1.mod2m(i);

            t2.one();
            t2.shl(i);
            t1.rsub(&t2);
            t1.norm();
            t1.shl(i);
            u.add(&t1);
            i <<= 1;
        }
        u.mod2m(BIGBITS);
        self.copy(&u);
        self.norm();
    }

// Set self=self mod m in constant time (if bd is known at compile time)
// bd is Max number of bits in b - Actual number of bits in m
    pub fn ctmod(&mut self,m:&BIG,bd:usize) {
        let mut k=bd;
        let mut r=BIG::new();
        let mut c=BIG::new_copy(m);
        self.norm();

        c.shl(k);
        loop {
            r.copy(self);
            r.sub(&c);
            r.norm();
            self.cmove(&r,(1 - ((r.w[NLEN - 1] >> (arch::CHUNK - 1)) & 1)) as isize);
            if k==0 {break;}
            c.fshr(1);
            k -= 1;  
        }
    }

    /* reduce self mod m */
    pub fn rmod(&mut self, m: &BIG) {
        let ss=self.nbits() as isize;
        let ms=m.nbits() as isize;
        let mut k=(ss-ms) as usize;
        if ss<ms {k=0;}
        self.ctmod(m,k);
    }

    pub fn ctdiv(&mut self, m:&BIG, bd:usize) {
        let mut k=bd; 
        self.norm();
        let mut e = BIG::new_int(1);
        let mut a = BIG::new_copy(self);
        let mut c = BIG::new_copy(m);
        let mut r = BIG::new();
        self.zero(); 
        
        c.shl(k);
        e.shl(k);

        loop {
            r.copy(&a);
            r.sub(&c);
            r.norm();
            let d = (1 - ((r.w[NLEN - 1] >> (arch::CHUNK - 1)) & 1)) as isize;
            a.cmove(&r, d);
            r.copy(self);
            r.add(&e);
            r.norm();
            self.cmove(&r, d);
            if k==0 {break;}
            k -= 1;
            c.fshr(1);
            e.fshr(1);
        }    
    }

    /* divide self by m */
    pub fn div(&mut self, m: &BIG) {
        let ss=self.nbits() as isize;
        let ms=m.nbits() as isize;
        let mut k=(ss-ms) as usize;
        if ss<ms {k=0;}
        self.ctdiv(m,k);
    }

    /* get 8*MODBYTES size random number */
    pub fn random(rng: &mut RAND) -> BIG {
        let mut m = BIG::new();
        let mut j = 0;
        let mut r: u8 = 0;
        /* generate random BIG */

        for _ in 0..8 * (MODBYTES as usize) {
            if j == 0 {
                r = rng.getbyte()
            } else {
                r >>= 1
            }

            let b = (r as Chunk) & 1;
            m.shl(1);
            m.w[0] += b;
            j += 1;
            j &= 7;
        }
        m
    }

    /* Create random BIG in portable way, one bit at a time */
    pub fn randomnum(q: &BIG, rng: &mut RAND) -> BIG {
        let mut d = DBIG::new();
        let mut j = 0;
        let mut r: u8 = 0;
        let t = BIG::new_copy(q);
        for _ in 0..2 * t.nbits() {
            if j == 0 {
                r = rng.getbyte();
            } else {
                r >>= 1
            }

            let b = (r as Chunk) & 1;
            d.shl(1);
            d.w[0] += b;
            j += 1;
            j &= 7;
        }
        d.dmod(q)
    }

/* create randum BIG less than r and less than trunc bits */
    pub fn randtrunc(q: &BIG, trunc: usize, rng: &mut RAND) -> BIG {
        let mut m=BIG::randomnum(q,rng);
	    if q.nbits() > trunc {
	        m.mod2m(trunc);
	    }
	    m
    }

    /* Jacobi Symbol (this/p). Returns 0, 1 or -1 */
    pub fn jacobi(&mut self, p: &BIG) -> isize {
        let mut m: usize = 0;
        let mut t = BIG::new();
        let mut x = BIG::new();
        let mut n = BIG::new();
        let zilch = BIG::new();
        let one = BIG::new_int(1);
        if p.parity() == 0 || BIG::comp(self, &zilch) == 0 || BIG::comp(p, &one) <= 0 {
            return 0;
        }
        self.norm();

        x.copy(self);
        n.copy(p);
        x.rmod(p);

        while BIG::comp(&n, &one) > 0 {
            if BIG::comp(&x, &zilch) == 0 {
                return 0;
            }
            let n8 = n.lastbits(3) as usize;
            let mut k = 0;
            while x.parity() == 0 {
                k += 1;
                x.shr(1);
            }
            if k % 2 == 1 {
                m += (n8 * n8 - 1) / 8
            }
            m += (n8 - 1) * ((x.lastbits(2) as usize) - 1) / 4;
            t.copy(&n);
            t.rmod(&x);
            n.copy(&x);
            x.copy(&t);
            m %= 2;
        }
        if m == 0 {
            1
        } else {
            -1
        }
    }

// Set self=1/self mod p. Binary method 
// NOTE: This function is NOT side-channel safe
// If a is a secret then ALWAYS calculate 1/a = m*(1/am) mod p 
// where m is a random masking value
    pub fn invmodp(&mut self, p: &BIG) {
        self.rmod(p);
	    if self.iszilch() {return;}
        let mut u = BIG::new_copy(self);
        let mut v = BIG::new_copy(p);
        let mut x1 = BIG::new_int(1);
        let mut x2 = BIG::new();
        let mut t = BIG::new();
        let one = BIG::new_int(1);

        while (BIG::comp(&u, &one) != 0) && (BIG::comp(&v, &one) != 0) {
            while u.parity() == 0 {
                u.fshr(1);
                t.copy(&x1);
                t.add(p);
                x1.cmove(&t,x1.parity());
                x1.norm();
                x1.fshr(1);
            }
            while v.parity() == 0 {
                v.fshr(1);
                t.copy(&x2);
                t.add(p);
                x2.cmove(&t,x2.parity());
                x2.norm();
                x2.fshr(1);
            }
            if BIG::comp(&u, &v) >= 0 {
                u.sub(&v);
                u.norm();
                t.copy(&x1);
                t.add(p);
                x1.cmove(&t,(BIG::comp(&x1,&x2)>>1)&1);
                x1.sub(&x2);
                x1.norm();
            } else {
                v.sub(&u);
                v.norm();
                t.copy(&x2);
                t.add(p);
                x2.cmove(&t,(BIG::comp(&x2,&x1)>>1)&1);
                x2.sub(&x1);
                x2.norm();
            }
        }
        self.copy(&x1);
        self.cmove(&x2,BIG::comp(&u,&one)&1);
    }

    /* return a*b as DBIG - Simple Karatsuba */
    pub fn mul(a: &BIG, b: &BIG) -> DBIG {
        let mut c = DBIG::new();
        let rm = BMASK as DChunk;
        let rb = BASEBITS;

        let mut d: [DChunk; DNLEN] = [0; DNLEN];
        for i in 0..NLEN {
            d[i] = (a.w[i] as DChunk) * (b.w[i] as DChunk);
        }
        let mut s = d[0];
        let mut t = s;
        c.w[0] = (t & rm) as Chunk;
        t >>= rb;
        for k in 1..NLEN {
            s += d[k];
            t += s;
            for i in 1 + k / 2..k + 1 {
                t += ((a.w[i] - a.w[k - i]) as DChunk) * ((b.w[k - i] - b.w[i]) as DChunk)
            }
            c.w[k] = (t & rm) as Chunk;
            t >>= rb;
        }
        for k in NLEN..2 * NLEN - 1 {
            s -= d[k - NLEN];
            t += s;
            let mut i = 1 + k / 2;
            while i < NLEN {
                t += ((a.w[i] - a.w[k - i]) as DChunk) * ((b.w[k - i] - b.w[i]) as DChunk);
                i += 1;
            }

            c.w[k] = (t & rm) as Chunk;
            t >>= rb;
        }
        c.w[2 * NLEN - 1] = t as Chunk;
        c
    }

    /* return a^2 as DBIG */
    pub fn sqr(a: &BIG) -> DBIG {
        let mut c = DBIG::new();
        let rm = BMASK as DChunk;
        let rb = BASEBITS;

        let mut t = (a.w[0] as DChunk) * (a.w[0] as DChunk);
        c.w[0] = (t & rm) as Chunk;
        let mut co = t >> rb;

        let mut j = 1;
        while j < NLEN - 1 {
            t = (a.w[j] as DChunk) * (a.w[0] as DChunk);
            for i in 1..(j + 1) / 2 {
                t += (a.w[j - i] as DChunk) * (a.w[i] as DChunk);
            }
            t += t;
            t += co;
            c.w[j] = (t & rm) as Chunk;
            co = t >> rb;
            j += 1;
            t = (a.w[j] as DChunk) * (a.w[0] as DChunk);
            for i in 1..(j + 1) / 2 {
                t += (a.w[j - i] as DChunk) * (a.w[i] as DChunk);
            }
            t += t;
            t += co;
            t += (a.w[j / 2] as DChunk) * (a.w[j / 2] as DChunk);
            c.w[j] = (t & rm) as Chunk;
            co = t >> rb;
            j += 1;
        }

        j = NLEN + (NLEN % 2) - 1;
        while j < DNLEN - 3 {
            t = (a.w[NLEN - 1] as DChunk) * (a.w[j + 1 - NLEN] as DChunk);
            for i in j + 2 - NLEN..(j + 1) / 2 {
                t += (a.w[j - i] as DChunk) * (a.w[i] as DChunk);
            }
            t += t;
            t += co;
            c.w[j] = (t & rm) as Chunk;
            co = t >> rb;
            j += 1;
            t = (a.w[NLEN - 1] as DChunk) * (a.w[j + 1 - NLEN] as DChunk);
            for i in j + 2 - NLEN..(j + 1) / 2 {
                t += (a.w[j - i] as DChunk) * (a.w[i] as DChunk);
            }
            t += t;
            t += co;
            t += (a.w[j / 2] as DChunk) * (a.w[j / 2] as DChunk);
            c.w[j] = (t & rm) as Chunk;
            co = t >> rb;
            j += 1;
        }

        t = (a.w[NLEN - 2] as DChunk) * (a.w[NLEN - 1] as DChunk);
        t += t;
        t += co;
        c.w[DNLEN - 3] = (t & rm) as Chunk;
        co = t >> rb;

        t = (a.w[NLEN - 1] as DChunk) * (a.w[NLEN - 1] as DChunk) + co;
        c.w[DNLEN - 2] = (t & rm) as Chunk;
        co = t >> rb;
        c.w[DNLEN - 1] = co as Chunk;

        c
    }

    /* Montgomery reduction */
    pub fn monty(md: &BIG, mc: Chunk, d: &mut DBIG) -> BIG {
        let mut b = BIG::new();
        let rm = BMASK as DChunk;
        let rb = BASEBITS;

        let mut dd: [DChunk; NLEN] = [0; NLEN];
        let mut v: [Chunk; NLEN] = [0; NLEN];

        b.zero();

        let mut t = d.w[0] as DChunk;
        v[0] = (((t & rm) as Chunk).wrapping_mul(mc)) & BMASK;
        t += (v[0] as DChunk) * (md.w[0] as DChunk);
        t = (d.w[1] as DChunk) + (t >> rb);
        let mut s: DChunk = 0;
        for k in 1..NLEN {
            t = t + s + (v[0] as DChunk) * (md.w[k] as DChunk);
            let mut i = 1 + k / 2;
            while i < k {
                t += ((v[k - i] - v[i]) as DChunk) * ((md.w[i] - md.w[k - i]) as DChunk);
                i += 1;
            }
            v[k] = (((t & rm) as Chunk).wrapping_mul(mc)) & BMASK;
            t += (v[k] as DChunk) * (md.w[0] as DChunk);
            t = (d.w[k + 1] as DChunk) + (t >> rb);
            dd[k] = (v[k] as DChunk) * (md.w[k] as DChunk);
            s += dd[k];
        }

        for k in NLEN..2 * NLEN - 1 {
            t += s;
            let mut i = 1 + k / 2;
            while i < NLEN {
                t += ((v[k - i] - v[i]) as DChunk) * ((md.w[i] - md.w[k - i]) as DChunk);
                i += 1;
            }
            b.w[k - NLEN] = (t & rm) as Chunk;
            t = (d.w[k + 1] as DChunk) + (t >> rb);
            s -= dd[k + 1 - NLEN];
        }
        b.w[NLEN - 1] = (t & rm) as Chunk;
        b
    }

    /* Fast combined shift, subtract and norm. Return sign of result */
    pub fn ssn(r: &mut BIG, a: &BIG, m: &mut BIG) -> isize {
        let n = NLEN - 1;
        m.w[0] = (m.w[0] >> 1) | ((m.w[1] << (BASEBITS - 1)) & BMASK);
        r.w[0] = a.w[0] - m.w[0];
        let mut carry = r.w[0] >> BASEBITS;
        r.w[0] &= BMASK;
        for i in 1..n {
            m.w[i] = (m.w[i] >> 1) | ((m.w[i + 1] << (BASEBITS - 1)) & BMASK);
            r.w[i] = a.w[i] - m.w[i] + carry;
            carry = r.w[i] >> BASEBITS;
            r.w[i] &= BMASK;
        }
        m.w[n] >>= 1;
        r.w[n] = a.w[n] - m.w[n] + carry;
        ((r.w[n] >> (arch::CHUNK - 1)) & 1) as isize
    }

    /* return a*b mod m */
    pub fn modmul(a1: &BIG, b1: &BIG, m: &BIG) -> BIG {
        let mut a = BIG::new_copy(a1);
        let mut b = BIG::new_copy(b1);
        a.rmod(m);
        b.rmod(m);
        let mut d = BIG::mul(&a, &b);
        d.ctdmod(m,m.nbits())
    }

    /* return a^2 mod m */
    pub fn modsqr(a1: &BIG, m: &BIG) -> BIG {
        let mut a = BIG::new_copy(a1);
        a.rmod(m);
        let mut d = BIG::sqr(&a);
        d.ctdmod(m,m.nbits())
    }

    /* return -a mod m */
    pub fn modneg(a1: &BIG, m: &BIG) -> BIG {
        let mut a = BIG::new_copy(a1);
        a.rmod(m);
	    a.rsub(m);
	    a.norm();
        a
    }

    /* return a+b mod m */
    pub fn modadd(a1: &BIG, b1: &BIG, m: &BIG) -> BIG {
        let mut a = BIG::new_copy(a1);
        let mut b = BIG::new_copy(b1);
        a.rmod(m);
        b.rmod(m);
        a.add(&b); a.norm();
        a.ctmod(m,1);
        a
    }

    /* return this^e mod m */
    pub fn powmod(&mut self, e1: &BIG, m: &BIG) -> BIG {
        self.norm();
        let mut e = BIG::new_copy(e1);
        e.norm();
        let mut a = BIG::new_int(1);
        let mut z = BIG::new_copy(&e);
        let mut s = BIG::new_copy(self);
        loop {
            let bt = z.parity();
            z.fshr(1);
            if bt == 1 {
                a = BIG::modmul(&a, &s, m)
            }
            if z.iszilch() {
                break;
            }
            s = BIG::modsqr(&s, m);
        }
        a
    }
}
