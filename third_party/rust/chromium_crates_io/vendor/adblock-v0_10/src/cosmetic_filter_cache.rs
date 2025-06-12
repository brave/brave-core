//! Provides behavior related to cosmetic filtering - that is, modifying a page's contents after
//! it's been loaded into a browser. This is primarily used to hide or clean up unwanted page
//! elements that are served inline with the rest of the first-party content from a page, but can
//! also be used to inject JavaScript "scriptlets" that intercept and modify the behavior of
//! scripts on the page at runtime.
//!
//! The primary API exposed by this module is the `CosmeticFilterCache` struct, which stores
//! cosmetic filters and allows them to be queried efficiently at runtime for any which may be
//! relevant to a particular page.

use crate::filters::cosmetic::{
    CosmeticFilter, CosmeticFilterAction, CosmeticFilterMask, CosmeticFilterOperator,
};
use crate::resources::{PermissionMask, ResourceStorage};
use crate::utils::Hash;

use std::collections::{HashMap, HashSet};

use memchr::memchr as find_char;
use serde::{Deserialize, Serialize};

/// Contains cosmetic filter information intended to be used on a particular URL.
#[derive(Debug, PartialEq, Eq, Deserialize, Serialize)]
pub struct UrlSpecificResources {
    /// `hide_selectors` is a set of any CSS selector on the page that should be hidden, i.e.
    /// styled as `{ display: none !important; }`.
    pub hide_selectors: HashSet<String>,
    /// Set of JSON-encoded procedural filters or filters with an action.
    pub procedural_actions: HashSet<String>,
    /// `exceptions` is a set of any class or id CSS selectors that should not have generic rules
    /// applied. In practice, these should be passed to `class_id_stylesheet` and not used
    /// otherwise.
    pub exceptions: HashSet<String>,
    /// `injected_script` is the Javascript code for any scriptlets that should be injected into
    /// the page.
    pub injected_script: String,
    /// `generichide` is set to true if there is a corresponding `$generichide` exception network
    /// filter. If so, the page should not query for additional generic rules using
    /// `hidden_class_id_selectors`.
    pub generichide: bool,
}

impl UrlSpecificResources {
    pub fn empty() -> Self {
        Self {
            hide_selectors: HashSet::new(),
            procedural_actions: HashSet::new(),
            exceptions: HashSet::new(),
            injected_script: String::new(),
            generichide: false,
        }
    }
}

/// The main engine driving cosmetic filtering.
///
/// There are two primary methods that should be considered when using this in a browser:
/// `hidden_class_id_selectors`, and `url_cosmetic_resources`.
///
/// Note that cosmetic filtering is imprecise and that this structure is intenionally designed for
/// efficient querying in the context of a browser, optimizing for low memory usage in the page
/// context and good performance. It is *not* designed to provide a 100% accurate report of what
/// will be blocked on any particular page, although when used correctly, all provided rules and
/// scriptlets should be safe to apply.
pub(crate) struct CosmeticFilterCache {
    /// Rules that are just the CSS class of an element to be hidden on all sites, e.g. `##.ad`.
    pub(crate) simple_class_rules: HashSet<String>,
    /// Rules that are just the CSS id of an element to be hidden on all sites, e.g. `###banner`.
    pub(crate) simple_id_rules: HashSet<String>,
    /// Rules that are the CSS selector of an element to be hidden on all sites, starting with a
    /// class, e.g. `##.ad image`.
    pub(crate) complex_class_rules: HashMap<String, Vec<String>>,
    /// Rules that are the CSS selector of an element to be hidden on all sites, starting with an
    /// id, e.g. `###banner > .text a`.
    pub(crate) complex_id_rules: HashMap<String, Vec<String>>,

    pub(crate) specific_rules: HostnameRuleDb,

    /// Rules that are the CSS selector of an element to be hidden on all sites that do not fit
    /// into any of the class or id buckets above, e.g. `##a[href="https://malware.com"]`
    pub(crate) misc_generic_selectors: HashSet<String>,
}

