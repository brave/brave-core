use criterion::*;
use tokio::runtime::Runtime;

use adblock::blocker::{Blocker, BlockerOptions};
use adblock::filters::network::{NetworkFilter, NetworkFilterMask};
use adblock::request::Request;
use adblock::resources::ResourceStorage;

const DEFAULT_LISTS_URL: &str =
    "https://raw.githubusercontent.com/brave/adblock-resources/master/filter_lists/list_catalog.json";

async fn get_all_filters() -> Vec<String> {
    use futures::FutureExt;

    #[derive(serde::Serialize, serde::Deserialize)]
    struct ComponentDescriptor {
        sources: Vec<SourceDescriptor>,
    }

    #[derive(serde::Serialize, serde::Deserialize)]
    struct SourceDescriptor {
        url: String,
    }

    let default_components = reqwest::get(DEFAULT_LISTS_URL)
        .then(|resp| resp.expect("Could not get default filter listing").text())
        .map(|text| {
            serde_json::from_str::<Vec<ComponentDescriptor>>(
                &text.expect("Could not get default filter listing as text"),
            )
            .expect("Could not parse default filter listing JSON")
        })
        .await;

    let filters_fut: Vec<_> = default_components[0]
        .sources
        .iter()
        .map(|list| {
            reqwest::get(&list.url)
                .then(|resp| resp.expect("Could not request rules").text())
                .map(|text| {
                    text.expect("Could not get rules as text")
                        .lines()
                        .map(|s| s.to_owned())
                        .collect::<Vec<_>>()
                })
        })
        .collect();

    futures::future::join_all(filters_fut)
        .await
        .iter()
        .flatten()
        .cloned()
        .collect()
}

/// Gets all rules with redirects, and modifies them to apply to resources at `a{0-n}.com/bad.js`
fn get_redirect_rules() -> Vec<NetworkFilter> {
    let async_runtime = Runtime::new().expect("Could not start Tokio runtime");

    let filters = async_runtime.block_on(get_all_filters());
    let (network_filters, _) = adblock::lists::parse_filters(&filters, true, Default::default());

    network_filters
        .into_iter()
        .filter(NetworkFilter::is_redirect)
        .filter(NetworkFilter::also_block_redirect)
        .filter(|rule| {
            rule.modifier_option.as_ref().unwrap() != "none"
        })
        .enumerate()
        .map(|(index, mut rule)| {
            rule.mask.insert(NetworkFilterMask::IS_LEFT_ANCHOR);
            rule.mask.insert(NetworkFilterMask::IS_RIGHT_ANCHOR);
            rule.hostname = Some(format!("a{}.com/bad.js", index));

            rule.filter = adblock::filters::network::FilterPart::Empty;
            rule.mask.remove(NetworkFilterMask::IS_HOSTNAME_ANCHOR);
            rule.mask.remove(NetworkFilterMask::IS_HOSTNAME_REGEX);
            rule.mask.remove(NetworkFilterMask::IS_REGEX);
            rule.mask.remove(NetworkFilterMask::IS_COMPLETE_REGEX);

            rule
        })
        .collect()
}

/// Loads the supplied rules, and the test set of resources, into a Blocker
fn get_preloaded_blocker(rules: Vec<NetworkFilter>) -> Blocker {
    let blocker_options = BlockerOptions {
        enable_optimizations: true,
    };

    let blocker = Blocker::new(rules, &blocker_options);

    blocker
}

fn build_resources_for_filters(#[allow(unused)] filters: &[NetworkFilter]) -> ResourceStorage {
    let mut resources = ResourceStorage::default();

    #[cfg(feature = "resource-assembler")]
    {
        use std::path::Path;
        use adblock::resources::resource_assembler::assemble_web_accessible_resources;

        let mut resource_data = assemble_web_accessible_resources(
            Path::new("data/test/fake-uBO-files/web_accessible_resources"),
            Path::new("data/test/fake-uBO-files/redirect-resources.js"),
        );
        #[allow(deprecated)]
        resource_data.append(
            &mut adblock::resources::resource_assembler::assemble_scriptlet_resources(Path::new(
                "data/test/fake-uBO-files/scriptlets.js",
            )),
        );

        resource_data
            .into_iter()
            .for_each(|resource| {
                let _res = resources.add_resource(resource);
            });
    }

    #[cfg(not(feature = "resource-assembler"))]
    {
        use adblock::resources::{Resource, ResourceType, MimeType};

        filters
            .iter()
            .filter(|f| f.is_redirect())
            .map(|f| {
                let mut redirect = f.modifier_option.as_ref().unwrap().as_str();
                // strip priority, if present
                if let Some(i) = redirect.rfind(':') {
                    redirect = &redirect[0..i];
                }

                Resource {
                    name: redirect.to_owned(),
                    aliases: vec![],
                    kind: ResourceType::Mime(MimeType::from_extension(&redirect)),
                    content: base64::encode(redirect),
                    dependencies: vec![],
                    permission: Default::default(),
                }
            })
            .for_each(|resource| {
                let _res = resources.add_resource(resource);
            });
    }

    resources
}

