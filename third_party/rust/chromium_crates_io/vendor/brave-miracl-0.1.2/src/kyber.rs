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

/* Kyber API high-level functions.  Constant time where it matters. Slow (spends nearly all of its time running SHA3) but small.

   M.Scott 06/07/2022
*/

use crate::sha3;
use crate::sha3::SHA3;

const LGN: usize = 8;
const DEGREE: usize = 1<<LGN;
const PRIME: i16 = 0xD01;

const ONE: i16 = 0x549;     // r mod q
const QINV: i32 = -3327;    // -1/q mod 2^16
//const TWO26: i32 = 1<<26;   // 2^26
const TWO25: i32 = 1<<25;   // 2^25
const BARC: i32 = 20159;    // ((TWO26 + PRIME/2)/PRIME)

pub const SECRET_CPA_SIZE_512: usize=2*(DEGREE*3)/2;
pub const PUBLIC_SIZE_512: usize=32+2*(DEGREE*3)/2;
pub const CIPHERTEXT_SIZE_512: usize= (10*2+4)*DEGREE/8;
pub const SECRET_CCA_SIZE_512: usize=SECRET_CPA_SIZE_512+PUBLIC_SIZE_512+64;
pub const SHARED_SECRET_512: usize=32;

pub const SECRET_CPA_SIZE_768: usize=3*(DEGREE*3)/2;
pub const PUBLIC_SIZE_768: usize=32+3*(DEGREE*3)/2;
pub const CIPHERTEXT_SIZE_768: usize= (10*3+4)*DEGREE/8;
pub const SECRET_CCA_SIZE_768: usize=SECRET_CPA_SIZE_768+PUBLIC_SIZE_768+64;
pub const SHARED_SECRET_768: usize=32;

pub const SECRET_CPA_SIZE_1024: usize=4*(DEGREE*3)/2;
pub const PUBLIC_SIZE_1024: usize=32+4*(DEGREE*3)/2;
pub const CIPHERTEXT_SIZE_1024: usize= (11*4+5)*DEGREE/8;
pub const SECRET_CCA_SIZE_1024: usize=SECRET_CPA_SIZE_1024+PUBLIC_SIZE_1024+64;
pub const SHARED_SECRET_1024: usize=32;

pub const MAXK:usize = 4;

// parameters for each security level
// K,eta1,eta2,du,dv,shared secret
const PARAMS_512:  [usize;6] = [2,3,2,10,4,32];
const PARAMS_768:  [usize;6] = [3,2,2,10,4,32];
const PARAMS_1024: [usize;6] = [4,2,2,11,5,32];

/* Start of public domain reference implementation code - translated from https://github.com/pq-crystals/kyber */

const ZETAS: [i16; 128] = [
  -1044,  -758,  -359, -1517,  1493,  1422,   287,   202,
   -171,   622,  1577,   182,   962, -1202, -1474,  1468,
    573, -1325,   264,   383,  -829,  1458, -1602,  -130,
   -681,  1017,   732,   608, -1542,   411,  -205, -1571,
   1223,   652,  -552,  1015, -1293,  1491,  -282, -1544,
    516,    -8,  -320,  -666, -1618, -1162,   126,  1469,
   -853,   -90,  -271,   830,   107, -1421,  -247,  -951,
   -398,   961, -1508,  -725,   448, -1065,   677, -1275,
  -1103,   430,   555,   843, -1251,   871,  1550,   105,
    422,   587,   177,  -235,  -291,  -460,  1574,  1653,
   -246,   778,  1159,  -147,  -777,  1483,  -602,  1119,
  -1590,   644,  -872,   349,   418,   329,  -156,   -75,
    817,  1097,   603,   610,  1322, -1285, -1465,   384,
  -1215,  -136,  1218, -1335,  -874,   220, -1187, -1659,
  -1185, -1530, -1278,   794, -1510,  -854,  -870,   478,
   -108,  -308,   996,   991,   958, -1460,  1522,  1628
];
/*
fn printbinary(array: &[u8]) {
    for i in 0..array.len() {
        print!("{:02X}", array[i])
    }
    println!("")
}
*/
/* Montgomery stuff */

fn montgomery_reduce(a: i32) -> i16 {
    let dp=PRIME as i32;
    let dt=(((a&0xffff)*QINV)&0xffff) as i16;
    let t=((a-((dt as i32)*dp))>>16) as i16;
    return t;
}

