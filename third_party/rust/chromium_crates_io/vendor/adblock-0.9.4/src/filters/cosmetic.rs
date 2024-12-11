//! Filters that take effect at a page-content level, including CSS selector-based filtering and
//! content script injection.

use memchr::{memchr as find_char, memmem, memrchr as find_char_reverse};
use serde::{Deserialize, Serialize};
use thiserror::Error;

use crate::resources::PermissionMask;
use crate::utils::Hash;

use css_validation::{is_valid_css_style, validate_css_selector};

#[derive(Debug, Error, PartialEq)]
pub enum CosmeticFilterError {
    #[error("punycode error")]
    PunycodeError,
    #[error("invalid action specifier")]
    InvalidActionSpecifier,
    #[error("unsupported syntax")]
    UnsupportedSyntax,
    #[error("missing sharp")]
    MissingSharp,
    #[error("invalid css style")]
    InvalidCssStyle,
    #[error("invalid css selector")]
    InvalidCssSelector,
    #[error("generic unhide")]
    GenericUnhide,
    #[error("generic script inject")]
    GenericScriptInject,
    #[error("procedural and action filters cannot be generic")]
    GenericAction,
    #[error("double negation")]
    DoubleNegation,
    #[error("empty rule")]
    EmptyRule,
    #[error("html filtering is unsupported")]
    HtmlFilteringUnsupported,
    #[error("scriptlet args could not be parsed")]
    InvalidScriptletArgs,
    #[error("location modifiers are unsupported")]
    LocationModifiersUnsupported,
    #[error("procedural filters can only accept a single CSS selector")]
    ProceduralFilterWithMultipleSelectors,
}

/// Refer to <https://github.com/uBlockOrigin/uBlock-issues/wiki/Static-filter-syntax#action-operators>
#[derive(PartialEq, Debug, Clone, Serialize, Deserialize)]
#[serde(tag = "type", content = "arg")]
#[serde(rename_all="kebab-case")]
pub enum CosmeticFilterAction {
    /// Rules with a remove action, e.g. `example.com##.ad:remove()`.
    ///
    /// Matching elements are to be removed from the DOM.
    Remove,
    /// Rules with a custom style for an element, e.g. `example.com##.ad:style(margin: 0)`.
    ///
    /// Argument is one or more CSS property declarations, separated by the standard ;. Some
    /// characters, strings, and values are forbidden.
    ///
    /// The specified CSS styling should be applied to matching elements in the DOM.
    Style(String),
    /// Rules with a remove attribute action, e.g. `example.com##.ad:remove-attr(onclick)`.
    ///
    /// Argument is the name of an HTML attribute.
    ///
    /// The attribute should be removed from matching elements in the DOM.
    RemoveAttr(String),
    /// Rules with a remove class action, e.g. `example.com##.ad:remove-class(advert)`.
    ///
    /// The parameter is the name of a CSS class.
    ///
    /// The class should be removed from matching elements in the DOM.
    RemoveClass(String),
}

impl CosmeticFilterAction {
    fn new_style(style: &str) -> Result<Self, CosmeticFilterError> {
        if !is_valid_css_style(style) {
            return Err(CosmeticFilterError::InvalidCssStyle);
        }
        Ok(Self::Style(style.to_string()))
    }

    fn new_remove_attr(attr: &str) -> Result<Self, CosmeticFilterError> {
        Self::forbid_regex_or_quoted_args(attr)?;
        Ok(CosmeticFilterAction::RemoveAttr(attr.to_string()))
    }

    fn new_remove_class(class: &str) -> Result<Self, CosmeticFilterError> {
        Self::forbid_regex_or_quoted_args(class)?;
        Ok(CosmeticFilterAction::RemoveClass(class.to_string()))
    }

    /// Regex and quoted args aren't supported yet
    fn forbid_regex_or_quoted_args(arg: &str) -> Result<(), CosmeticFilterError> {
        if arg.starts_with('/') || arg.starts_with('\"') || arg.starts_with('\'') {
            return Err(CosmeticFilterError::UnsupportedSyntax);
        }
        Ok(())
    }
}

bitflags::bitflags! {
    /// Boolean flags for cosmetic filter rules.
    #[derive(Serialize, Deserialize)]
    pub struct CosmeticFilterMask: u8 {
        const UNHIDE = 1 << 0;
        const SCRIPT_INJECT = 1 << 1;

        // Careful with checking for NONE - will always match
        const NONE = 0;
    }
}

/// Struct representing a parsed cosmetic filter rule.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct CosmeticFilter {
    pub entities: Option<Vec<Hash>>,
    pub hostnames: Option<Vec<Hash>>,
    pub mask: CosmeticFilterMask,
    pub not_entities: Option<Vec<Hash>>,
    pub not_hostnames: Option<Vec<Hash>>,
    pub raw_line: Option<Box<String>>,
    pub selector: Vec<CosmeticFilterOperator>,
    pub action: Option<CosmeticFilterAction>,
    pub permission: PermissionMask,
}

/// Individual parts of a cosmetic filter's selector. Most rules have a CSS selector; some may also
/// have one or more procedural operators.
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
#[serde(tag = "type", content = "arg")]
#[serde(rename_all="kebab-case")]
pub enum CosmeticFilterOperator {
    CssSelector(String),
    HasText(String),
    MatchesAttr(String),
    MatchesCss(String),
    MatchesCssBefore(String),
    MatchesCssAfter(String),
    MatchesPath(String),
    MinTextLength(String),
    Upward(String),
    Xpath(String),
}

pub(crate) enum CosmeticFilterLocationType {
    Entity,
    NotEntity,
    Hostname,
    NotHostname,
    Unsupported,
}

/// Contains hashes of all of the comma separated location items that were populated before the
/// hash separator in a cosmetic filter rule.
#[derive(Default)]
struct CosmeticFilterLocations {
    /// Locations of the form `entity.*`
    entities: Option<Vec<Hash>>,
    /// Locations of the form `~entity.*`
    not_entities: Option<Vec<Hash>>,
    /// Locations of the form `hostname`
    hostnames: Option<Vec<Hash>>,
    /// Locations of the form `~hostname`
    not_hostnames: Option<Vec<Hash>>,
}

impl CosmeticFilter {
    #[inline]
    pub(crate) fn locations_before_sharp(
        line: &str,
        sharp_index: usize,
    ) -> impl Iterator<Item = (CosmeticFilterLocationType, &str)> {
        line[0..sharp_index].split(',').filter_map(|part| {
            if part.is_empty() {
                return None;
            }
            let hostname = part;
            let negation = hostname.starts_with('~');
            let entity = hostname.ends_with(".*");
            let start = if negation { 1 } else { 0 };
            let end = if entity {
                hostname.len() - 2
            } else {
                hostname.len()
            };
            let location = &hostname[start..end];
            // AdGuard regex syntax
            if location.starts_with('/') {
                return Some((CosmeticFilterLocationType::Unsupported, part));
            }
            Some(match (negation, entity) {
                (true, true) => (CosmeticFilterLocationType::NotEntity, location),
                (true, false) => (CosmeticFilterLocationType::NotHostname, location),
                (false, true) => (CosmeticFilterLocationType::Entity, location),
                (false, false) => (CosmeticFilterLocationType::Hostname, location),
            })
        })
    }

    /// Parses the contents of a cosmetic filter rule up to the `##` or `#@#` separator.
    ///
    /// On success, returns hashes of all the comma separated location items that were populated in
    /// the rule.
    ///
    /// This should only be called if `sharp_index` is greater than 0, in which case all four are
    /// guaranteed to be `None`.
    #[inline]
    fn parse_before_sharp(
        line: &str,
        sharp_index: usize,
    ) -> Result<CosmeticFilterLocations, CosmeticFilterError> {
        let mut entities_vec = vec![];
        let mut not_entities_vec = vec![];
        let mut hostnames_vec = vec![];
        let mut not_hostnames_vec = vec![];

        if line.starts_with('[') {
            return Err(CosmeticFilterError::LocationModifiersUnsupported);
        }

        let mut any_unsupported = false;
        for (location_type, location) in Self::locations_before_sharp(line, sharp_index) {
            let mut hostname = String::new();
            if location.is_ascii() {
                hostname.push_str(location);
            } else {
                match idna::domain_to_ascii(location) {
                    Ok(x) if !x.is_empty() => hostname.push_str(&x),
                    _ => return Err(CosmeticFilterError::PunycodeError),
                }
            }
            let hash = crate::utils::fast_hash(&hostname);
            match location_type {
                CosmeticFilterLocationType::NotEntity => not_entities_vec.push(hash),
                CosmeticFilterLocationType::NotHostname => not_hostnames_vec.push(hash),
                CosmeticFilterLocationType::Entity => entities_vec.push(hash),
                CosmeticFilterLocationType::Hostname => hostnames_vec.push(hash),
                CosmeticFilterLocationType::Unsupported => {
                    any_unsupported = true;
                }
            }
        }
        // Discard the rule altogether if there are no supported location types
        if any_unsupported
            && hostnames_vec.is_empty()
            && entities_vec.is_empty()
            && not_hostnames_vec.is_empty()
            && not_entities_vec.is_empty()
        {
            return Err(CosmeticFilterError::UnsupportedSyntax);
        }

        /// Sorts `vec` and wraps it in `Some` if it's not empty, or returns `None` if it is.
        #[inline]
        fn sorted_or_none<T: std::cmp::Ord>(mut vec: Vec<T>) -> Option<Vec<T>> {
            if !vec.is_empty() {
                vec.sort();
                Some(vec)
            } else {
                None
            }
        }

        let entities = sorted_or_none(entities_vec);
        let hostnames = sorted_or_none(hostnames_vec);
        let not_entities = sorted_or_none(not_entities_vec);
        let not_hostnames = sorted_or_none(not_hostnames_vec);

        Ok(CosmeticFilterLocations {
            entities,
            not_entities,
            hostnames,
            not_hostnames,
        })
    }

