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

use crate::hash256::HASH256;
use crate::hash384::HASH384;
use crate::hash512::HASH512;
use crate::rand::RAND;
use crate::sha3::SHA3;

pub const MC_SHA2: usize = 2;
pub const MC_SHA3: usize = 3;
pub const SHA256: usize = 32;
pub const SHA384: usize = 48;
pub const SHA512: usize = 64;

#[allow(non_snake_case)]
/* General Purpose Hash function */
#[allow(clippy::too_many_arguments)]
pub fn GPhashit(
    hash: usize,
    sha: usize,
    w: &mut [u8],
    pad: usize,
    zpad: usize,
    a: Option<&[u8]>,
    n: isize,
    b: Option<&[u8]>,
) {
    let mut r: [u8; 64] = [0; 64];

    if hash == MC_SHA2 {
        if sha == SHA256 {
            let mut h = HASH256::new();
            for _ in 0..zpad {
                h.process(0);
            }
            if let Some(x) = a {
                h.process_array(x);
            }
            if n >= 0 {
                h.process_num(n as i32)
            }
            if let Some(x) = b {
                h.process_array(x);
            }
            let hs = h.hash();
            for i in 0..sha {
                r[i] = hs[i];
            }
        }
        if sha == SHA384 {
            let mut h = HASH384::new();
            for _ in 0..zpad {
                h.process(0);
            }
            if let Some(x) = a {
                h.process_array(x);
            }
            if n >= 0 {
                h.process_num(n as i32)
            }
            if let Some(x) = b {
                h.process_array(x);
            }
            let hs = h.hash();
            for i in 0..sha {
                r[i] = hs[i];
            }
        }
        if sha == SHA512 {
            let mut h = HASH512::new();
            for _ in 0..zpad {
                h.process(0);
            }
            if let Some(x) = a {
                h.process_array(x);
            }
            if n >= 0 {
                h.process_num(n as i32)
            }
            if let Some(x) = b {
                h.process_array(x);
            }
            let hs = h.hash();
            for i in 0..sha {
                r[i] = hs[i];
            }
        }
    }
    if hash == MC_SHA3 {
        let mut h = SHA3::new(sha);
        for _ in 0..zpad {
            h.process(0);
        }
        if let Some(x) = a {
            h.process_array(x);
        }
        if n >= 0 {
            h.process_num(n as i32)
        }
        if let Some(x) = b {
            h.process_array(x);
        }
        h.hash(&mut r);
    }

    if pad == 0 {
        for i in 0..sha {
            w[i] = r[i]
        }
    } else if pad <= sha {
        for i in 0..pad {
            w[i] = r[i]
        }
    } else {
        for i in 0..sha {
            w[i + pad - sha] = r[i]
        }
        for i in 0..(pad - sha) {
            w[i] = 0
        }
    }
}

#[allow(non_snake_case)]
pub fn SPhashit(hash: usize, sha: usize, w: &mut [u8], a: Option<&[u8]>) {
    GPhashit(hash, sha, w, 0, 0, a, -1, None);
}

pub fn inttobytes(n: usize, b: &mut [u8]) {
    let mut i = b.len();
    let mut m = n;
    while m > 0 && i > 0 {
        i -= 1;
        b[i] = (m & 0xff) as u8;
        m /= 256;
    }
}

pub fn kdf2(hash: usize, sha: usize, z: &[u8], p: Option<&[u8]>, olen: usize, k: &mut [u8]) {
    /* NOTE: the parameter olen is the length of the output K in bytes */
    let hlen = sha;
    let mut lk = 0;

    let mut cthreshold = olen / hlen;
    if olen % hlen != 0 {
        cthreshold += 1
    }

    for counter in 1..cthreshold + 1 {
        let mut b: [u8; 64] = [0; 64];
        GPhashit(hash, sha, &mut b, 0, 0, Some(z), counter as isize, p);
        if lk + hlen > olen {
            for i in 0..(olen % hlen) {
                k[lk] = b[i];
                lk += 1
            }
        } else {
            for i in 0..hlen {
                k[lk] = b[i];
                lk += 1
            }
        }
    }
}

