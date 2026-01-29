#[allow(
    clippy::cognitive_complexity,
    clippy::float_cmp,
    clippy::neg_cmp_op_on_partial_ord
)]
#[cfg(test)]
mod test {
    #[allow(unused_imports)]
    use core::cmp::Ordering;
    use float8::F8E4M3;
    #[cfg(feature = "num-traits")]
    use num_traits::{AsPrimitive, FromPrimitive, ToPrimitive};

    #[cfg(feature = "num-traits")]
    #[test]
    fn as_primitive() {
        let two = F8E4M3::from_f32(2.0);
        assert_eq!(<i32 as AsPrimitive<F8E4M3>>::as_(2), two);
        assert_eq!(<F8E4M3 as AsPrimitive<i32>>::as_(two), 2);

        assert_eq!(<f32 as AsPrimitive<F8E4M3>>::as_(2.0), two);
        assert_eq!(<F8E4M3 as AsPrimitive<f32>>::as_(two), 2.0);

        assert_eq!(<f64 as AsPrimitive<F8E4M3>>::as_(2.0), two);
        assert_eq!(<F8E4M3 as AsPrimitive<f64>>::as_(two), 2.0);
    }

    #[cfg(feature = "num-traits")]
    #[test]
    fn to_primitive() {
        let two = F8E4M3::from_f32(2.0);
        assert_eq!(ToPrimitive::to_i32(&two).unwrap(), 2i32);
        assert_eq!(ToPrimitive::to_f32(&two).unwrap(), 2.0f32);
        assert_eq!(ToPrimitive::to_f64(&two).unwrap(), 2.0f64);
    }

    #[cfg(feature = "num-traits")]
    #[test]
    fn from_primitive() {
        let two = F8E4M3::from_f32(2.0);
        assert_eq!(<F8E4M3 as FromPrimitive>::from_i32(2).unwrap(), two);
        assert_eq!(<F8E4M3 as FromPrimitive>::from_f32(2.0).unwrap(), two);
        assert_eq!(<F8E4M3 as FromPrimitive>::from_f64(2.0).unwrap(), two);
    }

    #[test]
    fn test_f8e4m3_consts() {
        // DIGITS
        let digits = ((F8E4M3::MANTISSA_DIGITS as f32 - 1.0) * 2f32.log10()).floor() as u32;
        assert_eq!(F8E4M3::DIGITS, digits);
        // sanity check to show test is good
        let digits32 = ((f32::MANTISSA_DIGITS as f32 - 1.0) * 2f32.log10()).floor() as u32;
        assert_eq!(f32::DIGITS, digits32);

        // EPSILON
        let one = F8E4M3::from_f32(1.0);
        let one_plus_epsilon = F8E4M3::from_bits(one.to_bits() + 1);
        let epsilon = F8E4M3::from_f32(one_plus_epsilon.to_f32() - 1.0);
        assert_eq!(F8E4M3::EPSILON, epsilon);
        // sanity check to show test is good
        let one_plus_epsilon32 = f32::from_bits(1.0f32.to_bits() + 1);
        let epsilon32 = one_plus_epsilon32 - 1f32;
        assert_eq!(f32::EPSILON, epsilon32);

        // MAX, MIN and MIN_POSITIVE
        let max = F8E4M3::from_bits(F8E4M3::INFINITY.to_bits() - 1);
        let min = F8E4M3::from_bits(F8E4M3::NEG_INFINITY.to_bits() - 1);
        let min_pos = F8E4M3::from_f32(2f32.powi(F8E4M3::MIN_EXP - 1));
        assert_eq!(F8E4M3::MAX, max);
        assert_eq!(F8E4M3::MIN, min);
        assert_eq!(F8E4M3::MIN_POSITIVE, min_pos);
        // sanity check to show test is good
        let max32 = f32::from_bits(f32::INFINITY.to_bits() - 1);
        let min32 = f32::from_bits(f32::NEG_INFINITY.to_bits() - 1);
        let min_pos32 = 2f32.powi(f32::MIN_EXP - 1);
        assert_eq!(f32::MAX, max32);
        assert_eq!(f32::MIN, min32);
        assert_eq!(f32::MIN_POSITIVE, min_pos32);

        // MIN_10_EXP and MAX_10_EXP
        let ten_to_min = 10f32.powi(F8E4M3::MIN_10_EXP);
        assert!(ten_to_min / 10.0 < F8E4M3::MIN_POSITIVE.to_f32());
        assert!(ten_to_min > F8E4M3::MIN_POSITIVE.to_f32());
        let ten_to_max = 10f32.powi(F8E4M3::MAX_10_EXP);
        assert!(ten_to_max < F8E4M3::MAX.to_f32());
        assert!(ten_to_max * 10.0 > F8E4M3::MAX.to_f32());
        // sanity check to show test is good
        let ten_to_min32 = 10f64.powi(f32::MIN_10_EXP);
        assert!(ten_to_min32 / 10.0 < f64::from(f32::MIN_POSITIVE));
        assert!(ten_to_min32 > f64::from(f32::MIN_POSITIVE));
        let ten_to_max32 = 10f64.powi(f32::MAX_10_EXP);
        assert!(ten_to_max32 < f64::from(f32::MAX));
        assert!(ten_to_max32 * 10.0 > f64::from(f32::MAX));
    }

