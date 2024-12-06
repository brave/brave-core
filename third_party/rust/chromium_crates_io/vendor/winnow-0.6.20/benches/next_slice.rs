use criterion::black_box;

use winnow::combinator::repeat;
use winnow::prelude::*;
use winnow::token::literal;
use winnow::token::one_of;

fn next_slice(c: &mut criterion::Criterion) {
    let mut group = c.benchmark_group("next_slice");

    let name = "ascii";
    let sample = "h".repeat(100);
    let sample = sample.as_str();
    group.bench_with_input(
        criterion::BenchmarkId::new("char", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_ascii_char.parse_peek(black_box(sample)).unwrap()));
        },
    );
    group.bench_with_input(
        criterion::BenchmarkId::new("str", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_ascii_str.parse_peek(black_box(sample)).unwrap()));
        },
    );
    group.bench_with_input(
        criterion::BenchmarkId::new("one_of", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_ascii_one_of.parse_peek(black_box(sample)).unwrap()));
        },
    );
    group.bench_with_input(
        criterion::BenchmarkId::new("tag_char", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_ascii_tag_char.parse_peek(black_box(sample)).unwrap()));
        },
    );
    group.bench_with_input(
        criterion::BenchmarkId::new("tag_str", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_ascii_tag_str.parse_peek(black_box(sample)).unwrap()));
        },
    );

    let name = "utf8";
    let sample = "🧑".repeat(100);
    let sample = sample.as_str();
    group.bench_with_input(
        criterion::BenchmarkId::new("char", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_utf8_char.parse_peek(black_box(sample)).unwrap()));
        },
    );
    group.bench_with_input(
        criterion::BenchmarkId::new("str", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_utf8_str.parse_peek(black_box(sample)).unwrap()));
        },
    );
    group.bench_with_input(
        criterion::BenchmarkId::new("one_of", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_utf8_one_of.parse_peek(black_box(sample)).unwrap()));
        },
    );
    group.bench_with_input(
        criterion::BenchmarkId::new("tag_char", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_utf8_tag_char.parse_peek(black_box(sample)).unwrap()));
        },
    );
    group.bench_with_input(
        criterion::BenchmarkId::new("tag_str", name),
        sample,
        |b, sample| {
            b.iter(|| black_box(parser_utf8_tag_str.parse_peek(black_box(sample)).unwrap()));
        },
    );

    group.finish();
}

fn parser_ascii_char(input: &mut &str) -> PResult<usize> {
    repeat(0.., 'h').parse_next(input)
}

fn parser_ascii_str(input: &mut &str) -> PResult<usize> {
    repeat(0.., "h").parse_next(input)
}

fn parser_ascii_one_of(input: &mut &str) -> PResult<usize> {
    repeat(0.., one_of('h')).parse_next(input)
}

fn parser_ascii_tag_char(input: &mut &str) -> PResult<usize> {
    repeat(0.., literal('h')).parse_next(input)
}

fn parser_ascii_tag_str(input: &mut &str) -> PResult<usize> {
    repeat(0.., literal("h")).parse_next(input)
}

fn parser_utf8_char(input: &mut &str) -> PResult<usize> {
    repeat(0.., '🧑').parse_next(input)
}

fn parser_utf8_str(input: &mut &str) -> PResult<usize> {
    repeat(0.., "🧑").parse_next(input)
}

fn parser_utf8_one_of(input: &mut &str) -> PResult<usize> {
    repeat(0.., one_of('🧑')).parse_next(input)
}

fn parser_utf8_tag_char(input: &mut &str) -> PResult<usize> {
    repeat(0.., literal('🧑')).parse_next(input)
}

fn parser_utf8_tag_str(input: &mut &str) -> PResult<usize> {
    repeat(0.., literal("🧑")).parse_next(input)
}

criterion::criterion_group!(benches, next_slice);
criterion::criterion_main!(benches);
