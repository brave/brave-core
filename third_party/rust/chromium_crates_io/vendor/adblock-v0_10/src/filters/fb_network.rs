//! Flatbuffer-compatible versions of [NetworkFilter] and related functionality.

use std::collections::HashMap;
use std::vec;

use flatbuffers::WIPOffset;

use crate::filters::network::{
    NetworkFilter, NetworkFilterMask, NetworkFilterMaskHelper, NetworkMatchable,
};
use crate::filters::unsafe_tools::{fb_vector_to_slice, VerifiedFlatFilterListMemory};

use crate::network_filter_list::NetworkFilterList;
use crate::regex_manager::RegexManager;
use crate::request::Request;
use crate::utils::{Hash, ShortHash};

#[allow(unknown_lints)]
#[allow(
    dead_code,
    clippy::all,
    unused_imports,
    unsafe_code,
    mismatched_lifetime_syntaxes
)]
#[path = "../flatbuffers/fb_network_filter_generated.rs"]
pub mod flat;
use flat::fb;

/// Builder for [NetworkFilterList].
pub(crate) struct FlatNetworkFiltersListBuilder<'a> {
    builder: flatbuffers::FlatBufferBuilder<'a>,
    filters: Vec<WIPOffset<fb::NetworkFilter<'a>>>,

    unique_domains_hashes: Vec<Hash>,
    unique_domains_hashes_map: HashMap<Hash, u32>,
}

impl FlatNetworkFiltersListBuilder<'_> {
    pub fn new() -> Self {
        Self {
            builder: flatbuffers::FlatBufferBuilder::new(),
            filters: vec![],
            unique_domains_hashes: vec![],
            unique_domains_hashes_map: HashMap::new(),
        }
    }

    fn get_or_insert_unique_domain_hash(&mut self, h: &Hash) -> u32 {
        if let Some(&index) = self.unique_domains_hashes_map.get(h) {
            return index;
        }
        let index = self.unique_domains_hashes.len() as u32;
        self.unique_domains_hashes.push(*h);
        self.unique_domains_hashes_map.insert(*h, index);
        index
    }

    pub fn add(&mut self, network_filter: &NetworkFilter) -> u32 {
        let opt_domains = network_filter.opt_domains.as_ref().map(|v| {
            let mut o: Vec<u32> = v
                .iter()
                .map(|x| self.get_or_insert_unique_domain_hash(x))
                .collect();
            o.sort_unstable();
            o.dedup();
            self.builder.create_vector(&o)
        });

        let opt_not_domains = network_filter.opt_not_domains.as_ref().map(|v| {
            let mut o: Vec<u32> = v
                .iter()
                .map(|x| self.get_or_insert_unique_domain_hash(x))
                .collect();
            o.sort_unstable();
            o.dedup();
            self.builder.create_vector(&o)
        });

        let modifier_option = network_filter
            .modifier_option
            .as_ref()
            .map(|s| self.builder.create_string(s));

        let hostname = network_filter
            .hostname
            .as_ref()
            .map(|s| self.builder.create_string(s));

        let tag = network_filter
            .tag
            .as_ref()
            .map(|s| self.builder.create_string(s));

        let patterns = if network_filter.filter.iter().len() > 0 {
            let offsets: Vec<WIPOffset<&str>> = network_filter
                .filter
                .iter()
                .map(|s| self.builder.create_string(s))
                .collect();
            Some(self.builder.create_vector(&offsets))
        } else {
            None
        };

        let raw_line = network_filter
            .raw_line
            .as_ref()
            .map(|v| self.builder.create_string(v.as_str()));

        let filter = fb::NetworkFilter::create(
            &mut self.builder,
            &fb::NetworkFilterArgs {
                mask: network_filter.mask.bits(),
                patterns,
                modifier_option,
                opt_domains,
                opt_not_domains,
                hostname,
                tag,
                raw_line,
            },
        );

        self.filters.push(filter);
        u32::try_from(self.filters.len() - 1).expect("< u32::MAX")
    }

    pub fn finish(
        &mut self,
        mut filter_map: HashMap<ShortHash, Vec<u32>>,
    ) -> VerifiedFlatFilterListMemory {
        let unique_domains_hashes = self.builder.create_vector(&self.unique_domains_hashes);

        let len = filter_map.len();

        // Convert filter_map keys to a sorted vector of (hash, filter_indices).
        let mut entries: Vec<_> = filter_map.drain().collect();
        entries.sort_unstable_by_key(|(k, _)| *k);

        // Convert sorted_entries to two flatbuffers vectors.
        let mut flat_index: Vec<ShortHash> = Vec::with_capacity(len);
        let mut flat_values: Vec<_> = Vec::with_capacity(len);
        for (key, filter_indices) in entries {
            for &filter_index in &filter_indices {
                flat_index.push(key);
                flat_values.push(self.filters[filter_index as usize]);
            }
        }

        let filter_map_index = self.builder.create_vector(&flat_index);
        let filter_map_values = self.builder.create_vector(&flat_values);

        let storage = fb::NetworkFilterList::create(
            &mut self.builder,
            &fb::NetworkFilterListArgs {
                filter_map_index: Some(filter_map_index),
                filter_map_values: Some(filter_map_values),
                unique_domains_hashes: Some(unique_domains_hashes),
            },
        );
        self.builder.finish(storage, None);

        // TODO: consider using builder.collapse() to avoid reallocating memory.
        VerifiedFlatFilterListMemory::from_builder(&self.builder)
    }
}

