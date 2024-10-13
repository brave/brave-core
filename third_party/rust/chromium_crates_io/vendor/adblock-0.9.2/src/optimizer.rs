use crate::filters::network::{FilterPart, NetworkFilter, NetworkFilterMask};
use itertools::*;
use std::collections::HashMap;

trait Optimization {
    fn fusion(&self, filters: &[NetworkFilter]) -> NetworkFilter;
    fn group_by_criteria(&self, filter: &NetworkFilter) -> String;
    fn select(&self, filter: &NetworkFilter) -> bool;
}

/// Fuse `NetworkFilter`s together by applying optimizations sequentially.
pub fn optimize(filters: Vec<NetworkFilter>) -> Vec<NetworkFilter> {
    let mut optimized: Vec<NetworkFilter> = Vec::new();

    /*
    let union_domain_group = UnionDomainGroup {};
    let (mut fused, unfused) = apply_optimisation(&union_domain_group, filters);
    optimized.append(&mut fused);
    */

    let simple_pattern_group = SimplePatternGroup {};
    let (mut fused, mut unfused) = apply_optimisation(&simple_pattern_group, filters);
    optimized.append(&mut fused);

    // Append whatever is still left unfused
    optimized.append(&mut unfused);

    // Re-sort the list, now that the order has been perturbed
    optimized.sort_by_key(|f| f.id);
    optimized
}

fn apply_optimisation<T: Optimization>(
    optimization: &T,
    filters: Vec<NetworkFilter>,
) -> (Vec<NetworkFilter>, Vec<NetworkFilter>) {
    let (positive, mut negative): (Vec<NetworkFilter>, Vec<NetworkFilter>) =
        filters.into_iter().partition_map(|f| {
            if optimization.select(&f) {
                Either::Left(f)
            } else {
                Either::Right(f)
            }
        });

    let mut to_fuse: HashMap<String, Vec<NetworkFilter>> = HashMap::with_capacity(positive.len());
    positive
        .into_iter()
        .for_each(|f| insert_dup(&mut to_fuse, optimization.group_by_criteria(&f), f));

    let mut fused = Vec::with_capacity(to_fuse.len());
    for (_, group) in to_fuse {
        if group.len() > 1 {
            // println!("Fusing {} filters together", group.len());
            fused.push(optimization.fusion(group.as_slice()));
        } else {
            group.into_iter().for_each(|f| negative.push(f));
        }
    }

    fused.shrink_to_fit();

    (fused, negative)
}

fn insert_dup<K, V>(map: &mut HashMap<K, Vec<V>>, k: K, v: V)
where
    K: std::cmp::Ord + std::hash::Hash,
{
    map.entry(k).or_insert_with(Vec::new).push(v)
}

struct SimplePatternGroup {}

impl Optimization for SimplePatternGroup {
    // Group simple patterns, into a single filter

    fn fusion(&self, filters: &[NetworkFilter]) -> NetworkFilter {
        let base_filter = &filters[0]; // FIXME: can technically panic, if filters list is empty
        let mut filter = base_filter.clone();

        // if any filter is empty (meaning matches anything), the entire combiation matches anything
        if filters
            .iter()
            .any(|f| matches!(f.filter, FilterPart::Empty))
        {
            filter.filter = FilterPart::Empty
        } else {
            let mut flat_patterns: Vec<String> = Vec::with_capacity(filters.len());
            for f in filters {
                match &f.filter {
                    FilterPart::Empty => (),
                    FilterPart::Simple(s) => flat_patterns.push(s.clone()),
                    FilterPart::AnyOf(s) => flat_patterns.extend_from_slice(s),
                }
            }

            if flat_patterns.is_empty() {
                filter.filter = FilterPart::Empty;
            } else if flat_patterns.len() == 1 {
                filter.filter = FilterPart::Simple(flat_patterns[0].clone())
            } else {
                filter.filter = FilterPart::AnyOf(flat_patterns)
            }
        }

        let is_regex = filters.iter().any(NetworkFilter::is_regex);
        filter.mask.set(NetworkFilterMask::IS_REGEX, is_regex);
        let is_complete_regex = filters.iter().any(|f| f.is_complete_regex());
        filter
            .mask
            .set(NetworkFilterMask::IS_COMPLETE_REGEX, is_complete_regex);

        if base_filter.raw_line.is_some() {
            filter.raw_line = Some(Box::new(
                filters
                    .iter()
                    .flat_map(|f| f.raw_line.clone())
                    .join(" <+> "),
            ))
        }

        filter
    }

