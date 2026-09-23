//! # Safety
//!
//! The functions in this module must only be executed on an ARM system with the CRC feature.

#[cfg_attr(not(target_arch = "aarch64"), allow(unused))]
#[target_feature(enable = "crc")]
pub unsafe fn crc32_acle_aarch64(crc: u32, buf: &[u8]) -> u32 {
    let mut c = !crc;

    // SAFETY: [u8; 8] safely transmutes into u64.
    let (before, middle, after) = unsafe { buf.align_to::<u64>() };

    // SAFETY: `remainder` requires the feature "crc" but so does this function
    c = unsafe { remainder(c, before) };

    if middle.is_empty() && after.is_empty() {
        return !c;
    }

    for d in middle {
        c = unsafe { __crc32d(c, *d) };
    }

    // SAFETY: `remainder` requires the feature "crc" but so does this function
    c = unsafe { remainder(c, after) };

    !c
}

#[inline]
#[target_feature(enable = "crc")]
unsafe fn remainder(mut c: u32, mut buf: &[u8]) -> u32 {
    if let [b0, b1, b2, b3, rest @ ..] = buf {
        c = unsafe { __crc32w(c, u32::from_le_bytes([*b0, *b1, *b2, *b3])) };
        buf = rest;
    }

    if let [b0, b1, rest @ ..] = buf {
        c = unsafe { __crc32h(c, u16::from_le_bytes([*b0, *b1])) };
        buf = rest;
    }

    if let [b0, rest @ ..] = buf {
        c = unsafe { __crc32b(c, *b0) };
        buf = rest;
    }

    debug_assert!(buf.is_empty());

    c
}

// FIXME the intrinsics below are stable since rust 1.80.0: remove these and use the standard
// library versions once our MSRV reaches that version.

/// CRC32 single round checksum for bytes (8 bits).
///
/// [Arm's documentation](https://developer.arm.com/architectures/instruction-sets/intrinsics/__crc32b)
#[target_feature(enable = "crc")]
#[cfg_attr(target_arch = "arm", target_feature(enable = "v8"))]
unsafe fn __crc32b(mut crc: u32, data: u8) -> u32 {
    unsafe {
        core::arch::asm!("crc32b {crc:w}, {crc:w}, {data:w}", crc = inout(reg) crc, data = in(reg) data);
        crc
    }
}

/// CRC32 single round checksum for half words (16 bits).
///
/// [Arm's documentation](https://developer.arm.com/architectures/instruction-sets/intrinsics/__crc32h)
#[target_feature(enable = "crc")]
#[cfg_attr(target_arch = "arm", target_feature(enable = "v8"))]
unsafe fn __crc32h(mut crc: u32, data: u16) -> u32 {
    unsafe {
        core::arch::asm!("crc32h {crc:w}, {crc:w}, {data:w}", crc = inout(reg) crc, data = in(reg) data);
        crc
    }
}

/// CRC32 single round checksum for words (32 bits).
///
/// [Arm's documentation](https://developer.arm.com/architectures/instruction-sets/intrinsics/__crc32w)
#[target_feature(enable = "crc")]
#[cfg_attr(target_arch = "arm", target_feature(enable = "v8"))]
pub unsafe fn __crc32w(mut crc: u32, data: u32) -> u32 {
    unsafe {
        core::arch::asm!("crc32w {crc:w}, {crc:w}, {data:w}", crc = inout(reg) crc, data = in(reg) data);
        crc
    }
}

/// CRC32 single round checksum for double words (64 bits).
///
/// [Arm's documentation](https://developer.arm.com/architectures/instruction-sets/intrinsics/__crc32d)
#[cfg(target_arch = "aarch64")]
#[target_feature(enable = "crc")]
unsafe fn __crc32d(mut crc: u32, data: u64) -> u32 {
    unsafe {
        core::arch::asm!("crc32x {crc:w}, {crc:w}, {data:x}", crc = inout(reg) crc, data = in(reg) data);
        crc
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    quickcheck::quickcheck! {
        #[cfg(target_arch = "aarch64")]
        fn crc32_acle_aarch64_is_crc32fast(v: Vec<u8>, start: u32) -> bool {
            let mut h = crc32fast::Hasher::new_with_initial(start);
            h.update(&v);

            let a = unsafe { crc32_acle_aarch64(start, &v) };
            let b = h.finalize();

            a == b
        }
    }

    #[test]
    fn test_crc32b() {
        if !crate::cpu_features::is_enabled_crc() {
            return;
        }

        unsafe {
            assert_eq!(__crc32b(0, 0), 0);
            assert_eq!(__crc32b(0, 255), 755167117);
        }
    }

    #[test]
    fn test_crc32h() {
        if !crate::cpu_features::is_enabled_crc() {
            return;
        }

        unsafe {
            assert_eq!(__crc32h(0, 0), 0);
            assert_eq!(__crc32h(0, 16384), 1994146192);
        }
    }

    #[test]
    fn test_crc32w() {
        if !crate::cpu_features::is_enabled_crc() {
            return;
        }

        unsafe {
            assert_eq!(__crc32w(0, 0), 0);
            assert_eq!(__crc32w(0, 4294967295), 3736805603);
        }
    }

    #[test]
    #[cfg(target_arch = "aarch64")]
    fn test_crc32d() {
        if !crate::cpu_features::is_enabled_crc() {
            return;
        }

        unsafe {
            assert_eq!(__crc32d(0, 0), 0);
            assert_eq!(__crc32d(0, 18446744073709551615), 1147535477);
        }
    }
}
