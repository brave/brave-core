use criterion::*;

use serde::{Deserialize, Serialize};

use adblock::request::Request;
use adblock::url_parser::parse_url;

#[allow(non_snake_case)]
#[derive(Serialize, Deserialize, Clone)]
struct TestRequest {
    frameUrl: String,
    url: String,
    cpt: String,
}

fn load_requests() -> Vec<TestRequest> {
    adblock::utils::read_file_lines("data/requests.json")
        .into_iter()
        .map(|r| serde_json::from_str(&r))
        .filter_map(Result::ok)
        .collect::<Vec<_>>()
}

fn request_parsing_throughput(c: &mut Criterion) {
    let mut group = c.benchmark_group("throughput-request");

    let requests = load_requests();
    let requests_len = requests.len() as u64;

    group.throughput(Throughput::Elements(requests_len));
    group.sample_size(10);

    group.bench_function("create", move |b| {
        b.iter(|| {
            let mut successful = 0;
            requests.iter().for_each(|r| {
                let req: Result<Request, _> = Request::from_urls(&r.url, &r.frameUrl, &r.cpt);
                if req.is_ok() {
                    successful += 1;
                }
            })
        })
    });

    group.finish();
}

fn request_extract_hostname(c: &mut Criterion) {
    let mut group = c.benchmark_group("throughput-request");

    let requests = load_requests();
    let requests_len = requests.len() as u64;

    group.throughput(Throughput::Elements(requests_len));
    group.sample_size(10);

    group.bench_function("hostname+domain extract", move |b| {
        b.iter(|| {
            let mut successful = 0;
            requests.iter().for_each(|r| {
                if parse_url(&r.url).is_some() {
                    successful += 1;
                }
                if parse_url(&r.frameUrl).is_some() {
                    successful += 1;
                }
            });
        })
    });

    group.finish();
}

fn request_new_throughput(c: &mut Criterion) {
    let mut group = c.benchmark_group("throughput-request");

    let requests = load_requests();
    let requests_len = requests.len() as u64;

    group.throughput(Throughput::Elements(requests_len));
    group.sample_size(10);

    let requests_parsed: Vec<_> = requests
        .iter()
        .map(|r| {
            let url_norm = r.url.to_ascii_lowercase();
            let source_url_norm = r.frameUrl.to_ascii_lowercase();

            let maybe_parsed_url = parse_url(&url_norm);
            if maybe_parsed_url.is_none() {
                return Err("bad url");
            }
            let parsed_url = maybe_parsed_url.unwrap();

            let maybe_parsed_source = parse_url(&source_url_norm);

            if maybe_parsed_source.is_none() {
                Ok((
                    r.cpt.clone(),
                    parsed_url.url.clone(),
                    String::from(parsed_url.schema()),
                    String::from(parsed_url.hostname()),
                    String::from(parsed_url.domain()),
                    String::from(""),
                    String::from(""),
                ))
            } else {
                let parsed_source = maybe_parsed_source.unwrap();
                Ok((
                    r.cpt.clone(),
                    parsed_url.url.clone(),
                    String::from(parsed_url.schema()),
                    String::from(parsed_url.hostname()),
                    String::from(parsed_url.domain()),
                    String::from(parsed_source.hostname()),
                    parsed_source.domain().to_owned(),
                ))
            }
        })
        .filter_map(Result::ok)
        .collect();

    group.bench_function("new", move |b| {
        b.iter(|| {
            let mut successful = 0;
            requests_parsed.iter().for_each(|r| {
                Request::new(&r.0, &r.1, &r.2, &r.3, &r.4, &r.5, &r.6);
                successful += 1;
            });
        })
    });

    group.finish();
}

criterion_group!(
    benches,
    request_new_throughput,
    request_extract_hostname,
    request_parsing_throughput
);
criterion_main!(benches);