impl CosmeticFilterCache {
    pub fn new() -> Self {
        Self {
            simple_class_rules: HashSet::new(),
            simple_id_rules: HashSet::new(),
            complex_class_rules: HashMap::new(),
            complex_id_rules: HashMap::new(),

            specific_rules: HostnameRuleDb::default(),

            misc_generic_selectors: HashSet::new(),
        }
    }

    pub fn from_rules(rules: Vec<CosmeticFilter>) -> Self {
        let mut self_ = Self {
            simple_class_rules: HashSet::with_capacity(rules.len() / 2),
            simple_id_rules: HashSet::with_capacity(rules.len() / 2),
            complex_class_rules: HashMap::with_capacity(rules.len() / 2),
            complex_id_rules: HashMap::with_capacity(rules.len() / 2),

            specific_rules: HostnameRuleDb::default(),

            misc_generic_selectors: HashSet::with_capacity(rules.len() / 30),
        };

        for rule in rules {
            self_.add_filter(rule)
        }

        self_
    }

    pub fn add_filter(&mut self, rule: CosmeticFilter) {
        if rule.has_hostname_constraint() {
            if let Some(generic_rule) = rule.hidden_generic_rule() {
                self.add_generic_filter(generic_rule);
            }
            self.specific_rules.store_rule(rule);
        } else {
            self.add_generic_filter(rule);
        }
    }

    /// Add a filter, assuming it has already been determined to be a generic rule
    fn add_generic_filter(&mut self, rule: CosmeticFilter) {
        let selector = match rule.plain_css_selector() {
            Some(s) => s.to_string(),
            None => {
                // Procedural cosmetic filters cannot be generic.
                // Silently ignoring this filter.
                return;
            }
        };

        if selector.starts_with('.') {
            if let Some(key) = key_from_selector(&selector) {
                assert!(key.starts_with('.'));
                let class = key[1..].to_string();
                if key == selector {
                    self.simple_class_rules.insert(class);
                } else if let Some(bucket) = self.complex_class_rules.get_mut(&class) {
                    bucket.push(selector);
                } else {
                    self.complex_class_rules.insert(class, vec![selector]);
                }
            }
        } else if selector.starts_with('#') {
            if let Some(key) = key_from_selector(&selector) {
                assert!(key.starts_with('#'));
                let id = key[1..].to_string();
                if key == selector {
                    self.simple_id_rules.insert(id);
                } else if let Some(bucket) = self.complex_id_rules.get_mut(&id) {
                    bucket.push(selector);
                } else {
                    self.complex_id_rules.insert(id, vec![selector]);
                }
            }
        } else {
            self.misc_generic_selectors.insert(selector);
        }
    }