    /// Parses the contents of a cosmetic filter rule following the `##` or `#@#` separator.
    ///
    /// On success, returns `selector` and `style` according to the rule.
    ///
    /// This should only be called if the rule part after the separator has been confirmed not to
    /// be a script injection rule using `+js()`.
    #[inline]
    fn parse_after_sharp_nonscript(
        after_sharp: &str,
    ) -> Result<(&str, Option<CosmeticFilterAction>), CosmeticFilterError> {
        if after_sharp.starts_with('^') {
            return Err(CosmeticFilterError::HtmlFilteringUnsupported);
        }

        const STYLE_TOKEN: &[u8] = b":style(";
        const REMOVE_ATTR_TOKEN: &[u8] = b":remove-attr(";
        const REMOVE_CLASS_TOKEN: &[u8] = b":remove-class(";

        const REMOVE_TOKEN: &str = ":remove()";

        const PAIRS: &[(&[u8], fn(&str) -> Result<CosmeticFilterAction, CosmeticFilterError>)] = &[
            (STYLE_TOKEN, CosmeticFilterAction::new_style),
            (REMOVE_ATTR_TOKEN, CosmeticFilterAction::new_remove_attr),
            (REMOVE_CLASS_TOKEN, CosmeticFilterAction::new_remove_class),
        ];

        let action;
        let selector;

        'init: {
            for (token, constructor) in PAIRS {
                if let Some(i) = memmem::find(after_sharp.as_bytes(), token) {
                    if after_sharp.ends_with(')') {
                        // indexing safe because of find and ends_with
                        let arg = &after_sharp[i + token.len()..after_sharp.len() - 1];

                        action = Some(constructor(arg)?);
                        selector = &after_sharp[..i];
                        break 'init;
                    } else {
                        return Err(CosmeticFilterError::InvalidActionSpecifier);
                    }
                }
            }
            if let Some(before_suffix) = after_sharp.strip_suffix(REMOVE_TOKEN) {
                action = Some(CosmeticFilterAction::Remove);
                selector = before_suffix;
                break 'init;
            } else {
                action = None;
                selector = after_sharp;
            }
        }

        Ok((selector, action))
    }

    /// Returns the CSS selector, for rules which only consist of a CSS selector.
    /// If a rule contains procedural operators, this method will return `None`.
    pub fn plain_css_selector(&self) -> Option<&str> {
        assert!(self.selector.len() > 0);
        if self.selector.len() > 1 {
            return None;
        }
        match &self.selector[0] {
            CosmeticFilterOperator::CssSelector(s) => Some(s),
            _ => None
        }
    }

    /// Parse the rule in `line` into a `CosmeticFilter`. If `debug` is true, the original rule
    /// will be reported in the resulting `CosmeticFilter` struct as well. Use `permission` to
    /// manage the filter's access to scriptlet resources for `+js(...)` injections.
    pub fn parse(line: &str, debug: bool, permission: PermissionMask) -> Result<CosmeticFilter, CosmeticFilterError> {
        let mut mask = CosmeticFilterMask::NONE;
        if let Some(sharp_index) = find_char(b'#', line.as_bytes()) {
            let after_sharp_index = sharp_index + 1;

            let second_sharp_index = match find_char(b'#', line[after_sharp_index..].as_bytes()) {
                Some(i) => i + after_sharp_index,
                None => return Err(CosmeticFilterError::UnsupportedSyntax),
            };

            let mut translate_abp_syntax = false;

            // Consume filter options embedded in the `##` marker:
            let mut between_sharps = &line[after_sharp_index..second_sharp_index];
            if between_sharps.starts_with('@') {
                // Exception marker will always come first.
                if sharp_index == 0 {
                    return Err(CosmeticFilterError::GenericUnhide);
                }
                mask |= CosmeticFilterMask::UNHIDE;
                between_sharps = &between_sharps[1..];
            }
            if between_sharps.starts_with('%') {
                // AdGuard script injection syntax - not supported
                // `#%#` / `#@%#`
                return Err(CosmeticFilterError::UnsupportedSyntax);
            }
            if between_sharps.starts_with('$') {
                // AdGuard `:style` syntax - not supported for now
                // `#$?#` for CSS rules, `#@$?#` — for exceptions
                return Err(CosmeticFilterError::UnsupportedSyntax);
            }
            if between_sharps.starts_with('?') {
                // ABP/ADG extended CSS syntax:
                // - #?# — for element hiding, #@?# — for exceptions
                translate_abp_syntax = true;
                between_sharps = &between_sharps[1..];
            }
            if !between_sharps.is_empty() {
                return Err(CosmeticFilterError::UnsupportedSyntax);
            }

            let suffix_start_index = second_sharp_index + 1;

            let CosmeticFilterLocations {
                entities,
                not_entities,
                hostnames,
                not_hostnames,
            } = if sharp_index > 0 {
                CosmeticFilter::parse_before_sharp(line, sharp_index)?
            } else {
                CosmeticFilterLocations::default()
            };

            let after_sharp = &line[suffix_start_index..].trim();

            if after_sharp.is_empty() {
                return Err(CosmeticFilterError::EmptyRule);
            }

            let (selector, action) = if line.len() - suffix_start_index > 4
                && line[suffix_start_index..].starts_with("+js(")
                && line.ends_with(')')
            {
                if sharp_index == 0 {
                    return Err(CosmeticFilterError::GenericScriptInject);
                }
                let args = &line[suffix_start_index + 4..line.len() - 1];
                if crate::resources::parse_scriptlet_args(args).is_none() {
                    return Err(CosmeticFilterError::InvalidScriptletArgs);
                }
                mask |= CosmeticFilterMask::SCRIPT_INJECT;
                (
                    // TODO: overloading `CssSelector` here is not ideal.
                    vec![CosmeticFilterOperator::CssSelector(String::from(&line[suffix_start_index + 4..line.len() - 1]))],
                    None,
                )
            } else {
                let (selector, action) = CosmeticFilter::parse_after_sharp_nonscript(after_sharp)?;
                let validated_selector = validate_css_selector(selector, translate_abp_syntax)?;
                if sharp_index == 0 && action.is_some() {
                    return Err(CosmeticFilterError::GenericAction);
                }
                (validated_selector, action)
            };

            if (not_entities.is_some() || not_hostnames.is_some())
                && mask.contains(CosmeticFilterMask::UNHIDE)
            {
                return Err(CosmeticFilterError::DoubleNegation);
            }

            let this = Self {
                entities,
                hostnames,
                mask,
                not_entities,
                not_hostnames,
                raw_line: if debug {
                    Some(Box::new(String::from(line)))
                } else {
                    None
                },
                selector,
                action,
                permission,
            };

            if !this.has_hostname_constraint() && this.plain_css_selector().is_none() {
                return Err(CosmeticFilterError::GenericAction);
            }

            Ok(this)
        } else {
            Err(CosmeticFilterError::MissingSharp)
        }
    }

    /// Any cosmetic filter rule that specifies (possibly negated) hostnames or entities has a
    /// hostname constraint.
    pub fn has_hostname_constraint(&self) -> bool {
        self.hostnames.is_some()
            || self.entities.is_some()
            || self.not_entities.is_some()
            || self.not_hostnames.is_some()
    }

    /// In general, adding a hostname or entity to a rule *increases* the number of situations in
    /// which it applies. However, if a specific rule only has negated hostnames or entities, it
    /// technically should apply to any hostname which does not match a negation.
    ///
    /// See: <https://github.com/chrisaljoudi/uBlock/issues/145>
    ///
    /// To account for this inconsistency, this method will generate and return the corresponding
    /// 'hidden' generic rule if one applies.
    ///
    /// Note that this behavior is not applied to script injections or rules with actions.
    pub fn hidden_generic_rule(&self) -> Option<CosmeticFilter> {
        if self.hostnames.is_some() || self.entities.is_some() {
            None
        } else if (self.not_hostnames.is_some() || self.not_entities.is_some())
            && (self.action.is_none() && !self.mask.contains(CosmeticFilterMask::SCRIPT_INJECT))
        {
            let mut generic_rule = self.clone();
            generic_rule.not_hostnames = None;
            generic_rule.not_entities = None;
            Some(generic_rule)
        } else {
            None
        }
    }
}

/// Returns a slice of `hostname` up to and including the segment that overlaps with the first
/// segment of `domain`, which has the effect of stripping ".com", ".co.uk", etc., as well as the
/// public suffix itself.
fn get_hostname_without_public_suffix<'a>(
    hostname: &'a str,
    domain: &str,
) -> Option<(&'a str, &'a str)> {
    let index_of_dot = find_char(b'.', domain.as_bytes());

    if let Some(index_of_dot) = index_of_dot {
        let public_suffix = &domain[index_of_dot + 1..];
        Some((
            &hostname[0..hostname.len() - public_suffix.len() - 1],
            &hostname[hostname.len() - domain.len() + index_of_dot + 1..],
        ))
    } else {
        None
    }
}

/// Given a hostname and the indices of an end position and the start of the domain, returns a
/// `Vec` of hashes of all subdomains the hostname falls under, ordered from least to most
/// specific.
///
/// Check the `label_hashing` tests for examples.
fn get_hashes_from_labels(hostname: &str, end: usize, start_of_domain: usize) -> Vec<Hash> {
    let mut hashes = vec![];
    if end == 0 {
        return hashes;
    }
    let mut dot_ptr = start_of_domain;

    while let Some(dot_index) = find_char_reverse(b'.', hostname[..dot_ptr].as_bytes()) {
        dot_ptr = dot_index;
        hashes.push(crate::utils::fast_hash(&hostname[dot_ptr + 1..end]));
    }

    hashes.push(crate::utils::fast_hash(&hostname[..end]));

    hashes
}

/// Returns a `Vec` of the hashes of all segments of `hostname` that may match an
/// entity-constrained rule.
pub(crate) fn get_entity_hashes_from_labels(hostname: &str, domain: &str) -> Vec<Hash> {
    if let Some((hostname_without_public_suffix, public_suffix)) =
        get_hostname_without_public_suffix(hostname, domain)
    {
        let mut hashes = get_hashes_from_labels(
            hostname_without_public_suffix,
            hostname_without_public_suffix.len(),
            hostname_without_public_suffix.len(),
        );
        hashes.push(crate::utils::fast_hash(public_suffix));
        hashes
    } else {
        vec![]
    }
}

/// Returns a `Vec` of the hashes of all segments of `hostname` that may match a
/// hostname-constrained rule.
pub(crate) fn get_hostname_hashes_from_labels(hostname: &str, domain: &str) -> Vec<Hash> {
    get_hashes_from_labels(hostname, hostname.len(), hostname.len() - domain.len())
}

#[cfg(not(feature = "css-validation"))]
mod css_validation {
    use super::{CosmeticFilterError, CosmeticFilterOperator};

