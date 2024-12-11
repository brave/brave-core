use std::error::Error;
use std::fmt;
use std::fmt::Debug;
use std::hash::Hasher;
use std::io::{BufRead, BufReader, Read};

use chrono::{DateTime, Utc};
use siphasher::sip128::{Hasher128, SipHasher};

use crate::model;
use crate::parser::util::{IdGenerator, TimestampParser};
use crate::xml;
use crate::xml::NS;

mod atom;
mod json;
mod rss0;
mod rss1;
mod rss2;

pub(crate) mod itunes;
pub(crate) mod mediarss;
pub(crate) mod util;

pub type ParseFeedResult<T> = Result<T, ParseFeedError>;

/// An error returned when parsing a feed from a source fails
#[derive(Debug)]
pub enum ParseFeedError {
    // TODO add line number/position
    ParseError(ParseErrorKind),
    // IO error
    IoError(std::io::Error),
    // Underlying issue with JSON (poorly formatted etc.)
    JsonSerde(serde_json::error::Error),
    // Unsupported version of the JSON feed
    JsonUnsupportedVersion(String),
    // Underlying issue with XML (poorly formatted etc.)
    XmlReader(xml::XmlError),
}

impl From<serde_json::error::Error> for ParseFeedError {
    fn from(err: serde_json::error::Error) -> Self {
        ParseFeedError::JsonSerde(err)
    }
}

impl From<std::io::Error> for ParseFeedError {
    fn from(err: std::io::Error) -> Self {
        ParseFeedError::IoError(err)
    }
}

impl From<xml::XmlError> for ParseFeedError {
    fn from(err: xml::XmlError) -> Self {
        ParseFeedError::XmlReader(err)
    }
}

impl fmt::Display for ParseFeedError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ParseFeedError::ParseError(pe) => write!(f, "unable to parse feed: {}", pe),
            ParseFeedError::IoError(ie) => write!(f, "unable to read feed: {}", ie),
            ParseFeedError::JsonSerde(je) => write!(f, "unable to parse JSON: {}", je),
            ParseFeedError::JsonUnsupportedVersion(version) => write!(f, "unsupported version: {}", version),
            ParseFeedError::XmlReader(xe) => write!(f, "unable to parse XML: {}", xe),
        }
    }
}

impl Error for ParseFeedError {
    fn source(&self) -> Option<&(dyn Error + 'static)> {
        match self {
            ParseFeedError::IoError(ie) => Some(ie),
            ParseFeedError::JsonSerde(je) => Some(je),
            ParseFeedError::XmlReader(xe) => Some(xe),
            _ => None,
        }
    }
}

/// Underlying cause of the parse failure
#[derive(Debug)]
pub enum ParseErrorKind {
    /// Could not find the expected root element (e.g. "channel" for RSS 2, a JSON node etc.)
    NoFeedRoot,
    /// The content type is unsupported, and we cannot parse the value into a known representation
    UnknownMimeType(String),
    /// Required content within the source was not found e.g. the XML child text element for a "content" element
    MissingContent(&'static str),
}

impl fmt::Display for ParseErrorKind {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            ParseErrorKind::NoFeedRoot => f.write_str("no root element"),
            ParseErrorKind::UnknownMimeType(mime) => write!(f, "unsupported content type {}", mime),
            ParseErrorKind::MissingContent(elem) => write!(f, "missing content element {}", elem),
        }
    }
}

/// Parser for various feed formats
pub struct Parser {
    base_uri: Option<String>,
    id_generator: Box<IdGenerator>,
    timestamp_parser: Box<TimestampParser>,
}

