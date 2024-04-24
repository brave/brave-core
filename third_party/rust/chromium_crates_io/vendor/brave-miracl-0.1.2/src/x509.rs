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

/* CORE X.509 Functions */

pub struct PKTYPE {
    pub kind: usize,
    pub hash: usize,
    pub curve: usize,
    pub len: usize,
}

pub struct FDTYPE {
    pub index: usize,
    pub length: usize,
}

// Supported Encryption/Signature Methods

pub const ECC:usize = 1;
pub const RSA:usize = 2;
pub const ECD:usize = 3;  // for Ed25519 and Ed448
pub const PQ:usize = 4;

// Supported Hash functions

pub const H256:usize = 2;
pub const H384:usize = 3;
pub const H512:usize = 4;
pub const SHAKE256:usize = 5;

// Supported Curves

pub const USE_NIST256:usize = 4;    /**< For the NIST 256-bit standard curve - WEIERSTRASS only */
pub const USE_ED25519:usize = 1;     /**< Bernstein's Modulus 2^255-19 - EDWARDS only */
pub const USE_ED448:usize = 5;
//const USE_BRAINPOOL:usize = 2;  /**< For Brainpool 256-bit curve - WEIERSTRASS only */
//const USE_ANSSI:usize = 3;      /**< For French 256-bit standard curve - WEIERSTRASS only */
pub const USE_NIST384:usize = 10;   /**< For the NIST 384-bit standard curve - WEIERSTRASS only */
pub const USE_NIST521:usize = 12;   /**< For the NIST 521-bit standard curve - WEIERSTRASS only */

const ANY: u8 = 0x00;
const SEQ: u8 = 0x30;
const OID: u8 = 0x006;
const INT: u8 = 0x02;
const NUL: u8 = 0x05;
//const ZER: u8 = 0x00;
//const UTF: u8 = 0x0C;
const UTC: u8 = 0x17;
const GTM: u8 = 0x18;
//const LOG: u8 = 0x01;
const BIT: u8 = 0x03;
const OCT: u8 = 0x04;
//const STR: u8 = 0x13;
const SET: u8 = 0x31;
//const IA5: u8 = 0x16;
const EXT: u8 = 0xA3;
const DNS: u8 = 0x82;

// Define some OIDs
// Elliptic Curve with SHA256

const ECCSHA256:[u8;8]=[0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02];
const ECCSHA384:[u8;8]=[0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x03];
const ECCSHA512:[u8;8]=[0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x04];
const ECPK:[u8;7]=[0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01];
const EDPK25519:[u8;3]=[0x2b, 0x65, 0x70];
const EDPK448:[u8;3]=[0x2b, 0x65, 0x71];
const PRIME25519:[u8;9]=[0x2B, 0x06, 0x01, 0x04, 0x01, 0xDA, 0x47, 0x0F, 0x01];
const PRIME256V1:[u8;8]=[0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07];
const SECP384R1:[u8;5]=[0x2B, 0x81, 0x04, 0x00, 0x22];
const SECP521R1:[u8;5]=[0x2B, 0x81, 0x04, 0x00, 0x23];
const RSAPK:[u8;9]=[0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01];
const RSASHA256:[u8;9]=[0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b];
const RSASHA384:[u8;9]=[0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0];
const RSASHA512:[u8;9]=[0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0d];
const DILITHIUM3:[u8;11]=[0x2b, 0x06, 0x01, 0x04, 0x01, 0x02, 0x82, 0x0B, 0x07, 0x06, 0x05];
// Cert details

pub const CN:[u8;3]=[0x55, 0x04, 0x06]; // countryName
pub const SN:[u8;3]=[0x55, 0x04, 0x08]; // stateName
pub const LN:[u8;3]=[0x55, 0x04, 0x07]; // localName
pub const ON:[u8;3]=[0x55, 0x04, 0x0A]; // orgName
pub const UN:[u8;3]=[0x55, 0x04, 0x0B]; // unitName
pub const MN:[u8;3]=[0x55, 0x04, 0x03]; // myName
pub const EN:[u8;9]=[0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01]; // emailName

