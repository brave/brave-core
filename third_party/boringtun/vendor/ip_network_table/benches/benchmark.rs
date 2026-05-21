use criterion::{black_box, criterion_group, criterion_main, Criterion};
use ip_network::{Ipv4Network, Ipv6Network};
use ip_network_table::IpNetworkTable;
use std::net::{Ipv4Addr, Ipv6Addr};

fn parse(c: &mut Criterion) {
    c.bench_function("insert ipv4", |b| {
        b.iter(|| {
            let mut table = IpNetworkTable::new();
            table.insert(
                Ipv4Network::new(Ipv4Addr::new(black_box(127), 0, 0, 1), 32).unwrap(),
                true,
            );
        })
    });
    c.bench_function("insert ipv6", |b| {
        b.iter(|| {
            let mut table = IpNetworkTable::new();
            table.insert(
                Ipv6Network::new(Ipv6Addr::new(black_box(1), 2, 3, 4, 5, 6, 7, 8), 128).unwrap(),
                true,
            );
        })
    });
}

criterion_group!(benches, parse);
criterion_main!(benches);
