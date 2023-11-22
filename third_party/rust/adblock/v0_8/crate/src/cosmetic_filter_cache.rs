//! Provides behavior related to cosmetic filtering - that is, modifying a page's contents after
//! it's been loaded into a browser. This is primarily used to hide or clean up unwanted page
//! elements that are served inline with the rest of the first-party content from a page, but can
//! also be used to inject JavaScript "scriptlets" that intercept and modify the behavior of
//! scripts on the page at runtime.
//!
//! The primary API exposed by this module is the `CosmeticFilterCache` struct, which stores
//! cosmetic filters and allows them to be queried efficiently at runtime for any which may be
//! relevant to a particular page.

use crate::filters::cosmetic::CosmeticFilter;
use crate::filters::cosmetic::CosmeticFilterMask;
use crate::resources::{PermissionMask, ResourceStorage};
use crate::utils::Hash;

use std::collections::{HashMap, HashSet};

use serde::{Deserialize, Serialize};

/// Contains cosmetic filter information intended to be used on a particular URL.
#[derive(Debug, PartialEq, Eq, Deserialize, Serialize)]
pub struct UrlSpecificResources {
    /// `hide_selectors` is a set of any CSS selector on the page that should be hidden, i.e.
    /// styled as `{ display: none !important; }`.
    pub hide_selectors: HashSet<String>,
    /// `style_selectors` is a map of CSS selectors on the page to respective non-hide style rules,
    /// i.e. any required styles other than `display: none`.
    pub style_selectors: HashMap<String, Vec<String>>,
    /// `remove_selectors` is a set of any CSS selector on the page that should be removed from the
    /// DOM.
    pub remove_selectors: HashSet<String>,
    /// `remove_attrs` is a map of CSS selectors on the page to respective HTML attributes that
    /// should be removed from matching elements.
    pub remove_attrs: HashMap<String, Vec<String>>,
    /// `remove_attrs` is a map of CSS selectors on the page to respective CSS classes that should
    /// be removed from matching elements.
    pub remove_classes: HashMap<String, Vec<String>>,
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
            style_selectors: HashMap::new(),
            remove_selectors: HashSet::new(),
            remove_attrs: HashMap::new(),
            remove_classes: HashMap::new(),
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
        if rule.mask.contains(CosmeticFilterMask::IS_CLASS_SELECTOR) {
            if let Some(key) = &rule.key {
                let key = key.clone();
                if rule.mask.contains(CosmeticFilterMask::IS_SIMPLE) {
                    self.simple_class_rules.insert(key);
                } else {
                    if let Some(bucket) = self.complex_class_rules.get_mut(&key) {
                        bucket.push(rule.selector);
                    } else {
                        self.complex_class_rules.insert(key, vec![rule.selector]);
                    }
                }
            }
        } else if rule.mask.contains(CosmeticFilterMask::IS_ID_SELECTOR) {
            if let Some(key) = &rule.key {
                let key = key.clone();
                if rule.mask.contains(CosmeticFilterMask::IS_SIMPLE) {
                    self.simple_id_rules.insert(key);
                } else {
                    if let Some(bucket) = self.complex_id_rules.get_mut(&key) {
                        bucket.push(rule.selector);
                    } else {
                        self.complex_id_rules.insert(key, vec![rule.selector]);
                    }
                }
            }
        } else {
            self.misc_generic_selectors.insert(rule.selector);
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
        classes: impl IntoIterator<Item=impl AsRef<str>>,
        ids: impl IntoIterator<Item=impl AsRef<str>>,
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
                selectors.extend(bucket.iter().filter(|sel| !exceptions.contains(*sel)).map(|s| s.to_owned()));
            }
        });
        ids.into_iter().for_each(|id| {
            let id = id.as_ref();
            if self.simple_id_rules.contains(id) && !exceptions.contains(&format!("#{}", id)) {
                selectors.push(format!("#{}", id));
            }
            if let Some(bucket) = self.complex_id_rules.get(id) {
                selectors.extend(bucket.iter().filter(|sel| !exceptions.contains(*sel)).map(|s| s.to_owned()));
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
        let mut style_selectors = HashMap::<_, Vec<_>>::new();
        let mut remove_selectors = HashSet::new();
        let mut remove_attrs = HashMap::<_, Vec<_>>::new();
        let mut remove_classes = HashMap::<_, Vec<_>>::new();
        let mut script_injections = HashMap::<&str, PermissionMask>::new();
        let mut exceptions = HashSet::new();

        let mut except_all_scripts = false;

        let hashes: Vec<&Hash> = request_entities.iter().chain(request_hostnames.iter()).collect();

        fn populate_set(hash: &Hash, source_bin: &HostnameFilterBin<String>, dest_set: &mut HashSet<String>) {
            if let Some(s) = source_bin.get(hash) {
                s.iter().for_each(|s| { dest_set.insert(s.to_owned()); });
            }
        }
        fn populate_map(hash: &Hash, source_bin: &HostnameFilterBin<(String, String)>, dest_map: &mut HashMap<String, Vec<String>>) {
            if let Some(s) = source_bin.get(hash) {
                s.iter().for_each(|s| {
                    dest_map.entry(s.0.to_owned()).and_modify(|v| v.push(s.1.to_owned())).or_insert_with(|| vec![s.1.to_owned()]);
                });
            }
        }
        for hash in hashes.iter() {
            populate_set(hash, &self.specific_rules.hide, &mut specific_hide_selectors);
            populate_set(hash, &self.specific_rules.remove, &mut remove_selectors);
            // special behavior: `script_injections` doesn't have to own the strings yet, since the
            // scripts need to be fetched and templated later
            if let Some(s) = self.specific_rules.inject_script.get(hash) {
                s.iter().for_each(|(s, mask)| {
                    script_injections.entry(s).and_modify(|entry| *entry |= *mask).or_insert(*mask);
                });
            }

            populate_map(hash, &self.specific_rules.style, &mut style_selectors);
            populate_map(hash, &self.specific_rules.remove_attr, &mut remove_attrs);
            populate_map(hash, &self.specific_rules.remove_class, &mut remove_classes);
        }

        fn prune_set(hash: &Hash, source_bin: &HostnameFilterBin<String>, dest_set: &mut HashSet<String>) {
            if let Some(s) = source_bin.get(hash) {
                s.iter().for_each(|s| {
                    dest_set.remove(s);
                });
            }
        }
        fn prune_map(hash: &Hash, source_bin: &HostnameFilterBin<(String, String)>, dest_map: &mut HashMap<String, Vec<String>>) {
            if let Some(s) = source_bin.get(hash) {
                s.iter().for_each(|s| {
                    if let Some(v) = dest_map.get_mut(&s.0) {
                        v.retain(|e| e != &s.1);
                        if v.is_empty() {
                            dest_map.remove(&s.0);
                        }
                    }
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
            prune_set(hash, &self.specific_rules.unremove, &mut remove_selectors);
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

            prune_map(hash, &self.specific_rules.unstyle, &mut style_selectors);
            prune_map(hash, &self.specific_rules.unremove_attr, &mut remove_attrs);
            prune_map(hash, &self.specific_rules.unremove_class, &mut remove_classes);
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

        let mut injected_script = String::new();
        script_injections.iter().for_each(|(s, mask)| {
            if let Ok(filled_template) = resources.get_scriptlet_resource(s, *mask) {
                injected_script += "try {\n";
                injected_script += &filled_template;
                injected_script += "\n} catch ( e ) { }\n";
            }
        });

        UrlSpecificResources {
            hide_selectors,
            style_selectors,
            remove_selectors,
            remove_attrs,
            remove_classes,
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
    /// Simple hostname-specific rules with a remove action, e.g. `example.com##.ad:remove()`.
    ///
    /// The parameter is the rule's CSS selector.
    pub remove: HostnameFilterBin<String>,
    /// Simple hostname-specific remove action exception rules, e.g. `example.com#@#.ad:remove()`.
    ///
    /// The parameter is the rule's CSS selector.
    pub unremove: HostnameFilterBin<String>,
    /// Hostname-specific rules with a custom style for an element, e.g.
    /// `example.com##.ad:style(margin: 0)`.
    ///
    /// The parameters are the rule's selector and its additional style.
    pub style: HostnameFilterBin<(String, String)>,
    /// Hostname-specific exception rules for a custom style for an element, e.g.
    /// `example.com#@#.ad:style(margin: 0)`.
    ///
    /// The parameters are the rule's selector and its additional style.
    ///
    /// In practice, this kind of rule does not appear in filter lists, although it is not
    /// explicitly forbidden according to any syntax documentation.
    pub unstyle: HostnameFilterBin<(String, String)>,
    /// Simple hostname-specific rules with a remove attribute action, e.g. `example.com##.ad:remove()`.
    ///
    /// The parameters are the rule's CSS selector and the class to remove.
    pub remove_attr: HostnameFilterBin<(String, String)>,
    /// Simple hostname-specific remove attribute action exception rules, e.g. `example.com#@#.ad:remove()`.
    ///
    /// The parameters are the rule's CSS selector and the class to remove.
    pub unremove_attr: HostnameFilterBin<(String, String)>,
    /// Simple hostname-specific rules with a remove class action, e.g. `example.com##.ad:remove()`.
    ///
    /// The parameters are the rule's CSS selector and the class to remove.
    pub remove_class: HostnameFilterBin<(String, String)>,
    /// Simple hostname-specific remove class action exception rules, e.g. `example.com#@#.ad:remove()`.
    ///
    /// The parameters are the rule's CSS selector and the class to remove.
    pub unremove_class: HostnameFilterBin<(String, String)>,
}

impl HostnameRuleDb {
    pub fn store_rule(&mut self, rule: CosmeticFilter) {
        use crate::filters::cosmetic::CosmeticFilterAction;
        use SpecificFilterType::*;

        let unhide = rule.mask.contains(CosmeticFilterMask::UNHIDE);
        let script_inject = rule.mask.contains(CosmeticFilterMask::SCRIPT_INJECT);
        let selector = rule.selector;

        let kind = match (unhide, script_inject, rule.action) {
            (false, false, None) => Hide(selector),
            (true, false, None) => Unhide(selector),
            (false, true, None) => InjectScript((selector, rule.permission)),
            (true, true, None) => UninjectScript((selector, rule.permission)),
            (false, false, Some(CosmeticFilterAction::Style(s))) => Style((selector, s)),
            (true, false, Some(CosmeticFilterAction::Style(s)) )=> Unstyle((selector, s)),
            (false, false, Some(CosmeticFilterAction::Remove)) => Remove(selector),
            (true, false, Some(CosmeticFilterAction::Remove)) => Unremove(selector),
            (false, false, Some(CosmeticFilterAction::RemoveClass(c))) => RemoveClass((selector, c)),
            (true, false, Some(CosmeticFilterAction::RemoveClass(c))) => UnremoveClass((selector, c)),
            (false, false, Some(CosmeticFilterAction::RemoveAttr(a))) => RemoveAttr((selector, a)),
            (true, false, Some(CosmeticFilterAction::RemoveAttr(a))) => UnremoveAttr((selector, a)),
            (_, true, Some(_)) => return, // shouldn't be possible
        };

        let tokens_to_insert = std::iter::empty()
            .chain(rule.hostnames.unwrap_or(Vec::new()))
            .chain(rule.entities.unwrap_or(Vec::new()));

        tokens_to_insert.for_each(|t| self.store(&t, kind.clone()));

        let tokens_to_insert_negated = std::iter::empty()
            .chain(rule.not_hostnames.unwrap_or(Vec::new()))
            .chain(rule.not_entities.unwrap_or(Vec::new()));

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
            Remove(s) => self.remove.insert(token, s),
            Unremove(s) => self.unremove.insert(token, s),
            Style(s) => self.style.insert(token, s),
            Unstyle(s) => self.unstyle.insert(token, s),
            RemoveAttr(s) => self.remove_attr.insert(token, s),
            UnremoveAttr(s) => self.unremove_attr.insert(token, s),
            RemoveClass(s) => self.remove_class.insert(token, s),
            UnremoveClass(s) => self.unremove_class.insert(token, s),
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
    Remove(String),
    Unremove(String),
    Style((String, String)),
    Unstyle((String, String)),
    RemoveAttr((String, String)),
    UnremoveAttr((String, String)),
    RemoveClass((String, String)),
    UnremoveClass((String, String)),
}

impl SpecificFilterType {
    fn negated(self) -> Self {
        match self {
            Self::Hide(s) => Self::Unhide(s),
            Self::Unhide(s) => Self::Hide(s),
            Self::InjectScript(s) => Self::UninjectScript(s),
            Self::UninjectScript(s) => Self::InjectScript(s),
            Self::Remove(s) => Self::Unremove(s),
            Self::Unremove(s) => Self::Remove(s),
            Self::Style(s) => Self::Unstyle(s),
            Self::Unstyle(s) => Self::Style(s),
            Self::RemoveAttr(s) => Self::UnremoveAttr(s),
            Self::UnremoveAttr(s) => Self::RemoveAttr(s),
            Self::RemoveClass(s) => Self::UnremoveClass(s),
            Self::UnremoveClass(s) => Self::RemoveClass(s),
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

#[cfg(test)]
mod cosmetic_cache_tests {
    use super::*;
    use crate::resources::Resource;

    fn cache_from_rules(rules: Vec<&str>) -> CosmeticFilterCache {
        let parsed_rules = rules
            .iter()
            .map(|r| CosmeticFilter::parse(r, false, Default::default()).unwrap())
            .collect::<Vec<_>>();

        CosmeticFilterCache::from_rules(parsed_rules)
    }

    #[test]
    fn exceptions() {
        let cfcache = cache_from_rules(vec!["~example.com##.item", "sub.example.com#@#.item2"]);
        let resources = ResourceStorage::default();

        let out = cfcache.hostname_cosmetic_resources(&resources, "test.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "example.com", false);
        expected.exceptions.insert(".item".into());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.example.com", false);
        expected.exceptions.insert(".item2".into());
        assert_eq!(out, expected);
    }

    #[test]
    fn exceptions2() {
        let cfcache = cache_from_rules(vec!["example.com,~sub.example.com##.item"]);
        let resources = ResourceStorage::default();

        let out = cfcache.hostname_cosmetic_resources(&resources, "test.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "example.com", false);
        expected.hide_selectors.insert(".item".to_owned());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.example.com", false);
        let mut expected = UrlSpecificResources::empty();
        expected.exceptions.insert(".item".into());
        assert_eq!(out, expected);
    }

    #[test]
    fn style_exceptions() {
        let cfcache = cache_from_rules(vec![
            "example.com,~sub.example.com##.element:style(background: #fff)",
            "sub.test.example.com#@#.element:style(background: #fff)",
            "a1.sub.example.com##.element",
            "a2.sub.example.com##.element:style(background: #000)",
            "a3.example.com##.element:style(background: #000)",
        ]);
        let resources = ResourceStorage::default();

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.example.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.test.example.com", false);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a1.sub.example.com", false);
        expected.hide_selectors.insert(".element".to_owned());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "test.example.com", false);
        expected.hide_selectors.clear();
        expected
            .style_selectors
            .insert(".element".to_owned(), vec!["background: #fff".to_owned()]);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a2.sub.example.com", false);
        expected.style_selectors.clear();
        expected
            .style_selectors
            .insert(".element".to_owned(), vec!["background: #000".to_owned()]);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a3.example.com", false);
        expected.style_selectors.clear();
        expected
            .style_selectors
            .insert(".element".to_owned(), vec!["background: #000".to_owned(), "background: #fff".to_owned()]);
        // order is non-deterministic
        if out != expected {
            expected
                .style_selectors
                .get_mut(".element")
                .unwrap()
                .reverse();
            assert_eq!(out, expected);
        }
    }

    #[test]
    fn script_exceptions() {
        use crate::resources::{MimeType, ResourceType};

        let cfcache = cache_from_rules(vec![
            "example.com,~sub.example.com##+js(set-constant.js, atob, trueFunc)",
            "sub.test.example.com#@#+js(set-constant.js, atob, trueFunc)",
            "cosmetic.net##+js(nowebrtc.js)",
            "g.cosmetic.net##+js(window.open-defuser.js)",
            "c.g.cosmetic.net#@#+js(nowebrtc.js)",
            "d.g.cosmetic.net#@#+js()",
        ]);
        let resources = ResourceStorage::from_resources([
            Resource {
                name: "set-constant.js".into(),
                aliases: vec![],
                kind: ResourceType::Template,
                content: base64::encode("set-constant.js, {{1}}, {{2}}"),
                dependencies: vec![],
                permission: Default::default(),
            },
            Resource::simple("nowebrtc.js", MimeType::ApplicationJavascript, "nowebrtc.js"),
            Resource::simple("window.open-defuser.js", MimeType::ApplicationJavascript, "window.open-defuser.js"),
        ]);

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.example.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.test.example.com", false);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "test.example.com", false);
        expected.injected_script =
            "try {\nset-constant.js, atob, trueFunc\n} catch ( e ) { }\n".to_owned();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "cosmetic.net", false);
        expected.injected_script = "try {\nnowebrtc.js\n} catch ( e ) { }\n".to_owned();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "g.cosmetic.net", false);
        expected.injected_script = "try {\nnowebrtc.js\n} catch ( e ) { }\ntry {\nwindow.open-defuser.js\n} catch ( e ) { }\n".to_owned();
        // order is non-deterministic
        if out != expected {
            expected.injected_script = "try {\nwindow.open-defuser.js\n} catch ( e ) { }\ntry {\nnowebrtc.js\n} catch ( e ) { }\n".to_owned();
            assert_eq!(out, expected);
        }

        let out = cfcache.hostname_cosmetic_resources(&resources, "c.g.cosmetic.net", false);
        expected.injected_script = "try {\nwindow.open-defuser.js\n} catch ( e ) { }\n".to_owned();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "d.g.cosmetic.net", false);
        expected.injected_script = "".to_owned();
        assert_eq!(out, expected);
    }

    #[test]
    fn remove_exceptions() {
        let cfcache = cache_from_rules(vec![
            "example.com,~sub.example.com##.element:remove()",
            "sub.test.example.com#@#.element:remove()",
            "a1.sub.example.com##.element",
            "a2.sub.example.com##.element:remove()",
            "a3.example.com##.element:remove()",
        ]);
        let resources = ResourceStorage::default();

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.example.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.test.example.com", false);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a1.sub.example.com", false);
        expected.hide_selectors.insert(".element".to_owned());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "test.example.com", false);
        expected.hide_selectors.clear();
        expected.remove_selectors.insert(".element".to_owned());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a2.sub.example.com", false);
        expected.remove_selectors.clear();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a3.example.com", false);
        expected.remove_selectors.clear();
        expected.remove_selectors.insert(".element".to_owned());
        assert_eq!(out, expected);
    }

    #[test]
    fn remove_attr_exceptions() {
        let cfcache = cache_from_rules(vec![
            "example.com,~sub.example.com##.element:remove-attr(style)",
            "sub.test.example.com#@#.element:remove-attr(style)",
            "a1.sub.example.com##.element",
            "a2.sub.example.com##.element:remove-attr(src)",
            "a3.example.com##.element:remove-attr(src)",
        ]);
        let resources = ResourceStorage::default();

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.example.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.test.example.com", false);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a1.sub.example.com", false);
        expected.hide_selectors.insert(".element".to_owned());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "test.example.com", false);
        expected.hide_selectors.clear();
        expected
            .remove_attrs
            .insert(".element".to_owned(), vec!["style".to_owned()]);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a2.sub.example.com", false);
        expected.remove_attrs.clear();
        expected
            .remove_attrs
            .insert(".element".to_owned(), vec!["src".to_owned()]);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a3.example.com", false);
        expected.remove_attrs.clear();
        expected
            .remove_attrs
            .insert(".element".to_owned(), vec!["src".to_owned(), "style".to_owned()]);
        // order is non-deterministic
        if out != expected {
            expected
                .remove_attrs
                .get_mut(".element")
                .unwrap()
                .reverse();
            assert_eq!(out, expected);
        }
    }

    #[test]
    fn remove_class_exceptions() {
        let cfcache = cache_from_rules(vec![
            "example.com,~sub.example.com##.element:remove-class(overlay)",
            "sub.test.example.com#@#.element:remove-class(overlay)",
            "a1.sub.example.com##.element",
            "a2.sub.example.com##.element:remove-class(banner)",
            "a3.example.com##.element:remove-class(banner)",
        ]);
        let resources = ResourceStorage::default();

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.example.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "sub.test.example.com", false);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a1.sub.example.com", false);
        expected.hide_selectors.insert(".element".to_owned());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "test.example.com", false);
        expected.hide_selectors.clear();
        expected
            .remove_classes
            .insert(".element".to_owned(), vec!["overlay".to_owned()]);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a2.sub.example.com", false);
        expected.remove_classes.clear();
        expected
            .remove_classes
            .insert(".element".to_owned(), vec!["banner".to_owned()]);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources(&resources, "a3.example.com", false);
        expected.remove_classes.clear();
        expected
            .remove_classes
            .insert(".element".to_owned(), vec!["banner".to_owned(), "overlay".to_owned()]);
        // order is non-deterministic
        if out != expected {
            expected
                .remove_classes
                .get_mut(".element")
                .unwrap()
                .reverse();
            assert_eq!(out, expected);
        }
    }

    /// Avoid impossible type inference for type parameter `impl AsRef<str>`
    const EMPTY: &[&str] = &[];

    #[test]
    fn matching_hidden_class_id_selectors() {
        let rules = [
            "##.a-class",
            "###simple-id",
            "##.a-class .with .children",
            "##.children .including #simple-id",
            "##a.a-class",
        ];
        let cfcache = CosmeticFilterCache::from_rules(
            rules
                .iter()
                .map(|r| CosmeticFilter::parse(r, false, Default::default()).unwrap())
                .collect::<Vec<_>>(),
        );

        let out = cfcache.hidden_class_id_selectors(["with"], EMPTY, &HashSet::default());
        assert_eq!(out, Vec::<String>::new());

        let out = cfcache.hidden_class_id_selectors(EMPTY, ["with"], &HashSet::default());
        assert_eq!(out, Vec::<String>::new());

        let out = cfcache.hidden_class_id_selectors(EMPTY, ["a-class"], &HashSet::default());
        assert_eq!(out, Vec::<String>::new());

        let out =
            cfcache.hidden_class_id_selectors(["simple-id"], EMPTY, &HashSet::default());
        assert_eq!(out, Vec::<String>::new());

        let out = cfcache.hidden_class_id_selectors(["a-class"], EMPTY, &HashSet::default());
        assert_eq!(out, [".a-class", ".a-class .with .children"]);

        let out = cfcache.hidden_class_id_selectors(
            ["children", "a-class"],
            EMPTY,
            &HashSet::default(),
        );
        assert_eq!(
            out,
            [
                ".children .including #simple-id",
                ".a-class",
                ".a-class .with .children",
            ]
        );

        let out =
            cfcache.hidden_class_id_selectors(EMPTY, ["simple-id"], &HashSet::default());
        assert_eq!(out, ["#simple-id"]);

        let out = cfcache.hidden_class_id_selectors(
            ["children", "a-class"],
            ["simple-id"],
            &HashSet::default(),
        );
        assert_eq!(
            out,
            [
                ".children .including #simple-id",
                ".a-class",
                ".a-class .with .children",
                "#simple-id",
            ]
        );
    }

    #[test]
    fn class_id_exceptions() {
        let rules = vec![
            "##.a-class",
            "###simple-id",
            "##.a-class .with .children",
            "##.children .including #simple-id",
            "##a.a-class",
            "example.*#@#.a-class",
            "~test.com###test-element",
        ];
        let cfcache = CosmeticFilterCache::from_rules(
            rules
                .iter()
                .map(|r| CosmeticFilter::parse(r, false, Default::default()).unwrap())
                .collect::<Vec<_>>(),
        );
        let resources = ResourceStorage::default();
        let exceptions = cfcache
            .hostname_cosmetic_resources(&resources, "example.co.uk", false)
            .exceptions;

        let out = cfcache.hidden_class_id_selectors(["a-class"], EMPTY, &exceptions);
        assert_eq!(out, [".a-class .with .children"]);

        let out = cfcache.hidden_class_id_selectors(
            ["children", "a-class"],
            ["simple-id"],
            &exceptions,
        );
        assert_eq!(
            out,
            [
                ".children .including #simple-id",
                ".a-class .with .children",
                "#simple-id",
            ]
        );

        let out = cfcache.hidden_class_id_selectors(EMPTY, ["test-element"], &exceptions);
        assert_eq!(out, ["#test-element"]);

        let exceptions = cfcache
            .hostname_cosmetic_resources(&resources, "a1.test.com", false)
            .exceptions;

        let out = cfcache.hidden_class_id_selectors(["a-class"], EMPTY, &exceptions);
        assert_eq!(out, [".a-class", ".a-class .with .children"]);

        let out = cfcache.hidden_class_id_selectors(
            ["children", "a-class"],
            ["simple-id"],
            &exceptions,
        );
        assert_eq!(
            out,
            [
                ".children .including #simple-id",
                ".a-class",
                ".a-class .with .children",
                "#simple-id",
            ]
        );

        let out = cfcache.hidden_class_id_selectors(EMPTY, ["test-element"], &exceptions);
        assert_eq!(out, Vec::<String>::new());
    }

    #[test]
    fn misc_generic_exceptions() {
        let rules = vec![
            "##a[href=\"bad.com\"]",
            "##div > p",
            "##a[href=\"notbad.com\"]",
            "example.com#@#div > p",
            "~example.com##a[href=\"notbad.com\"]",
        ];
        let cfcache = CosmeticFilterCache::from_rules(
            rules
                .iter()
                .map(|r| CosmeticFilter::parse(r, false, Default::default()).unwrap())
                .collect::<Vec<_>>(),
        );
        let resources = ResourceStorage::default();

        let hide_selectors = cfcache
            .hostname_cosmetic_resources(&resources, "test.com", false)
            .hide_selectors;
        let mut expected_hides = HashSet::new();
        expected_hides.insert("a[href=\"bad.com\"]".to_owned());
        expected_hides.insert("div > p".to_owned());
        expected_hides.insert("a[href=\"notbad.com\"]".to_owned());
        assert_eq!(hide_selectors, expected_hides);

        let hide_selectors = cfcache
            .hostname_cosmetic_resources(&resources, "example.com", false)
            .hide_selectors;
        let mut expected_hides = HashSet::new();
        expected_hides.insert("a[href=\"bad.com\"]".to_owned());
        assert_eq!(hide_selectors, expected_hides);
    }

    #[test]
    fn apply_to_tld() {
        use crate::resources::ResourceType;

        // toolforge.org and github.io are examples of TLDs with multiple segments. These rules
        // should still be parsed correctly and applied on corresponding subdomains.
        let rules = vec![
            "toolforge.org##+js(abort-on-property-read, noAdBlockers)",
            "github.io##div.adToBlock",
        ];
        let cfcache = CosmeticFilterCache::from_rules(
            rules
                .iter()
                .map(|r| CosmeticFilter::parse(r, false, Default::default()).unwrap())
                .collect::<Vec<_>>(),
        );
        let resources = ResourceStorage::from_resources([
            Resource {
                name: "abort-on-property-read.js".into(),
                aliases: vec!["aopr".to_string()],
                kind: ResourceType::Template,
                content: base64::encode("abort-on-property-read.js, {{1}}"),
                dependencies: vec![],
                permission: Default::default(),
            }
        ]);

        let injected_script = cfcache
            .hostname_cosmetic_resources(&resources, "antonok.toolforge.org", false)
            .injected_script;
        assert_eq!(
            injected_script,
            "try {\nabort-on-property-read.js, noAdBlockers\n} catch ( e ) { }\n"
        );

        let hide_selectors = cfcache
            .hostname_cosmetic_resources(&resources, "antonok.github.io", false)
            .hide_selectors;
        let mut expected_hides = HashSet::new();
        expected_hides.insert("div.adToBlock".to_owned());
        assert_eq!(hide_selectors, expected_hides);
    }
}
