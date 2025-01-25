use crate::model::{Category, Feed, Image, MediaCredit, MediaObject, MediaRating, MediaThumbnail, Person};
use crate::parser::atom;
use crate::parser::util::{if_some_then, parse_npt};
use crate::parser::ParseFeedResult;
use crate::xml::{Element, NS};
use std::io::BufRead;
use std::time::Duration;

// Process <itunes> elements at channel level updating the Feed object as required
pub(crate) fn handle_itunes_channel_element<R: BufRead>(element: Element<R>, feed: &mut Feed) -> ParseFeedResult<()> {
    match element.ns_and_tag() {
        (NS::Itunes, "image") => if_some_then(handle_image(element), |image| {
            // Assign to feed logo if not already set
            if feed.logo.is_none() {
                feed.logo = Some(image.image);
            }
        }),

        (NS::Itunes, "category") => if_some_then(handle_category(element)?, |category| feed.categories.push(category)),

        (NS::Itunes, "explicit") => if_some_then(handle_explicit(element), |rating| {
            // Assign if not already set from media
            if feed.rating.is_none() {
                feed.rating = Some(rating);
            }
        }),

        (NS::Itunes, "author") => if_some_then(element.child_as_text(), |person| feed.authors.push(Person::new(&person))),
        (NS::Itunes, "owner") => if_some_then(handle_owner(element)?, |owner| feed.contributors.push(owner)),

        // Nothing required for unknown elements
        _ => {}
    }

    Ok(())
}

// Process <itunes> elements at item level and turn them into something that looks like MediaRSS objects.
pub(crate) fn handle_itunes_item_element<R: BufRead>(element: Element<R>, media_obj: &mut MediaObject) -> ParseFeedResult<()> {
    match element.ns_and_tag() {
        (NS::Itunes, "title") => media_obj.title = atom::handle_text(element)?,

        (NS::Itunes, "image") => if_some_then(handle_image(element), |thumbnail| media_obj.thumbnails.push(thumbnail)),

        (NS::Itunes, "duration") => if_some_then(handle_duration(element), |duration| media_obj.duration = Some(duration)),

        (NS::Itunes, "author") => if_some_then(handle_author(element), |credit| media_obj.credits.push(credit)),

        (NS::Itunes, "summary") => media_obj.description = atom::handle_text(element)?,

        // Nothing required for unknown elements
        _ => {}
    }

    Ok(())
}

// Handles <itunes:author>
fn handle_author<R: BufRead>(element: Element<R>) -> Option<MediaCredit> {
    element.child_as_text().map(MediaCredit::new)
}

// Handles <itunes:category>
fn handle_category<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<Category>> {
    Ok(if let Some(text) = element.attr_value("text") {
        // Create a new category for this level
        let mut category = Category::new(&text);

        // Add any sub-categories
        for child in element.children() {
            let child = child?;
            if child.ns_and_tag() == (NS::Itunes, "category") {
                if let Some(subcat) = handle_category(child)? {
                    category.subcategories.push(subcat);
                }
            }
        }

        Some(category)
    } else {
        None
    })
}

// Handles <itunes:duration>
fn handle_duration<R: BufRead>(element: Element<R>) -> Option<Duration> {
    element.child_as_text().and_then(|text| parse_npt(&text))
}

// Handles <itunes:explicit> by mapping to {true|false} and wrapping in MediaRating instance
fn handle_explicit<R: BufRead>(element: Element<R>) -> Option<MediaRating> {
    element
        .child_as_text()
        .filter(|v| v.to_lowercase() == "true")
        .map(|v| MediaRating::new(v).urn("itunes"))
}

// Handles <itunes:image>
fn handle_image<R: BufRead>(element: Element<R>) -> Option<MediaThumbnail> {
    element.attr_value("href").map(|url| MediaThumbnail::new(Image::new(url)))
}

// Handles <itunes:owner>
fn handle_owner<R: BufRead>(element: Element<R>) -> ParseFeedResult<Option<Person>> {
    let mut email = None;
    let mut name = None;

    for child in element.children() {
        let child = child?;
        match child.ns_and_tag() {
            (NS::Itunes, "email") => email = child.child_as_text(),
            (NS::Itunes, "name") => name = child.child_as_text(),

            // Nothing required for unknown elements
            _ => {}
        }
    }

    Ok(if let (Some(email), Some(name)) = (email, name) {
        Some(Person::new(&name).email(&email))
    } else {
        None
    })
}
