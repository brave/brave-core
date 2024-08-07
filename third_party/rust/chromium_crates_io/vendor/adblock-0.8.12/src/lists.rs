//! Parsing functions and collections for handling with multiple filter rules.

use std::convert::TryFrom;

use crate::filters::network::{NetworkFilter, NetworkFilterError};
use crate::filters::cosmetic::{CosmeticFilter, CosmeticFilterError};
use crate::resources::PermissionMask;

use itertools::{Either, Itertools};
use memchr::memchr as find_char;
use serde::{Deserialize, Serialize};
use thiserror::Error;

/// Specifies rule types to keep during parsing.
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum RuleTypes {
    All,
    NetworkOnly,
    CosmeticOnly,
}

impl Default for RuleTypes {
    fn default() -> Self {
        Self::All
    }
}

impl RuleTypes {
    pub fn loads_network_rules(&self) -> bool {
        matches!(self, Self::All | Self::NetworkOnly)
    }

    pub fn loads_cosmetic_rules(&self) -> bool {
        matches!(self, Self::All | Self::CosmeticOnly)
    }
}

/// Options for tweaking how a filter or list of filters is interpreted when parsing. It's
/// recommended to use _struct update syntax_ with a `default()` "rest" value; adding new fields to
/// this struct will not be considered a breaking change.
///
/// ```
/// # use adblock::lists::{FilterFormat, ParseOptions};
/// let parse_options = ParseOptions {
///     format: FilterFormat::Hosts,
///     ..ParseOptions::default()
/// };
/// ```
#[derive(Copy, Clone, Deserialize)]
pub struct ParseOptions {
    /// Assume filters are in the given format when parsing. Defaults to `FilterFormat::Standard`.
    #[serde(default)]
    pub format: FilterFormat,
    /// Specifies rule types to keep during parsing. Defaults to `RuleTypes::All`. This can be used
    /// to reduce the memory impact of engines that will only be used for cosmetic filtering or
    /// network filtering, but not both. It can also be useful for iOS and macOS when exporting to
    /// content-blocking syntax, as these platforms limit the number of content blocking rules that
    /// can be loaded.
    #[serde(default)]
    pub rule_types: RuleTypes,
    /// Specifies permissions to use when parsing a given filter list. See [`PermissionMask`] for
    /// more info.
    #[serde(default)]
    pub permissions: PermissionMask,
}

impl Default for ParseOptions {
    fn default() -> Self {
        ParseOptions {
            format: FilterFormat::Standard,
            rule_types: RuleTypes::All,
            permissions: PermissionMask::default(),
        }
    }
}

/// Manages a set of rules to be added to an [`crate::Engine`].
///
/// To be able to efficiently handle special options like `$badfilter`, and to allow optimizations,
/// all rules must be available when the `Engine` is first created. `FilterSet` allows assembling a
/// compound list from multiple different sources before compiling the rules into an `Engine`.
#[derive(Clone)]
pub struct FilterSet {
    debug: bool,
    pub(crate) network_filters: Vec<NetworkFilter>,
    pub(crate) cosmetic_filters: Vec<CosmeticFilter>,
}

/// Collects metadata for the list by reading just until the first non-comment line.
pub fn read_list_metadata(list: &str) -> FilterListMetadata {
    let mut metadata = FilterListMetadata::default();

    // uBO only searches within the first 1024 characters; the same optimization can be useful here
    let mut cutoff = list.len().min(1024);

    while !list.is_char_boundary(cutoff) {
        cutoff -= 1;
    }

    // String slice is safe here because `cutoff` is guaranteed to be a character boundary
    for line in list[0..cutoff].lines() {
        if line.starts_with('!') {
            metadata.try_add(line);
        } else if line.starts_with('[') {
            continue;
        } else {
            break;
        }
    }

    metadata
}

