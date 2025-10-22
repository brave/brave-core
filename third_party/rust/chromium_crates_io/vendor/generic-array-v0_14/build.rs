fn main() {
    if version_check::is_min_version("1.41.0").unwrap_or(false) {
        println!("cargo:rustc-cfg=relaxed_coherence");
    }

    // generic-array 1.x requires MSRV 1.65.0, so if they're using an ancient compiler
    // don't punish them with deprecation warnings.
    if version_check::is_min_version("1.65.0").unwrap_or(false) {
        // `rustc-check-cfg` was unstable before 1.80.0
        if version_check::is_min_version("1.80.0").unwrap_or(false) {
            println!("cargo:rustc-check-cfg=cfg(ga_is_deprecated)");
        }

        println!("cargo:rustc-cfg=ga_is_deprecated");
        println!(
            "cargo:warning=generic-array 0.14 is deprecated; please upgrade to generic-array 1.x"
        );
    }
}
