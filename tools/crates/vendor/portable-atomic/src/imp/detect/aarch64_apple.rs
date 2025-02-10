// SPDX-License-Identifier: Apache-2.0 OR MIT

/*
Run-time CPU feature detection on AArch64 Apple targets by using sysctlbyname.

On macOS, this module is currently only enabled on tests because AArch64 macOS
always supports FEAT_LSE and FEAT_LSE2 (see build script for more).

If macOS supporting FEAT_LSE128/FEAT_LRCPC3 becomes popular in the future, this module will
be used to support outline-atomics for FEAT_LSE128/FEAT_LRCPC3.
M4 is Armv9.2 and it doesn't support FEAT_LSE128/FEAT_LRCPC3.

Refs: https://developer.apple.com/documentation/kernel/1387446-sysctlbyname/determining_instruction_set_characteristics

TODO: non-macOS targets doesn't always supports FEAT_LSE2, but sysctl on them on the App Store is...?
- https://developer.apple.com/forums/thread/9440
- https://nabla-c0d3.github.io/blog/2015/06/16/ios9-security-privacy
- https://github.com/rust-lang/stdarch/pull/1636
*/

include!("common.rs");

use core::{mem, ptr};

// core::ffi::c_* (except c_void) requires Rust 1.64, libc requires Rust 1.63
#[allow(non_camel_case_types)]
mod ffi {
    pub(crate) use super::c_types::{c_char, c_int, c_size_t, c_void};

    sys_fn!({
        extern "C" {
            // https://developer.apple.com/documentation/kernel/1387446-sysctlbyname
            // https://github.com/apple-oss-distributions/xnu/blob/8d741a5de7ff4191bf97d57b9f54c2f6d4a15585/bsd/sys/sysctl.h
            pub(crate) fn sysctlbyname(
                name: *const c_char,
                old_p: *mut c_void,
                old_len_p: *mut c_size_t,
                new_p: *mut c_void,
                new_len: c_size_t,
            ) -> c_int;
        }
    });
}

unsafe fn sysctlbyname32(name: &[u8]) -> Option<u32> {
    const OUT_LEN: ffi::c_size_t = mem::size_of::<u32>() as ffi::c_size_t;

    debug_assert_eq!(name.last(), Some(&0), "{:?}", name);
    debug_assert_eq!(name.iter().filter(|&&v| v == 0).count(), 1, "{:?}", name);

    let mut out = 0_u32;
    let mut out_len = OUT_LEN;
    // SAFETY:
    // - the caller must guarantee that `name` a valid C string.
    // - `out_len` does not exceed the size of `out`.
    // - `sysctlbyname` is thread-safe.
    let res = unsafe {
        ffi::sysctlbyname(
            name.as_ptr().cast::<ffi::c_char>(),
            (&mut out as *mut u32).cast::<ffi::c_void>(),
            &mut out_len,
            ptr::null_mut(),
            0,
        )
    };
    if res != 0 {
        return None;
    }
    debug_assert_eq!(out_len, OUT_LEN);
    Some(out)
}

#[cold]
fn _detect(info: &mut CpuInfo) {
    // hw.optional.armv8_1_atomics is available on macOS 11+ (note: AArch64 support was added in macOS 11),
    // hw.optional.arm.FEAT_* are only available on macOS 12+.
    // Query both names in case future versions of macOS remove the old name.
    // https://github.com/golang/go/commit/c15593197453b8bf90fc3a9080ba2afeaf7934ea
    // https://github.com/google/boringssl/commit/91e0b11eba517d83b910b20fe3740eeb39ecb37e
    // SAFETY: we passed a valid C string.
    if unsafe {
        sysctlbyname32(b"hw.optional.arm.FEAT_LSE\0").unwrap_or(0) != 0
            || sysctlbyname32(b"hw.optional.armv8_1_atomics\0").unwrap_or(0) != 0
    } {
        info.set(CpuInfo::HAS_LSE);
    }
    // SAFETY: we passed a valid C string.
    if unsafe { sysctlbyname32(b"hw.optional.arm.FEAT_LSE2\0").unwrap_or(0) != 0 } {
        info.set(CpuInfo::HAS_LSE2);
    }
    // SAFETY: we passed a valid C string.
    if unsafe { sysctlbyname32(b"hw.optional.arm.FEAT_LSE128\0").unwrap_or(0) != 0 } {
        info.set(CpuInfo::HAS_LSE128);
    }
    // SAFETY: we passed a valid C string.
    if unsafe { sysctlbyname32(b"hw.optional.arm.FEAT_LRCPC3\0").unwrap_or(0) != 0 } {
        info.set(CpuInfo::HAS_RCPC3);
    }
}

