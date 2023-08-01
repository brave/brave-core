use criterion::*;
use once_cell::sync::Lazy;

use adblock::blocker::{Blocker, BlockerOptions};
use adblock::utils::{read_file_lines, rules_from_lists};

static DEFAULT_LISTS: Lazy<Vec<String>> = Lazy::new(|| {
    rules_from_lists(&vec![String::from(
        "data/easylist.to/easylist/easylist.txt",
    )])
});
static DEFAULT_RULES_LISTS: Lazy<Vec<Vec<String>>> =
    Lazy::new(|| vec![read_file_lines("data/easylist.to/easylist/easylist.txt")]);

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

fn bench_parsing_impl(lists: &Vec<Vec<String>>) -> usize {
    let mut dummy = 0;

    for list in lists {
        let (network_filters, _) = adblock::lists::parse_filters(list, false, Default::default());
        dummy = dummy + network_filters.len() % 1000000;
    }

    dummy
}

fn list_parse(c: &mut Criterion) {
    let mut group = c.benchmark_group("parse-filters");

    group.throughput(Throughput::Elements(1));
    group.sample_size(10);

    group.bench_function("network filters", |b| {
        b.iter(|| bench_parsing_impl(&DEFAULT_RULES_LISTS))
    });

    group.bench_function("all filters", |b| {
        b.iter(|| bench_parsing_impl(&DEFAULT_RULES_LISTS))
    });

    group.finish();
}

fn get_blocker(rules: &Vec<String>) -> Blocker {
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

    let rules = rules_from_lists(&vec![
        String::from("data/easylist.to/easylist/easylist.txt"),
        String::from("data/easylist.to/easylist/easyprivacy.txt"),
    ]);

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
