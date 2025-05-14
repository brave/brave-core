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

const HASH512_H0: u64 = 0x6a09e667f3bcc908;
const HASH512_H1: u64 = 0xbb67ae8584caa73b;
const HASH512_H2: u64 = 0x3c6ef372fe94f82b;
const HASH512_H3: u64 = 0xa54ff53a5f1d36f1;
const HASH512_H4: u64 = 0x510e527fade682d1;
const HASH512_H5: u64 = 0x9b05688c2b3e6c1f;
const HASH512_H6: u64 = 0x1f83d9abfb41bd6b;
const HASH512_H7: u64 = 0x5be0cd19137e2179;

const HASH512_K: [u64; 80] = [
    0x428a2f98d728ae22,
    0x7137449123ef65cd,
    0xb5c0fbcfec4d3b2f,
    0xe9b5dba58189dbbc,
    0x3956c25bf348b538,
    0x59f111f1b605d019,
    0x923f82a4af194f9b,
    0xab1c5ed5da6d8118,
    0xd807aa98a3030242,
    0x12835b0145706fbe,
    0x243185be4ee4b28c,
    0x550c7dc3d5ffb4e2,
    0x72be5d74f27b896f,
    0x80deb1fe3b1696b1,
    0x9bdc06a725c71235,
    0xc19bf174cf692694,
    0xe49b69c19ef14ad2,
    0xefbe4786384f25e3,
    0x0fc19dc68b8cd5b5,
    0x240ca1cc77ac9c65,
    0x2de92c6f592b0275,
    0x4a7484aa6ea6e483,
    0x5cb0a9dcbd41fbd4,
    0x76f988da831153b5,
    0x983e5152ee66dfab,
    0xa831c66d2db43210,
    0xb00327c898fb213f,
    0xbf597fc7beef0ee4,
    0xc6e00bf33da88fc2,
    0xd5a79147930aa725,
    0x06ca6351e003826f,
    0x142929670a0e6e70,
    0x27b70a8546d22ffc,
    0x2e1b21385c26c926,
    0x4d2c6dfc5ac42aed,
    0x53380d139d95b3df,
    0x650a73548baf63de,
    0x766a0abb3c77b2a8,
    0x81c2c92e47edaee6,
    0x92722c851482353b,
    0xa2bfe8a14cf10364,
    0xa81a664bbc423001,
    0xc24b8b70d0f89791,
    0xc76c51a30654be30,
    0xd192e819d6ef5218,
    0xd69906245565a910,
    0xf40e35855771202a,
    0x106aa07032bbd1b8,
    0x19a4c116b8d2d0c8,
    0x1e376c085141ab53,
    0x2748774cdf8eeb99,
    0x34b0bcb5e19b48a8,
    0x391c0cb3c5c95a63,
    0x4ed8aa4ae3418acb,
    0x5b9cca4f7763e373,
    0x682e6ff3d6b2b8a3,
    0x748f82ee5defb2fc,
    0x78a5636f43172f60,
    0x84c87814a1f0ab72,
    0x8cc702081a6439ec,
    0x90befffa23631e28,
    0xa4506cebde82bde9,
    0xbef9a3f7b2c67915,
    0xc67178f2e372532b,
    0xca273eceea26619c,
    0xd186b8c721c0c207,
    0xeada7dd6cde0eb1e,
    0xf57d4f7fee6ed178,
    0x06f067aa72176fba,
    0x0a637dc5a2c898a6,
    0x113f9804bef90dae,
    0x1b710b35131c471b,
    0x28db77f523047d84,
    0x32caab7b40c72493,
    0x3c9ebe0a15c9bebc,
    0x431d67c49c100d4c,
    0x4cc5d4becb3e42b6,
    0x597f299cfc657e2a,
    0x5fcb6fab3ad6faec,
    0x6c44198c4a475817,
];

pub struct HASH512 {
    length: [u64; 2],
    h: [u64; 8],
    w: [u64; 80],
}

impl HASH512 {
    fn s(n: u64, x: u64) -> u64 {
        ((x) >> n) | ((x) << (64 - n))
    }
    fn r(n: u64, x: u64) -> u64 {
        (x) >> n
    }

    fn ch(x: u64, y: u64, z: u64) -> u64 {
        (x & y) ^ (!(x) & z)
    }

    fn maj(x: u64, y: u64, z: u64) -> u64 {
        (x & y) ^ (x & z) ^ (y & z)
    }

    fn sig0(x: u64) -> u64 {
        HASH512::s(28, x) ^ HASH512::s(34, x) ^ HASH512::s(39, x)
    }

    fn sig1(x: u64) -> u64 {
        HASH512::s(14, x) ^ HASH512::s(18, x) ^ HASH512::s(41, x)
    }

    fn theta0(x: u64) -> u64 {
        HASH512::s(1, x) ^ HASH512::s(8, x) ^ HASH512::r(7, x)
    }

