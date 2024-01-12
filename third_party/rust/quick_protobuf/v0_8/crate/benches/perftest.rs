#![feature(test)]

extern crate quick_protobuf;
extern crate test;

mod perftest_data;

use std::borrow::Cow;
use std::cmp::min;

use crate::perftest_data::*;

use quick_protobuf::message::MessageWrite;
use quick_protobuf::MessageRead;
use quick_protobuf::{BytesReader, Reader, Writer};
use test::{black_box, Bencher};

#[bench]
fn read_file(b: &mut Bencher) {
    let path = format!(
        "{}/benches/perftest_data/perftest_data.pbbin",
        env!("CARGO_MANIFEST_DIR")
    );
    b.iter(|| {
        let mut reader = Reader::from_file(&path).unwrap();
        reader.read(PerftestData::from_reader).unwrap().test1.len()
    })
}

macro_rules! perfbench {
    ($gen:ident, $m:ident, $write:ident, $read:ident) => {
        #[bench]
        fn $write(b: &mut Bencher) {
            let v = $gen();
            b.iter(|| {
                let mut buf = black_box(Vec::new());
                let mut w = Writer::new(&mut buf);
                for i in &v {
                    i.write_message(&mut w).unwrap();
                }
            })
        }

        #[bench]
        fn $read(b: &mut Bencher) {
            let v = $gen();
            let mut buf = Vec::new();
            {
                let mut w = Writer::new(&mut buf);
                for i in &v {
                    i.write_message(&mut w).unwrap();
                }
            }
            b.iter(|| {
                let mut r = BytesReader::from_bytes(&buf);
                while !r.is_eof() {
                    let _ = black_box($m::from_reader(&mut r, &buf).unwrap());
                }
            })
        }
    };
}

fn generate_test1() -> Vec<Test1> {
    (0..500)
        .map(|i| Test1 { value: Some(i) })
        .chain((0..200).map(|_| Test1 { value: None }))
        .collect()
}

perfbench!(generate_test1, Test1, write_test1, read_test1);

fn generate_repeated_bool() -> Vec<TestRepeatedBool> {
    (1..10)
        .map(|j| TestRepeatedBool {
            values: (0..100).map(|i| i % j == 0).collect(),
        })
        .collect()
}

perfbench!(
    generate_repeated_bool,
    TestRepeatedBool,
    write_repeated_bool,
    read_repeated_bool
);

fn generate_repeated_packed_int32() -> Vec<TestRepeatedPackedInt32> {
    (1..40)
        .map(|j| TestRepeatedPackedInt32 {
            values: (0..100).map(|i| i * j).collect(),
        })
        .collect()
}

perfbench!(
    generate_repeated_packed_int32,
    TestRepeatedPackedInt32,
    write_repeated_packed_int32,
    read_repeated_packed_int32
);

fn generate_repeated_packed_float() -> Vec<TestRepeatedPackedFloat<'static>> {
    (1..40)
        .map(|j| TestRepeatedPackedFloat {
            values: Cow::Owned((0..100).map(|i| (i * j) as f32).collect()),
        })
        .collect()
}

perfbench!(
    generate_repeated_packed_float,
    TestRepeatedPackedFloat,
    write_repeated_packed_float,
    read_repeated_packed_float
);

fn generate_repeated_messages() -> Vec<TestRepeatedMessages> {
    let mut messages = Vec::new();
    messages.push(TestRepeatedMessages {
        messages1: vec![],
        messages2: vec![],
        messages3: vec![],
    });

    for _ in 0..5 {
        let i1 = min(messages.len() % 3, messages.len() - 1);
        let i2 = min(messages.len() % 6, messages.len() - 1);
        let i3 = min(messages.len() % 9, messages.len() - 1);
        let m1 = messages[i1].clone();
        let m2 = messages[i2].clone();
        let m3 = messages[i3].clone();
        messages.push(TestRepeatedMessages {
            messages1: vec![m1.clone()],
            messages2: vec![m1.clone(), m2.clone()],
            messages3: vec![m1.clone(), m2.clone(), m3.clone()],
        });
    }
    messages
}

