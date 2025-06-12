//! The adblock [`Engine`] is the primary interface for adblocking.

use crate::blocker::{Blocker, BlockerOptions, BlockerResult};
use crate::cosmetic_filter_cache::{CosmeticFilterCache, UrlSpecificResources};
use crate::lists::{FilterSet, ParseOptions};
use crate::regex_manager::RegexManagerDiscardPolicy;
use crate::request::Request;
use crate::resources::{Resource, ResourceStorage};

use std::collections::HashSet;

/// Drives high-level blocking logic and is responsible for loading filter lists into an optimized
/// format that can be queried efficiently.
///
/// For performance optimization reasons, the [`Engine`] is not designed to have rules added or
/// removed after its initial creation. Making changes to the rules loaded is accomplished by
/// creating a new engine to replace it.
///
/// ## Usage
///
/// ### Initialization
///
/// You'll first want to combine all of your filter lists in a [`FilterSet`], which will parse list
/// header metadata. Once all lists have been composed together, you can call
/// [`Engine::from_filter_set`] to start using them for blocking.
///
/// You may also want to supply certain assets for `$redirect` filters and `##+js(...)` scriptlet
/// injections. These are known as [`Resource`]s, and can be provided with
/// [`Engine::use_resources`]. See the [`crate::resources`] module for more information.
///
/// ### Network blocking
///
/// Use the [`Engine::check_network_request`] method to determine how to handle a network request.
///
/// If you _only_ need network blocking, consider using a [`Blocker`] directly.
///
/// ### Cosmetic filtering
///
/// Call [`Engine::url_cosmetic_resources`] to determine what actions should be taken to prepare a
/// particular page before it starts loading.
///
/// Once the page has been loaded, any new CSS classes or ids that appear on the page should be passed to
/// [`Engine::hidden_class_id_selectors`] on an ongoing basis to determine additional elements that
/// should be hidden dynamically.
pub struct Engine {
    blocker: Blocker,
    cosmetic_cache: CosmeticFilterCache,
    resources: ResourceStorage,
}

impl Default for Engine {
    /// Equivalent to `Engine::new(true)`.
    fn default() -> Self {
        Self::new(true)
    }
}

impl Engine {
    /// Creates a new adblocking `Engine`. `Engine`s created without rules should generally only be
    /// used with deserialization.
    /// - `optimize` specifies whether or not to attempt to compress the internal representation by
    ///   combining similar rules.
    pub fn new(optimize: bool) -> Self {
        let blocker_options = BlockerOptions {
            enable_optimizations: optimize,
        };

        Self {
            blocker: Blocker::new(vec![], &blocker_options),
            cosmetic_cache: CosmeticFilterCache::new(),
            resources: ResourceStorage::default(),
        }
    }

    /// Loads rules in a single format, enabling optimizations and discarding debug information.
    pub fn from_rules(
        rules: impl IntoIterator<Item = impl AsRef<str>>,
        opts: ParseOptions,
    ) -> Self {
        let mut filter_set = FilterSet::new(false);
        filter_set.add_filters(rules, opts);
        Self::from_filter_set(filter_set, true)
    }

    /// Loads rules, enabling optimizations and including debug information.
    pub fn from_rules_debug(
        rules: impl IntoIterator<Item = impl AsRef<str>>,
        opts: ParseOptions,
    ) -> Self {
        Self::from_rules_parametrised(rules, opts, true, true)
    }

    pub fn from_rules_parametrised(
        filter_rules: impl IntoIterator<Item = impl AsRef<str>>,
        opts: ParseOptions,
        debug: bool,
        optimize: bool,
    ) -> Self {
        let mut filter_set = FilterSet::new(debug);
        filter_set.add_filters(filter_rules, opts);
        Self::from_filter_set(filter_set, optimize)
    }

    /// Loads rules from the given `FilterSet`. It is recommended to use a `FilterSet` when adding
    /// rules from multiple sources.
    pub fn from_filter_set(set: FilterSet, optimize: bool) -> Self {
        let FilterSet {
            network_filters,
            cosmetic_filters,
            ..
        } = set;

        let blocker_options = BlockerOptions {
            enable_optimizations: optimize,
        };

        Self {
            blocker: Blocker::new(network_filters, &blocker_options),
            cosmetic_cache: CosmeticFilterCache::from_rules(cosmetic_filters),
            resources: ResourceStorage::default(),
        }
    }

    /// Check if a request for a network resource from `url`, of type `request_type`, initiated by
    /// `source_url`, should be blocked.
    pub fn check_network_request(&self, request: &Request) -> BlockerResult {
        self.blocker.check(request, &self.resources)
    }

    pub fn check_network_request_subset(
        &self,
        request: &Request,
        previously_matched_rule: bool,
        force_check_exceptions: bool,
    ) -> BlockerResult {
        self.blocker.check_parameterised(
            request,
            &self.resources,
            previously_matched_rule,
            force_check_exceptions,
        )
    }

    /// Returns a string containing any additional CSP directives that should be added to this
    /// request's response. Only applies to document and subdocument requests.
    ///
    /// If multiple policies are present from different rules, they will be joined by commas.
    pub fn get_csp_directives(&self, request: &Request) -> Option<String> {
        self.blocker.get_csp_directives(request)
    }