/// A list of string parts that can be matched against a URL.
pub(crate) struct FlatPatterns<'a> {
    patterns: Option<flatbuffers::Vector<'a, flatbuffers::ForwardsUOffset<&'a str>>>,
}

impl<'a> FlatPatterns<'a> {
    #[inline(always)]
    pub fn new(
        patterns: Option<flatbuffers::Vector<'a, flatbuffers::ForwardsUOffset<&'a str>>>,
    ) -> Self {
        Self { patterns }
    }

    #[inline(always)]
    pub fn iter(&self) -> FlatPatternsIterator<'_> {
        FlatPatternsIterator {
            patterns: self,
            len: self.patterns.map_or(0, |d| d.len()),
            index: 0,
        }
    }
}

/// Iterator over [FlatPatterns].
pub(crate) struct FlatPatternsIterator<'a> {
    patterns: &'a FlatPatterns<'a>,
    len: usize,
    index: usize,
}

impl<'a> Iterator for FlatPatternsIterator<'a> {
    type Item = &'a str;

    #[inline(always)]
    fn next(&mut self) -> Option<Self::Item> {
        self.patterns.patterns.and_then(|fi| {
            if self.index < self.len {
                self.index += 1;
                Some(fi.get(self.index - 1))
            } else {
                None
            }
        })
    }
}

impl ExactSizeIterator for FlatPatternsIterator<'_> {
    #[inline(always)]
    fn len(&self) -> usize {
        self.len
    }
}

/// Internal implementation of [NetworkFilter] that is compatible with flatbuffers.
pub(crate) struct FlatNetworkFilter<'a> {
    key: u64,
    owner: &'a NetworkFilterList,
    fb_filter: &'a fb::NetworkFilter<'a>,

    pub(crate) mask: NetworkFilterMask,
}

impl<'a> FlatNetworkFilter<'a> {
    #[inline(always)]
    pub fn new(
        filter: &'a fb::NetworkFilter<'a>,
        index: usize,
        owner: &'a NetworkFilterList,
    ) -> Self {
        let list_address: *const NetworkFilterList = owner as *const NetworkFilterList;

        Self {
            fb_filter: filter,
            key: index as u64 | (((list_address) as u64) << 32),
            mask: NetworkFilterMask::from_bits_retain(filter.mask()),
            owner,
        }
    }

    #[inline(always)]
    pub fn tag(&self) -> Option<&'a str> {
        self.fb_filter.tag()
    }

    #[inline(always)]
    pub fn modifier_option(&self) -> Option<String> {
        self.fb_filter.modifier_option().map(|o| o.to_string())
    }

    #[inline(always)]
    pub fn include_domains(&self) -> Option<&[u32]> {
        self.fb_filter
            .opt_domains()
            .map(|data| fb_vector_to_slice(data))
    }

    #[inline(always)]
    pub fn exclude_domains(&self) -> Option<&[u32]> {
        self.fb_filter
            .opt_not_domains()
            .map(|data| fb_vector_to_slice(data))
    }

    #[inline(always)]
    pub fn hostname(&self) -> Option<&'a str> {
        if self.mask.is_hostname_anchor() {
            self.fb_filter.hostname()
        } else {
            None
        }
    }

    #[inline(always)]
    pub fn patterns(&self) -> FlatPatterns<'_> {
        FlatPatterns::new(self.fb_filter.patterns())
    }

    #[inline(always)]
    pub fn raw_line(&self) -> Option<String> {
        self.fb_filter.raw_line().map(|v| v.to_string())
    }
}

impl NetworkFilterMaskHelper for FlatNetworkFilter<'_> {
    #[inline]
    fn has_flag(&self, v: NetworkFilterMask) -> bool {
        self.mask.contains(v)
    }
}

impl NetworkMatchable for FlatNetworkFilter<'_> {
    fn matches(&self, request: &Request, regex_manager: &mut RegexManager) -> bool {
        use crate::filters::network_matchers::{
            check_excluded_domains_mapped, check_included_domains_mapped, check_options,
            check_pattern,
        };
        if !check_options(self.mask, request) {
            return false;
        }
        if !check_included_domains_mapped(
            self.include_domains(),
            request,
            &self.owner.unique_domains_hashes_map,
        ) {
            return false;
        }
        if !check_excluded_domains_mapped(
            self.exclude_domains(),
            request,
            &self.owner.unique_domains_hashes_map,
        ) {
            return false;
        }
        check_pattern(
            self.mask,
            self.patterns().iter(),
            self.hostname(),
            self.key,
            request,
            regex_manager,
        )
    }

    #[cfg(test)]
    fn matches_test(&self, request: &Request) -> bool {
        self.matches(request, &mut RegexManager::default())
    }
}
