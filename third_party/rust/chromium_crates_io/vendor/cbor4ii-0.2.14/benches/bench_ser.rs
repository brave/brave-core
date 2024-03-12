use std::collections::HashMap;
use serde::Serialize;
use criterion::{black_box, criterion_group, criterion_main, Criterion};


#[derive(Serialize)]
struct Log<'a> {
    level: usize,
    id: u64,
    time: (u64, (u32, u32)),
    path: &'a str,
    line: Option<u32>,
    fields: HashMap<String, String>,
    msg: &'a str,
}

fn bench_ser(c: &mut Criterion) {
    let map = {
        let mut map = HashMap::new();
        map.insert("key".into(), "value".into());
        map.insert("id".into(), "1".into());
        map
    };
    let msg = {
        use std::fmt::Write;

        let mut msg = String::new();
        for i in 0..321 {
            write!(&mut msg, "{}", i).unwrap();
        }
        msg
    };
    let log = Log {
        level: 3,
        id: c as *const _ as usize as u64,
        time: (0x2021, (0x99, 0x11111)),
        path: file!(),
        line: Some(line!()),
        fields: map,
        msg: &msg
    };

    c.bench_function("cbor4ii-ser", |b| {
        let mut buf = Vec::new();

        b.iter(|| {
            buf.clear();
            cbor4ii::serde::to_writer(black_box(&mut buf), black_box(&log)).unwrap();
        })
    });

    c.bench_function("serde_cbor-ser", |b| {
        let mut buf = Vec::new();

        b.iter(|| {
            buf.clear();
            serde_cbor::to_writer(black_box(&mut buf), black_box(&log)).unwrap();
        })
    });

    c.bench_function("ciborium-ser", |b| {
        let mut buf = Vec::new();

        b.iter(|| {
            buf.clear();
            ciborium::ser::into_writer(black_box(&log), black_box(&mut buf)).unwrap();
        })
    });
}

criterion_group!(ser, bench_ser);
criterion_main!(ser);