    /// Sets this engine's tags to be _only_ the ones provided in `tags`.
    ///
    /// Tags can be used to cheaply enable or disable network rules with a corresponding `$tag`
    /// option.
    pub fn use_tags(&mut self, tags: &[&str]) {
        self.blocker.use_tags(tags);
    }

    /// Sets this engine's tags to additionally include the ones provided in `tags`.
    ///
    /// Tags can be used to cheaply enable or disable network rules with a corresponding `$tag`
    /// option.
    pub fn enable_tags(&mut self, tags: &[&str]) {
        self.blocker.enable_tags(tags);
    }

    /// Sets this engine's tags to no longer include the ones provided in `tags`.
    ///
    /// Tags can be used to cheaply enable or disable network rules with a corresponding `$tag`
    /// option.
    pub fn disable_tags(&mut self, tags: &[&str]) {
        self.blocker.disable_tags(tags);
    }

    /// Checks if a given tag exists in this engine.
    ///
    /// Tags can be used to cheaply enable or disable network rules with a corresponding `$tag`
    /// option.
    pub fn tag_exists(&self, tag: &str) -> bool {
        self.blocker.tags_enabled().contains(&tag.to_owned())
    }

    /// Sets this engine's resources to be _only_ the ones provided in `resources`.
    pub fn use_resources(&mut self, resources: impl IntoIterator<Item = Resource>) {
        self.resources = ResourceStorage::from_resources(resources);
    }

    /// Sets this engine's resources to additionally include `resource`.
    pub fn add_resource(
        &mut self,
        resource: Resource,
    ) -> Result<(), crate::resources::AddResourceError> {
        self.resources.add_resource(resource)
    }

    // Cosmetic filter functionality

    /// If any of the provided CSS classes or ids could cause a certain generic CSS hide rule
    /// (i.e. `{ display: none !important; }`) to be required, this method will return a list of
    /// CSS selectors corresponding to rules referencing those classes or ids, provided that the
    /// corresponding rules are not excepted.
    ///
    /// `exceptions` should be passed directly from `UrlSpecificResources`.
    pub fn hidden_class_id_selectors(
        &self,
        classes: impl IntoIterator<Item = impl AsRef<str>>,
        ids: impl IntoIterator<Item = impl AsRef<str>>,
        exceptions: &HashSet<String>,
    ) -> Vec<String> {
        self.cosmetic_cache
            .hidden_class_id_selectors(classes, ids, exceptions)
    }

    /// Returns a set of cosmetic filter resources required for a particular url. Once this has
    /// been called, all CSS ids and classes on a page should be passed to
    /// `hidden_class_id_selectors` to obtain any stylesheets consisting of generic rules (if the
    /// returned `generichide` value is false).
    pub fn url_cosmetic_resources(&self, url: &str) -> UrlSpecificResources {
        let request = if let Ok(request) = Request::new(url, url, "document") {
            request
        } else {
            return UrlSpecificResources::empty();
        };

        let generichide = self.blocker.check_generic_hide(&request);
        self.cosmetic_cache.hostname_cosmetic_resources(
            &self.resources,
            &request.hostname,
            generichide,
        )
    }

    pub fn set_regex_discard_policy(&mut self, new_discard_policy: RegexManagerDiscardPolicy) {
        self.blocker.set_regex_discard_policy(new_discard_policy);
    }

    #[cfg(feature = "regex-debug-info")]
    pub fn discard_regex(&mut self, regex_id: u64) {
        self.blocker.discard_regex(regex_id);
    }

    #[cfg(feature = "regex-debug-info")]
    pub fn get_regex_debug_info(&self) -> crate::regex_manager::RegexDebugInfo {
        self.blocker.get_regex_debug_info()
    }

    /// Serializes the `Engine` into a binary format so that it can be quickly reloaded later.
    pub fn serialize(&self) -> Result<Vec<u8>, crate::data_format::SerializationError> {
        crate::data_format::serialize_engine(&self.blocker, &self.cosmetic_cache)
    }

    /// Deserialize the `Engine` from the binary format generated by `Engine::serialize`.
    ///
    /// Note that the binary format has a built-in version number that may be incremented. There is
    /// no guarantee that later versions of the format will be deserializable across minor versions
    /// of adblock-rust; the format is provided only as a caching optimization.
    pub fn deserialize(
        &mut self,
        serialized: &[u8],
    ) -> Result<(), crate::data_format::DeserializationError> {
        let current_tags = self.blocker.tags_enabled();
        let (blocker, cosmetic_cache) = crate::data_format::deserialize_engine(serialized)?;
        self.blocker = blocker;
        self.blocker
            .use_tags(&current_tags.iter().map(|s| &**s).collect::<Vec<_>>());
        self.cosmetic_cache = cosmetic_cache;
        Ok(())
    }
}

/// Static assertions for `Engine: Send + Sync` traits.
#[cfg(not(feature = "unsync-regex-caching"))]
fn _assertions() {
    fn _assert_send<T: Send>() {}
    fn _assert_sync<T: Sync>() {}

    _assert_send::<Engine>();
    _assert_sync::<Engine>();
}

#[cfg(test)]
#[path = "../tests/unit/engine.rs"]
mod unit_tests;