    fn group_by_criteria(&self, filter: &NetworkFilter) -> String {
        format!("{:b}:{:?}", filter.mask, filter.is_complete_regex())
    }
    fn select(&self, filter: &NetworkFilter) -> bool {
        filter.opt_domains.is_none()
            && filter.opt_not_domains.is_none()
            && !filter.is_hostname_anchor()
            && !filter.is_redirect()
            && !filter.is_csp()
    }
}

/*
struct UnionDomainGroup {}

impl Optimization for UnionDomainGroup {
    fn fusion(&self, filters: &[NetworkFilter]) -> NetworkFilter {
        let base_filter = &filters[0]; // FIXME: can technically panic, if filters list is empty
        let mut filter = base_filter.clone();
        let mut domains = HashSet::new();
        let mut not_domains = HashSet::new();

        filters.iter().for_each(|f| {
            if let Some(opt_domains) = f.opt_domains.as_ref() {
                for d in opt_domains {
                    domains.insert(d);
                }
            }
            if let Some(opt_not_domains) = f.opt_not_domains.as_ref() {
                for d in opt_not_domains {
                    not_domains.insert(d);
                }
            }
        });

        if !domains.is_empty() {
            let mut domains = domains.into_iter().cloned().collect::<Vec<_>>();
            domains.sort_unstable();
            let opt_domains_union = Some(domains.iter().fold(0, |acc, x| acc | x));
            filter.opt_domains = Some(domains);
            filter.opt_domains_union = opt_domains_union;
        }
        if !not_domains.is_empty() {
            let mut domains = not_domains.into_iter().cloned().collect::<Vec<_>>();
            domains.sort_unstable();
            let opt_not_domains_union = Some(domains.iter().fold(0, |acc, x| acc | x));
            filter.opt_not_domains = Some(domains);
            filter.opt_not_domains_union = opt_not_domains_union;
        }

        if base_filter.raw_line.is_some() {
            filter.raw_line = Some(Box::new(
                filters
                    .iter()
                    .flat_map(|f| f.raw_line.clone())
                    .join(" <+> "),
            ))
        }

        filter
    }

    fn group_by_criteria(&self, filter: &NetworkFilter) -> String {
        format!(
            "{:?}:{}:{:b}:{:?}",
            filter.hostname.as_ref(),
            filter.filter.string_view().unwrap_or_default(),
            filter.mask,
            filter.modifier_option.as_ref()
        )
    }

    fn select(&self, filter: &NetworkFilter) -> bool {
        !filter.is_csp() && (filter.opt_domains.is_some() || filter.opt_not_domains.is_some())
    }
}
*/

#[cfg(test)]
mod optimization_tests_pattern_group {
    use super::*;
    use crate::filters::network::CompiledRegex;
    use crate::filters::network::NetworkMatchable;
    use crate::lists;
    use crate::regex_manager::RegexManager;
    use crate::request::Request;
    use regex::bytes::RegexSetBuilder as BytesRegexSetBuilder;

    fn check_regex_match(regex: &CompiledRegex, pattern: &str, matches: bool) {
        let is_match = regex.is_match(pattern);
        assert!(
            is_match == matches,
            "Expected {} match {} = {}",
            regex.to_string(),
            pattern,
            matches
        );
    }

    fn check_match(
        regex_manager: &mut RegexManager,
        filter: &NetworkFilter,
        url_path: &str,
        matches: bool,
    ) {
        let is_match = filter.matches(&Request::new(
          ("https://example.com/".to_string() + url_path).as_str(),
          "https://google.com",
          ""
        ).unwrap(), regex_manager);
        assert!(
            is_match == matches,
            "Expected {} match {} = {}",
            filter.to_string(),
            url_path,
            matches
        );
    }