    /// Generic class/id rules are by far the most common type of cosmetic filtering rule, and they
    /// apply to all sites. Rather than injecting all of these rules onto every page, which would
    /// blow up memory usage, we only inject rules based on classes and ids that actually appear on
    /// the page (in practice, a `MutationObserver` is used to identify those elements). We can
    /// include rules like `.a-class div#ads > .advertisement`, keyed by the `.a-class` selector,
    /// since we know that this rule cannot possibly apply unless there is an `.a-class` element on
    /// the page.
    ///
    /// This method returns all of the generic CSS selectors of elements to hide (i.e. with a
    /// `display: none !important` CSS rule) that could possibly be or become relevant to the page
    /// given the new classes and ids that have appeared on the page. It guarantees that it will be
    /// safe to hide those elements on a particular page by taking into account the page's
    /// hostname-specific set of exception rules.
    ///
    /// The exceptions should be returned directly as they appear in the page's
    /// `UrlSpecificResources`. The exceptions, along with the set of already-seen classes and ids,
    /// must be cached externally as the cosmetic filtering subsystem here is designed to be
    /// stateless with regard to active page sessions.
    pub fn hidden_class_id_selectors(
        &self,
        classes: impl IntoIterator<Item = impl AsRef<str>>,
        ids: impl IntoIterator<Item = impl AsRef<str>>,
        exceptions: &HashSet<String>,
    ) -> Vec<String> {
        let mut selectors = vec![];

        classes.into_iter().for_each(|class| {
            let class = class.as_ref();
            if self.simple_class_rules.contains(class)
                && !exceptions.contains(&format!(".{}", class))
            {
                selectors.push(format!(".{}", class));
            }
            if let Some(bucket) = self.complex_class_rules.get(class) {
                selectors.extend(
                    bucket
                        .iter()
                        .filter(|sel| !exceptions.contains(*sel))
                        .map(|s| s.to_owned()),
                );
            }
        });
        ids.into_iter().for_each(|id| {
            let id = id.as_ref();
            if self.simple_id_rules.contains(id) && !exceptions.contains(&format!("#{}", id)) {
                selectors.push(format!("#{}", id));
            }
            if let Some(bucket) = self.complex_id_rules.get(id) {
                selectors.extend(
                    bucket
                        .iter()
                        .filter(|sel| !exceptions.contains(*sel))
                        .map(|s| s.to_owned()),
                );
            }
        });

        selectors
    }

    /// Any rules that can't be handled by `hidden_class_id_selectors` are returned by
    /// `hostname_cosmetic_resources`. As soon as a page navigation is committed, this method
    /// should be queried to get the initial set of cosmetic filtering operations to apply to the
    /// page. This provides any rules specifying elements to hide by selectors that are too complex
    /// to be returned by `hidden_class_id_selectors` (i.e. not directly starting with a class or
    /// id selector, like `div[class*="Ads"]`), or any rule that is only applicable to a particular
    /// hostname or set of hostnames (like `example.com##.a-class`). The first category is always
    /// injected into every page, and makes up a relatively small number of rules in practice.
    pub fn hostname_cosmetic_resources(
        &self,
        resources: &ResourceStorage,
        hostname: &str,
        generichide: bool,
    ) -> UrlSpecificResources {
        let domain_str = {
            let (start, end) = crate::url_parser::get_host_domain(hostname);
            &hostname[start..end]
        };

        let (request_entities, request_hostnames) = hostname_domain_hashes(hostname, domain_str);

        let mut specific_hide_selectors = HashSet::new();
        let mut procedural_actions = HashSet::new();
        let mut script_injections = HashMap::<&str, PermissionMask>::new();
        let mut exceptions = HashSet::new();

        let mut except_all_scripts = false;

        let hashes: Vec<&Hash> = request_entities
            .iter()
            .chain(request_hostnames.iter())
            .collect();

        fn populate_set(
            hash: &Hash,
            source_bin: &HostnameFilterBin<String>,
            dest_set: &mut HashSet<String>,
        ) {
            if let Some(s) = source_bin.get(hash) {
                s.iter().for_each(|s| {
                    dest_set.insert(s.to_owned());
                });
            }
        }
        for hash in hashes.iter() {
            populate_set(
                hash,
                &self.specific_rules.hide,
                &mut specific_hide_selectors,
            );
            populate_set(
                hash,
                &self.specific_rules.procedural_action,
                &mut procedural_actions,
            );
            // special behavior: `script_injections` doesn't have to own the strings yet, since the
            // scripts need to be fetched and templated later
            if let Some(s) = self.specific_rules.inject_script.get(hash) {
                s.iter().for_each(|(s, mask)| {
                    script_injections
                        .entry(s)
                        .and_modify(|entry| *entry |= *mask)
                        .or_insert(*mask);
                });
            }
        }

        fn prune_set(
            hash: &Hash,
            source_bin: &HostnameFilterBin<String>,
            dest_set: &mut HashSet<String>,
        ) {
            if let Some(s) = source_bin.get(hash) {
                s.iter().for_each(|s| {
                    dest_set.remove(s);
                });
            }
        }
        for hash in hashes.iter() {
            // special behavior: unhide rules need to go in `exceptions` as well
            if let Some(s) = self.specific_rules.unhide.get(hash) {
                s.iter().for_each(|s| {
                    specific_hide_selectors.remove(s);
                    exceptions.insert(s.to_owned());
                });
            }
            prune_set(
                hash,
                &self.specific_rules.procedural_action_exception,
                &mut procedural_actions,
            );
            // same logic but not using prune_set since strings are unowned, (see above)
            if let Some(s) = self.specific_rules.uninject_script.get(hash) {
                for s in s {
                    if s.is_empty() {
                        except_all_scripts = true;
                        script_injections.clear();
                    }
                    if except_all_scripts {
                        continue;
                    }
                    script_injections.remove(s.as_str());
                }
            }
        }

        let hide_selectors = if generichide {
            specific_hide_selectors
        } else {
            let mut hide_selectors = self
                .misc_generic_selectors
                .difference(&exceptions)
                .cloned()
                .collect::<HashSet<_>>();
            specific_hide_selectors.into_iter().for_each(|sel| {
                hide_selectors.insert(sel);
            });
            hide_selectors
        };

        let injected_script = resources.get_scriptlet_resources(script_injections);

        UrlSpecificResources {
            hide_selectors,
            procedural_actions,
            exceptions,
            injected_script,
            generichide,
        }
    }
}

