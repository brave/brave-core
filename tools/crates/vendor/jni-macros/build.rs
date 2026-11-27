use rustc_version::{Version, version};

// Even though the `#[unsafe(no_mangle)]` syntax is required with the 2024 edition,
// the syntax itself was only added in 1.82.
//
// Since there are still projects that need to support older versions of Rust that
// means we need to gate the usage of this syntax behind a cfg flag.
fn main() {
    println!("cargo::rustc-check-cfg=cfg(has_unsafe_attr)");

    let v = version().expect("query rustc version");
    // Unsafe-attribute syntax is available since 1.82
    if v >= Version::new(1, 82, 0) {
        println!("cargo:rustc-cfg=has_unsafe_attr");
    }
}
