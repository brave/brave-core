//! Filters that take effect at a page-content level, including CSS selector-based filtering and
//! content script injection.

use memchr::{memchr as find_char, memmem, memrchr as find_char_reverse};
use once_cell::sync::Lazy;
use regex::Regex;
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
    #[error("generic action")]
    GenericAction,
    #[error("double negation")]
    DoubleNegation,
    #[error("empty rule")]
    EmptyRule,
    #[error("html filtering is unsupported")]
    HtmlFilteringUnsupported,
}

/// Refer to <https://github.com/uBlockOrigin/uBlock-issues/wiki/Static-filter-syntax#action-operators>
#[derive(PartialEq, Debug, Clone, Serialize, Deserialize)]
pub enum CosmeticFilterAction {
    Remove,
    /// Argument is one or more CSS property declarations, separated by the standard ;. Some
    /// characters, strings, and values are forbidden.
    Style(String),
    RemoveAttr(String),
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
        const IS_UNICODE = 1 << 2;
        const IS_CLASS_SELECTOR = 1 << 3;
        const IS_ID_SELECTOR = 1 << 4;
        const IS_SIMPLE = 1 << 5;

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
    pub selector: String,
    pub key: Option<String>,
    pub action: Option<CosmeticFilterAction>,
    pub permission: PermissionMask,
}

