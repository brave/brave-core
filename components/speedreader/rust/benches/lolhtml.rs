extern crate reqwest;
extern crate speedreader;
extern crate url;

use criterion::{black_box, criterion_group, criterion_main, Criterion};

fn bench_lolhtml(c: &mut Criterion) {
    let article_url = "https://www.cnet.com/roadshow/features/2020-acura-nsx-road-trip-daytona/";

    let client = reqwest::blocking::Client::new();
    let data = client.get(article_url).send().unwrap().text().unwrap();
    let sr = speedreader::SpeedReader::default();
    let config = sr.get_rewriter_type(article_url);
    assert!(config == speedreader::RewriterType::Streaming);
    let user_data = sr.get_opaque_config(article_url);

    c.bench_function("lolhtml-cnet", |b| {
        b.iter(|| {
            let mut output = vec![];
            let mut rewriter = sr
                .get_rewriter(
                    article_url,
                    &user_data,
                    black_box(|c: &[u8]| output.extend_from_slice(c)),
                    Some(config),
                )
                .unwrap();
            rewriter.write(data.as_bytes()).ok();
            rewriter.end().ok();
        })
    });
}

fn bench_html5ever(c: &mut Criterion) {
    let article_url = "https://www.cnet.com/roadshow/features/2020-acura-nsx-road-trip-daytona/";

    let client = reqwest::blocking::Client::new();
    let data = client.get(article_url).send().unwrap().text().unwrap();
    let sr = speedreader::SpeedReader::with_whitelist(speedreader::whitelist::Whitelist::default());
    let user_data = sr.get_opaque_config(article_url);

    c.bench_function("html5ever-cnet", |b| {
        b.iter(|| {
            let mut output = vec![];
            let mut rewriter = sr
                .get_rewriter(
                    article_url,
                    &user_data,
                    black_box(|c: &[u8]| output.extend_from_slice(c)),
                    Some(speedreader::RewriterType::Heuristics),
                )
                .unwrap();
            rewriter.write(data.as_bytes()).ok();
            rewriter.end().ok();
        })
    });
}

criterion_group!(benches, bench_lolhtml, bench_html5ever);
criterion_main!(benches);
