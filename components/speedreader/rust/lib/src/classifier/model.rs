#![allow(clippy::all)]
#![allow(unused_parens)]

// Build script auto-generates Rust code from C model using baisc regex rules for the transformation
// Include of the generated code makes it behave here as if it was all manually written
include!(concat!(env!("OUT_DIR"), "/predictor.rs"));
