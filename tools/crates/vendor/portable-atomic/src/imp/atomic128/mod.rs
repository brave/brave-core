// SPDX-License-Identifier: Apache-2.0 OR MIT

/*
128-bit atomic implementations on 64-bit architectures

See README.md for details.
*/

// AArch64
#[cfg(any(
    all(target_arch = "aarch64", any(not(portable_atomic_no_asm), portable_atomic_unstable_asm)),
    all(target_arch = "arm64ec", not(portable_atomic_no_asm))
))]
// Use intrinsics.rs on Miri and Sanitizer that do not support inline assembly.
#[cfg_attr(
    all(any(miri, portable_atomic_sanitize_thread), portable_atomic_new_atomic_intrinsics),
    path = "intrinsics.rs"
)]
pub(super) mod aarch64;

// powerpc64
#[cfg(all(
    target_arch = "powerpc64",
    portable_atomic_unstable_asm_experimental_arch,
    any(
        target_feature = "quadword-atomics",
        portable_atomic_target_feature = "quadword-atomics",
        all(
            feature = "fallback",
            not(portable_atomic_no_outline_atomics),
            any(
                all(
                    target_os = "linux",
                    any(
                        all(
                            target_env = "gnu",
                            any(target_endian = "little", not(target_feature = "crt-static")),
                        ),
                        all(
                            any(target_env = "musl", target_env = "ohos", target_env = "uclibc"),
                            not(target_feature = "crt-static"),
                        ),
                        portable_atomic_outline_atomics,
                    ),
                ),
                target_os = "android",
                target_os = "freebsd",
                target_os = "openbsd",
            ),
            not(any(miri, portable_atomic_sanitize_thread)),
        ),
    ),
))]
// Use intrinsics.rs on Miri and Sanitizer that do not support inline assembly.
#[cfg_attr(
    all(any(miri, portable_atomic_sanitize_thread), not(portable_atomic_pre_llvm_15)),
    path = "intrinsics.rs"
)]
pub(super) mod powerpc64;

// riscv64
#[cfg(all(
    target_arch = "riscv64",
    not(portable_atomic_no_asm),
    any(
        target_feature = "experimental-zacas",
        portable_atomic_target_feature = "experimental-zacas",
        all(
            feature = "fallback",
            not(portable_atomic_no_outline_atomics),
            any(test, portable_atomic_outline_atomics), // TODO(riscv): currently disabled by default
            any(target_os = "linux", target_os = "android"),
            not(any(miri, portable_atomic_sanitize_thread)),
        ),
    ),
))]
// Use intrinsics.rs on Miri and Sanitizer that do not support inline assembly.
#[cfg_attr(any(miri, portable_atomic_sanitize_thread), path = "intrinsics.rs")]
pub(super) mod riscv64;

// s390x
#[cfg(all(target_arch = "s390x", not(portable_atomic_no_asm)))]
// Use intrinsics.rs on Miri and Sanitizer that do not support inline assembly.
#[cfg_attr(any(miri, portable_atomic_sanitize_thread), path = "intrinsics.rs")]
pub(super) mod s390x;

// x86_64
#[cfg(all(
    target_arch = "x86_64",
    not(all(any(miri, portable_atomic_sanitize_thread), portable_atomic_no_cmpxchg16b_intrinsic)),
    any(not(portable_atomic_no_asm), portable_atomic_unstable_asm),
    any(
        target_feature = "cmpxchg16b",
        portable_atomic_target_feature = "cmpxchg16b",
        all(
            feature = "fallback",
            not(portable_atomic_no_outline_atomics),
            not(any(target_env = "sgx", miri)),
        ),
    ),
))]
// Use intrinsics.rs on Miri and Sanitizer that do not support inline assembly.
#[cfg_attr(any(miri, portable_atomic_sanitize_thread), path = "intrinsics.rs")]
pub(super) mod x86_64;
