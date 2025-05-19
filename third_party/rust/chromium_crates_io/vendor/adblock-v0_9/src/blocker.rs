//! Holds [`Blocker`], which handles all network-based adblocking queries.

use memchr::{memchr as find_char, memrchr as find_char_reverse};
use once_cell::sync::Lazy;
use serde::Serialize;
use std::collections::HashSet;
use std::ops::DerefMut;

use crate::filters::network::{NetworkFilter, NetworkFilterMaskHelper};
use crate::network_filter_list::NetworkFilterList;
use crate::regex_manager::{RegexManager, RegexManagerDiscardPolicy};
use crate::request::Request;
use crate::resources::ResourceStorage;
use crate::utils::Hash;

/// Options used when constructing a [`Blocker`].
pub struct BlockerOptions {
    pub enable_optimizations: bool,
}

/// Describes how a particular network request should be handled.
#[derive(Debug, Serialize)]
pub struct BlockerResult {
    /// Was a blocking filter matched for this request?
    pub matched: bool,
    /// Important is used to signal that a rule with the `important` option
    /// matched. An `important` match means that exceptions should not apply
    /// and no further checking is neccesary--the request should be blocked
    /// (empty body or cancelled).
    ///
    /// Brave Browser keeps multiple instances of [`Blocker`], so `important`
    /// here is used to correct behaviour between them: checking should stop
    /// instead of moving to the next instance iff an `important` rule matched.
    pub important: bool,
    /// Specifies what to load instead of the original request, rather than
    /// just blocking it outright. This can come from a filter with a `redirect`
    /// or `redirect-rule` option. If present, the field will contain the body
    /// of the redirect to be injected.
    ///
    /// Note that the presence of a redirect does _not_ imply that the request
    /// should be blocked. The `redirect-rule` option can produce a redirection
    /// that's only applied if another blocking filter matches a request.
    pub redirect: Option<String>,
    /// `removeparam` may remove URL parameters. If the original request URL was
    /// modified at all, the new version will be here. This should be used
    /// as long as the request is not blocked.
    pub rewritten_url: Option<String>,
    /// Contains a string representation of any matched exception rule.
    /// Effectively this means that there was a match, but the request should
    /// not be blocked.
    ///
    /// If debugging was _not_ enabled (see [`crate::FilterSet::new`]), this
    /// will only contain a constant `"NetworkFilter"` placeholder string.
    pub exception: Option<String>,
    /// When `matched` is true, this contains a string representation of the
    /// matched blocking rule.
    ///
    /// If debugging was _not_ enabled (see [`crate::FilterSet::new`]), this
    /// will only contain a constant `"NetworkFilter"` placeholder string.
    pub filter: Option<String>,
}

impl Default for BlockerResult {
    fn default() -> BlockerResult {
        BlockerResult {
            matched: false,
            important: false,
            redirect: None,
            rewritten_url: None,
            exception: None,
            filter: None,
        }
    }
}

// only check for tags in tagged and exception rule buckets,
// pass empty set for the rest
static NO_TAGS: Lazy<HashSet<String>> = Lazy::new(HashSet::new);

/// Stores network filters for efficient querying.
pub struct Blocker {
    pub(crate) csp: NetworkFilterList,
    pub(crate) exceptions: NetworkFilterList,
    pub(crate) importants: NetworkFilterList,
    pub(crate) redirects: NetworkFilterList,
    pub(crate) removeparam: NetworkFilterList,
    pub(crate) filters_tagged: NetworkFilterList,
    pub(crate) filters: NetworkFilterList,
    pub(crate) generic_hide: NetworkFilterList,

    // Enabled tags are not serialized - when deserializing, tags of the existing
    // instance (the one we are recreating lists into) are maintained
    pub(crate) tags_enabled: HashSet<String>,
    pub(crate) tagged_filters_all: Vec<NetworkFilter>,

    pub(crate) enable_optimizations: bool,

    // Not serialized
    #[cfg(feature = "unsync-regex-caching")]
    pub(crate) regex_manager: std::cell::RefCell<RegexManager>,
    #[cfg(not(feature = "unsync-regex-caching"))]
    pub(crate) regex_manager: std::sync::Mutex<RegexManager>,
}

impl Blocker {
    /// Decide if a network request (usually from WebRequest API) should be
    /// blocked, redirected or allowed.
    pub fn check(&self, request: &Request, resources: &ResourceStorage) -> BlockerResult {
        self.check_parameterised(request, resources, false, false)
    }

    #[cfg(feature = "unsync-regex-caching")]
    fn borrow_regex_manager(&self) -> std::cell::RefMut<RegexManager> {
        #[allow(unused_mut)]
        let mut manager = self.regex_manager.borrow_mut();

        #[cfg(not(target_arch = "wasm32"))]
        manager.update_time();

        manager
    }

