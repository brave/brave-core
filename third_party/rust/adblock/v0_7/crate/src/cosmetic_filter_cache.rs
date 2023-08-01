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
use crate::resources::{Resource, ScriptletResourceStorage};
use crate::utils::Hash;

use std::collections::{HashMap, HashSet};

use serde::{Deserialize, Serialize};

/// Contains cosmetic filter information intended to be used on a particular URL.
///
/// `hide_selectors` is a set of any CSS selector on the page that should be hidden, i.e. styled as
/// `{ display: none !important; }`.
///
/// `style_selectors` is a map of CSS selectors on the page to respective non-hide style rules,
/// i.e. any required styles other than `display: none`.
///
/// `exceptions` is a set of any class or id CSS selectors that should not have generic rules
/// applied. In practice, these should be passed to `class_id_stylesheet` and not used otherwise.
///
/// `injected_script` is the Javascript code for any scriptlets that should be injected into the
/// page.
///
/// `generichide` is set to true if there is a corresponding `$generichide` exception network
/// filter. If so, the page should not query for additional generic rules using
/// `hidden_class_id_selectors`.
#[derive(Debug, PartialEq, Eq, Deserialize, Serialize)]
pub struct UrlSpecificResources {
    pub hide_selectors: HashSet<String>,
    pub style_selectors: HashMap<String, Vec<String>>,
    pub exceptions: HashSet<String>,
    pub injected_script: String,
    pub generichide: bool,
}

impl UrlSpecificResources {
    pub fn empty() -> Self {
        Self {
            hide_selectors: HashSet::new(),
            style_selectors: HashMap::new(),
            exceptions: HashSet::new(),
            injected_script: String::new(),
            generichide: false,
        }
    }
}

