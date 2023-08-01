#![cfg(any())] // This attribute disables the entire module
use criterion::*;

use adblock::cosmetic_filter_cache::CosmeticFilterCache;
use adblock::lists::{parse_filters, FilterFormat};
use adblock::utils::rules_from_lists;

fn by_hostname(c: &mut Criterion) {
    let mut group = c.benchmark_group("cosmetic-hostname-match");

    group.throughput(Throughput::Elements(1));
    group.sample_size(20);

    group.bench_function("easylist", move |b| {
        let rules = rules_from_lists(&vec!["data/easylist.to/easylist/easylist.txt".to_owned()]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        b.iter(|| cfcache.hostname_cosmetic_resources("google.com"))
    });
    group.bench_function("many lists", move |b| {
        let rules = rules_from_lists(&vec![
            "data/easylist.to/easylist/easylist.txt".to_owned(),
            "data/easylist.to/easylistgermany/easylistgermany.txt".to_owned(),
            "data/uBlockOrigin/filters.txt".to_owned(),
            "data/uBlockOrigin/unbreak.txt".to_owned(),
        ]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        b.iter(|| cfcache.hostname_cosmetic_resources("google.com"))
    });
    group.bench_function("complex_hostname", move |b| {
        let rules = rules_from_lists(&vec![
            "data/easylist.to/easylist/easylist.txt".to_owned(),
            "data/easylist.to/easylistgermany/easylistgermany.txt".to_owned(),
            "data/uBlockOrigin/filters.txt".to_owned(),
            "data/uBlockOrigin/unbreak.txt".to_owned(),
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
        let rules = rules_from_lists(&vec!["data/easylist.to/easylist/easylist.txt".to_owned()]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        let exceptions = Default::default();
        b.iter(|| {
            cfcache.hidden_class_id_selectors(
                &vec!["ad".to_owned()][..],
                &vec!["ad".to_owned()][..],
                &exceptions,
            )
        })
    });
    group.bench_function("many lists", move |b| {
        let rules = rules_from_lists(&vec![
            "data/easylist.to/easylist/easylist.txt".to_owned(),
            "data/easylist.to/easylistgermany/easylistgermany.txt".to_owned(),
            "data/uBlockOrigin/filters.txt".to_owned(),
            "data/uBlockOrigin/unbreak.txt".to_owned(),
        ]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        let exceptions = Default::default();
        b.iter(|| {
            cfcache.hidden_class_id_selectors(
                &vec!["ad".to_owned()][..],
                &vec!["ad".to_owned()][..],
                &exceptions,
            )
        })
    });
    group.bench_function("many matching classes and ids", move |b| {
        let rules = rules_from_lists(&vec![
            "data/easylist.to/easylist/easylist.txt".to_owned(),
            "data/easylist.to/easylistgermany/easylistgermany.txt".to_owned(),
            "data/uBlockOrigin/filters.txt".to_owned(),
            "data/uBlockOrigin/unbreak.txt".to_owned(),
        ]);
        let (_, cosmetic_filters) = parse_filters(&rules, false, FilterFormat::Standard);
        let cfcache = CosmeticFilterCache::from_rules(cosmetic_filters);
        let exceptions = Default::default();
        let class_list = vec![
            "block-bg-advertisement-region-1".to_owned(),
            "photobox-adbox".to_owned(),
            "headerad-720".to_owned(),
            "rscontainer".to_owned(),
            "rail-article-sponsored".to_owned(),
            "fbPhotoSnowboxAds".to_owned(),
            "sidebar_ad_module".to_owned(),
            "ad-728x90_forum".to_owned(),
            "commercial-unit-desktop-rhs".to_owned(),
            "sponsored-editorial".to_owned(),
            "rr-300x600-ad".to_owned(),
            "adfoot".to_owned(),
            "lads".to_owned(),
        ];
        let id_list = vec![
            "footer-adspace".to_owned(),
            "adsponsored_links_box".to_owned(),
            "lsadvert-top".to_owned(),
            "mn".to_owned(),
            "col-right-ad".to_owned(),
            "view_ads_bottom_bg_middle".to_owned(),
            "ad_468x60".to_owned(),
            "rightAdColumn".to_owned(),
            "content".to_owned(),
            "rhs_block".to_owned(),
            "center_col".to_owned(),
            "header".to_owned(),
            "advertisingModule160x600".to_owned(),
        ];
        b.iter(|| cfcache.hidden_class_id_selectors(&class_list[..], &id_list[..], &exceptions))
    });

    group.finish();
}

criterion_group!(cosmetic_benches, by_hostname, by_classes_ids,);
criterion_main!(cosmetic_benches);
