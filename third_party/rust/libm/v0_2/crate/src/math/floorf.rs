use core::f32;

/// Floor (f32)
///
/// Finds the nearest integer less than or equal to `x`.
#[cfg_attr(all(test, assert_no_panic), no_panic::no_panic)]
pub fn floorf(x: f32) -> f32 {
    // On wasm32 we know that LLVM's intrinsic will compile to an optimized
    // `f32.floor` native instruction, so we can leverage this for both code size
    // and speed.
    llvm_intrinsically_optimized! {
        #[cfg(target_arch = "wasm32")] {
            return unsafe { ::core::intrinsics::floorf32(x) }
        }
    }
    let mut ui = x.to_bits();
    let e = (((ui >> 23) as i32) & 0xff) - 0x7f;

    if e >= 23 {
        return x;
    }
    if e >= 0 {
        let m: u32 = 0x007fffff >> e;
        if (ui & m) == 0 {
            return x;
        }
        force_eval!(x + f32::from_bits(0x7b800000));
        if ui >> 31 != 0 {
            ui += m;
        }
        ui &= !m;
    } else {
        force_eval!(x + f32::from_bits(0x7b800000));
        if ui >> 31 == 0 {
            ui = 0;
        } else if ui << 1 != 0 {
            return -1.0;
        }
    }
    f32::from_bits(ui)
}

// PowerPC tests are failing on LLVM 13: https://github.com/rust-lang/rust/issues/88520
#[cfg(not(target_arch = "powerpc64"))]
#[cfg(test)]
mod tests {
    use super::*;
    use core::f32::*;

    #[test]
    fn sanity_check() {
        assert_eq!(floorf(0.5), 0.0);
        assert_eq!(floorf(1.1), 1.0);
        assert_eq!(floorf(2.9), 2.0);
    }

    /// The spec: https://en.cppreference.com/w/cpp/numeric/math/floor
    #[test]
    fn spec_tests() {
        // Not Asserted: that the current rounding mode has no effect.
        assert!(floorf(NAN).is_nan());
        for f in [0.0, -0.0, INFINITY, NEG_INFINITY].iter().copied() {
            assert_eq!(floorf(f), f);
        }
    }
}