fn barrett_reduce(a: i16) -> i16 {
    let da=a as i32;
    let mut t=((BARC*da + TWO25) >> 26) as i16;
    t*=PRIME;
    return a-t;
}

fn fqmul(a: i16, b: i16) -> i16 {
    return montgomery_reduce((a as i32)*(b as i32));
}

fn ntt(r: &mut [i16]) {
    let mut k=1;
    let mut len=128;
    while len>=2 {
        let mut start=0;
        while start<256 {
            let zeta=ZETAS[k]; k+=1;
            let mut j=start;
            while j<start+len {
                let t=fqmul(zeta,r[j+len]);
                r[j+len]=r[j]-t;
                r[j] += t;
                j+=1;
            }
            start = j+len
        }
        len >>= 1;
    }
}

fn invntt(r: &mut [i16]) {
    let f=1441 as i16;
    let mut k=127;
    let mut len=2;
    while len<=128 {
        let mut start=0;
        while start<256 {
            let zeta=ZETAS[k]; k-=1;
            let mut j=start;
            while j<start+len {
                let t=r[j];
                r[j]=barrett_reduce(t+r[j+len]);  // problem here
                r[j+len] -= t;
                r[j+len]=fqmul(zeta,r[j+len]);
                j+=1;
            }
            start=j+len;
        }
        len<<=1;
    }
    for j in 0..256 {
        r[j]=fqmul(r[j],f);
    }
}

fn basemul(r: &mut [i16],a: &[i16],b: &[i16],zeta: i16) {
    r[0]=fqmul(a[1],b[1]);
    r[0]=fqmul(r[0],zeta);
    r[0]+=fqmul(a[0],b[0]);
    r[1]=fqmul(a[0],b[1]);
    r[1]+=fqmul(a[1],b[0]);
}

fn poly_reduce(r: &mut [i16]) {
    for i in 0..DEGREE {
        r[i]=barrett_reduce(r[i]);
    }
}

fn poly_ntt(r: &mut [i16]) {
    ntt(r);
    poly_reduce(r);
}

fn poly_invntt(r: &mut [i16]) {
    invntt(r);
}

// Note r must be distinct from a and b
fn poly_mul(r: &mut [i16],a: &[i16],b: &[i16]) {
    for i in 0..DEGREE/4 {
        let x=4*i; let y=x+2; let z=x+4;
        basemul(&mut r[x..y],&a[x..y],&b[x..y],ZETAS[64+i]);
        basemul(&mut r[y..z],&a[y..z],&b[y..z],-ZETAS[64+i]);
    }
}

fn poly_tomont(r: &mut [i16]) {
    for i in 0..DEGREE {
        r[i]=montgomery_reduce((r[i] as i32)*(ONE as i32));
    }
}

/* End of public domain reference code use */

fn poly_add(p1: &mut [i16],p2: &[i16],p3: &[i16]) {
    for i in 0..DEGREE {
        p1[i] = p2[i]+p3[i];
    }
}

fn poly_acc(p1: &mut [i16],p3: &[i16]) {
    for i in 0..DEGREE {
        p1[i] += p3[i];
    }
}

fn poly_dec(p1: &mut [i16],p3: &[i16]) {
    for i in 0..DEGREE {
        p1[i] -= p3[i];
    }
}

// Generate a[i][j] from rho
fn expandaij(rho: &[u8],aij: &mut [i16],i:usize,j:usize) {
    let mut buff: [u8; 3*DEGREE] = [0; 3*DEGREE];
    let mut sh = SHA3::new(sha3::SHAKE128);
    for m in 0..32 {
        sh.process(rho[m])
    }
    sh.process(j as u8);
    sh.process(i as u8);
    sh.shake(&mut buff, 3*DEGREE);
    let mut m=0;
    let mut n=0;
    let dp = PRIME as u32;
    while n<DEGREE {
        let d1=(buff[m] as u32) + 256*((buff[m+1]&0x0f) as u32);
        let d2=((buff[m+1]/16) as u32) + 16*(buff[m+2] as u32);
        if d1<dp {
            aij[n]=d1 as i16; n+=1;
        }
        if d2<dp && n<DEGREE {
            aij[n]=d2 as i16; n+=1;
        }
        m+=3;
    }
}

fn getbit(b: &[u8],n: usize) -> i16 {
    let wd=n/8;
    let bt=n%8;
    return ((b[wd]>>bt)&1) as i16;
}

