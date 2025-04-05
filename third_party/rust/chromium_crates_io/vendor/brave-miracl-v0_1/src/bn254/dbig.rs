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
use crate::bn254::big;
use crate::bn254::big::BIG;

pub struct DBIG {
    pub w: [Chunk; big::DNLEN],
}

#[cfg(feature = "std")]
impl std::fmt::Debug for DBIG {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}    

#[cfg(feature = "std")]
impl std::fmt::Display for DBIG {
    fn fmt(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
        write!(formatter, "{}", self.tostring())
    }
}

impl DBIG {
    pub fn new() -> DBIG {
        DBIG {
            w: [0; big::DNLEN as usize],
        }
    }

    pub fn new_copy(y: &DBIG) -> DBIG {
        let mut s = DBIG::new();
        for i in 0..big::DNLEN {
            s.w[i] = y.w[i]
        }
        s
    }

    pub fn new_scopy(x: &BIG) -> DBIG {
        let mut b = DBIG::new();
        for i in 0..big::NLEN {
            b.w[i] = x.w[i];
        }
        b.w[big::NLEN - 1] = x.get(big::NLEN - 1) & big::BMASK; /* top word normalized */
        b.w[big::NLEN] = x.get(big::NLEN - 1) >> big::BASEBITS;

        for i in big::NLEN + 1..big::DNLEN {
            b.w[i] = 0
        }
        b
    }

    /* split DBIG at position n, return higher half, keep lower half */
    pub fn split(&mut self, n: usize) -> BIG {
        let mut t = BIG::new();
        let m = n % big::BASEBITS;
        let mut carry = self.w[big::DNLEN - 1] << (big::BASEBITS - m);

        for i in (big::NLEN - 1..big::DNLEN - 1).rev() {
            let nw = (self.w[i] >> m) | carry;
            carry = (self.w[i] << (big::BASEBITS - m)) & big::BMASK;
            t.set(i + 1 - big::NLEN, nw);
        }
        self.w[big::NLEN - 1] &= ((1 as Chunk) << m) - 1;
        t
    }

    /* general shift left */
    pub fn shl(&mut self, k: usize) {
        let n = k % big::BASEBITS;
        let m = k / big::BASEBITS;
        self.w[big::DNLEN - 1] =
            (self.w[big::DNLEN - 1 - m] << n) | (self.w[big::DNLEN - m - 2] >> (big::BASEBITS - n));
        for i in (m + 1..big::DNLEN - 1).rev() {
            self.w[i] =
                ((self.w[i - m] << n) & big::BMASK) | (self.w[i - m - 1] >> (big::BASEBITS - n));
        }

        self.w[m] = (self.w[0] << n) & big::BMASK;
        for i in 0..m {
            self.w[i] = 0
        }
    }

    /* general shift right */
    pub fn shr(&mut self, k: usize) {
        let n = k % big::BASEBITS;
        let m = k / big::BASEBITS;
        for i in 0..big::DNLEN - m - 1 {
            self.w[i] =
                (self.w[m + i] >> n) | ((self.w[m + i + 1] << (big::BASEBITS - n)) & big::BMASK);
        }
        self.w[big::DNLEN - m - 1] = self.w[big::DNLEN - 1] >> n;
        for i in big::DNLEN - m..big::DNLEN {
            self.w[i] = 0
        }
    }

    /* Copy from another DBIG */
    pub fn copy(&mut self, x: &DBIG) {
        for i in 0..big::DNLEN {
            self.w[i] = x.w[i];
        }
    }

    pub fn ucopy(&mut self, x: &BIG) {
        for i in 0..big::NLEN {
            self.w[i] = 0;
        }
        for i in big::NLEN..big::DNLEN {
            self.w[i] = x.w[i - big::NLEN];
        }
    }

    pub fn cmove(&mut self, g: &DBIG, d: isize) -> Chunk {
        let b = -d as Chunk;
        let mut w=0 as Chunk;
        let r=self.w[0]^g.w[1];
        let mut ra=r.wrapping_add(r); ra >>= 1;
        for i in 0..big::DNLEN {
            let mut t = b & (self.w[i] ^ g.w[i]);
            t^=r;
            let e=self.w[i]^t; w^=e;
            self.w[i]=e^ra; 
        }
        return w;
    }

    /* self+=x */
    pub fn add(&mut self, x: &DBIG) {
        for i in 0..big::DNLEN {
            self.w[i] += x.w[i];
        }
    }

