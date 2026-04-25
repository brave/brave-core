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

pub const HASH224: usize = 28;
pub const HASH256: usize = 32;
pub const HASH384: usize = 48;
pub const HASH512: usize = 64;
pub const SHAKE128: usize = 16;
pub const SHAKE256: usize = 32;

const ROUNDS: usize = 24;

const RC: [u64; 24] = [
    0x0000000000000001,
    0x0000000000008082,
    0x800000000000808A,
    0x8000000080008000,
    0x000000000000808B,
    0x0000000080000001,
    0x8000000080008081,
    0x8000000000008009,
    0x000000000000008A,
    0x0000000000000088,
    0x0000000080008009,
    0x000000008000000A,
    0x000000008000808B,
    0x800000000000008B,
    0x8000000000008089,
    0x8000000000008003,
    0x8000000000008002,
    0x8000000000000080,
    0x000000000000800A,
    0x800000008000000A,
    0x8000000080008081,
    0x8000000000008080,
    0x0000000080000001,
    0x8000000080008008,
];

pub struct SHA3 {
    length: usize,
    rate: usize,
    len: usize,
    //s: [[u64; 5]; 5],
    s: [u64; 25],
}

impl SHA3 {
    fn rotl(x: u64, n: u64) -> u64 {
        ((x) << n) | ((x) >> (64 - n))
    }

    fn transform(&mut self) {
        for k in 0..ROUNDS {
            let c0 = self.s[0] ^ self.s[5] ^ self.s[10] ^ self.s[15] ^ self.s[20];
            let c1 = self.s[1] ^ self.s[6] ^ self.s[11] ^ self.s[16] ^ self.s[21];
            let c2 = self.s[2] ^ self.s[7] ^ self.s[12] ^ self.s[17] ^ self.s[22];
            let c3 = self.s[3] ^ self.s[8] ^ self.s[13] ^ self.s[18] ^ self.s[23];
            let c4 = self.s[4] ^ self.s[9] ^ self.s[14] ^ self.s[19] ^ self.s[24];

            let d0 = c4 ^ SHA3::rotl(c1, 1);
            let d1 = c0 ^ SHA3::rotl(c2, 1);
            let d2 = c1 ^ SHA3::rotl(c3, 1);
            let d3 = c2 ^ SHA3::rotl(c4, 1);
            let d4 = c3 ^ SHA3::rotl(c0, 1);

            let b00 = self.s[0] ^ d0;
            let b02 = SHA3::rotl(self.s[1] ^ d1, 1);
            let b04 = SHA3::rotl(self.s[2] ^ d2, 62);
            let b01 = SHA3::rotl(self.s[3] ^ d3, 28);
            let b03 = SHA3::rotl(self.s[4] ^ d4, 27);

            let b13 = SHA3::rotl(self.s[5] ^ d0, 36);
            let b10 = SHA3::rotl(self.s[6] ^ d1, 44);
            let b12 = SHA3::rotl(self.s[7] ^ d2, 6);
            let b14 = SHA3::rotl(self.s[8] ^ d3, 55);
            let b11 = SHA3::rotl(self.s[9] ^ d4, 20);

            let b21 = SHA3::rotl(self.s[10] ^ d0, 3);
            let b23 = SHA3::rotl(self.s[11] ^ d1, 10);
            let b20 = SHA3::rotl(self.s[12] ^ d2, 43);
            let b22 = SHA3::rotl(self.s[13] ^ d3, 25);
            let b24 = SHA3::rotl(self.s[14] ^ d4, 39);

            let b34 = SHA3::rotl(self.s[15] ^ d0, 41);
            let b31 = SHA3::rotl(self.s[16] ^ d1, 45);
            let b33 = SHA3::rotl(self.s[17] ^ d2, 15);
            let b30 = SHA3::rotl(self.s[18] ^ d3, 21);
            let b32 = SHA3::rotl(self.s[19] ^ d4, 8);

            let b42 = SHA3::rotl(self.s[20] ^ d0, 18);
            let b44 = SHA3::rotl(self.s[21] ^ d1, 2);
            let b41 = SHA3::rotl(self.s[22] ^ d2, 61);
            let b43 = SHA3::rotl(self.s[23] ^ d3, 56);
            let b40 = SHA3::rotl(self.s[24] ^ d4, 14);

            self.s[0] = b00 ^ (!b10 & b20);
            self.s[1] = b10 ^ (!b20 & b30);
            self.s[2] = b20 ^ (!b30 & b40);
            self.s[3] = b30 ^ (!b40 & b00);
            self.s[4] = b40 ^ (!b00 & b10);

            self.s[5] = b01 ^ (!b11 & b21);
            self.s[6] = b11 ^ (!b21 & b31);
            self.s[7] = b21 ^ (!b31 & b41);
            self.s[8] = b31 ^ (!b41 & b01);
            self.s[9] = b41 ^ (!b01 & b11);

            self.s[10] = b02 ^ (!b12 & b22);
            self.s[11] = b12 ^ (!b22 & b32);
            self.s[12] = b22 ^ (!b32 & b42);
            self.s[13] = b32 ^ (!b42 & b02);
            self.s[14] = b42 ^ (!b02 & b12);

            self.s[15] = b03 ^ (!b13 & b23);
            self.s[16] = b13 ^ (!b23 & b33);
            self.s[17] = b23 ^ (!b33 & b43);
            self.s[18] = b33 ^ (!b43 & b03);
            self.s[19] = b43 ^ (!b03 & b13);

            self.s[20] = b04 ^ (!b14 & b24);
            self.s[21] = b14 ^ (!b24 & b34);
            self.s[22] = b24 ^ (!b34 & b44);
            self.s[23] = b34 ^ (!b44 & b04);
            self.s[24] = b44 ^ (!b04 & b14);

            self.s[0] ^= RC[k];
        }
    }