/* Password based Key Derivation Function */
/* Input password p, salt s, and repeat count */
/* Output key of length olen */
pub fn pbkdf2(
    hash: usize,
    sha: usize,
    pass: &[u8],
    salt: &[u8],
    rep: usize,
    olen: usize,
    k: &mut [u8],
) {
    let mut d = olen / sha;
    if olen % sha != 0 {
        d += 1
    }
    let mut f: [u8; 64] = [0; 64];
    let mut u: [u8; 64] = [0; 64];
    let mut ku: [u8; 64] = [0; 64];
    let mut s: [u8; 36] = [0; 36]; // Maximum salt of 32 bytes + 4
    let mut n: [u8; 4] = [0; 4];

    let sl = salt.len();
    let mut kp = 0;
    for i in 0..d {
        for j in 0..sl {
            s[j] = salt[j]
        }
        inttobytes(i + 1, &mut n);
        for j in 0..4 {
            s[sl + j] = n[j]
        }

        hmac1(hash, sha, &mut f, sha, &s[0..sl + 4], pass);

        for j in 0..sha {
            u[j] = f[j]
        }

        for _ in 1..rep {
            hmac1(hash, sha, &mut ku, sha, &u, pass);
            for m in 0..sha {
                u[m] = ku[m];
                f[m] ^= u[m]
            }
        }
        for j in 0..sha {
            if kp < olen {
                k[kp] = f[j]
            }
            kp += 1
        }
    }
}

fn blksize(hash: usize, sha: usize) -> usize {
    let mut lb = 0;
    if hash == MC_SHA2 {
        lb = 64;
        if sha > 32 {
            lb = 128;
        }
    }
    if hash == MC_SHA3 {
        lb = 200 - 2 * sha;
    }
    lb
}

/* Calculate HMAC of m using key k. HMAC is tag of length olen (which is length of tag) */
pub fn hmac1(hash: usize, sha: usize, tag: &mut [u8], olen: usize, k: &[u8], m: &[u8]) -> bool {
    /* Input is from an octet m        *
    	* olen is requested output length in bytes. k is the key  *
    	* The output is the calculated tag */
    let mut b: [u8; 64] = [0; 64]; /* Not good */
    let mut k0: [u8; 128] = [0; 128];

    let lb = blksize(hash, sha);
    if lb == 0 {
        return false;
    }

    for i in 0..lb {
        k0[i] = 0
    }

    if k.len() > lb {
        SPhashit(hash, sha, &mut b, Some(k));
        //GPhashit(hash, sha, &mut b,0,0,k, 0, None);
        for i in 0..sha {
            k0[i] = b[i]
        }
    } else {
        for i in 0..k.len() {
            k0[i] = k[i]
        }
    }

    for i in 0..lb {
        k0[i] ^= 0x36
    }

    GPhashit(hash, sha, &mut b, 0, 0, Some(&k0[0..lb]), -1, Some(m));

    for i in 0..lb {
        k0[i] ^= 0x6a
    }
    GPhashit(
        hash,
        sha,
        tag,
        olen,
        0,
        Some(&k0[0..lb]),
        -1,
        Some(&b[0..sha]),
    );

    true
}

pub fn hkdf_extract(hash: usize, hlen: usize, prk: &mut [u8], salt: Option<&[u8]>, ikm: &[u8]) {
    if let Some(x) = salt {
        hmac1(hash, hlen, prk, hlen, x, ikm);
    } else {
        let h: [u8; 64] = [0; 64];
        hmac1(hash, hlen, prk, hlen, &h[0..hlen], ikm);
    }
}

pub fn hkdf_expand(hash: usize, hlen: usize, okm: &mut [u8], olen: usize, prk: &[u8], info: &[u8]) {
    let n = olen / hlen;
    let flen = olen % hlen;

    let mut t: [u8; 1024] = [0; 1024]; // >= info.length+hlen+1
    let mut k: [u8; 64] = [0; 64];

    let mut l = 0;
    let mut m = 0;
    for i in 1..=n {
        for j in 0..info.len() {
            t[l] = info[j];
            l += 1;
        }
        t[l] = i as u8;
        l += 1;
        hmac1(hash, hlen, &mut k, hlen, prk, &t[0..l]);
        l = 0;
        for j in 0..hlen {
            okm[m] = k[j];
            m += 1;
            t[l] = k[j];
            l += 1;
        }
    }
    if flen > 0 {
        for j in 0..info.len() {
            t[l] = info[j];
            l += 1;
        }
        t[l] = (n + 1) as u8;
        l += 1;
        hmac1(hash, hlen, &mut k, flen, prk, &t[0..l]);
        for j in 0..flen {
            okm[m] = k[j];
            m += 1;
        }
    }
}