/// Each hostname-specific filter can be pointed to by several different hostnames, and each
/// hostname can correspond to several different filters. To effectively store and access those
/// filters by hostname, all the non-hostname information for filters is stored in per-hostname
/// "buckets" within a Vec, and each bucket is identified by its index. Hostname hashes are used as
/// keys to get the indices of relevant buckets, which are in turn used to retrieve all the filters
/// that apply.
#[derive(Default)]
pub(crate) struct HostnameFilterBin<T>(pub HashMap<Hash, Vec<T>>);

impl<T> HostnameFilterBin<T> {
    pub fn insert(&mut self, token: &Hash, filter: T) {
        if let Some(bucket) = self.0.get_mut(token) {
            bucket.push(filter);
        } else {
            self.0.insert(*token, vec![filter]);
        }
    }

    fn get(&self, token: &Hash) -> Option<&Vec<T>> {
        self.0.get(token)
    }
}

impl HostnameFilterBin<String> {
    /// Convenience method that serializes to JSON
    pub fn insert_procedural_action_filter(&mut self, token: &Hash, f: &ProceduralOrActionFilter) {
        self.insert(token, serde_json::to_string(f).unwrap());
    }
}

/// Holds filter bins categorized by filter type.
#[derive(Default)]
pub(crate) struct HostnameRuleDb {
    /// Simple hostname-specific hide rules, e.g. `example.com##.ad`.
    ///
    /// The parameter is the rule's CSS selector.
    pub hide: HostnameFilterBin<String>,
    /// Simple hostname-specific hide exception rules, e.g. `example.com#@#.ad`.
    ///
    /// The parameter is the rule's CSS selector.
    pub unhide: HostnameFilterBin<String>,
    /// Hostname-specific rules with a scriptlet to inject along with any arguments, e.g.
    /// `example.com##+js(acis, Number.isNan)`.
    ///
    /// The parameter is the contents of the `+js(...)` syntax construct.
    pub inject_script: HostnameFilterBin<(String, PermissionMask)>,
    /// Hostname-specific rules to except a scriptlet to inject along with any arguments, e.g.
    /// `example.com#@#+js(acis, Number.isNan)`.
    ///
    /// The parameter is the contents of the `+js(...)` syntax construct.
    ///
    /// In practice, these rules are extremely rare in filter lists.
    pub uninject_script: HostnameFilterBin<String>,
    /// Procedural filters and/or filters with a [`CosmeticFilterAction`].
    ///
    /// Each is a [`ProceduralOrActionFilter`] struct serialized as JSON.
    pub procedural_action: HostnameFilterBin<String>,
    /// Exceptions for procedural filters and/or filters with a [`CosmeticFilterAction`].
    ///
    /// Each is a [`ProceduralOrActionFilter`] struct serialized as JSON.
    pub procedural_action_exception: HostnameFilterBin<String>,
}

