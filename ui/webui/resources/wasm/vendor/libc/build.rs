use std::process::{Command, Output};
use std::{env, str};

// List of cfgs this build script is allowed to set. The list is needed to support check-cfg, as we
// need to know all the possible cfgs that this script will set. If you need to set another cfg
// make sure to add it to this list as well.
const ALLOWED_CFGS: &'static [&'static str] = &[
    "emscripten_old_stat_abi",
    "espidf_time32",
    "freebsd10",
    "freebsd11",
    "freebsd12",
    "freebsd13",
    "freebsd14",
    "freebsd15",
    // FIXME(ctest): this config shouldn't be needed but ctest can't parse `const extern fn`
    "libc_const_extern_fn",
    "libc_deny_warnings",
    "libc_thread_local",
    "libc_ctest",
    // Corresponds to `__USE_TIME_BITS64` in UAPI
    "linux_time_bits64",
];

// Extra values to allow for check-cfg.
const CHECK_CFG_EXTRA: &'static [(&'static str, &'static [&'static str])] = &[
    (
        "target_os",
        &[
            "switch", "aix", "ohos", "hurd", "rtems", "visionos", "nuttx", "cygwin",
        ],
    ),
    ("target_env", &["illumos", "wasi", "aix", "ohos"]),
    (
        "target_arch",
        &["loongarch64", "mips32r6", "mips64r6", "csky"],
    ),
];

fn main() {
    // Avoid unnecessary re-building.
    println!("cargo:rerun-if-changed=build.rs");

    let (rustc_minor_ver, _is_nightly) = rustc_minor_nightly();
    let rustc_dep_of_std = env::var("CARGO_FEATURE_RUSTC_DEP_OF_STD").is_ok();
    let libc_ci = env::var("LIBC_CI").is_ok();

    // The ABI of libc used by std is backward compatible with FreeBSD 12.
    // The ABI of libc from crates.io is backward compatible with FreeBSD 11.
    //
    // On CI, we detect the actual FreeBSD version and match its ABI exactly,
    // running tests to ensure that the ABI is correct.
    println!("cargo:rerun-if-env-changed=RUST_LIBC_UNSTABLE_FREEBSD_VERSION");
    // Allow overriding the default version for testing
    let which_freebsd = if let Ok(version) = env::var("RUST_LIBC_UNSTABLE_FREEBSD_VERSION") {
        let vers = version.parse().unwrap();
        println!("cargo:warning=setting FreeBSD version to {vers}");
        vers
    } else if libc_ci {
        which_freebsd().unwrap_or(11)
    } else if rustc_dep_of_std {
        12
    } else {
        11
    };

    match which_freebsd {
        x if x < 10 => panic!("FreeBSD older than 10 is not supported"),
        10 => set_cfg("freebsd10"),
        11 => set_cfg("freebsd11"),
        12 => set_cfg("freebsd12"),
        13 => set_cfg("freebsd13"),
        14 => set_cfg("freebsd14"),
        _ => set_cfg("freebsd15"),
    }

    match emcc_version_code() {
        Some(v) if (v < 30142) => set_cfg("emscripten_old_stat_abi"),
        // Non-Emscripten or version >= 3.1.42.
        _ => (),
    }

    let linux_time_bits64 = env::var("RUST_LIBC_UNSTABLE_LINUX_TIME_BITS64").is_ok();
    println!("cargo:rerun-if-env-changed=RUST_LIBC_UNSTABLE_LINUX_TIME_BITS64");
    if linux_time_bits64 {
        set_cfg("linux_time_bits64");
    }

    // On CI: deny all warnings
    if libc_ci {
        set_cfg("libc_deny_warnings");
    }

    // #[thread_local] is currently unstable
    if rustc_dep_of_std {
        set_cfg("libc_thread_local");
    }

    // Set unconditionally when ctest is not being invoked.
    set_cfg("libc_const_extern_fn");

    // Since Rust 1.80, configuration that isn't recognized by default needs to be provided to
    // avoid warnings.
    if rustc_minor_ver >= 80 {
        for cfg in ALLOWED_CFGS {
            if rustc_minor_ver >= 75 {
                println!("cargo:rustc-check-cfg=cfg({})", cfg);
            } else {
                println!("cargo:rustc-check-cfg=values({})", cfg);
            }
        }
        for &(name, values) in CHECK_CFG_EXTRA {
            let values = values.join("\",\"");
            if rustc_minor_ver >= 75 {
                println!("cargo:rustc-check-cfg=cfg({},values(\"{}\"))", name, values);
            } else {
                println!("cargo:rustc-check-cfg=values({},\"{}\")", name, values);
            }
        }
    }
}

