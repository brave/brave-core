#![cfg(any())] // This attribute disables the entire module
use criterion::*;

use adblock::cosmetic_filter_cache::CosmeticFilterCache;
use adblock::lists::{parse_filters, FilterFormat};

#[path = "../tests/test_utils.rs"]
mod test_utils;
use test_utils::rules_from_lists;

fn by_hostname(c: &mut Criterion) {
    let mut group = c.benchmark_group("cosmetic-hostname-match");

    group.throughput(Throughput::Elements(1));
    group.sample_size(20);

    group.bench_function("easylist", move |b| {
        let rules = rules_from_lists(&["data/easylist.to/easylist/easylist.txt"]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        b.iter(|| cfcache.hostname_cosmetic_resources("google.com"))
    });
    group.bench_function("many lists", move |b| {
        let rules = rules_from_lists(&[
            "data/easylist.to/easylist/easylist.txt",
            "data/easylist.to/easylistgermany/easylistgermany.txt",
            "data/uBlockOrigin/filters.txt",
            "data/uBlockOrigin/unbreak.txt",
        ]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        b.iter(|| cfcache.hostname_cosmetic_resources("google.com"))
    });
    group.bench_function("complex_hostname", move |b| {
        let rules = rules_from_lists(&[
            "data/easylist.to/easylist/easylist.txt",
            "data/easylist.to/easylistgermany/easylistgermany.txt",
            "data/uBlockOrigin/filters.txt",
            "data/uBlockOrigin/unbreak.txt",
        ]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        b.iter(|| cfcache.hostname_cosmetic_resources("ads.serve.1.domain.google.com"))
    });

    group.finish();
}

fn by_classes_ids(c: &mut Criterion) {
    let mut group = c.benchmark_group("cosmetic-class-id-match");

    group.throughput(Throughput::Elements(1));
    group.sample_size(20);

    group.bench_function("easylist", move |b| {
        let rules = rules_from_lists(&["data/easylist.to/easylist/easylist.txt"]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        let exceptions = Default::default();
        b.iter(|| {
            cfcache.hidden_class_id_selectors(
                &["ad"],
                &["ad"],
                &exceptions,
            )
        })
    });
    group.bench_function("many lists", move |b| {
        let rules = rules_from_lists(&[
            "data/easylist.to/easylist/easylist.txt",
            "data/easylist.to/easylistgermany/easylistgermany.txt",
            "data/uBlockOrigin/filters.txt",
            "data/uBlockOrigin/unbreak.txt",
        ]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        let exceptions = Default::default();
        b.iter(|| {
            cfcache.hidden_class_id_selectors(
                &["ad"],
                &["ad"],
                &exceptions,
            )
        })
    });
    group.bench_function("many matching classes and ids", move |b| {
        let rules = rules_from_lists(&[
            "data/easylist.to/easylist/easylist.txt",
            "data/easylist.to/easylistgermany/easylistgermany.txt",
            "data/uBlockOrigin/filters.txt",
            "data/uBlockOrigin/unbreak.txt",
        ]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        let exceptions = Default::default();
        let class_list = [
            "block-bg-advertisement-region-1",
            "photobox-adbox",
            "headerad-720",
            "rscontainer",
            "rail-article-sponsored",
            "fbPhotoSnowboxAds",
            "sidebar_ad_module",
            "ad-728x90_forum",
            "commercial-unit-desktop-rhs",
            "sponsored-editorial",
            "rr-300x600-ad",
            "adfoot",
            "lads",
        ];
        let id_list = [
            "footer-adspace",
            "adsponsored_links_box",
            "lsadvert-top",
            "mn",
            "col-right-ad",
            "view_ads_bottom_bg_middle",
            "ad_468x60",
            "rightAdColumn",
            "content",
            "rhs_block",
            "center_col",
            "header",
            "advertisingModule160x600",
        ];
        b.iter(|| cfcache.hidden_class_id_selectors(&class_list, &id_list, &exceptions))
    });

    group.finish();
}

criterion_group!(cosmetic_benches, by_hostname, by_classes_ids,);
criterion_main!(cosmetic_benches);
