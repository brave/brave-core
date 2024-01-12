use std::f32;
use std::f64;

use super::test_default_values::*;
use quick_protobuf::*;

#[test]
fn test_default_value_simple() {
    let bytes = &[];
    let mut reader = BytesReader::from_bytes(bytes);
    let d = TestDefaultValues::from_reader(&mut reader, bytes).unwrap();
    assert_eq!(1.0, d.double_field);
    assert_eq!(2.0, d.float_field);
    assert_eq!(3, d.int32_field);
    assert_eq!(4, d.int64_field);
    assert_eq!(5, d.uint32_field);
    assert_eq!(6, d.uint64_field);
    assert_eq!(7, d.sint32_field);
    assert_eq!(8, d.sint64_field);
    assert_eq!(9, d.fixed32_field);
    assert_eq!(10, d.fixed64_field);
    assert_eq!(11, d.sfixed32_field);
    assert_eq!(12, d.sfixed64_field);
    assert_eq!(true, d.bool_field);
    assert_eq!("abc\n22", d.string_field);
    assert_eq!(b"cde\n33".to_vec(), d.bytes_field.to_vec());
    assert!(EnumForDefaultValue::TWO.eq(&d.enum_field));
    assert!(d
        .enum_field_without_default
        .map_or(true, |e| e.eq(&EnumForDefaultValue::ONE)));
}

#[test]
fn test_default_value_extreme() {
    let bytes = &[];
    let mut reader = BytesReader::from_bytes(bytes);
    let d = TestExtremeDefaultValues::from_reader(&mut reader, bytes).unwrap();
    assert_eq!(f64::INFINITY, d.inf_double);
    assert_eq!(f64::NEG_INFINITY, d.neg_inf_double);
    assert!(d.nan_double.is_nan());
    assert_eq!(f32::INFINITY, d.inf_float);
    assert_eq!(f32::NEG_INFINITY, d.neg_inf_float);
    assert!(d.nan_float.is_nan());
}
