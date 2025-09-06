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

const GCM_NB: usize = 4;
const GCM_ACCEPTING_HEADER: usize = 0;
const GCM_ACCEPTING_CIPHER: usize = 1;
const GCM_NOT_ACCEPTING_MORE: usize = 2;
const GCM_FINISHED: usize = 3;
//const GCM_ENCRYPTING: usize = 0;
//const GCM_DECRYPTING: usize = 1;

use crate::aes;
use crate::aes::AES;

pub struct GCM {
    table: [[u32; 4]; 128],
    statex: [u8; 16],
    y_0: [u8; 16],
    //  counter: usize,
    lena: [u32; 2],
    lenc: [u32; 2],
    status: usize,
    a: AES,
}

impl GCM {
    fn pack(b: [u8; 4]) -> u32 {
        /* pack bytes into a 32-bit Word */
        ((b[0] as u32) << 24) | ((b[1] as u32) << 16) | ((b[2] as u32) << 8) | (b[3] as u32)
    }

    fn unpack(a: u32) -> [u8; 4] {
        /* unpack bytes from a word */
        [
            ((a >> 24) & 0xff) as u8,
            ((a >> 16) & 0xff) as u8,
            ((a >> 8) & 0xff) as u8,
            (a & 0xff) as u8,
        ]
    }

    fn precompute(&mut self, h: &[u8]) {
        let mut b: [u8; 4] = [0; 4];
        let mut j = 0;
        for i in 0..GCM_NB {
            b[0] = h[j];
            b[1] = h[j + 1];
            b[2] = h[j + 2];
            b[3] = h[j + 3];
            self.table[0][i] = GCM::pack(b);
            j += 4;
        }
        for i in 1..128 {
            let mut c: u32 = 0;
            for j in 0..GCM_NB {
                self.table[i][j] = c | (self.table[i - 1][j]) >> 1;
                c = self.table[i - 1][j] << 31;
            }
            if c != 0 {
                self.table[i][0] ^= 0xE1000000
            } /* irreducible polynomial */
        }
    }

    fn gf2mul(&mut self) {
        /* gf2m mul - Z=H*X mod 2^128 */
        let mut p: [u32; 4] = [0; 4];

        for i in 0..4 {
            p[i] = 0
        }
        let mut j: usize = 8;
        let mut m = 0;
        for i in 0..128 {
            j -= 1;
            let mut c = ((self.statex[m] >> j) & 1) as u32;
            c = (!c).wrapping_add(1); // + 1;
            for k in 0..GCM_NB {
                p[k] ^= self.table[i][k] & c
            }
            if j == 0 {
                j = 8;
                m += 1;
                if m == 16 {
                    break;
                }
            }
        }
        j = 0;
        for i in 0..GCM_NB {
            let b = GCM::unpack(p[i]);
            self.statex[j] = b[0];
            self.statex[j + 1] = b[1];
            self.statex[j + 2] = b[2];
            self.statex[j + 3] = b[3];
            j += 4;
        }
    }

    fn wrap(&mut self) {
        /* Finish off GHASH */
        let mut f: [u32; 4] = [0; 4];
        let mut el: [u8; 16] = [0; 16];

        /* convert lengths from bytes to bits */
        f[0] = (self.lena[0] << 3) | (self.lena[1] & 0xE0000000) >> 29;
        f[1] = self.lena[1] << 3;
        f[2] = (self.lenc[0] << 3) | (self.lenc[1] & 0xE0000000) >> 29;
        f[3] = self.lenc[1] << 3;
        let mut j = 0;
        for i in 0..GCM_NB {
            let b = GCM::unpack(f[i]);
            el[j] = b[0];
            el[j + 1] = b[1];
            el[j + 2] = b[2];
            el[j + 3] = b[3];
            j += 4;
        }
        for i in 0..16 {
            self.statex[i] ^= el[i]
        }
        self.gf2mul();
    }

    fn ghash(&mut self, plain: &[u8], len: usize) -> bool {
        if self.status == GCM_ACCEPTING_HEADER {
            self.status = GCM_ACCEPTING_CIPHER
        }
        if self.status != GCM_ACCEPTING_CIPHER {
            return false;
        }

        let mut j = 0;
        while j < len {
            for i in 0..16 {
                if j >= len {
                    break;
                }
                self.statex[i] ^= plain[j];
                j += 1;
                self.lenc[1] += 1;
                if self.lenc[1] == 0 {
                    self.lenc[0] += 1
                }
            }
            self.gf2mul();
        }
        if len % 16 != 0 {
            self.status = GCM_NOT_ACCEPTING_MORE
        }
        true
    }

