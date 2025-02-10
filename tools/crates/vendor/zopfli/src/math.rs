#[allow(dead_code)] // False positive

/// Provides math operations for doubles on `no_std` targets that are not available on `core`.
pub trait F64MathExt {
    /// Computes the absolute value of `self`.
    #[must_use = "method returns a new number and does not mutate the original value"]
    fn abs(self) -> Self;

    /// Returns the natural logarithm of the number.
    ///
    /// This method is allowed to return different values for 0 and negative numbers
    /// than its counterpart on `std` on some exotic platforms.
    #[must_use = "method returns a new number and does not mutate the original value"]
    fn ln(self) -> Self;
}

impl F64MathExt for f64 {
    #[inline]
    fn abs(self) -> Self {
        f64::from_bits(self.to_bits() & ((1u64 << 63) - 1))
    }

    fn ln(self) -> Self {
        // This implementation is not numerically precise at all, but is good enough
        // for our purposes still: the core_intrinsics feature is discouraged, and
        // on e.g. x86 platforms they compile down to a libm call anyway, which can
        // be troublesome for freestanding targets
        logf(self as f32) as f64
    }
}

// Function taken from:
// https://github.com/rust-lang/libm/blob/a0a5bd85c913fe5f6c42bc11ef292e575f97ca6d/src/math/logf.rs
// See the link above for author and licensing information of this code snippet
#[allow(clippy::eq_op, clippy::excessive_precision)]
fn logf(mut x: f32) -> f32 {
    const LN2_HI: f32 = core::f32::consts::LN_2; /* 0x3f317180 */
    const LN2_LO: f32 = 9.0580006145e-06; /* 0x3717f7d1 */
    /* |(log(1+s)-log(1-s))/s - Lg(s)| < 2**-34.24 (~[-4.95e-11, 4.97e-11]). */
    const LG1: f32 = 0.66666662693; /*  0xaaaaaa.0p-24*/
    const LG2: f32 = 0.40000972152; /*  0xccce13.0p-25 */
    const LG3: f32 = 0.28498786688; /*  0x91e9ee.0p-25 */
    const LG4: f32 = 0.24279078841; /*  0xf89e26.0p-26 */

    let x1p25 = f32::from_bits(0x4c000000); // 0x1p25f === 2 ^ 25

    let mut ix = x.to_bits();
    let mut k = 0i32;

    if (ix < 0x00800000) || ((ix >> 31) != 0) {
        /* x < 2**-126  */
        if ix << 1 == 0 {
            return -1. / (x * x); /* log(+-0)=-inf */
        }
        if (ix >> 31) != 0 {
            return (x - x) / 0.; /* log(-#) = NaN */
        }
        /* subnormal number, scale up x */
        k -= 25;
        x *= x1p25;
        ix = x.to_bits();
    } else if ix >= 0x7f800000 {
        return x;
    } else if ix == 0x3f800000 {
        return 0.;
    }

    /* reduce x into [sqrt(2)/2, sqrt(2)] */
    ix += 0x3f800000 - 0x3f3504f3;
    k += ((ix >> 23) as i32) - 0x7f;
    ix = (ix & 0x007fffff) + 0x3f3504f3;
    x = f32::from_bits(ix);

    let f = x - 1.;
    let s = f / (2. + f);
    let z = s * s;
    let w = z * z;
    let t1 = w * (LG2 + w * LG4);
    let t2 = z * (LG1 + w * LG3);
    let r = t2 + t1;
    let hfsq = 0.5 * f * f;
    let dk = k as f32;
    s * (hfsq + r) + dk * LN2_LO - hfsq + f + dk * LN2_HI
}