/// Representations of filters with complex behavior that relies on in-page JS logic.
///
/// These get stored in-memory as JSON and should be deserialized/acted on by a content script.
/// JSON is pragmatic here since there are relatively fewer of these type of rules, and they will
/// be handled by in-page JS anyways.
#[derive(Deserialize, Serialize, Clone)]
pub struct ProceduralOrActionFilter {
    /// A selector for elements that this filter applies to.
    /// This may be a plain CSS selector, or it can consist of multiple procedural operators.
    pub selector: Vec<CosmeticFilterOperator>,
    /// An action to apply to matching elements.
    /// If no action is present, the filter assumes default behavior of hiding the element with
    /// a style of `display: none !important`.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub action: Option<CosmeticFilterAction>,
}

impl ProceduralOrActionFilter {
    /// Returns `(selector, style)` if the filter can be expressed in pure CSS.
    pub fn as_css(&self) -> Option<(String, String)> {
        match (&self.selector[..], &self.action) {
            ([CosmeticFilterOperator::CssSelector(selector)], None) => {
                Some((selector.to_string(), "display: none !important".to_string()))
            }
            (
                [CosmeticFilterOperator::CssSelector(selector)],
                Some(CosmeticFilterAction::Style(style)),
            ) => Some((selector.to_string(), style.to_string())),
            _ => None,
        }
    }

    /// Convenience constructor for pure CSS style filters.
    pub(crate) fn from_css(selector: String, style: String) -> Self {
        Self {
            selector: vec![CosmeticFilterOperator::CssSelector(selector)],
            action: Some(CosmeticFilterAction::Style(style)),
        }
    }
}

impl HostnameRuleDb {
    pub fn store_rule(&mut self, rule: CosmeticFilter) {
        use SpecificFilterType::*;

        let unhide = rule.mask.contains(CosmeticFilterMask::UNHIDE);
        let script_inject = rule.mask.contains(CosmeticFilterMask::SCRIPT_INJECT);

        let kind = match (
            script_inject,
            rule.plain_css_selector().map(|s| s.to_string()),
            rule.action,
        ) {
            (false, Some(selector), None) => Hide(selector),
            (true, Some(selector), None) => InjectScript((selector, rule.permission)),
            (false, selector, action) => ProceduralOrAction(
                serde_json::to_string(&ProceduralOrActionFilter {
                    selector: selector
                        .map(|selector| vec![CosmeticFilterOperator::CssSelector(selector)])
                        .unwrap_or(rule.selector),
                    action,
                })
                .unwrap(),
            ),
            (true, _, Some(_)) => return, // script injection with action - shouldn't be possible
            (true, None, _) => return, // script injection without plain CSS selector - shouldn't be possible
        };

        let kind = if unhide { kind.negated() } else { kind };

        let tokens_to_insert = std::iter::empty()
            .chain(rule.hostnames.unwrap_or_default())
            .chain(rule.entities.unwrap_or_default());

        tokens_to_insert.for_each(|t| self.store(&t, kind.clone()));

        let tokens_to_insert_negated = std::iter::empty()
            .chain(rule.not_hostnames.unwrap_or_default())
            .chain(rule.not_entities.unwrap_or_default());

        let negated = kind.negated();

        tokens_to_insert_negated.for_each(|t| self.store(&t, negated.clone()));
    }

    fn store(&mut self, token: &Hash, kind: SpecificFilterType) {
        use SpecificFilterType::*;

        match kind {
            Hide(s) => self.hide.insert(token, s),
            Unhide(s) => self.unhide.insert(token, s),
            InjectScript(s) => self.inject_script.insert(token, s),
            UninjectScript((s, _)) => self.uninject_script.insert(token, s),
            ProceduralOrAction(s) => self.procedural_action.insert(token, s),
            ProceduralOrActionException(s) => self.procedural_action_exception.insert(token, s),
        }
    }
}