pub enum CosmeticFilterLocationType {
    Entity,
    NotEntity,
    Hostname,
    NotHostname,
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
    pub fn locations_before_sharp(
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
        mask: &mut CosmeticFilterMask,
    ) -> Result<CosmeticFilterLocations, CosmeticFilterError> {
        let mut entities_vec = vec![];
        let mut not_entities_vec = vec![];
        let mut hostnames_vec = vec![];
        let mut not_hostnames_vec = vec![];

        for (location_type, location) in Self::locations_before_sharp(line, sharp_index) {
            let mut hostname = String::new();
            if location.is_ascii() {
                hostname.push_str(location);
            } else {
                *mask |= CosmeticFilterMask::IS_UNICODE;
                match idna::domain_to_ascii(location) {
                    Ok(x) => hostname.push_str(&x),
                    Err(_) => return Err(CosmeticFilterError::PunycodeError),
                }
            }
            let hash = crate::utils::fast_hash(&hostname);
            match location_type {
                CosmeticFilterLocationType::NotEntity => not_entities_vec.push(hash),
                CosmeticFilterLocationType::NotHostname => not_hostnames_vec.push(hash),
                CosmeticFilterLocationType::Entity => entities_vec.push(hash),
                CosmeticFilterLocationType::Hostname => hostnames_vec.push(hash),
            }
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
                CosmeticFilter::parse_before_sharp(line, sharp_index, &mut mask)?
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
                mask |= CosmeticFilterMask::SCRIPT_INJECT;
                (
                    String::from(&line[suffix_start_index + 4..line.len() - 1]),
                    None,
                )
            } else {
                let (selector, action) = CosmeticFilter::parse_after_sharp_nonscript(after_sharp)?;
                let validated_selector = match validate_css_selector(selector, translate_abp_syntax)
                {
                    Some(s) => s,
                    None => return Err(CosmeticFilterError::InvalidCssSelector),
                };
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

            if !selector.is_ascii() {
                mask |= CosmeticFilterMask::IS_UNICODE;
            }

            let key = if !mask.contains(CosmeticFilterMask::SCRIPT_INJECT) {
                if selector.starts_with('.') {
                    let key = key_from_selector(&selector)?;
                    mask |= CosmeticFilterMask::IS_CLASS_SELECTOR;
                    if key == selector {
                        mask |= CosmeticFilterMask::IS_SIMPLE;
                    }
                    Some(String::from(&key[1..]))
                } else if selector.starts_with('#') {
                    let key = key_from_selector(&selector)?;
                    mask |= CosmeticFilterMask::IS_ID_SELECTOR;
                    if key == selector {
                        mask |= CosmeticFilterMask::IS_SIMPLE;
                    }
                    Some(String::from(&key[1..]))
                } else {
                    None
                }
            } else {
                None
            };

            Ok(CosmeticFilter {
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
                key,
                action,
                permission,
            })
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
pub fn get_entity_hashes_from_labels(hostname: &str, domain: &str) -> Vec<Hash> {
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
pub fn get_hostname_hashes_from_labels(hostname: &str, domain: &str) -> Vec<Hash> {
    get_hashes_from_labels(hostname, hostname.len(), hostname.len() - domain.len())
}

#[cfg(not(feature = "css-validation"))]
mod css_validation {
    pub fn validate_css_selector(selector: &str, _accept_abp_selectors: bool) -> Option<String> {
        Some(selector.to_string())
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

    /// Returns a validated canonical CSS selector for the given input, or nothing if one can't be
    /// determined.
    ///
    /// For the majority of filters, this works by trivial regex matching. More complex filters are
    /// assembled into a mock stylesheet which is then parsed using `cssparser` and validated.
    ///
    /// In addition to normalizing formatting, this function will remove unsupported procedural
    /// selectors and convert others to canonical representations (i.e. `:-abp-has` -> `:has`).
    pub fn validate_css_selector(selector: &str, accept_abp_selectors: bool) -> Option<String> {
        use once_cell::sync::Lazy;
        use regex::Regex;
        static RE_SIMPLE_SELECTOR: Lazy<Regex> =
            Lazy::new(|| Regex::new(r"^[#.]?[A-Za-z_][\w-]*$").unwrap());

        if RE_SIMPLE_SELECTOR.is_match(selector) {
            return Some(selector.to_string());
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
            .and_then(|r| r.ok())
            .map(|prelude| prelude.to_css_string());

        if rule_list_parser.next().is_some() {
            return None;
        }

        prelude
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
                _ => None,
            }
            .unwrap_or(name.as_ref());
            match canonical_name {
                "-abp-contains" | "-abp-has" | "-abp-properties" | "has-text" | "if" | "if-not"
                | "matches-attr" | "matches-css" | "matches-css-after" | "matches-css-before"
                | "matches-media" | "matches-path" | "min-text-length" | "nth-ancestor"
                | "properties" | "subject" | "upward" | "remove" | "remove-attr"
                | "remove-class" | "watch-attr" | "xpath" => {
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
        type LocalName = DummyValue;
        type NamespaceUrl = DummyValue;
        type NamespacePrefix = DummyValue;
        type BorrowedNamespaceUrl = DummyValue;
        type BorrowedLocalName = DummyValue;
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
                Self::AnythingElse(name, None) => write!(dest, "{}", name)?,
                Self::AnythingElse(name, Some(args)) => write!(dest, "{}({})", name, args)?,
            }
            Ok(())
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
        assert!(validate_css_selector(r#"rm -rf ./*"#, false).is_none());
        assert!(validate_css_selector(r#"javascript:alert("All pseudo-classes are valid")"#, false).is_some());
        assert!(validate_css_selector(r#"javascript:alert("But opening comments are still forbidden" /*)"#, false).is_none());
        assert!(validate_css_selector(r#"This is not a CSS selector."#, false).is_none());
        assert!(validate_css_selector(r#"./malware.sh"#, false).is_none());
        assert!(validate_css_selector(r#"https://safesite.ru"#, false).is_none());
        assert!(validate_css_selector(r#"(function(){var e=60;return String.fromCharCode(e.charCodeAt(0))})();"#, false).is_none());
        assert!(validate_css_selector(r#"#!/usr/bin/sh"#, false).is_none());
        assert!(validate_css_selector(r#"input,input/*"#, false).is_none());
        // Accept a closing comment within a string. It should still be impossible to create an
        // opening comment to match it.
        assert!(validate_css_selector(r#"input[x="*/{}*{background:url(https://hackvertor.co.uk/images/logo.gif)}"]"#, false).is_some());
    }
}

static RE_PLAIN_SELECTOR: Lazy<Regex> = Lazy::new(|| Regex::new(r"^[#.][\w\\-]+").unwrap());
static RE_PLAIN_SELECTOR_ESCAPED: Lazy<Regex> =
    Lazy::new(|| Regex::new(r"^[#.](?:\\[0-9A-Fa-f]+ |\\.|\w|-)+").unwrap());
static RE_ESCAPE_SEQUENCE: Lazy<Regex> = Lazy::new(|| Regex::new(r"\\([0-9A-Fa-f]+ |.)").unwrap());

/// Returns the first token of a CSS selector.
///
/// This should only be called once `selector` has been verified to start with either a "#" or "."
/// character.
fn key_from_selector(selector: &str) -> Result<String, CosmeticFilterError> {
    // If there are no escape characters in the selector, just take the first class or id token.
    let mat = RE_PLAIN_SELECTOR.find(selector);
    if let Some(location) = mat {
        let key = &location.as_str();
        if find_char(b'\\', key.as_bytes()).is_none() {
            return Ok((*key).into());
        }
    } else {
        return Err(CosmeticFilterError::InvalidCssSelector);
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
            if capture.chars().count() == 1 {   // Check number of unicode characters rather than byte length
                key += capture;
            } else {
                // This u32 conversion can overflow
                let codepoint = u32::from_str_radix(&capture[..capture.len() - 1], 16)
                    .map_err(|_| CosmeticFilterError::InvalidCssSelector)?;

                // Not all u32s are valid Unicode codepoints
                key += &core::char::from_u32(codepoint)
                    .ok_or(CosmeticFilterError::InvalidCssSelector)?
                    .to_string();
            }
        }
        Ok(key + &escaped[beginning..])
    } else {
        Err(CosmeticFilterError::InvalidCssSelector)
    }
}

#[cfg(test)]
mod key_from_selector_tests {
    use super::key_from_selector;

    #[test]
    fn no_escapes() {
        assert_eq!(key_from_selector(r#"#selector"#).unwrap(), "#selector");
        assert_eq!(key_from_selector(r#"#ad-box[href="https://popads.net"]"#).unwrap(), "#ad-box");
        assert_eq!(key_from_selector(r#".p"#).unwrap(), ".p");
        assert_eq!(key_from_selector(r#".ad #ad.adblockblock"#).unwrap(), ".ad");
        assert_eq!(key_from_selector(r#"#container.contained"#).unwrap(), "#container");
    }

    #[test]
    fn escaped_characters() {
        assert_eq!(key_from_selector(r"#Meebo\:AdElement\.Root").unwrap(), "#Meebo:AdElement.Root");
        assert_eq!(key_from_selector(r"#\ Banner\ Ad\ -\ 590\ x\ 90").unwrap(), "# Banner Ad - 590 x 90");
        assert_eq!(key_from_selector(r"#\ rek").unwrap(), "# rek");
        assert_eq!(key_from_selector(r#"#\:rr .nH[role="main"] .mq:first-child"#).unwrap(), "#:rr");
        assert_eq!(key_from_selector(r#"#adspot-300x600\,300x250-pos-1"#).unwrap(), "#adspot-300x600,300x250-pos-1");
        assert_eq!(key_from_selector(r#"#adv_\'146\'"#).unwrap(), "#adv_\'146\'");
        assert_eq!(key_from_selector(r#"#oas-mpu-left\<\/div\>"#).unwrap(), "#oas-mpu-left</div>");
        assert_eq!(key_from_selector(r#".Trsp\(op\).Trsdu\(3s\)"#).unwrap(), ".Trsp(op)");
    }

    #[test]
    fn escape_codes() {
        assert_eq!(key_from_selector(r#"#\5f _mom_ad_12"#).unwrap(), "#__mom_ad_12");
        assert_eq!(key_from_selector(r#"#\5f _nq__hh[style="display:block!important"]"#).unwrap(), "#__nq__hh");
        assert_eq!(key_from_selector(r#"#\31 000-014-ros"#).unwrap(), "#1000-014-ros");
        assert_eq!(key_from_selector(r#"#\33 00X250ad"#).unwrap(), "#300X250ad");
        assert_eq!(key_from_selector(r#"#\5f _fixme"#).unwrap(), "#__fixme");
        assert_eq!(key_from_selector(r#"#\37 28ad"#).unwrap(), "#728ad");
    }

    #[test]
    fn bad_escapes() {
        assert!(key_from_selector(r#"#\5ffffffffff overflows"#).is_err());
        assert!(key_from_selector(r#"#\5fffffff is_too_large"#).is_err());
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
        selector: String,
        key: Option<String>,
        action: Option<CosmeticFilterAction>,

        unhide: bool,
        script_inject: bool,
        is_unicode: bool,
        is_class_selector: bool,
        is_id_selector: bool,
    }

    impl From<&CosmeticFilter> for CosmeticFilterBreakdown {
        fn from(filter: &CosmeticFilter) -> CosmeticFilterBreakdown {
            CosmeticFilterBreakdown {
                entities: filter.entities.as_ref().cloned(),
                hostnames: filter.hostnames.as_ref().cloned(),
                not_entities: filter.not_entities.as_ref().cloned(),
                not_hostnames: filter.not_hostnames.as_ref().cloned(),
                selector: filter.selector.clone(),
                key: filter.key.as_ref().cloned(),
                action: filter.action.as_ref().cloned(),

                unhide: filter.mask.contains(CosmeticFilterMask::UNHIDE),
                script_inject: filter.mask.contains(CosmeticFilterMask::SCRIPT_INJECT),
                is_unicode: filter.mask.contains(CosmeticFilterMask::IS_UNICODE),
                is_class_selector: filter.mask.contains(CosmeticFilterMask::IS_CLASS_SELECTOR),
                is_id_selector: filter.mask.contains(CosmeticFilterMask::IS_ID_SELECTOR),
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
                selector: "".to_string(),
                key: None,
                action: None,

                unhide: false,
                script_inject: false,
                is_unicode: false,
                is_class_selector: false,
                is_id_selector: false,
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
                selector: "div.popup".to_string(),
                ..Default::default()
            },
        );
        check_parse_result(
            "###selector",
            CosmeticFilterBreakdown {
                selector: "#selector".to_string(),
                is_id_selector: true,
                key: Some("selector".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            "##.selector",
            CosmeticFilterBreakdown {
                selector: ".selector".to_string(),
                is_class_selector: true,
                key: Some("selector".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            "##a[href=\"foo.com\"]",
            CosmeticFilterBreakdown {
                selector: "a[href=\"foo.com\"]".to_string(),
                ..Default::default()
            },
        );
        check_parse_result(
            "##[href=\"foo.com\"]",
            CosmeticFilterBreakdown {
                selector: "[href=\"foo.com\"]".to_string(),
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
                selector: r#"div[class^="adv-box"]"#.to_string(),
                hostnames: sort_hash_domains(vec!["u00p.com"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"distractify.com##div[class*="AdInArticle"]"#,
            CosmeticFilterBreakdown {
                selector: r#"div[class*="AdInArticle"]"#.to_string(),
                hostnames: sort_hash_domains(vec!["distractify.com"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"soundtrackcollector.com,the-numbers.com##a[href^="http://affiliates.allposters.com/"]"#,
            CosmeticFilterBreakdown {
                selector: r#"a[href^="http://affiliates.allposters.com/"]"#.to_string(),
                hostnames: sort_hash_domains(vec!["soundtrackcollector.com", "the-numbers.com"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"thelocal.at,thelocal.ch,thelocal.de,thelocal.dk,thelocal.es,thelocal.fr,thelocal.it,thelocal.no,thelocal.se##div[class*="-widget"]"#,
            CosmeticFilterBreakdown {
                selector: r#"div[class*="-widget"]"#.to_string(),
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
                selector: r#"div[class^="banner_"]"#.to_string(),
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
                selector: r#"div[style="width:300px; height:250px;"]"#.to_string(),
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
                selector: r#"a[href$="/vghd.shtml"]"#.to_string(),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"##a[href*=".adk2x.com/"]"#,
            CosmeticFilterBreakdown {
                selector: r#"a[href*=".adk2x.com/"]"#.to_string(),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"##a[href^="//40ceexln7929.com/"]"#,
            CosmeticFilterBreakdown {
                selector: r#"a[href^="//40ceexln7929.com/"]"#.to_string(),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"##a[href*=".trust.zone"]"#,
            CosmeticFilterBreakdown {
                selector: r#"a[href*=".trust.zone"]"#.to_string(),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"tf2maps.net##a[href="http://forums.tf2maps.net/payments.php"]"#,
            CosmeticFilterBreakdown {
                selector: r#"a[href="http://forums.tf2maps.net/payments.php"]"#.to_string(),
                hostnames: sort_hash_domains(vec!["tf2maps.net"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"rarbg.to,rarbg.unblockall.org,rarbgaccess.org,rarbgmirror.com,rarbgmirror.org,rarbgmirror.xyz,rarbgproxy.com,rarbgproxy.org,rarbgunblock.com##a[href][target="_blank"] > button"#,
            CosmeticFilterBreakdown {
                selector: r#"a[href][target="_blank"] > button"#.to_string(),
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
                selector: r#"goyavelab-defuser.js"#.to_string(),
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
                selector: r#"set-constant.js, Object.keys, trueFunc"#.to_string(),
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
                selector: r#"abort-current-inline-script.js, Number.isNaN"#.to_string(),
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
                selector: r#"abort-on-property-read.js, document.cookie"#.to_string(),
                hostnames: sort_hash_domains(vec!["prad.de"]),
                script_inject: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"computerbild.de##+js(abort-on-property-read.js, Date.prototype.toUTCString)"#,
            CosmeticFilterBreakdown {
                selector: r#"abort-on-property-read.js, Date.prototype.toUTCString"#.to_string(),
                hostnames: sort_hash_domains(vec!["computerbild.de"]),
                script_inject: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"computerbild.de##+js(setTimeout-defuser.js, ())return)"#,
            CosmeticFilterBreakdown {
                selector: r#"setTimeout-defuser.js, ())return"#.to_string(),
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
                selector: r#"nowebrtc.js"#.to_string(),
                entities: sort_hash_domains(vec!["monova"]),
                script_inject: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"monova.*##tr.success.desktop"#,
            CosmeticFilterBreakdown {
                selector: r#"tr.success.desktop"#.to_string(),
                entities: sort_hash_domains(vec!["monova"]),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"monova.*#@#script + [class] > [class]:first-child"#,
            CosmeticFilterBreakdown {
                selector: r#"script + [class] > [class]:first-child"#.to_string(),
                entities: sort_hash_domains(vec!["monova"]),
                unhide: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"adshort.im,adsrt.*#@#[id*="ScriptRoot"]"#,
            CosmeticFilterBreakdown {
                selector: r#"[id*="ScriptRoot"]"#.to_string(),
                hostnames: sort_hash_domains(vec!["adshort.im"]),
                entities: sort_hash_domains(vec!["adsrt"]),
                unhide: true,
                ..Default::default()
            },
        );
        check_parse_result(
            r#"downloadsource.*##.date:not(dt):style(display: block !important;)"#,
            CosmeticFilterBreakdown {
                selector: r#".date:not(dt)"#.to_string(),
                entities: sort_hash_domains(vec!["downloadsource"]),
                action: Some(CosmeticFilterAction::Style("display: block !important;".into())),
                is_class_selector: true,
                key: Some("date".to_string()),
                ..Default::default()
            },
        );
    }

    #[test]
    fn styles() {
        check_parse_result(
            r#"chip.de##.video-wrapper > video[style]:style(display:block!important;padding-top:0!important;)"#,
            CosmeticFilterBreakdown {
                selector: r#".video-wrapper > video[style]"#.to_string(),
                hostnames: sort_hash_domains(vec!["chip.de"]),
                action: Some(CosmeticFilterAction::Style("display:block!important;padding-top:0!important;".into())),
                is_class_selector: true,
                key: Some("video-wrapper".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"allmusic.com##.advertising.medium-rectangle:style(min-height: 1px !important;)"#,
            CosmeticFilterBreakdown {
                selector: r#".advertising.medium-rectangle"#.to_string(),
                hostnames: sort_hash_domains(vec!["allmusic.com"]),
                action: Some(CosmeticFilterAction::Style("min-height: 1px !important;".into())),
                is_class_selector: true,
                key: Some("advertising".to_string()),
                ..Default::default()
            },
        );
        #[cfg(feature = "css-validation")]
        check_parse_result(
            r#"quora.com##.signup_wall_prevent_scroll .SiteHeader,.signup_wall_prevent_scroll .LoggedOutFooter,.signup_wall_prevent_scroll .ContentWrapper:style(filter: none !important;)"#,
            CosmeticFilterBreakdown {
                selector: r#".signup_wall_prevent_scroll .SiteHeader, .signup_wall_prevent_scroll .LoggedOutFooter, .signup_wall_prevent_scroll .ContentWrapper"#.to_string(),
                hostnames: sort_hash_domains(vec!["quora.com"]),
                action: Some(CosmeticFilterAction::Style("filter: none !important;".into())),
                is_class_selector: true,
                key: Some("signup_wall_prevent_scroll".to_string()),
                ..Default::default()
            }
        );
        check_parse_result(
            r#"imdb.com##body#styleguide-v2:style(background-color: #e3e2dd !important; background-image: none !important;)"#,
            CosmeticFilterBreakdown {
                selector: r#"body#styleguide-v2"#.to_string(),
                hostnames: sort_hash_domains(vec!["imdb.com"]),
                action: Some(CosmeticFilterAction::Style("background-color: #e3e2dd !important; background-image: none !important;".into())),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"streamcloud.eu###login > div[style^="width"]:style(display: block !important)"#,
            CosmeticFilterBreakdown {
                selector: r#"#login > div[style^="width"]"#.to_string(),
                hostnames: sort_hash_domains(vec!["streamcloud.eu"]),
                action: Some(CosmeticFilterAction::Style("display: block !important".into())),
                is_id_selector: true,
                key: Some("login".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            r#"moonbit.co.in,moondoge.co.in,moonliteco.in##[src^="//coinad.com/ads/"]:style(visibility: collapse !important)"#,
            CosmeticFilterBreakdown {
                selector: r#"[src^="//coinad.com/ads/"]"#.to_string(),
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
                selector: "#неделя".to_string(),
                is_unicode: true,
                is_id_selector: true,
                key: Some("неделя".to_string()),
                ..Default::default()
            },
        );
        check_parse_result(
            "неlloworlд.com#@##week",
            CosmeticFilterBreakdown {
                selector: "#week".to_string(),
                hostnames: sort_hash_domains(vec!["xn--lloworl-5ggb3f.com"]),
                is_unicode: true,
                is_id_selector: true,
                key: Some("week".to_string()),
                unhide: true,
                ..Default::default()
            }
        );
    }

    #[test]
    #[cfg(feature = "css-validation")]
    fn unsupported() {
        assert!(parse_cf("yandex.*##.serp-item:if(:scope > div.organic div.organic__subtitle:matches-css-after(content: /[Рр]еклама/))").is_err());
        assert!(parse_cf(r#"facebook.com,facebookcorewwwi.onion##.ego_column:if(a[href^="/campaign/landing"])"#).is_err());
        assert!(parse_cf(r#"readcomiconline.to##^script:has-text(this[atob)"#).is_err());
        assert!(parse_cf("twitter.com##article:has-text(/Promoted|Gesponsert|Реклама|Promocionado/):xpath(../..)").is_err());
        assert!(parse_cf("##").is_err());
        assert!(parse_cf("").is_err());

        // `:has` was previously limited to procedural filtering, but is now a native CSS feature.
        assert!(parse_cf(r#"thedailywtf.com##.article-body > div:has(a[href*="utm_medium"])"#).is_ok());
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
    #[cfg(feature = "css-validation")]
    fn abp_has_conversion() {
        let rule = parse_cf("imgur.com#?#div.Gallery-Sidebar-PostContainer:-abp-has(div.promoted-hover)").unwrap();
        assert_eq!(rule.selector, "div.Gallery-Sidebar-PostContainer:has(div.promoted-hover)");
        let rule = parse_cf(r##"webtools.fineaty.com#?#div[class*=" hidden-"]:-abp-has(.adsbygoogle)"##).unwrap();
        assert_eq!(rule.selector, r#"div[class*=" hidden-"]:has(.adsbygoogle)"#);
        let rule = parse_cf(r##"facebook.com,facebookcorewwwi.onion#?#._6y8t:-abp-has(a[href="/ads/about/?entry_product=ad_preferences"])"##).unwrap();
        assert_eq!(rule.selector, r#"._6y8t:has(a[href="/ads/about/?entry_product=ad_preferences"])"#);
        let rule = parse_cf(r##"mtgarena.pro#?##root > div > div:-abp-has(> .vm-placement)"##).unwrap();
        assert_eq!(rule.selector, r#"#root > div > div:has(> .vm-placement)"#);
        // Error without `#?#`:
        assert!(parse_cf(r##"mtgarena.pro###root > div > div:-abp-has(> .vm-placement)"##).is_err());
    }
}
