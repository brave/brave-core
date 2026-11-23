fn main() {
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();

    println!("cargo::rustc-check-cfg=cfg(use_fls_attach_guard)");
    println!("cargo::rustc-check-cfg=cfg(use_tls_attach_guard)");
    if target_os == "windows" {
        let force_tls = std::env::var("_JNI_WINDOWS_FORCE_USE_TLS").is_ok();
        if force_tls {
            println!("cargo:rustc-cfg=use_tls_attach_guard");
        } else {
            println!("cargo:rustc-cfg=use_fls_attach_guard");
        }
    } else {
        println!("cargo:rustc-cfg=use_tls_attach_guard");
    }

    // Re-run if the environment variable changes
    println!("cargo:rerun-if-env-changed=_JNI_WINDOWS_FORCE_USE_TLS");
}
