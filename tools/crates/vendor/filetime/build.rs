use std::env;
use std::process::Command;

fn main() {
    println!("cargo:rerun-if-changed=build.rs");

    if rustc_minor_version().map_or(false, |minor| minor >= 99) {
        println!("cargo:rustc-cfg=rust_199");
    }
}

fn rustc_minor_version() -> Option<u32> {
    let rustc = env::var_os("RUSTC")?;
    let output = Command::new(rustc).arg("--version").output().ok()?;
    let version = String::from_utf8(output.stdout).ok()?;
    // Parse "rustc 1.XX.Y ..." into `XX`.
    let mut pieces = version.split_whitespace().nth(1)?.split('.');
    if pieces.next()? != "1" {
        return None;
    }
    pieces.next()?.parse().ok()
}
