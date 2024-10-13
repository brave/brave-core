use criterion::*;
use once_cell::sync::Lazy;

use adblock::blocker::{Blocker, BlockerOptions};

#[path = "../tests/test_utils.rs"]
mod test_utils;
use test_utils::rules_from_lists;

static DEFAULT_LISTS: Lazy<Vec<String>> = Lazy::new(|| {
    rules_from_lists(&[
        "data/easylist.to/easylist/easylist.txt",
    ]).collect()
});

fn bench_string_hashing(filters: &Vec<String>) -> adblock::utils::Hash {
    let mut dummy: adblock::utils::Hash = 0;
    for filter in filters {
        dummy = (dummy + adblock::utils::fast_hash(filter)) % 1000000000;
    }
    dummy
}

fn bench_string_tokenize(filters: &Vec<String>) -> usize {
    let mut dummy: usize = 0;
    for filter in filters {
        dummy = (dummy + adblock::utils::tokenize(filter).len()) % 1000000000;
    }
    dummy
}

fn string_hashing(c: &mut Criterion) {
    let mut group = c.benchmark_group("string-hashing");

    group.throughput(Throughput::Elements(1));

    group.bench_function("hash", move |b| {
        b.iter(|| bench_string_hashing(&DEFAULT_LISTS))
    });

    group.finish();
}

fn string_tokenize(c: &mut Criterion) {
    let mut group = c.benchmark_group("string-tokenize");

    group.throughput(Throughput::Elements(1));

    group.bench_function("tokenize", move |b| {
        b.iter(|| bench_string_tokenize(&DEFAULT_LISTS))
    });

    group.finish();
}

fn bench_parsing_impl(lists: &Vec<&Vec<String>>) -> usize {
    let mut dummy = 0;

    for list in lists {
        let (network_filters, _) = adblock::lists::parse_filters(*list, false, Default::default());
        dummy = dummy + network_filters.len() % 1000000;
    }

    dummy
}

fn list_parse(c: &mut Criterion) {
    let mut group = c.benchmark_group("parse-filters");

    group.throughput(Throughput::Elements(1));
    group.sample_size(10);

    group.bench_function("network filters", |b| {
        b.iter(|| bench_parsing_impl(&vec![DEFAULT_LISTS.as_ref()]))
    });

    group.bench_function("all filters", |b| {
        b.iter(|| bench_parsing_impl(&vec![DEFAULT_LISTS.as_ref()]))
    });

    group.finish();
}

fn get_blocker(rules: impl IntoIterator<Item=impl AsRef<str>>) -> Blocker {
    let (network_filters, _) = adblock::lists::parse_filters(rules, false, Default::default());

    println!("Got {} network filters", network_filters.len());

    let blocker_options = BlockerOptions {
        enable_optimizations: true,
    };

    Blocker::new(network_filters, &blocker_options)
}

fn blocker_new(c: &mut Criterion) {
    let mut group = c.benchmark_group("blocker_new");

    group.throughput(Throughput::Elements(1));
    group.sample_size(10);

    let rules: Vec<_> = rules_from_lists(&[
        "data/easylist.to/easylist/easylist.txt",
        "data/easylist.to/easylist/easyprivacy.txt",
    ]).collect();

    group.bench_function("el+ep", move |b| b.iter(|| get_blocker(&rules)));

    group.finish();
}

criterion_group!(
    benches,
    blocker_new,
    list_parse,
    string_hashing,
    string_tokenize
);
criterion_main!(benches);
