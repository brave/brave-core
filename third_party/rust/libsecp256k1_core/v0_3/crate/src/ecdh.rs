use crate::{
    ecmult::ECMultContext,
    group::{Affine, Jacobian},
    scalar::Scalar,
};
use digest::{generic_array::GenericArray, Digest};

impl ECMultContext {
    pub fn ecdh_raw<D: Digest + Default>(
        &self,
        point: &Affine,
        scalar: &Scalar,
    ) -> Option<GenericArray<u8, D::OutputSize>> {
        let mut digest: D = Default::default();

        let mut pt = *point;
        let s = *scalar;

        if s.is_zero() {
            return None;
        }

        let mut res = Jacobian::default();
        self.ecmult_const(&mut res, &pt, &s);
        pt.set_gej(&res);

        pt.x.normalize();
        pt.y.normalize();

        let x = pt.x.b32();
        let y = 0x02 | (if pt.y.is_odd() { 1 } else { 0 });

        digest.update(&[y]);
        digest.update(&x);
        Some(digest.finalize_reset())
    }
}