    #[cfg(not(feature = "unsync-regex-caching"))]
    fn borrow_regex_manager(&self) -> std::sync::MutexGuard<RegexManager> {
        let mut manager = self.regex_manager.lock().unwrap();
        manager.update_time();
        manager
    }

    pub fn check_generic_hide(&self, hostname_request: &Request) -> bool {
        let mut regex_manager = self.borrow_regex_manager();
        self.generic_hide
            .check(hostname_request, &HashSet::new(), &mut regex_manager)
            .is_some()
    }

    pub fn check_parameterised(
        &self,
        request: &Request,
        resources: &ResourceStorage,
        matched_rule: bool,
        force_check_exceptions: bool,
    ) -> BlockerResult {
        let mut regex_manager = self.borrow_regex_manager();
        if !request.is_supported {
            return BlockerResult::default();
        }

        // Check the filters in the following order:
        // 1. $important (not subject to exceptions)
        // 2. redirection ($redirect=resource)
        // 3. normal filters - if no match by then
        // 4. exceptions - if any non-important match of forced

        // Always check important filters
        let important_filter = self.importants.check(request, &NO_TAGS, &mut regex_manager);

        // only check the rest of the rules if not previously matched
        let filter = if important_filter.is_none() && !matched_rule {
            self.filters_tagged
                .check(request, &self.tags_enabled, &mut regex_manager)
                .or_else(|| self.filters.check(request, &NO_TAGS, &mut regex_manager))
        } else {
            important_filter
        };

        let exception = match filter.as_ref() {
            // if no other rule matches, only check exceptions if forced to
            None if matched_rule || force_check_exceptions => {
                self.exceptions
                    .check(request, &self.tags_enabled, &mut regex_manager)
            }
            None => None,
            // If matched an important filter, exceptions don't atter
            Some(f) if f.is_important() => None,
            Some(_) => self
                .exceptions
                .check(request, &self.tags_enabled, &mut regex_manager),
        };

        let redirect_filters =
            self.redirects
                .check_all(request, &NO_TAGS, regex_manager.deref_mut());

        // Extract the highest priority redirect directive.
        // 1. Exceptions - can bail immediately if found
        // 2. Find highest priority non-exception redirect
        let redirect_resource = {
            let mut exceptions = vec![];
            for redirect_filter in redirect_filters.iter() {
                if redirect_filter.is_exception() {
                    if let Some(redirect) = redirect_filter.modifier_option.as_ref() {
                        exceptions.push(redirect);
                    }
                }
            }
            let mut resource_and_priority = None;
            for redirect_filter in redirect_filters.iter() {
                if !redirect_filter.is_exception() {
                    if let Some(redirect) = redirect_filter.modifier_option.as_ref() {
                        if !exceptions.contains(&redirect) {
                            // parse redirect + priority
                            let (resource, priority) =
                                if let Some(idx) = find_char_reverse(b':', redirect.as_bytes()) {
                                    let priority_str = &redirect[idx + 1..];
                                    let resource = &redirect[..idx];
                                    if let Ok(priority) = priority_str.parse::<i32>() {
                                        (resource, priority)
                                    } else {
                                        (&redirect[..], 0)
                                    }
                                } else {
                                    (&redirect[..], 0)
                                };
                            if let Some((_, p1)) = resource_and_priority {
                                if priority > p1 {
                                    resource_and_priority = Some((resource, priority));
                                }
                            } else {
                                resource_and_priority = Some((resource, priority));
                            }
                        }
                    }
                }
            }
            resource_and_priority.map(|(r, _)| r)
        };

        let redirect: Option<String> = redirect_resource.and_then(|resource_name| {
            resources.get_redirect_resource(resource_name).or_else(|| {
                // It's acceptable to pass no redirection if no matching resource is loaded.
                // TODO - it may be useful to return a status flag to indicate that this occurred.
                #[cfg(test)]
                eprintln!("Matched rule with redirect option but did not find corresponding resource to send");
                None
            })
        });

        let important = filter.is_some()
            && filter
                .as_ref()
                .map(|f| f.is_important())
                .unwrap_or_else(|| false);

        let rewritten_url = if important {
            None
        } else {
            Self::apply_removeparam(&self.removeparam, request, regex_manager.deref_mut())
        };

        // If something has already matched before but we don't know what, still return a match
        let matched = exception.is_none() && (filter.is_some() || matched_rule);
        BlockerResult {
            matched,
            important,
            redirect,
            rewritten_url,
            exception: exception.as_ref().map(|f| f.to_string()), // copy the exception
            filter: filter.as_ref().map(|f| f.to_string()),       // copy the filter
        }
    }

