use quick_protobuf::*;

use crate::rust_protobuf::hex::{encode_hex, decode_hex};
use super::basic::*;

#[test]
fn test_zeros_are_not_written() {
    let mut m = TestTypesSingular::default();
    m.bool_field = Some(false);
    m.enum_field = Some(TestEnumDescriptor::UNKNOWN);
    m.fixed32_field = Some(0);
    test_serialize!("", &m);
}
