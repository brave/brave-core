use criterion::black_box;

use winnow::combinator::alt;
use winnow::combinator::repeat;
use winnow::prelude::*;
use winnow::token::take_till;
use winnow::token::take_while;

fn contains_token(c: &mut criterion::Criterion) {
    let data = [
        ("contiguous", CONTIGUOUS),
        ("interleaved", INTERLEAVED),
        ("canada", CANADA),
    ];
    let mut group = c.benchmark_group("contains_token");
    for (name, sample) in data {
        let len = sample.len();
        group.throughput(criterion::Throughput::Bytes(len as u64));

        group.bench_with_input(criterion::BenchmarkId::new("slice", name), &len, |b, _| {
            b.iter(|| black_box(parser_slice.parse_peek(black_box(sample)).unwrap()));
        });
        group.bench_with_input(criterion::BenchmarkId::new("array", name), &len, |b, _| {
            b.iter(|| black_box(parser_array.parse_peek(black_box(sample)).unwrap()));
        });
        group.bench_with_input(criterion::BenchmarkId::new("tuple", name), &len, |b, _| {
            b.iter(|| black_box(parser_tuple.parse_peek(black_box(sample)).unwrap()));
        });
        group.bench_with_input(
            criterion::BenchmarkId::new("closure-or", name),
            &len,
            |b, _| {
                b.iter(|| black_box(parser_closure_or.parse_peek(black_box(sample)).unwrap()));
            },
        );
        group.bench_with_input(
            criterion::BenchmarkId::new("closure-matches", name),
            &len,
            |b, _| {
                b.iter(|| {
                    black_box(
                        parser_closure_matches
                            .parse_peek(black_box(sample))
                            .unwrap(),
                    )
                });
            },
        );
    }
    group.finish();
}

fn parser_slice(input: &mut &str) -> PResult<usize> {
    let contains = &['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'][..];
    repeat(
        0..,
        alt((take_while(1.., contains), take_till(1.., contains))),
    )
    .parse_next(input)
}

fn parser_array(input: &mut &str) -> PResult<usize> {
    let contains = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'];
    repeat(
        0..,
        alt((take_while(1.., contains), take_till(1.., contains))),
    )
    .parse_next(input)
}

fn parser_tuple(input: &mut &str) -> PResult<usize> {
    let contains = ('0', '1', '2', '3', '4', '5', '6', '7', '8', '9');
    repeat(
        0..,
        alt((take_while(1.., contains), take_till(1.., contains))),
    )
    .parse_next(input)
}

fn parser_closure_or(input: &mut &str) -> PResult<usize> {
    let contains = |c: char| {
        c == '0'
            || c == '1'
            || c == '2'
            || c == '3'
            || c == '4'
            || c == '5'
            || c == '6'
            || c == '7'
            || c == '8'
            || c == '9'
    };
    repeat(
        0..,
        alt((take_while(1.., contains), take_till(1.., contains))),
    )
    .parse_next(input)
}

fn parser_closure_matches(input: &mut &str) -> PResult<usize> {
    let contains = |c: char| matches!(c, '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9');
    repeat(
        0..,
        alt((take_while(1.., contains), take_till(1.., contains))),
    )
    .parse_next(input)
}

const CONTIGUOUS: &str = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
const INTERLEAVED: &str = "0123456789abc0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab";
const CANADA: &str = include_str!("../third_party/nativejson-benchmark/data/canada.json");

criterion::criterion_group!(benches, contains_token);
criterion::criterion_main!(benches);
