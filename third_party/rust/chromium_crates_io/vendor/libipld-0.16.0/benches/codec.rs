use criterion::{black_box, criterion_group, criterion_main, Criterion};
use libipld::cbor::DagCborCodec;
use libipld::cid::Cid;
use libipld::codec::Codec;
use libipld::{ipld, Ipld};

fn bench_codec(c: &mut Criterion) {
    c.bench_function("roundtrip", |b| {
        let cid =
            Cid::try_from("bafyreibvjvcv745gig4mvqs4hctx4zfkono4rjejm2ta6gtyzkqxfjeily").unwrap();
        let ipld = ipld!({
          "number": 1,
          "list": [true, null, false],
          "bytes": vec![0, 1, 2, 3],
          "map": { "float": 0.0, "string": "hello" },
          "link": cid,
        });
        b.iter(|| {
            for _ in 0..1000 {
                let bytes = DagCborCodec.encode(&ipld).unwrap();
                let ipld2: Ipld = DagCborCodec.decode(&bytes).unwrap();
                black_box(ipld2);
            }
        });
    });
}

criterion_group! {
    name = codec;
    config = Criterion::default();
    targets = bench_codec
}

criterion_main!(codec);
