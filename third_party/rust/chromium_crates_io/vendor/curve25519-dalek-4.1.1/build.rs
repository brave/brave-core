//! This selects the curve25519_dalek_bits either by default from target_pointer_width or explicitly set

#![deny(clippy::unwrap_used, dead_code)]

#[allow(non_camel_case_types)]
#[derive(PartialEq, Debug)]
enum DalekBits {
    Dalek32,
    Dalek64,
}

fn main() {
    let curve25519_dalek_bits = match std::env::var("CARGO_CFG_CURVE25519_DALEK_BITS").as_deref() {
        Ok("32") => DalekBits::Dalek32,
        Ok("64") => DalekBits::Dalek64,
        _ => deterministic::determine_curve25519_dalek_bits(),
    };

    match curve25519_dalek_bits {
        DalekBits::Dalek64 => println!("cargo:rustc-cfg=curve25519_dalek_bits=\"64\""),
        DalekBits::Dalek32 => println!("cargo:rustc-cfg=curve25519_dalek_bits=\"32\""),
    }

    if rustc_version::version_meta()
        .expect("failed to detect rustc version")
        .channel
        == rustc_version::Channel::Nightly
    {
        println!("cargo:rustc-cfg=nightly");
    }

    let rustc_version = rustc_version::version().expect("failed to detect rustc version");
    if rustc_version.major == 1 && rustc_version.minor <= 64 {
        // Old versions of Rust complain when you have an `unsafe fn` and you use `unsafe {}` inside,
        // so for those we want to apply the `#[allow(unused_unsafe)]` attribute to get rid of that warning.
        println!("cargo:rustc-cfg=allow_unused_unsafe");
    }

    let target_arch = match std::env::var("CARGO_CFG_TARGET_ARCH") {
        Ok(arch) => arch,
        _ => "".to_string(),
    };

    // Backend overrides / defaults
    let curve25519_dalek_backend =
        match std::env::var("CARGO_CFG_CURVE25519_DALEK_BACKEND").as_deref() {
            Ok("fiat") => "fiat",
            Ok("serial") => "serial",
            Ok("simd") => {
                // simd can only be enabled on x86_64 & 64bit target_pointer_width
                match is_capable_simd(&target_arch, curve25519_dalek_bits) {
                    true => "simd",
                    // If override is not possible this must result to compile error
                    // See: issues/532
                    false => panic!("Could not override curve25519_dalek_backend to simd"),
                }
            }
            // default between serial / simd (if potentially capable)
            _ => match is_capable_simd(&target_arch, curve25519_dalek_bits) {
                true => "simd",
                false => "serial",
            },
        };
    println!("cargo:rustc-cfg=curve25519_dalek_backend=\"{curve25519_dalek_backend}\"");
}

// Is the target arch & curve25519_dalek_bits potentially simd capable ?
fn is_capable_simd(arch: &str, bits: DalekBits) -> bool {
    arch == "x86_64" && bits == DalekBits::Dalek64
}

// Deterministic cfg(curve25519_dalek_bits) when this is not explicitly set.
mod deterministic {

    use super::*;

    // Standard Cargo TARGET environment variable of triplet is required
    static ERR_MSG_NO_TARGET: &str = "Standard Cargo TARGET environment variable is not set";

    // Custom Non-Rust standard target platforms require explicit settings.
    static ERR_MSG_NO_PLATFORM: &str = "Unknown Rust target platform.";

    // Warning when the curve25519_dalek_bits cannot be determined
    fn determine_curve25519_dalek_bits_warning(cause: &str) {
        println!("cargo:warning=\"Defaulting to curve25519_dalek_bits=32: {cause}\"");
    }

    // Determine the curve25519_dalek_bits based on Rust standard TARGET triplet
    pub(super) fn determine_curve25519_dalek_bits() -> DalekBits {
        use platforms::target::PointerWidth;

        // TARGET environment is supplied by Cargo
        // https://doc.rust-lang.org/cargo/reference/environment-variables.html
        let target_triplet = match std::env::var("TARGET") {
            Ok(t) => t,
            Err(_) => {
                determine_curve25519_dalek_bits_warning(ERR_MSG_NO_TARGET);
                return DalekBits::Dalek32;
            }
        };

        // platforms crate is the source of truth used to determine the platform
        let platform = match platforms::Platform::find(&target_triplet) {
            Some(p) => p,
            None => {
                determine_curve25519_dalek_bits_warning(ERR_MSG_NO_PLATFORM);
                return DalekBits::Dalek32;
            }
        };

        #[allow(clippy::match_single_binding)]
        match platform.target_arch {
            //Issues: 449 and 456
            //TODO(Arm): Needs tests + benchmarks to back this up
            //platforms::target::Arch::Arm => DalekBits::Dalek64,
            //TODO(Wasm32): Needs tests + benchmarks to back this up
            //platforms::target::Arch::Wasm32 => DalekBits::Dalek64,
            _ => match platform.target_pointer_width {
                PointerWidth::U64 => DalekBits::Dalek64,
                PointerWidth::U32 => DalekBits::Dalek32,
                // Intended default solely for non-32/64 target pointer widths
                // Otherwise known target platforms only.
                _ => DalekBits::Dalek32,
            },
        }
    }
}
