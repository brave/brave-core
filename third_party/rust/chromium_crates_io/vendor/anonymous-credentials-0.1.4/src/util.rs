use brave_miracl::{
    bn254::{
        big::BIG,
        ecp::ECP,
        ecp2::ECP2,
        fp12::FP12,
        fp2::FP2,
        pair::{another, fexp, initmp, miller},
        rom::{CURVE_GX, CURVE_GY, CURVE_ORDER, CURVE_PXA, CURVE_PXB, CURVE_PYA, CURVE_PYB},
    },
    hash256::HASH256,
    rand::RAND,
};
use lazy_static::lazy_static;

use super::data::{BIG_SIZE, ECP_SIZE};

lazy_static! {
    pub static ref G1_ECP: ECP = {
        let gx = BIG::new_ints(&CURVE_GX);
        let gy = BIG::new_ints(&CURVE_GY);
        ECP::new_bigs(&gx, &gy)
    };
    pub static ref G2_ECP: ECP2 = {
        let pxa = BIG::new_ints(&CURVE_PXA);
        let pxb = BIG::new_ints(&CURVE_PXB);
        let pya = BIG::new_ints(&CURVE_PYA);
        let pyb = BIG::new_ints(&CURVE_PYB);
        let wx = FP2::new_bigs(&pxa, &pxb);
        let wy = FP2::new_bigs(&pya, &pyb);
        ECP2::new_fp2s(&wx, &wy)
    };
    pub static ref CURVE_ORDER_BIG: BIG = BIG::new_ints(&CURVE_ORDER);
}

pub fn random_mod_curve_order(rng: &mut RAND) -> BIG {
    BIG::randomnum(&CURVE_ORDER_BIG, rng)
}

pub fn hash256(data: &[u8]) -> [u8; 32] {
    let mut hash = HASH256::new();
    hash.process_array(data);
    hash.hash()
}

pub fn pair_normalized_triple_ate(p: &ECP2, q: &ECP, r: &ECP2, s: &ECP, t: &ECP2, u: &ECP) -> FP12 {
    let mut rr = initmp();
    another(&mut rr, p, q);
    another(&mut rr, r, s);
    another(&mut rr, t, u);
    let r = miller(&mut rr);
    fexp(&r)
}

pub fn ecp_challenge_equals(
    message: Option<&[u8; BIG_SIZE]>,
    y: &ECP,
    z: &ECP,
    a: &ECP,
    b: &ECP,
    ar: &ECP,
    br: &ECP,
) -> BIG {
    let (mut all_bytes, msg_len) = match message {
        Some(message) => {
            let mut all_bytes = vec![0u8; ECP_SIZE * 6 + BIG_SIZE];
            all_bytes[..BIG_SIZE].copy_from_slice(message);
            (all_bytes, message.len())
        }
        None => (vec![0u8; ECP_SIZE * 6], 0),
    };

    y.tobytes(&mut all_bytes[msg_len..], false);
    z.tobytes(&mut all_bytes[msg_len + ECP_SIZE..], false);
    a.tobytes(&mut all_bytes[msg_len + ECP_SIZE * 2..], false);
    b.tobytes(&mut all_bytes[msg_len + ECP_SIZE * 3..], false);
    ar.tobytes(&mut all_bytes[msg_len + ECP_SIZE * 4..], false);
    br.tobytes(&mut all_bytes[msg_len + ECP_SIZE * 5..], false);

    let hash = hash256(&all_bytes);

    let mut c = BIG::frombytes(&hash);
    c.rmod(&CURVE_ORDER_BIG);
    c
}
