use std::io::BufRead;

use mime::Mime;

use crate::model::{Category, Content, Entry, Feed, FeedType, Generator, Image, Link, MediaObject, Person, Text};
use crate::parser::mediarss::handle_media_element;
use crate::parser::util;
use crate::parser::util::if_some_then;
use crate::parser::{mediarss, Parser};
use crate::parser::{ParseErrorKind, ParseFeedError, ParseFeedResult};
use crate::xml::{Element, NS};

#[cfg(test)]
mod tests;

/// Parses an Atom feed into our model
pub(crate) fn parse_feed<R: BufRead>(parser: &Parser, root: Element<R>) -> ParseFeedResult<Feed> {
    let mut feed = Feed::new(FeedType::Atom);

    feed.language = util::handle_language_attr(&root);

    for child in root.children() {
        let child = child?;
        match child.ns_and_tag() {
            (NS::Atom, "id") => if_some_then(child.child_as_text(), |id| feed.id = id),

            (NS::Atom, "title") => feed.title = handle_text(child)?,

            (NS::Atom, "updated") => if_some_then(child.child_as_text(), |text| feed.updated = parser.parse_timestamp(&text)),

            (NS::Atom, "author") => if_some_then(handle_person(child)?, |person| feed.authors.push(person)),

            (NS::Atom, "link") => if_some_then(handle_link(child), |link| feed.links.push(link)),

            (NS::Atom, "category") => if_some_then(handle_category(child), |category| feed.categories.push(category)),

            (NS::Atom, "contributor") => if_some_then(handle_person(child)?, |person| feed.contributors.push(person)),

            (NS::Atom, "generator") => feed.generator = handle_generator(child),

            (NS::Atom, "icon") => feed.icon = handle_image(child),

            (NS::Atom, "logo") => feed.logo = handle_image(child),

            (NS::Atom, "rights") => feed.rights = handle_text(child)?,

            (NS::Atom, "subtitle") => feed.description = handle_text(child)?,

            (NS::Atom, "entry") => if_some_then(handle_entry(parser, child)?, |entry| feed.entries.push(entry)),

            // Nothing required for unknown elements
            _ => {}
        }
    }

    Ok(feed)
}

/// Parses an Atom entry into our model
///
/// Note that the entry is wrapped in an empty Feed to keep the API consistent
pub(crate) fn parse_entry<R: BufRead>(parser: &Parser, root: Element<R>) -> ParseFeedResult<Feed> {
    let mut feed = Feed::new(FeedType::Atom);

    if_some_then(handle_entry(parser, root)?, |entry| feed.entries.push(entry));

    Ok(feed)
}

// Handles an Atom <category>
fn handle_category<R: BufRead>(element: Element<R>) -> Option<Category> {
    // Always need a term
    if let Some(term) = element.attr_value("term") {
        let mut category = Category::new(&term);

        for attr in element.attributes {
            match attr.name.as_str() {
                "scheme" => category.scheme = Some(attr.value.clone()),
                "label" => category.label = Some(attr.value.clone()),

                // Nothing required for unknown attributes
                _ => {}
            }
        }

        Some(category)
    } else {
        // A missing category isn't fatal
        None
    }
}

// Handles an Atom <content> element
fn handle_content<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<Content>> {
    // Extract the content type so we can parse the body
    let content_type = element.attr_value("type");

    // from http://www.atomenabled.org/developers/syndication/#contentElement
    match content_type.as_deref() {
        // Should be handled as a text element per "In the most common case, the type attribute is either text, html, xhtml, in which case the content element is defined identically to other text constructs"
        Some("text") | Some("html") | Some("xhtml") | Some("text/html") | None => {
            handle_text(element)?
                .map(|text| {
                    let mut content = Content::default();
                    content.body = Some(text.content);
                    content.content_type = text.content_type;
                    Some(content)
                })
                // The text is required for a text or HTML element
                .ok_or(ParseFeedError::ParseError(ParseErrorKind::MissingContent("content.text")))
        }

        // XML per "Otherwise, if the type attribute ends in +xml or /xml, then an xml document of this type is contained inline."
        Some(ct) if ct.ends_with(" +xml") || ct.ends_with("/xml") => {
            handle_text(element)?
                .map(|body| {
                    let mut content = Content::default();
                    content.body = Some(body.content);
                    content.content_type = mime::TEXT_XML;
                    Some(content)
                })
                // The XML is required for an XML content element
                .ok_or(ParseFeedError::ParseError(ParseErrorKind::MissingContent("content.xml")))
        }

        // Escaped text per "Otherwise, if the type attribute starts with text, then an escaped document of this type is contained inline." and
        // also handles base64 encoded document of the indicated mime type per "Otherwise, a base64 encoded document of the indicated media type is contained inline."
        Some(ct) => {
            if let Ok(mime) = ct.parse::<Mime>() {
                element
                    .child_as_text()
                    .map(|body| {
                        let content = Content {
                            body: Some(body),
                            content_type: mime,
                            ..Default::default()
                        };
                        Some(content)
                    })
                    // The text is required for an inline text or base64 element
                    .ok_or(ParseFeedError::ParseError(ParseErrorKind::MissingContent("content.inline")))
            } else {
                Err(ParseFeedError::ParseError(ParseErrorKind::UnknownMimeType(ct.into())))
            }
        }
    }
}

