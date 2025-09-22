use std::io::BufRead;

use mime::Mime;

use crate::model::{Category, Content, Entry, Feed, FeedType, Generator, Image, Link, MediaContent, MediaObject, Person};
use crate::parser::itunes::{handle_itunes_channel_element, handle_itunes_item_element};
use crate::parser::mediarss::handle_media_element;
use crate::parser::util::{if_ok_then_some, if_some_then};
use crate::parser::{atom, Parser};
use crate::parser::{mediarss, util};
use crate::parser::{ParseErrorKind, ParseFeedError, ParseFeedResult};
use crate::xml::{Element, NS};

#[cfg(test)]
mod tests;

/// Parses an RSS 2.0 feed into our model
pub(crate) fn parse<R: BufRead>(parser: &Parser, root: Element<R>) -> ParseFeedResult<Feed> {
    // Only expecting a channel element
    let found_channel = root.children().find(|result| match result {
        Ok(element) => &element.name == "channel",
        Err(_) => true,
    });
    if let Some(channel) = found_channel {
        handle_channel(parser, channel?)
    } else {
        Err(ParseFeedError::ParseError(ParseErrorKind::NoFeedRoot))
    }
}

// Handles the <channel> element
fn handle_channel<R: BufRead>(parser: &Parser, channel: Element<R>) -> ParseFeedResult<Feed> {
    let mut feed = Feed::new(FeedType::RSS2);

    for child in channel.children() {
        let child = child?;
        match child.ns_and_tag() {
            (NS::RSS, "title") => feed.title = util::handle_text(child),

            (NS::RSS, "link") => if_some_then(util::handle_link(child), |link| feed.links.push(link)),

            (NS::Atom, "link") => if_some_then(atom::handle_link(child), |link| feed.links.push(link)),

            (NS::RSS, "description") => feed.description = util::handle_text(child),

            (NS::RSS, "language") => feed.language = child.child_as_text().map(|text| text.to_lowercase()),

            (NS::RSS, "copyright") => feed.rights = util::handle_text(child),

            (NS::RSS, "managingEditor") => if_some_then(handle_contact("managingEditor", child), |person| feed.contributors.push(person)),

            (NS::RSS, "webMaster") => if_some_then(handle_contact("webMaster", child), |person| feed.contributors.push(person)),

            (NS::RSS, "pubDate") => feed.published = util::handle_timestamp(parser, child),

            // Some feeds have "updated" instead of "lastBuildDate"
            (NS::RSS, "lastBuildDate") | (NS::RSS, "updated") => feed.updated = util::handle_timestamp(parser, child),

            (NS::RSS, "category") => if_some_then(handle_category(child), |category| feed.categories.push(category)),

            (NS::RSS, "generator") => feed.generator = handle_generator(child),

            (NS::RSS, "ttl") => if_some_then(child.child_as_text(), |text| if_ok_then_some(text.parse::<u32>(), |ttl| feed.ttl = ttl)),

            (NS::RSS, "image") => feed.logo = handle_image(child)?,

            (NS::RSS, "item") => if_some_then(handle_item(parser, child)?, |item| feed.entries.push(item)),

            (NS::Itunes, _) => handle_itunes_channel_element(child, &mut feed)?,

            // Nothing required for unknown elements
            _ => {}
        }
    }

    // RSS 2.0 defines <lastBuildDate> on an item as optional so for completeness we set them to the updated date of the feed
    for entry in feed.entries.iter_mut() {
        entry.updated = feed.updated;
    }

    Ok(feed)
}

// Handles <category>
fn handle_category<R: BufRead>(element: Element<R>) -> Option<Category> {
    element.children_as_string().ok().flatten().map(|text| {
        let mut category = Category::new(&text);
        category.scheme = element.attr_value("domain");
        category
    })
}

// Handles <managingEditor> and <webMaster>
fn handle_contact<R: BufRead>(role: &str, element: Element<R>) -> Option<Person> {
    element.child_as_text().map(|email| {
        let mut person = Person::new(role);
        person.email = Some(email);
        person
    })
}

fn handle_generator<R: BufRead>(element: Element<R>) -> Option<Generator> {
    element.child_as_text().map(|c| {
        let mut generator = Generator::new(&c);

        for attr in element.attributes {
            let tag_name = attr.name.as_str();
            if tag_name == "uri" {
                generator.uri = Some(attr.value.clone());
            }
        }

        generator
    })
}

// Handles <enclosure>
fn handle_enclosure<R: BufRead>(element: Element<R>, media_obj: &mut MediaObject) {
    let mut content = MediaContent::new();

    for attr in &element.attributes {
        let tag_name = attr.name.as_str();
        match tag_name {
            "url" => content.url = util::parse_uri(&attr.value, element.xml_base.as_ref()),
            "length" => content.size = attr.value.parse::<u64>().ok(),
            "type" => if_ok_then_some(attr.value.parse::<Mime>(), |mime| content.content_type = mime),

            // Nothing required for unknown elements
            _ => {}
        }
    }

    // Wrap in a media object if we have a sufficient definition of a media object
    if content.url.is_some() {
        media_obj.content.push(content);
    }
}