    #[test]
    fn test_f8e4m3_consts_from_f32() {
        let one = F8E4M3::from_f32(1.0);
        let zero = F8E4M3::from_f32(0.0);
        let neg_zero = F8E4M3::from_f32(-0.0);
        let neg_one = F8E4M3::from_f32(-1.0);
        let inf = F8E4M3::from_f32(f32::INFINITY);
        let neg_inf = F8E4M3::from_f32(f32::NEG_INFINITY);
        let nan = F8E4M3::from_f32(f32::NAN);

        assert_eq!(F8E4M3::ONE, one);
        assert_eq!(F8E4M3::ZERO, zero);
        assert!(zero.is_sign_positive());
        assert_eq!(F8E4M3::NEG_ZERO, neg_zero);
        assert!(neg_zero.is_sign_negative());
        assert_eq!(F8E4M3::NEG_ONE, neg_one);
        assert!(neg_one.is_sign_negative());
        assert_eq!(F8E4M3::INFINITY, inf);
        assert_eq!(F8E4M3::NEG_INFINITY, neg_inf);
        assert!(inf.is_infinite());
        assert!(F8E4M3::INFINITY.is_infinite());
        assert!(neg_inf.is_infinite());
        assert!(F8E4M3::NEG_INFINITY.is_infinite());
        assert!(nan.is_nan());
        assert!(F8E4M3::NAN.is_nan());

        let e = F8E4M3::from_f32(std::f32::consts::E);
        let pi = F8E4M3::from_f32(std::f32::consts::PI);
        let frac_1_pi = F8E4M3::from_f32(std::f32::consts::FRAC_1_PI);
        let frac_1_sqrt_2 = F8E4M3::from_f32(std::f32::consts::FRAC_1_SQRT_2);
        let frac_2_pi = F8E4M3::from_f32(std::f32::consts::FRAC_2_PI);
        let frac_2_sqrt_pi = F8E4M3::from_f32(std::f32::consts::FRAC_2_SQRT_PI);
        let frac_pi_2 = F8E4M3::from_f32(std::f32::consts::FRAC_PI_2);
        let frac_pi_3 = F8E4M3::from_f32(std::f32::consts::FRAC_PI_3);
        let frac_pi_4 = F8E4M3::from_f32(std::f32::consts::FRAC_PI_4);
        let frac_pi_6 = F8E4M3::from_f32(std::f32::consts::FRAC_PI_6);
        let frac_pi_8 = F8E4M3::from_f32(std::f32::consts::FRAC_PI_8);
        let ln_10 = F8E4M3::from_f32(std::f32::consts::LN_10);
        let ln_2 = F8E4M3::from_f32(std::f32::consts::LN_2);
        let log10_e = F8E4M3::from_f32(std::f32::consts::LOG10_E);
        // std::f32::consts::LOG10_2 requires rustc 1.43.0
        let log10_2 = F8E4M3::from_f32(2f32.log10());
        let log2_e = F8E4M3::from_f32(std::f32::consts::LOG2_E);
        // std::f32::consts::LOG2_10 requires rustc 1.43.0
        let log2_10 = F8E4M3::from_f32(10f32.log2());
        let sqrt_2 = F8E4M3::from_f32(std::f32::consts::SQRT_2);

        assert_eq!(F8E4M3::E, e);
        assert_eq!(F8E4M3::PI, pi);
        assert_eq!(F8E4M3::FRAC_1_PI, frac_1_pi);
        assert_eq!(F8E4M3::FRAC_1_SQRT_2, frac_1_sqrt_2);
        assert_eq!(F8E4M3::FRAC_2_PI, frac_2_pi);
        assert_eq!(F8E4M3::FRAC_2_SQRT_PI, frac_2_sqrt_pi);
        assert_eq!(F8E4M3::FRAC_PI_2, frac_pi_2);
        assert_eq!(F8E4M3::FRAC_PI_3, frac_pi_3);
        assert_eq!(F8E4M3::FRAC_PI_4, frac_pi_4);
        assert_eq!(F8E4M3::FRAC_PI_6, frac_pi_6);
        assert_eq!(F8E4M3::FRAC_PI_8, frac_pi_8);
        assert_eq!(F8E4M3::LN_10, ln_10);
        assert_eq!(F8E4M3::LN_2, ln_2);
        assert_eq!(F8E4M3::LOG10_E, log10_e);
        assert_eq!(F8E4M3::LOG10_2, log10_2);
        assert_eq!(F8E4M3::LOG2_E, log2_e);
        assert_eq!(F8E4M3::LOG2_10, log2_10);
        assert_eq!(F8E4M3::SQRT_2, sqrt_2);
    }

