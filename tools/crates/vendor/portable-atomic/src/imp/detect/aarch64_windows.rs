// SPDX-License-Identifier: Apache-2.0 OR MIT

/*
Run-time CPU feature detection on AArch64 Windows by using IsProcessorFeaturePresent.

Run-time detection of FEAT_LSE on Windows by is_aarch64_feature_detected is supported on Rust 1.70+.
https://github.com/rust-lang/stdarch/pull/1373

Refs: https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocessorfeaturepresent
*/

include!("common.rs");

// windows-sys requires Rust 1.60
#[allow(clippy::upper_case_acronyms)]
mod ffi {
    pub(crate) type DWORD = u32;
    pub(crate) type BOOL = i32;

    pub(crate) const FALSE: BOOL = 0;

    // Defined in winnt.h of Windows SDK.
    pub(crate) const PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE: DWORD = 34;

    extern "system" {
        // https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocessorfeaturepresent
        pub(crate) fn IsProcessorFeaturePresent(ProcessorFeature: DWORD) -> BOOL;
    }
}

#[cold]
fn _detect(info: &mut CpuInfo) {
    // SAFETY: calling IsProcessorFeaturePresent is safe, and FALSE is also
    // returned if the HAL does not support detection of the specified feature.
    if unsafe {
        ffi::IsProcessorFeaturePresent(ffi::PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE) != ffi::FALSE
    } {
        info.set(CpuInfo::HAS_LSE);
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

    // Static assertions for FFI bindings.
    // This checks that FFI bindings defined in this crate and FFI bindings defined
    // in windows-sys have compatible signatures (or the same values if constants).
    // Since this is static assertion, we can detect problems with
    // `cargo check --tests --target <target>` run in CI (via TESTS=1 build.sh)
    // without actually running tests on these platforms.
    // (Unlike libc, windows-sys programmatically generates bindings from Windows
    // API metadata, so it should be enough to check compatibility with the
    // windows-sys' signatures/values.)
    // See also https://github.com/taiki-e/test-helper/blob/HEAD/tools/codegen/src/ffi.rs.
    // TODO(codegen): auto-generate this test
    #[allow(clippy::cast_possible_wrap, clippy::cast_sign_loss, clippy::cast_possible_truncation)]
    const _: fn() = || {
        let _: ffi::DWORD = 0 as windows_sys::Win32::System::Threading::PROCESSOR_FEATURE_ID;
        let _: ffi::BOOL = 0 as windows_sys::Win32::Foundation::BOOL;
        let mut _is_processor_feature_present: unsafe extern "system" fn(ffi::DWORD) -> ffi::BOOL =
            ffi::IsProcessorFeaturePresent;
        _is_processor_feature_present =
            windows_sys::Win32::System::Threading::IsProcessorFeaturePresent;
        static_assert!(ffi::FALSE == windows_sys::Win32::Foundation::FALSE);
        static_assert!(
            ffi::PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE
                == windows_sys::Win32::System::Threading::PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE
        );
    };
}