// Extensions
pub const AN:[u8;3]=[0x55,0x1D,0x11]; // altName
pub const KU:[u8;3]=[0x55,0x1D,0x0F]; // keyUsage
pub const BC:[u8;3]=[0x55,0x1D,0x13]; // basicConstraints

fn getalen(tag: u8,b:&[u8],j:usize) -> usize {
    let mut k=j;
    let mut len:usize;
    if tag!=0 && b[k]!=tag {
        return 0;
    }
    k+=1;
    if b[k] == 0x81 {
        k+=1;
        len=b[k] as usize;
    } else if b[k]==0x82 {
        k+=1;
        len=256*(b[k] as usize); k+=1;
        len+= b[k] as usize;
    } else {
        len=b[k] as usize;
        if len>127 {
            return 0;
        }
    }
    return len;
}

fn skip(len: usize) -> usize {
    if len<128 {
        return 2;
    }
    if len<256 {
        return 3;
    }
    return 4;
}

fn bround(len:usize) -> usize {
    if len%8 == 0 {
        return len;
    }
    return len+(8-len%8);
}

impl PKTYPE {
    pub fn new() -> PKTYPE {
        PKTYPE {
            kind: 0,
            hash: 0,
            curve:0,
            len:0,
        }
    }
}

impl FDTYPE {
    pub fn new() -> FDTYPE {
        FDTYPE {
            index: 0,
            length: 0,
        }
    }
}

