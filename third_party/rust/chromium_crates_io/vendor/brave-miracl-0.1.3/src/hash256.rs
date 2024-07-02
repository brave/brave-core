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

const HASH256_H0: u32 = 0x6A09E667;
const HASH256_H1: u32 = 0xBB67AE85;
const HASH256_H2: u32 = 0x3C6EF372;
const HASH256_H3: u32 = 0xA54FF53A;
const HASH256_H4: u32 = 0x510E527F;
const HASH256_H5: u32 = 0x9B05688C;
const HASH256_H6: u32 = 0x1F83D9AB;
const HASH256_H7: u32 = 0x5BE0CD19;

const HASH256_K: [u32; 64] = [
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];

pub struct HASH256 {
    length: [u32; 2],
    h: [u32; 8],
    w: [u32; 64],
}

impl HASH256 {
    fn s(n: u32, x: u32) -> u32 {
        ((x) >> n) | ((x) << (32 - n))
    }
    fn r(n: u32, x: u32) -> u32 {
        (x) >> n
    }

    fn ch(x: u32, y: u32, z: u32) -> u32 {
        (x & y) ^ (!(x) & z)
    }

    fn maj(x: u32, y: u32, z: u32) -> u32 {
        (x & y) ^ (x & z) ^ (y & z)
    }
    fn sig0(x: u32) -> u32 {
        HASH256::s(2, x) ^ HASH256::s(13, x) ^ HASH256::s(22, x)
    }

    fn sig1(x: u32) -> u32 {
        HASH256::s(6, x) ^ HASH256::s(11, x) ^ HASH256::s(25, x)
    }

    fn theta0(x: u32) -> u32 {
        HASH256::s(7, x) ^ HASH256::s(18, x) ^ HASH256::r(3, x)
    }

    fn theta1(x: u32) -> u32 {
        HASH256::s(17, x) ^ HASH256::s(19, x) ^ HASH256::r(10, x)
    }

    pub fn as_bytes(&self, array: &mut [u8]) {
        let mut ptr = 0;
        for i in 0..2 {
            let mut t = self.length[i];
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
            array[ptr] = t as u8;
            ptr += 1;
        }
        for i in 0..8 {
            let mut t = self.h[i];
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
            array[ptr] = t as u8;
            ptr += 1;
        }
        for i in 0..64 {
            let mut t = self.w[i];
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
            array[ptr] = t as u8;
            ptr += 1;
        }
    }

    pub fn from_bytes(&mut self, array: &[u8]) {
        let mut ptr = 0;
        for i in 0..2 {
            let mut t = array[ptr + 3] as u32;
            t = 256 * t + (array[ptr + 2] as u32);
            t = 256 * t + (array[ptr + 1] as u32);
            t = 256 * t + (array[ptr] as u32);
            self.length[i] = t;
            ptr += 4;
        }
        for i in 0..8 {
            let mut t = array[ptr + 3] as u32;
            t = 256 * t + (array[ptr + 2] as u32);
            t = 256 * t + (array[ptr + 1] as u32);
            t = 256 * t + (array[ptr] as u32);
            self.h[i] = t;
            ptr += 4;
        }
        for i in 0..64 {
            let mut t = array[ptr + 3] as u32;
            t = 256 * t + (array[ptr + 2] as u32);
            t = 256 * t + (array[ptr + 1] as u32);
            t = 256 * t + (array[ptr] as u32);
            self.w[i] = t;
            ptr += 4;
        }
    }