impl Default for FilterSet {
    /// Equivalent to `FilterSet::new(false)`, or `FilterSet::new(true)` when compiled in test
    /// configuration.
    fn default() -> Self {
        #[cfg(not(test))]
        let debug = false;

        #[cfg(test)]
        let debug = true;

        Self::new(debug)
    }
}

/// Corresponds to the `expires` field of `FilterListMetadata`.
#[derive(Debug, PartialEq, Serialize)]
pub enum ExpiresInterval {
    Hours(u16),
    Days(u8),
}

impl TryFrom<&str> for ExpiresInterval {
    type Error = ();

    fn try_from(v: &str) -> Result<Self, ()> {
        const DAYS_MAX: u8 = 14;
        const HOURS_MAX: u16 = DAYS_MAX as u16 * 24;

        // Extract time amount and unit from str
        let mut v_split = v.split(' ');
        let amount = v_split.next().ok_or(())?;
        let unit = v_split.next().ok_or(())?;
        // str::parse::<u16> accepts a leading plus sign, but we explicitly forbid it here
        if amount.starts_with('+') {
            return Err(());
        }
        // Only accept values in the range [1, MAX] for values with a matching unit
        match unit {
            "hour" | "hours" => {
                let amount = amount.parse::<u16>().map_err(|_| ())?;
                if (1..=HOURS_MAX).contains(&amount) {
                    return Ok(Self::Hours(amount));
                }
            },
            "day" | "days" => {
                let amount = amount.parse::<u8>().map_err(|_| ())?;
                if (1..=DAYS_MAX).contains(&amount) {
                    return Ok(Self::Days(amount))
                }
            }
            _ => ()
        }
        Err(())
    }
}

/// Includes information about any "special comments" as described by
/// <https://help.eyeo.com/adblockplus/how-to-write-filters#special-comments>
#[derive(Default, Serialize)]
pub struct FilterListMetadata {
    /// `! Homepage: http://example.com` - This comment determines which webpage should be linked
    /// as filter list homepage.
    pub homepage: Option<String>,
    /// `! Title: FooList` - This comment sets a fixed title for the filter list. If this comment
    /// is present, the user is no longer able to change the title.
    pub title: Option<String>,
    /// `! Expires: 5 days` - This comment sets the update interval for the filter list. The value
    /// can be given in days (e.g. 5 days) or hours (e.g. 8 hours). Any value between 1 hour and 14
    /// days is possible. Note that the update will not necessarily happen after this time
    /// interval. The actual update time is slightly randomized and depends on some additional
    /// factors to reduce server load.
    pub expires: Option<ExpiresInterval>,
    /// `! Redirect: http://example.com/list.txt` - This comment indicates that the filter list has
    /// moved to a new download address. Adblock Plus ignores any file content beyond that comment
    /// and immediately tries downloading from the new address. In case of success, the address of
    /// the filter list is updated in the settings. This comment is ignored if the new address is
    /// the same as the current address, meaning that it can be used to enforce the "canonical"
    /// address of the filter list.
    pub redirect: Option<String>,
}

impl FilterListMetadata {
    /// Attempts to add a line of a filter list to this collection of metadata. Only comment lines
    /// with valid metadata content will be added. Previously added information will not be
    /// rewritten.
    fn try_add(&mut self, line: &str) {
        if let Some(kv) = line.strip_prefix("! ") {
            if let Some((key, value)) = kv.split_once(": ") {
                match key {
                    "Homepage" if self.homepage.is_none() => self.homepage = Some(value.to_string()),
                    "Title" if self.title.is_none() => self.title = Some(value.to_string()),
                    "Expires" if self.expires.is_none() => {
                        if let Ok(expires) = ExpiresInterval::try_from(value) {
                            self.expires = Some(expires);
                        }
                    }
                    "Redirect" if self.redirect.is_none() => self.redirect = Some(value.to_string()),
                    _ => (),
                }
            }
        }
    }
}