fn cbd(bts: &[u8],eta: usize,f: &mut [i16]) {
    for i in 0..DEGREE {
        let mut a=0 as i16;
        let mut b=0 as i16;
        for j in 0..eta {
            a+=getbit(bts,2*i*eta+j);
            b+=getbit(bts,2*i*eta+eta+j);
        }
        f[i] = a-b;
    }
}

// extract ab bits into word from dense byte stream
fn nextword(ab: usize,t: &[u8],ptr: &mut usize,bts: &mut usize) -> i16 {
    let mut r=(t[*ptr]>>(*bts)) as i16;
    let mask=((1<<ab)-1) as i16;
    let mut i=0;
    let mut gotbits=8-(*bts); // bits left in current byte
    while gotbits<ab {
        i+=1;
        let w=t[(*ptr)+i] as i16;
        r |= w<<gotbits;
        gotbits+=8;
    }
    *bts += ab;
    while *bts>=8 {
        *bts -= 8;
        *ptr += 1;
    }
    return r&mask;
}

fn nextbyte16(ab: usize,t: &[i16],ptr: &mut usize,bts: &mut usize) -> u8 {
    let mut left=ab-(*bts);
    let mut i=0;
    let mut w=t[*ptr]; w+=(w>>15)&PRIME;
    let mut r=w>>(*bts);
    while left<8 {
        i+=1;
        w=t[(*ptr)+i]; w+=(w>>15)&PRIME;
        r|=w<<left;
        left += ab;
    }
    *bts += 8;
    while *bts>=ab {
        *bts -= ab;
        *ptr += 1;
    }
    return (r&0xff) as u8;
}

fn encode(t: &[i16],len: usize,l: usize,pack: &mut [u8]) {
    let mut ptr=0;
    let mut bts=0;
    for n in 0..len*(DEGREE*l)/8 {
        pack[n]=nextbyte16(l,t,&mut ptr, &mut bts); 
    }
}

// return 0 if encoding is unchanged
fn chk_encode(t: &[i16],len: usize,l: usize,pack: &[u8]) -> u8 {
    let mut ptr=0;
    let mut bts=0;
    let mut diff=0 as u8;
    for n in 0..len*(DEGREE*l)/8 {
        let m=nextbyte16(l,t,&mut ptr, &mut bts); 
        diff|=m^pack[n];
    }
    return diff;
}

fn decode(pack: &[u8],l: usize,t: &mut [i16],len: usize) {
    let mut ptr=0;
    let mut bts=0;
    for i in 0..len*DEGREE {
        t[i]=nextword(l,pack,&mut ptr,&mut bts);
    }
}

// Bernsteins safe division by 0xD01
fn safediv(xx: i32) -> i32 {
  let mut x=xx;
  let mut q = 0 as i32;

  let mut qpart = (((x as i64)*645083)>>31) as i32;
  x -= qpart*0xD01; q += qpart;

  qpart = ((((x as i64)*645083)>>31) as i32)+1;
  x -= qpart*0xD01; q += qpart+(x>>31);

  return q;
}

fn compress(t: &mut [i16],len:usize,d:usize) {
    let twod=(1<<d) as i32;
    let dp=PRIME as i32;
    for i in 0..len*DEGREE {
        t[i]+=(t[i]>>15)&PRIME;
        t[i]= (safediv(twod*(t[i] as i32)+dp/2)&(twod-1)) as i16;
    }
}
fn decompress(t: &mut [i16],len:usize,d:usize) {
    let twod1=(1<<(d-1)) as i32;
    let dp=PRIME as i32;
    for i in 0..len*DEGREE {
        t[i]=((dp*(t[i] as i32)+twod1)>>d) as i16;
    }
}