fn ceil(a: usize, b: usize) -> usize {
    (a - 1) / b + 1
}

pub fn xof_expand(hlen: usize, okm: &mut [u8], olen: usize, dst: &[u8], msg: &[u8]) {
    let mut h = SHA3::new(hlen);
    for i in 0..msg.len() {
        h.process(msg[i]);
    }
    h.process(((olen >> 8) & 0xff) as u8);
    h.process((olen & 0xff) as u8);

    for i in 0..dst.len() {
        h.process(dst[i]);
    }
    h.process((dst.len() & 0xff) as u8);

    h.shake(okm, olen);
}

pub fn xmd_expand(hash: usize, hlen: usize, okm: &mut [u8], olen: usize, dst: &[u8], msg: &[u8]) {
    let mut w: [u8; 64] = [0; 64];
    if dst.len() >= 256 {
        GPhashit(
            hash,
            hlen,
            &mut w,
            0,
            0,
            Some(b"H2C-OVERSIZE-DST-"),
            -1,
            Some(dst),
        );
        xmd_expand_short_dst(hash, hlen, okm, olen, &w[0..hlen], msg);
    } else {
        xmd_expand_short_dst(hash, hlen, okm, olen, dst, msg);
    }
}

// Assumes dst.len() < 256.
fn xmd_expand_short_dst(
    hash: usize,
    hlen: usize,
    okm: &mut [u8],
    olen: usize,
    dst: &[u8],
    msg: &[u8],
) {
    let mut tmp: [u8; 260] = [0; 260];
    let mut h0: [u8; 64] = [0; 64];
    let mut h1: [u8; 64] = [0; 64];
    let mut h2: [u8; 64] = [0; 64];

    let ell = ceil(olen, hlen);
    let blk = blksize(hash, hlen);
    tmp[0] = ((olen >> 8) & 0xff) as u8;
    tmp[1] = (olen & 0xff) as u8;
    tmp[2] = 0;

    for j in 0..dst.len() {
        tmp[3 + j] = dst[j];
    }
    tmp[3 + dst.len()] = (dst.len() & 0xff) as u8;

    GPhashit(
        hash,
        hlen,
        &mut h0,
        0,
        blk,
        Some(msg),
        -1,
        Some(&tmp[0..dst.len() + 4]),
    );

    let mut k = 0;
    for i in 1..=ell {
        for j in 0..hlen {
            h1[j] ^= h0[j];
            h2[j] = h1[j];
        }
        tmp[0] = i as u8;

        for j in 0..dst.len() {
            tmp[1 + j] = dst[j];
        }
        tmp[1 + dst.len()] = (dst.len() & 0xff) as u8;

        GPhashit(
            hash,
            hlen,
            &mut h1,
            0,
            0,
            Some(&h2[0..hlen]),
            -1,
            Some(&tmp[0..dst.len() + 2]),
        );
        for j in 0..hlen {
            okm[k] = h1[j];
            k += 1;
            if k == olen {
                break;
            }
        }
    }
}

/* Mask Generation Function */

pub fn mgf1(sha: usize, z: &[u8], olen: usize, k: &mut [u8]) {
    let hlen = sha;

    let mut j = 0;
    for i in 0..k.len() {
        k[i] = 0
    }

    let mut cthreshold = olen / hlen;
    if olen % hlen != 0 {
        cthreshold += 1
    }
    for counter in 0..cthreshold {
        let mut b: [u8; 64] = [0; 64];
        GPhashit(MC_SHA2, sha, &mut b, 0, 0, Some(z), counter as isize, None);
        //hashit(sha, Some(z), counter as isize, &mut b);

        if j + hlen > olen {
            for i in 0..(olen % hlen) {
                k[j] = b[i];
                j += 1
            }
        } else {
            for i in 0..hlen {
                k[j] = b[i];
                j += 1
            }
        }
    }
}

