use cargo_metadata::MetadataCommand;
use std::path::Path;
use std::process::Command;

fn main() {
    let metadata = MetadataCommand::new()
        .exec()
        .expect("Failed to execute cargo metadata");

    let boringtun = metadata
        .packages
        .iter()
        .find(|p| p.name == "boringtun")
        .expect("boringtun not found in cargo metadata");

    // Copy the FFI header to include/
    let manifest_dir = std::env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR not set");
    let header_src = boringtun
        .manifest_path
        .parent()
        .unwrap()
        .join("src/wireguard_ffi.h");
    let include_dir = Path::new(&manifest_dir).join("include");
    std::fs::create_dir_all(&include_dir).expect("Failed to create include/");
    std::fs::copy(&header_src, include_dir.join("wireguard_ffi.h"))
        .expect("Failed to copy wireguard_ffi.h");

    // Build boringtun directly as a cdylib
    let out_dir = std::env::var("OUT_DIR").expect("OUT_DIR not set");
    let target = std::env::var("TARGET").expect("TARGET not set");
    let profile = std::env::var("PROFILE").expect("PROFILE not set");

    // Force boringtun to recompile by removing its artifacts from OUT_DIR
    let boringtun_out = Path::new(&out_dir).join(&target).join(&profile);
    if boringtun_out.exists() {
        std::fs::remove_dir_all(&boringtun_out).ok();
    }

    let boringtun_manifest = boringtun.manifest_path.as_std_path();

    let mut cmd = Command::new("cargo");
    cmd.args([
        "rustc",
        "--manifest-path", boringtun_manifest.to_str().unwrap(),
        "--target", &target,
        "--profile", &profile,
        "--lib",
        "--no-default-features",
        "--features", "ffi-bindings",
    ]);
    cmd.env("CARGO_TARGET_DIR", &out_dir);

    let status = cmd.status().expect("Failed to invoke cargo rustc");
    assert!(status.success(), "cargo rustc failed");

    // Tell Cargo where to find the built library
    println!("cargo:rustc-link-search=native={}/{}/{}",
        out_dir, target, profile);
    println!("cargo:rustc-link-lib=dylib=boringtun");
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed={}", header_src.as_str());
}