fn cpa_keypair(params: &[usize],tau: &[u8],sk: &mut [u8],pk: &mut [u8]) {
    let mut rho:[u8;32]=[0;32];
    let mut sigma:[u8;33]=[0;33];
    let mut buff:[u8;256]=[0;256];

    let mut r:[i16;DEGREE]=[0;DEGREE];
    let mut w:[i16;DEGREE]=[0;DEGREE];
    let mut aij:[i16;DEGREE]=[0;DEGREE];
    let mut s:[i16;MAXK*DEGREE]=[0;MAXK*DEGREE];
    let mut e:[i16;MAXK*DEGREE]=[0;MAXK*DEGREE];
    let mut p:[i16;MAXK*DEGREE]=[0;MAXK*DEGREE]; 

    let mut sh = SHA3::new(sha3::HASH512);

    let ck=params[0];
    let eta1=params[1];
    let public_key_size=32+ck*(DEGREE*3)/2;

    for i in 0..32 {
        sh.process(tau[i]);
    }
    sh.hash(&mut buff);
    for i in 0..32 {
        rho[i]=buff[i];
        sigma[i]=buff[i+32];
    }
    sigma[32]=0;

// create s
    for i in 0..ck {
        sh=SHA3::new(sha3::SHAKE256);
        for j in 0..33 {
            sh.process(sigma[j]);
        }
        sh.shake(&mut buff,64*eta1);
        cbd(&buff,eta1,&mut s[i*DEGREE..]);
        sigma[32] += 1;
    }

// create e
    for i in 0..ck {
        sh=SHA3::new(sha3::SHAKE256);
        for j in 0..33 {
            sh.process(sigma[j]);
        }
        sh.shake(&mut buff,64*eta1);
        cbd(&buff,eta1,&mut e[i*DEGREE..]);
        sigma[32] += 1;
    } 
 
    for k in 0..ck {
        let row=k*DEGREE;
        poly_ntt(&mut s[row..]);
        poly_ntt(&mut e[row..]);
    }

    for i in 0..ck {
        let row=i*DEGREE;
        expandaij(&rho,&mut aij,i,0);
        poly_mul(&mut r,&aij,&s);
        for j in 1..ck {
            expandaij(&rho,&mut aij,i,j);
            poly_mul(&mut w,&s[j*DEGREE..],&aij);
            poly_acc(&mut r,&w);
        }
        poly_reduce(&mut r);
        poly_tomont(&mut r);
        poly_add(&mut p[row..],&r,&e[row..]);
        poly_reduce(&mut p[row..]);
    }

    encode(&s,ck,12,sk);
    encode(&p,ck,12,pk);
    for i in 0..32 {
        pk[public_key_size-32+i]=rho[i];
    }
}

fn cpa_base_encrypt(params: &[usize],coins: &[u8],pk: &[u8],ss: &[u8],u: &mut [i16],v: &mut [i16]) {
    let mut rho:[u8;32]=[0;32];
    let mut sigma:[u8;33]=[0;33];
    let mut buff:[u8;256]=[0;256];

    let mut r:[i16;DEGREE]=[0;DEGREE];
    let mut w:[i16;DEGREE]=[0;DEGREE];
    let mut aij:[i16;DEGREE]=[0;DEGREE];
    let mut q:[i16;MAXK*DEGREE]=[0;MAXK*DEGREE];
    let mut p:[i16;MAXK*DEGREE]=[0;MAXK*DEGREE];

    let ck=params[0];
    let eta1=params[1];
    let eta2=params[2];
    let du=params[3];
    let dv=params[4];
    let public_key_size=32+ck*(DEGREE*3)/2;

    for i in 0..32 {
        sigma[i]=coins[i]; 
    }
    sigma[32]=0;
    for i in 0..32 {
        rho[i]=pk[i+public_key_size-32]; 
    }
// create q
    for i in 0..ck {
        let mut sh=SHA3::new(sha3::SHAKE256);
        for j in 0..33 {
            sh.process(sigma[j]);
        }
        sh.shake(&mut buff,64*eta1);
        cbd(&buff,eta1,&mut q[i*DEGREE..]);
        sigma[32] += 1;        
    }
// create e1
    for i in 0..ck {
        let mut sh=SHA3::new(sha3::SHAKE256);
        for j in 0..33 {
            sh.process(sigma[j]);
        }
        sh.shake(&mut buff,64*eta2);
        cbd(&buff,eta1,&mut u[i*DEGREE..]);
        sigma[32] += 1;        
    }
    for i in 0..ck {
        let row=DEGREE*i;
        poly_ntt(&mut q[row..]);
    }

    for i in 0..ck {
        let row=i*DEGREE;
        expandaij(&rho,&mut aij,0,i);
        poly_mul(&mut r,&aij,&q);
        for j in 1..ck {
            expandaij(&rho,&mut aij,j,i);
            poly_mul(&mut w,&q[j*DEGREE..],&aij);
            poly_acc(&mut r,&w);
        }
        poly_reduce(&mut r);
        poly_invntt(&mut r);
        poly_acc(&mut u[row..],&r);
        poly_reduce(&mut u[row..]);
    }

    decode(&pk,12,&mut p,ck);

    poly_mul(v,&p,&q);
    for i in 1..ck {
        let row=DEGREE*i;
        poly_mul(&mut r,&p[row..],&q[row..]);
        poly_acc(v,&r);
    }
    poly_invntt(v);

    let mut sh = SHA3::new(sha3::SHAKE256);
    for j in 0..33 {
        sh.process(sigma[j]);
    }
    sh.shake(&mut buff,64*eta2);
    cbd(&buff,eta1,&mut w); // e2

    poly_acc(v,&w);

    decode(&ss,1,&mut r,1);
    decompress(&mut r,1,1);
    poly_acc(v,&r);
    poly_reduce(v);
    compress(u,ck,du);
    compress(v,1,dv);
}