// Input private key in PKCS#8 format
// e.g. openssl req -x509 -nodes -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365
// e.g. openssl req -x509 -nodes -days 3650 -newkey ec:<(openssl ecparam -name prime256v1) -keyout key.pem -out ecdsacert.pem
// extract private key from uncompressed key.pem into octet
// For RSA octet = p|q|dp|dq|c where pk->len is multiple of 5
// For ECC octet = k
pub fn extract_private_key(c: &[u8],pk: &mut [u8]) -> PKTYPE {
    let mut soid:[u8;12]=[0;12];
    let mut ret=PKTYPE::new();
    let mut j=0 as usize;
    let pklen=pk.len();

    let mut len=getalen(SEQ,c,j);  // Check for expected SEQ clause, and get length
    if len == 0  {                  // if not a SEQ clause, there is a problem, exit
        return ret;
    }
    j+=skip(len);                   // skip over length to clause contents.
    if len+j != c.len() {
        return ret;
    }
    len=getalen(INT,c,j);
    if len == 0  {                  // if not a SEQ clause, there is a problem, exit
        return ret;
    }
    j+=skip(len)+len;
    len=getalen(SEQ,c,j);
    if len == 0  {                  // if not a SEQ clause, there is a problem, exit
        return ret;
    }
    j+=skip(len);
// extract OID
    len=getalen(OID,c,j);
    if len==0 {
        return ret;
    }
    j+=skip(len);

    let mut fin=j+len;
    if len>soid.len() {
        return ret;
    }
    let mut slen=0;
    while j<fin {
        soid[slen]=c[j];
        slen+=1;
        j+=1;
    }
    j=fin;
    if EDPK25519 == soid[0..slen] {
        len=getalen(OCT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        len=getalen(OCT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        let rlen=32;
        if rlen>pklen {
            return ret;
        }
        ret.len=rlen;
        for i in 0..rlen-len {
            pk[i]=0;
        }
        for i in rlen-len..rlen {
            pk[i]=c[j];
            j+=1;
        }
        ret.kind = ECD;
        ret.curve = USE_ED25519;
    }
    if EDPK448 == soid[0..slen] {
        len=getalen(OCT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        len=getalen(OCT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        let rlen=57;
        if rlen>pklen {
            return ret;
        }
        ret.len=rlen;
        for i in 0..rlen-len {
            pk[i]=0;
        }
        for i in rlen-len..rlen {
            pk[i]=c[j];
            j+=1;
        }
        ret.kind = ECD;
        ret.curve = USE_ED448;
    }
    if DILITHIUM3 == soid[0..slen] {
        len=getalen(OCT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        len=getalen(OCT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        let mut tlen=len;
        if tlen>pk.len() {
            tlen=pk.len();
        }

        for i in 0..tlen {
            pk[i]=c[j];
            j+=1;
        }
        ret.len=tlen;
        ret.kind=PQ;
        ret.curve=8*tlen;
    }    
    if ECPK == soid[0..slen] {
        len=getalen(OID,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        
        fin=j+len;
        if len>soid.len() {
            return ret;
        }
        slen=0;
        while j<fin {
            soid[slen]=c[j];
            slen+=1;
            j+=1;
        }
        j=fin;
        len=getalen(OCT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        len=getalen(SEQ,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        len=getalen(INT,c,j);
        if len == 0  {     
            return ret;
        }
        j+=skip(len)+len;    // jump over version
        len=getalen(OCT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);

        ret.kind=ECC;
        let mut rlen=0;
        if PRIME256V1 == soid[0..slen] {
            ret.curve=USE_NIST256;
            rlen=32;
        }
        if SECP384R1 == soid[0..slen] {
            ret.curve=USE_NIST384;
            rlen=48;
        }
        if SECP521R1 == soid[0..slen] {
            ret.curve=USE_NIST521;
            rlen=66;
        }
        if rlen>pklen {
            ret.curve=0;
            ret.len=0;
            return ret;
        }
        ret.len=rlen;
        for i in 0..rlen-len {
            pk[i]=0;
        }
        for i in rlen-len..rlen {
            pk[i]=c[j];
            j+=1;
        }
    }
    if RSAPK == soid[0..slen] {
        len=getalen(NUL,c,j);
        if len!=0 {
            return ret;
        }
        j+=skip(len); 

        len=getalen(OCT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);       

        len=getalen(SEQ,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);

        len=getalen(INT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len)+len; // jump over version

        len=getalen(INT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len)+len; // jump over n

        len=getalen(INT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len)+len; // jump over e

        len=getalen(INT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len)+len; // jump over d

        len=getalen(INT,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len); // get p

        if c[j]==0 {
            j+=1;
            len-=1;
        }
        let mut rlen=bround(len);

        if 5*rlen>pklen {
            return ret;
        }

        for i in 0..rlen-len {
            pk[i]=0;
        }
        for i in rlen-len..rlen {
            pk[i]=c[j];
            j+=1;
        }

        let flen=rlen;   // should be same length for all
        for k in 1..5 {
            len=getalen(INT,c,j);
            if len==0 {
                return ret;
            }
            j+=skip(len); // get q,dp,dq,c
            if c[j]==0 {
                j+=1;
                len-=1;
            }
            rlen=bround(len);  
            if rlen!=flen {
                return ret;
            }
            for i in 0..rlen-len {
                pk[i]=0;
            }
            for i in rlen-len..rlen {
                pk[k*flen+i]=c[j];
                j+=1;
            }
        }
        ret.len=5*flen;
        ret.kind=RSA;
        ret.curve=16*flen;
    }
    return ret;
}

//  Input signed cert as octet, and extract signature
//  Return 0 for failure, ECC for Elliptic Curve signature, RSA for RSA signature
//  Note that signature type is not provided here - its the type of the public key that
//  is used to verify it that matters, and which determines for example the curve to be used!
pub fn extract_cert_sig(sc: &[u8],sig: &mut [u8]) -> PKTYPE {
    let mut soid:[u8;12]=[0;12];
    let mut ret=PKTYPE::new();
    let mut j=0 as usize;
    let mut len=getalen(SEQ,sc,j);  // Check for expected SEQ clause, and get length
    let siglen=sig.len();

    if len == 0  {                  // if not a SEQ clause, there is a problem, exit
        return ret;
    }
    j+=skip(len);                   // skip over length to clause contents. Add len to skip clause
    if len+j != sc.len() {
        return ret;
    }
    len=getalen(SEQ,sc,j);
    if len==0 {
        return ret;
    }
    j+=skip(len) + len; // jump over cert to signature OID
    len=getalen(SEQ,sc,j);
    if len==0 {
        return ret;
    }
    j+=skip(len);
    let sj=j+len;      // Needed to jump over signature OID

// dive in to extract OID
    len=getalen(OID,sc,j);
    if len==0 {
        return ret;
    }
    j+=skip(len);
    let mut fin=j+len;
    if len>soid.len() {
        return ret;
    }

    let mut slen=0;
    while j<fin {
        soid[slen]=sc[j];
        slen+=1;
        j+=1;

    }
    if EDPK25519 == soid[0..slen] {
        ret.kind=ECD;
        ret.hash=H512;
    }
    if EDPK448 == soid[0..slen] {
        ret.kind=ECD;
        ret.hash=SHAKE256;
    }
    if ECCSHA256 == soid[0..slen] {
        ret.kind=ECC;
        ret.hash=H256;
    }
    if ECCSHA384 == soid[0..slen] {
        ret.kind=ECC;
        ret.hash=H384;
    } 
    if ECCSHA512 == soid[0..slen] {
        ret.kind=ECC;
        ret.hash=H512;
    }
    if RSASHA256 == soid[0..slen] {
        ret.kind=RSA;
        ret.hash=H256;
    }
    if RSASHA384 == soid[0..slen] {
        ret.kind=RSA;
        ret.hash=H384;
    }
    if RSASHA512 == soid[0..slen] {
        ret.kind=RSA;
        ret.hash=H512;
    }
    if DILITHIUM3 == soid[0..slen] {
        ret.kind=PQ;
        ret.hash=0; // hash type is implicit
    }
    if ret.kind==0 { 
        return ret;  // unsupported type
    }

    j=sj;
    len=getalen(BIT,sc,j);
    if len==0 {
        ret.kind=0;
        return ret;
    }
    j+=skip(len);
    j+=1;
    len-=1; // skip bit shift (hopefully 0!)

    if ret.kind==ECD {
        if len>siglen {
            ret.kind=0;
            return ret;
        }
        ret.len=len;
        slen=0;
        fin=j+len;
        while j<fin {
            sig[slen]=sc[j];
            j+=1;
            slen+=1;
        }
        if ret.hash==H512 {
            ret.curve=USE_ED25519;
        }
        if ret.hash==SHAKE256 {
            ret.curve=USE_ED448;
        }
    }

    if ret.kind==ECC {
        len=getalen(SEQ,sc,j);
        if len==0 {
            ret.kind=0;
            return ret;
        }
        j+=skip(len);

        // pick up r part of signature
        len=getalen(INT,sc,j);
        if len==0 {
            ret.kind=0;
            return ret;
        }
        j+=skip(len);
        if sc[j]==0 { // skip leading zero
            j+=1;
            len-=1;
        }
        let mut rlen=bround(len);
        let mut ex=rlen-len;

        if 2*rlen>siglen {
            ret.kind=0;
            return ret;
        }
        ret.len=2*rlen;

        slen=0;
        for _ in 0..ex {
            sig[slen]=0;
            slen+=1;
        }
        fin=j+len;
        while j<fin {
            sig[slen]=sc[j];
            j+=1;
            slen+=1;
        }
        // pick up s part of signature
        len=getalen(INT,sc,j);
        if len==0 {
            ret.kind=0;
            return ret;
        }
        j+=skip(len);
        if sc[j]==0 { // skip leading zero
            j+=1;
            len-=1;
        }
        rlen=bround(len);
        ex=rlen-len;
        for _ in 0..ex {
            sig[slen]=0;
            slen+=1;
        }
        fin=j+len;
        while j<fin {
            sig[slen]=sc[j];
            j+=1;
            slen+=1;
        }
        if ret.hash==H256 {
            ret.curve=USE_NIST256;
        }
        if ret.hash==H384 {
            ret.curve=USE_NIST384;
        }
        if ret.hash==H512 {
            ret.curve=USE_NIST521;
        }
    }
    if ret.kind==RSA {
        let rlen=bround(len);
        let ex=rlen-len;
        if rlen>siglen {
            ret.kind=0;
            ret.curve=0;
            return ret;
        }
        ret.len=rlen;
        slen=0;
        for _ in 0..ex {
            sig[slen]=0;
            slen+=1;
        }
        fin=j+len;
        while j<fin {
            sig[slen]=sc[j];
            j+=1;
            slen+=1;
        }
        ret.curve=8*rlen;
    }
    if ret.kind==PQ {
        if len>siglen {
            ret.kind=0;
            ret.curve=0;
            return ret;
        }
        ret.len=len;
        slen=0;
        fin=j+len;
        while j<fin {
            sig[slen]=sc[j];
            j+=1;
            slen+=1;
        }
        ret.curve=8*len;
    }
    return ret;
}

// Extract pointer to cert inside signed cert, and return its length;
// let cert=&sc[ptr..ptr+len]
pub fn find_cert(sc: &[u8],ptr: &mut usize) -> usize {
    let mut j:usize=0;

    let mut len=getalen(SEQ,sc,j);
    if len==0 {
        return 0;
    }
    j+=skip(len);

    let k=j;
    len=getalen(SEQ,sc,j);
    if len==0 {
        return 0;
    }
    j+=skip(len);
    let fin=j+len;
    *ptr=k;
    return fin-k;
}

// Extract certificate from signed cert
pub fn extract_cert(sc: &[u8],cert: &mut [u8]) -> usize {
    let mut ptr=0;
    let n=find_cert(sc,&mut ptr);
    let k=ptr;
    let fin=n+k;
    if fin-k>cert.len() {
        return 0;
    }
    for i in k..fin {
        cert[i-k]=sc[i];
    }
    return n;
}

// extract pointer to ASN.1 raw public Key inside certificate, and return its length;
// let public_key=&c[ptr..ptr+len]
pub fn find_public_key(c: &[u8],ptr: &mut usize) -> usize {
    let mut j:usize=0;
    let mut len=getalen(SEQ,c,j);
    if len==0 {
        return 0;
    }
    j+=skip(len);

    if len+j != c.len() {
        return 0;
    }

    len=getalen(ANY,c,j);
    if len==0 {
        return 0;
    }
    j+=skip(len)+len; //jump over version clause

    len=getalen(INT,c,j);
    if len>0 {
        j+=skip(len)+len; // jump over serial number clause (if there is one)
    }

    len=getalen(SEQ,c,j);
    if len==0 {
        return 0;
    }
    j+=skip(len)+len; // jump over signature algorithm

    len=getalen(SEQ,c,j);
    if len==0 {
        return 0;
    }
    j += skip(len) + len; // skip issuer

    len=getalen(SEQ,c,j);
    if len==0 {
        return 0;
    }
    j += skip(len) + len; // skip validity

    len=getalen(SEQ,c,j);
    if len==0 {
        return 0;
    }
    j += skip(len) + len; // skip subject

    let k=j;
    len=getalen(SEQ,c,j);
    if len==0 {
        return 0;
    }
    j += skip(len); // 

    let fin=j+len;
    *ptr=k;
    return fin-k;
}

// get Public details from ASN.1 description
pub fn get_public_key(c: &[u8],key: &mut [u8]) -> PKTYPE {
    let mut koid:[u8;12]=[0;12];
    let mut ret=PKTYPE::new();
    let mut j=0;
    let keylen=key.len();

    let mut len=getalen(SEQ,c,j);
    if len==0 {
        return ret;
    }
    j += skip(len); // 

    len=getalen(SEQ,c,j);
    if len==0 {
        return ret;
    }
    j += skip(len); //

// ** Maybe dive in and check Public Key OIDs here?
// ecpublicKey & prime256v1, secp384r1 or secp521r1 for ECC
// rsapublicKey for RSA

    let sj=j+len;

    len=getalen(OID,c,j);
    if len==0 {
        return ret;
    }
    j += skip(len); 

    let mut fin=j+len;
    if len>koid.len() {
        return ret;
    }
    let mut slen=0;
    while j<fin {
        koid[slen]=c[j];
        slen+=1;
        j+=1;
    }
    ret.kind=0;
    if ECPK == koid[0..slen] {
        ret.kind=ECC;
    }
    if EDPK25519 == koid[0..slen] {
        ret.kind=ECD; ret.curve=USE_ED25519
    }
    if EDPK448 == koid[0..slen] {
        ret.kind=ECD; ret.curve=USE_ED448
    }
    if RSAPK == koid[0..slen] {
        ret.kind=RSA;
    }
    if DILITHIUM3 == koid[0..slen] {
        ret.kind=PQ;
    }

    if ret.kind==0 {
        return ret;
    }
    if ret.kind==ECC {
        len=getalen(OID,c,j);
        if len==0 {
            ret.kind=0;
            return ret;
        }
        j+=skip(len);

        fin=j+len;
        if len>koid.len() {
            ret.kind=0;
            return ret;
        }
        slen=0;
        while j<fin {
            koid[slen]=c[j];
            slen+=1;
            j+=1;
        }
        if PRIME25519==koid[0..slen] {
            ret.curve=USE_ED25519;
        }
        if PRIME256V1==koid[0..slen] {
            ret.curve=USE_NIST256;
        }
        if SECP384R1==koid[0..slen] {
            ret.curve=USE_NIST384;
        }
        if SECP521R1==koid[0..slen] {
            ret.curve=USE_NIST521;
        }
    }
    j=sj;

    len=getalen(BIT,c,j);
    if len==0 {
        ret.kind=0;
        return ret;
    }
    j+=skip(len);
    j+=1;
    len-=1; // skip bit shift (hopefully 0!)

    if ret.kind==ECC || ret.kind==ECD || ret.kind==PQ {
        if len>keylen {
            ret.kind=0;
            return ret;
        }
        ret.len=len;
        fin=j+len;
        slen=0;
        while j<fin {
            key[slen]=c[j];
            slen+=1;
            j+=1;
        }
    }
    if ret.kind==PQ {
        ret.curve=8*len;
    }
    if ret.kind==RSA { // Key is (modulus,exponent) - assume exponent is 65537
        len=getalen(SEQ,c,j);
        if len==0 {
            ret.kind=0;
            return ret;
        }
        j+=skip(len);

        len=getalen(INT,c,j);
        if len==0 {
            ret.kind=0;
            return ret;
        }
        j+=skip(len);
        if c[j]==0 {
            j+=1;
            len-=1;
        }
        if len>keylen {
            ret.kind=0;
            return ret;
        }
        ret.len=len;
        fin=j+len;
        slen=0;
        while j<fin {
            key[slen]=c[j];
            slen+=1;
            j+=1;
        }
        ret.curve=8*len;
    }
    return ret; 
}

// Extract Public Key from inside Certificate
pub fn extract_public_key(c: &[u8],key: &mut [u8]) -> PKTYPE {
    let mut ptr=0;
    let pklen = find_public_key(c,&mut ptr); // ptr is pointer into certificate, at start of ASN.1 raw public key
    let cc=&c[ptr..ptr+pklen];
    return get_public_key(&cc,key);
}

pub fn find_issuer(c: &[u8]) -> FDTYPE {
    let mut j:usize=0;
    let mut ret=FDTYPE::new();
    let mut len=getalen(SEQ,c,j);
    if len==0 {
        return ret;
    }
    j += skip(len); 

    if len+j!=c.len() {
        return ret;
    }

    len=getalen(ANY,c,j);
    if len==0 {
        return ret;
    }
    j += skip(len)+len; // jump over version clause
    
    len=getalen(INT,c,j);
    if len>0 {
        j+=skip(len)+len; // jump over serial number clause (if there is one)
    }

    len=getalen(SEQ,c,j);
    if len==0 {
        return ret;
    }
    j += skip(len) + len; // jump over signature algorithm

    len=getalen(SEQ,c,j);
    ret.index=j;
    ret.length=len+skip(len);

    return ret;
}

pub fn find_validity(c: &[u8]) -> usize {
    let pos=find_issuer(c);
    let j=pos.index+pos.length; // skip issuer

    //let mut j=find_issuer(c);
    //let len=getalen(SEQ,c,j);
    //if len==0 {
    //    return 0;
    //}
    //j+=skip(len)+len; // skip issuer
    return j;
}

pub fn find_subject(c: &[u8]) -> FDTYPE {
    let mut j=find_validity(c);
    let mut ret=FDTYPE::new();
    let mut len=getalen(SEQ,c,j);
    if len==0 {
        return ret;
    }
    j+=skip(len)+len; // skip validity

    len=getalen(SEQ,c,j);
    ret.index=j;
    ret.length=len+skip(len);

    return ret;
}

pub fn self_signed(c: &[u8]) -> bool {
    let ksub=find_subject(c);
    let kiss=find_issuer(c);

    if ksub.length!=kiss.length {
        return false;
    }

//    let sublen=getalen(SEQ,c,ksub);
//    let isslen=getalen(SEQ,c,kiss);
//    if sublen != isslen {
//        return false;
//    }
//    ksub+=skip(sublen);
//    kiss+=skip(isslen);
    let mut m:u8=0;
    for i in 0..ksub.length {
        m |= c[i+ksub.index]-c[i+kiss.index];
    }
    if m!=0 {
        return false;
    }
    return true;
}

// NOTE: When extracting cert information, we actually return just an index to the data inside the cert, and maybe its length
// So no memory is assigned to store cert info. It is the callers responsibility to allocate such memory if required, and copy
// cert information into it.

// Find entity property indicated by SOID, given start of issuer or subject field. Return index in cert, flen=length of field

pub fn find_entity_property(c: &[u8],soid: &[u8],start: usize) -> FDTYPE {
    let mut ret=FDTYPE::new();
    let mut foid:[u8;32]=[0;32];
    let mut j=start;
    let tlen=getalen(SEQ,c,j);
    if tlen==0 {
        return ret;
    }
    j+=skip(tlen);
    let k=j;
    while j<k+tlen {
        let mut len=getalen(SET,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        len=getalen(SEQ,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        len=getalen(OID,c,j);
        if len==0 {
            return ret;
        }
        j+=skip(len);
        let fin=j+len;
        if len>foid.len() {
            return ret;
        }
        let mut flen:usize=0;
        while j<fin {
            foid[flen]=c[j];
            flen+=1;
            j+=1;
        }
        len=getalen(ANY,c,j); // get text, could be any type
        if len==0 {
            return ret;
        }
        j+=skip(len);
        if foid[0..flen]==*soid {
            ret.index=j; // if its the right one..
            ret.length=len;
            return ret;
        }
        j+=len; // skip over it
    }
    return ret;
}

pub fn find_start_date(c: &[u8],start: usize) -> usize {
    let mut j=start;
    let mut len=getalen(SEQ,c,j);
    if len==0 {
        return 0;
    }
    j+=skip(len);

    len=getalen(UTC,c,j);
    if len==0 { // could be generalised time
        len=getalen(GTM,c,j);
        if len==0 {
            return 0;
        }
        j += skip(len);
        j +=2; // skip century
    } else {
        j+=skip(len);
    }
    return j;
}

pub fn find_expiry_date(c: &[u8],start: usize) -> usize {
    let mut j=start;
    let mut len=getalen(SEQ,c,j);
    if len==0 {
        return 0;
    }
    j+=skip(len);

    len=getalen(UTC,c,j);
    if len==0 {
        len=getalen(GTM,c,j);
        if len==0 {
            return 0;
        }
    }
    j+=skip(len)+len;

    len=getalen(UTC,c,j);
    if len==0 { // could be generalised time
        len=getalen(GTM,c,j);
        if len==0 {
            return 0;
        }
        j += skip(len);
        j +=2;  // skip century
    } else {
        j+=skip(len);
    }
    return j;
}

pub fn find_extensions(c: &[u8]) -> usize {
    let pos=find_subject(c);
    let mut j=pos.index+pos.length;

//    let mut len=getalen(SEQ,c,j);
//    if len==0 {
//        return 0;
//    }
//    j+=skip(len)+len; // skip subject

    let len=getalen(SEQ,c,j);
    if len==0 {
        return 0;
    }
    j+=skip(len)+len; // skip public key

    if j>=c.len() {
        return 0;
    }
    return j;
}

pub fn find_extension(c: &[u8],soid: &[u8],start:usize) -> FDTYPE {
    let mut ret=FDTYPE::new();
    let mut foid:[u8;32]=[0;32];
    
    let mut j=start;
    let tlen=getalen(EXT,c,j);
    if tlen==0 {
        return ret;
    }
    j+=skip(tlen);

    let tlen=getalen(SEQ,c,j);
    if tlen==0 {
        return ret;
    }
    j+=skip(tlen);

    let k=j;
    while j<k+tlen {
        let mut len=getalen(SEQ,c,j);
        if len==0 {
            return ret;
        } 
        j+=skip(len);
        let nj=j+len;
        len=getalen(OID,c,j);
        j+=skip(len);
        let fin=j+len;
        if len>foid.len() {
            return ret;
        }
        let mut flen:usize=0;
        while j<fin {
            foid[flen]=c[j];
            flen+=1;
            j+=1;
        }
        if foid[0..flen]==*soid {
            ret.index=j; // if its the right one..
            ret.length=nj-j;
            return ret;
        }
        j=nj;  // skip over this extension
    }
    return ret;
}

// return 1 if name found, else 0, where name is URL
// input cert, and pointer to SAN extension
// Takes wild-card into consideration

pub fn find_alt_name(c: &[u8],start: usize,name: &[u8]) -> bool {
    if start==0 {
        return false;
    }
    let mut j=start;
    let mut tlen=getalen(OCT,c,j);
    if tlen==0 {
        return false;
    }
    j+=skip(tlen);

    tlen=getalen(SEQ,c,j);
    if tlen==0 {
        return false;
    }
    j+=skip(tlen);
    let k=j;
    while j<k+tlen {
        let tag=c[j];
        let mut len=getalen(ANY,c,j);
        if len==0 {
            return false;
        }
        j+=skip(len); // ?? If its not dns, skip over it j+=len
        if tag!=DNS { // only interested in URLs
            j+=len;
            continue;
        }
        let mut cmp=true;
        let mut m=0;
        let nlen=name.len();
        if c[j]=='*' as u8 {
            j+=1; len-=1; // skip over *
            while m<nlen { // advance to first .
                if name[m]=='.' as u8 {
                    break;
                }
                m+=1;
            }
        }
        for _ in 0..len {
            if m==nlen { // name has ended before comparison completed
                cmp=false;
                j+=1;
                continue;
            }
            if c[j] != name[m] {
                cmp=false;
            }
            m+=1; j+=1;
        }
        if m!=nlen {
            cmp=false;
        }
        if cmp {
            return true;
        }
    }
    return false;
}