/// Run `rustc --version` and capture the output, adjusting arguments as needed if `clippy-driver`
/// is used instead.
fn rustc_version_cmd(is_clippy_driver: bool) -> Output {
    let rustc = env::var_os("RUSTC").expect("Failed to get rustc version: missing RUSTC env");

    let mut cmd = match env::var_os("RUSTC_WRAPPER") {
        Some(ref wrapper) if wrapper.is_empty() => Command::new(rustc),
        Some(wrapper) => {
            let mut cmd = Command::new(wrapper);
            cmd.arg(rustc);
            if is_clippy_driver {
                cmd.arg("--rustc");
            }

            cmd
        }
        None => Command::new(rustc),
    };

    cmd.arg("--version");

    let output = cmd.output().expect("Failed to get rustc version");

    if !output.status.success() {
        panic!(
            "failed to run rustc: {}",
            String::from_utf8_lossy(output.stderr.as_slice())
        );
    }

    output
}

/// Return the minor version of `rustc`, as well as a bool indicating whether or not the version
/// is a nightly.
fn rustc_minor_nightly() -> (u32, bool) {
    macro_rules! otry {
        ($e:expr) => {
            match $e {
                Some(e) => e,
                None => panic!("Failed to get rustc version"),
            }
        };
    }

    let mut output = rustc_version_cmd(false);

    if otry!(str::from_utf8(&output.stdout).ok()).starts_with("clippy") {
        output = rustc_version_cmd(true);
    }

    let version = otry!(str::from_utf8(&output.stdout).ok());

    let mut pieces = version.split('.');

    if pieces.next() != Some("rustc 1") {
        panic!("Failed to get rustc version");
    }

    let minor = pieces.next();

    // If `rustc` was built from a tarball, its version string
    // will have neither a git hash nor a commit date
    // (e.g. "rustc 1.39.0"). Treat this case as non-nightly,
    // since a nightly build should either come from CI
    // or a git checkout
    let nightly_raw = otry!(pieces.next()).split('-').nth(1);
    let nightly = nightly_raw
        .map(|raw| raw.starts_with("dev") || raw.starts_with("nightly"))
        .unwrap_or(false);
    let minor = otry!(otry!(minor).parse().ok());

    (minor, nightly)
}

fn which_freebsd() -> Option<i32> {
    let output = Command::new("freebsd-version").output().ok()?;
    if !output.status.success() {
        return None;
    }

    let stdout = String::from_utf8(output.stdout).ok()?;

    match &stdout {
        s if s.starts_with("10") => Some(10),
        s if s.starts_with("11") => Some(11),
        s if s.starts_with("12") => Some(12),
        s if s.starts_with("13") => Some(13),
        s if s.starts_with("14") => Some(14),
        s if s.starts_with("15") => Some(15),
        _ => None,
    }
}

fn emcc_version_code() -> Option<u64> {
    let output = Command::new("emcc").arg("-dumpversion").output().ok()?;
    if !output.status.success() {
        return None;
    }

    let version = String::from_utf8(output.stdout).ok()?;

    // Some Emscripten versions come with `-git` attached, so split the
    // version string also on the `-` char.
    let mut pieces = version.trim().split(['.', '-']);

    let major = pieces.next().and_then(|x| x.parse().ok()).unwrap_or(0);
    let minor = pieces.next().and_then(|x| x.parse().ok()).unwrap_or(0);
    let patch = pieces.next().and_then(|x| x.parse().ok()).unwrap_or(0);

    Some(major * 10000 + minor * 100 + patch)
}

fn set_cfg(cfg: &str) {
    if !ALLOWED_CFGS.contains(&cfg) {
        panic!("trying to set cfg {}, but it is not in ALLOWED_CFGS", cfg);
    }
    println!("cargo:rustc-cfg={}", cfg);
}