    #[test]
    fn test_f8e4m3_consts_from_f64() {
        let one = F8E4M3::from_f64(1.0);
        let zero = F8E4M3::from_f64(0.0);
        let neg_zero = F8E4M3::from_f64(-0.0);
        let inf = F8E4M3::from_f64(f64::INFINITY);
        let neg_inf = F8E4M3::from_f64(f64::NEG_INFINITY);
        let nan = F8E4M3::from_f64(f64::NAN);

        assert_eq!(F8E4M3::ONE, one);
        assert_eq!(F8E4M3::ZERO, zero);
        assert!(zero.is_sign_positive());
        assert_eq!(F8E4M3::NEG_ZERO, neg_zero);
        assert!(neg_zero.is_sign_negative());
        assert_eq!(F8E4M3::INFINITY, inf);
        assert_eq!(F8E4M3::NEG_INFINITY, neg_inf);
        assert!(nan.is_nan());
        assert!(F8E4M3::NAN.is_nan());

        let e = F8E4M3::from_f64(std::f64::consts::E);
        let pi = F8E4M3::from_f64(std::f64::consts::PI);
        let frac_1_pi = F8E4M3::from_f64(std::f64::consts::FRAC_1_PI);
        let frac_1_sqrt_2 = F8E4M3::from_f64(std::f64::consts::FRAC_1_SQRT_2);
        let frac_2_pi = F8E4M3::from_f64(std::f64::consts::FRAC_2_PI);
        let frac_2_sqrt_pi = F8E4M3::from_f64(std::f64::consts::FRAC_2_SQRT_PI);
        let frac_pi_2 = F8E4M3::from_f64(std::f64::consts::FRAC_PI_2);
        let frac_pi_3 = F8E4M3::from_f64(std::f64::consts::FRAC_PI_3);
        let frac_pi_4 = F8E4M3::from_f64(std::f64::consts::FRAC_PI_4);
        let frac_pi_6 = F8E4M3::from_f64(std::f64::consts::FRAC_PI_6);
        let frac_pi_8 = F8E4M3::from_f64(std::f64::consts::FRAC_PI_8);
        let ln_10 = F8E4M3::from_f64(std::f64::consts::LN_10);
        let ln_2 = F8E4M3::from_f64(std::f64::consts::LN_2);
        let log10_e = F8E4M3::from_f64(std::f64::consts::LOG10_E);
        // std::f64::consts::LOG10_2 requires rustc 1.43.0
        let log10_2 = F8E4M3::from_f64(2f64.log10());
        let log2_e = F8E4M3::from_f64(std::f64::consts::LOG2_E);
        // std::f64::consts::LOG2_10 requires rustc 1.43.0
        let log2_10 = F8E4M3::from_f64(10f64.log2());
        let sqrt_2 = F8E4M3::from_f64(std::f64::consts::SQRT_2);

        assert_eq!(F8E4M3::E, e);
        assert_eq!(F8E4M3::PI, pi);
        assert_eq!(F8E4M3::FRAC_1_PI, frac_1_pi);
        assert_eq!(F8E4M3::FRAC_1_SQRT_2, frac_1_sqrt_2);
        assert_eq!(F8E4M3::FRAC_2_PI, frac_2_pi);
        assert_eq!(F8E4M3::FRAC_2_SQRT_PI, frac_2_sqrt_pi);
        assert_eq!(F8E4M3::FRAC_PI_2, frac_pi_2);
        assert_eq!(F8E4M3::FRAC_PI_3, frac_pi_3);
        assert_eq!(F8E4M3::FRAC_PI_4, frac_pi_4);
        assert_eq!(F8E4M3::FRAC_PI_6, frac_pi_6);
        assert_eq!(F8E4M3::FRAC_PI_8, frac_pi_8);
        assert_eq!(F8E4M3::LN_10, ln_10);
        assert_eq!(F8E4M3::LN_2, ln_2);
        assert_eq!(F8E4M3::LOG10_E, log10_e);
        assert_eq!(F8E4M3::LOG10_2, log10_2);
        assert_eq!(F8E4M3::LOG2_E, log2_e);
        assert_eq!(F8E4M3::LOG2_10, log2_10);
        assert_eq!(F8E4M3::SQRT_2, sqrt_2);
    }

