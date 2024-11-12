use criterion::black_box;

use winnow::combinator::repeat;
use winnow::prelude::*;
use winnow::token::take_until;

fn find_slice(c: &mut criterion::Criterion) {
    let empty = "";
    let start_byte = "\r".repeat(100);
    let start_slice = "\r\n".repeat(100);
    let small = format!("{:>10}\r\n", "").repeat(100);
    let large = format!("{:>10000}\r\n", "").repeat(100);

    let data = [
        ("empty", (empty, empty)),
        ("start", (&start_byte, &start_slice)),
        ("medium", (&small, &small)),
        ("large", (&large, &large)),
    ];
    let mut group = c.benchmark_group("find_slice");
    for (name, samples) in data {
        group.bench_with_input(
            criterion::BenchmarkId::new("byte", name),
            samples.0,
            |b, sample| {
                b.iter(|| black_box(parser_byte.parse_peek(black_box(sample)).unwrap()));
            },
        );

        group.bench_with_input(
            criterion::BenchmarkId::new("slice", name),
            samples.1,
            |b, sample| {
                b.iter(|| black_box(parser_slice.parse_peek(black_box(sample)).unwrap()));
            },
        );
    }
    group.finish();
}

fn parser_byte(input: &mut &str) -> PResult<usize> {
    repeat(0.., (take_until(0.., "\r"), "\r")).parse_next(input)
}

fn parser_slice(input: &mut &str) -> PResult<usize> {
    repeat(0.., (take_until(0.., "\r\n"), "\r\n")).parse_next(input)
}

criterion::criterion_group!(benches, find_slice);
criterion::criterion_main!(benches);
