//! This module implements "simplified SWU" hashing to short Weierstrass curves
//! with a = 0.

use ff::{Field, FromUniformBytes, PrimeField};
use static_assertions::const_assert;
use subtle::ConstantTimeEq;

use crate::arithmetic::CurveExt;

/// Hashes over a message and writes the output to all of `buf`.
pub fn hash_to_field<F: FromUniformBytes<64>>(
    curve_id: &str,
    domain_prefix: &str,
    message: &[u8],
    buf: &mut [F; 2],
) {
    assert!(domain_prefix.len() < 256);
    assert!((22 + curve_id.len() + domain_prefix.len()) < 256);

    // Assume that the field size is 32 bytes and k is 256, where k is defined in
    // <https://www.ietf.org/archive/id/draft-irtf-cfrg-hash-to-curve-10.html#name-security-considerations-3>.
    const CHUNKLEN: usize = 64;
    const_assert!(CHUNKLEN * 2 < 256);

    // Input block size of BLAKE2b.
    const R_IN_BYTES: usize = 128;

    let personal = [0u8; 16];
    let empty_hasher = blake2b_simd::Params::new()
        .hash_length(CHUNKLEN)
        .personal(&personal)
        .to_state();

    let b_0 = empty_hasher
        .clone()
        .update(&[0; R_IN_BYTES])
        .update(message)
        .update(&[0, (CHUNKLEN * 2) as u8, 0])
        .update(domain_prefix.as_bytes())
        .update(b"-")
        .update(curve_id.as_bytes())
        .update(b"_XMD:BLAKE2b_SSWU_RO_")
        .update(&[(22 + curve_id.len() + domain_prefix.len()) as u8])
        .finalize();

    let b_1 = empty_hasher
        .clone()
        .update(b_0.as_array())
        .update(&[1])
        .update(domain_prefix.as_bytes())
        .update(b"-")
        .update(curve_id.as_bytes())
        .update(b"_XMD:BLAKE2b_SSWU_RO_")
        .update(&[(22 + curve_id.len() + domain_prefix.len()) as u8])
        .finalize();

    let b_2 = {
        let mut empty_hasher = empty_hasher;
        for (l, r) in b_0.as_array().iter().zip(b_1.as_array().iter()) {
            empty_hasher.update(&[*l ^ *r]);
        }
        empty_hasher
            .update(&[2])
            .update(domain_prefix.as_bytes())
            .update(b"-")
            .update(curve_id.as_bytes())
            .update(b"_XMD:BLAKE2b_SSWU_RO_")
            .update(&[(22 + curve_id.len() + domain_prefix.len()) as u8])
            .finalize()
    };

    for (big, buf) in [b_1, b_2].iter().zip(buf.iter_mut()) {
        let mut little = [0u8; CHUNKLEN];
        little.copy_from_slice(big.as_array());
        little.reverse();
        *buf = F::from_uniform_bytes(&little);
    }
}

/// Implements a degree 3 isogeny map.
pub fn iso_map<F: Field, C: CurveExt<Base = F>, I: CurveExt<Base = F>>(
    p: &I,
    iso: &[C::Base; 13],
) -> C {
    // The input and output are in Jacobian coordinates, using the method
    // in "Avoiding inversions" [WB2019, section 4.3].

    let (x, y, z) = p.jacobian_coordinates();

    let z2 = z.square();
    let z3 = z2 * z;
    let z4 = z2.square();
    let z6 = z3.square();

    let num_x = ((iso[0] * x + iso[1] * z2) * x + iso[2] * z4) * x + iso[3] * z6;
    let div_x = (z2 * x + iso[4] * z4) * x + iso[5] * z6;

    let num_y = (((iso[6] * x + iso[7] * z2) * x + iso[8] * z4) * x + iso[9] * z6) * y;
    let div_y = (((x + iso[10] * z2) * x + iso[11] * z4) * x + iso[12] * z6) * z3;

    let zo = div_x * div_y;
    let xo = num_x * div_y * zo;
    let yo = num_y * div_x * zo.square();

    C::new_jacobian(xo, yo, zo).unwrap()
}

#[allow(clippy::many_single_char_names)]
pub fn map_to_curve_simple_swu<F: PrimeField, C: CurveExt<Base = F>, I: CurveExt<Base = F>>(
    u: &F,
    theta: F,
    z: F,
) -> I {
    // 1. tv1 = inv0(Z^2 * u^4 + Z * u^2)
    // 2. x1 = (-B / A) * (1 + tv1)
    // 3. If tv1 == 0, set x1 = B / (Z * A)
    // 4. gx1 = x1^3 + A * x1 + B
    //
    // We use the "Avoiding inversions" optimization in [WB2019, section 4.2]
    // (not to be confused with section 4.3):
    //
    //   here       [WB2019]
    //   -------    ---------------------------------
    //   Z          ξ
    //   u          t
    //   Z * u^2    ξ * t^2 (called u, confusingly)
    //   x1         X_0(t)
    //   x2         X_1(t)
    //   gx1        g(X_0(t))
    //   gx2        g(X_1(t))
    //
    // Using the "here" names:
    //    x1 = num_x1/div      = [B*(Z^2 * u^4 + Z * u^2 + 1)] / [-A*(Z^2 * u^4 + Z * u^2]
    //   gx1 = num_gx1/div_gx1 = [num_x1^3 + A * num_x1 * div^2 + B * div^3] / div^3

    let a = I::a();
    let b = I::b();
    let z_u2 = z * u.square();
    let ta = z_u2.square() + z_u2;
    let num_x1 = b * (ta + F::ONE);
    let div = a * F::conditional_select(&-ta, &z, ta.is_zero());
    let num2_x1 = num_x1.square();
    let div2 = div.square();
    let div3 = div2 * div;
    let num_gx1 = (num2_x1 + a * div2) * num_x1 + b * div3;

    // 5. x2 = Z * u^2 * x1
    let num_x2 = z_u2 * num_x1; // same div

    // 6. gx2 = x2^3 + A * x2 + B  [optimized out; see below]
    // 7. If is_square(gx1), set x = x1 and y = sqrt(gx1)
    // 8. Else set x = x2 and y = sqrt(gx2)
    let (gx1_square, y1) = F::sqrt_ratio(&num_gx1, &div3);

    // This magic also comes from a generalization of [WB2019, section 4.2].
    //
    // The Sarkar square root algorithm with input s gives us a square root of
    // h * s for free when s is not square, where h is a fixed nonsquare.
    // In our implementation, h = ROOT_OF_UNITY.
    // We know that Z / h is a square since both Z and h are
    // nonsquares. Precompute theta as a square root of Z / ROOT_OF_UNITY.
    //
    // We have gx2 = g(Z * u^2 * x1) = Z^3 * u^6 * gx1
    //                               = (Z * u^3)^2 * (Z/h * h * gx1)
    //                               = (Z * theta * u^3)^2 * (h * gx1)
    //
    // When gx1 is not square, y1 is a square root of h * gx1, and so Z * theta * u^3 * y1
    // is a square root of gx2. Note that we don't actually need to compute gx2.

    let y2 = theta * z_u2 * u * y1;
    let num_x = F::conditional_select(&num_x2, &num_x1, gx1_square);
    let y = F::conditional_select(&y2, &y1, gx1_square);

    // 9. If sgn0(u) != sgn0(y), set y = -y
    let y = F::conditional_select(&(-y), &y, u.is_odd().ct_eq(&y.is_odd()));

    I::new_jacobian(num_x * div, y * div3, div).unwrap()
}
