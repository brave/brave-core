#![allow(dead_code)]

// utility module from rust-protobuf to decode/encode in hexadecimal
// ... used only for tests
mod hex;

#[macro_use]
mod test_utils;

mod v2;

//#[cfg(proto3)]
mod v3;
