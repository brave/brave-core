//! Safe adapters to the real kernels, compiled only for tests and `--cfg fuzzing`.
//!
//! Keep input generation and assertions in fuzz/core.rs. This module only checks
//! memory/CPU preconditions; character validation belongs to the checked kernel.

use crate::{decode, encode, CheckCase};

#[derive(Clone, Copy)]
enum Kind {
    Scalar,
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    Sse41,
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    Avx2,
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    Avx512,
    #[cfg(target_arch = "aarch64")]
    Neon,
}

// A caller cannot construct an adapter for an unsupported instruction set.
#[derive(Clone, Copy)]
pub struct Backend(Kind);

pub fn backends() -> impl Iterator<Item = Backend> {
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    let (sse41, avx2, avx512) = {
        // Unit tests discover capabilities independently of production dispatch.
        // This also exercises AVX-512 when compiling with a static AVX2 baseline.
        #[cfg(test)]
        {
            let avx2 = std::arch::is_x86_feature_detected!("avx2");
            (
                std::arch::is_x86_feature_detected!("sse4.1"),
                avx2,
                avx2 && std::arch::is_x86_feature_detected!("avx512f")
                    && std::arch::is_x86_feature_detected!("avx512bw"),
            )
        }
        #[cfg(not(test))]
        {
            use crate::Vectorization::{AVX2, AVX512, SSE41};
            let kind = crate::vectorization_support();
            (
                matches!(kind, SSE41 | AVX2 | AVX512),
                matches!(kind, AVX2 | AVX512),
                kind == AVX512,
            )
        }
    };
    IntoIterator::into_iter([
        Some(Backend(Kind::Scalar)),
        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        sse41.then_some(Backend(Kind::Sse41)),
        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        avx2.then_some(Backend(Kind::Avx2)),
        #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
        avx512.then_some(Backend(Kind::Avx512)),
        #[cfg(target_arch = "aarch64")]
        (crate::vectorization_support() == crate::Vectorization::Neon)
            .then_some(Backend(Kind::Neon)),
    ])
    .flatten()
}

impl Backend {
    pub fn name(self) -> &'static str {
        match self.0 {
            Kind::Scalar => "scalar",
            #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
            Kind::Sse41 => "sse41",
            #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
            Kind::Avx2 => "avx2",
            #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
            Kind::Avx512 => "avx512",
            #[cfg(target_arch = "aarch64")]
            Kind::Neon => "neon",
        }
    }

    pub fn encode(self, src: &[u8], dst: &mut [u8], upper: bool) {
        assert_eq!(src.len().checked_mul(2), Some(dst.len()));
        // SAFETY: backends() checked the ISA, and the assertion establishes the
        // exact 1:2 ratio. The kernels only write initialized ASCII, so casting
        // the destination to MaybeUninit never invalidates initialized storage.
        unsafe {
            let dst = core::slice::from_raw_parts_mut(dst.as_mut_ptr().cast(), dst.len());
            match self.0 {
                Kind::Scalar => encode::hex_encode_custom_case_fallback(src, dst, upper),
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Sse41 => encode::hex_encode_sse41(src, dst, upper),
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Avx2 | Kind::Avx512 => encode::hex_encode_avx2(src, dst, upper),
                #[cfg(target_arch = "aarch64")]
                Kind::Neon => encode::hex_encode_neon(src, dst, upper),
            }
        }
    }

    pub fn check(self, src: &[u8], case: CheckCase) -> bool {
        #[allow(unused_unsafe)] // Non-SIMD targets only compile the safe fallback.
        // SAFETY: backends() checked the ISA. Each checker bounds its own loads
        // and accepts arbitrary source bytes and lengths, including odd lengths.
        unsafe {
            match self.0 {
                Kind::Scalar => decode::hex_check_fallback_with_case(src, case),
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Sse41 => decode::x86::hex_check_sse_with_case(src, case),
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Avx2 => decode::x86::hex_check_avx2_with_case(src, case),
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Avx512 => decode::x86::hex_check_avx512_with_case(src, case),
                #[cfg(target_arch = "aarch64")]
                Kind::Neon => decode::aarch64::hex_check_neon_with_case(src, case),
            }
        }
    }

    pub fn decode(self, src: &[u8], dst: &mut [u8], case: CheckCase) -> bool {
        assert_eq!(dst.len().checked_mul(2), Some(src.len()));
        // This is the real dispatch minimum for SIMD loads, not a character
        // pre-check. Valid and damaged vector-sized inputs go straight through.
        if src.len() < 16 {
            return decode::hex_decode_short_scalar(src, dst, case).is_ok();
        }
        #[allow(unused_unsafe)] // The NEON dispatcher and scalar path are safe.
        // SAFETY: backends() checked the ISA and the assertion guarantees even
        // input and the exact 2:1 ratio. The checked kernels accept invalid text
        // and must reject it before writing; do not pre-check characters here.
        unsafe {
            match self.0 {
                Kind::Scalar => {
                    if !decode::hex_check_fallback_with_case(src, case) {
                        return false;
                    }
                    decode::hex_decode_fallback(src, dst);
                    true
                }
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Sse41 => decode::x86::hex_decode_sse41_checked(src, dst, case).is_ok(),
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Avx2 => decode::x86::hex_decode_avx2_checked(src, dst, case).is_ok(),
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Avx512 => decode::x86::hex_decode_avx512_checked(src, dst, case).is_ok(),
                // Use the actual short/bounded/general NEON selection so the
                // harness cannot drift from the production length thresholds.
                #[cfg(target_arch = "aarch64")]
                Kind::Neon => decode::decode_checked(src, dst, case).is_ok(),
            }
        }
    }

    // On failure, earlier valid blocks may already be written. This adapter
    // checks the same memory/CPU preconditions without pre-validating text.
    pub fn decode_owned(self, src: &[u8], dst: &mut [u8], case: CheckCase) -> bool {
        assert_eq!(dst.len().checked_mul(2), Some(src.len()));
        #[allow(unused_unsafe)] // Non-SIMD targets use the checked scalar path.
        // SAFETY: backends() checked the ISA; the slices have the exact 2:1 ratio.
        unsafe {
            match self.0 {
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Avx2 => decode::x86::hex_decode_avx2_owned(src, dst, case).is_ok(),
                #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
                Kind::Avx512 => decode::x86::hex_decode_avx512_owned(src, dst, case).is_ok(),
                #[cfg(target_arch = "aarch64")]
                Kind::Neon => decode::aarch64::hex_decode_neon_owned(src, dst, case).is_ok(),
                _ => self.decode(src, dst, case),
            }
        }
    }
}
