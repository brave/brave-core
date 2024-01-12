// DO NOT link to module tree
// This file is run remotely from test_owned.rs and should fail to compile

#[path = "./test_owned_pb.rs"]
pub mod proto;

fn main() {
    use std::convert::TryFrom;

    let encoded: Vec<u8> = vec![
        0x0au8, 0x07u8, 0x74u8, 0x65u8, 0x73u8, 0x74u8, 0x69u8, 0x6eu8, 0x67u8,
    ]; // s = "testing"

    let proto = proto::FooOwned::try_from(encoded).unwrap();
    let owned_copy = proto.proto().s.to_owned();

    drop(proto);
    println!("{:?}", owned_copy);
}
