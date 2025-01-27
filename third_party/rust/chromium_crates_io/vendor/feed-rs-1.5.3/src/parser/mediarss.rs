use std::io::BufRead;
use std::time::Duration;

use mime::Mime;

use crate::model::{Image, MediaCommunity, MediaContent, MediaCredit, MediaObject, MediaRating, MediaText, MediaThumbnail, Text};
use crate::parser::util::{if_ok_then_some, if_some_then, parse_npt};
use crate::parser::{util, ParseErrorKind, ParseFeedError, ParseFeedResult};
use crate::xml::{Element, NS};

// TODO When an element appears at a shallow level, such as <channel> or <item>, it means that the element should be applied to every media object within its scope.
// TODO Duplicated elements appearing at deeper levels of the document tree have higher priority over other levels. For example, <media:content> level elements are favored over <item> level elements. The priority level is listed from strongest to weakest: <media:content>, <media:group>, <item>, <channel>.

/// Handles the top-level "media:group", a collection of mediarss elements.
pub(crate) fn handle_media_group<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<MediaObject>> {
    let mut media_obj = MediaObject::default();

    for child in element.children() {
        let child = child?;
        if child.ns_and_tag().0 == NS::MediaRSS {
            handle_media_element(child, &mut media_obj)?;
        }
    }

    Ok(Some(media_obj))
}

/// Process the mediarss element into the supplied media object
/// This isn't the typical pattern, but MediaRSS has a strange shape (content within group, with other elements as peers...or no group and some elements as children)
/// So this signature is used to parse into a media object from a group, or a default one created at the entry level
pub(crate) fn handle_media_element<R: BufRead>(element: Element<R>, media_obj: &mut MediaObject) -> ParseFeedResult<()> {
    // Top level elements that should be propagated down to content items
    let mut rating = None;

    match element.ns_and_tag() {
        (NS::MediaRSS, "title") => media_obj.title = handle_text(element)?,

        (NS::MediaRSS, "content") => handle_media_content(element, media_obj)?,

        (NS::MediaRSS, "thumbnail") => if_some_then(handle_media_thumbnail(element), |thumbnail| media_obj.thumbnails.push(thumbnail)),

        (NS::MediaRSS, "description") => media_obj.description = handle_text(element)?,

        (NS::MediaRSS, "community") => media_obj.community = handle_media_community(element)?,

        (NS::MediaRSS, "credit") => if_some_then(handle_media_credit(element), |credit| media_obj.credits.push(credit)),

        (NS::MediaRSS, "text") => if_some_then(handle_media_text(element), |text| media_obj.texts.push(text)),

        (NS::MediaRSS, "rating") => rating = handle_media_rating(element),

        // Nothing required for unknown elements
        _ => {}
    }

    // If we found a rating at this level it needs to be propagated down to all content items (that don't have a rating already)
    if let Some(rating) = rating {
        media_obj.content.iter_mut().for_each(|content| {
            if content.rating.is_none() {
                content.rating = Some(rating.clone());
            }
        })
    }

    Ok(())
}

// Handle "media:community"
fn handle_media_community<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<MediaCommunity>> {
    let mut community = MediaCommunity::new();

    for child in element.children() {
        let child = child?;
        match child.ns_and_tag() {
            (NS::MediaRSS, "starRating") => {
                for attr in &child.attributes {
                    match attr.name.as_str() {
                        "average" => if_ok_then_some(attr.value.parse::<f64>(), |v| community.stars_avg = v),
                        "count" => if_ok_then_some(attr.value.parse::<u64>(), |v| community.stars_count = v),
                        "min" => if_ok_then_some(attr.value.parse::<u64>(), |v| community.stars_min = v),
                        "max" => if_ok_then_some(attr.value.parse::<u64>(), |v| community.stars_max = v),

                        // Nothing required for unknown attributes
                        _ => {}
                    }
                }
            }
            (NS::MediaRSS, "statistics") => {
                for attr in &child.attributes {
                    match attr.name.as_str() {
                        "views" => if_ok_then_some(attr.value.parse::<u64>(), |v| community.stats_views = v),
                        "favorites" => if_ok_then_some(attr.value.parse::<u64>(), |v| community.stats_favorites = v),

                        // Nothing required for unknown attributes
                        _ => {}
                    }
                }
            }

            // Nothing required for unknown elements
            _ => {}
        }
    }

    Ok(Some(community))
}