    #[test]
    fn test_nan_conversion_to_smaller() {
        let nan64 = f64::from_bits(0x7FF0_0000_0000_0001u64);
        let neg_nan64 = f64::from_bits(0xFFF0_0000_0000_0001u64);
        let nan32 = f32::from_bits(0x7F80_0001u32);
        let neg_nan32 = f32::from_bits(0xFF80_0001u32);
        let nan32_from_64 = nan64 as f32;
        let neg_nan32_from_64 = neg_nan64 as f32;
        let nan8_from_64 = F8E4M3::from_f64(nan64);
        let neg_nan8_from_64 = F8E4M3::from_f64(neg_nan64);
        let nan8_from_32 = F8E4M3::from_f32(nan32);
        let neg_nan8_from_32 = F8E4M3::from_f32(neg_nan32);

        assert!(nan64.is_nan() && nan64.is_sign_positive());
        assert!(neg_nan64.is_nan() && neg_nan64.is_sign_negative());
        assert!(nan32.is_nan() && nan32.is_sign_positive());
        assert!(neg_nan32.is_nan() && neg_nan32.is_sign_negative());

        // f32/f64 NaN conversion sign is non-deterministic: https://github.com/starkat99/half-rs/issues/103
        assert!(nan32_from_64.is_nan());
        assert!(neg_nan32_from_64.is_nan());
        assert!(nan8_from_64.is_nan());
        assert!(neg_nan8_from_64.is_nan());
        assert!(nan8_from_32.is_nan());
        assert!(neg_nan8_from_32.is_nan());
    }

