extern crate feed_rs;
use feed_rs::parser;
use lazy_static::lazy_static;
use regex::Regex;

#[cxx::bridge(namespace = brave_news)]
mod ffi {
    pub struct FeedItem {
        id: String,
        title: String,
        description: String,
        image_url: String,
        destination_url: String,
        published_timestamp: i64,
    }

    pub struct FeedData {
        id: String,
        title: String,
        items: Vec<FeedItem>,
    }

    extern "Rust" {
        fn parse_feed_string(source: String, output: &mut FeedData) -> bool;
    }
}

fn parse_feed_string(source: String, output: &mut ffi::FeedData) -> bool {
    lazy_static! {
        static ref IMAGE_REGEX: Regex = Regex::new("<img[^>]+src=\"?([^\" >]+)").unwrap();
    }
    // TODO(petemill): To see these errors, put `env_logger::init()` in an
    // 'init' function which only gets called once.
    let feed_result = parser::parse(source.as_bytes());
    if let Err(error) = feed_result {
        log::debug!("Could not parse feed: {}", error);
        return false;
    }
    // Parsing was successful, convert to FeedData
    let feed = feed_result.unwrap();
    output.title = if let Some(title) = feed.title {
        voca_rs::strip::strip_tags(&title.content)
    } else {
        String::new()
    };
    for feed_item_data in feed.entries {
        // Check we can make a valid entry
        if feed_item_data.links.is_empty()
            || feed_item_data.published.is_none()
            || (feed_item_data.title.is_none() && feed_item_data.summary.is_none())
        {
            // Ignore if we can't use this entry
            continue;
        }
        let mut image_url = String::new();
        // Attempt to get an image by looking at a couple possible fields:
        //  - media[].content[].url
        //  - media[].thumbnails[].image.uri
        for media_object in feed_item_data.media {
            let mut largest_width = 0;
            for content_item in media_object.content {
                if let Some(content_item_url) = content_item.url {
                    let this_width = content_item.width.unwrap_or(0);
                    if this_width >= largest_width {
                        image_url = content_item_url.to_string();
                        largest_width = this_width;
                    }
                }
            }
            if !image_url.is_empty() {
                // An image_url from media_object.content[] is
                // likely larger than one from media_object.thumbnails[]
                // so if we got one then let's just proceed with that one.
                continue;
            }
            largest_width = 0;
            for content_item in media_object.thumbnails {
                if !content_item.image.uri.is_empty() {
                    let this_width = content_item.image.width.unwrap_or(0);
                    if this_width >= largest_width {
                        image_url = content_item.image.uri;
                        largest_width = this_width;
                    }
                }
            }
        }

        let mut summary: String = String::new();
        if feed_item_data.summary.is_some() {
            summary = feed_item_data.summary.unwrap().content;
        } else if feed_item_data.content.is_some() {
            summary = feed_item_data.content.unwrap().body.unwrap_or_else(String::new);
        }
        // If we still don't have an image, attempt to parse an html
        // <img> element from the summary. This is not uncommon.
        if image_url.is_empty() && !summary.is_empty() {
            // This relies on the string being already html-decoded, which
            // feed-rs (so far) does.
            let optional_caps = IMAGE_REGEX.captures(&summary);
            if let Some(caps) = optional_caps {
                if let Some(capture) = caps.get(1) {
                    image_url = String::from(capture.as_str());
                }
            }
        }
        let feed_item = ffi::FeedItem {
            id: feed_item_data.id,
            title: voca_rs::strip::strip_tags(
                &(if feed_item_data.title.is_some() {
                    feed_item_data.title.unwrap().content
                } else {
                    summary.clone()
                }),
            ),
            description: voca_rs::strip::strip_tags(&summary),
            image_url,
            destination_url: feed_item_data.links[0].href.clone(),
            published_timestamp: feed_item_data.published.map_or(0, |date| date.timestamp())
        };
        output.items.push(feed_item);
    }
    true
}
