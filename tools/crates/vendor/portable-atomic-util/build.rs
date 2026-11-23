// SPDX-License-Identifier: Apache-2.0 OR MIT

// The rustc-cfg emitted by the build script are *not* public API.

use std::env;

use self::version::{Version, rustc_version};

fn main() {
    println!("cargo:rerun-if-changed=build.rs");

    let target = &*env::var("TARGET").expect("TARGET not set");

    let version = match rustc_version() {
        Some(version) => version,
        None => {
            if env::var_os("PORTABLE_ATOMIC_DENY_WARNINGS").is_some() {
                panic!("unable to determine rustc version")
            }
            println!(
                "cargo:warning={}: unable to determine rustc version; assuming latest stable rustc (1.{})",
                env!("CARGO_PKG_NAME"),
                Version::LATEST.minor
            );
            Version::LATEST
        }
    };

    if version.minor >= 80 {
        // Custom cfgs set by build script. Not public API.
        // grep -F 'cargo:rustc-cfg=' portable-atomic-util/build.rs | grep -Ev '^ *//' | sed -E 's/^.*cargo:rustc-cfg=//; s/(=\\)?".*$//' | LC_ALL=C sort -u | tr '\n' ',' | sed -E 's/,$/\n/'
        println!(
            "cargo:rustc-check-cfg=cfg(portable_atomic_no_core_unwind_safe,portable_atomic_no_error_in_core,portable_atomic_no_io_safety,portable_atomic_no_min_const_generics,portable_atomic_no_strict_provenance,portable_atomic_no_track_caller,portable_atomic_no_unsafe_op_in_unsafe_fn,portable_atomic_sanitize_thread)"
        );
    }

    // Note that cfgs are `no_`*, not `has_*`. This allows treating as the latest
    // stable rustc is used when the build script doesn't run. This is useful
    // for non-cargo build systems that don't run the build script.

    // track_caller stabilized in Rust 1.46 (nightly-2020-07-02): https://github.com/rust-lang/rust/pull/72445
    if !version.probe(46, 2020, 7, 1) {
        println!("cargo:rustc-cfg=portable_atomic_no_track_caller");
    }
    // min_const_generics stabilized in Rust 1.51 (nightly-2020-12-28): https://github.com/rust-lang/rust/pull/79135
    if !version.probe(51, 2020, 12, 27) {
        println!("cargo:rustc-cfg=portable_atomic_no_min_const_generics");
    }
    // unsafe_op_in_unsafe_fn stabilized in Rust 1.52 (nightly-2021-03-11): https://github.com/rust-lang/rust/pull/79208
    if !version.probe(52, 2021, 3, 10) {
        println!("cargo:rustc-cfg=portable_atomic_no_unsafe_op_in_unsafe_fn");
    }
    // https://github.com/rust-lang/rust/pull/84662 merged in Rust 1.56 (nightly-2021-08-02).
    if !version.probe(56, 2021, 8, 1) {
        println!("cargo:rustc-cfg=portable_atomic_no_core_unwind_safe");
    }
    // io_safety stabilized in Rust 1.63 (nightly-2022-06-16): https://github.com/rust-lang/rust/pull/95118
    // std::os::hermit::io::AsFd requires Rust 1.69 (https://github.com/rust-lang/rust/commit/b5fb4f3d9b1b308d59cab24ef2f9bf23dad948aa)
    if !version.probe(63, 2022, 6, 15)
        || version.minor < 69
            && env::var("CARGO_CFG_TARGET_OS").expect("CARGO_CFG_TARGET_OS not set") == "hermit"
    {
        println!("cargo:rustc-cfg=portable_atomic_no_io_safety");
    }
    // error_in_core stabilized in Rust 1.81 (nightly-2024-06-09): https://github.com/rust-lang/rust/pull/125951
    if !version.probe(81, 2024, 6, 8) {
        println!("cargo:rustc-cfg=portable_atomic_no_error_in_core");
    }
    // strict_provenance/exposed_provenance APIs stabilized in Rust 1.84 (nightly-2024-10-22): https://github.com/rust-lang/rust/pull/130350
    if !version.probe(84, 2024, 10, 21) {
        println!("cargo:rustc-cfg=portable_atomic_no_strict_provenance");
    }

    // `cfg(sanitize = "..")` is not stabilized.
    // x86_64-unknown-linux-gnutsan is available on stable since Rust 1.96: https://github.com/rust-lang/rust/pull/152757
    let sanitize = env::var("CARGO_CFG_SANITIZE").unwrap_or_default();
    if sanitize.contains("thread") || target == "x86_64-unknown-linux-gnutsan" {
        println!("cargo:rustc-cfg=portable_atomic_sanitize_thread");
    }
}

