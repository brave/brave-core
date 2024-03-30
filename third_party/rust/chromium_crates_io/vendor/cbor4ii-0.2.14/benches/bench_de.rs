use std::collections::HashMap;
use serde::{ Serialize, Deserialize };
use criterion::{black_box, criterion_group, criterion_main, Criterion};


#[derive(Serialize, Deserialize)]
struct Log<'a> {
    level: usize,
    id: u64,
    time: (u64, (u32, u32)),
    path: &'a str,
    line: Option<u32>,
    fields: HashMap<String, String>,
    msg: &'a str,
}

fn bench_de(c: &mut Criterion) {
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
    let buf = cbor4ii::serde::to_vec(Vec::new(), &log).unwrap();

    c.bench_function("cbor4ii-de", |b| {
        b.iter(|| {
            let _log: Log = cbor4ii::serde::from_slice(black_box(&buf)).unwrap();
        })
    });

    c.bench_function("serde_cbor-de", |b| {
        b.iter(|| {
            let _log: Log = serde_cbor::from_slice(black_box(&buf)).unwrap();
        })
    });

    // ciborium does not support zero copy decode :(
}

criterion_group!(de, bench_de);
criterion_main!(de);