    #[test]
    fn test_nan_conversion_to_larger() {
        let nan8 = F8E4M3::from_bits(0x7F);
        let neg_nan8 = F8E4M3::from_bits(0xFF);
        let nan32 = f32::from_bits(0x7F80_0001u32);
        let neg_nan32 = f32::from_bits(0xFF80_0001u32);
        let nan32_from_8 = f32::from(nan8);
        let neg_nan32_from_8 = f32::from(neg_nan8);
        let nan64_from_8 = f64::from(nan8);
        let neg_nan64_from_8 = f64::from(neg_nan8);
        let nan64_from_32 = f64::from(nan32);
        let neg_nan64_from_32 = f64::from(neg_nan32);

        assert!(nan8.is_nan() && nan8.is_sign_positive());
        assert!(neg_nan8.is_nan() && neg_nan8.is_sign_negative());
        assert!(nan32.is_nan() && nan32.is_sign_positive());
        assert!(neg_nan32.is_nan() && neg_nan32.is_sign_negative());

        // f32/f64 NaN conversion sign is non-deterministic: https://github.com/starkat99/half-rs/issues/103
        assert!(nan32_from_8.is_nan());
        assert!(neg_nan32_from_8.is_nan());
        assert!(nan64_from_8.is_nan());
        assert!(neg_nan64_from_8.is_nan());
        assert!(nan64_from_32.is_nan());
        assert!(neg_nan64_from_32.is_nan());
    }

    #[test]
    fn test_f8e4m3_to_f32() {
        let f = F8E4M3::from_f32(7.0);
        assert_eq!(f.to_f32(), 7.0f32);

        // 7.1 is NOT exactly representable in 8-bit, it's rounded
        let f = F8E4M3::from_f32(7.1);
        let diff = (f.to_f32() - 7.1f32).abs();
        // diff must be <= 4 * EPSILON, as 7 has two more significant bits than 1
        assert!(diff <= 4.0 * F8E4M3::EPSILON.to_f32());

        assert_eq!(F8E4M3::from_bits(0x0000_0001).to_f32(), 2.0f32.powi(-9));
        assert_eq!(
            F8E4M3::from_bits(0x0000_0005).to_f32(),
            5.0 * 2.0f32.powi(-9)
        );

        assert_eq!(
            F8E4M3::from_bits(0x0000_0001),
            F8E4M3::from_f32(2.0f32.powi(-9))
        );
        assert_eq!(
            F8E4M3::from_bits(0x0000_0005),
            F8E4M3::from_f32(5.0 * 2.0f32.powi(-9))
        );
    }

    #[test]
    fn test_f8e4m3_to_f64() {
        let f = F8E4M3::from_f64(7.0);
        assert_eq!(f.to_f64(), 7.0f64);

        // 7.1 is NOT exactly representable in 8-bit, it's rounded
        let f = F8E4M3::from_f64(7.1);
        let diff = (f.to_f64() - 7.1f64).abs();
        // diff must be <= 4 * EPSILON, as 7 has two more significant bits than 1
        assert!(diff <= 4.0 * F8E4M3::EPSILON.to_f64());

        assert_eq!(F8E4M3::from_bits(0x0000_0001).to_f64(), 2.0f64.powi(-9));
        assert_eq!(
            F8E4M3::from_bits(0x0000_0005).to_f64(),
            5.0 * 2.0f64.powi(-9)
        );

        assert_eq!(
            F8E4M3::from_bits(0x0000_0001),
            F8E4M3::from_f64(2.0f64.powi(-9))
        );
        assert_eq!(
            F8E4M3::from_bits(0x0000_0005),
            F8E4M3::from_f64(5.0 * 2.0f64.powi(-9))
        );
    }