impl Parser {
    /// Parse the input (Atom, a flavour of RSS or JSON Feed) into our model
    ///
    /// # Arguments
    ///
    /// * `input` - A source of content such as a string, file etc.
    ///
    /// NOTE: feed-rs uses the encoding attribute in the XML prolog to decode content.
    /// HTTP libraries (such as reqwest) provide a `text()` method which applies the content-encoding header and decodes the source into UTF-8.
    /// This then causes feed-rs to fail when it attempts to interpret the UTF-8 stream as a different character set.
    /// Instead, pass the raw, encoded source to feed-rs e.g. the `.bytes()` method if using reqwest.
    ///
    /// # Examples
    ///
    /// ```
    /// use feed_rs::parser;
    /// let xml = r#"
    /// <feed>
    ///    <title type="text">sample feed</title>
    ///    <updated>2005-07-31T12:29:29Z</updated>
    ///    <id>feed1</id>
    ///    <entry>
    ///        <title>sample entry</title>
    ///        <id>entry1</id>
    ///    </entry>
    /// </feed>
    /// "#;
    /// let feed_from_xml = parser::parse(xml.as_bytes()).unwrap();
    ///
    ///
    /// ```
    pub fn parse<R: Read>(&self, source: R) -> ParseFeedResult<model::Feed> {
        // Buffer the reader for performance (e.g. when streaming from a network) and so we can peek to determine the type of content
        let mut input = BufReader::new(source);

        // Determine whether this is XML or JSON and call the appropriate parser
        input.fill_buf()?;
        let first_char = input.buffer().iter().find(|b| **b == b'<' || **b == b'{').map(|b| *b as char);
        let result = match first_char {
            Some('<') => self.parse_xml(input),

            Some('{') => self.parse_json(input),

            _ => Err(ParseFeedError::ParseError(ParseErrorKind::NoFeedRoot)),
        };

        // Post processing as required
        if let Ok(mut feed) = result {
            assign_missing_ids(&self.id_generator, &mut feed, self.base_uri.as_deref());

            Ok(feed)
        } else {
            result
        }
    }

    // Handles JSON content
    fn parse_json<R: BufRead>(&self, source: R) -> ParseFeedResult<model::Feed> {
        json::parse(self, source)
    }

    // Parses timestamps with the configured parser (internal, or supplied via the builder)
    fn parse_timestamp(&self, text: &str) -> Option<DateTime<Utc>> {
        (self.timestamp_parser)(text)
    }

    // Handles XML content
    fn parse_xml<R: BufRead>(&self, source: R) -> ParseFeedResult<model::Feed> {
        // Set up the source of XML elements from the input
        let element_source = xml::ElementSource::new(source, self.base_uri.as_deref())?;
        if let Ok(Some(root)) = element_source.root() {
            // Dispatch to the correct parser
            let version = root.attr_value("version");
            match (root.name.as_str(), version.as_deref()) {
                ("feed", _) => {
                    element_source.set_default_default_namespace(NS::Atom);
                    return atom::parse_feed(self, root);
                }
                ("entry", _) => {
                    element_source.set_default_default_namespace(NS::Atom);
                    return atom::parse_entry(self, root);
                }
                ("rss", Some("2.0")) => {
                    element_source.set_default_default_namespace(NS::RSS);
                    return rss2::parse(self, root);
                }
                ("rss", Some("0.91")) | ("rss", Some("0.92")) => {
                    element_source.set_default_default_namespace(NS::RSS);
                    return rss0::parse(self, root);
                }
                ("RDF", _) => {
                    element_source.set_default_default_namespace(NS::RSS);
                    return rss1::parse(self, root);
                }
                _ => {}
            };
        }

        // Couldn't find a recognised feed within the provided XML stream
        Err(ParseFeedError::ParseError(ParseErrorKind::NoFeedRoot))
    }
}

/// Parses the provided source with the defaults
///
/// Customisation of the parser (e.g. base URI, custom timestamp parsers etc can be configured through the builder.
pub fn parse<R: Read>(source: R) -> ParseFeedResult<model::Feed> {
    parse_with_uri(source, None)
}

/// Convenience during transition to the builder
#[deprecated(since = "1.5.3", note = "Replaced with base_uri() function on the builder")]
pub fn parse_with_uri<R: Read>(source: R, uri: Option<&str>) -> ParseFeedResult<model::Feed> {
    Builder::new().base_uri(uri).build().parse(source)
}

/// Builder to create instances of `FeedParser`
pub struct Builder {
    base_uri: Option<String>,
    id_generator: Box<IdGenerator>,
    timestamp_parser: Box<TimestampParser>,
}

