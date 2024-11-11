#[macro_use]
extern crate criterion;

use criterion::Criterion;

use winnow::ascii::float;
use winnow::binary::be_u64;
use winnow::error::ErrMode;
use winnow::error::ErrorKind;
use winnow::error::InputError;
use winnow::error::ParserError;
use winnow::prelude::*;
use winnow::stream::ParseSlice;

type Stream<'i> = &'i [u8];

fn parser(i: &mut Stream<'_>) -> PResult<u64> {
    be_u64.parse_next(i)
}

fn number(c: &mut Criterion) {
    let data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];

    parser
        .parse_peek(&data[..])
        .expect("should parse correctly");
    c.bench_function("number", move |b| {
        b.iter(|| parser.parse_peek(&data[..]).unwrap());
    });
}

fn float_bytes(c: &mut Criterion) {
    println!(
        "float_bytes result: {:?}",
        float::<_, f64, InputError<_>>.parse_peek(&b"-1.234E-12"[..])
    );
    c.bench_function("float bytes", |b| {
        b.iter(|| float::<_, f64, InputError<_>>.parse_peek(&b"-1.234E-12"[..]));
    });
}

fn float_str(c: &mut Criterion) {
    println!(
        "float_str result: {:?}",
        float::<_, f64, InputError<_>>.parse_peek("-1.234E-12")
    );
    c.bench_function("float str", |b| {
        b.iter(|| float::<_, f64, InputError<_>>.parse_peek("-1.234E-12"));
    });
}

fn std_float(input: &mut &[u8]) -> PResult<f64> {
    match input.parse_slice() {
        Some(n) => Ok(n),
        None => Err(ErrMode::from_error_kind(input, ErrorKind::Slice)),
    }
}

fn std_float_bytes(c: &mut Criterion) {
    println!(
        "std_float_bytes result: {:?}",
        std_float.parse_peek(&b"-1.234E-12"[..])
    );
    c.bench_function("std_float bytes", |b| {
        b.iter(|| std_float.parse_peek(&b"-1.234E-12"[..]));
    });
}

criterion_group!(benches, number, float_bytes, std_float_bytes, float_str);
criterion_main!(benches);
