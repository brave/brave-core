use std::io::BufRead;

use crate::model::{Content, Entry, Feed, FeedType, Image, Link, Person, Text};
use crate::parser::util::if_some_then;
use crate::parser::{util, ParseFeedResult, Parser};
use crate::xml::{Element, NS};

#[cfg(test)]
mod tests;

/// Parses an RSS 1.0 feed into our model
pub(crate) fn parse<R: BufRead>(parser: &Parser, root: Element<R>) -> ParseFeedResult<Feed> {
    let mut feed = Feed::new(FeedType::RSS1);

    for child in root.children() {
        let child = child?;
        match child.ns_and_tag() {
            (NS::RSS, "channel") => handle_channel(parser, &mut feed, child)?,

            (NS::RSS, "image") => feed.logo = handle_image(child)?,

            (NS::RSS, "item") => if_some_then(handle_item(parser, child)?, |entry| feed.entries.push(entry)),

            // Nothing required for unknown elements
            _ => {}
        }
    }

    Ok(feed)
}

// Handles the <channel> element
fn handle_channel<R: BufRead>(parser: &Parser, feed: &mut Feed, channel: Element<R>) -> ParseFeedResult<()> {
    for child in channel.children() {
        let child = child?;
        match child.ns_and_tag() {
            (NS::RSS, "title") => feed.title = util::handle_text(child),

            (NS::RSS, "link") => if_some_then(util::handle_link(child), |link| feed.links.push(link)),

            (NS::RSS, "description") => feed.description = util::handle_text(child),

            (NS::DublinCore, "creator") => if_some_then(child.child_as_text(), |name| feed.authors.push(Person::new(&name))),

            (NS::DublinCore, "date") => feed.published = util::handle_timestamp(parser, child),

            (NS::DublinCore, "language") => feed.language = child.child_as_text(),

            (NS::DublinCore, "rights") => feed.rights = util::handle_text(child),

            // Nothing required for unknown elements
            _ => {}
        }
    }

    Ok(())
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

            // Nothing required for unknown elements
            _ => {}
        }
    }

    // If we don't have a URI there is no point returning an image
    Ok(if !image.uri.is_empty() { Some(image) } else { None })
}

// Handles <item>
fn handle_item<R: BufRead>(parser: &Parser, element: Element<R>) -> ParseFeedResult<Option<Entry>> {
    let mut entry = Entry::default();

    // Per https://www.w3.org/wiki/RssContent:
    //   How to encode content in a RSS 1.0 feed is an unsolved problem, many persons have made different problems and there's no consensus for a definitive solution.
    // But we see it in real feeds, so might as well add it in the same manner as RSS2.0 best practice
    let mut content_encoded: Option<Text> = None;

    for child in element.children() {
        let child = child?;
        match child.ns_and_tag() {
            (NS::RSS, "title") => entry.title = util::handle_text(child),

            (NS::RSS, "link") => if_some_then(util::handle_link(child), |link| entry.links.push(link)),

            (NS::RSS, "description") => entry.summary = util::handle_text(child),

            (NS::Content, "encoded") => content_encoded = util::handle_encoded(child)?,

            (NS::DublinCore, "creator") => if_some_then(child.child_as_text(), |name| entry.authors.push(Person::new(&name))),

            (NS::DublinCore, "date") => entry.published = util::handle_timestamp(parser, child),

            (NS::DublinCore, "description") => {
                if entry.summary.is_none() {
                    entry.summary = util::handle_text(child)
                }
            }

            (NS::DublinCore, "rights") => entry.rights = util::handle_text(child),

            // Nothing required for unknown elements
            _ => {}
        }
    }

    // Use content_encoded if we didn't find an enclosure above
    if entry.content.is_none() {
        if let Some(ce) = content_encoded {
            entry.content = Some(Content {
                body: Some(ce.content),
                content_type: ce.content_type,
                length: None,
                src: ce.src.map(|s| Link::new(s, element.xml_base.as_ref())),
            });
        }
    }

    // If we found at least 1 link
    Ok(if !entry.links.is_empty() {
        Some(entry)
    } else {
        // No point returning anything if we are missing a destination
        None
    })
}