impl FilterSet {
    /// Creates a new `FilterSet`. `debug` specifies whether or not to save information about the
    /// original raw filter rules alongside the more compact internal representation. If enabled,
    /// this information will be passed to the corresponding `Engine`.
    pub fn new(debug: bool) -> Self {
        Self {
            debug,
            network_filters: Vec::new(),
            cosmetic_filters: Vec::new(),
        }
    }

    /// Adds the contents of an entire filter list to this `FilterSet`. Filters that cannot be
    /// parsed successfully are ignored. Returns any discovered metadata about the list of rules
    /// added.
    pub fn add_filter_list(&mut self, filter_list: &str, opts: ParseOptions) -> FilterListMetadata {
        self.add_filters(filter_list.lines(), opts)
    }

    /// Adds a collection of filter rules to this `FilterSet`. Filters that cannot be parsed
    /// successfully are ignored. Returns any discovered metadata about the list of rules added.
    pub fn add_filters(&mut self, filters: impl IntoIterator<Item=impl AsRef<str>>, opts: ParseOptions) -> FilterListMetadata {
        let (metadata, mut parsed_network_filters, mut parsed_cosmetic_filters) = parse_filters_with_metadata(filters, self.debug, opts);
        self.network_filters.append(&mut parsed_network_filters);
        self.cosmetic_filters.append(&mut parsed_cosmetic_filters);
        metadata
    }

    /// Adds the string representation of a single filter rule to this `FilterSet`.
    pub fn add_filter(&mut self, filter: &str, opts: ParseOptions) -> Result<(), FilterParseError> {
        let filter_parsed = parse_filter(filter, self.debug, opts);
        match filter_parsed? {
            ParsedFilter::Network(filter) => self.network_filters.push(filter),
            ParsedFilter::Cosmetic(filter) => self.cosmetic_filters.push(filter),
        }
        Ok(())
    }

    /// Consumes this `FilterSet`, returning an equivalent list of content blocking rules and a
    /// corresponding new list containing the `String` representation of all filters that were
    /// successfully converted (as `FilterFormat::Standard` rules).
    ///
    /// The list of content blocking rules will be properly ordered to ensure correct behavior of
    /// `ignore-previous-rules`-typed rules.
    ///
    /// This function will fail if the `FilterSet` was not created in debug mode.
    #[cfg(feature = "content-blocking")]
    pub fn into_content_blocking(self) -> Result<(Vec<crate::content_blocking::CbRule>, Vec<String>), ()> {
        use crate::content_blocking;

        if !self.debug {
            return Err(())
        }

        let mut ignore_previous_rules = vec![];
        let mut other_rules = vec![];

        let mut filters_used = vec![];

        self.network_filters.into_iter().for_each(|filter| {
            let original_rule = *filter.raw_line.clone().expect("All rules should be in debug mode");
            if let Ok(equivalent) = TryInto::<content_blocking::CbRuleEquivalent>::try_into(filter) {
                filters_used.push(original_rule);
                equivalent.into_iter().for_each(|cb_rule| {
                    match &cb_rule.action.typ {
                        content_blocking::CbType::IgnorePreviousRules => ignore_previous_rules.push(cb_rule),
                        _ => other_rules.push(cb_rule),
                    }
                });
            }
        });

        let add_fp_document_exception = !filters_used.is_empty();

        self.cosmetic_filters.into_iter().for_each(|filter| {
            let original_rule = *filter.raw_line.clone().expect("All rules should be in debug mode");
            if let Ok(cb_rule) = TryInto::<content_blocking::CbRule>::try_into(filter) {
                filters_used.push(original_rule);
                match &cb_rule.action.typ {
                    content_blocking::CbType::IgnorePreviousRules => ignore_previous_rules.push(cb_rule),
                    _ => other_rules.push(cb_rule),
                }
            }
        });

        other_rules.append(&mut ignore_previous_rules);

        if add_fp_document_exception {
            other_rules.push(content_blocking::ignore_previous_fp_documents());
        }

        Ok((other_rules, filters_used))
    }
}