    /* Initialise Hash function */
    pub fn init(&mut self, olen: usize) {
        /* initialise */
        for i in 0..25 {
            self.s[i] = 0;
        }
        self.length = 0;
        self.len = olen;
        self.rate = 200 - 2 * olen;
    }

    pub fn new(olen: usize) -> SHA3 {
        let mut nh = SHA3 {
            length: 0,
            rate: 0,
            len: 0,
            s: [0; 25],
        };
        nh.init(olen);
        nh
    }

    pub fn new_copy(hh: &SHA3) -> SHA3 {
        let mut nh = SHA3 {
            length: 0,
            rate: 0,
            len: 0,
            s: [0; 25],
        };
        nh.length = hh.length;
        nh.len = hh.len;
        nh.rate = hh.rate;
        for i in 0..25 {
            nh.s[i] = hh.s[i];
        }
        nh
    }

    /* process a single byte */
    pub fn process(&mut self, byt: u8) {
        /* process the next message byte */
        let cnt = self.length;
        let b = cnt % 8;
        let ind = cnt / 8;
        self.s[ind] ^= (byt as u64) << (8 * b);
        self.length += 1;
        if self.length == self.rate {
            self.length = 0;
            self.transform();
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

    pub fn squeeze(&mut self, buff: &mut [u8], olen: usize) {
        let mut m = 0;
        let nb = olen / self.rate;

        for _ in 0..nb {
            for i in 0..self.rate / 8 {
                let mut el = self.s[i];
                for _ in 0..8 {
                    buff[m] = (el & 0xff) as u8;
                    m += 1;
                    el >>= 8;
                }
            }
            self.transform();
        }

        let mut i = 0;
        while m < olen {
            let mut el = self.s[i];
            i += 1;
            for _ in 0..8 {
                buff[m] = (el & 0xff) as u8;
                m += 1;
                if m >= olen {
                    break;
                }
                el >>= 8;
            }
        }

        /*
        loop {
            for i in 0..25 {
                let mut el = self.s[i];
                for _ in 0..8 {
                    buff[m] = (el & 0xff) as u8;
                    m += 1;
                    if m >= olen || (m % self.rate) == 0 {
                        done = true;
                        break;
                    }
                    el >>= 8;
                }
                if done {
                    break;
                }
            }
            if m >= olen {
                break;
            }
            done = false;
            self.transform();
        } */
    }

    /* Generate 32-byte Hash */
    pub fn hash(&mut self, digest: &mut [u8]) {
        /* pad message and finish - supply digest */
        let q = self.rate - self.length;
        if q == 1 {
            self.process(0x86);
        } else {
            self.process(0x06);
            while self.length != self.rate - 1 {
                self.process(0x00)
            }
            self.process(0x80);
        }
        let hlen = self.len;
        self.squeeze(digest, hlen);
    }

    pub fn continuing_hash(&mut self, digest: &mut [u8]) {
        let mut sh = SHA3::new_copy(self);
        sh.hash(digest)
    }

    pub fn shake(&mut self, digest: &mut [u8], olen: usize) {
        let q = self.rate - self.length;
        if q == 1 {
            self.process(0x9f);
        } else {
            self.process(0x1f);
            while self.length != self.rate - 1 {
                self.process(0x00)
            }
            self.process(0x80);
        }
        self.squeeze(digest, olen);
    }

    pub fn continuing_shake(&mut self, digest: &mut [u8], olen: usize) {
        let mut sh = SHA3::new_copy(self);
        sh.shake(digest, olen);
    }
}

//916f6061fe879741ca6469b43971dfdb28b1a32dc36cb3254e812be27aad1d18
//afebb2ef542e6579c50cad06d2e578f9f8dd6881d7dc824d26360feebf18a4fa73e3261122948efcfd492e74e82e2189ed0fb440d187f382270cb455f21dd185
//98be04516c04cc73593fef3ed0352ea9f6443942d6950e29a372a681c3deaf4535423709b02843948684e029010badcc0acd8303fc85fdad3eabf4f78cae165635f57afd28810fc2
/*
fn main() {
    let s = String::from("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");
    let mut digest: [u8;100]=[0;100];
    let test = s.into_bytes();

    let mut sh=SHA3::new(HASH256);
    for i in 0..test.len(){
        sh.process(test[i]);
    }
    sh.hash(&mut digest);
    for i in 0..32 {print!("{:02x}",digest[i])}
    println!("");

    sh=SHA3::new(HASH512);
    for i in 0..test.len(){
        sh.process(test[i]);
    }
    sh.hash(&mut digest);
    for i in 0..64 {print!("{:02x}",digest[i])}
    println!("");

    sh=SHA3::new(SHAKE256);
    for i in 0..test.len(){
        sh.process(test[i]);
    }
    sh.shake(&mut digest,72);
    for i in 0..72 {print!("{:02x}",digest[i])}
    println!("");

} */