pub fn mgf1xor(sha: usize, z: &[u8], olen: usize, k: &mut [u8]) {
    let hlen = sha;
    let mut j = 0;

    let mut cthreshold = olen / hlen;
    if olen % hlen != 0 {
        cthreshold += 1
    }
    for counter in 0..cthreshold {
        let mut b: [u8; 64] = [0; 64];
        GPhashit(MC_SHA2, sha, &mut b, 0, 0, Some(z), counter as isize, None);

        if j + hlen > olen {
            for i in 0..(olen % hlen) {
                k[j] ^= b[i];
                j += 1
            }
        } else {
            for i in 0..hlen {
                k[j] ^= b[i];
                j += 1
            }
        }
    }
}

// PKCS 1.5
/* SHAXXX identifier strings */
const SHA256ID: [u8; 19] = [
    0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
    0x00, 0x04, 0x20,
];
const SHA384ID: [u8; 19] = [
    0x30, 0x41, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, 0x05,
    0x00, 0x04, 0x30,
];
const SHA512ID: [u8; 19] = [
    0x30, 0x51, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05,
    0x00, 0x04, 0x40,
];

pub fn pkcs15(sha: usize, m: &[u8], w: &mut [u8], rfs: usize) -> bool {
    let olen = rfs;
    let hlen = sha;
    let idlen = 19;
    let mut b: [u8; 64] = [0; 64]; /* Not good */

    if olen < idlen + hlen + 10 {
        return false;
    }
    SPhashit(MC_SHA2, sha, &mut b, Some(m));

    for i in 0..w.len() {
        w[i] = 0
    }
    let mut i = 0;
    w[i] = 0;
    i += 1;
    w[i] = 1;
    i += 1;
    for _ in 0..olen - idlen - hlen - 3 {
        w[i] = 0xff;
        i += 1
    }
    w[i] = 0;
    i += 1;
    if hlen == SHA256 {
        for j in 0..idlen {
            w[i] = SHA256ID[j];
            i += 1
        }
    }
    if hlen == SHA384 {
        for j in 0..idlen {
            w[i] = SHA384ID[j];
            i += 1
        }
    }
    if hlen == SHA512 {
        for j in 0..idlen {
            w[i] = SHA512ID[j];
            i += 1
        }
    }
    for j in 0..hlen {
        w[i] = b[j];
        i += 1
    }
    true
}

// Alternate PKCS 1.5
/* SHAXXX identifier strings */
const SHA256IDB: [u8; 17] = [
    0x30, 0x2f, 0x30, 0x0b, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x04,
    0x20,
];
const SHA384IDB: [u8; 17] = [
    0x30, 0x3f, 0x30, 0x0b, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, 0x04,
    0x30,
];
const SHA512IDB: [u8; 17] = [
    0x30, 0x4f, 0x30, 0x0b, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x04,
    0x40,
];

pub fn pkcs15b(sha: usize, m: &[u8], w: &mut [u8], rfs: usize) -> bool {
    let olen = rfs;
    let hlen = sha;
    let idlen = 17;
    let mut b: [u8; 64] = [0; 64]; /* Not good */

    if olen < idlen + hlen + 10 {
        return false;
    }
    SPhashit(MC_SHA2, sha, &mut b, Some(m));
    for i in 0..w.len() {
        w[i] = 0
    }
    let mut i = 0;
    w[i] = 0;
    i += 1;
    w[i] = 1;
    i += 1;
    for _ in 0..olen - idlen - hlen - 3 {
        w[i] = 0xff;
        i += 1
    }
    w[i] = 0;
    i += 1;
    if hlen == SHA256 {
        for j in 0..idlen {
            w[i] = SHA256IDB[j];
            i += 1
        }
    }
    if hlen == SHA384 {
        for j in 0..idlen {
            w[i] = SHA384IDB[j];
            i += 1
        }
    }
    if hlen == SHA512 {
        for j in 0..idlen {
            w[i] = SHA512IDB[j];
            i += 1
        }
    }
    for j in 0..hlen {
        w[i] = b[j];
        i += 1
    }
    true
}