/// Denotes the format of a particular list resource, which affects how its rules should be parsed.
#[derive(Debug, Clone, Copy, Serialize, Deserialize)]
pub enum FilterFormat {
    /// Rules should be parsed in ABP/uBO-style format.
    Standard,
    /// Each line consists of an IP address (usually 127.0.0.1 or 0.0.0.0), some whitespace, and a
    /// single hostname. This syntax is normally used directly for HOSTS-based adblockers. These
    /// rules will be treated equivalently to `"||hostname^"` rules in `Standard` format; the IP
    /// addresses will not be used.
    ///
    /// Note that some sources provide a more raw format, where each line consists of just a
    /// hostname. This option will also accept that format.
    ///
    /// For this option, `!` is accepted as a comment character at the beginning of a line, and `#`
    /// is accepted as a comment character anywhere in a line.
    Hosts,
}

/// Default to parsing lists in `Standard` format.
impl Default for FilterFormat {
    fn default() -> Self {
        Self::Standard
    }
}

/// Describes the type of a single filter.
#[derive(Debug, PartialEq)]
pub enum FilterType {
    /// A network filter, used for changing the behavior of network requests
    Network,
    /// A network filter, used for changing the behavior of fetched pages
    Cosmetic,
    /// Something else that isn't supported
    NotSupported,
}

/// Successful result of parsing a single filter rule
pub enum ParsedFilter {
    Network(NetworkFilter),
    Cosmetic(CosmeticFilter),
}

impl From<NetworkFilter> for ParsedFilter {
    fn from(v: NetworkFilter) -> Self {
        ParsedFilter::Network(v)
    }
}

impl From<CosmeticFilter> for ParsedFilter {
    fn from(v: CosmeticFilter) -> Self {
        ParsedFilter::Cosmetic(v)
    }
}

