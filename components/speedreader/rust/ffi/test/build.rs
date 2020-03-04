use cc;
use glob::glob;
use std::path::{Path, PathBuf};

const CFLAGS: &'static [&str] = &[
    "-std=c++14",
    "-Wcast-qual",
    "-Wwrite-strings",
    "-Wshadow",
    "-Winline",
    "-Wdisabled-optimization",
    "-Wuninitialized",
    "-Wcast-align",
    "-Wno-missing-field-initializers",
];

const SRC_DIR: &str = "src";
const PICOTEST_DIR: &str = "src/deps/picotest";
const INCLUDE_DIR: &str = "../include";

fn glob_cc_files<P: AsRef<Path>>(dirname: P, extension: &str) -> Vec<PathBuf> {
    glob(
        dirname
            .as_ref()
            .join(extension)
            .to_str()
            .expect("Path is not valid unicode."),
    )
    .expect("Failed to read glob pattern")
    .filter_map(Result::ok)
    .collect::<Vec<_>>()
}

fn main() {
    let mut build = cc::Build::new();

    for cflag in CFLAGS {
        build.flag(cflag);
    }

    // Collect all the C files from src/deps/picotest and src.
    let c_files = glob_cc_files(PICOTEST_DIR, "*.c");

    let mut cpp_files = glob_cc_files(SRC_DIR, "*.cpp");
    let mut extras = glob_cc_files("../src", "*.cpp");
    cpp_files.append(&mut extras);

    cc::Build::new()
        .debug(false)
        .cpp(false)
        .opt_level(3)
        .flag_if_supported("-Wl,no-as-needed")
        .warnings(true)
        .extra_warnings(true)
        .warnings_into_errors(true)
        .include(PICOTEST_DIR)
        .files(c_files)
        .compile("picotest");

    build
        .debug(false)
        .cpp(true)
        .opt_level(3)
        .flag_if_supported("-Wl,no-as-needed")
        .warnings(true)
        .extra_warnings(true)
        .warnings_into_errors(true)
        .include(INCLUDE_DIR)
        .include(PICOTEST_DIR)
        .files(cpp_files)
        .compile("speedreader_ctests");

    // Link against the C API.
    println!("cargo:rustc-link-lib=dylib=speedreader_ffi");
}
