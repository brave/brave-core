use brave_miracl::{
    bn254::{big::BIG, ecp::ECP, pair::g1mul},
    rand::RAND,
};

use super::{
    data::{CredentialBIG, ECPProof, Signature, UserCredentials, BIG_SIZE},
    util::{ecp_challenge_equals, hash256, random_mod_curve_order, CURVE_ORDER_BIG},
};

fn make_ecp_proof_equals(
    rng: &mut RAND,
    message: &[u8; BIG_SIZE],
    a: &ECP,
    b: &ECP,
    y: &ECP,
    z: &ECP,
    x: &BIG,
) -> ECPProof {
    let r = random_mod_curve_order(rng);

    let ar = g1mul(&a, &r);
    let br = g1mul(&b, &r);

    let c = ecp_challenge_equals(Some(message), y, z, a, b, &ar, &br);
    let mut s = BIG::modmul(&c, x, &CURVE_ORDER_BIG);
    s.add(&r);
    s.rmod(&CURVE_ORDER_BIG);

    ECPProof { c, s }
}

pub fn sign(
    rng: &mut RAND,
    gsk: &CredentialBIG,
    credentials: &UserCredentials,
    msg: &[u8],
    bsn: &[u8],
) -> Signature {
    // Randomize credentials for signature
    let r = random_mod_curve_order(rng);

    let a = g1mul(&credentials.a, &r);
    let b = g1mul(&credentials.b, &r);
    let c = g1mul(&credentials.c, &r);
    let d = g1mul(&credentials.d, &r);

    // Map basename to point in G1
    let bsn_hash = hash256(bsn);
    let bsn_point = ECP::mapit(&bsn_hash);
    let nym = g1mul(&bsn_point, &gsk.0);

    // Compute H(H(msg) || H(bsn)) to be used in proof of equality
    let mut msg_bsn_hash_data = [0u8; BIG_SIZE * 2];
    msg_bsn_hash_data[..BIG_SIZE].copy_from_slice(&hash256(msg));
    msg_bsn_hash_data[BIG_SIZE..].copy_from_slice(&hash256(bsn));
    let msg_bsn_hash = hash256(&msg_bsn_hash_data);

    let proof = make_ecp_proof_equals(rng, &msg_bsn_hash, &b, &bsn_point, &d, &nym, &gsk.0);
    Signature {
        a,
        b,
        c,
        d,
        nym,
        proof,
    }
}