// Handles an Atom <entry>
fn handle_entry<R: BufRead>(parser: &Parser, element: Element<R>) -> ParseFeedResult<Option<Entry>> {
    // Create a default MediaRSS content object for non-grouped elements
    let mut media_obj = MediaObject::default();

    // Parse the entry
    let mut entry = Entry::default();
    for child in element.children() {
        let child = child?;
        match child.ns_and_tag() {
            // Extract the fields from the spec
            (NS::Atom, "id") => if_some_then(child.child_as_text(), |id| entry.id = id),

            (NS::Atom, "title") => entry.title = handle_text(child)?,

            (NS::Atom, "updated") => if_some_then(child.child_as_text(), |text| entry.updated = parser.parse_timestamp(&text)),

            (NS::Atom, "author") => if_some_then(handle_person(child)?, |person| entry.authors.push(person)),

            (NS::Atom, "content") => {
                entry.base = util::handle_base_attr(&child);
                entry.language = util::handle_language_attr(&child);
                entry.content = handle_content(child)?;
            }

            (NS::Atom, "link") => if_some_then(handle_link(child), |link| entry.links.push(link)),

            (NS::Atom, "summary") => entry.summary = handle_text(child)?,

            (NS::Atom, "category") => if_some_then(handle_category(child), |category| entry.categories.push(category)),

            (NS::Atom, "contributor") => if_some_then(handle_person(child)?, |person| entry.contributors.push(person)),

            // Some feeds have "pubDate" instead of "published"
            (NS::Atom, "published") | (NS::Atom, "pubDate") => if_some_then(child.child_as_text(), |text| entry.published = parser.parse_timestamp(&text)),

            (NS::Atom, "rights") => entry.rights = handle_text(child)?,

            // MediaRSS group creates a new object for this group of elements
            (NS::MediaRSS, "group") => if_some_then(mediarss::handle_media_group(child)?, |obj| entry.media.push(obj)),

            // MediaRSS tags that are not grouped are parsed into the default object
            (NS::MediaRSS, _) => handle_media_element(child, &mut media_obj)?,

            // Nothing required for unknown elements
            _ => {}
        }
    }

    // If a media:content or media:thumbnail item was found in this entry, then attach it
    if !media_obj.content.is_empty() || !media_obj.thumbnails.is_empty() {
        entry.media.push(media_obj);
    }

    Ok(Some(entry))
}

// Handles an Atom <generator>
fn handle_generator<R: BufRead>(element: Element<R>) -> Option<Generator> {
    element.child_as_text().map(|content| {
        let mut generator = Generator::new(&content);

        for attr in element.attributes {
            match attr.name.as_str() {
                "uri" => generator.uri = Some(attr.value.clone()),
                "version" => generator.version = Some(attr.value.clone()),
                // Nothing required for unknown attributes
                _ => {}
            }
        }

        generator
    })
}

// Handles an Atom <icon> or <logo>
fn handle_image<R: BufRead>(element: Element<R>) -> Option<Image> {
    element
        .child_as_text()
        .map(|raw_uri| {
            util::parse_uri(&raw_uri, element.xml_base.as_ref())
                .map(|parsed| parsed.to_string())
                .unwrap_or(raw_uri)
        })
        .map(Image::new)
}

// Handles an Atom <link>
pub(crate) fn handle_link<R: BufRead>(element: Element<R>) -> Option<Link> {
    // Always need an href
    element.attr_value("href").map(|href| {
        let mut link = Link::new(href, element.xml_base.as_ref());

        for attr in element.attributes {
            match attr.name.as_str() {
                "rel" => link.rel = Some(attr.value.clone()),
                "type" => link.media_type = Some(attr.value.clone()),
                "hreflang" => link.href_lang = Some(attr.value.clone()),
                "title" => link.title = Some(attr.value.clone()),
                "length" => link.length = attr.value.parse::<u64>().ok(),

                // Nothing required for unrecognised attributes
                _ => {}
            }
        }

        // Default "rel" to "alternate" if not set
        if link.rel.is_none() {
            link.rel = Some(String::from("alternate"));
        }

        link
    })
}

// Handles an Atom <author> or <contributor>
fn handle_person<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<Person>> {
    let mut person = Person::new("unknown");

    for child in element.children() {
        let child = child?;
        let tag_name = child.name.as_str();
        let child_text = child.child_as_text();
        match (tag_name, child_text) {
            // Extract the fields from the spec
            ("name", Some(name)) => person.name = name,
            ("uri", uri) => person.uri = uri,
            ("email", email) => person.email = email,

            // Nothing required for unknown elements
            _ => {}
        }
    }

    Ok(Some(person))
}

// Directly handles an Atom <title>, <summary>, <rights> or <subtitle> element
pub(crate) fn handle_text<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<Text>> {
    // Find type, defaulting to "text" if not present
    let type_attr = element.attributes.iter().find(|a| &a.name == "type").map_or("text", |a| a.value.as_str());

    let mime = match type_attr {
        "text" => Ok(mime::TEXT_PLAIN),
        "html" | "xhtml" | "text/html" => Ok(mime::TEXT_HTML),

        // Unknown content type
        _ => Err(ParseFeedError::ParseError(ParseErrorKind::UnknownMimeType(type_attr.into()))),
    }?;

    element
        .children_as_string()?
        .map(|content| {
            let mut text = Text::new(content);
            text.content_type = mime;
            Some(text)
        })
        // Need the text for a text element
        .ok_or(ParseFeedError::ParseError(ParseErrorKind::MissingContent("text")))
}