    /* Initialize GCM mode */
    pub fn init(&mut self, nk: usize, key: &[u8], niv: usize, iv: &[u8]) {
        /* iv size niv is usually 12 bytes (96 bits). AES key size nk can be 16,24 or 32 bytes */
        let mut h: [u8; 16] = [0; 16];

        for i in 0..16 {
            h[i] = 0;
            self.statex[i] = 0
        }

        self.a = AES::new();

        self.a.init(aes::ECB, nk, key, None);
        self.a.ecb_encrypt(&mut h); /* E(K,0) */
        self.precompute(&h);

        self.lena[0] = 0;
        self.lenc[0] = 0;
        self.lena[1] = 0;
        self.lenc[1] = 0;
        if niv == 12 {
            for i in 0..12 {
                self.a.f[i] = iv[i]
            }
            let b = GCM::unpack(1);
            self.a.f[12] = b[0];
            self.a.f[13] = b[1];
            self.a.f[14] = b[2];
            self.a.f[15] = b[3]; /* initialise IV */
            for i in 0..16 {
                self.y_0[i] = self.a.f[i]
            }
        } else {
            self.status = GCM_ACCEPTING_CIPHER;
            self.ghash(iv, niv); /* GHASH(H,0,IV) */
            self.wrap();
            for i in 0..16 {
                self.a.f[i] = self.statex[i];
                self.y_0[i] = self.a.f[i];
                self.statex[i] = 0
            }
            self.lena[0] = 0;
            self.lenc[0] = 0;
            self.lena[1] = 0;
            self.lenc[1] = 0;
        }
        self.status = GCM_ACCEPTING_HEADER;
    }

    pub fn new() -> GCM {
        GCM {
            table: [[0; 4]; 128],
            statex: [0; 16],
            y_0: [0; 16],
            //counter:0,
            lena: [0; 2],
            lenc: [0; 2],
            status: 0,
            a: AES::new(),
        }
    }

    /* Add Header data - included but not encrypted */
    pub fn add_header(&mut self, header: &[u8], len: usize) -> bool {
        /* Add some header. Won't be encrypted, but will be authenticated. len is length of header */
        if self.status != GCM_ACCEPTING_HEADER {
            return false;
        }
        let mut j = 0;
        while j < len {
            for i in 0..16 {
                if j >= len {
                    break;
                }
                self.statex[i] ^= header[j];
                j += 1;
                self.lena[1] += 1;
                if self.lena[1] == 0 {
                    self.lena[0] += 1
                }
            }
            self.gf2mul();
        }
        if len % 16 != 0 {
            self.status = GCM_ACCEPTING_CIPHER
        }
        true
    }

    /* Add Plaintext - included and encrypted */
    /* if plain is None - encrypts cipher in place */
    pub fn add_plain(&mut self, cipher: &mut [u8], plain: Option<&[u8]>, len: usize) -> bool {
        let mut cb: [u8; 16] = [0; 16];
        let mut b: [u8; 4] = [0; 4];

        let mut counter: u32;
        if self.status == GCM_ACCEPTING_HEADER {
            self.status = GCM_ACCEPTING_CIPHER
        }
        if self.status != GCM_ACCEPTING_CIPHER {
            return false;
        }

        let mut j = 0;
        while j < len {
            b[0] = self.a.f[12];
            b[1] = self.a.f[13];
            b[2] = self.a.f[14];
            b[3] = self.a.f[15];
            counter = GCM::pack(b);
            counter += 1;
            b = GCM::unpack(counter);
            self.a.f[12] = b[0];
            self.a.f[13] = b[1];
            self.a.f[14] = b[2];
            self.a.f[15] = b[3]; /* increment counter */
            for i in 0..16 {
                cb[i] = self.a.f[i]
            }
            self.a.ecb_encrypt(&mut cb); /* encrypt it  */

            for i in 0..16 {
                if j >= len {
                    break;
                }

                if let Some(sp) = plain {
                    cipher[j] = sp[j] ^ cb[i];
                } else {
                    cipher[j] ^= cb[i];
                }

                self.statex[i] ^= cipher[j];
                j += 1;
                self.lenc[1] += 1;
                if self.lenc[1] == 0 {
                    self.lenc[0] += 1
                }
            }
            self.gf2mul()
        }
        if len % 16 != 0 {
            self.status = GCM_NOT_ACCEPTING_MORE
        }
        true
    }

    /* Add Ciphertext - decrypts to plaintext */
    /* if cipher is None - decrypts plain in place */
    pub fn add_cipher(&mut self, plain: &mut [u8], cipher: Option<&[u8]>, len: usize) -> bool {
        let mut cb: [u8; 16] = [0; 16];
        let mut b: [u8; 4] = [0; 4];

        let mut counter: u32;

        if self.status == GCM_ACCEPTING_HEADER {
            self.status = GCM_ACCEPTING_CIPHER
        }
        if self.status != GCM_ACCEPTING_CIPHER {
            return false;
        }

        let mut j = 0;
        while j < len {
            b[0] = self.a.f[12];
            b[1] = self.a.f[13];
            b[2] = self.a.f[14];
            b[3] = self.a.f[15];
            counter = GCM::pack(b);
            counter += 1;
            b = GCM::unpack(counter);
            self.a.f[12] = b[0];
            self.a.f[13] = b[1];
            self.a.f[14] = b[2];
            self.a.f[15] = b[3]; /* increment counter */
            for i in 0..16 {
                cb[i] = self.a.f[i]
            }
            self.a.ecb_encrypt(&mut cb); /* encrypt it  */
            for i in 0..16 {
                if j >= len {
                    break;
                }
                let oc: u8;
                if let Some(sc) = cipher {
                    oc = sc[j];
                } else {
                    oc = plain[j];
                }

                plain[j] = oc ^ cb[i];
                self.statex[i] ^= oc;
                j += 1;
                self.lenc[1] += 1;
                if self.lenc[1] == 0 {
                    self.lenc[0] += 1
                }
            }
            self.gf2mul()
        }
        if len % 16 != 0 {
            self.status = GCM_NOT_ACCEPTING_MORE
        }
        true
    }

