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

use crate::bn254::ecp;
use crate::bn254::ecdh;

use crate::hmac;
use crate::rand::RAND;

const GROUP: usize = ecdh::EGS;
const POINT: usize = 2*ecdh::EFS+1;

const MAX_LABEL: usize = 20;                // may need adjustment

#[allow(non_snake_case)]
fn reverse (x: &mut [u8]) {
	let lx=x.len();
	for i in 0..lx/2 {
		let ch=x[i];
		x[i]=x[lx-i-1];
		x[lx-i-1]=ch;
	}
}
/*
fn printbinary(array: &[u8]) {
    for i in 0..array.len() {
        print!("{:02X}", array[i])
    }
    println!("")
}
*/

#[allow(non_snake_case)]
fn labeledExtract(prk: &mut [u8],salt: Option<&[u8]>,suite_id: &[u8],label: &str,ikm: Option<&[u8]>) {
    let rfc="HPKE-v1";
    let prefix1=rfc.as_bytes();
    let prefix2=label.as_bytes();
    let mut likm: [u8; 18+MAX_LABEL+2*POINT] = [0; 18+MAX_LABEL+2*POINT];
    let mut k=0;
    for i in 0..prefix1.len() {
        likm[k]=prefix1[i];
        k+=1;
    }
    for i in 0..suite_id.len() {
        likm[k]=suite_id[i];
        k+=1;
    }
    for i in 0..prefix2.len() {
        likm[k]=prefix2[i];
        k+=1;
    }
    if let Some(sikm) = ikm {
        for i in 0..sikm.len() {
            likm[k]=sikm[i];
            k+=1;
        }
    }
    hmac::hkdf_extract(hmac::MC_SHA2,ecp::HASH_TYPE,prk,salt,&likm[0..k]);
}

#[allow(non_snake_case)]
fn labeledExpand(okm: &mut [u8],prk: &[u8],suite_id: &[u8],label: &str,info: Option<&[u8]>,el: usize) {
    let mut ar: [u8; 2] = [0; 2];
    let rfc="HPKE-v1";
    let prefix1=rfc.as_bytes();
    let prefix2=label.as_bytes();
    hmac::inttobytes(el,&mut ar);
    let mut linfo: [u8; 20+MAX_LABEL+3*POINT] = [0; 20+MAX_LABEL+3*POINT];
    linfo[0]=ar[0];
    linfo[1]=ar[1];
    let mut k=2;
    for i in 0..prefix1.len() {
        linfo[k]=prefix1[i];
        k+=1;
    }
    for i in 0..suite_id.len() {
        linfo[k]=suite_id[i];
        k+=1;
    }
    for i in 0..prefix2.len() {
        linfo[k]=prefix2[i];
        k+=1;
    }
    if let Some(sinfo) = info {
        for i in 0..sinfo.len() {
            linfo[k]=sinfo[i];
            k+=1;
        }
    }
    hmac:: hkdf_expand(hmac::MC_SHA2,ecp::HASH_TYPE,okm,el,prk,&linfo[0..k]);
}

#[allow(non_snake_case)]
fn extractAndExpand(config_id: usize,okm: &mut [u8],dh: &[u8],context: &[u8]) {
    let kem = config_id&255;
    let txt="KEM";
    let mut suite_id: [u8;5] = [0;5];
    let mut kem_id: [u8; 2] = [0; 2];
    let ckem=txt.as_bytes();
    hmac::inttobytes(kem,&mut kem_id);
    let mut k=0;
    for i in 0..ckem.len() {
        suite_id[k]=ckem[i];
        k+=1;
    }
    suite_id[k]=kem_id[0]; k+=1;
    suite_id[k]=kem_id[1];

    let mut prk: [u8;ecp::HASH_TYPE]=[0;ecp::HASH_TYPE];
    labeledExtract(&mut prk,None,&suite_id,"eae_prk",Some(dh));
    labeledExpand(okm,&prk,&suite_id,"shared_secret",Some(&context),ecp::HASH_TYPE);
}