/// Maps network filter rules into `Request`s that would trigger those rules
pub fn build_custom_requests(rules: Vec<NetworkFilter>) -> Vec<Request> {
    rules
        .iter()
        .map(|rule| {
            let raw_type = if rule.mask.contains(NetworkFilterMask::FROM_IMAGE) {
                "image"
            } else if rule.mask.contains(NetworkFilterMask::FROM_MEDIA) {
                "media"
            } else if rule.mask.contains(NetworkFilterMask::FROM_OBJECT) {
                "object"
            } else if rule.mask.contains(NetworkFilterMask::FROM_OTHER) {
                "other"
            } else if rule.mask.contains(NetworkFilterMask::FROM_PING) {
                "ping"
            } else if rule.mask.contains(NetworkFilterMask::FROM_SCRIPT) {
                "script"
            } else if rule.mask.contains(NetworkFilterMask::FROM_STYLESHEET) {
                "stylesheet"
            } else if rule.mask.contains(NetworkFilterMask::FROM_SUBDOCUMENT) {
                "subdocument"
            } else if rule.mask.contains(NetworkFilterMask::FROM_DOCUMENT) {
                "main_frame"
            } else if rule.mask.contains(NetworkFilterMask::FROM_XMLHTTPREQUEST) {
                "xhr"
            } else if rule.mask.contains(NetworkFilterMask::FROM_WEBSOCKET) {
                "websocket"
            } else if rule.mask.contains(NetworkFilterMask::FROM_FONT) {
                "font"
            } else {
                unreachable!()
            };

            let rule_hostname = rule.hostname.clone().unwrap();
            let url = format!("https://{}", rule_hostname.clone());
            let domain = &rule_hostname[..rule_hostname.find('/').unwrap()];
            let hostname = domain;

            let raw_line = rule.raw_line.clone().unwrap();
            let source_hostname = if rule.opt_domains.is_some() {
                let domain_start = raw_line.rfind("domain=").unwrap() + "domain=".len();
                let from_start = &raw_line[domain_start..];
                let domain_end = from_start
                    .find('|')
                    .or_else(|| from_start.find(","))
                    .or_else(|| Some(from_start.len()))
                    .unwrap()
                    + domain_start;
                let source_hostname = &raw_line[domain_start..domain_end];

                source_hostname
            } else if rule.mask.contains(NetworkFilterMask::THIRD_PARTY) {
                "always-third-party.com"
            } else {
                hostname
            };

            let source_url = format!("https://{}", source_hostname);

            Request::new(
                &url,
                &source_url,
                raw_type,
            ).unwrap()
        })
        .collect::<Vec<_>>()
}

fn bench_fn(blocker: &Blocker, resources: &ResourceStorage, requests: &[Request]) {
    requests.iter().for_each(|request| {
        let block_result = blocker.check(&request, &resources);
        assert!(block_result.redirect.is_some(), "{:?}, {:?}", request, block_result);
    });
}

fn redirect_performance(c: &mut Criterion) {
    let mut group = c.benchmark_group("redirect_performance");

    let rules = get_redirect_rules();

    let blocker = get_preloaded_blocker(rules.clone());
    let resources = build_resources_for_filters(&rules);
    let requests = build_custom_requests(rules.clone());
    let requests_len = requests.len() as u64;

    group.throughput(Throughput::Elements(requests_len));
    group.sample_size(10);

    group.bench_function("without_alias_lookup", move |b| {
        b.iter(|| bench_fn(&blocker, &resources, &requests))
    });

    group.finish();
}

criterion_group!(benches, redirect_performance,);
criterion_main!(benches);