    #[test]
    fn regex_set_works() {
        let regex_set = BytesRegexSetBuilder::new(&[
            r"/static/ad\.",
            "/static/ad-",
            "/static/ad/.*",
            "/static/ads/.*",
            "/static/adv/.*",
        ])
        .unicode(false)
        .build();

        let fused_regex = CompiledRegex::CompiledSet(regex_set.unwrap());
        assert!(matches!(fused_regex, CompiledRegex::CompiledSet(_)));
        check_regex_match(&fused_regex, "/static/ad.", true);
        check_regex_match(&fused_regex, "/static/ad-", true);
        check_regex_match(&fused_regex, "/static/ads-", false);
        check_regex_match(&fused_regex, "/static/ad/", true);
        check_regex_match(&fused_regex, "/static/ad", false);
        check_regex_match(&fused_regex, "/static/ad/foobar", true);
        check_regex_match(&fused_regex, "/static/ad/foobar/asd?q=1", true);
        check_regex_match(&fused_regex, "/static/ads/", true);
        check_regex_match(&fused_regex, "/static/ads", false);
        check_regex_match(&fused_regex, "/static/ads/foobar", true);
        check_regex_match(&fused_regex, "/static/ads/foobar/asd?q=1", true);
        check_regex_match(&fused_regex, "/static/adv/", true);
        check_regex_match(&fused_regex, "/static/adv", false);
        check_regex_match(&fused_regex, "/static/adv/foobar", true);
        check_regex_match(&fused_regex, "/static/adv/foobar/asd?q=1", true);
    }

    #[test]
    fn combines_simple_regex_patterns() {
        let rules = [
            "/static/ad-",
            "/static/ad.",
            "/static/ad/*",
            "/static/ads/*",
            "/static/adv/*",
        ];

        let (filters, _) = lists::parse_filters(&rules, true, Default::default());

        let optimization = SimplePatternGroup {};

        filters
            .iter()
            .for_each(|f| assert!(optimization.select(f), "Expected rule to be selected"));

        let fused = optimization.fusion(&filters);

        assert!(fused.is_regex() == false, "Expected rule to not be a regex");
        assert_eq!(
            fused.to_string(),
            "/static/ad- <+> /static/ad. <+> /static/ad/* <+> /static/ads/* <+> /static/adv/*"
        );
        let mut regex_manager = RegexManager::default();
        check_match(&mut regex_manager, &fused, "/static/ad-", true);
        check_match(&mut regex_manager, &fused, "/static/ad.", true);
        check_match(&mut regex_manager, &fused, "/static/ad%", false);
        check_match(&mut regex_manager, &fused, "/static/ads-", false);
        check_match(&mut regex_manager, &fused, "/static/ad/", true);
        check_match(&mut regex_manager, &fused, "/static/ad", false);
        check_match(&mut regex_manager, &fused, "/static/ad/foobar", true);
        check_match(
            &mut regex_manager,
            &fused,
            "/static/ad/foobar/asd?q=1",
            true,
        );
        check_match(&mut regex_manager, &fused, "/static/ads/", true);
        check_match(&mut regex_manager, &fused, "/static/ads", false);
        check_match(&mut regex_manager, &fused, "/static/ads/foobar", true);
        check_match(
            &mut regex_manager,
            &fused,
            "/static/ads/foobar/asd?q=1",
            true,
        );
        check_match(&mut regex_manager, &fused, "/static/adv/", true);
        check_match(&mut regex_manager, &fused, "/static/adv", false);
        check_match(&mut regex_manager, &fused, "/static/adv/foobar", true);
        check_match(
            &mut regex_manager,
            &fused,
            "/static/adv/foobar/asd?q=1",
            true,
        );
    }

    #[test]
    fn separates_pattern_by_grouping() {
        let rules = [
            "/analytics-v1.",
            "/v1/pixel?",
            "/api/v1/stat?",
            "/analytics/v1/*$domain=~my.leadpages.net",
            "/v1/ads/*",
        ];

        let (filters, _) = lists::parse_filters(&rules, true, Default::default());

        let optimization = SimplePatternGroup {};

        let (fused, skipped) = apply_optimisation(&optimization, filters);

        assert_eq!(fused.len(), 1);
        let filter = fused.get(0).unwrap();
        assert_eq!(
            filter.to_string(),
            "/analytics-v1. <+> /v1/pixel? <+> /api/v1/stat? <+> /v1/ads/*"
        );

        assert!(filter.matches_test(
            &Request::new(
                "https://example.com/v1/pixel?",
                "https://my.leadpages.net",
                ""
            )
            .unwrap()
        ));

        assert_eq!(skipped.len(), 1);
        let filter = skipped.get(0).unwrap();
        assert_eq!(
            filter.to_string(),
            "/analytics/v1/*$domain=~my.leadpages.net"
        );

        assert!(filter.matches_test(
            &Request::new(
                "https://example.com/analytics/v1/foobar",
                "https://foo.leadpages.net",
                ""
            )
            .unwrap()
        ))
    }
}

