#![allow(missing_docs)]

extern crate alloc;

use criterion::{black_box, criterion_group, criterion_main, BatchSize, Criterion};
use dev_util::Bucket;

macro_rules! bench_function {
    ($group:ident, $function:expr, $data:ident $(,)?) => {
        let mut iter = $data.iter();
        $group.bench_function($data.name(), |b| {
            b.iter_batched(
                || iter.next().unwrap(),
                |i| {
                    let output = $function(i);
                    let _ = black_box(output);
                },
                BatchSize::SmallInput,
            );
        });
    };
}

fn bench(c: &mut Criterion) {
    let null_strings = Bucket::null();
    let ascii_non_null_strings = Bucket::ascii_non_null();
    let ascii_null_alternating_strings = Bucket::ascii_null_alternating();
    let utf8_clamped_2_strings = Bucket::utf8_clamped_width(2);
    let utf8_clamped_3_strings = Bucket::utf8_clamped_width(3);
    let utf8_clamped_4_strings = Bucket::utf8_clamped_width(4);
    let interspersed_strings = Bucket::interspersed();

    let null_bytes = null_strings.clone().into_bytes();
    let ascii_non_null_bytes = ascii_non_null_strings.clone().into_bytes();
    let ascii_null_alternating_bytes = ascii_null_alternating_strings.clone().into_bytes();
    let utf8_clamped_2_bytes = utf8_clamped_2_strings.clone().into_bytes();
    let utf8_clamped_3_bytes = utf8_clamped_3_strings.clone().into_bytes();
    let utf8_clamped_4_bytes = utf8_clamped_4_strings.clone().into_bytes();
    let interspersed_bytes = interspersed_strings.clone().into_bytes();

    let surrogate_pair_bytes = Bucket::surrogate_pairs();
    let mutf8_null_bytes = Bucket::mutf8_null_bytes();
    let interspersed_cesu8_bytes = Bucket::interspersed_cesu8();
    let interspersed_mutf8_bytes = Bucket::interspersed_mutf8();

    ////////////////////////////////////////////////////////////////////////////

    macro_rules! encode_group {
        ($group_name:literal, $function:expr $(,)?) => {
            let mut group = c.benchmark_group($group_name);
            group.throughput(Bucket::<String>::THROUGHPUT);

            bench_function!(group, $function, null_strings);
            bench_function!(group, $function, ascii_non_null_strings);
            bench_function!(group, $function, ascii_null_alternating_strings);
            bench_function!(group, $function, utf8_clamped_2_strings);
            bench_function!(group, $function, utf8_clamped_3_strings);
            bench_function!(group, $function, utf8_clamped_4_strings);
            bench_function!(group, $function, interspersed_strings);

            group.finish();
        };
    }

    encode_group!("encode_cesu8", simd_cesu8::encode);
    encode_group!("encode_mutf8", simd_cesu8::mutf8::encode);

    ////////////////////////////////////////////////////////////////////////////

    macro_rules! decode_group {
        ($group_name:literal, $function:expr $(,)?) => {
            let mut group = c.benchmark_group($group_name);
            group.throughput(Bucket::<Vec<u8>>::THROUGHPUT);

            bench_function!(group, $function, null_bytes);
            bench_function!(group, $function, ascii_non_null_bytes);
            bench_function!(group, $function, ascii_null_alternating_bytes);
            bench_function!(group, $function, utf8_clamped_2_bytes);
            bench_function!(group, $function, utf8_clamped_3_bytes);
            bench_function!(group, $function, utf8_clamped_4_bytes);
            bench_function!(group, $function, interspersed_bytes);

            bench_function!(group, $function, surrogate_pair_bytes);
            bench_function!(group, $function, interspersed_cesu8_bytes);

            if ($group_name).contains("mutf8") {
                bench_function!(group, $function, mutf8_null_bytes);
                bench_function!(group, $function, interspersed_mutf8_bytes);
            }

            group.finish();
        };
    }

    decode_group!("decode_cesu8", simd_cesu8::decode);
    decode_group!("decode_mutf8", simd_cesu8::mutf8::decode);
}

criterion_group!(benches, bench);
criterion_main!(benches);
