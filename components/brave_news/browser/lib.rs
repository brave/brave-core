// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

extern crate feed_rs;

use feed_rs::parser;
use lazy_static::lazy_static;
use regex::Regex;

#[allow(unsafe_op_in_unsafe_fn)]
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
        fn strip_html(subject: &str) -> String;
        fn parse_feed_bytes(source: &[u8], output: &mut FeedData) -> bool;
    }
}

// Note: This function isn't likely to be perfect, but it does guarantee that
// there will be no HTML tags in the output (as it strips both '<' and '>' from
// from the output).
fn strip_html(subject: &str) -> String {
    let mut output = String::with_capacity(subject.len());
    let mut depth_tag = 0;
    let mut depth_comment = 0;

    // We want to know the last 3 characters so we can see if they were --> for
    // closing a comment.
    let last_chars_size = 3;
    let mut last_chars = String::with_capacity(last_chars_size);
    let mut iterator = subject.chars();

    while let Some(c) = iterator.next() {
        // Only store the last characters if they're one of the ones we're
        // interested in.
        if c == '-' || c == '>' {
            last_chars.push(c);
            if last_chars.len() > last_chars_size {
                last_chars = last_chars.chars().skip(1).collect();
            }
        } else {
            last_chars.clear();
        }

        if c == '<' {
            let is_comment = iterator.as_str().get(..3).map_or(false, |s| s == "!--");
            if is_comment {
                depth_comment += 1;
            } else if depth_comment == 0 {
                depth_tag += 1;
            }
            continue;
        }

        if c == '>' {
            if depth_comment > 0 {
                if last_chars == "-->" {
                    // If this was the close for a comment, reduce our comment depth.
                    depth_comment -= 1
                }
            } else if depth_tag > 0 {
                // Only reduce our tag depth if we aren't in a comment
                depth_tag -= 1;
            }
            continue;
        }

        if depth_comment > 0 || depth_tag > 0 {
            continue;
        }

        output.push(c);
    }

    output
}

fn parse_feed_bytes(source: &[u8], output: &mut ffi::FeedData) -> bool {
    lazy_static! {
        static ref IMAGE_REGEX: Regex = Regex::new("<img[^>]+src=\"?([^\" >]+)").unwrap();
    }

    // TODO(petemill): To see these errors, put `env_logger::init()` in an
    // 'init' function which only gets called once.
    let feed_result = parser::parse(source);
    if let Err(error) = feed_result {
        log::debug!("Could not parse feed: {}", error);
        return false;
    }
    // Parsing was successful, convert to FeedData
    let feed = feed_result.unwrap();
    output.title =
        if let Some(title) = feed.title { strip_html(&title.content) } else { String::new() };
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
            title: strip_html(
                &(if feed_item_data.title.is_some() {
                    feed_item_data.title.unwrap().content
                } else {
                    summary.clone()
                }),
            ),
            description: strip_html(&summary),
            image_url,
            destination_url: feed_item_data.links[0].href.clone(),
            published_timestamp: feed_item_data.published.map_or(0, |date| date.timestamp()),
        };
        output.items.push(feed_item);
    }
    true
}