/// Unsuccessful result of parsing a single filter rule.
#[derive(Debug, Error)]
pub enum FilterParseError {
    #[error("network filter error: {0}")]
    Network(#[source] NetworkFilterError),
    #[error("cosmetic filter error: {0}")]
    Cosmetic(#[source] CosmeticFilterError),
    #[error("unsupported")]
    Unsupported,
    #[error("empty")]
    Empty,
}

impl From<NetworkFilterError> for FilterParseError {
    fn from(v: NetworkFilterError) -> Self {
        FilterParseError::Network(v)
    }
}

impl From<CosmeticFilterError> for FilterParseError {
    fn from(v: CosmeticFilterError) -> Self {
        FilterParseError::Cosmetic(v)
    }
}

/// Parse a single filter rule
pub fn parse_filter(
    line: &str,
    debug: bool,
    opts: ParseOptions,
) -> Result<ParsedFilter, FilterParseError> {
    let filter = line.trim();

    if filter.is_empty() {
        return Err(FilterParseError::Empty);
    }

    match opts.format {
        FilterFormat::Standard => {
            match (detect_filter_type(filter), opts.rule_types) {
                (FilterType::Network, RuleTypes::All | RuleTypes::NetworkOnly) => NetworkFilter::parse(filter, debug, opts)
                    .map(|f| f.into())
                    .map_err(|e| e.into()),
                (FilterType::Cosmetic, RuleTypes::All | RuleTypes::CosmeticOnly) => CosmeticFilter::parse(filter, debug, opts.permissions)
                    .map(|f| f.into())
                    .map_err(|e| e.into()),
                _ => Err(FilterParseError::Unsupported),
            }
        }
        FilterFormat::Hosts => {
            // Hosts-style rules can only ever be network rules
            if !opts.rule_types.loads_network_rules() {
                return Err(FilterParseError::Unsupported);
            }
            if filter.starts_with('!') {
                return Err(FilterParseError::Unsupported);
            }
            // Discard contents after first `#` character
            let filter = if let Some(hash_loc) = find_char(b'#', filter.as_bytes()) {
                let filter = &filter[..hash_loc];
                let filter = filter.trim();

                if filter.is_empty() {
                    return Err(FilterParseError::Unsupported);
                }

                filter
            } else {
                filter
            };

            // Take the last of at most 2 whitespace separated fields
            let mut filter_parts = filter.split_whitespace();
            let hostname = match (filter_parts.next(), filter_parts.next(), filter_parts.next()) {
                (None, None, None) => return Err(FilterParseError::Unsupported),
                (Some(hostname), None, None) => hostname,
                (Some(_ip), Some(hostname), None) => hostname,
                (Some(_), Some(_), Some(_)) => return Err(FilterParseError::Unsupported),
                _ => unreachable!(),
            };

            // Matches in hosts lists are usually redirected to localhost. For that reason, some
            // lists include an entry for "localhost", which should be explicitly ignored when
            // performing request-level adblocking.
            if hostname == "localhost" {
                return Err(FilterParseError::Unsupported);
            }

            NetworkFilter::parse_hosts_style(hostname, debug)
                .map(|f| f.into())
                .map_err(|e| e.into())
        }
    }
}

/// Parse an entire list of filters, ignoring any errors
pub fn parse_filters(
    list: impl IntoIterator<Item=impl AsRef<str>>,
    debug: bool,
    opts: ParseOptions,
) -> (Vec<NetworkFilter>, Vec<CosmeticFilter>) {
    let (_metadata, network_filters, cosmetic_filters) = parse_filters_with_metadata(
        list,
        debug,
        opts,
    );

    (network_filters, cosmetic_filters)
}

/// Parse an entire list of filters, ignoring any errors
pub fn parse_filters_with_metadata(
    list: impl IntoIterator<Item=impl AsRef<str>>,
    debug: bool,
    opts: ParseOptions,
) -> (FilterListMetadata, Vec<NetworkFilter>, Vec<CosmeticFilter>) {
    let mut metadata = FilterListMetadata::default();

    let list_iter = list.into_iter();

    let (network_filters, cosmetic_filters): (Vec<_>, Vec<_>) = list_iter
        .map(|line| {
            metadata.try_add(line.as_ref());
            parse_filter(line.as_ref(), debug, opts)
        })
        .filter_map(Result::ok)
        .partition_map(|filter| match filter {
            ParsedFilter::Network(f) => Either::Left(f),
            ParsedFilter::Cosmetic(f) => Either::Right(f),
        });

    (metadata, network_filters, cosmetic_filters)
}

/// Given a single line, checks if this would likely be a cosmetic filter, a
/// network filter or something that is not supported. This check is performed
/// before calling a more specific parser to create an instance of
/// `NetworkFilter` or `CosmeticFilter`.
fn detect_filter_type(filter: &str) -> FilterType {
    // Ignore comments
    if filter.len() == 1
        || filter.starts_with('!')
        || (filter.starts_with('#') && filter[1..].starts_with(char::is_whitespace))
        || filter.starts_with("[Adblock")
    {
        return FilterType::NotSupported;
    }

    if filter.starts_with('|') || filter.starts_with("@@|") {
        return FilterType::Network;
    }

    // Check if filter is cosmetic
    if let Some(sharp_index) = find_char(b'#', filter.as_bytes()) {
        let after_sharp_index = sharp_index + 1;

        // Check the next few bytes for a second `#`
        // Indexing is safe here because it uses the filter's byte
        // representation and guards against short strings
        if find_char(b'#', &filter.as_bytes()[after_sharp_index..(after_sharp_index+4).min(filter.len())]).is_some() {
            return FilterType::Cosmetic;
        }
    }

    // Ignore Adguard cosmetics
    if filter.contains("$$") {
        return FilterType::NotSupported;
    }

    // Everything else is a network filter
    FilterType::Network
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_hosts_style() {
        {
            let input = "www.malware.com";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_ok());
        }
        {
            let input = "www.malware.com/virus.txt";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_err());
        }
        {
            let input = "127.0.0.1 www.malware.com";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_ok());
        }
        {
            let input = "127.0.0.1\t\twww.malware.com";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_ok());
        }
        {
            let input = "0.0.0.0    www.malware.com";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_ok());
        }
        {
            let input = "0.0.0.0    www.malware.com     # replace after issue #289336 is addressed";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_ok());
        }
        {
            let input = "! Title: list.txt";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_err());
        }
        {
            let input = "127.0.0.1 localhost";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_err());
        }
        {
            let input = "127.0.0.1 com";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_err());
        }
        {
            let input = ".com";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_err());
        }
        {
            let input = "*.com";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_err());
        }
        {
            let input = "www.";
            let result = parse_filter(input, true, ParseOptions { format: FilterFormat::Hosts, ..Default::default() });
            assert!(result.is_err());
        }
    }

    #[test]
    fn adguard_cosmetic_detection() {
        {
            let input = r#"example.org$$script[data-src="banner"]"#;
            let result = parse_filter(input, true, Default::default());
            assert!(result.is_err());
        }
        {
            let input = "example.org##+js(set-local-storage-item, Test, $$remove$$)";
            let result = parse_filter(input, true, Default::default());
            assert!(result.is_ok());
        }
        {
            let input = "[$app=org.example.app]example.com##.textad";
            let result = parse_filter(input, true, Default::default());
            assert!(result.is_err());
        }
        {
            let input = r#"[$domain=/^i\[a-z\]*\.strmrdr\[a-z\]+\..*/]##+js(set-constant, adscfg.enabled, false)"#;
            let result = parse_filter(input, true, Default::default());
            assert!(result.is_err());
        }
    }

    #[test]
    fn parse_filter_failed_fuzz_1() {
        let input = "Ѥ";
        let result = parse_filter(input, true, Default::default());
        assert!(result.is_ok());
    }

    #[test]
    fn parse_filter_failed_fuzz_2() {
        assert!(parse_filter(r#"###\\\00DB \008D"#, true, Default::default()).is_ok());
        assert!(parse_filter(r#"###\Û"#, true, Default::default()).is_ok());
    }

    #[test]
    fn parse_filter_failed_fuzz_3() {
        let input = "||$3p=/";
        let result = parse_filter(input, true, Default::default());
        assert!(result.is_ok());
    }

    #[test]
    fn parse_filter_failed_fuzz_4() {
        // \\##+js(,\xdd\x8d
        let parsed = parse_filter(
            &String::from_utf8(vec![92, 35, 35, 43, 106, 115, 40, 44, 221, 141]).unwrap(),
            true,
            Default::default(),
        );
        #[cfg(feature = "css-validation")]
        assert!(parsed.is_err());
        #[cfg(not(feature = "css-validation"))]
        assert!(parsed.is_ok());
    }

    #[test]
    #[cfg(feature = "css-validation")]
    fn parse_filter_opening_comment() {
        assert!(parse_filter(
            "##input,input/*",
            true,
            Default::default(),
        ).is_err());
    }

    #[test]
    fn test_parse_expires_interval() {
        assert_eq!(ExpiresInterval::try_from("0 hour"), Err(()));
        assert_eq!(ExpiresInterval::try_from("0 hours"), Err(()));
        assert_eq!(ExpiresInterval::try_from("1 hour"), Ok(ExpiresInterval::Hours(1)));
        assert_eq!(ExpiresInterval::try_from("1 hours"), Ok(ExpiresInterval::Hours(1)));
        assert_eq!(ExpiresInterval::try_from("2 hours"), Ok(ExpiresInterval::Hours(2)));
        assert_eq!(ExpiresInterval::try_from("2 hour"), Ok(ExpiresInterval::Hours(2)));
        assert_eq!(ExpiresInterval::try_from("3.5 hours"), Err(()));
        assert_eq!(ExpiresInterval::try_from("336 hours"), Ok(ExpiresInterval::Hours(336)));
        assert_eq!(ExpiresInterval::try_from("337 hours"), Err(()));

        assert_eq!(ExpiresInterval::try_from("0 day"), Err(()));
        assert_eq!(ExpiresInterval::try_from("0 days"), Err(()));
        assert_eq!(ExpiresInterval::try_from("1 day"), Ok(ExpiresInterval::Days(1)));
        assert_eq!(ExpiresInterval::try_from("1 days"), Ok(ExpiresInterval::Days(1)));
        assert_eq!(ExpiresInterval::try_from("2 days"), Ok(ExpiresInterval::Days(2)));
        assert_eq!(ExpiresInterval::try_from("2 day"), Ok(ExpiresInterval::Days(2)));
        assert_eq!(ExpiresInterval::try_from("3.5 days"), Err(()));
        assert_eq!(ExpiresInterval::try_from("14 days"), Ok(ExpiresInterval::Days(14)));
        assert_eq!(ExpiresInterval::try_from("15 days"), Err(()));

        assert_eq!(ExpiresInterval::try_from("-5 hours"), Err(()));
        assert_eq!(ExpiresInterval::try_from("+5 hours"), Err(()));

        assert_eq!(ExpiresInterval::try_from("2 days (update frequency)"), Ok(ExpiresInterval::Days(2)));
        assert_eq!(ExpiresInterval::try_from("2 hours (update frequency)"), Ok(ExpiresInterval::Hours(2)));
    }

    #[test]
    fn test_parsing_list_metadata() {
        let list = [
            "[Adblock Plus 2.0]",
            "! Title: 0131 Block List",
            "! Homepage: https://austinhuang.me/0131-block-list",
            "! Licence: https://creativecommons.org/licenses/by-sa/4.0/",
            "! Expires: 7 days",
            "! Version: 20220411",
            "",
            "! => https://austinhuang.me/0131-block-list/list.txt",
        ];

        let mut filter_set = FilterSet::new(false);
        let metadata = filter_set.add_filters(list, ParseOptions::default());

        assert_eq!(metadata.title, Some("0131 Block List".to_string()));
        assert_eq!(metadata.homepage, Some("https://austinhuang.me/0131-block-list".to_string()));
        assert_eq!(metadata.expires, Some(ExpiresInterval::Days(7)));
        assert_eq!(metadata.redirect, None);
    }

    #[test]
    /// Some lists are formatted in unusual ways. This example has a version string with
    /// non-numeric characters and an `Expires` field with extra information trailing afterwards.
    /// Valid fields should still be recognized and parsed accordingly.
    fn test_parsing_list_best_effort() {
        let list = [
            "[Adblock Plus 2]",
            "!-----------------------------------",
            "!             ABOUT",
            "!-----------------------------------",
            "! Version: 1.2.0.0",
            "! Title: ABPVN Advanced",
            "! Last modified: 09/03/2021",
            "! Expires: 7 days (update frequency)",
            "! Homepage: https://www.haopro.net/",
        ];

        let mut filter_set = FilterSet::new(false);
        let metadata = filter_set.add_filters(list, ParseOptions::default());

        assert_eq!(metadata.title, Some("ABPVN Advanced".to_string()));
        assert_eq!(metadata.homepage, Some("https://www.haopro.net/".to_string()));
        assert_eq!(metadata.expires, Some(ExpiresInterval::Days(7)));
        assert_eq!(metadata.redirect, None);
    }

    #[test]
    fn test_read_metadata() {
        {
            let list =
r##"! Title: uBlock₀ filters – Annoyances
! Description: Filters optimized for uBlock Origin, to be used with Fanboy's
!              and/or Adguard's "Annoyances" list(s)
! Expires: 4 days
! Last modified: %timestamp%
! License: https://github.com/uBlockOrigin/uAssets/blob/master/LICENSE
! Homepage: https://github.com/uBlockOrigin/uAssets
! Forums: https://github.com/uBlockOrigin/uAssets/issues"##;
            let metadata = read_list_metadata(&list);

            assert_eq!(metadata.title, Some("uBlock₀ filters – Annoyances".to_string()));
            assert_eq!(metadata.homepage, Some("https://github.com/uBlockOrigin/uAssets".to_string()));
            assert_eq!(metadata.expires, Some(ExpiresInterval::Days(4)));
            assert_eq!(metadata.redirect, None);
        }
        {
            let list =
r##"[uBlock Origin]
! Title: PersianBlocker
! Description: سرانجام، یک لیست بهینه و گسترده برای مسدودسازی تبلیغ ها و ردیاب ها در سایت های پارسی زبان!
! Expires: 2 days
! Last modified: 2022-12-11
! Homepage: https://github.com/MasterKia/PersianBlocker
! License: AGPLv3 (https://github.com/MasterKia/PersianBlocker/blob/main/LICENSE)

! مشکل/پیشنهاد: https://github.com/MasterKia/PersianBlocker/issues
! مشارکت: https://github.com/MasterKia/PersianBlocker/pulls

!  لیستی برای برگرداندن آزادی کاربران، چون هر کاربر این آزادی را دارد که چه چیزی وارد مرورگرش می‌شود و چه چیزی وارد نمی‌شود
!-------------------------v Experimental Generic Filters v-----------------------!
! applicationha.com, androidgozar.com, downloadkral.com, gold-team.org, iranecar.com, icoff.ee, koolakmag.ir,
!! mybia4music.com, my-film.pw, pedal.ir, vgdl.ir, sakhamusic.ir
/wp-admin/admin-ajax.php?postviews_id=$xhr
"##;
            let metadata = read_list_metadata(&list);

            assert_eq!(metadata.title, Some("PersianBlocker".to_string()));
            assert_eq!(metadata.homepage, Some("https://github.com/MasterKia/PersianBlocker".to_string()));
            assert_eq!(metadata.expires, Some(ExpiresInterval::Days(2)));
            assert_eq!(metadata.redirect, None);
        }
    }

    #[test]
    fn parse_cosmetic_variants() {
        {
            let input = "example.com##.selector";
            let result = parse_filter(input, true, Default::default());
            assert!(matches!(result, Ok(ParsedFilter::Cosmetic(..))));
        }
        {
            let input = "9gag.com#?#article:-abp-has(.promoted)";
            let result = parse_filter(input, true, Default::default());
            assert!(matches!(result, Ok(ParsedFilter::Cosmetic(..))));
        }
        #[cfg(feature = "css-validation")]
        {
            let input = "sportowefakty.wp.pl#@?#body > [class]:not([id]):matches-css(position: fixed):matches-css(top: 0px)";
            let result = parse_filter(input, true, Default::default());
            assert!(matches!(result, Err(FilterParseError::Cosmetic(CosmeticFilterError::InvalidCssSelector))));
        }
        {
            let input = r#"odkrywamyzakryte.com#%#//scriptlet("abort-on-property-read", "sc_adv_out")"#;
            let result = parse_filter(input, true, Default::default());
            assert!(matches!(result, Err(FilterParseError::Cosmetic(CosmeticFilterError::UnsupportedSyntax))));
        }
        {
            let input = "bikeradar.com,spiegel.de#@%#!function(){function b(){}function a(a){return{get:function(){return a},set:b}}function c(a)";
            let result = parse_filter(input, true, Default::default());
            assert!(matches!(result, Err(FilterParseError::Cosmetic(CosmeticFilterError::UnsupportedSyntax))));
        }
        {
            let input = "nczas.com#$#.adsbygoogle { position: absolute!important; left: -3000px!important; }";
            let result = parse_filter(input, true, Default::default());
            assert!(matches!(result, Err(FilterParseError::Cosmetic(CosmeticFilterError::UnsupportedSyntax))));
        }
        {
            let input = "kurnik.pl#@$#.adsbygoogle { height: 1px !important; width: 1px !important; }";
            let result = parse_filter(input, true, Default::default());
            assert!(matches!(result, Err(FilterParseError::Cosmetic(CosmeticFilterError::UnsupportedSyntax))));
        }
    }
}
