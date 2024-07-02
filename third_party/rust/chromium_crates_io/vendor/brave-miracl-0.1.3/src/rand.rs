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

//extern crate mcore;

use crate::hash256::HASH256;

pub const RAND_NK: usize = 21;
const RAND_NJ: usize = 6;
const RAND_NV: usize = 8;

// Marsaglia-Zaman random generator (https://projecteuclid.org/euclid.aoap/1177005878)
// Analysis: https://ieeexplore.ieee.org/document/669305
#[allow(non_camel_case_types)]
pub struct RAND {
    pub ira: [u32; RAND_NK], /* random number...   */
    pub rndptr: usize,
    pub borrow: u32,
    pub pool_ptr: usize,
    pub pool: [u8; 32],
}

impl RAND {
    pub fn new() -> RAND {
        RAND {
            ira: [0; RAND_NK],
            rndptr: 0,
            borrow: 0,
            pool_ptr: 0,
            pool: [0; 32],
        }
    }

    #[allow(dead_code)]
    pub fn clean(&mut self) {
        self.pool_ptr = 0;
        self.rndptr = 0;
        for i in 0..32 {
            self.pool[i] = 0
        }
        for i in 0..RAND_NK {
            self.ira[i] = 0
        }
        self.borrow = 0;
    }

    fn sbrand(&mut self) -> u32 {
        /* Marsaglia & Zaman random number generator */
        self.rndptr += 1;
        if self.rndptr < RAND_NK {
            return self.ira[self.rndptr];
        }
        self.rndptr = 0;
        let mut k = RAND_NK - RAND_NJ;
        for i in 0..RAND_NK {
            /* calculate next NK values */
            if k == RAND_NK {
                k = 0
            }
            let t = self.ira[k];
            let pdiff = t.wrapping_sub(self.ira[i]).wrapping_sub(self.borrow);
            if pdiff < t {
                self.borrow = 0
            }
            if pdiff > t {
                self.borrow = 1
            }
            self.ira[i] = pdiff;
            k += 1;
        }
        self.ira[0]
    }

    fn sirand(&mut self, seed: u32) {
        let mut m: u32 = 1;
        let mut sd = seed;
        self.borrow = 0;
        self.rndptr = 0;
        self.ira[0] ^= sd;
        for i in 1..RAND_NK {
            /* fill initialisation vector */
            let inn = (RAND_NV * i) % RAND_NK;
            self.ira[inn] ^= m; /* note XOR */
            let t = m;
            m = sd.wrapping_sub(m);
            sd = t;
        }
        for _ in 0..10000 {
            self.sbrand();
        } /* "warm-up" & stir the generator */
    }

    fn fill_pool(&mut self) {
        let mut sh = HASH256::new();
        for _ in 0..128 {
            sh.process((self.sbrand() & 0xff) as u8)
        }
        let w = sh.hash();
        for i in 0..32 {
            self.pool[i] = w[i]
        }
        self.pool_ptr = 0;
    }

    fn pack(b: [u8; 4]) -> u32 {
        /* pack 4 bytes into a 32-bit Word */
        (((b[3] as u32) & 0xff) << 24)
            | (((b[2] as u32) & 0xff) << 16)
            | (((b[1] as u32) & 0xff) << 8)
            | ((b[0] as u32) & 0xff)
    }

    pub fn seed(&mut self, rawlen: usize, raw: &[u8]) {
        /* initialise from at least 128 byte string of raw random entropy */
        let mut b: [u8; 4] = [0; 4];
        let mut sh = HASH256::new();
        self.pool_ptr = 0;

        for i in 0..RAND_NK {
            self.ira[i] = 0
        }
        if rawlen > 0 {
            for i in 0..rawlen {
                sh.process(raw[i]);
            }
            let digest = sh.hash();

            /* initialise PRNG from distilled randomness */

            for i in 0..8 {
                b[0] = digest[4 * i];
                b[1] = digest[4 * i + 1];
                b[2] = digest[4 * i + 2];
                b[3] = digest[4 * i + 3];
                self.sirand(RAND::pack(b));
            }
        }
        self.fill_pool();
    }

    pub fn getbyte(&mut self) -> u8 {
        let r = self.pool[self.pool_ptr];
        self.pool_ptr += 1;
        if self.pool_ptr >= 32 {
            self.fill_pool()
        }
        r
    }
}

/* test main program

fn main() {
    let mut raw : [u8;100]=[0;100];
    let mut rng=RAND::new();

    rng.clean();
    for i in 0..100 {raw[i]=i as u8}

    rng.seed(100,&raw);

    for _ in 0..1000 {
        print!("{:03} ",rng.getbyte());
    }
} */
