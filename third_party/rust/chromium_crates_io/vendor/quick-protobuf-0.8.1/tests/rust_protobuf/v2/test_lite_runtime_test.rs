use crate::rust_protobuf::hex::{decode_hex, encode_hex};
use quick_protobuf::*;

use super::test_lite_runtime::*;

#[test]
fn test_lite_runtime() {
    let mut m = TestLiteRuntime::default();
    m.v = Some(10);
    test_serialize_deserialize!("08 0a", &m, TestLiteRuntime);

    // test it doesn't crash
    format!("{:?}", m);
}