perfbench!(
    generate_repeated_messages,
    TestRepeatedMessages,
    write_repeated_messages,
    read_repeated_messages
);

fn generate_optional_messages() -> Vec<TestOptionalMessages> {
    let mut messages = Vec::new();
    messages.push(TestOptionalMessages {
        message1: None,
        message2: None,
        message3: None,
    });

    for _ in 0..10 {
        let i1 = min(messages.len() % 3, messages.len() - 1);
        let i2 = min(messages.len() % 6, messages.len() - 1);
        let i3 = min(messages.len() % 9, messages.len() - 1);
        let m1 = messages[i1].clone();
        let m2 = messages[i2].clone();
        let m3 = messages[i3].clone();
        messages.push(TestOptionalMessages {
            message1: Some(Box::new(m1.clone())),
            message2: Some(Box::new(m2.clone())),
            message3: Some(Box::new(m3.clone())),
        });
    }
    messages
}

perfbench!(
    generate_optional_messages,
    TestOptionalMessages,
    write_optional_messages,
    read_optional_messages
);

fn generate_strings() -> Vec<TestStrings<'static>> {
    let mut s = "hello world from quick-protobuf!!!"
        .split(' ')
        .cycle()
        .map(|s| Cow::Borrowed(s));
    (1..100)
        .map(|_| TestStrings {
            s1: s.by_ref().next(),
            s2: s.by_ref().next(),
            s3: s.by_ref().next(),
        })
        .collect()
}

perfbench!(generate_strings, TestStrings, write_strings, read_strings);

fn generate_small_bytes() -> Vec<TestBytes<'static>> {
    let mut s = "hello world from quick-protobuf!!!"
        .split(' ')
        .cycle()
        .map(|s| Cow::Borrowed(s.as_bytes()));
    (1..800)
        .map(|_| TestBytes {
            b1: s.by_ref().next(),
        })
        .collect()
}

perfbench!(
    generate_small_bytes,
    TestBytes,
    write_small_bytes,
    read_small_bytes
);

fn generate_large_bytes() -> Vec<TestBytes<'static>> {
    let mut s = "hello world from quick-protobuf!!!"
        .split(' ')
        .cycle()
        .map(|s| s.as_bytes());
    (1..30)
        .map(|_| TestBytes {
            b1: Some(Cow::Owned(s.by_ref().take(500).fold(
                Vec::new(),
                |mut cur, nxt| {
                    cur.extend_from_slice(nxt);
                    cur
                },
            ))),
        })
        .collect()
}

perfbench!(
    generate_large_bytes,
    TestBytes,
    write_large_bytes,
    read_large_bytes
);

fn generate_map() -> Vec<TestMap<'static>> {
    let mut s = "hello world from quick-protobuf!!!".split(' ').cycle();
    (1..30)
        .map(|_| TestMap {
            value: s
                .by_ref()
                .take(500)
                .map(|s| (Cow::Owned(s.to_string()), s.len() as u32))
                .collect(),
        })
        .collect()
}

perfbench!(generate_map, TestMap, write_map, read_map);

fn generate_all() -> Vec<PerftestData<'static>> {
    vec![PerftestData {
        test1: generate_test1(),
        test_repeated_bool: generate_repeated_bool(),
        test_repeated_messages: generate_repeated_messages(),
        test_optional_messages: generate_optional_messages(),
        test_strings: generate_strings(),
        test_repeated_packed_int32: generate_repeated_packed_int32(),
        test_repeated_packed_float: generate_repeated_packed_float(),
        test_small_bytearrays: generate_small_bytes(),
        test_large_bytearrays: generate_large_bytes(),
        test_map: generate_map(),
    }]
}

perfbench!(generate_all, PerftestData, write_all, read_all);