mod version {
    use std::{env, iter, process::Command, str};

    pub(crate) fn rustc_version() -> Option<Version> {
        let rustc = env::var_os("RUSTC")?;
        let rustc_wrapper = if env::var_os("CARGO_ENCODED_RUSTFLAGS").is_some() {
            env::var_os("RUSTC_WRAPPER").filter(|v| !v.is_empty())
        } else {
            // Cargo sets environment variables for wrappers correctly only since https://github.com/rust-lang/cargo/pull/9601.
            None
        };
        // Do not apply RUSTC_WORKSPACE_WRAPPER: https://github.com/cuviper/autocfg/issues/58#issuecomment-2067625980
        let mut rustc = rustc_wrapper.into_iter().chain(iter::once(rustc));
        let mut cmd = Command::new(rustc.next().unwrap());
        cmd.args(rustc);
        // Use verbose version output because the packagers add extra strings to the normal version output.
        // Do not use long flags (--version --verbose) because clippy-deriver doesn't handle them properly.
        // -vV is also matched with that cargo internally uses: https://github.com/rust-lang/cargo/blob/0.80.0/src/cargo/util/rustc.rs#L65
        let output = cmd.arg("-vV").output().ok()?;
        let verbose_version = str::from_utf8(&output.stdout).ok()?;
        Version::parse(verbose_version)
    }

    pub(crate) struct Version {
        pub(crate) minor: u32,
        pub(crate) nightly: bool,
        commit_date: Date,
    }

    impl Version {
        // The known latest stable version. If we unable to determine
        // the rustc version, we assume this is the current version.
        // It is no problem if this is older than the actual latest stable.
        pub(crate) const LATEST: Self = Self::stable(98);

        pub(crate) const fn stable(rustc_minor: u32) -> Self {
            Self { minor: rustc_minor, nightly: false, commit_date: Date::UNKNOWN }
        }

        pub(crate) fn probe(&self, minor: u32, year: u16, month: u8, day: u8) -> bool {
            if self.nightly {
                self.minor > minor
                    || self.minor == minor && self.commit_date >= Date::new(year, month, day)
            } else {
                self.minor >= minor
            }
        }

        pub(crate) fn parse(verbose_version: &str) -> Option<Self> {
            let mut release = verbose_version
                .lines()
                .find(|line| line.starts_with("release: "))
                .map(|line| &line["release: ".len()..])?
                .splitn(2, '-');
            let version = release.next().unwrap();
            let channel = release.next().unwrap_or_default();
            let mut digits = version.splitn(3, '.');
            let major = digits.next()?;
            if major != "1" {
                return None;
            }
            let minor = digits.next()?.parse::<u32>().ok()?;
            let _patch = digits.next().unwrap_or("0").parse::<u32>().ok()?;
            let nightly = match env::var_os("RUSTC_BOOTSTRAP") {
                // When -1 is passed rustc works like stable, e.g., cfg(target_feature = "unstable_target_feature") will never be set. https://github.com/rust-lang/rust/pull/132993
                Some(ref v) if v == "-1" => false,
                // When 1 is passed stable rustc works like nightly, but we ignore it for now.
                _ => channel == "nightly" || channel == "dev",
            };

            // we don't refer commit date on stable/beta.
            if nightly {
                let commit_date = (|| {
                    let mut commit_date = verbose_version
                        .lines()
                        .find(|line| line.starts_with("commit-date: "))
                        .map(|line| &line["commit-date: ".len()..])?
                        .splitn(3, '-');
                    let year = commit_date.next()?.parse::<u16>().ok()?;
                    let month = commit_date.next()?.parse::<u8>().ok()?;
                    let day = commit_date.next()?.parse::<u8>().ok()?;
                    if month > 12 || day > 31 {
                        return None;
                    }
                    Some(Date::new(year, month, day))
                })();
                Some(Self { minor, nightly, commit_date: commit_date.unwrap_or(Date::UNKNOWN) })
            } else {
                Some(Self::stable(minor))
            }
        }
    }

    #[derive(PartialEq, PartialOrd)]
    pub(crate) struct Date {
        pub(crate) year: u16,
        pub(crate) month: u8,
        pub(crate) day: u8,
    }

    impl Date {
        const UNKNOWN: Self = Self::new(0, 0, 0);

        const fn new(year: u16, month: u8, day: u8) -> Self {
            Self { year, month, day }
        }
    }
}
