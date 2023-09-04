use std::error::Error;
use std::fmt;
use std::hash::Hasher;
use std::io::{BufRead, BufReader, Read};

use siphasher::sip128::{Hasher128, SipHasher};

use crate::model;
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

pub type ParseFeedResult<T> = std::result::Result<T, ParseFeedError>;

/// An error returned when parsing a feed from a source fails
#[derive(Debug)]
pub enum ParseFeedError {
    // TODO add line number/position
    ParseError(ParseErrorKind),
    // IO error
    IoError(std::io::Error),
    // Underlying issue with JSON (poorly formatted etc)
    JsonSerde(serde_json::error::Error),
    // Unsupported version of the JSON feed
    JsonUnsupportedVersion(String),
    // Underlying issue with XML (poorly formatted etc)
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
    /// Could not find the expected root element (e.g. "channel" for RSS 2, a JSON node etc)
    NoFeedRoot,
    /// The content type is unsupported and we cannot parse the value into a known representation
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

/// Convenience for `parse_with_uri()` with `None` as the base_uri
pub fn parse<R: Read>(source: R) -> ParseFeedResult<model::Feed> {
    parse_with_uri(source, None)
}

/// Parse the input (Atom, a flavour of RSS or JSON Feed) into our model
///
/// # Arguments
///
/// * `input` - A source of content such as a string, file etc.
/// * `uri` - Source of the content, used to resolve relative URLs in XML based feeds
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
pub fn parse_with_uri<R: Read>(source: R, uri: Option<&str>) -> ParseFeedResult<model::Feed> {
    // Buffer the reader for performance (e.g. when streaming from a network) and so we can peek to determine the type of content
    let mut input = BufReader::new(source);

    // Determine whether this is XML or JSON and call the appropriate parser
    input.fill_buf()?;
    let first_char = input.buffer().iter().find(|b| **b == b'<' || **b == b'{').map(|b| *b as char);
    let result = match first_char {
        Some('<') => parse_xml(input, uri),

        Some('{') => parse_json(input),

        _ => Err(ParseFeedError::ParseError(ParseErrorKind::NoFeedRoot)),
    };

    // Post processing as required
    if let Ok(mut feed) = result {
        assign_missing_ids(&mut feed, uri);

        Ok(feed)
    } else {
        result
    }
}

// Assigns IDs to missing feed + entries as required
fn assign_missing_ids(feed: &mut model::Feed, uri: Option<&str>) {
    if feed.id.is_empty() {
        feed.id = create_id(&feed.links, &feed.title, uri);
    }

    for entry in feed.entries.iter_mut() {
        if entry.id.is_empty() {
            entry.id = create_id(&entry.links, &entry.title, uri);
        }
    }
}

const LINK_HASH_KEY1: u64 = 0x5d78_4074_2887_2d60;
const LINK_HASH_KEY2: u64 = 0x90ee_ca4c_90a5_e228;

// Creates a unique ID from the first link, or a UUID if no links are available
fn create_id(links: &[model::Link], title: &Option<model::Text>, uri: Option<&str>) -> String {
    if let Some(link) = links.iter().next() {
        // Generate a stable ID for this item based on the first link
        let mut hasher = SipHasher::new_with_keys(LINK_HASH_KEY1, LINK_HASH_KEY2);
        hasher.write(link.href.as_bytes());
        if let Some(title) = title {
            hasher.write(title.content.as_bytes());
        }
        let hash = hasher.finish128();
        format!("{:x}{:x}", hash.h1, hash.h2)
    } else if let (Some(uri), Some(title)) = (uri, title) {
        // if no links were provided by the feed use the optional URI passed by the caller
        let mut hasher = SipHasher::new_with_keys(LINK_HASH_KEY1, LINK_HASH_KEY2);
        hasher.write(uri.as_bytes());
        hasher.write(title.content.as_bytes());
        let hash = hasher.finish128();
        format!("{:x}{:x}", hash.h1, hash.h2)
    } else {
        // Generate a UUID as last resort
        util::uuid_gen()
    }
}

// Handles JSON content
fn parse_json<R: BufRead>(source: R) -> ParseFeedResult<model::Feed> {
    json::parse(source)
}

// Handles XML content
fn parse_xml<R: BufRead>(source: R, uri: Option<&str>) -> ParseFeedResult<model::Feed> {
    // Set up the source of XML elements from the input
    let element_source = xml::ElementSource::new(source, uri)?;
    if let Ok(Some(root)) = element_source.root() {
        // Dispatch to the correct parser
        let version = root.attr_value("version");
        match (root.name.as_str(), version.as_deref()) {
            ("feed", _) => {
                element_source.set_default_default_namespace(NS::Atom);
                return atom::parse_feed(root);
            }
            ("entry", _) => {
                element_source.set_default_default_namespace(NS::Atom);
                return atom::parse_entry(root);
            }
            ("rss", Some("2.0")) => {
                element_source.set_default_default_namespace(NS::RSS);
                return rss2::parse(root);
            }
            ("rss", Some("0.91")) | ("rss", Some("0.92")) => {
                element_source.set_default_default_namespace(NS::RSS);
                return rss0::parse(root);
            }
            ("RDF", _) => {
                element_source.set_default_default_namespace(NS::RSS);
                return rss1::parse(root);
            }
            _ => {}
        };
    }

    // Couldn't find a recognised feed within the provided XML stream
    Err(ParseFeedError::ParseError(ParseErrorKind::NoFeedRoot))
}

#[cfg(test)]
mod fuzz;