    pub fn validate_css_selector(selector: &str, _accept_abp_selectors: bool) -> Result<Vec<CosmeticFilterOperator>, CosmeticFilterError> {
        Ok(vec![CosmeticFilterOperator::CssSelector(selector.to_string())])
    }

    pub fn is_valid_css_style(_style: &str) -> bool {
        true
    }
}

#[cfg(feature = "css-validation")]
mod css_validation {
    //! Methods for validating CSS selectors and style rules extracted from cosmetic filter rules.
    use core::fmt::{Result as FmtResult, Write};
    use cssparser::{CowRcStr, ParseError, Parser, ParserInput, SourceLocation, ToCss, Token};
    use selectors::parser::SelectorParseErrorKind;
    use super::{CosmeticFilterError, CosmeticFilterOperator};

    /// Returns a validated canonical CSS selector for the given input, or nothing if one can't be
    /// determined.
    ///
    /// For the majority of filters, this works by trivial regex matching. More complex filters are
    /// assembled into a mock stylesheet which is then parsed using `cssparser` and validated.
    ///
    /// In addition to normalizing formatting, this function will remove unsupported procedural
    /// selectors and convert others to canonical representations (i.e. `:-abp-has` -> `:has`).
    pub fn validate_css_selector(selector: &str, accept_abp_selectors: bool) -> Result<Vec<CosmeticFilterOperator>, CosmeticFilterError> {
        use once_cell::sync::Lazy;
        use regex::Regex;
        static RE_SIMPLE_SELECTOR: Lazy<Regex> =
            Lazy::new(|| Regex::new(r"^[#.]?[A-Za-z_][\w-]*$").unwrap());

        if RE_SIMPLE_SELECTOR.is_match(selector) {
            return Ok(vec![CosmeticFilterOperator::CssSelector(selector.to_string())]);
        }

        // Use `mock-stylesheet-marker` where uBO uses `color: red` since we have control over the
        // parsing logic within the block.
        let mock_stylesheet = format!("{}{{mock-stylesheet-marker}}", selector);
        let mut pi = ParserInput::new(&mock_stylesheet);
        let mut parser = Parser::new(&mut pi);
        let mut rule_list_parser = cssparser::RuleListParser::new_for_stylesheet(
            &mut parser,
            QualifiedRuleParserImpl {
                accept_abp_selectors,
            },
        );

        let prelude = rule_list_parser
            .next()
            .and_then(|r| r.ok());

        // There should only be one rule
        if rule_list_parser.next().is_some() {
            return Err(CosmeticFilterError::InvalidCssSelector);
        }

        fn has_procedural_operator(selector: &selectors::parser::Selector<SelectorImpl>) -> bool {
            let mut iter = selector.iter();
            loop {
                while let Some(component) = iter.next() {
                    if is_procedural_operator(component) {
                        return true;
                    }
                }
                if iter.next_sequence().is_none() {
                    break false;
                }
            }
        }

        fn is_procedural_operator(c: &selectors::parser::Component<SelectorImpl>) -> bool {
            use selectors::parser::Component;
            // Avoid using `to_procedural_operator.is_some()`, which will re-allocate the argument string.
            match c {
                Component::NonTSPseudoClass(NonTSPseudoClass::HasText(_)) => true,
                Component::NonTSPseudoClass(NonTSPseudoClass::MatchesAttr(_)) => true,
                Component::NonTSPseudoClass(NonTSPseudoClass::MatchesCss(_)) => true,
                Component::NonTSPseudoClass(NonTSPseudoClass::MatchesCssBefore(_)) => true,
                Component::NonTSPseudoClass(NonTSPseudoClass::MatchesCssAfter(_)) => true,
                Component::NonTSPseudoClass(NonTSPseudoClass::MatchesPath(_)) => true,
                Component::NonTSPseudoClass(NonTSPseudoClass::MinTextLength(_)) => true,
                Component::NonTSPseudoClass(NonTSPseudoClass::Upward(_)) => true,
                Component::NonTSPseudoClass(NonTSPseudoClass::Xpath(_)) => true,
                _ => false,
            }
        }

        if let Some(prelude) = prelude {
            if !prelude.0.iter().any(|s| has_procedural_operator(s)) {
                // There are no procedural filters, so all selectors use standard CSS.
                // It's ok to return that as a "single" selector.
                return Ok(vec![CosmeticFilterOperator::CssSelector(prelude.to_css_string())]);
            }

            if prelude.0.len() != 1 {
                // Procedural filters don't work well with multiple selectors
                return Err(CosmeticFilterError::ProceduralFilterWithMultipleSelectors);
            }

            // Safe; early return if length is not 1
            let selector = prelude.0.into_iter().next().unwrap();

            /// Shim for items returned by `selectors::parser::SelectorIter`. Sequences and
            /// components iterate in opposite directions, but we want to process them in a
            /// consistent order, so they need to be manually collected and reversed.
            enum SelectorsPart<'a> {
                /// `SelectorIter` iterates over `Component`s from left-to-right.
                Component(&'a selectors::parser::Component<SelectorImpl>),
                /// `SelectorIter` iterates over sequences of `Component`s separated by
                /// `Combinator`s from right-to-left.
                Combinator(selectors::parser::Combinator),
            }

            // Collect the selector parts into `parts` in right-to-left order, reversing the
            // components of each sequence as necessary.
            let mut parts = vec![];
            let mut iter = selector.iter();
            loop {
                // Manual iteration/collection necessary due to `SelectorIter`'s multi-iterator
                // construction.
                // - `.rev()` cannot work directly because it's not a fixed-size iterator.
                // - `.collect()` cannot work because it takes ownership of the iterator, which is
                //                still required later for `next_sequence`.
                let mut components = vec![];
                while let Some(component) = iter.next() {
                    components.push(SelectorsPart::Component(component));
                }
                parts.extend(components.into_iter().rev());
                if let Some(combinator) = iter.next_sequence() {
                    parts.push(SelectorsPart::Combinator(combinator));
                } else {
                    break;
                }
            }

            // We can now prepare `output` by iterating over all the parts of the selector in
            // left-to-right order.
            let mut pending_css_selector = String::new();
            let mut output = vec![];
            for part in parts.into_iter().rev() {
                use selectors::parser::Component;
                match part {
                    SelectorsPart::Component(Component::NonTSPseudoClass(c)) => {
                        if let Some(procedural_operator) = c.to_procedural_operator() {
                            if !pending_css_selector.is_empty() {
                                output.push(CosmeticFilterOperator::CssSelector(pending_css_selector));
                                pending_css_selector = String::new();
                            }
                            output.push(procedural_operator);
                        } else {
                            c.to_css(&mut pending_css_selector).map_err(|_| CosmeticFilterError::InvalidCssSelector)?;
                        }
                    }
                    SelectorsPart::Component(other) => {
                        other.to_css(&mut pending_css_selector).map_err(|_| CosmeticFilterError::InvalidCssSelector)?;
                    }
                    SelectorsPart::Combinator(combinator) => {
                        combinator.to_css(&mut pending_css_selector).map_err(|_| CosmeticFilterError::InvalidCssSelector)?;
                    }
                }
            }
            if !pending_css_selector.is_empty() {
                output.push(CosmeticFilterOperator::CssSelector(pending_css_selector));
            }

            Ok(output)
        } else {
            Err(CosmeticFilterError::InvalidCssSelector)
        }
    }

    struct QualifiedRuleParserImpl {
        accept_abp_selectors: bool,
    }

    impl<'i> cssparser::QualifiedRuleParser<'i> for QualifiedRuleParserImpl {
        type Prelude = selectors::SelectorList<SelectorImpl>;
        type QualifiedRule = selectors::SelectorList<SelectorImpl>;
        type Error = ();