    /* Finish and extract Tag */
    pub fn finish(&mut self, tag: &mut [u8], extract: bool) {
        /* Finish off GHASH and extract tag (MAC) */
        self.wrap();
        /* extract tag */
        if extract {
            self.a.ecb_encrypt(&mut (self.y_0)); /* E(K,Y0) */
            for i in 0..16 {
                self.y_0[i] ^= self.statex[i]
            }
            for i in 0..16 {
                tag[i] = self.y_0[i];
                self.y_0[i] = 0;
                self.statex[i] = 0
            }
        }
        self.status = GCM_FINISHED;
        self.a.end();
    }

    /// Convert a hex-encoded byte sequence to binary
    pub fn hex2bytes(hex: &[u8], bin: &mut [u8]) {
        let len = hex.len();

        for i in 0..len / 2 {
            let mut v: u8;
            v = hexchar2nibble(hex[2 * i]);
            v <<= 4;
            v += hexchar2nibble(hex[2 * i + 1]);
            bin[i] = v;
        }
    }
}

/// Convert an ASCII hex character to the corresponding integer value
///
/// Helper function for `GCM::hex2bytes`.
/// Returns a u8 in the range 0..=0xf.
fn hexchar2nibble(c: u8) -> u8 {
    match c {
        b'0'..=b'9' => c - b'0',
        b'A'..=b'F' => c - b'A' + 10,
        b'a'..=b'f' => c - b'a' + 10,
        _ => 0,
    }
}

pub fn encrypt(c: &mut [u8], t: &mut [u8], k: &[u8], iv: &[u8], h: &[u8], p: &[u8]) {
    let mut g = GCM::new();
    g.init(k.len(), k, iv.len(), iv);
    g.add_header(h, h.len());
    g.add_plain(c, Some(p), p.len());
    g.finish(t, true)
}

pub fn decrypt(p: &mut [u8], t: &mut [u8], k: &[u8], iv: &[u8], h: &[u8], c: &[u8]) {
    let mut g = GCM::new();
    g.init(k.len(), k, iv.len(), iv);
    g.add_header(h, h.len());
    g.add_cipher(p, Some(c), c.len());
    g.finish(t, true);
}

/*
fn main()
{
    let kt=b"feffe9928665731c6d6a8f9467308308";
    let mt=b"d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39";
    let ht=b"feedfacedeadbeeffeedfacedeadbeefabaddad2";
    let nt=b"9313225df88406e555909c5aff5269aa6a7a9538534f7da1e4c303d2a318a728c3c0c95156809539fcf0e2429a6b525416aedbf5a0de6a57a637b39b";
// Tag should be 619cc5aefffe0bfa462af43c1699d050

    let mut gcm=GCM::new();

    let len=mt.len()/2;
    let lenh=ht.len()/2;
    let lenk=kt.len()/2;
    let leniv=nt.len()/2;

    //let mut t:[u8;16]=[0;16]; // Tag
    let mut k:[u8;16]=[0;16];   // AES Key
    let mut h:[u8;64]=[0;64];   // Header - to be included in Authentication, but not encrypted
    let mut n:[u8;100]=[0;100]; // IV - Initialisation vector
    let mut m:[u8;100]=[0;100]; // Plaintext to be encrypted/authenticated
    let mut c:[u8;100]=[0;100]; // Ciphertext
    let mut p:[u8;100]=[0;100]; // Recovered Plaintext

    GCM::hex2bytes(mt,&mut m);
    GCM::hex2bytes(ht,&mut h);
    GCM::hex2bytes(kt,&mut k);
    GCM::hex2bytes(nt,&mut n);

     println!("Plaintext=");
    for i in 0..len {print!("{:02x}",m[i])}
    println!("");

    gcm.init(lenk,&k,leniv,&n);

    gcm.add_header(&h,lenh);
    gcm.add_plain(&mut c,&m,len);
    let mut t=gcm.finish(true);

     println!("Ciphertext=");
    for i in 0..len {print!("{:02x}",c[i])}
    println!("");

     println!("Tag=");
    for i in 0..16 {print!("{:02x}",t[i])}
    println!("");

    gcm.init(lenk,&k,leniv,&n);

    gcm.add_header(&h,lenh);
    gcm.add_cipher(&mut p,&c,len);
    t=gcm.finish(true);

     println!("Plaintext=");
    for i in 0..len {print!("{:02x}",p[i])}
    println!("");

    println!("Tag=");
    for i in 0..16 {print!("{:02x}",t[i])}
    println!("");

}
*/