fn cpa_encrypt(params: &[usize],coins: &[u8],pk: &[u8],ss: &[u8],ct: &mut [u8]) {
    let mut v:[i16;DEGREE]=[0;DEGREE];
    let mut u:[i16;MAXK*DEGREE]=[0;MAXK*DEGREE];
    let ck=params[0];
    let du=params[3];
    let dv=params[4];
    let ciphertext_size=(du*ck+dv)*DEGREE/8;
    cpa_base_encrypt(params,coins,pk,ss,&mut u,&mut v);  
    encode(&u,ck,du,ct);
    encode(&v,1,dv,&mut ct[ciphertext_size-(dv*DEGREE/8)..]);
}

// Re-encrypt and check that ct is OK (if so return is zero)
fn cpa_check_encrypt(params: &[usize],coins: &[u8],pk: &[u8],ss: &[u8],ct: &[u8]) -> u8 {
    let mut v:[i16;DEGREE]=[0;DEGREE];
    let mut u:[i16;MAXK*DEGREE]=[0;MAXK*DEGREE];
    let ck=params[0];
    let du=params[3];
    let dv=params[4];
    let ciphertext_size=(du*ck+dv)*DEGREE/8;
    cpa_base_encrypt(params,coins,pk,ss,&mut u,&mut v);  
    let d1=chk_encode(&u,ck,du,ct);
    let d2=chk_encode(&v,1,dv,&ct[ciphertext_size-(dv*DEGREE/8)..]);
    if (d1|d2)==0 {
        return 0;
    } else {
        return 0xff;
    }
}

fn cpa_decrypt(params: &[usize],sk: &[u8],ct: &[u8],ss: &mut [u8]) {
    let mut w:[i16;DEGREE]=[0;DEGREE];
    let mut v:[i16;DEGREE]=[0;DEGREE];
    let mut r:[i16;DEGREE]=[0;DEGREE];
    let mut u:[i16;MAXK*DEGREE]=[0;MAXK*DEGREE];
    let mut s:[i16;MAXK*DEGREE]=[0;MAXK*DEGREE];
    
    let ck=params[0];
    let du=params[3];
    let dv=params[4];

    decode(ct,du,&mut u,ck);
    decode(&ct[(du*ck*DEGREE)/8..],dv,&mut v,1);
    decompress(&mut u,ck,du);
    decompress(&mut v,1,dv);
    decode(sk,12,&mut s,ck);

    poly_ntt(&mut u);
    poly_mul(&mut w,&u,&s);
    for i in 1..ck {
        let row=DEGREE*i;
        poly_ntt(&mut u[row..]);
        poly_mul(&mut r,&u[row..],&s[row..]);
        poly_acc(&mut w,&r);
    }
    poly_reduce(&mut w);
    poly_invntt(&mut w);
    poly_dec(&mut v,&w);
    compress(&mut v,1,1);
    encode(&v,1,1,ss);
}

fn cca_keypair(params: &[usize],randbytes64: &[u8],sk: &mut [u8],pk: &mut [u8]) {
    let ck=params[0];
    let secret_cpa_key_size=ck*(DEGREE*3)/2;
    let public_key_size=32+ck*(DEGREE*3)/2;

    cpa_keypair(params,randbytes64,sk,pk);
    for i in 0..public_key_size {
        sk[i+secret_cpa_key_size]=pk[i];
    }
    let mut sh = SHA3::new(sha3::HASH256);
    for i in 0..public_key_size {
        sh.process(pk[i]);
    }
    sh.hash(&mut sk[secret_cpa_key_size+public_key_size..]);
    for i in 0..32 {
        sk[i+secret_cpa_key_size+public_key_size+32]=randbytes64[i+32];
    }
}

