use criterion::black_box;

use winnow::combinator::opt;
use winnow::prelude::*;
use winnow::stream::AsChar;
use winnow::stream::Stream as _;
use winnow::token::one_of;

fn iter(c: &mut criterion::Criterion) {
    let data = [
        ("contiguous", CONTIGUOUS.as_bytes()),
        ("interleaved", INTERLEAVED.as_bytes()),
        ("canada", CANADA.as_bytes()),
    ];
    let mut group = c.benchmark_group("iter");
    for (name, sample) in data {
        let len = sample.len();
        group.throughput(criterion::Throughput::Bytes(len as u64));

        group.bench_with_input(
            criterion::BenchmarkId::new("iterate", name),
            &len,
            |b, _| {
                b.iter(|| black_box(iterate.parse_peek(black_box(sample)).unwrap()));
            },
        );
        group.bench_with_input(
            criterion::BenchmarkId::new("next_token", name),
            &len,
            |b, _| {
                b.iter(|| black_box(next_token.parse_peek(black_box(sample)).unwrap()));
            },
        );
        group.bench_with_input(
            criterion::BenchmarkId::new("opt(one_of)", name),
            &len,
            |b, _| {
                b.iter(|| black_box(opt_one_of.parse_peek(black_box(sample)).unwrap()));
            },
        );
        group.bench_with_input(
            criterion::BenchmarkId::new("take_while", name),
            &len,
            |b, _| {
                b.iter(|| black_box(take_while.parse_peek(black_box(sample)).unwrap()));
            },
        );
        group.bench_with_input(criterion::BenchmarkId::new("repeat", name), &len, |b, _| {
            b.iter(|| black_box(repeat.parse_peek(black_box(sample)).unwrap()));
        });
    }
    group.finish();
}

fn iterate(input: &mut &[u8]) -> PResult<usize> {
    let mut count = 0;
    for byte in input.iter() {
        if byte.is_dec_digit() {
            count += 1;
        }
    }
    input.finish();
    Ok(count)
}

fn next_token(input: &mut &[u8]) -> PResult<usize> {
    let mut count = 0;
    while let Some(byte) = input.next_token() {
        if byte.is_dec_digit() {
            count += 1;
        }
    }
    Ok(count)
}

fn opt_one_of(input: &mut &[u8]) -> PResult<usize> {
    let mut count = 0;
    while !input.is_empty() {
        while opt(one_of(AsChar::is_dec_digit))
            .parse_next(input)?
            .is_some()
        {
            count += 1;
        }
        while opt(one_of(|b: u8| !b.is_dec_digit()))
            .parse_next(input)?
            .is_some()
        {}
    }
    Ok(count)
}

fn take_while(input: &mut &[u8]) -> PResult<usize> {
    let mut count = 0;
    while !input.is_empty() {
        count += winnow::token::take_while(0.., AsChar::is_dec_digit)
            .parse_next(input)?
            .len();
        let _ = winnow::token::take_while(0.., |b: u8| !b.is_dec_digit()).parse_next(input)?;
    }
    Ok(count)
}

fn repeat(input: &mut &[u8]) -> PResult<usize> {
    let mut count = 0;
    while !input.is_empty() {
        count += winnow::combinator::repeat(0.., one_of(AsChar::is_dec_digit))
            .map(|count: usize| count)
            .parse_next(input)?;
        winnow::combinator::repeat(0.., one_of(|b: u8| !b.is_dec_digit())).parse_next(input)?;
    }
    Ok(count)
}

const CONTIGUOUS: &str = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
const INTERLEAVED: &str = "0123456789abc0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab";
const CANADA: &str = include_str!("../third_party/nativejson-benchmark/data/canada.json");

criterion::criterion_group!(benches, iter);
criterion::criterion_main!(benches);