#[allow(non_snake_case)]
pub fn deriveKeyPair(config_id: usize,mut sk: &mut [u8],mut pk: &mut [u8],seed: &[u8]) -> bool {
    let mut counter=0;
    let kem = config_id&255;
    let txt="KEM";
    let mut suite_id: [u8;5] = [0;5];
    let mut kem_id: [u8; 2] = [0; 2];
    let ckem=txt.as_bytes();
    hmac::inttobytes(kem,&mut kem_id);
    let mut k=0;
    for i in 0..ckem.len() {
        suite_id[k]=ckem[i];
        k+=1;
    }
    suite_id[k]=kem_id[0]; k+=1;
    suite_id[k]=kem_id[1];

    let mut prk: [u8;ecp::HASH_TYPE]=[0;ecp::HASH_TYPE];
    labeledExtract(&mut prk,None,&suite_id,"dkp_prk",Some(&seed));

    //println!("prk= {:02X?}",prk);

	if kem==32 || kem==33 { // RFC7748
        labeledExpand(&mut sk,&prk,&suite_id,"sk",None,GROUP);
        reverse(&mut sk);
		if kem==32 {
			sk[GROUP-1]&=248;
			sk[0]&=127;
			sk[0]|=64;
		} else {
			sk[GROUP-1]&=252;
			sk[0]|=128;
		}
    } else {
        let mut bit_mask=0xff;
        if kem==18 {
            bit_mask=1;
        }
        for i in 0..GROUP {
            sk[i]=0;
        }
        while !ecdh::in_range(&sk) && counter<256 {
            let mut info: [u8;1]=[0;1];
            info[0]=counter as u8;
            labeledExpand(sk,&prk,&suite_id,"candidate",Some(&info),GROUP);
            sk[0] &= bit_mask as u8;
            counter += 1;
        }
    }
    //for i in 0..sk.len() {
//	print!({}
//    println!("SK= {:02X?}",sk);
  //  println!("kem= {}",kem);
    //println!("counter= {}",counter);
    ecdh::key_pair_generate(None::<&mut RAND>, &mut sk, &mut pk);
    if kem==32 || kem==33 {
        reverse(&mut pk);
    }
    counter<256
}

#[allow(non_snake_case)]
pub fn encap(config_id: usize,skE: &[u8],z: &mut [u8],pkE: &[u8],pkR: &[u8]) {
    let pklen=pkE.len();
    let mut dh: [u8; ecdh::EFS] = [0; ecdh::EFS];
    let mut kemcontext: [u8; 2*POINT] = [0;2*POINT];
    let kem = config_id&255;
    let mut rev: [u8; POINT]=[0; POINT];

	if kem==32 || kem==33 {
        for i in 0..pklen {
            rev[i]=pkR[i];
        }
        reverse(&mut rev[0..pklen]);
	    ecdh::ecpsvdp_dh(&skE, &rev[0..pklen], &mut dh, 0);
		reverse(&mut dh[0..pklen]);
	} else {
	    ecdh::ecpsvdp_dh(&skE, &pkR, &mut dh, 0);
    }
    let mut k=0;
    for i in 0..pklen {
        kemcontext[k]=pkE[i];
        k+=1;
    }
    for i in 0..pklen {
        kemcontext[k]=pkR[i];
        k+=1;
    }
//print!("e dh= "); printbinary(&dh[0..pklen]);
    extractAndExpand(config_id,z,&dh,&kemcontext[0..k]);
}

#[allow(non_snake_case)]
pub fn decap(config_id: usize,skR: &[u8],z: &mut [u8],pkE: &[u8],pkR: &[u8]) {
    let pklen=pkE.len();
    let mut dh: [u8; ecdh::EFS] = [0; ecdh::EFS];
    let mut kemcontext: [u8; 2*POINT] = [0;2*POINT];
    let mut rev: [u8; POINT]=[0; POINT];
	let kem = config_id&255;

	if kem==32 || kem==33 {
        for i in 0..pklen {
            rev[i]=pkE[i];
        }
        reverse(&mut rev[0..pklen]);
	    ecdh::ecpsvdp_dh(&skR, &rev[0..pklen], &mut dh, 0);
		reverse(&mut dh[0..pklen]);
	} else {
	    ecdh::ecpsvdp_dh(&skR, &pkE, &mut dh, 0);
    }

    let mut k=0;
    for i in 0..pklen {
        kemcontext[k]=pkE[i];
        k+=1;
    }
    for i in 0..pklen {  // not a mistake
        kemcontext[k]=pkR[i];
        k+=1;
    }
//print!("d dh= "); printbinary(&dh[0..pklen]);
    extractAndExpand(config_id,z,&dh,&kemcontext[0..k]);
}

#[allow(non_snake_case)]
pub fn authencap(config_id: usize,skE: &[u8],skS: &[u8],z: &mut [u8],pkE: &[u8],pkR: &[u8],pkS: &[u8]) {
    let mut dh: [u8; 2*ecdh::EFS] = [0; 2*ecdh::EFS];
    let mut dh1: [u8; ecdh::EFS] = [0; ecdh::EFS];

    let mut kemcontext: [u8; 3*POINT] = [0;3*POINT];
	let kem = config_id&255;
    let pklen=pkE.len();
    let mut rev: [u8; POINT]=[0; POINT];


	if kem==32 || kem==33 {
        for i in 0..pklen {
            rev[i]=pkR[i];
        }
        reverse(&mut rev[0..pklen]);
        ecdh::ecpsvdp_dh(&skE, &rev[0..pklen], &mut dh, 0);
        ecdh::ecpsvdp_dh(&skS, &rev[0..pklen], &mut dh1, 0);
		reverse(&mut dh[0..pklen]);
		reverse(&mut dh1[0..pklen]);
	} else {
        ecdh::ecpsvdp_dh(&skE, &pkR, &mut dh, 0);
        ecdh::ecpsvdp_dh(&skS, &pkR, &mut dh1, 0);
    }

	for i in 0..ecdh::EFS {
		dh[i+ecdh::EFS] = dh1[i];
	}

    for i in 0..pklen {
        kemcontext[i]=pkE[i];
        kemcontext[pklen+i]= pkR[i];
        kemcontext[2*pklen+i]= pkS[i];
    }
//print!("e dh= "); printbinary(&dh[0..pklen]);
//print!("e kemcontext= "); printbinary(&kemcontext[0..3*pklen]);
    extractAndExpand(config_id,z,&dh,&kemcontext[0..3*pklen]);
}