fn cca_encrypt(params: &[usize],randbytes32: &[u8],pk: &[u8],ss: &mut [u8],ct: &mut [u8]) {
    let mut hm:[u8;32]=[0;32];
    let mut h:[u8;32]=[0;32];
    let mut g:[u8;64]=[0;64];
    let ck=params[0];
    let du=params[3];
    let dv=params[4];
    let public_key_size=32+ck*(DEGREE*3)/2;
    let ciphertext_size=(du*ck+dv)*DEGREE/8;
    let shared_secret_size=params[5];

    let mut sh = SHA3::new(sha3::HASH256);
    for i in 0..32 {
        sh.process(randbytes32[i]);
    }
    sh.hash(&mut hm);
    
    sh = SHA3::new(sha3::HASH256);
    for i in 0..public_key_size {
        sh.process(pk[i]);
    }
    sh.hash(&mut h);

    sh = SHA3::new(sha3::HASH512);
    sh.process_array(&hm);
    sh.process_array(&h);
    sh.hash(&mut g);
    cpa_encrypt(params,&g[32..],&pk,&hm,ct);

    sh = SHA3::new(sha3::HASH256);
    for i in 0..ciphertext_size {
        sh.process(ct[i]);
    }
    sh.hash(&mut h);
    sh = SHA3::new(sha3::SHAKE256);
    sh.process_array(&g[0..32]);
    sh.process_array(&h);
    sh.shake(ss,shared_secret_size);
}

fn cca_decrypt(params: &[usize],sk: &[u8],ct: &[u8],ss: &mut [u8]) {
    let mut m:[u8;32]=[0;32];
    let mut g:[u8;64]=[0;64];
    let ck=params[0];
    let secret_cpa_key_size=ck*(DEGREE*3)/2;
    let public_key_size=32+ck*(DEGREE*3)/2;
    let shared_secret_size=params[5];    

    let pk=&sk[secret_cpa_key_size..secret_cpa_key_size+public_key_size];
    let h=&sk[secret_cpa_key_size+public_key_size..secret_cpa_key_size+public_key_size+32];
    let z=&sk[secret_cpa_key_size+public_key_size+32..secret_cpa_key_size+public_key_size+64];

    cpa_decrypt(params,sk,ct,&mut m);

    let mut sh = SHA3::new(sha3::HASH512);
    sh.process_array(&m);
    sh.process_array(h);
    sh.hash(&mut g);

    let mask=cpa_check_encrypt(params,&g[32..],pk,&m,ct); // FO check ct is correct

    for i in 0..32 {
        g[i]^=(g[i]^z[i])&mask;
    }

    sh = SHA3::new(sha3::HASH256);
    sh.process_array(&ct);
    sh.hash(&mut m);

    sh = SHA3::new(sha3::SHAKE256);
    sh.process_array(&g[0..32]);
    sh.process_array(&m);
    sh.shake(ss,shared_secret_size);
}

// ********************* Kyber API ******************************

pub fn keypair_512(randbytes64: &[u8],sk: &mut [u8],pk: &mut [u8]) {
    cca_keypair(&PARAMS_512,randbytes64,sk,pk);
}

pub fn keypair_768(randbytes64: &[u8],sk: &mut [u8],pk: &mut [u8]) {
    cca_keypair(&PARAMS_768,randbytes64,sk,pk);
}

pub fn keypair_1024(randbytes64: &[u8],sk: &mut [u8],pk: &mut [u8]) {
    cca_keypair(&PARAMS_1024,randbytes64,sk,pk);
}

pub fn encrypt_512(randbytes32: &[u8],pk: &[u8],ss: &mut [u8],ct: &mut [u8]) {
    cca_encrypt(&PARAMS_512,randbytes32,pk,ss,ct);
}

pub fn encrypt_768(randbytes32: &[u8],pk: &[u8],ss: &mut [u8],ct: &mut [u8]) {
    cca_encrypt(&PARAMS_768,randbytes32,pk,ss,ct);
}

pub fn encrypt_1024(randbytes32: &[u8],pk: &[u8],ss: &mut [u8],ct: &mut [u8]) {
    cca_encrypt(&PARAMS_1024,randbytes32,pk,ss,ct);
}

pub fn decrypt_512(sk: &[u8],ct: &[u8],ss: &mut [u8]) {
    cca_decrypt(&PARAMS_512,sk,ct,ss);
}

pub fn decrypt_768(sk: &[u8],ct: &[u8],ss: &mut [u8]) {
    cca_decrypt(&PARAMS_768,sk,ct,ss);
}

pub fn decrypt_1024(sk: &[u8],ct: &[u8],ss: &mut [u8]) {
    cca_decrypt(&PARAMS_1024,sk,ct,ss);
}