// Handle the core attributes and elements from "media:content"
fn handle_media_content<R: BufRead>(element: Element<R>, media_obj: &mut MediaObject) -> ParseFeedResult<()> {
    let mut content = MediaContent::new();

    for attr in &element.attributes {
        match attr.name.as_str() {
            "url" => content.url = util::parse_uri(&attr.value, element.xml_base.as_ref()),

            "type" => if_ok_then_some(attr.value.parse::<Mime>(), |v| content.content_type = v),

            "width" => if_ok_then_some(attr.value.parse::<u32>(), |v| content.width = v),
            "height" => if_ok_then_some(attr.value.parse::<u32>(), |v| content.height = v),

            "fileSize" => if_ok_then_some(attr.value.parse::<u64>(), |v| content.size = v),

            "duration" => if_ok_then_some(attr.value.parse::<u64>(), |v| content.duration = v.map(Duration::from_secs)),

            // Nothing required for unknown attributes
            _ => {}
        }
    }

    // If we found a URL, we consider this a valid content element, and need to parse its children for content related info (e.g. rating)
    // or sub-elements from the media spec
    if content.url.is_some() {
        for child in element.children() {
            let child = child?;
            match child.ns_and_tag() {
                (NS::MediaRSS, "rating") => content.rating = handle_media_rating(child),

                // These elements are modelled as fields on the parent MediaObject, but only set if the parent field does not already have a value
                (NS::MediaRSS, "title") => {
                    if media_obj.title.is_none() {
                        media_obj.title = handle_text(child)?
                    }
                }
                (NS::MediaRSS, "description") => {
                    if media_obj.description.is_none() {
                        media_obj.description = handle_text(child)?
                    }
                }

                // These elements are accumulated in the corresponding field of the parent MediaObject
                (NS::MediaRSS, "text") => if_some_then(handle_media_text(child), |text| media_obj.texts.push(text)),
                (NS::MediaRSS, "credit") => if_some_then(handle_media_credit(child), |credit| media_obj.credits.push(credit)),

                // Other elements in the namespace are handled recursively
                (NS::MediaRSS, _) => handle_media_element(child, media_obj)?,

                // Nothing required for unknown elements
                _ => {}
            }
        }

        // Emit this parsed content
        media_obj.content.push(content);
    }

    Ok(())
}

// Handles the "media:credit" element
fn handle_media_credit<R: BufRead>(element: Element<R>) -> Option<MediaCredit> {
    element.child_as_text().map(MediaCredit::new)
}

// Handles the "media:rating" element
fn handle_media_rating<R: BufRead>(element: Element<R>) -> Option<MediaRating> {
    // Schema is "urn:simple" by default
    let scheme = element.attr_value("scheme").unwrap_or_else(|| "urn:simple".into());

    element.child_as_text().map(|rating| MediaRating::new(rating).urn(scheme.as_str()))
}

// Handles the "media:text" element
fn handle_media_text<R: BufRead>(element: Element<R>) -> Option<MediaText> {
    let mut start_time = None;
    let mut end_time = None;
    let mut mime = None;
    for attr in &element.attributes {
        match attr.name.as_str() {
            "start" => if_some_then(parse_npt(&attr.value), |npt| start_time = Some(npt)),
            "end" => if_some_then(parse_npt(&attr.value), |npt| end_time = Some(npt)),
            "type" => {
                mime = match attr.value.as_str() {
                    "plain" => Some(mime::TEXT_PLAIN),
                    "html" => Some(mime::TEXT_HTML),
                    _ => None,
                }
            }

            // Nothing required for unknown attributes
            _ => {}
        }
    }

    element.child_as_text().map(|t| {
        // Parse out the actual text of this element
        let mut text = Text::new(t);
        text.content_type = mime.map_or(mime::TEXT_PLAIN, |m| m);
        let mut media_text = MediaText::new(text);

        // Add the time boundaries if we found them
        media_text.start_time = start_time;
        media_text.end_time = end_time;

        media_text
    })
}

// Handles the "media:thumbnail" element
fn handle_media_thumbnail<R: BufRead>(element: Element<R>) -> Option<MediaThumbnail> {
    // Extract the attributes on the thumbnail element
    let mut url = None;
    let mut width = None;
    let mut height = None;
    let mut time = None;
    for attr in &element.attributes {
        match attr.name.as_str() {
            "url" => url = Some(attr.value.clone()),

            "width" => if_ok_then_some(attr.value.parse::<u32>(), |v| width = v),
            "height" => if_ok_then_some(attr.value.parse::<u32>(), |v| height = v),

            "time" => if_some_then(parse_npt(&attr.value), |npt| time = Some(npt)),

            // Nothing required for unknown attributes
            _ => {}
        }
    }

    // We need url at least to assemble the image
    if let Some(url) = url {
        let mut image = Image::new(url);
        image.width = width;
        image.height = height;

        let mut thumbnail = MediaThumbnail::new(image);
        thumbnail.time = time;

        Some(thumbnail)
    } else {
        None
    }
}

// Handles a title or description element
fn handle_text<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<Text>> {
    // Find type, defaulting to "plain" if not present
    let type_attr = element.attributes.iter().find(|a| &a.name == "type").map_or("plain", |a| a.value.as_str());

    let mime = match type_attr {
        "plain" => Ok(mime::TEXT_PLAIN),
        "html" => Ok(mime::TEXT_HTML),

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