/*
#[cfg(test)]
mod optimization_tests_union_domain {
    use super::*;
    use crate::filters::network::NetworkMatchable;
    use crate::lists;
    use crate::request::Request;
    use crate::utils;

    #[test]
    fn merges_domains() {
        let rules = [
            "/analytics-v1$domain=google.com",
            "/analytics-v1$domain=example.com",
        ];

        let (filters, _) = lists::parse_filters(&rules, true, Default::default());
        let optimization = UnionDomainGroup {};
        let (fused, _) = apply_optimisation(&optimization, filters);

        assert_eq!(fused.len(), 1);
        let filter = fused.get(0).unwrap();
        assert_eq!(
            filter.to_string(),
            "/analytics-v1$domain=google.com <+> /analytics-v1$domain=example.com"
        );

        let expected_domains = vec![
            utils::fast_hash("example.com"),
            utils::fast_hash("google.com"),
        ];
        assert!(filter.opt_domains.is_some());
        let filter_domains = filter.opt_domains.as_ref().unwrap();
        for dom in expected_domains {
            assert!(filter_domains.contains(&dom));
        }

        assert!(
            filter.matches_test(
                &Request::new(
                    "https://example.com/analytics-v1/foobar",
                    "https://google.com",
                    ""
                )
                .unwrap()
            ) == true
        );
        assert!(
            filter.matches_test(
                &Request::new(
                    "https://example.com/analytics-v1/foobar",
                    "https://foo.leadpages.net",
                    ""
                )
                .unwrap()
            ) == false
        );
    }

    #[test]
    fn skips_rules_with_no_domain() {
        let rules = [
            "/analytics-v1$domain=google.com",
            "/analytics-v1$domain=example.com",
            "/analytics-v1",
        ];

        let (filters, _) = lists::parse_filters(&rules, true, Default::default());
        let optimization = UnionDomainGroup {};
        let (_, skipped) = apply_optimisation(&optimization, filters);

        assert_eq!(skipped.len(), 1);
        let filter = skipped.get(0).unwrap();
        assert_eq!(filter.to_string(), "/analytics-v1");
    }

    #[test]
    fn optimises_domains() {
        let rules = [
            "/analytics-v1$domain=google.com",
            "/analytics-v1$domain=example.com",
            "/analytics-v1$domain=exampleone.com|exampletwo.com",
            "/analytics-v1",
        ];

        let (filters, _) = lists::parse_filters(&rules, true, Default::default());

        let optimization = UnionDomainGroup {};

        let (fused, skipped) = apply_optimisation(&optimization, filters);

        assert_eq!(fused.len(), 1);
        let filter = fused.get(0).unwrap();
        assert_eq!(
            filter.to_string(),
            "/analytics-v1$domain=google.com <+> /analytics-v1$domain=example.com <+> /analytics-v1$domain=exampleone.com|exampletwo.com"
        );

        assert_eq!(skipped.len(), 1);
        let skipped_filter = skipped.get(0).unwrap();
        assert_eq!(skipped_filter.to_string(), "/analytics-v1");

        assert!(
            filter.matches_test(
                &Request::new(
                    "https://example.com/analytics-v1/foobar",
                    "https://google.com",
                    ""
                )
                .unwrap()
            ) == true
        );
        assert!(
            filter.matches_test(
                &Request::new(
                    "https://example.com/analytics-v1/foobar",
                    "https://example.com",
                    ""
                )
                .unwrap()
            ) == true
        );
        assert!(
            filter.matches_test(
                &Request::new(
                    "https://example.com/analytics-v1/foobar",
                    "https://exampletwo.com",
                    ""
                )
                .unwrap()
            ) == true
        );
        assert!(
            filter.matches_test(
                &Request::new(
                    "https://example.com/analytics-v1/foobar",
                    "https://foo.leadpages.net",
                    ""
                )
                .unwrap()
            ) == false
        );
    }
}
*/
