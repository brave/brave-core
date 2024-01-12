use std::sync::Arc;
use std::thread;

// use protobuf::CodedInputStream;
// use protobuf::Message;
use quick_protobuf::*;

use super::basic::*;

// test messages are sync
#[test]
fn test_sync() {
    let m = Arc::new({
        let mut r = TestTypesSingular::default();
        r.int32_field = Some(23);
        r
    });

    let threads: Vec<_> = (0..4)
        .map(|_| {
            let m_copy = m.clone();
            thread::spawn(move || {
                let mut bytes = Vec::new();
                {
                    let mut writer = Writer::new(&mut bytes);
                    m_copy.write_message(&mut writer).unwrap();
                }
                let mut reader = BytesReader::from_bytes(&bytes);
                let read = TestTypesSingular::from_reader(&mut reader, &bytes).unwrap();
                read.int32_field
            })
        })
        .collect();

    let results = threads
        .into_iter()
        .map(|t| t.join().unwrap())
        .collect::<Vec<_>>();
    assert_eq!(&[Some(23), Some(23), Some(23), Some(23)], &results[..]);
}