        fn parse_prelude<'t>(
            &mut self,
            input: &mut Parser<'i, 't>,
        ) -> Result<Self::Prelude, ParseError<'i, Self::Error>> {
            selectors::SelectorList::parse(
                &SelectorParseImpl {
                    accept_abp_selectors: self.accept_abp_selectors,
                },
                input,
            )
            .map_err(|_| ParseError {
                kind: cssparser::ParseErrorKind::Custom(()),
                location: SourceLocation { line: 0, column: 0 },
            })
        }

        /// Check that the block is exactly equal to "mock-stylesheet-marker", and return just the
        /// selector list as the prelude
        fn parse_block<'t>(
            &mut self,
            prelude: Self::Prelude,
            _start: &cssparser::ParserState,
            input: &mut Parser<'i, 't>,
        ) -> Result<Self::QualifiedRule, ParseError<'i, Self::Error>> {
            let err = Err(ParseError {
                kind: cssparser::ParseErrorKind::Custom(()),
                location: SourceLocation { line: 0, column: 0 },
            });
            match input.next() {
                Ok(Token::Ident(i)) if i.as_ref() == "mock-stylesheet-marker" => (),
                _ => return err,
            }
            if input.next().is_ok() {
                return err;
            }
            Ok(prelude)
        }
    }

    /// Default implementations for `AtRuleParser` parsing methods return false. This is
    /// acceptable; at-rules should not be valid in cosmetic rules.
    impl cssparser::AtRuleParser<'_> for QualifiedRuleParserImpl {
        type PreludeNoBlock = ();
        type PreludeBlock = ();
        /// unused; just to satisfy type checking
        type AtRule = selectors::SelectorList<SelectorImpl>;
        type Error = ();
    }

    pub fn is_valid_css_style(style: &str) -> bool {
        if style.contains('\\') {
            return false;
        }
        if style.contains("url(") {
            return false;
        }
        if style.contains("/*") {
            return false;
        }
        true
    }

    struct SelectorParseImpl {
        accept_abp_selectors: bool,
    }

    fn nested_matching_close(arg: &Token) -> Option<Token<'static>> {
        match arg {
            Token::Function(..) => Some(Token::CloseParenthesis),
            Token::ParenthesisBlock => Some(Token::CloseParenthesis),
            Token::CurlyBracketBlock => Some(Token::CloseCurlyBracket),
            Token::SquareBracketBlock => Some(Token::CloseSquareBracket),
            _ => None,
        }
    }

    /// Just convert the rest of the selector to a string
    fn to_css_nested<'i>(
        arguments: &mut Parser<'i, '_>,
    ) -> Result<String, ParseError<'i, SelectorParseErrorKind<'i>>> {
        let mut inner = String::new();
        while let Ok(arg) = arguments.next_including_whitespace() {
            if arg.to_css(&mut inner).is_err() {
                return Err(arguments.new_custom_error(SelectorParseErrorKind::InvalidState));
            };
            if let Some(closing_token) = nested_matching_close(arg) {
                let nested = arguments.parse_nested_block(to_css_nested)?;
                inner.push_str(&nested);
                closing_token.to_css(&mut inner).map_err(|_| {
                    arguments.new_custom_error(SelectorParseErrorKind::InvalidState)
                })?;
            }
        }
        Ok(inner)
    }

    impl<'i> selectors::parser::Parser<'i> for SelectorParseImpl {
        type Impl = SelectorImpl;
        type Error = SelectorParseErrorKind<'i>;

        fn parse_slotted(&self) -> bool {
            true
        }
        fn parse_part(&self) -> bool {
            true
        }
        fn parse_is_and_where(&self) -> bool {
            true
        }
        fn parse_host(&self) -> bool {
            true
        }
        fn parse_non_ts_pseudo_class(
            &self,
            _location: SourceLocation,
            name: CowRcStr<'i>,
        ) -> Result<
            <Self::Impl as selectors::parser::SelectorImpl>::NonTSPseudoClass,
            ParseError<'i, Self::Error>,
        > {
            Ok(NonTSPseudoClass::AnythingElse(name.to_string(), None))
        }
        fn parse_non_ts_functional_pseudo_class<'t>(
            &self,
            name: CowRcStr<'i>,
            arguments: &mut Parser<'i, 't>,
        ) -> Result<
            <Self::Impl as selectors::parser::SelectorImpl>::NonTSPseudoClass,
            ParseError<'i, Self::Error>,
        > {
            let canonical_name = match (self.accept_abp_selectors, name.as_ref()) {
                (true, "-abp-has") => Some("has"),
                (true, "-abp-contains") => Some("has-text"),
                (true, "contains") => Some("has-text"),
                _ => None,
            }
            .unwrap_or(name.as_ref());
            match canonical_name {
                "has-text" => {
                    let text = to_css_nested(arguments)?;
                    return Ok(NonTSPseudoClass::HasText(text));
                }
                "matches-attr" => {
                    let text = to_css_nested(arguments)?;
                    return Ok(NonTSPseudoClass::MatchesAttr(text));
                }
                "matches-css" => {
                    let text = to_css_nested(arguments)?;
                    return Ok(NonTSPseudoClass::MatchesCss(text));
                }
                "matches-css-before" => {
                    let text = to_css_nested(arguments)?;
                    return Ok(NonTSPseudoClass::MatchesCssBefore(text));
                }
                "matches-css-after" => {
                    let text = to_css_nested(arguments)?;
                    return Ok(NonTSPseudoClass::MatchesCssAfter(text));
                }
                "matches-path" => {
                    let text = to_css_nested(arguments)?;
                    return Ok(NonTSPseudoClass::MatchesPath(text));
                }
                "min-text-length" => {
                    let text = to_css_nested(arguments)?;
                    return Ok(NonTSPseudoClass::MinTextLength(text));
                }
                "upward" => {
                    let text = to_css_nested(arguments)?;
                    return Ok(NonTSPseudoClass::Upward(text));
                }
                "xpath" => {
                    let text = to_css_nested(arguments)?;
                    return Ok(NonTSPseudoClass::Xpath(text));
                }
                "-abp-contains" | "-abp-has" | "-abp-properties" | "contains" | "if" | "if-not"
                | "matches-property" | "nth-ancestor" | "properties" | "subject" | "remove"
                | "remove-attr" | "remove-class" => {
                    return Err(arguments.new_custom_error(
                        SelectorParseErrorKind::UnsupportedPseudoClassOrElement(name),
                    ))
                }
                _ => (),
            }

            let inner_selector = to_css_nested(arguments)?;

            Ok(NonTSPseudoClass::AnythingElse(
                canonical_name.to_string(),
                Some(inner_selector),
            ))
        }
        fn parse_pseudo_element(
            &self,
            _location: SourceLocation,
            name: CowRcStr<'i>,
        ) -> Result<
            <Self::Impl as selectors::parser::SelectorImpl>::PseudoElement,
            ParseError<'i, Self::Error>,
        > {
            Ok(PseudoElement(name.to_string(), None))
        }
        fn parse_functional_pseudo_element<'t>(
            &self,
            name: CowRcStr<'i>,
            arguments: &mut Parser<'i, 't>,
        ) -> Result<
            <Self::Impl as selectors::parser::SelectorImpl>::PseudoElement,
            ParseError<'i, Self::Error>,
        > {
            let inner_selector = to_css_nested(arguments)?;
            Ok(PseudoElement(name.to_string(), Some(inner_selector)))
        }
    }

    /// The `selectors` library requires an object that implements `SelectorImpl` to store data
    /// about a parsed selector. For performance, the actual content of parsed selectors is
    /// discarded as much as possible - it only matters whether the returned `Result` is `Ok` or
    /// `Err`.
    #[derive(Debug, Clone)]
    struct SelectorImpl;

    impl selectors::parser::SelectorImpl for SelectorImpl {
        type ExtraMatchingData = ();
        type AttrValue = CssString;
        type Identifier = CssIdent;
        type LocalName = CssString;
        type NamespaceUrl = DummyValue;
        type NamespacePrefix = DummyValue;
        type BorrowedNamespaceUrl = DummyValue;
        type BorrowedLocalName = CssString;
        type NonTSPseudoClass = NonTSPseudoClass;
        type PseudoElement = PseudoElement;
    }

    /// Serialized using `CssStringWriter`.
    #[derive(Debug, Clone, PartialEq, Eq, Default)]
    struct CssString(String);

    impl ToCss for CssString {
        fn to_css<W: Write>(&self, dest: &mut W) -> core::fmt::Result {
            cssparser::CssStringWriter::new(dest).write_str(&self.0)
        }
    }

    impl<'a> From<&'a str> for CssString {
        fn from(s: &'a str) -> Self {
            CssString(s.to_string())
        }
    }

    /// Serialized using `serialize_identifier`.
    #[derive(Debug, Clone, PartialEq, Eq, Default)]
    struct CssIdent(String);

    impl ToCss for CssIdent {
        fn to_css<W: Write>(&self, dest: &mut W) -> core::fmt::Result {
            cssparser::serialize_identifier(&self.0, dest)
        }
    }

    impl<'a> From<&'a str> for CssIdent {
        fn from(s: &'a str) -> Self {
            CssIdent(s.to_string())
        }
    }

    /// For performance, individual fields of parsed selectors are discarded. Instead, they are
    /// parsed into a `DummyValue` with no fields.
    #[derive(Debug, Clone, PartialEq, Eq, Default)]
    struct DummyValue(String);

    impl ToCss for DummyValue {
        fn to_css<W: Write>(&self, dest: &mut W) -> core::fmt::Result {
            write!(dest, "{}", self.0)
        }
    }

    impl<'a> From<&'a str> for DummyValue {
        fn from(s: &'a str) -> Self {
            DummyValue(s.to_string())
        }
    }

    /// Dummy struct for non-tree-structural pseudo-classes.
    #[derive(Clone, PartialEq, Eq)]
    enum NonTSPseudoClass {
        /// The `:has-text` or `:-abp-contains` procedural operator.
        HasText(String),
        /// The `:matches-attr` procedural operator.
        MatchesAttr(String),
        /// The `:matches-css` procedural operator.
        MatchesCss(String),
        /// The `:matches-css-before` procedural operator.
        MatchesCssBefore(String),
        /// The `:matches-css-after` procedural operator.
        MatchesCssAfter(String),
        /// The `:matches-path` procedural operator.
        MatchesPath(String),
        /// The `:min-text-length` procedural operator.
        MinTextLength(String),
        /// The `:upward` procedural operator.
        Upward(String),
        /// The `:xpath` procedural operator.
        Xpath(String),
        /// Any native CSS pseudoclass that isn't a procedural operator. Second argument contains inner arguments, if present.
        AnythingElse(String, Option<String>),
    }

    impl selectors::parser::NonTSPseudoClass for NonTSPseudoClass {
        type Impl = SelectorImpl;
        fn is_active_or_hover(&self) -> bool {
            false
        }
        fn is_user_action_state(&self) -> bool {
            false
        }
    }

    impl ToCss for NonTSPseudoClass {
        fn to_css<W: Write>(&self, dest: &mut W) -> FmtResult {
            write!(dest, ":")?;
            match self {
                Self::HasText(text) => write!(dest, "has-text({})", text)?,
                Self::MatchesAttr(text) => write!(dest, "matches-attr({})", text)?,
                Self::MatchesCss(text) => write!(dest, "matches-css({})", text)?,
                Self::MatchesCssBefore(text) => write!(dest, "matches-css-before({})", text)?,
                Self::MatchesCssAfter(text) => write!(dest, "matches-css-after({})", text)?,
                Self::MatchesPath(text) => write!(dest, "matches-path({})", text)?,
                Self::MinTextLength(text) => write!(dest, "min-text-length({})", text)?,
                Self::Upward(text) => write!(dest, "upward({})", text)?,
                Self::Xpath(text) => write!(dest, "xpath({})", text)?,
                Self::AnythingElse(name, None) => write!(dest, "{}", name)?,
                Self::AnythingElse(name, Some(args)) => write!(dest, "{}({})", name, args)?,
            }
            Ok(())
        }
    }

    impl NonTSPseudoClass {
        fn to_procedural_operator(&self) -> Option<CosmeticFilterOperator> {
            match self {
                NonTSPseudoClass::HasText(a) => Some(CosmeticFilterOperator::HasText(a.to_owned())),
                NonTSPseudoClass::MatchesAttr(a) => Some(CosmeticFilterOperator::MatchesAttr(a.to_owned())),
                NonTSPseudoClass::MatchesCss(a) => Some(CosmeticFilterOperator::MatchesCss(a.to_owned())),
                NonTSPseudoClass::MatchesCssBefore(a) => Some(CosmeticFilterOperator::MatchesCssBefore(a.to_owned())),
                NonTSPseudoClass::MatchesCssAfter(a) => Some(CosmeticFilterOperator::MatchesCssAfter(a.to_owned())),
                NonTSPseudoClass::MatchesPath(a) => Some(CosmeticFilterOperator::MatchesPath(a.to_owned())),
                NonTSPseudoClass::MinTextLength(a) => Some(CosmeticFilterOperator::MinTextLength(a.to_owned())),
                NonTSPseudoClass::Upward(a) => Some(CosmeticFilterOperator::Upward(a.to_owned())),
                NonTSPseudoClass::Xpath(a) => Some(CosmeticFilterOperator::Xpath(a.to_owned())),
                _ => None,
            }
        }
    }

    /// Dummy struct for pseudo-elements.
    #[derive(Clone, PartialEq, Eq)]
    struct PseudoElement(String, Option<String>);

    impl selectors::parser::PseudoElement for PseudoElement {
        type Impl = SelectorImpl;

        fn valid_after_slotted(&self) -> bool {
            true
        }
    }

    impl ToCss for PseudoElement {
        fn to_css<W: Write>(&self, dest: &mut W) -> FmtResult {
            write!(dest, "::")?;
            match self {
                Self(name, None) => write!(dest, "{}", name)?,
                Self(name, Some(args)) => write!(dest, "{}({})", name, args)?,
            }
            Ok(())
        }
    }

    #[test]
    fn bad_selector_inputs() {
        assert!(validate_css_selector(r#"rm -rf ./*"#, false).is_err());
        assert!(validate_css_selector(r#"javascript:alert("All pseudo-classes are valid")"#, false).is_ok());
        assert!(validate_css_selector(r#"javascript:alert("But opening comments are still forbidden" /*)"#, false).is_err());
        assert!(validate_css_selector(r#"This is not a CSS selector."#, false).is_err());
        assert!(validate_css_selector(r#"./malware.sh"#, false).is_err());
        assert!(validate_css_selector(r#"https://safesite.ru"#, false).is_err());
        assert!(validate_css_selector(r#"(function(){var e=60;return String.fromCharCode(e.charCodeAt(0))})();"#, false).is_err());
        assert!(validate_css_selector(r#"#!/usr/bin/sh"#, false).is_err());
        assert!(validate_css_selector(r#"input,input/*"#, false).is_err());
        // Accept a closing comment within a string. It should still be impossible to create an
        // opening comment to match it.
        assert!(validate_css_selector(r#"input[x="*/{}*{background:url(https://hackvertor.co.uk/images/logo.gif)}"]"#, false).is_ok());
    }

    #[test]
    fn escaped_quote_in_tag_name() {
        assert_eq!(validate_css_selector(r#"head\""#, false), Ok(vec![CosmeticFilterOperator::CssSelector(r#"head\""#.to_string())]));
    }
}

#[cfg(test)]
mod parse_tests {
    use super::*;

    /// An easily modified summary of a `CosmeticFilter` rule to be used in tests.
    #[derive(Debug, PartialEq)]
    struct CosmeticFilterBreakdown {
        entities: Option<Vec<Hash>>,
        hostnames: Option<Vec<Hash>>,
        not_entities: Option<Vec<Hash>>,
        not_hostnames: Option<Vec<Hash>>,
        selector: SelectorType,
        action: Option<CosmeticFilterAction>,

        unhide: bool,
        script_inject: bool,
    }

    impl From<&CosmeticFilter> for CosmeticFilterBreakdown {
        fn from(filter: &CosmeticFilter) -> CosmeticFilterBreakdown {
            CosmeticFilterBreakdown {
                entities: filter.entities.as_ref().cloned(),
                hostnames: filter.hostnames.as_ref().cloned(),
                not_entities: filter.not_entities.as_ref().cloned(),
                not_hostnames: filter.not_hostnames.as_ref().cloned(),
                selector: SelectorType::from(filter),
                action: filter.action.as_ref().cloned(),

                unhide: filter.mask.contains(CosmeticFilterMask::UNHIDE),
                script_inject: filter.mask.contains(CosmeticFilterMask::SCRIPT_INJECT),
            }
        }
    }

    impl From<CosmeticFilter> for CosmeticFilterBreakdown {
        fn from(filter: CosmeticFilter) -> CosmeticFilterBreakdown {
            (&filter).into()
        }
    }

    impl Default for CosmeticFilterBreakdown {
        fn default() -> Self {
            CosmeticFilterBreakdown {
                entities: None,
                hostnames: None,
                not_entities: None,
                not_hostnames: None,
                selector: SelectorType::PlainCss(String::from("")),
                action: None,

                unhide: false,
                script_inject: false,
            }
        }
    }

    #[derive(Debug, PartialEq)]
    enum SelectorType {
        PlainCss(String),
        Procedural(Vec<CosmeticFilterOperator>),
    }

    impl From<&CosmeticFilter> for SelectorType {
        fn from(v: &CosmeticFilter) -> Self {
            if let Some(selector) = v.plain_css_selector() {
                Self::PlainCss(selector.to_string())
            } else {
                Self::Procedural(v.selector.clone())
            }
        }
    }

    fn parse_cf(rule: &str) -> Result<CosmeticFilter, CosmeticFilterError> {
        CosmeticFilter::parse(rule, false, Default::default())
    }

    /// Asserts that `rule` parses into a `CosmeticFilter` equivalent to the summary provided by
    /// `expected`.
    fn check_parse_result(rule: &str, expected: CosmeticFilterBreakdown) {
        let filter: CosmeticFilterBreakdown = parse_cf(rule).unwrap().into();
        assert_eq!(expected, filter);
    }

    #[test]
    fn simple_selectors() {
        check_parse_result(
            "##div.popup",
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss("div.popup".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            "###selector",
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss("#selector".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            "##.selector",
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(".selector".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            "##a[href=\"foo.com\"]",
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss("a[href=\"foo.com\"]".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            "##[href=\"foo.com\"]",
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss("[href=\"foo.com\"]".to_string()),
                ..Default::default()
            },
        );
    }

    /// Produces a sorted vec of the hashes of all the given domains.
    ///
    /// For convenience, the return value is wrapped in a `Some()` to be consumed by a
    /// `CosmeticFilterBreakdown`.
    fn sort_hash_domains(domains: Vec<&str>) -> Option<Vec<Hash>> {
        let mut hashes: Vec<_> = domains.iter().map(|d| crate::utils::fast_hash(d)).collect();
        hashes.sort();
        Some(hashes)
    }

    #[test]
    fn hostnames() {
        check_parse_result(
            r#"u00p.com##div[class^="adv-box"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"div[class^="adv-box"]"#.to_string()),
                hostnames: sort_hash_domains(vec!["u00p.com"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"distractify.com##div[class*="AdInArticle"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"div[class*="AdInArticle"]"#.to_string()),
                hostnames: sort_hash_domains(vec!["distractify.com"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"soundtrackcollector.com,the-numbers.com##a[href^="http://affiliates.allposters.com/"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"a[href^="http://affiliates.allposters.com/"]"#.to_string()),
                hostnames: sort_hash_domains(vec!["soundtrackcollector.com", "the-numbers.com"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"thelocal.at,thelocal.ch,thelocal.de,thelocal.dk,thelocal.es,thelocal.fr,thelocal.it,thelocal.no,thelocal.se##div[class*="-widget"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"div[class*="-widget"]"#.to_string()),
                hostnames: sort_hash_domains(vec![
                    "thelocal.at",
                    "thelocal.ch",
                    "thelocal.de",
                    "thelocal.dk",
                    "thelocal.es",
                    "thelocal.fr",
                    "thelocal.it",
                    "thelocal.no",
                    "thelocal.se",
                ]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"base64decode.org,base64encode.org,beautifyjson.org,minifyjson.org,numgen.org,pdfmrg.com,pdfspl.com,prettifycss.com,pwdgen.org,strlength.com,strreverse.com,uglifyjs.net,urldecoder.org##div[class^="banner_"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"div[class^="banner_"]"#.to_string()),
                hostnames: sort_hash_domains(vec![
                    "base64decode.org",
                    "base64encode.org",
                    "beautifyjson.org",
                    "minifyjson.org",
                    "numgen.org",
                    "pdfmrg.com",
                    "pdfspl.com",
                    "prettifycss.com",
                    "pwdgen.org",
                    "strlength.com",
                    "strreverse.com",
                    "uglifyjs.net",
                    "urldecoder.org",
                ]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"adforum.com,alliednews.com,americustimesrecorder.com,andovertownsman.com,athensreview.com,batesvilleheraldtribune.com,bdtonline.com,channel24.pk,chickashanews.com,claremoreprogress.com,cleburnetimesreview.com,clintonherald.com,commercejournal.com,commercial-news.com,coopercrier.com,cordeledispatch.com,corsicanadailysun.com,crossville-chronicle.com,cullmantimes.com,dailyiowegian.com,dailyitem.com,daltondailycitizen.com,derrynews.com,duncanbanner.com,eagletribune.com,edmondsun.com,effinghamdailynews.com,enewscourier.com,enidnews.com,farmtalknewspaper.com,fayettetribune.com,flasharcade.com,flashgames247.com,flyergroup.com,foxsportsasia.com,gainesvilleregister.com,gloucestertimes.com,goshennews.com,greensburgdailynews.com,heraldbanner.com,heraldbulletin.com,hgazette.com,homemagonline.com,itemonline.com,jacksonvilleprogress.com,jerusalemonline.com,joplinglobe.com,journal-times.com,journalexpress.net,kexp.org,kokomotribune.com,lockportjournal.com,mankatofreepress.com,mcalesternews.com,mccrearyrecord.com,mcleansborotimesleader.com,meadvilletribune.com,meridianstar.com,mineralwellsindex.com,montgomery-herald.com,mooreamerican.com,moultrieobserver.com,muskogeephoenix.com,ncnewsonline.com,newburyportnews.com,newsaegis.com,newsandtribune.com,niagara-gazette.com,njeffersonnews.com,normantranscript.com,opposingviews.com,orangeleader.com,oskaloosa.com,ottumwacourier.com,outlookmoney.com,palestineherald.com,panews.com,paulsvalleydailydemocrat.com,pellachronicle.com,pharostribune.com,pressrepublican.com,pryordailytimes.com,randolphguide.com,record-eagle.com,register-herald.com,register-news.com,reporter.net,rockwallheraldbanner.com,roysecityheraldbanner.com,rushvillerepublican.com,salemnews.com,sentinel-echo.com,sharonherald.com,shelbyvilledailyunion.com,siteslike.com,standardmedia.co.ke,starbeacon.com,stwnewspress.com,suwanneedemocrat.com,tahlequahdailypress.com,theadanews.com,theawesomer.com,thedailystar.com,thelandonline.com,themoreheadnews.com,thesnaponline.com,tiftongazette.com,times-news.com,timesenterprise.com,timessentinel.com,timeswv.com,tonawanda-news.com,tribdem.com,tribstar.com,unionrecorder.com,valdostadailytimes.com,washtimesherald.com,waurikademocrat.com,wcoutlook.com,weatherforddemocrat.com,woodwardnews.net,wrestlinginc.com##div[style="width:300px; height:250px;"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"div[style="width:300px; height:250px;"]"#.to_string()),
                hostnames: sort_hash_domains(vec![
                    "adforum.com",
                    "alliednews.com",
                    "americustimesrecorder.com",
                    "andovertownsman.com",
                    "athensreview.com",
                    "batesvilleheraldtribune.com",
                    "bdtonline.com",
                    "channel24.pk",
                    "chickashanews.com",
                    "claremoreprogress.com",
                    "cleburnetimesreview.com",
                    "clintonherald.com",
                    "commercejournal.com",
                    "commercial-news.com",
                    "coopercrier.com",
                    "cordeledispatch.com",
                    "corsicanadailysun.com",
                    "crossville-chronicle.com",
                    "cullmantimes.com",
                    "dailyiowegian.com",
                    "dailyitem.com",
                    "daltondailycitizen.com",
                    "derrynews.com",
                    "duncanbanner.com",
                    "eagletribune.com",
                    "edmondsun.com",
                    "effinghamdailynews.com",
                    "enewscourier.com",
                    "enidnews.com",
                    "farmtalknewspaper.com",
                    "fayettetribune.com",
                    "flasharcade.com",
                    "flashgames247.com",
                    "flyergroup.com",
                    "foxsportsasia.com",
                    "gainesvilleregister.com",
                    "gloucestertimes.com",
                    "goshennews.com",
                    "greensburgdailynews.com",
                    "heraldbanner.com",
                    "heraldbulletin.com",
                    "hgazette.com",
                    "homemagonline.com",
                    "itemonline.com",
                    "jacksonvilleprogress.com",
                    "jerusalemonline.com",
                    "joplinglobe.com",
                    "journal-times.com",
                    "journalexpress.net",
                    "kexp.org",
                    "kokomotribune.com",
                    "lockportjournal.com",
                    "mankatofreepress.com",
                    "mcalesternews.com",
                    "mccrearyrecord.com",
                    "mcleansborotimesleader.com",
                    "meadvilletribune.com",
                    "meridianstar.com",
                    "mineralwellsindex.com",
                    "montgomery-herald.com",
                    "mooreamerican.com",
                    "moultrieobserver.com",
                    "muskogeephoenix.com",
                    "ncnewsonline.com",
                    "newburyportnews.com",
                    "newsaegis.com",
                    "newsandtribune.com",
                    "niagara-gazette.com",
                    "njeffersonnews.com",
                    "normantranscript.com",
                    "opposingviews.com",
                    "orangeleader.com",
                    "oskaloosa.com",
                    "ottumwacourier.com",
                    "outlookmoney.com",
                    "palestineherald.com",
                    "panews.com",
                    "paulsvalleydailydemocrat.com",
                    "pellachronicle.com",
                    "pharostribune.com",
                    "pressrepublican.com",
                    "pryordailytimes.com",
                    "randolphguide.com",
                    "record-eagle.com",
                    "register-herald.com",
                    "register-news.com",
                    "reporter.net",
                    "rockwallheraldbanner.com",
                    "roysecityheraldbanner.com",
                    "rushvillerepublican.com",
                    "salemnews.com",
                    "sentinel-echo.com",
                    "sharonherald.com",
                    "shelbyvilledailyunion.com",
                    "siteslike.com",
                    "standardmedia.co.ke",
                    "starbeacon.com",
                    "stwnewspress.com",
                    "suwanneedemocrat.com",
                    "tahlequahdailypress.com",
                    "theadanews.com",
                    "theawesomer.com",
                    "thedailystar.com",
                    "thelandonline.com",
                    "themoreheadnews.com",
                    "thesnaponline.com",
                    "tiftongazette.com",
                    "times-news.com",
                    "timesenterprise.com",
                    "timessentinel.com",
                    "timeswv.com",
                    "tonawanda-news.com",
                    "tribdem.com",
                    "tribstar.com",
                    "unionrecorder.com",
                    "valdostadailytimes.com",
                    "washtimesherald.com",
                    "waurikademocrat.com",
                    "wcoutlook.com",
                    "weatherforddemocrat.com",
                    "woodwardnews.net",
                    "wrestlinginc.com",
                ]),
                ..Default::default()
            },
        );
    }

    #[test]
    fn href() {
        check_parse_result(
            r#"##a[href$="/vghd.shtml"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"a[href$="/vghd.shtml"]"#.to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"##a[href*=".adk2x.com/"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"a[href*=".adk2x.com/"]"#.to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"##a[href^="//40ceexln7929.com/"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"a[href^="//40ceexln7929.com/"]"#.to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"##a[href*=".trust.zone"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"a[href*=".trust.zone"]"#.to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"tf2maps.net##a[href="http://forums.tf2maps.net/payments.php"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"a[href="http://forums.tf2maps.net/payments.php"]"#.to_string()),
                hostnames: sort_hash_domains(vec!["tf2maps.net"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"rarbg.to,rarbg.unblockall.org,rarbgaccess.org,rarbgmirror.com,rarbgmirror.org,rarbgmirror.xyz,rarbgproxy.com,rarbgproxy.org,rarbgunblock.com##a[href][target="_blank"] > button"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"a[href][target="_blank"] > button"#.to_string()),
                hostnames: sort_hash_domains(vec![
                    "rarbg.to",
                    "rarbg.unblockall.org",
                    "rarbgaccess.org",
                    "rarbgmirror.com",
                    "rarbgmirror.org",
                    "rarbgmirror.xyz",
                    "rarbgproxy.com",
                    "rarbgproxy.org",
                    "rarbgunblock.com",
                ]),
                ..Default::default()
            },
        );
    }

    #[test]
    fn injected_scripts() {
        check_parse_result(
            r#"hentaifr.net,jeu.info,tuxboard.com,xstory-fr.com##+js(goyavelab-defuser.js)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"goyavelab-defuser.js"#.to_string()),
                hostnames: sort_hash_domains(vec![
                    "hentaifr.net",
                    "jeu.info",
                    "tuxboard.com",
                    "xstory-fr.com",
                ]),
                script_inject: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"haus-garten-test.de,sozialversicherung-kompetent.de##+js(set-constant.js, Object.keys, trueFunc)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"set-constant.js, Object.keys, trueFunc"#.to_string()),
                hostnames: sort_hash_domains(vec![
                    "haus-garten-test.de",
                    "sozialversicherung-kompetent.de",
                ]),
                script_inject: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"airliners.de,auszeit.bio,autorevue.at,clever-tanken.de,fanfiktion.de,finya.de,frag-mutti.de,frustfrei-lernen.de,fussballdaten.de,gameswelt.*,liga3-online.de,lz.de,mt.de,psychic.de,rimondo.com,spielen.de,weltfussball.at,weristdeinfreund.de##+js(abort-current-inline-script.js, Number.isNaN)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"abort-current-inline-script.js, Number.isNaN"#.to_string()),
                hostnames: sort_hash_domains(vec![
                    "airliners.de",
                    "auszeit.bio",
                    "autorevue.at",
                    "clever-tanken.de",
                    "fanfiktion.de",
                    "finya.de",
                    "frag-mutti.de",
                    "frustfrei-lernen.de",
                    "fussballdaten.de",
                    "liga3-online.de",
                    "lz.de",
                    "mt.de",
                    "psychic.de",
                    "rimondo.com",
                    "spielen.de",
                    "weltfussball.at",
                    "weristdeinfreund.de",
                ]),
                entities: sort_hash_domains(vec!["gameswelt"]),
                script_inject: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"prad.de##+js(abort-on-property-read.js, document.cookie)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"abort-on-property-read.js, document.cookie"#.to_string()),
                hostnames: sort_hash_domains(vec!["prad.de"]),
                script_inject: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"computerbild.de##+js(abort-on-property-read.js, Date.prototype.toUTCString)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"abort-on-property-read.js, Date.prototype.toUTCString"#.to_string()),
                hostnames: sort_hash_domains(vec!["computerbild.de"]),
                script_inject: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"computerbild.de##+js(setTimeout-defuser.js, ())return)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"setTimeout-defuser.js, ())return"#.to_string()),
                hostnames: sort_hash_domains(vec!["computerbild.de"]),
                script_inject: true,
                ..Default::default()
            },
        );
    }

    #[test]
    fn entities() {
        check_parse_result(
            r#"monova.*##+js(nowebrtc.js)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"nowebrtc.js"#.to_string()),
                entities: sort_hash_domains(vec!["monova"]),
                script_inject: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"monova.*##tr.success.desktop"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"tr.success.desktop"#.to_string()),
                entities: sort_hash_domains(vec!["monova"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"monova.*#@#script + [class] > [class]:first-child"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"script + [class] > [class]:first-child"#.to_string()),
                entities: sort_hash_domains(vec!["monova"]),
                unhide: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"adshort.im,adsrt.*#@#[id*="ScriptRoot"]"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"[id*="ScriptRoot"]"#.to_string()),
                hostnames: sort_hash_domains(vec!["adshort.im"]),
                entities: sort_hash_domains(vec!["adsrt"]),
                unhide: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"downloadsource.*##.date:not(dt):style(display: block !important;)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#".date:not(dt)"#.to_string()),
                entities: sort_hash_domains(vec!["downloadsource"]),
                action: Some(CosmeticFilterAction::Style("display: block !important;".into())),
                ..Default::default()
            },
        );
    }

    #[test]
    fn styles() {
        check_parse_result(
            r#"chip.de##.video-wrapper > video[style]:style(display:block!important;padding-top:0!important;)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#".video-wrapper > video[style]"#.to_string()),
                hostnames: sort_hash_domains(vec!["chip.de"]),
                action: Some(CosmeticFilterAction::Style("display:block!important;padding-top:0!important;".into())),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"allmusic.com##.advertising.medium-rectangle:style(min-height: 1px !important;)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#".advertising.medium-rectangle"#.to_string()),
                hostnames: sort_hash_domains(vec!["allmusic.com"]),
                action: Some(CosmeticFilterAction::Style("min-height: 1px !important;".into())),
                ..Default::default()
            },
        );
        #[cfg(feature = "css-validation")]
        check_parse_result(
            r#"quora.com##.signup_wall_prevent_scroll .SiteHeader,.signup_wall_prevent_scroll .LoggedOutFooter,.signup_wall_prevent_scroll .ContentWrapper:style(filter: none !important;)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#".signup_wall_prevent_scroll .SiteHeader, .signup_wall_prevent_scroll .LoggedOutFooter, .signup_wall_prevent_scroll .ContentWrapper"#.to_string()),
                hostnames: sort_hash_domains(vec!["quora.com"]),
                action: Some(CosmeticFilterAction::Style("filter: none !important;".into())),
                ..Default::default()
            }
        );
        check_parse_result(
            r#"imdb.com##body#styleguide-v2:style(background-color: #e3e2dd !important; background-image: none !important;)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"body#styleguide-v2"#.to_string()),
                hostnames: sort_hash_domains(vec!["imdb.com"]),
                action: Some(CosmeticFilterAction::Style("background-color: #e3e2dd !important; background-image: none !important;".into())),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"streamcloud.eu###login > div[style^="width"]:style(display: block !important)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"#login > div[style^="width"]"#.to_string()),
                hostnames: sort_hash_domains(vec!["streamcloud.eu"]),
                action: Some(CosmeticFilterAction::Style("display: block !important".into())),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"moonbit.co.in,moondoge.co.in,moonliteco.in##[src^="//coinad.com/ads/"]:style(visibility: collapse !important)"#,
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss(r#"[src^="//coinad.com/ads/"]"#.to_string()),
                hostnames: sort_hash_domains(vec![
                    "moonbit.co.in",
                    "moondoge.co.in",
                    "moonliteco.in",
                ]),
                action: Some(CosmeticFilterAction::Style("visibility: collapse !important".into())),
                ..Default::default()
            },
        );
    }

    #[test]
    fn unicode() {
        check_parse_result(
            "###неделя",
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss("#неделя".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            "неlloworlд.com#@##week",
            CosmeticFilterBreakdown {
                selector: SelectorType::PlainCss("#week".to_string()),
                hostnames: sort_hash_domains(vec!["xn--lloworl-5ggb3f.com"]),
                unhide: true,
                ..Default::default()
            }
        );
    }

    /// As of writing, these procedural filters with multiple comma-separated selectors aren't
    /// fully supported by uBO. Here, they are treated as parsing errors.
    #[test]
    #[cfg(feature = "css-validation")]
    fn multi_selector_procedural_filters() {
        assert!(parse_cf("example.com##h1:has-text(Example Domain),p:has-text(More)").is_err());
        assert!(parse_cf("example.com##h1,p:has-text(ill)").is_err());
        assert!(parse_cf("example.com##h1:has-text(om),p").is_err());
    }

    #[test]
    #[cfg(feature = "css-validation")]
    fn procedural_operators() {
        /// Check against simple `example.com` domains. Domain parsing is well-handled by other
        /// tests, but procedural filters cannot be generic.
        fn check_procedural(raw: &str, expected_selectors: Vec<CosmeticFilterOperator>) {
            check_parse_result(
                &format!("example.com##{}", raw),
                CosmeticFilterBreakdown {
                    selector: SelectorType::Procedural(expected_selectors),
                    hostnames: sort_hash_domains(vec![
                        "example.com",
                    ]),
                    ..Default::default()
                }
            );
        }
        check_procedural(
            ".items:has-text(Sponsored)",
            vec![
                CosmeticFilterOperator::CssSelector(".items".to_string()),
                CosmeticFilterOperator::HasText("Sponsored".to_string()),
            ],
        );
        check_procedural(
            "div.items:has(p):has-text(Sponsored)",
            vec![
                CosmeticFilterOperator::CssSelector("div.items:has(p)".to_string()),
                CosmeticFilterOperator::HasText("Sponsored".to_string()),
            ],
        );
        check_procedural(
            "div.items:has-text(Sponsored):has(p)",
            vec![
                CosmeticFilterOperator::CssSelector("div.items".to_string()),
                CosmeticFilterOperator::HasText("Sponsored".to_string()),
                CosmeticFilterOperator::CssSelector(":has(p)".to_string()),
            ],
        );
        check_procedural(
            ".items:has-text(Sponsored) .container",
            vec![
                CosmeticFilterOperator::CssSelector(".items".to_string()),
                CosmeticFilterOperator::HasText("Sponsored".to_string()),
                CosmeticFilterOperator::CssSelector(" .container".to_string()),
            ],
        );
        check_procedural(
            ".items:has-text(Sponsored) > .container",
            vec![
                CosmeticFilterOperator::CssSelector(".items".to_string()),
                CosmeticFilterOperator::HasText("Sponsored".to_string()),
                CosmeticFilterOperator::CssSelector(" > .container".to_string()),
            ],
        );
        check_procedural(
            ".items:has-text(Sponsored) + .container:has-text(Ad) ~ div",
            vec![
                CosmeticFilterOperator::CssSelector(".items".to_string()),
                CosmeticFilterOperator::HasText("Sponsored".to_string()),
                CosmeticFilterOperator::CssSelector(" + .container".to_string()),
                CosmeticFilterOperator::HasText("Ad".to_string()),
                CosmeticFilterOperator::CssSelector(" ~ div".to_string()),
            ],
        );
    }

    #[test]
    #[cfg(feature = "css-validation")]
    fn unsupported() {
        assert!(parse_cf("yandex.*##.serp-item:if(:scope > div.organic div.organic__subtitle:matches-css-after(content: /[Рр]еклама/))").is_err());
        assert!(parse_cf(r#"facebook.com,facebookcorewwwi.onion##.ego_column:if(a[href^="/campaign/landing"])"#).is_err());
        assert!(parse_cf(r#"readcomiconline.to##^script:has-text(this[atob)"#).is_err());
        assert!(parse_cf("##").is_err());
        assert!(parse_cf("").is_err());

        // `:has` was previously limited to procedural filtering, but is now a native CSS feature.
        assert!(parse_cf(r#"thedailywtf.com##.article-body > div:has(a[href*="utm_medium"])"#).is_ok());

        // `:has-text` and `:xpath` are now supported procedural filters
        assert!(parse_cf("twitter.com##article:has-text(/Promoted|Gesponsert|Реклама|Promocionado/):xpath(../..)").is_ok());

        // generic procedural filters are not supported
        assert!(parse_cf("##.t-rec > .t886:has-text(cookies)").is_err());
    }

    #[test]
    fn hidden_generic() {
        let rule = parse_cf("##.selector").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.com##.selector").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.*##.selector").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.com,~a.test.com##.selector").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.*,~a.test.com##.selector").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.*,~a.test.*##.selector").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.com#@#.selector").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("~test.com##.selector").unwrap();
        assert_eq!(
            CosmeticFilterBreakdown::from(rule.hidden_generic_rule().unwrap()),
            parse_cf("##.selector").unwrap().into(),
        );

        let rule = parse_cf("~test.*##.selector").unwrap();
        assert_eq!(
            CosmeticFilterBreakdown::from(rule.hidden_generic_rule().unwrap()),
            parse_cf("##.selector").unwrap().into(),
        );

        let rule = parse_cf("~test.*,~a.test.*##.selector").unwrap();
        assert_eq!(
            CosmeticFilterBreakdown::from(rule.hidden_generic_rule().unwrap()),
            parse_cf("##.selector").unwrap().into(),
        );

        let rule = parse_cf("test.com##.selector:style(border-radius: 13px)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.*##.selector:style(border-radius: 13px)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("~test.com##.selector:style(border-radius: 13px)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("~test.*##.selector:style(border-radius: 13px)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.com#@#.selector:style(border-radius: 13px)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.com##+js(nowebrtc.js)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.*##+js(nowebrtc.js)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("~test.com##+js(nowebrtc.js)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("~test.*##+js(nowebrtc.js)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());

        let rule = parse_cf("test.com#@#+js(nowebrtc.js)").unwrap();
        assert!(rule.hidden_generic_rule().is_none());
    }
}

#[cfg(test)]
mod util_tests {
    use super::*;
    use crate::utils::fast_hash;

    #[test]
    fn label_hashing() {
        assert_eq!(get_hashes_from_labels("foo.bar.baz", 11, 11), vec![fast_hash("baz"), fast_hash("bar.baz"), fast_hash("foo.bar.baz")]);
        assert_eq!(get_hashes_from_labels("foo.bar.baz.com", 15, 8), vec![fast_hash("baz.com"), fast_hash("bar.baz.com"), fast_hash("foo.bar.baz.com")]);
        assert_eq!(get_hashes_from_labels("foo.bar.baz.com", 11, 11), vec![fast_hash("baz"), fast_hash("bar.baz"), fast_hash("foo.bar.baz")]);
        assert_eq!(get_hashes_from_labels("foo.bar.baz.com", 11, 8), vec![fast_hash("baz"), fast_hash("bar.baz"), fast_hash("foo.bar.baz")]);
    }

    #[test]
    fn without_public_suffix() {
        assert_eq!(get_hostname_without_public_suffix("", ""), None);
        assert_eq!(get_hostname_without_public_suffix("com", ""), None);
        assert_eq!(get_hostname_without_public_suffix("com", "com"), None);
        assert_eq!(get_hostname_without_public_suffix("foo.com", "foo.com"), Some(("foo", "com")));
        assert_eq!(get_hostname_without_public_suffix("foo.bar.com", "bar.com"), Some(("foo.bar", "com")));
        assert_eq!(get_hostname_without_public_suffix("test.github.io", "test.github.io"), Some(("test", "github.io")));
    }
}

#[cfg(test)]
mod matching_tests {
    use super::*;
    use crate::utils::bin_lookup;

    trait MatchByStr {
        fn matches(&self, request_entities: &[Hash], request_hostnames: &[Hash]) -> bool;
        fn matches_str(&self, hostname: &str, domain: &str) -> bool;
    }

    impl MatchByStr for CosmeticFilter {
        /// `hostname` and `domain` should be specified as, e.g. "subdomain.domain.com" and
        /// "domain.com", respectively. This function will panic if the specified `domain` is
        /// longer than the specified `hostname`.
        fn matches_str(&self, hostname: &str, domain: &str) -> bool {
            debug_assert!(hostname.len() >= domain.len());

            let request_entities = get_entity_hashes_from_labels(hostname, domain);

            let request_hostnames = get_hostname_hashes_from_labels(hostname, domain);

            self.matches(&request_entities[..], &request_hostnames[..])
        }

        /// Check whether this rule applies to content from the hostname and domain corresponding to
        /// the provided hash lists.
        ///
        /// See the `matches_str` test function for an example of how to convert hostnames and
        /// domains into the appropriate hash lists.
        fn matches(&self, request_entities: &[Hash], request_hostnames: &[Hash]) -> bool {
            let has_hostname_constraint = self.has_hostname_constraint();
            if !has_hostname_constraint {
                return true;
            }
            if request_entities.is_empty()
                && request_hostnames.is_empty()
                && has_hostname_constraint
            {
                return false;
            }

            if let Some(ref filter_not_hostnames) = self.not_hostnames {
                if request_hostnames
                    .iter()
                    .any(|hash| bin_lookup(filter_not_hostnames, *hash))
                {
                    return false;
                }
            }

            if let Some(ref filter_not_entities) = self.not_entities {
                if request_entities
                    .iter()
                    .any(|hash| bin_lookup(filter_not_entities, *hash))
                {
                    return false;
                }
            }

            if self.hostnames.is_some() || self.entities.is_some() {
                if let Some(ref filter_hostnames) = self.hostnames {
                    if request_hostnames
                        .iter()
                        .any(|hash| bin_lookup(filter_hostnames, *hash))
                    {
                        return true;
                    }
                }

                if let Some(ref filter_entities) = self.entities {
                    if request_entities
                        .iter()
                        .any(|hash| bin_lookup(filter_entities, *hash))
                    {
                        return true;
                    }
                }

                return false;
            }

            true
        }
    }

    fn parse_cf(rule: &str) -> Result<CosmeticFilter, CosmeticFilterError> {
        CosmeticFilter::parse(rule, false, Default::default())
    }

    #[test]
    fn generic_filter() {
        let rule = parse_cf("##.selector").unwrap();
        assert!(rule.matches_str("foo.com", "foo.com"));
    }

    #[test]
    fn single_domain() {
        let rule = parse_cf("foo.com##.selector").unwrap();
        assert!(rule.matches_str("foo.com", "foo.com"));
        assert!(!rule.matches_str("bar.com", "bar.com"));
    }

    #[test]
    fn multiple_domains() {
        let rule = parse_cf("foo.com,test.com##.selector").unwrap();
        assert!(rule.matches_str("foo.com", "foo.com"));
        assert!(rule.matches_str("test.com", "test.com"));
        assert!(!rule.matches_str("bar.com", "bar.com"));
    }

    #[test]
    fn subdomain() {
        let rule = parse_cf("foo.com,test.com##.selector").unwrap();
        assert!(rule.matches_str("sub.foo.com", "foo.com"));
        assert!(rule.matches_str("sub.test.com", "test.com"));

        let rule = parse_cf("foo.com,sub.test.com##.selector").unwrap();
        assert!(rule.matches_str("sub.test.com", "test.com"));
        assert!(!rule.matches_str("test.com", "test.com"));
        assert!(!rule.matches_str("com", "com"));
    }

    #[test]
    fn entity() {
        let rule = parse_cf("foo.com,sub.test.*##.selector").unwrap();
        assert!(rule.matches_str("foo.com", "foo.com"));
        assert!(rule.matches_str("bar.foo.com", "foo.com"));
        assert!(rule.matches_str("sub.test.com", "test.com"));
        assert!(rule.matches_str("sub.test.fr", "test.fr"));
        assert!(!rule.matches_str("sub.test.evil.biz", "evil.biz"));

        let rule = parse_cf("foo.*##.selector").unwrap();
        assert!(rule.matches_str("foo.co.uk", "foo.co.uk"));
        assert!(rule.matches_str("bar.foo.co.uk", "foo.co.uk"));
        assert!(rule.matches_str("baz.bar.foo.co.uk", "foo.co.uk"));
        assert!(!rule.matches_str("foo.evil.biz", "evil.biz"));
    }

    #[test]
    fn nonmatching() {
        let rule = parse_cf("foo.*##.selector").unwrap();
        assert!(!rule.matches_str("foo.bar.com", "bar.com"));
        assert!(!rule.matches_str("bar-foo.com", "bar-foo.com"));
    }

    #[test]
    fn entity_negations() {
        let rule = parse_cf("~foo.*##.selector").unwrap();
        assert!(!rule.matches_str("foo.com", "foo.com"));
        assert!(rule.matches_str("foo.evil.biz", "evil.biz"));

        let rule = parse_cf("~foo.*,~bar.*##.selector").unwrap();
        assert!(rule.matches_str("baz.com", "baz.com"));
        assert!(!rule.matches_str("foo.com", "foo.com"));
        assert!(!rule.matches_str("sub.foo.com", "foo.com"));
        assert!(!rule.matches_str("bar.com", "bar.com"));
        assert!(!rule.matches_str("sub.bar.com", "bar.com"));
    }

    #[test]
    fn hostname_negations() {
        let rule = parse_cf("~foo.com##.selector").unwrap();
        assert!(!rule.matches_str("foo.com", "foo.com"));
        assert!(!rule.matches_str("bar.foo.com", "foo.com"));
        assert!(rule.matches_str("foo.com.bar", "com.bar"));
        assert!(rule.matches_str("foo.co.uk", "foo.co.uk"));

        let rule = parse_cf("~foo.com,~foo.de,~bar.com##.selector").unwrap();
        assert!(!rule.matches_str("foo.com", "foo.com"));
        assert!(!rule.matches_str("sub.foo.com", "foo.com"));
        assert!(!rule.matches_str("foo.de", "foo.de"));
        assert!(!rule.matches_str("sub.foo.de", "foo.de"));
        assert!(!rule.matches_str("bar.com", "bar.com"));
        assert!(!rule.matches_str("sub.bar.com", "bar.com"));
        assert!(rule.matches_str("bar.de", "bar.de"));
        assert!(rule.matches_str("sub.bar.de", "bar.de"));
    }

    #[test]
    fn entity_with_suffix_exception() {
        let rule = parse_cf("foo.*,~foo.com##.selector").unwrap();
        assert!(!rule.matches_str("foo.com", "foo.com"));
        assert!(!rule.matches_str("sub.foo.com", "foo.com"));
        assert!(rule.matches_str("foo.de", "foo.de"));
        assert!(rule.matches_str("sub.foo.de", "foo.de"));
    }

    #[test]
    fn entity_with_subdomain_exception() {
        let rule = parse_cf("foo.*,~sub.foo.*##.selector").unwrap();
        assert!(rule.matches_str("foo.com", "foo.com"));
        assert!(rule.matches_str("foo.de", "foo.de"));
        assert!(!rule.matches_str("sub.foo.com", "foo.com"));
        assert!(!rule.matches_str("bar.com", "bar.com"));
        assert!(rule.matches_str("sub2.foo.com", "foo.com"));
    }

    #[test]
    fn no_domain_provided() {
        let rule = parse_cf("foo.*##.selector").unwrap();
        assert!(!rule.matches_str("foo.com", ""));
    }

    #[test]
    fn no_hostname_provided() {
        let rule = parse_cf("domain.com##.selector").unwrap();
        assert!(!rule.matches_str("", ""));
        let rule = parse_cf("domain.*##.selector").unwrap();
        assert!(!rule.matches_str("", ""));
        let rule = parse_cf("~domain.*##.selector").unwrap();
        assert!(!rule.matches_str("", ""));
        let rule = parse_cf("~domain.com##.selector").unwrap();
        assert!(!rule.matches_str("", ""));
    }

    #[test]
    fn respects_etld() {
        let rule = parse_cf("github.io##.selector").unwrap();
        assert!(rule.matches_str("test.github.io", "github.io"));
    }

    #[test]
    fn multiple_selectors() {
        assert!(parse_cf("youtube.com##.masthead-ad-control,.ad-div,.pyv-afc-ads-container").is_ok());
        assert!(parse_cf("m.economictimes.com###appBanner,#stickyBanner").is_ok());
        assert!(parse_cf("googledrivelinks.com###wpsafe-generate, #wpsafe-link:style(display: block !important;)").is_ok());
    }

    #[test]
    fn actions() {
        assert!(parse_cf("example.com###adBanner:style(background: transparent)").is_ok());
        assert!(parse_cf("example.com###adBanner:remove()").is_ok());
        assert!(parse_cf("example.com###adBanner:remove-attr(style)").is_ok());
        assert!(parse_cf("example.com###adBanner:remove-class(src)").is_ok());
    }

    #[test]
    fn zero_width_space() {
        assert!(parse_cf(r#"​##a[href^="https://www.g2fame.com/"] > img"#).is_err());
    }

    #[test]
    fn adg_regex() {
        assert!(parse_cf(r"/^dizipal\d+\.com$/##.web").is_err());
        // Filter is still salvageable if at least one location is supported
        assert!(parse_cf(r"/^dizipal\d+\.com,test.net$/##.web").is_ok());
    }

    #[test]
    #[cfg(feature = "css-validation")]
    fn abp_has_conversion() {
        let rule = parse_cf("imgur.com#?#div.Gallery-Sidebar-PostContainer:-abp-has(div.promoted-hover)").unwrap();
        assert_eq!(rule.plain_css_selector(), Some("div.Gallery-Sidebar-PostContainer:has(div.promoted-hover)"));
        let rule = parse_cf(r##"webtools.fineaty.com#?#div[class*=" hidden-"]:-abp-has(.adsbygoogle)"##).unwrap();
        assert_eq!(rule.plain_css_selector(), Some(r#"div[class*=" hidden-"]:has(.adsbygoogle)"#));
        let rule = parse_cf(r##"facebook.com,facebookcorewwwi.onion#?#._6y8t:-abp-has(a[href="/ads/about/?entry_product=ad_preferences"])"##).unwrap();
        assert_eq!(rule.plain_css_selector(), Some(r#"._6y8t:has(a[href="/ads/about/?entry_product=ad_preferences"])"#));
        let rule = parse_cf(r##"mtgarena.pro#?##root > div > div:-abp-has(> .vm-placement)"##).unwrap();
        assert_eq!(rule.plain_css_selector(), Some(r#"#root > div > div:has(> .vm-placement)"#));
        // Error without `#?#`:
        assert!(parse_cf(r##"mtgarena.pro###root > div > div:-abp-has(> .vm-placement)"##).is_err());
    }
}