#[allow(
    clippy::alloc_instead_of_core,
    clippy::std_instead_of_alloc,
    clippy::std_instead_of_core,
    clippy::undocumented_unsafe_blocks,
    clippy::wildcard_imports
)]
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_macos() {
        unsafe {
            assert_eq!(sysctlbyname32(b"hw.optional.armv8_1_atomics\0"), Some(1));
            assert_eq!(sysctlbyname32(b"hw.optional.arm.FEAT_LSE\0"), Some(1));
            assert_eq!(sysctlbyname32(b"hw.optional.arm.FEAT_LSE2\0"), Some(1));
            assert_eq!(sysctlbyname32(b"hw.optional.arm.FEAT_LSE128\0"), None);
            assert_eq!(std::io::Error::last_os_error().kind(), std::io::ErrorKind::NotFound);
            assert_eq!(sysctlbyname32(b"hw.optional.arm.FEAT_LRCPC\0"), Some(1));
            assert_eq!(sysctlbyname32(b"hw.optional.arm.FEAT_LRCPC2\0"), Some(1));
            assert_eq!(sysctlbyname32(b"hw.optional.arm.FEAT_LRCPC3\0"), None);
            assert_eq!(std::io::Error::last_os_error().kind(), std::io::ErrorKind::NotFound);
        }
    }

    #[cfg(target_pointer_width = "64")]
    #[test]
    fn test_alternative() {
        use c_types::*;
        #[cfg(not(portable_atomic_no_asm))]
        use std::arch::asm;
        use std::mem;
        use test_helper::sys;
        // Call syscall using asm instead of libc.
        // Note that macOS does not guarantee the stability of raw syscall.
        // (And they actually changed it: https://go-review.googlesource.com/c/go/+/25495)
        //
        // This is currently used only for testing.
        unsafe fn sysctlbyname32_no_libc(name: &[u8]) -> Result<u32, c_int> {
            // https://github.com/apple-oss-distributions/xnu/blob/8d741a5de7ff4191bf97d57b9f54c2f6d4a15585/bsd/kern/syscalls.master#L298
            #[inline]
            unsafe fn sysctl(
                name: *const c_int,
                name_len: c_uint,
                old_p: *mut c_void,
                old_len_p: *mut c_size_t,
                new_p: *const c_void,
                new_len: c_size_t,
            ) -> Result<c_int, c_int> {
                // https://github.com/apple-oss-distributions/xnu/blob/8d741a5de7ff4191bf97d57b9f54c2f6d4a15585/osfmk/mach/i386/syscall_sw.h#L158
                #[inline]
                const fn syscall_construct_unix(n: u64) -> u64 {
                    const SYSCALL_CLASS_UNIX: u64 = 2;
                    const SYSCALL_CLASS_SHIFT: u64 = 24;
                    const SYSCALL_CLASS_MASK: u64 = 0xFF << SYSCALL_CLASS_SHIFT;
                    const SYSCALL_NUMBER_MASK: u64 = !SYSCALL_CLASS_MASK;
                    (SYSCALL_CLASS_UNIX << SYSCALL_CLASS_SHIFT) | (SYSCALL_NUMBER_MASK & n)
                }
                #[allow(clippy::cast_possible_truncation)]
                // SAFETY: the caller must uphold the safety contract.
                unsafe {
                    // https://github.com/apple-oss-distributions/xnu/blob/8d741a5de7ff4191bf97d57b9f54c2f6d4a15585/bsd/kern/syscalls.master#L4
                    let mut n = syscall_construct_unix(202);
                    let r: i64;
                    asm!(
                        "svc 0",
                        "b.cc 2f",
                        "mov x16, x0",
                        "mov x0, #-1",
                        "2:",
                        inout("x16") n,
                        inout("x0") ptr_reg!(name) => r,
                        inout("x1") name_len as u64 => _,
                        in("x2") ptr_reg!(old_p),
                        in("x3") ptr_reg!(old_len_p),
                        in("x4") ptr_reg!(new_p),
                        in("x5") new_len as u64,
                        options(nostack),
                    );
                    if r as c_int == -1 {
                        Err(n as c_int)
                    } else {
                        Ok(r as c_int)
                    }
                }
            }
            // https://github.com/apple-oss-distributions/Libc/blob/af11da5ca9d527ea2f48bb7efbd0f0f2a4ea4812/gen/FreeBSD/sysctlbyname.c
            unsafe fn sysctlbyname(
                name: &[u8],
                old_p: *mut c_void,
                old_len_p: *mut c_size_t,
                new_p: *mut c_void,
                new_len: c_size_t,
            ) -> Result<c_int, c_int> {
                let mut real_oid: [c_int; sys::CTL_MAXNAME as usize + 2] = unsafe { mem::zeroed() };

                // Note that this is undocumented API.
                // Although FreeBSD defined it in sys/sysctl.h since https://github.com/freebsd/freebsd-src/commit/382e01c8dc7f328f46c61c82a29222f432f510f7
                let mut name2oid_oid: [c_int; 2] = [0, 3];

                let mut oid_len = mem::size_of_val(&real_oid);
                unsafe {
                    sysctl(
                        name2oid_oid.as_mut_ptr(),
                        2,
                        real_oid.as_mut_ptr().cast::<c_void>(),
                        &mut oid_len,
                        name.as_ptr().cast::<c_void>() as *mut c_void,
                        name.len() - 1,
                    )?
                };
                oid_len /= mem::size_of::<c_int>();
                #[allow(clippy::cast_possible_truncation)]
                unsafe {
                    sysctl(real_oid.as_mut_ptr(), oid_len as u32, old_p, old_len_p, new_p, new_len)
                }
            }

            const OUT_LEN: ffi::c_size_t = mem::size_of::<u32>() as ffi::c_size_t;

            debug_assert_eq!(name.last(), Some(&0), "{:?}", name);
            debug_assert_eq!(name.iter().filter(|&&v| v == 0).count(), 1, "{:?}", name);

            let mut out = 0_u32;
            let mut out_len = OUT_LEN;
            // SAFETY:
            // - the caller must guarantee that `name` a valid C string.
            // - `out_len` does not exceed the size of `out`.
            // - `sysctlbyname` is thread-safe.
            let res = unsafe {
                sysctlbyname(
                    name,
                    (&mut out as *mut u32).cast::<ffi::c_void>(),
                    &mut out_len,
                    ptr::null_mut(),
                    0,
                )?
            };
            debug_assert_eq!(res, 0);
            debug_assert_eq!(out_len, OUT_LEN);
            Ok(out)
        }

        for name in [
            &b"hw.optional.armv8_1_atomics\0"[..],
            b"hw.optional.arm.FEAT_LSE\0",
            b"hw.optional.arm.FEAT_LSE2\0",
            b"hw.optional.arm.FEAT_LSE128\0",
            b"hw.optional.arm.FEAT_LRCPC\0",
            b"hw.optional.arm.FEAT_LRCPC2\0",
            b"hw.optional.arm.FEAT_LRCPC3\0",
        ] {
            unsafe {
                if let Some(res) = sysctlbyname32(name) {
                    assert_eq!(res, sysctlbyname32_no_libc(name).unwrap());
                } else {
                    assert_eq!(sysctlbyname32_no_libc(name).unwrap_err(), libc::ENOENT);
                }
            }
        }
    }
}
