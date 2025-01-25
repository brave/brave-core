use std::io::BufRead;

use crate::model::{Feed, FeedType};
use crate::parser::{rss2, ParseFeedResult, Parser};
use crate::xml::Element;

#[cfg(test)]
mod tests;

/// Parses an RSS 0.9x feed into our model
pub(crate) fn parse<R: BufRead>(parser: &Parser, root: Element<R>) -> ParseFeedResult<Feed> {
    // The 0.9x models are upward compatible with 2.x so we just delegate to that parser then set the correct type
    rss2::parse(parser, root).map(|mut feed| {
        feed.feed_type = FeedType::RSS0;
        feed
    })
}
