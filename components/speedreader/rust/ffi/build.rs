extern crate cbindgen;

use std::env;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let output_dir = ".";//env::var("OUT_DIR").unwrap();

    // panic!("Output dir: {}", output_dir);
    cbindgen::generate(crate_dir)
        .expect("Unable to generate bindings")
        .write_to_file(format!("{}/include/speedreader_ffi.h", output_dir));
}