pub fn pss_encode(sha: usize, m: &[u8], rng: &mut RAND, f: &mut [u8], rfs: usize) -> bool {
    let emlen = rfs;
    let embits = 8 * emlen - 1;
    let hlen = sha;
    let mut h: [u8; 64] = [0; 64];
    let mut salt: [u8; 64] = [0; 64];
    let mut md: [u8; 136] = [0; 136];
    for i in 0..hlen {
        salt[i] = rng.getbyte()
    }
    let mask = (0xffu8) >> (8 * emlen - embits);
    SPhashit(MC_SHA2, sha, &mut h, Some(m));
    if emlen < hlen + hlen + 2 {
        return false;
    }
    for i in 0..8 {
        md[i] = 0;
    }
    for i in 0..hlen {
        md[8 + i] = h[i];
    }
    for i in 0..hlen {
        md[8 + hlen + i] = salt[i];
    }
    SPhashit(MC_SHA2, sha, &mut h, Some(&md[0..8 + hlen + hlen]));
    for i in 0..emlen - hlen - hlen - 2 {
        f[i] = 0;
    }
    f[emlen - hlen - hlen - 2] = 0x01;
    for i in 0..hlen {
        f[emlen + i - hlen - hlen - 1] = salt[i];
    }
    mgf1xor(sha, &h[0..hlen], emlen - hlen - 1, f);
    f[0] &= mask;
    for i in 0..hlen {
        f[emlen + i - hlen - 1] = h[i];
    }
    f[emlen - 1] = 0xbcu8;
    true
}

pub fn pss_verify(sha: usize, m: &[u8], f: &[u8]) -> bool {
    let emlen = f.len();
    let embits = 8 * emlen - 1;
    let hlen = sha;
    let mut db: [u8; 512] = [0; 512];
    let mut hmask: [u8; 64] = [0; 64];
    let mut h: [u8; 64] = [0; 64];
    let mut salt: [u8; 64] = [0; 64];
    let mut md: [u8; 136] = [0; 136];
    let mask = (0xffu8) >> (8 * emlen - embits);

    SPhashit(MC_SHA2, sha, &mut hmask, Some(m));
    if emlen < hlen + hlen + 2 {
        return false;
    }
    if f[emlen - 1] != 0xbcu8 {
        return false;
    }
    if (f[0] & (!mask)) != 0 {
        return false;
    }
    for i in 0..emlen - hlen - 1 {
        db[i] = f[i]
    }
    for i in 0..hlen {
        h[i] = f[emlen + i - hlen - 1]
    }
    mgf1xor(sha, &h[0..hlen], emlen - hlen - 1, &mut db);
    db[0] &= mask;

    let mut k = 0u8;
    for i in 0..emlen - hlen - hlen - 2 {
        k |= db[i]
    }
    if k != 0 {
        return false;
    }
    if db[emlen - hlen - hlen - 2] != 0x01 {
        return false;
    }
    for i in 0..hlen {
        salt[i] = db[emlen + i - hlen - hlen - 1]
    }
    for i in 0..8 {
        md[i] = 0
    }
    for i in 0..hlen {
        md[8 + i] = hmask[i]
    }
    for i in 0..hlen {
        md[8 + hlen + i] = salt[i]
    }
    SPhashit(MC_SHA2, sha, &mut hmask, Some(&md[0..8 + hlen + hlen]));
    k = 0;
    for i in 0..hlen {
        k |= h[i] - hmask[i];
    }
    if k != 0 {
        return false;
    }
    true
}

/* OAEP Message Encoding for Encryption */
pub fn oaep_encode(
    sha: usize,
    m: &[u8],
    rng: &mut RAND,
    p: Option<&[u8]>,
    f: &mut [u8],
    rfs: usize,
) -> bool {
    let olen = rfs - 1;
    let mlen = m.len();

    let hlen = sha;

    let mut seed: [u8; 64] = [0; 64];

    let seedlen = hlen;
    if mlen > olen - hlen - seedlen - 1 {
        return false;
    }

    let mut dbmask: [u8; 512] = [0; 512];

    SPhashit(MC_SHA2, sha, f, p);
    //hashit(sha, p, -1, f);
    let slen = olen - mlen - hlen - seedlen - 1;

    for i in 0..slen {
        f[hlen + i] = 0
    }
    f[hlen + slen] = 1;
    for i in 0..mlen {
        f[hlen + slen + 1 + i] = m[i]
    }

    for i in 0..seedlen {
        seed[i] = rng.getbyte()
    }

    mgf1(sha, &seed[0..seedlen], olen - seedlen, &mut dbmask);

    for i in 0..olen - seedlen {
        dbmask[i] ^= f[i]
    }

    mgf1(sha, &dbmask[0..olen - seedlen], seedlen, f);

    for i in 0..seedlen {
        f[i] ^= seed[i]
    }

    for i in 0..olen - seedlen {
        f[i + seedlen] = dbmask[i]
    }

    /* pad to length rfs */
    let d = 1;
    for i in (d..rfs).rev() {
        f[i] = f[i - d];
    }
    for i in (0..d).rev() {
        f[i] = 0;
    }
    true
}