    fn transform(&mut self) {
        /* basic transformation step */
        for j in 16..64 {
            self.w[j] = HASH256::theta1(self.w[j - 2])
                .wrapping_add(self.w[j - 7])
                .wrapping_add(HASH256::theta0(self.w[j - 15]))
                .wrapping_add(self.w[j - 16]);
        }
        let mut a = self.h[0];
        let mut b = self.h[1];
        let mut c = self.h[2];
        let mut d = self.h[3];
        let mut e = self.h[4];
        let mut f = self.h[5];
        let mut g = self.h[6];
        let mut hh = self.h[7];
        for j in 0..64 {
            /* 64 times - mush it up */
            let t1 = hh
                .wrapping_add(HASH256::sig1(e))
                .wrapping_add(HASH256::ch(e, f, g))
                .wrapping_add(HASH256_K[j])
                .wrapping_add(self.w[j]);
            let t2 = HASH256::sig0(a).wrapping_add(HASH256::maj(a, b, c));
            hh = g;
            g = f;
            f = e;
            e = d.wrapping_add(t1);
            d = c;
            c = b;
            b = a;
            a = t1.wrapping_add(t2);
        }
        self.h[0] = self.h[0].wrapping_add(a);
        self.h[1] = self.h[1].wrapping_add(b);
        self.h[2] = self.h[2].wrapping_add(c);
        self.h[3] = self.h[3].wrapping_add(d);
        self.h[4] = self.h[4].wrapping_add(e);
        self.h[5] = self.h[5].wrapping_add(f);
        self.h[6] = self.h[6].wrapping_add(g);
        self.h[7] = self.h[7].wrapping_add(hh);
    }

    /* Initialise Hash function */
    pub fn init(&mut self) {
        /* initialise */
        for i in 0..64 {
            self.w[i] = 0
        }
        self.length[0] = 0;
        self.length[1] = 0;
        self.h[0] = HASH256_H0;
        self.h[1] = HASH256_H1;
        self.h[2] = HASH256_H2;
        self.h[3] = HASH256_H3;
        self.h[4] = HASH256_H4;
        self.h[5] = HASH256_H5;
        self.h[6] = HASH256_H6;
        self.h[7] = HASH256_H7;
    }

    pub fn new() -> HASH256 {
        let mut nh = HASH256 {
            length: [0; 2],
            h: [0; 8],
            w: [0; 64],
        };
        nh.init();
        nh
    }

    pub fn new_copy(hh: &HASH256) -> HASH256 {
        let mut nh = HASH256 {
            length: [0; 2],
            h: [0; 8],
            w: [0; 64],
        };
        nh.length[0] = hh.length[0];
        nh.length[1] = hh.length[1];
        for i in 0..64 {
            nh.w[i] = hh.w[i];
        }
        for i in 0..8 {
            nh.h[i] = hh.h[i];
        }
        nh
    }

    /* process a single byte */
    pub fn process(&mut self, byt: u8) {
        /* process the next message byte */
        let cnt = ((self.length[0] / 32) % 16) as usize;
        self.w[cnt] <<= 8;
        self.w[cnt] |= byt as u32;
        self.length[0] += 8;
        if self.length[0] == 0 {
            self.length[1] += 1;
            self.length[0] = 0
        }
        if (self.length[0] % 512) == 0 {
            self.transform()
        }
    }

    /* process an array of bytes */

    pub fn process_array(&mut self, b: &[u8]) {
        for i in 0..b.len() {
            self.process(b[i])
        }
    }

    /* process a 32-bit integer */
    pub fn process_num(&mut self, n: i32) {
        self.process(((n >> 24) & 0xff) as u8);
        self.process(((n >> 16) & 0xff) as u8);
        self.process(((n >> 8) & 0xff) as u8);
        self.process((n & 0xff) as u8);
    }

    /* Generate 32-byte Hash */
    pub fn hash(&mut self) -> [u8; 32] {
        /* pad message and finish - supply digest */
        let mut digest: [u8; 32] = [0; 32];
        let len0 = self.length[0];
        let len1 = self.length[1];
        self.process(0x80);
        while (self.length[0] % 512) != 448 {
            self.process(0)
        }
        self.w[14] = len1;
        self.w[15] = len0;
        self.transform();
        for i in 0..32 {
            /* convert to bytes */
            digest[i] = ((self.h[i / 4] >> (8 * (3 - i % 4))) & 0xff) as u8;
        }
        self.init();
        digest
    }

    pub fn continuing_hash(&self) -> [u8; 32] {
        let mut sh = HASH256::new_copy(self);
        sh.hash()
    }
}

//248d6a61 d20638b8 e5c02693 0c3e6039 a33ce459 64ff2167 f6ecedd4 19db06c1
/*
fn main() {
    let s = String::from("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
    let test = s.into_bytes();
    let mut sh=HASH256::new();

    for i in 0..test.len(){
        sh.process(test[i]);
    }

    let digest=sh.hash();
    for i in 0..32 {print!("{:02x}",digest[i])}
}
*/
