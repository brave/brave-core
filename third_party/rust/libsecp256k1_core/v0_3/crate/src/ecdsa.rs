use crate::{
    ecmult::{ECMultContext, ECMultGenContext},
    field::Field,
    group::{Affine, Jacobian},
    scalar::Scalar,
    Error,
};

const P_MINUS_ORDER: Field = Field::new(0, 0, 0, 1, 0x45512319, 0x50B75FC4, 0x402DA172, 0x2FC9BAEE);

const ORDER_AS_FE: Field = Field::new(
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFE, 0xBAAEDCE6, 0xAF48A03B, 0xBFD25E8C, 0xD0364141,
);

impl ECMultContext {
    pub fn verify_raw(
        &self,
        sigr: &Scalar,
        sigs: &Scalar,
        pubkey: &Affine,
        message: &Scalar,
    ) -> bool {
        let c;
        let (sn, u1, u2): (Scalar, Scalar, Scalar);

        if sigr.is_zero() || sigs.is_zero() {
            return false;
        }

        sn = sigs.inv_var();
        u1 = &sn * message;
        u2 = &sn * sigr;
        let mut pubkeyj: Jacobian = Jacobian::default();
        pubkeyj.set_ge(pubkey);
        let mut pr: Jacobian = Jacobian::default();
        self.ecmult(&mut pr, &pubkeyj, &u2, &u1);
        if pr.is_infinity() {
            return false;
        }

        c = sigr.b32();
        let mut xr: Field = Default::default();
        let _ = xr.set_b32(&c);

        if pr.eq_x_var(&xr) {
            return true;
        }
        if xr >= P_MINUS_ORDER {
            return false;
        }
        xr += ORDER_AS_FE;
        if pr.eq_x_var(&xr) {
            return true;
        }
        false
    }

    pub fn recover_raw(
        &self,
        sigr: &Scalar,
        sigs: &Scalar,
        rec_id: u8,
        message: &Scalar,
    ) -> Result<Affine, Error> {
        debug_assert!(rec_id < 4);

        if sigr.is_zero() || sigs.is_zero() {
            return Err(Error::InvalidSignature);
        }

        let brx = sigr.b32();
        let mut fx = Field::default();
        let overflow = fx.set_b32(&brx);
        debug_assert!(overflow);

        if rec_id & 2 > 0 {
            if fx >= P_MINUS_ORDER {
                return Err(Error::InvalidSignature);
            }
            fx += ORDER_AS_FE;
        }
        let mut x = Affine::default();
        if !x.set_xo_var(&fx, rec_id & 1 > 0) {
            return Err(Error::InvalidSignature);
        }
        let mut xj = Jacobian::default();
        xj.set_ge(&x);
        let rn = sigr.inv();
        let mut u1 = &rn * message;
        u1 = -u1;
        let u2 = &rn * sigs;
        let mut qj = Jacobian::default();
        self.ecmult(&mut qj, &xj, &u2, &u1);

        let mut pubkey = Affine::default();
        pubkey.set_gej_var(&qj);

        if pubkey.is_infinity() {
            Err(Error::InvalidSignature)
        } else {
            Ok(pubkey)
        }
    }
}

impl ECMultGenContext {
    pub fn sign_raw(
        &self,
        seckey: &Scalar,
        message: &Scalar,
        nonce: &Scalar,
    ) -> Result<(Scalar, Scalar, u8), Error> {
        let mut rp = Jacobian::default();
        self.ecmult_gen(&mut rp, nonce);
        let mut r = Affine::default();
        r.set_gej(&rp);
        r.x.normalize();
        r.y.normalize();
        let b = r.x.b32();
        let mut sigr = Scalar::default();
        let overflow = bool::from(sigr.set_b32(&b));
        debug_assert!(!sigr.is_zero());
        debug_assert!(!overflow);

        let mut recid = (if overflow { 2 } else { 0 }) | (if r.y.is_odd() { 1 } else { 0 });
        let mut n = &sigr * seckey;
        n += message;
        let mut sigs = nonce.inv();
        sigs *= &n;
        n.clear();
        rp.clear();
        r.clear();
        if sigs.is_zero() {
            return Err(Error::InvalidMessage);
        }
        if sigs.is_high() {
            sigs = -sigs;
            recid ^= 1;
        }
        Ok((sigr, sigs, recid))
    }
}