impl Builder {
    /// Create a new instance of the builder
    pub fn new() -> Builder {
        Builder::default()
    }

    /// Source of the content, used to resolve relative URLs in XML based feeds
    pub fn base_uri<S: AsRef<str>>(mut self, uri: Option<S>) -> Self {
        self.base_uri = uri.map(|s| s.as_ref().to_string());
        self
    }

    /// Create a new instance of the parser
    pub fn build(self) -> Parser {
        Parser {
            base_uri: self.base_uri,
            id_generator: self.id_generator,
            timestamp_parser: self.timestamp_parser,
        }
    }

    /// Registers an ID generator
    pub fn id_generator<F>(mut self, generator: F) -> Self
        where
            F: Fn(&[model::Link], &Option<model::Text>, Option<&str>) -> String + 'static,
    {
        self.id_generator = Box::new(generator);
        self
    }

    /// Registers an ID generator compatible with v0.2 of feed-rs
    pub fn id_generator_v0_2(self) -> Self {
        self.id_generator(|links, title, _uri| {
            // If we have a link without relative components, use that
            if let Some(link) = links.iter().find(|l| l.rel.is_none()) {
                // Trim the trailing slash if it exists
                let mut link = model::Link::new(link.href.clone(), None);
                if link.href.ends_with('/') {
                    link.href.pop();
                }

                generate_id_from_link_and_title(&link, title)
            } else {
                util::uuid_gen()
            }
        })
    }

    /// Registers a custom timestamp parser
    pub fn timestamp_parser<F>(mut self, ts_parser: F) -> Self
        where
            F: Fn(&str) -> Option<DateTime<Utc>> + 'static,
    {
        self.timestamp_parser = Box::new(ts_parser);
        self
    }
}

/// Creates a parser instance with sensible defaults
impl Default for Builder {
    fn default() -> Self {
        Builder {
            base_uri: None,
            id_generator: Box::new(generate_id),
            timestamp_parser: Box::new(util::parse_timestamp_lenient),
        }
    }
}

// Assigns IDs to missing feed + entries as required
fn assign_missing_ids(id_generator: &IdGenerator, feed: &mut model::Feed, uri: Option<&str>) {
    if feed.id.is_empty() {
        feed.id = id_generator(&feed.links, &feed.title, uri);
    }

    for entry in feed.entries.iter_mut() {
        if entry.id.is_empty() {
            entry.id = id_generator(&entry.links, &entry.title, uri);
        }
    }
}

const LINK_HASH_KEY1: u64 = 0x5d78_4074_2887_2d60;
const LINK_HASH_KEY2: u64 = 0x90ee_ca4c_90a5_e228;

// Creates a unique ID by trying the following in order:
// 1) the first link + optional title
// 2) the uri + title provided
// 3) a UUID
pub fn generate_id(links: &[model::Link], title: &Option<model::Text>, uri: Option<&str>) -> String {
    if let Some(link) = links.first() {
        generate_id_from_link_and_title(link, title)
    } else if let (Some(uri), Some(title)) = (uri, title) {
        generate_id_from_uri_and_title(uri, title)
    } else {
        // Generate a UUID as last resort
        util::uuid_gen()
    }
}

// Generate an ID from the link + title
pub fn generate_id_from_link_and_title(link: &model::Link, title: &Option<model::Text>) -> String {
    let mut hasher = SipHasher::new_with_keys(LINK_HASH_KEY1, LINK_HASH_KEY2);
    hasher.write(link.href.as_bytes());
    if let Some(title) = title {
        hasher.write(title.content.as_bytes());
    }
    let hash = hasher.finish128();
    format!("{:x}{:x}", hash.h1, hash.h2)
}

// Generate an ID from the URI and title
pub fn generate_id_from_uri_and_title(uri: &str, title: &model::Text) -> String {
    let mut hasher = SipHasher::new_with_keys(LINK_HASH_KEY1, LINK_HASH_KEY2);
    hasher.write(uri.as_bytes());
    hasher.write(title.content.as_bytes());
    let hash = hasher.finish128();
    format!("{:x}{:x}", hash.h1, hash.h2)
}

#[cfg(test)]
mod tests;