    fn theta1(x: u64) -> u64 {
        HASH512::s(19, x) ^ HASH512::s(61, x) ^ HASH512::r(6, x)
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
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
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
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
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
        for i in 0..80 {
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
            array[ptr] = (t % 256) as u8;
            t /= 256;
            ptr += 1;
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
            let mut t = array[ptr + 7] as u64;
            t = 256 * t + (array[ptr + 6] as u64);
            t = 256 * t + (array[ptr + 5] as u64);
            t = 256 * t + (array[ptr + 4] as u64);
            t = 256 * t + (array[ptr + 3] as u64);
            t = 256 * t + (array[ptr + 2] as u64);
            t = 256 * t + (array[ptr + 1] as u64);
            t = 256 * t + (array[ptr] as u64);
            self.length[i] = t;
            ptr += 8;
        }
        for i in 0..8 {
            let mut t = array[ptr + 7] as u64;
            t = 256 * t + (array[ptr + 6] as u64);
            t = 256 * t + (array[ptr + 5] as u64);
            t = 256 * t + (array[ptr + 4] as u64);
            t = 256 * t + (array[ptr + 3] as u64);
            t = 256 * t + (array[ptr + 2] as u64);
            t = 256 * t + (array[ptr + 1] as u64);
            t = 256 * t + (array[ptr] as u64);
            self.h[i] = t;
            ptr += 8;
        }
        for i in 0..80 {
            let mut t = array[ptr + 7] as u64;
            t = 256 * t + (array[ptr + 6] as u64);
            t = 256 * t + (array[ptr + 5] as u64);
            t = 256 * t + (array[ptr + 4] as u64);
            t = 256 * t + (array[ptr + 3] as u64);
            t = 256 * t + (array[ptr + 2] as u64);
            t = 256 * t + (array[ptr + 1] as u64);
            t = 256 * t + (array[ptr] as u64);
            self.w[i] = t;
            ptr += 8;
        }
    }

    fn transform(&mut self) {
        /* basic transformation step */
        for j in 16..80 {
            self.w[j] = HASH512::theta1(self.w[j - 2])
                .wrapping_add(self.w[j - 7])
                .wrapping_add(HASH512::theta0(self.w[j - 15]))
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
        for j in 0..80 {
            /* 64 times - mush it up */
            let t1 = hh
                .wrapping_add(HASH512::sig1(e))
                .wrapping_add(HASH512::ch(e, f, g))
                .wrapping_add(HASH512_K[j])
                .wrapping_add(self.w[j]);
            let t2 = HASH512::sig0(a).wrapping_add(HASH512::maj(a, b, c));
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
        self.h[0] = HASH512_H0;
        self.h[1] = HASH512_H1;
        self.h[2] = HASH512_H2;
        self.h[3] = HASH512_H3;
        self.h[4] = HASH512_H4;
        self.h[5] = HASH512_H5;
        self.h[6] = HASH512_H6;
        self.h[7] = HASH512_H7;
    }

    pub fn new() -> HASH512 {
        let mut nh = HASH512 {
            length: [0; 2],
            h: [0; 8],
            w: [0; 80],
        };
        nh.init();
        nh
    }

    pub fn new_copy(hh: &HASH512) -> HASH512 {
        let mut nh = HASH512 {
            length: [0; 2],
            h: [0; 8],
            w: [0; 80],
        };
        nh.length[0] = hh.length[0];
        nh.length[1] = hh.length[1];
        for i in 0..80 {
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
        let cnt = ((self.length[0] / 64) % 16) as usize;
        self.w[cnt] <<= 8;
        self.w[cnt] |= byt as u64;
        self.length[0] += 8;
        if self.length[0] == 0 {
            self.length[1] += 1;
            self.length[0] = 0
        }
        if (self.length[0] % 1024) == 0 {
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
    pub fn hash(&mut self) -> [u8; 64] {
        /* pad message and finish - supply digest */
        let mut digest: [u8; 64] = [0; 64];
        let len0 = self.length[0];
        let len1 = self.length[1];
        self.process(0x80);
        while (self.length[0] % 1024) != 896 {
            self.process(0)
        }
        self.w[14] = len1;
        self.w[15] = len0;
        self.transform();
        for i in 0..64 {
            /* convert to bytes */
            digest[i] = ((self.h[i / 8] >> (8 * (7 - i % 8))) & 0xff) as u8;
        }
        self.init();
        digest
    }
    pub fn continuing_hash(&self) -> [u8; 64] {
        let mut sh = HASH512::new_copy(self);
        sh.hash()
    }
}

//8e959b75dae313da 8cf4f72814fc143f 8f7779c6eb9f7fa1 7299aeadb6889018 501d289e4900f7e4 331b99dec4b5433a c7d329eeb6dd2654 5e96e55b874be909
/*
fn main() {
    let s = String::from("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
    let test = s.into_bytes();
    let mut sh=HASH512::new();

    for i in 0..test.len(){
        sh.process(test[i]);
    }

    let digest=sh.hash();
    for i in 0..64 {print!("{:02x}",digest[i])}
} */
