#![no_std]

extern crate std as ext_std;

extern crate alloc;
extern crate quick_protobuf;

#[global_allocator]
static A: ext_std::alloc::System = ext_std::alloc::System;

mod pb_rs_nostd;

use crate::pb_rs_nostd::protos::no_std::NoStdMessage;
use quick_protobuf::{deserialize_from_slice, serialize_into_slice};

fn main() {
    let message = NoStdMessage::default();

    let mut buf = [0u8; 1024];
    serialize_into_slice(&message, &mut buf).unwrap();

    let read_message = deserialize_from_slice(&buf).unwrap();
    assert_eq!(message, read_message);
}