/// Exists to use common logic for binning filters correctly
#[derive(Clone)]
enum SpecificFilterType {
    Hide(String),
    Unhide(String),
    InjectScript((String, PermissionMask)),
    UninjectScript((String, PermissionMask)),
    ProceduralOrAction(String),
    ProceduralOrActionException(String),
}

impl SpecificFilterType {
    fn negated(self) -> Self {
        match self {
            Self::Hide(s) => Self::Unhide(s),
            Self::Unhide(s) => Self::Hide(s),
            Self::InjectScript(s) => Self::UninjectScript(s),
            Self::UninjectScript(s) => Self::InjectScript(s),
            Self::ProceduralOrAction(s) => Self::ProceduralOrActionException(s),
            Self::ProceduralOrActionException(s) => Self::ProceduralOrAction(s),
        }
    }
}

fn hostname_domain_hashes(hostname: &str, domain: &str) -> (Vec<Hash>, Vec<Hash>) {
    let request_entities =
        crate::filters::cosmetic::get_entity_hashes_from_labels(hostname, domain);
    let request_hostnames =
        crate::filters::cosmetic::get_hostname_hashes_from_labels(hostname, domain);

    (request_entities, request_hostnames)
}

/// Returns the first token of a CSS selector.
///
/// This should only be called once `selector` has been verified to start with either a "#" or "."
/// character.
fn key_from_selector(selector: &str) -> Option<String> {
    use once_cell::sync::Lazy;
    use regex::Regex;

    static RE_PLAIN_SELECTOR: Lazy<Regex> = Lazy::new(|| Regex::new(r"^[#.][\w\\-]+").unwrap());
    static RE_PLAIN_SELECTOR_ESCAPED: Lazy<Regex> =
        Lazy::new(|| Regex::new(r"^[#.](?:\\[0-9A-Fa-f]+ |\\.|\w|-)+").unwrap());
    static RE_ESCAPE_SEQUENCE: Lazy<Regex> =
        Lazy::new(|| Regex::new(r"\\([0-9A-Fa-f]+ |.)").unwrap());

    // If there are no escape characters in the selector, just take the first class or id token.
    let mat = RE_PLAIN_SELECTOR.find(selector);
    if let Some(location) = mat {
        let key = &location.as_str();
        if find_char(b'\\', key.as_bytes()).is_none() {
            return Some((*key).into());
        }
    } else {
        return None;
    }

    // Otherwise, the characters in the selector must be escaped.
    let mat = RE_PLAIN_SELECTOR_ESCAPED.find(selector);
    if let Some(location) = mat {
        let mut key = String::with_capacity(selector.len());
        let escaped = &location.as_str();
        let mut beginning = 0;
        let mat = RE_ESCAPE_SEQUENCE.captures_iter(escaped);
        for capture in mat {
            // Unwrap is safe because the 0th capture group is the match itself
            let location = capture.get(0).unwrap();
            key += &escaped[beginning..location.start()];
            beginning = location.end();
            // Unwrap is safe because there is a capture group specified in the regex
            let capture = capture.get(1).unwrap().as_str();
            if capture.chars().count() == 1 {
                // Check number of unicode characters rather than byte length
                key += capture;
            } else {
                // This u32 conversion can overflow
                let codepoint = u32::from_str_radix(&capture[..capture.len() - 1], 16).ok()?;

                // Not all u32s are valid Unicode codepoints
                key += &core::char::from_u32(codepoint)?.to_string();
            }
        }
        Some(key + &escaped[beginning..])
    } else {
        None
    }
}

#[cfg(test)]
#[path = "../tests/unit/cosmetic_filter_cache.rs"]
mod unit_tests;