/// Splits the given hostname-specific rules into three collections:
/// - a set of CSS selectors that should be hidden on all pages under the hostname
/// - a mapping from CSS selectors to any additional (i.e. not `display: none`) CSS styles that
///   should be applied to those elements
/// - a list of any scriptlets that should be injected into the page's JavaScript context
fn hostname_specific_rules(
    rules: &[&SpecificFilterType],
) -> (HashSet<String>, HashMap<String, Vec<String>>, Vec<String>) {
    if rules.is_empty() {
        (HashSet::default(), HashMap::default(), vec![])
    } else {
        let mut script_rules = Vec::with_capacity(10);

        let mut hide_rules = HashSet::with_capacity(rules.len());
        let mut style_rules: HashMap<String, Vec<String>> = HashMap::with_capacity(rules.len());

        rules.iter().for_each(|rule| match rule {
            SpecificFilterType::Hide(sel) => {
                hide_rules.insert(sel.to_owned());
            }
            SpecificFilterType::Style(sel, style) => {
                if let Some(entry) = style_rules.get_mut(sel) {
                    entry.push(style.to_owned());
                } else {
                    style_rules.insert(sel.to_owned(), vec![style.to_owned()]);
                }
            }
            SpecificFilterType::ScriptInject(sel) => {
                script_rules.push(sel.to_owned());
            }
            _ => unreachable!(),
        });

        (hide_rules, style_rules, script_rules)
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
#[derive(Deserialize, Serialize)]
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

    pub(crate) scriptlets: ScriptletResourceStorage,
}

impl CosmeticFilterCache {
    pub fn new() -> Self {
        Self {
            simple_class_rules: HashSet::new(),
            simple_id_rules: HashSet::new(),
            complex_class_rules: HashMap::new(),
            complex_id_rules: HashMap::new(),

            specific_rules: HostnameRuleDb::new(),

            misc_generic_selectors: HashSet::new(),

            scriptlets: Default::default(),
        }
    }

    pub fn from_rules(rules: Vec<CosmeticFilter>) -> Self {
        let mut self_ = Self {
            simple_class_rules: HashSet::with_capacity(rules.len() / 2),
            simple_id_rules: HashSet::with_capacity(rules.len() / 2),
            complex_class_rules: HashMap::with_capacity(rules.len() / 2),
            complex_id_rules: HashMap::with_capacity(rules.len() / 2),

            specific_rules: HostnameRuleDb::new(),

            misc_generic_selectors: HashSet::with_capacity(rules.len() / 30),

            scriptlets: Default::default(),
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
        classes: &[String],
        ids: &[String],
        exceptions: &HashSet<String>,
    ) -> Vec<String> {
        let mut simple_classes = vec![];
        let mut simple_ids = vec![];
        let mut complex_selectors = vec![];

        classes.iter().for_each(|class| {
            if self.simple_class_rules.contains(class)
                && !exceptions.contains(&format!(".{}", class))
            {
                simple_classes.push(class);
            }
            if let Some(bucket) = self.complex_class_rules.get(class) {
                complex_selectors.extend(bucket.iter().filter(|sel| !exceptions.contains(*sel)));
            }
        });
        ids.iter().for_each(|id| {
            if self.simple_id_rules.contains(id) && !exceptions.contains(&format!("#{}", id)) {
                simple_ids.push(id);
            }
            if let Some(bucket) = self.complex_id_rules.get(id) {
                complex_selectors.extend(bucket.iter().filter(|sel| !exceptions.contains(*sel)));
            }
        });

        if simple_classes.is_empty() && simple_ids.is_empty() && complex_selectors.is_empty() {
            return vec![];
        }

        simple_classes
            .into_iter()
            .map(|class| format!(".{}", class))
            .chain(simple_ids.into_iter().map(|id| format!("#{}", id)))
            .chain(complex_selectors.into_iter().cloned())
            .collect::<Vec<_>>()
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
        hostname: &str,
        generichide: bool,
    ) -> UrlSpecificResources {
        let domain_str = {
            let (start, end) = crate::url_parser::get_host_domain(hostname);
            &hostname[start..end]
        };

        let (request_entities, request_hostnames) = hostname_domain_hashes(hostname, domain_str);

        let mut rules_that_apply = vec![];
        for hash in request_entities.iter().chain(request_hostnames.iter()) {
            if let Some(specific_rules) = self.specific_rules.retrieve(hash) {
                rules_that_apply.extend(specific_rules);
            }
        }

        let mut exceptions = HostnameExceptionsBuilder::default();

        rules_that_apply.iter().for_each(|r| {
            exceptions.insert_if_exception(r);
        });

        let rules_that_apply = rules_that_apply
            .iter()
            .map(|r| r.to_owned())
            .filter(|r| exceptions.allow_specific_rule(r))
            .collect::<Vec<_>>();

        let (hostname_hide_selectors, style_selectors, script_injections) =
            hostname_specific_rules(&rules_that_apply[..]);

        let hide_selectors = if generichide {
            hostname_hide_selectors
        } else {
            let mut hide_selectors = self
                .misc_generic_selectors
                .difference(&exceptions.hide_exceptions)
                .cloned()
                .collect::<HashSet<_>>();
            hostname_hide_selectors.into_iter().for_each(|sel| {
                hide_selectors.insert(sel);
            });
            hide_selectors
        };

        let mut injected_script = String::new();
        script_injections.iter().for_each(|s| {
            if let Ok(filled_template) = self.scriptlets.get_scriptlet(s) {
                injected_script += "try {\n";
                injected_script += &filled_template;
                injected_script += "\n} catch ( e ) { }\n";
            }
        });

        UrlSpecificResources {
            hide_selectors,
            style_selectors,
            exceptions: exceptions.hide_exceptions,
            injected_script,
            generichide,
        }
    }

    /// Sets the internal resources to be those provided, silently discarding errors.
    ///
    /// Use `add_resource` if error information is required.
    pub fn use_resources(&mut self, resources: &[Resource]) {
        let mut scriptlets = ScriptletResourceStorage::default();

        resources.iter().for_each(|resource| {
            let _result = scriptlets.add_resource(resource);
        });

        self.scriptlets = scriptlets;
    }

    /// Adds a single scriptlet resource.
    pub fn add_resource(
        &mut self,
        resource: &Resource,
    ) -> Result<(), crate::resources::AddResourceError> {
        self.scriptlets.add_resource(resource)
    }
}

/// Used internally to build hostname-specific rulesets by canceling out rules which match any
/// exceptions
#[derive(Default, Debug, PartialEq, Eq)]
struct HostnameExceptionsBuilder {
    hide_exceptions: HashSet<String>,
    style_exceptions: HashSet<(String, String)>,
    script_inject_exceptions: HashSet<String>,
}

impl HostnameExceptionsBuilder {
    /// Saves the given rule if it's an exception, or ignores it otherwise.
    pub fn insert_if_exception(&mut self, rule: &SpecificFilterType) {
        use SpecificFilterType as Rule;

        match rule {
            Rule::Hide(_) | Rule::Style(_, _) | Rule::ScriptInject(_) => (),
            Rule::Unhide(sel) => {
                self.hide_exceptions.insert(sel.clone());
            }
            Rule::UnhideStyle(sel, style) => {
                self.style_exceptions.insert((sel.clone(), style.clone()));
            }
            Rule::UnhideScriptInject(script) => {
                self.script_inject_exceptions.insert(script.clone());
            }
        }
    }

    /// A generic selector is allowed if it is not excepted by this set of exceptions.
    pub fn allow_generic_selector(&self, selector: &str) -> bool {
        !self.hide_exceptions.contains(selector)
    }

    /// Specific rules are allowed if they can be used to hide, restyle, or inject a script in the
    /// context of this set of exceptions - i.e. if the rule itself is not an exception rule and
    /// doesn't have a corresponding exception rule added previously.
    pub fn allow_specific_rule(&self, rule: &SpecificFilterType) -> bool {
        match rule {
            SpecificFilterType::Hide(sel) => !self.hide_exceptions.contains(sel),
            SpecificFilterType::Style(sel, style) => !self
                .style_exceptions
                .contains(&(sel.to_string(), style.to_string())),
            SpecificFilterType::ScriptInject(sel) => !self.script_inject_exceptions.contains(sel),
            _ => false,
        }
    }
}

/// Each hostname-specific filter can be pointed to by several different hostnames, and each
/// hostname can correspond to several different filters. To effectively store and access those
/// filters by hostname, all the non-hostname information for filters is stored in per-hostname
/// "buckets" within a Vec, and each bucket is identified by its index. Hostname hashes are used as
/// keys to get the indices of relevant buckets, which are in turn used to retrieve all the filters
/// that apply.
#[derive(Deserialize, Serialize, Default)]
pub(crate) struct HostnameRuleDb {
    #[serde(serialize_with = "crate::data_format::utils::stabilize_hashmap_serialization")]
    db: HashMap<Hash, Vec<SpecificFilterType>>,
}

impl HostnameRuleDb {
    pub fn new() -> Self {
        HostnameRuleDb { db: HashMap::new() }
    }

    pub fn store_rule(&mut self, rule: CosmeticFilter) {
        let kind = SpecificFilterType::from(&rule);

        if let Some(hostnames) = rule.hostnames {
            hostnames.iter().for_each(|h| self.store(h, kind.clone()));
        }
        if let Some(entities) = rule.entities {
            entities.iter().for_each(|e| self.store(e, kind.clone()));
        }

        let kind = kind.negated();

        if let Some(not_hostnames) = rule.not_hostnames {
            not_hostnames
                .iter()
                .for_each(|h| self.store(h, kind.clone()));
        }
        if let Some(not_entities) = rule.not_entities {
            not_entities
                .iter()
                .for_each(|e| self.store(e, kind.clone()));
        }
    }

    fn store(&mut self, hostname: &Hash, kind: SpecificFilterType) {
        if let Some(bucket) = self.db.get_mut(hostname) {
            bucket.push(kind);
        } else {
            self.db.insert(*hostname, vec![kind]);
        }
    }

    pub fn retrieve<'a>(&'a self, hostname: &Hash) -> Option<&'a [SpecificFilterType]> {
        if let Some(bucket) = self.db.get(hostname) {
            Some(bucket)
        } else {
            None
        }
    }
}