    #[test]
    fn test_comparisons() {
        let zero = F8E4M3::from_f64(0.0);
        let one = F8E4M3::from_f64(1.0);
        let neg_zero = F8E4M3::from_f64(-0.0);
        let neg_one = F8E4M3::from_f64(-1.0);

        assert_eq!(zero.partial_cmp(&neg_zero), Some(Ordering::Equal));
        assert_eq!(neg_zero.partial_cmp(&zero), Some(Ordering::Equal));
        assert!(zero == neg_zero);
        assert!(neg_zero == zero);
        assert!(!(zero != neg_zero));
        assert!(!(neg_zero != zero));
        assert!(!(zero < neg_zero));
        assert!(!(neg_zero < zero));
        assert!(zero <= neg_zero);
        assert!(neg_zero <= zero);
        assert!(!(zero > neg_zero));
        assert!(!(neg_zero > zero));
        assert!(zero >= neg_zero);
        assert!(neg_zero >= zero);

        assert_eq!(one.partial_cmp(&neg_zero), Some(Ordering::Greater));
        assert_eq!(neg_zero.partial_cmp(&one), Some(Ordering::Less));
        assert!(!(one == neg_zero));
        assert!(!(neg_zero == one));
        assert!(one != neg_zero);
        assert!(neg_zero != one);
        assert!(!(one < neg_zero));
        assert!(neg_zero < one);
        assert!(!(one <= neg_zero));
        assert!(neg_zero <= one);
        assert!(one > neg_zero);
        assert!(!(neg_zero > one));
        assert!(one >= neg_zero);
        assert!(!(neg_zero >= one));

        assert_eq!(one.partial_cmp(&neg_one), Some(Ordering::Greater));
        assert_eq!(neg_one.partial_cmp(&one), Some(Ordering::Less));
        assert!(!(one == neg_one));
        assert!(!(neg_one == one));
        assert!(one != neg_one);
        assert!(neg_one != one);
        assert!(!(one < neg_one));
        assert!(neg_one < one);
        assert!(!(one <= neg_one));
        assert!(neg_one <= one);
        assert!(one > neg_one);
        assert!(!(neg_one > one));
        assert!(one >= neg_one);
        assert!(!(neg_one >= one));
    }

    #[test]
    fn arithmetic() {
        assert_eq!(F8E4M3::ONE + F8E4M3::ONE, F8E4M3::from_f32(2.));
        assert_eq!(F8E4M3::ONE - F8E4M3::ONE, F8E4M3::ZERO);
        assert_eq!(F8E4M3::ONE * F8E4M3::ONE, F8E4M3::ONE);
        assert_eq!(
            F8E4M3::from_f32(2.) * F8E4M3::from_f32(2.),
            F8E4M3::from_f32(4.)
        );
        assert_eq!(F8E4M3::ONE / F8E4M3::ONE, F8E4M3::ONE);
        assert_eq!(
            F8E4M3::from_f32(4.) / F8E4M3::from_f32(2.),
            F8E4M3::from_f32(2.)
        );
        assert_eq!(
            F8E4M3::from_f32(4.) % F8E4M3::from_f32(3.),
            F8E4M3::from_f32(1.)
        );
    }

    #[cfg(feature = "std")]
    #[test]
    fn formatting() {
        let f = F8E4M3::from_f32(0.1152344);

        assert_eq!(format!("{f:.3}"), "0.117");
        assert_eq!(format!("{f:.4}"), "0.1172");
        assert_eq!(format!("{f:+.4}"), "+0.1172");
        assert_eq!(format!("{f:>+10.4}"), "   +0.1172");

        assert_eq!(format!("{f:.3?}"), "0.117");
        assert_eq!(format!("{f:.4?}"), "0.1172");
        assert_eq!(format!("{f:+.4?}"), "+0.1172");
        assert_eq!(format!("{f:>+10.4?}"), "   +0.1172");
    }
}
