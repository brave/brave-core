use std::io::Read;

use mime::Mime;

use crate::model::{Category, Content, Entry, Feed, FeedType, Image, Link, Person, Text};
use crate::parser::util::if_some_then;
use crate::parser::{ParseFeedError, ParseFeedResult, Parser};

#[cfg(test)]
mod tests;

/// Parses a JSON feed into our model
pub(crate) fn parse<R: Read>(parser: &Parser, stream: R) -> ParseFeedResult<Feed> {
    let parsed = serde_json::from_reader(stream);
    if let Ok(json_feed) = parsed {
        convert(parser, json_feed)
    } else {
        // Unable to parse the JSON
        Err(ParseFeedError::JsonSerde(parsed.err().unwrap()))
    }
}

// Convert the JSON Feed into our standard model
fn convert(parser: &Parser, jf: JsonFeed) -> ParseFeedResult<Feed> {
    let mut feed = Feed::new(FeedType::JSON);

    // If the version exists, it should be something we support
    if !jf.version.starts_with("https://jsonfeed.org/version/1") {
        return ParseFeedResult::Err(ParseFeedError::JsonUnsupportedVersion(jf.version));
    }

    // Convert feed level fields
    feed.title = Some(Text::new(jf.title));

    if_some_then(jf.home_page_url, |uri| feed.links.push(Link::new(uri, None)));

    if_some_then(jf.feed_url, |uri| feed.links.push(Link::new(uri, None)));

    if_some_then(jf.description, |text| feed.description = Some(Text::new(text)));

    if_some_then(jf.icon, |uri| feed.logo = Some(Image::new(uri)));

    if_some_then(jf.language, |language| feed.language = Some(language));

    if_some_then(jf.favicon, |uri| feed.icon = Some(Image::new(uri)));

    handle_authors(&mut feed.authors, &jf.author, &jf.authors);

    // Convert items within the JSON feed
    jf.items.into_iter().for_each(|ji| {
        feed.entries.push(handle_item(parser, ji));
    });

    // Per the spec, any items without an author inherit the feed
    if !feed.authors.is_empty() {
        feed.entries.iter_mut().for_each(|entry| {
            if entry.authors.is_empty() {
                entry.authors.append(&mut feed.authors.clone());
            }
        });
    }

    Ok(feed)
}

fn accumulate_author(authors: &mut Vec<Person>, ja: &JsonAuthor) {
    // Only add if we haven't already seen this person
    if let Some(name) = &ja.name {
        if !authors.iter().any(|a| a.name.as_str() == name) {
            let mut person = Person::new(name);
            person.uri.clone_from(&ja.url);

            authors.push(person);
        }
    }
}

// Handles an attachment
fn handle_attachment(attachment: JsonAttachment) -> Link {
    let mut link = Link::new(&attachment.url, None);

    link.media_type = Some(attachment.mime_type);
    link.title = attachment.title;
    link.length = attachment.size_in_bytes;

    link
}

// Converts author / authors objects into our model
fn handle_authors(accumulated: &mut Vec<Person>, author: &Option<JsonAuthor>, authors: &Option<Vec<JsonAuthor>>) {
    if let Some(ja) = author {
        accumulate_author(accumulated, ja);
    }

    if let Some(jas) = authors {
        for ja in jas {
            accumulate_author(accumulated, ja);
        }
    }
}

// Handles HTML or plain text content
fn handle_content(content: Option<String>, content_type: Mime) -> Option<Content> {
    content.map(|body| Content {
        length: Some(body.as_bytes().len() as u64),
        body: Some(body.trim().into()),
        content_type,
        ..Default::default()
    })
}

// Converts a JSON feed item into our model
fn handle_item(parser: &Parser, ji: JsonItem) -> Entry {
    let mut entry = Entry {
        id: ji.id.unwrap_or("".into()),
        ..Default::default()
    };

    if_some_then(ji.url, |uri| entry.links.push(Link::new(uri, None)));

    if_some_then(ji.external_url, |uri| entry.links.push(Link::new(uri, None)));

    if_some_then(ji.title, |text| entry.title = Some(Text::new(text)));

    // Content HTML, content text and summary are mapped across to our model with the preference toward HTML and explicit summary fields
    entry.content = handle_content(ji.content_html, mime::TEXT_HTML);
    entry.summary = ji.summary.map(Text::new);
    if let Some(content_text) = handle_content(ji.content_text, mime::TEXT_PLAIN) {
        // If we don't have HTML content, use the text content as the entry content
        // otherwise, if the summary was not provided, we push the text there

        if entry.content.is_none() {
            entry.content = Some(content_text);
        } else if entry.summary.is_none() {
            entry.summary = content_text.body.map(Text::new);
        }
    }

    if_some_then(ji.date_published, |published| entry.published = parser.parse_timestamp(&published));

    if_some_then(ji.date_modified, |modified| entry.updated = parser.parse_timestamp(&modified));

    handle_authors(&mut entry.authors, &ji.author, &ji.authors);

    if_some_then(ji.tags, |tags| {
        tags.into_iter().map(|t| Category::new(&t)).for_each(|category| entry.categories.push(category))
    });

    if_some_then(ji.attachments, |attachments| {
        attachments.into_iter().map(handle_attachment).for_each(|link| entry.links.push(link))
    });

    entry
}

#[derive(Debug, Deserialize)]
struct JsonFeed {
    pub version: String,
    pub title: String,
    pub home_page_url: Option<String>,
    pub feed_url: Option<String>,
    pub language: Option<String>,
    pub description: Option<String>,
    pub icon: Option<String>,
    pub favicon: Option<String>,
    pub author: Option<JsonAuthor>,
    pub authors: Option<Vec<JsonAuthor>>,
    pub items: Vec<JsonItem>,
}

#[derive(Debug, Deserialize)]
struct JsonAttachment {
    url: String,
    mime_type: String,
    title: Option<String>,
    size_in_bytes: Option<u64>,
}

#[derive(Debug, Deserialize)]
struct JsonAuthor {
    name: Option<String>,
    url: Option<String>,
}

#[derive(Debug, Deserialize)]
struct JsonItem {
    pub id: Option<String>,
    pub url: Option<String>,
    pub external_url: Option<String>,
    pub title: Option<String>,
    pub content_html: Option<String>,
    pub content_text: Option<String>,
    pub summary: Option<String>,
    pub date_published: Option<String>,
    pub date_modified: Option<String>,
    pub author: Option<JsonAuthor>,
    pub authors: Option<Vec<JsonAuthor>>,
    pub tags: Option<Vec<String>>,
    pub attachments: Option<Vec<JsonAttachment>>,
}