/// Each variant describes a single rule that is specific to a particular hostname.
#[derive(Clone, Debug, Deserialize, Serialize)]
pub enum SpecificFilterType {
    /// A simple hostname-specific hide rule, e.g. `example.com##.ad`.
    ///
    /// The parameter is the rule's CSS selector.
    Hide(String),
    /// A simple hostname-specific hide exception rule, e.g. `example.com#@#.ad`.
    ///
    /// The parameter is the rule's CSS selector.
    Unhide(String),

    /// A hostname-specific rule with a custom style for an element, e.g.
    /// `example.com##.ad:style(margin: 0)`.
    ///
    /// The parameters are the rule's selector and its additional style.
    Style(String, String),
    /// A hostname-specific exception rule for a custom style for an element, e.g.
    /// `example.com#@#.ad:style(margin: 0)`.
    ///
    /// The parameters are the rule's selector and its additional style.
    ///
    /// In practice, this kind of rule does not appear in filter lists, although it is not
    /// explicitly forbidden according to any syntax documentation.
    UnhideStyle(String, String),

    /// A hostname-specific rule with a scriptlet to inject along with any arguments, e.g.
    /// `example.com##+js(acis, Number.isNan)`.
    ///
    /// The parameter is the contents of the `+js(...)` syntax construct.
    ScriptInject(String),
    /// A hostname-specific rule to except a scriptlet to inject along with any arguments, e.g.
    /// `example.com#@#+js(acis, Number.isNan)`.
    ///
    /// The parameter is the contents of the `+js(...)` syntax construct.
    ///
    /// In practice, these rules are extremely rare in filter lists.
    UnhideScriptInject(String),
}