/* OAEP Message Decoding for Decryption */
pub fn oaep_decode(sha: usize, p: Option<&[u8]>, f: &mut [u8], rfs: usize) -> usize {
    let olen = rfs - 1;

    let hlen = sha;
    let mut seed: [u8; 64] = [0; 64];
    let seedlen = hlen;
    let mut chash: [u8; 64] = [0; 64];

    if olen < seedlen + hlen + 1 {
        return 0;
    }
    let mut dbmask: [u8; 512] = [0; 512];

    if f.len() < rfs {
        let d = rfs - f.len();
        for i in (d..rfs).rev() {
            f[i] = f[i - d];
        }
        for i in (0..d).rev() {
            f[i] = 0;
        }
    }
    SPhashit(MC_SHA2, sha, &mut chash, p);
    //hashit(sha, p, -1, &mut chash);

    let x = f[0];

    for i in seedlen..olen {
        dbmask[i - seedlen] = f[i + 1];
    }

    mgf1(sha, &dbmask[0..olen - seedlen], seedlen, &mut seed);
    for i in 0..seedlen {
        seed[i] ^= f[i + 1]
    }
    mgf1(sha, &seed[0..seedlen], olen - seedlen, f);
    for i in 0..olen - seedlen {
        dbmask[i] ^= f[i]
    }

    let mut comp = 0;
    for i in 0..hlen {
        comp |= (chash[i] ^ dbmask[i]) as usize;
    }

    for i in 0..olen - seedlen - hlen {
        dbmask[i] = dbmask[i + hlen]
    }

    for i in 0..hlen {
        seed[i] = 0;
        chash[i] = 0
    }

    // find first non-zero t in array
    let mut k = 0;
    let mut t = 0;
    let m = olen - seedlen - hlen;
    for i in 0..m {
        if t == 0 && dbmask[i] != 0 {
            k = i;
            t = dbmask[i];
        }
    }

    if comp != 0 || x != 0 || t != 0x01 {
        for i in 0..olen - seedlen {
            dbmask[i] = 0
        }
        return 0;
    }

    for i in 0..m - k - 1 {
        f[i] = dbmask[i + k + 1];
    }

    for i in 0..olen - seedlen {
        dbmask[i] = 0
    }

    m - k - 1
}

/*

use core::sha3;
use core::hmac;



    let mut okm: [u8;100]=[0;100];
    let msg: &[u8] = b"abc";
    let dst: &[u8] = b"P256_XMD:SHA-256_SSWU_RO_TESTGEN";
    hmac::xof_expand(sha3::SHAKE128,&mut okm,48,&dst,&msg);
    print!("okm= "); printbinary(&okm[0..48]);
    hmac::xmd_expand(hmac::MC_SHA2,32,&mut okm,48,&dst,&msg);
    print!("okm= "); printbinary(&okm[0..48]);


    let mut ikm: [u8;22]=[0;22];
    let mut salt: [u8;13]=[0;13];
    let mut info: [u8;10]=[0;10];
    let mut prk: [u8;32]=[0;32];
    let mut okm: [u8;42]=[0;42];

    for i in 0..22 {ikm[i]=0x0b;}
    for i in 0..13 {salt[i]=i as u8;}
    for i in 0..10 {info[i]=(0xf0+i) as u8;}

    hmac::hkdf_extract(hmac::MC_SHA2,32,&mut prk,Some(&salt),&ikm);

    print!("PRK= ");
    for i in 0..32 {
        print!("{:02X}",prk[i]);
    }

    hmac::hkdf_expand(hmac::MC_SHA2,32,&mut okm,42,&prk,&info);
    print!("OKM= ");
    for i in 0..42 {
        print!("{:02X}",okm[i]);
    }


*/
