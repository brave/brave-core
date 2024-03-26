use quick_protobuf::*;

use super::test_required::*;

#[test]
fn test_write_missing_required() {
    let mut buf = Vec::new();
    let mut writer = Writer::new(&mut buf);
    // every message implements Default and required fields do have a value
    TestRequired::default().write_message(&mut writer).unwrap();
}

#[test]
fn test_read_missing_required() {
    let bytes = &[];
    let mut reader = BytesReader::from_bytes(bytes);
    // every message implements Default and required fields do have a value
    TestRequired::from_reader(&mut reader, bytes).unwrap();
}