#[allow(non_snake_case)]
pub fn authdecap(config_id: usize,skR: &[u8],z: &mut [u8],pkE: &[u8],pkR: &[u8],pkS: &[u8]) {
    let mut dh: [u8; 2*ecdh::EFS] = [0; 2*ecdh::EFS];
    let mut dh1: [u8; ecdh::EFS] = [0; ecdh::EFS];
    let mut kemcontext: [u8; 3*POINT] = [0;3*POINT];
	let kem = config_id&255;
    let pklen=pkE.len();
    let mut rev: [u8; POINT]=[0; POINT];

	if kem==32 || kem==33 {
        for i in 0..pklen {
            rev[i]=pkE[i];
        }
        reverse(&mut rev[0..pklen]);
        ecdh::ecpsvdp_dh(&skR, &rev[0..pklen], &mut dh, 0);
        for i in 0..pklen {
            rev[i]=pkS[i];
        }
        reverse(&mut rev[0..pklen]);
        ecdh::ecpsvdp_dh(&skR, &rev[0..pklen], &mut dh1, 0);
		reverse(&mut dh[0..pklen]);
		reverse(&mut dh1[0..pklen]);
	} else {
        ecdh::ecpsvdp_dh(&skR, &pkE, &mut dh, 0);
        ecdh::ecpsvdp_dh(&skR, &pkS, &mut dh1, 0);
    }

	for i in 0..ecdh::EFS {
		dh[i+ecdh::EFS] = dh1[i];
	}

    for i in 0..pklen {
        kemcontext[i]=pkE[i];
        kemcontext[pklen+i]= pkR[i];
        kemcontext[2*pklen+i]= pkS[i];
    }
//print!("d dh= "); printbinary(&dh[0..pklen]);
//print!("d kemcontext= "); printbinary(&kemcontext[0..3*pklen]);
    extractAndExpand(config_id,z,&dh,&kemcontext[0..3*pklen]);
}

#[allow(non_snake_case)]
pub fn keyschedule(config_id: usize,key: &mut [u8],nonce: &mut [u8],exp_secret: &mut [u8],mode: usize,z: &mut [u8],info: &[u8],psk: Option<&[u8]>,pskID: Option<&[u8]>) {

    let mut context: [u8; 1+2*ecp::HASH_TYPE] = [0; 1+2*ecp::HASH_TYPE];
	let kem=config_id&255;
	let kdf=(config_id>>8)&3;
	let aead=(config_id>>10)&3;

    let txt="HPKE";
    let ckem=txt.as_bytes();
    let mut suite_id: [u8;10] = [0;10];
    let mut num: [u8; 2] = [0; 2];

    let mut k=0;
    for i in 0..ckem.len() {
        suite_id[k]=ckem[i];
        k+=1;
    }
    hmac::inttobytes(kem,&mut num);
    suite_id[k]=num[0]; k+=1;
    suite_id[k]=num[1]; k+=1;
    hmac::inttobytes(kdf,&mut num);
    suite_id[k]=num[0]; k+=1;
    suite_id[k]=num[1]; k+=1;
    hmac::inttobytes(aead,&mut num);
    suite_id[k]=num[0]; k+=1;
    suite_id[k]=num[1];

    let mut k=0;
    let mut h: [u8; 64] = [0; 64];
    let mut secret: [u8; 64] = [0; 64];

    context[k]=mode as u8; k+=1;

    labeledExtract(&mut h,None,&suite_id,"psk_id_hash",pskID);
    for i in 0..ecp::HASH_TYPE {
		context[k] = h[i]; k+=1;
    }
    labeledExtract(&mut h,None,&suite_id,"info_hash",Some(&info));
    for i in 0..ecp::HASH_TYPE {
		context[k] = h[i]; k+=1;
    }

    //labeledExtract(&mut h,None,&suite_id,"psk_hash",psk);

    //labeledExtract(&mut secret,Some(&h),&suite_id,"secret",Some(z));

    labeledExtract(&mut secret,Some(z),&suite_id,"secret",psk);

    labeledExpand(key,&secret,&suite_id,"key",Some(&context[0..k]),ecp::AESKEY);
    labeledExpand(nonce,&secret,&suite_id,"base_nonce",Some(&context[0..k]),12);
    labeledExpand(exp_secret,&secret,&suite_id,"exp",Some(&context[0..k]),ecp::HASH_TYPE);
}
