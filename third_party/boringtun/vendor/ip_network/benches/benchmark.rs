#[macro_use]
extern crate criterion;

use std::net::{Ipv4Addr, Ipv6Addr};
use ip_network::{Ipv4Network, Ipv6Network, IpNetwork};
use criterion::Criterion;
use std::str::FromStr;

fn parse(c: &mut Criterion) {
    c.bench_function("parse ipv4", |b| {
        b.iter(|| "127.0.0.1/32".parse::<Ipv4Network>().unwrap())
    });
    c.bench_function("parse ipv6", |b| {
        b.iter(|| "::1/128".parse::<Ipv6Network>().unwrap())
    });
    c.bench_function("parse ipv6 IpNetwork", |b| {
        b.iter(|| "::1/128".parse::<IpNetwork>().unwrap())
    });
}

fn contains(c: &mut Criterion) {
    let ipv4_network = Ipv4Network::new(Ipv4Addr::new(127, 0, 0, 0), 8).unwrap();
    let ipv6_network = Ipv6Network::new(Ipv6Addr::new(127, 0, 0, 0, 0, 0, 0, 0), 16).unwrap();

    c.bench_function("contains ipv4", move |b| {
        b.iter(|| {
            ipv4_network.contains(Ipv4Addr::new(127, 0, 0, 1));
        })
    });
    c.bench_function("contains ipv6", move |b| {
        b.iter(|| {
            ipv6_network.contains(Ipv6Addr::new(127, 0, 0, 1, 0, 0, 0, 0));
        })
    });
}

fn collapse_addresses(c: &mut Criterion) {
    let ipv4_addresses = [
        Ipv4Network::from_str("1.1.1.0/32").unwrap(),
        Ipv4Network::from_str("1.1.1.1/32").unwrap(),
        Ipv4Network::from_str("1.1.1.2/32").unwrap(),
        Ipv4Network::from_str("1.1.1.3/32").unwrap(),
        Ipv4Network::from_str("1.1.1.4/32").unwrap(),
        Ipv4Network::from_str("1.1.1.0/32").unwrap(),
    ];

    let ipv6_addresses = [
        "2603:1046:1400::/48",
        "2603:1046:1401::/48",
        "2603:1046:1402::/48",
        "2603:1046:1403::/48",
        "2603:1046:1404::/48",
        "2603:1046:1405::/48",
        "2603:1046:1406::/48",
        "2603:1046:1407::/48",
        "2603:1046:1408::/48",
        "2603:1046:140a::/48",
        "2603:1046:140b::/48",
        "2603:1046:1500:10::/64",
        "2603:1046:1500:14::/64",
        "2603:1046:1500:18::/64",
        "2603:1046:1500:1c::/64",
        "2603:1046:1500:20::/64",
        "2603:1046:1500:24::/64",
        "2603:1046:1500:28::/64",
        "2603:1046:1500:2c::/64",
        "2603:1046:1500:30::/64",
        "2603:1046:1500:4::/64",
        "2603:1046:1500:8::/64",
    ]
    .iter()
    .map(|s| Ipv6Network::from_str(s).unwrap())
    .collect::<Vec<_>>();

    c.bench_function("collapse_addresses ipv4", move |b| {
        b.iter(|| {
            Ipv4Network::collapse_addresses(&ipv4_addresses);
        })
    });
    c.bench_function("collapse_addresses ipv6", move |b| {
        b.iter(|| {
            Ipv6Network::collapse_addresses(&ipv6_addresses);
        })
    });
}

criterion_group!(benches, parse, contains, collapse_addresses);
criterion_main!(benches);