    fn apply_removeparam(
        removeparam_filters: &NetworkFilterList,
        request: &Request,
        regex_manager: &mut RegexManager,
    ) -> Option<String> {
        /// Represents an `&`-separated argument from a URL query parameter string
        enum QParam<'a> {
            /// Just a key, e.g. `...&key&...`
            KeyOnly(&'a str),
            /// Key-value pair separated by an equal sign, e.g. `...&key=value&...`
            KeyValue(&'a str, &'a str),
        }

        impl<'a> std::fmt::Display for QParam<'a> {
            fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
                match self {
                    Self::KeyOnly(k) => write!(f, "{}", k),
                    Self::KeyValue(k, v) => write!(f, "{}={}", k, v),
                }
            }
        }

        let url = &request.original_url;
        // Only check for removeparam if there's a query string in the request URL
        if let Some(i) = find_char(b'?', url.as_bytes()) {
            // String indexing safety: indices come from `.len()` or `find_char` on individual ASCII
            // characters (1 byte each), some plus 1.
            let params_start = i + 1;
            let hash_index = if let Some(j) = find_char(b'#', url[params_start..].as_bytes()) {
                params_start + j
            } else {
                url.len()
            };
            let qparams = &url[params_start..hash_index];
            let mut params: Vec<(QParam, bool)> = qparams
                .split('&')
                .map(|pair| {
                    if let Some((k, v)) = pair.split_once('=') {
                        QParam::KeyValue(k, v)
                    } else {
                        QParam::KeyOnly(pair)
                    }
                })
                .map(|param| (param, true))
                .collect();

            let filters = removeparam_filters.check_all(request, &NO_TAGS, regex_manager);
            let mut rewrite = false;
            for removeparam_filter in filters {
                if let Some(removeparam) = &removeparam_filter.modifier_option {
                    params.iter_mut().for_each(|(param, include)| {
                        if let QParam::KeyValue(k, v) = param {
                            if !v.is_empty() && k == removeparam {
                                *include = false;
                                rewrite = true;
                            }
                        }
                    });
                }
            }
            if rewrite {
                let p = itertools::join(
                    params
                        .into_iter()
                        .filter(|(_, include)| *include)
                        .map(|(param, _)| param.to_string()),
                    "&",
                );
                let new_param_str = if p.is_empty() {
                    String::from("")
                } else {
                    format!("?{}", p)
                };
                Some(format!(
                    "{}{}{}",
                    &url[0..i],
                    new_param_str,
                    &url[hash_index..]
                ))
            } else {
                None
            }
        } else {
            None
        }
    }

    /// Given a "main_frame" or "subdocument" request, check if some content security policies
    /// should be injected in the page.
    pub fn get_csp_directives(&self, request: &Request) -> Option<String> {
        use crate::request::RequestType;

        if request.request_type != RequestType::Document
            && request.request_type != RequestType::Subdocument
        {
            return None;
        }

        let mut regex_manager = self.borrow_regex_manager();
        let filters = self
            .csp
            .check_all(request, &self.tags_enabled, &mut regex_manager);

        if filters.is_empty() {
            return None;
        }

        let mut disabled_directives: HashSet<&str> = HashSet::new();
        let mut enabled_directives: HashSet<&str> = HashSet::new();

        for filter in filters.iter() {
            if filter.is_exception() {
                if filter.is_csp() {
                    if let Some(csp_directive) = &filter.modifier_option {
                        disabled_directives.insert(csp_directive);
                    } else {
                        // Exception filters with empty `csp` options will disable all CSP
                        // injections for matching pages.
                        return None;
                    }
                }
            } else if filter.is_csp() {
                if let Some(csp_directive) = &filter.modifier_option {
                    enabled_directives.insert(csp_directive);
                }
            }
        }

        let mut remaining_directives = enabled_directives.difference(&disabled_directives);

        let mut merged = if let Some(directive) = remaining_directives.next() {
            String::from(*directive)
        } else {
            return None;
        };

        remaining_directives.for_each(|directive| {
            merged.push(',');
            merged.push_str(directive);
        });

        Some(merged)
    }

    pub fn new(network_filters: Vec<NetworkFilter>, options: &BlockerOptions) -> Self {
        // Capacity of filter subsets estimated based on counts in EasyList and EasyPrivacy - if necessary
        // the Vectors will grow beyond the pre-set capacity, but it is more efficient to allocate all at once
        // $csp=
        let mut csp = Vec::with_capacity(200);
        // @@filter
        let mut exceptions = Vec::with_capacity(network_filters.len() / 8);
        // $important
        let mut importants = Vec::with_capacity(200);
        // $redirect, $redirect-rule
        let mut redirects = Vec::with_capacity(200);
        // $removeparam
        let mut removeparam = Vec::with_capacity(60);
        // $tag=
        let mut tagged_filters_all = Vec::with_capacity(200);
        // $badfilter
        let mut badfilters = Vec::with_capacity(100);
        // $generichide
        let mut generic_hide = Vec::with_capacity(4000);
        // All other filters
        let mut filters = Vec::with_capacity(network_filters.len());

        // Injections
        // TODO: resource handling

        if !network_filters.is_empty() {
            for filter in network_filters.iter() {
                if filter.is_badfilter() {
                    badfilters.push(filter);
                }
            }
            let badfilter_ids: HashSet<Hash> = badfilters
                .iter()
                .map(|f| f.get_id_without_badfilter())
                .collect();
            for filter in network_filters {
                // skip any bad filters
                let filter_id = filter.get_id();
                if badfilter_ids.contains(&filter_id) || filter.is_badfilter() {
                    continue;
                }

                // Redirects are independent of blocking behavior.
                if filter.is_redirect() {
                    redirects.push(filter.clone());
                }

                if filter.is_csp() {
                    csp.push(filter);
                } else if filter.is_removeparam() {
                    removeparam.push(filter);
                } else if filter.is_generic_hide() {
                    generic_hide.push(filter);
                } else if filter.is_exception() {
                    exceptions.push(filter);
                } else if filter.is_important() {
                    importants.push(filter);
                } else if filter.tag.is_some() && !filter.is_redirect() {
                    // `tag` + `redirect` is unsupported for now.
                    tagged_filters_all.push(filter);
                } else {
                    if (filter.is_redirect() && filter.also_block_redirect())
                        || !filter.is_redirect()
                    {
                        filters.push(filter);
                    }
                }
            }
        }

        tagged_filters_all.shrink_to_fit();

        Self {
            csp: NetworkFilterList::new(csp, options.enable_optimizations),
            exceptions: NetworkFilterList::new(exceptions, options.enable_optimizations),
            importants: NetworkFilterList::new(importants, options.enable_optimizations),
            redirects: NetworkFilterList::new(redirects, options.enable_optimizations),
            // Don't optimize removeparam, since it can fuse filters without respecting distinct
            // queryparam values
            removeparam: NetworkFilterList::new(removeparam, false),
            filters_tagged: NetworkFilterList::new(Vec::new(), options.enable_optimizations),
            filters: NetworkFilterList::new(filters, options.enable_optimizations),
            generic_hide: NetworkFilterList::new(generic_hide, options.enable_optimizations),
            // Tags special case for enabling/disabling them dynamically
            tags_enabled: HashSet::new(),
            tagged_filters_all,
            // Options
            enable_optimizations: options.enable_optimizations,
            regex_manager: Default::default(),
        }
    }

    pub fn use_tags(&mut self, tags: &[&str]) {
        let tag_set: HashSet<String> = tags.iter().map(|&t| String::from(t)).collect();
        self.tags_with_set(tag_set);
    }

    pub fn enable_tags(&mut self, tags: &[&str]) {
        let tag_set: HashSet<String> = tags
            .iter()
            .map(|&t| String::from(t))
            .collect::<HashSet<_>>()
            .union(&self.tags_enabled)
            .cloned()
            .collect();
        self.tags_with_set(tag_set);
    }

    pub fn disable_tags(&mut self, tags: &[&str]) {
        let tag_set: HashSet<String> = self
            .tags_enabled
            .difference(&tags.iter().map(|&t| String::from(t)).collect())
            .cloned()
            .collect();
        self.tags_with_set(tag_set);
    }

    fn tags_with_set(&mut self, tags_enabled: HashSet<String>) {
        self.tags_enabled = tags_enabled;
        let filters: Vec<NetworkFilter> = self
            .tagged_filters_all
            .iter()
            .filter(|n| n.tag.is_some() && self.tags_enabled.contains(n.tag.as_ref().unwrap()))
            .cloned()
            .collect();
        self.filters_tagged = NetworkFilterList::new(filters, self.enable_optimizations);
    }

    pub fn tags_enabled(&self) -> Vec<String> {
        self.tags_enabled.iter().cloned().collect()
    }

    pub fn set_regex_discard_policy(&self, new_discard_policy: RegexManagerDiscardPolicy) {
        let mut regex_manager = self.borrow_regex_manager();
        regex_manager.set_discard_policy(new_discard_policy);
    }

    #[cfg(feature = "regex-debug-info")]
    pub fn discard_regex(&self, regex_id: u64) {
        let mut regex_manager = self.borrow_regex_manager();
        regex_manager.discard_regex(regex_id);
    }

    #[cfg(feature = "regex-debug-info")]
    pub fn get_regex_debug_info(&self) -> crate::regex_manager::RegexDebugInfo {
        let regex_manager = self.borrow_regex_manager();
        regex_manager.get_debug_info()
    }
}

#[cfg(test)]
#[path = "../tests/unit/blocker.rs"]
mod unit_tests;