// Handles <image>
fn handle_image<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<Image>> {
    let mut image = Image::new("".to_owned());

    for child in element.children() {
        let child = child?;
        match child.ns_and_tag() {
            (NS::RSS, "url") => if_some_then(child.child_as_text(), |url| image.uri = url),

            (NS::RSS, "title") => image.title = child.child_as_text(),

            (NS::RSS, "link") => if_some_then(child.child_as_text(), |uri| image.link = Some(Link::new(uri, element.xml_base.as_ref()))),

            (NS::RSS, "width") => if_some_then(child.child_as_text(), |width| {
                if let Ok(width) = width.parse::<u32>() {
                    if width > 0 && width <= 144 {
                        image.width = Some(width)
                    }
                }
            }),

            (NS::RSS, "height") => if_some_then(child.child_as_text(), |height| {
                if let Ok(height) = height.parse::<u32>() {
                    if height > 0 && height <= 400 {
                        image.height = Some(height)
                    }
                }
            }),

            (NS::RSS, "description") => image.description = child.child_as_text(),

            // Nothing required for unknown elements
            _ => {}
        }
    }

    // If we don't have a URI there is no point returning an image
    Ok(if !image.uri.is_empty() { Some(image) } else { None })
}

// Handles <content:encoded>
fn handle_content_encoded<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<Content>> {
    let src = element.xml_base.as_ref().map(|xml_base| Link::new(xml_base, element.xml_base.as_ref()));

    Ok(element.children_as_string()?.and_then(|string| {
        if string.is_empty() {
            None
        } else {
            Some(Content {
                body: Some(string),
                content_type: mime::TEXT_HTML,
                src,
                ..Default::default()
            })
        }
    }))
}

// Handles <item>
//
// There is some complexity around "enclosure", "content:encoded", MediaRSS and Itunes support
// * "enclosure": the RSS spec states that <enclosure> "Describes a media object that is attached to the item." - https://validator.w3.org/feed/docs/rss2.html#ltenclosuregtSubelementOfLtitemgt
// * "content:encoded": RSS best practices state <content:encoded> "...defines the full content of an item (OPTIONAL). This element has a more precise purpose than the description element, which can be the full content, a summary or some other form of excerpt at the publisher's discretion." - https://www.rssboard.org/rss-profile#namespace-elements-content-encoded
// * The MediaRSS and Itunes namespaces define media objects or attributes of items in the feed
//
// Handling is as follows:
// * "enclosure" is treated as if it was a MediaRSS MediaContent element and wrapped in a MediaObject
// * "content:encoded" is mapped to the content field of an Entry
// * MediaRSS elements without a parent group are added to a default MediaObject
// * Itunes elements are added to the default MediaObject
fn handle_item<R: BufRead>(parser: &Parser, element: Element<R>) -> ParseFeedResult<Option<Entry>> {
    let mut entry = Entry::default();

    // Create a default media object e.g. MediaRSS elements that are not within a "<media:group>", enclosures etc
    let mut media_obj = MediaObject::default();

    for child in element.children() {
        let child = child?;
        match child.ns_and_tag() {
            (NS::RSS, "title") => entry.title = util::handle_text(child),

            (NS::RSS, "link") => if_some_then(util::handle_link(child), |link| entry.links.push(link)),

            (NS::RSS, "description") => entry.summary = util::handle_encoded(child)?,

            (NS::RSS, "author") => if_some_then(handle_contact("author", child), |person| entry.authors.push(person)),

            (NS::RSS, "category") => if_some_then(handle_category(child), |category| entry.categories.push(category)),

            (NS::RSS, "guid") => if_some_then(child.child_as_text(), |guid| entry.id = guid.trim().to_string()),

            (NS::RSS, "enclosure") => handle_enclosure(child, &mut media_obj),

            (NS::RSS, "pubDate") | (NS::DublinCore, "date") => entry.published = util::handle_timestamp(parser, child),

            (NS::Content, "encoded") => entry.content = handle_content_encoded(child)?,

            (NS::DublinCore, "creator") => if_some_then(child.children_as_string().ok().flatten(), |name| entry.authors.push(Person::new(&name))),

            // Itunes elements populate the default MediaObject
            (NS::Itunes, _) => handle_itunes_item_element(child, &mut media_obj)?,

            // MediaRSS group creates a new object for this group of elements
            (NS::MediaRSS, "group") => if_some_then(mediarss::handle_media_group(child)?, |obj| entry.media.push(obj)),

            // MediaRSS tags that are not grouped are parsed into the default object
            (NS::MediaRSS, _) => handle_media_element(child, &mut media_obj)?,

            // Nothing required for unknown elements
            _ => {}
        }
    }

    // If a media:content item with content exists, then emit it
    if media_obj.has_content() {
        entry.media.push(media_obj);
    }

    Ok(Some(entry))
}