    /* self-=x */
    pub fn sub(&mut self, x: &DBIG) {
        for i in 0..big::DNLEN {
            self.w[i] -= x.w[i];
        }
    }

    /* self=x-self */
    pub fn rsub(&mut self, x: &DBIG) {
        for i in 0..big::DNLEN {
            self.w[i] = x.w[i] - self.w[i];
        }
    }

    /* Compare a and b, return 0 if a==b, -1 if a<b, +1 if a>b. Inputs must be normalised */
    pub fn comp(a: &DBIG, b: &DBIG) -> isize {
        let mut gt = 0 as Chunk;
        let mut eq = 1 as Chunk;
        for i in (0..big::DNLEN).rev() {
  		    gt |= ((b.w[i]-a.w[i]) >> big::BASEBITS) & eq;
		    eq &= ((b.w[i]^a.w[i])-1) >> big::BASEBITS;
        }
        (gt+gt+eq-1) as isize
    }

    /* convert from byte array to BIG */
    pub fn frombytes(b: &[u8]) -> DBIG {
        let mut m = DBIG::new();
        for i in 0..(b.len()) {
            m.shl(8);
            m.w[0] += b[i] as Chunk;
        }
        m
    }

    /* normalise BIG - force all digits < 2^big::BASEBITS */
    pub fn norm(&mut self) {
        let mut carry  = self.w[0]>>big::BASEBITS;
        self.w[0] &= big::BMASK;
        for i in 1..big::DNLEN - 1 {
            let d = self.w[i] + carry;
            self.w[i] = d & big::BMASK;
            carry = d >> big::BASEBITS;
        }
        self.w[big::DNLEN - 1] += carry
    }

// Set self=self mod m in constant time (if bd is known at compile time)
// bd is Max number of bits in b - Actual number of bits in m
    pub fn ctdmod(&mut self, m: &BIG, bd: usize) -> BIG {
        let mut k=bd;
        self.norm();
        let mut c = DBIG::new_scopy(m);
        let mut dr = DBIG::new();

        c.shl(k);

        loop {
            dr.copy(self);
            dr.sub(&c);
            dr.norm();
            self.cmove(&dr,(1 - ((dr.w[big::DNLEN - 1] >> (arch::CHUNK - 1)) & 1)) as isize);
            if k==0 {break;}
            c.shr(1);
            k -= 1; 
        }
        BIG::new_dcopy(self)
    }

    /* reduces self DBIG mod a BIG, and returns the BIG */
    pub fn dmod(&mut self, m: &BIG) -> BIG {
        let ss=self.nbits() as isize;
        let ms=m.nbits() as isize;
        let mut k=(ss-ms) as usize;
        if ss<ms {k=0;}
        self.ctdmod(m,k)
    }

// self=self/m  in constant time (if bd is known at compile time)
// bd is Max number of bits in b - Actual number of bits in m
    pub fn ctdiv(&mut self, m: &BIG, bd:usize) -> BIG {
        let mut k=bd;
        let mut c = DBIG::new_scopy(m);
        let mut a = BIG::new();
        let mut e = BIG::new_int(1);
        let mut dr = DBIG::new();
        let mut r = BIG::new();
        self.norm();

        c.shl(k);
        e.shl(k);

        loop {
            dr.copy(self);
            dr.sub(&c);
            dr.norm();
            let d = (1 - ((dr.w[big::DNLEN - 1] >> (arch::CHUNK - 1)) & 1)) as isize;
            self.cmove(&dr, d);
            r.copy(&a);
            r.add(&e);
            r.norm();
            a.cmove(&r, d);
            if k==0 {break;}
            k -= 1;
            c.shr(1);
            e.shr(1);
        }
        a
    }

    /* return this/c */
    pub fn div(&mut self, m: &BIG) -> BIG {
        let ss=self.nbits() as isize;
        let ms=m.nbits() as isize;
        let mut k=(ss-ms) as usize;
        if ss<ms {k=0;}
        self.ctdiv(m,k)
    }

    /* return number of bits */
    pub fn nbits(&self) -> usize {
        let mut k = big::DNLEN - 1;
        let mut s = DBIG::new_copy(&self);
        s.norm();
        while (k as isize) >= 0 && s.w[k] == 0 {
            k = k.wrapping_sub(1)
        }
        if (k as isize) < 0 {
            return 0;
        }
        let mut bts = (big::BASEBITS as usize) * k;
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

        for i in (0..len).rev() {
            let mut b = DBIG::new_copy(&self);
            b.shr(i * 4);
            s = s + &format!("{:X}", b.w[0] & 15);
        }
        s
    }
}