/// This implementation assumes the given rule has hostname or entity constraints, and that the
/// appropriate 'hidden' generic rule has already been applied externally if necessary.
impl From<&CosmeticFilter> for SpecificFilterType {
    fn from(rule: &CosmeticFilter) -> Self {
        let unhide = rule.mask.contains(CosmeticFilterMask::UNHIDE);

        if let Some(ref style) = rule.style {
            if unhide {
                SpecificFilterType::UnhideStyle(rule.selector.clone(), style.clone())
            } else {
                SpecificFilterType::Style(rule.selector.clone(), style.clone())
            }
        } else if rule.mask.contains(CosmeticFilterMask::SCRIPT_INJECT) {
            if unhide {
                SpecificFilterType::UnhideScriptInject(rule.selector.clone())
            } else {
                SpecificFilterType::ScriptInject(rule.selector.clone())
            }
        } else {
            if unhide {
                SpecificFilterType::Unhide(rule.selector.clone())
            } else {
                SpecificFilterType::Hide(rule.selector.clone())
            }
        }
    }
}

impl SpecificFilterType {
    pub fn negated(self) -> Self {
        match self {
            SpecificFilterType::Hide(sel) => SpecificFilterType::Unhide(sel),
            SpecificFilterType::Unhide(sel) => SpecificFilterType::Hide(sel),
            SpecificFilterType::Style(sel, style) => SpecificFilterType::UnhideStyle(sel, style),
            SpecificFilterType::UnhideStyle(sel, style) => SpecificFilterType::Style(sel, style),
            SpecificFilterType::ScriptInject(script) => {
                SpecificFilterType::UnhideScriptInject(script)
            }
            SpecificFilterType::UnhideScriptInject(script) => {
                SpecificFilterType::ScriptInject(script)
            }
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

    fn cache_from_rules(rules: Vec<&str>) -> CosmeticFilterCache {
        let parsed_rules = rules
            .iter()
            .map(|r| CosmeticFilter::parse(r, false).unwrap())
            .collect::<Vec<_>>();

        CosmeticFilterCache::from_rules(parsed_rules)
    }

    #[test]
    fn exceptions() {
        let cfcache = cache_from_rules(vec!["~example.com##.item", "sub.example.com#@#.item2"]);

        let out = cfcache.hostname_cosmetic_resources("test.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("example.com", false);
        expected.exceptions.insert(".item".into());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("sub.example.com", false);
        expected.exceptions.insert(".item2".into());
        assert_eq!(out, expected);
    }

    #[test]
    fn exceptions2() {
        let cfcache = cache_from_rules(vec!["example.com,~sub.example.com##.item"]);

        let out = cfcache.hostname_cosmetic_resources("test.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("example.com", false);
        expected.hide_selectors.insert(".item".to_owned());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("sub.example.com", false);
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
        ]);

        let out = cfcache.hostname_cosmetic_resources("sub.example.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("sub.test.example.com", false);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("a1.sub.example.com", false);
        expected.hide_selectors.insert(".element".to_owned());
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("test.example.com", false);
        expected.hide_selectors.clear();
        expected
            .style_selectors
            .insert(".element".to_owned(), vec!["background: #fff".to_owned()]);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("a2.sub.example.com", false);
        expected.style_selectors.clear();
        expected
            .style_selectors
            .insert(".element".to_owned(), vec!["background: #000".to_owned()]);
        assert_eq!(out, expected);
    }

    #[test]
    fn script_exceptions() {
        use crate::resources::{MimeType, ResourceType};

        let mut cfcache = cache_from_rules(vec![
            "example.com,~sub.example.com##+js(set-constant.js, atob, trueFunc)",
            "sub.test.example.com#@#+js(set-constant.js, atob, trueFunc)",
            "cosmetic.net##+js(nowebrtc.js)",
            "g.cosmetic.net##+js(window.open-defuser.js)",
            "c.g.cosmetic.net#@#+js(nowebrtc.js)",
        ]);

        cfcache.use_resources(&[
            Resource {
                name: "set-constant.js".into(),
                aliases: vec![],
                kind: ResourceType::Template,
                content: base64::encode("set-constant.js, {{1}}, {{2}}"),
            },
            Resource {
                name: "nowebrtc.js".into(),
                aliases: vec![],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("nowebrtc.js"),
            },
            Resource {
                name: "window.open-defuser.js".into(),
                aliases: vec![],
                kind: ResourceType::Mime(MimeType::ApplicationJavascript),
                content: base64::encode("window.open-defuser.js"),
            },
        ]);

        let out = cfcache.hostname_cosmetic_resources("sub.example.com", false);
        let mut expected = UrlSpecificResources::empty();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("sub.test.example.com", false);
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("test.example.com", false);
        expected.injected_script =
            "try {\nset-constant.js, atob, trueFunc\n} catch ( e ) { }\n".to_owned();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("cosmetic.net", false);
        expected.injected_script = "try {\nnowebrtc.js\n} catch ( e ) { }\n".to_owned();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("g.cosmetic.net", false);
        expected.injected_script = "try {\nnowebrtc.js\n} catch ( e ) { }\ntry {\nwindow.open-defuser.js\n} catch ( e ) { }\n".to_owned();
        assert_eq!(out, expected);

        let out = cfcache.hostname_cosmetic_resources("c.g.cosmetic.net", false);
        expected.injected_script = "try {\nwindow.open-defuser.js\n} catch ( e ) { }\n".to_owned();
        assert_eq!(out, expected);
    }

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
                .map(|r| CosmeticFilter::parse(r, false).unwrap())
                .collect::<Vec<_>>(),
        );

        let out = cfcache.hidden_class_id_selectors(&["with".into()], &[], &HashSet::default());
        assert_eq!(out, Vec::<String>::new());

        let out = cfcache.hidden_class_id_selectors(&[], &["with".into()], &HashSet::default());
        assert_eq!(out, Vec::<String>::new());

        let out = cfcache.hidden_class_id_selectors(&[], &["a-class".into()], &HashSet::default());
        assert_eq!(out, Vec::<String>::new());

        let out =
            cfcache.hidden_class_id_selectors(&["simple-id".into()], &[], &HashSet::default());
        assert_eq!(out, Vec::<String>::new());

        let out = cfcache.hidden_class_id_selectors(&["a-class".into()], &[], &HashSet::default());
        assert_eq!(out, [".a-class", ".a-class .with .children"]);

        let out = cfcache.hidden_class_id_selectors(
            &["children".into(), "a-class".into()],
            &[],
            &HashSet::default(),
        );
        assert_eq!(
            out,
            [
                ".a-class",
                ".children .including #simple-id",
                ".a-class .with .children"
            ]
        );

        let out =
            cfcache.hidden_class_id_selectors(&[], &["simple-id".into()], &HashSet::default());
        assert_eq!(out, ["#simple-id"]);

        let out = cfcache.hidden_class_id_selectors(
            &["children".into(), "a-class".into()],
            &["simple-id".into()],
            &HashSet::default(),
        );
        assert_eq!(
            out,
            [
                ".a-class",
                "#simple-id",
                ".children .including #simple-id",
                ".a-class .with .children"
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
                .map(|r| CosmeticFilter::parse(r, false).unwrap())
                .collect::<Vec<_>>(),
        );
        let exceptions = cfcache
            .hostname_cosmetic_resources("example.co.uk", false)
            .exceptions;

        let out = cfcache.hidden_class_id_selectors(&["a-class".into()], &[], &exceptions);
        assert_eq!(out, [".a-class .with .children"]);

        let out = cfcache.hidden_class_id_selectors(
            &["children".into(), "a-class".into()],
            &["simple-id".into()],
            &exceptions,
        );
        assert_eq!(
            out,
            [
                "#simple-id",
                ".children .including #simple-id",
                ".a-class .with .children"
            ]
        );

        let out = cfcache.hidden_class_id_selectors(&[], &["test-element".into()], &exceptions);
        assert_eq!(out, ["#test-element"]);

        let exceptions = cfcache
            .hostname_cosmetic_resources("a1.test.com", false)
            .exceptions;

        let out = cfcache.hidden_class_id_selectors(&["a-class".into()], &[], &exceptions);
        assert_eq!(out, [".a-class", ".a-class .with .children"]);

        let out = cfcache.hidden_class_id_selectors(
            &["children".into(), "a-class".into()],
            &["simple-id".into()],
            &exceptions,
        );
        assert_eq!(
            out,
            [
                ".a-class",
                "#simple-id",
                ".children .including #simple-id",
                ".a-class .with .children"
            ]
        );

        let out = cfcache.hidden_class_id_selectors(&[], &["test-element".into()], &exceptions);
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
                .map(|r| CosmeticFilter::parse(r, false).unwrap())
                .collect::<Vec<_>>(),
        );

        let hide_selectors = cfcache
            .hostname_cosmetic_resources("test.com", false)
            .hide_selectors;
        let mut expected_hides = HashSet::new();
        expected_hides.insert("a[href=\"bad.com\"]".to_owned());
        expected_hides.insert("div > p".to_owned());
        expected_hides.insert("a[href=\"notbad.com\"]".to_owned());
        assert_eq!(hide_selectors, expected_hides);

        let hide_selectors = cfcache
            .hostname_cosmetic_resources("example.com", false)
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
        let mut cfcache = CosmeticFilterCache::from_rules(
            rules
                .iter()
                .map(|r| CosmeticFilter::parse(r, false).unwrap())
                .collect::<Vec<_>>(),
        );
        cfcache.use_resources(&[Resource {
            name: "abort-on-property-read.js".into(),
            aliases: vec!["aopr".to_string()],
            kind: ResourceType::Template,
            content: base64::encode("abort-on-property-read.js, {{1}}"),
        }]);

        let injected_script = cfcache
            .hostname_cosmetic_resources("antonok.toolforge.org", false)
            .injected_script;
        assert_eq!(
            injected_script,
            "try {\nabort-on-property-read.js, noAdBlockers\n} catch ( e ) { }\n"
        );

        let hide_selectors = cfcache
            .hostname_cosmetic_resources("antonok.github.io", false)
            .hide_selectors;
        let mut expected_hides = HashSet::new();
        expected_hides.insert("div.adToBlock".to_owned());
        assert_eq!(hide_selectors, expected_hides);
    }
}
