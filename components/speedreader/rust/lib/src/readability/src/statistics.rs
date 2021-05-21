use crate::{dom, util};
use kuchiki::iter::NodeIterator;
use kuchiki::NodeRef as Handle;
use kuchiki::{ElementData, Sink};
use util::count_ignore_consecutive_whitespace;

#[derive(Default)]
pub struct ReadableFeatures {
    /// Set if the <meta og:type="article"> is set
    pub is_open_graph_article: bool,
    /// The score used in Readability.js, saturated to save time
    pub moz_score: f64,
    /// Calculated similar to moz_score, with no paragraph thresholds applied
    pub moz_score_all_sqrt: f64,
    /// The raw paragraph lengths.
    pub moz_score_all_linear: usize,
}

impl ReadableFeatures {
    #[inline]
    pub fn fully_saturated(&self) -> bool {
        self.moz_score >= *MOZ_SCORE_SATURATION
            && self.moz_score_all_sqrt >= *MOZ_SCORE_ALL_SQRT_SATURATION
            && self.moz_score_all_linear >= *MOZ_SCORE_ALL_LINEAR_SATURATION
    }
}

// Elements with ids and classes with these values are unlikely to contain
// readable content.
static UNLIKELY_CANDIDATES: &'static [&'static str; 23] = &[
    "banner",
    "combx",
    "comment",
    "community",
    "disqus",
    "extra",
    "foot",
    "header",
    "masthead",
    "menu",
    "related",
    "remark",
    "rss",
    "share",
    "shoutbox",
    "sidebar",
    "skyscraper",
    "sponsor",
    "ad-break",
    "agegate",
    "pagination",
    "pager",
    "popup",
];

// Elements with ids and classes with these values are likely to contain
// readable content.
static LIKELY_CANDIDATES: &'static [&'static str; 6] =
    &["and", "article", "body", "column", "main", "shadow"];

// Stop calculating paragraph length after this limit is reached
const TEXT_LENGTH_SATURATION: usize = 1000;

// For untagged documents, only consider paragraphs with at least 140
// characters. This roughly corresponds two English sentences.
const PARAGRAPH_LENGTH_THRESHOLD_WEBSITE: usize = 140;

lazy_static! {
    // Upper bound of all moz scores, saturated with 6 paragraphs. This is a
    // pretty large over-estimate, and gives us a moz score around 176.

    static ref MOZ_SCORE_SATURATION: f64 =
        6.0 * (TEXT_LENGTH_SATURATION as f64 - PARAGRAPH_LENGTH_THRESHOLD_WEBSITE as f64).sqrt();
    static ref MOZ_SCORE_ALL_SQRT_SATURATION: f64 = 6.0 * (TEXT_LENGTH_SATURATION as f64).sqrt();
    static ref MOZ_SCORE_ALL_LINEAR_SATURATION: usize = 6 * TEXT_LENGTH_SATURATION;
}

/// Returns the length of all text in the tree rooted at handle.
fn text_len_saturated(handle: &Handle) -> usize {
    let mut len: usize = 0;
    for contents in handle.descendants().text_nodes() {
        // TODO(keur): Undecided, but it's possible don't actually care about
        // undercounting the spaces here, and can simplify with
        // contents.borrow().chars().filter(|c| !c.is_whitespace())
        len += count_ignore_consecutive_whitespace(contents.borrow().trim().chars());
        if len > TEXT_LENGTH_SATURATION {
            return TEXT_LENGTH_SATURATION;
        }
    }
    len
}

/// Naive zero allocation iterator over an inline-CSS style string. We only use
/// this to tokenize simple cases like display: none, visibility: hidden, and
/// opacity: 0.0, so we don't need a full-blown CSS parser.
pub struct NaiveInlineCSSStyleIterator<'a>(std::str::Split<'a, char>);

impl<'a> NaiveInlineCSSStyleIterator<'a> {
    #[inline]
    pub fn new(s: &'a str) -> Self {
        NaiveInlineCSSStyleIterator(s.split(';'))
    }
}

impl<'a> Iterator for NaiveInlineCSSStyleIterator<'a> {
    type Item = (&'a str, &'a str);

    #[inline]
    fn next(&mut self) -> Option<Self::Item> {
        self.0.next().and_then(|n| {
            let mut splitter = n.splitn(2, ':');
            let property = splitter.next()?.trim();
            let value = splitter.next()?.trim();
            if !property.is_empty() && !value.is_empty() {
                Some((property, value))
            } else {
                None
            }
        })
    }

    #[inline]
    fn size_hint(&self) -> (usize, Option<usize>) {
        self.0.size_hint()
    }
}

/// Naive function to check if an element's inline-CSS makes it hidden. Not
/// intended to cover all possible cases, just the obvious ones.
#[inline]
fn is_visible(data: &ElementData) -> bool {
    if let Some(ref style) = data.attributes.borrow().get("style") {
        let iter = NaiveInlineCSSStyleIterator::new(style);
        for i in iter {
            let invisible = match i.0 {
                "display" => i.1 == "none",
                "visibility" => i.1 == "hidden",
                "opacity" => {
                    if let Some(opacity) = i.1.parse::<f32>().ok() {
                        opacity == 0.0
                    } else {
                        false
                    }
                }
                _ => false,
            };
            if invisible {
                return false;
            }
        }
    }
    true
}

/// Helper function to check if "id" and "class" attributes are present in
/// LIKELY_CANDIDATES and UNLIKELY_CANDIDATES.
fn does_match_attrs(data: &ElementData, attrs: &[&str]) -> bool {
    // The reason we don't compress this code into a loop over "id" and "class"
    // is we need to keep this fast as possible. We would need to implement our
    // own case insensitive String.contains() method for it to work with classes,
    // since String.to_lowercase() allocates memory.
    if let Some(id) = data.attributes.borrow().get("id") {
        for attr in attrs.iter() {
            if attr.eq_ignore_ascii_case(id) {
                return true;
            }
        }
    }
    if let Some(classes) = data.attributes.borrow().get("class") {
        for class in classes.split_whitespace() {
            for attr in attrs.iter() {
                if attr.eq_ignore_ascii_case(class) {
                    return true;
                }
            }
        }
    }
    false
}

/// Populates the moz_score, moz_score_all_sqrt, and moz_score_all_linear fields
/// in ReadableFeatures. All of these fields are saturated to a dom with 6
/// readable paragraphs to save time, since that is well over a reasonable
/// minimum score.
fn collect_scores(node: &Handle, paragraph_len_threshold: usize, features: &mut ReadableFeatures) {
    let mut node_to_score: Option<Handle> = None;
    if let Some(ref node_data) = node.as_element() {
        // We want to stop recursing when:
        //   (1) The node is the beginning of a list
        //   (2) The node is not visible
        //   (3) The feature vector is fully saturated.
        if Some(&local_name!("li")) == dom::get_tag_name(node)
            || !is_visible(node_data)
            || features.fully_saturated()
        {
            return;
        }
        match node_data.name.local {
            local_name!("p") | local_name!("pre") => {
                node_to_score = Some(Handle::clone(&node));
            }
            local_name!("br") => {
                if let Some(ref sibling) = node.previous_sibling() {
                    if sibling.as_text().is_some() {
                        if let Some(ref parent) = node.parent() {
                            if Some(&local_name!("div")) == dom::get_tag_name(&parent) {
                                // We matched <div>text content<br>. Arc90 converts these
                                // into <p> nodes, so let's score them too.
                                node_to_score = Some(Handle::clone(&parent));
                            }
                        }
                    }
                }
            }
            _ => (),
        }
        if let Some(ref chosen) = node_to_score {
            let chosen_data = chosen.as_element().unwrap(); // safe unwrap, see above conditions
            if !does_match_attrs(chosen_data, UNLIKELY_CANDIDATES)
                || does_match_attrs(chosen_data, LIKELY_CANDIDATES)
            {
                let len = text_len_saturated(chosen);

                // Standard Moz score in readability
                if len >= paragraph_len_threshold {
                    features.moz_score += ((len - paragraph_len_threshold) as f64).sqrt();
                    features.moz_score = features.moz_score.min(*MOZ_SCORE_SATURATION);
                }

                // Sqrt moz score with no paragraph subtraction
                features.moz_score_all_sqrt += (len as f64).sqrt();
                features.moz_score_all_sqrt = features
                    .moz_score_all_sqrt
                    .min(*MOZ_SCORE_ALL_SQRT_SATURATION);

                // Moz score with no paragraph subtraction, no sqrt
                features.moz_score_all_linear += len;
                features.moz_score_all_linear = features
                    .moz_score_all_linear
                    .min(*MOZ_SCORE_ALL_LINEAR_SATURATION);
            }
        }
        for child in node.children() {
            collect_scores(&child, paragraph_len_threshold, features);
        }
    }
}

/// Returns true if <meta property="og:type" content="article"> is in the
/// document head.
fn is_open_graph_article(head: &Handle) -> bool {
    for data in head.descendants().elements() {
        if data.name.local != local_name!("meta") {
            continue;
        }
        if let Some(ref property) = data.attributes.borrow().get(local_name!("property")) {
            if property == &"og:type" {
                if let Some(ref content) = data.attributes.borrow().get(local_name!("content")) {
                    if content.eq_ignore_ascii_case(&"article") {
                        return true;
                    }
                }
            }
        }
    }
    false
}

/// Does a quick pass over the dom and populates a feature struct. The caller can
/// use these values to determine if the page is readable without doing expensive
/// distilling.
pub fn collect_statistics(dom: &Sink) -> Option<ReadableFeatures> {
    let mut features = ReadableFeatures::default();
    let head = dom::document_head(&dom)?;
    let body = dom::document_body(&dom)?;
    features.is_open_graph_article = is_open_graph_article(&head);
    collect_scores(&body, PARAGRAPH_LENGTH_THRESHOLD_WEBSITE, &mut features);
    Some(features)
}

#[cfg(test)]
mod tests {
    use super::*;
    use assert_approx_eq::assert_approx_eq;

    use html5ever::parse_document;
    use html5ever::tendril::TendrilSink;
    use std::io::{Cursor, Read};

    fn collect_statistics_for_test<R>(input: &mut R) -> Result<ReadableFeatures, std::io::Error>
    where
        R: Read,
    {
        let dom: Sink = parse_document(Sink::default(), Default::default())
            .from_utf8()
            .read_from(input)?;

        collect_statistics(&dom).ok_or(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            "Could not collect statistics",
        ))
    }

    #[test]
    fn test_is_opengraph() {
        let input = r#"
        <html>
          <head>
          <meta property="og:type" content="ArTiCLE"/>
          </head>
          <body></body>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let features = collect_statistics_for_test(&mut cursor).unwrap();
        assert!(
            features.is_open_graph_article,
            "Article content not detected as opengraph"
        );
    }

    #[test]
    fn test_dont_overcount_whitespace() {
        let input = r#"
        <html>
          <head></head>
          <body>
            <p>
                           1234567890
                              1234567890
                        1
                        2
                        3
                        4
                        5    6 7
            </p>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let features = collect_statistics_for_test(&mut cursor).unwrap();
        // 35 = 6 nl + 2 spaces + 27 digits
        assert_eq!(35, features.moz_score_all_linear);
    }

    #[test]
    fn test_dont_count_nested_list() {
        let input = r#"
        <html>
          <head></head>
          <body>
            <ul>
              <li><div><div><div><p>1234567890</p></div></div></div></li>
            </ul>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let features = collect_statistics_for_test(&mut cursor).unwrap();
        assert_eq!(0, features.moz_score_all_linear);
    }

    #[test]
    fn test_moz_score_features_unsaturated() {
        // NOTE: strlen("this text is counted") == 20
        let input = r#"
<html>
<head><meta property="og:type" content="article"/></head>
<body>
<p class="something">this text is countedthis text is counted</p>
<ul><li><p>skipped under list</p></li></ul>
<p class="something">
this text is countedthis text is counted
this text is countedthis text is counted
this text is countedthis text is counted
this text is countedthis text is counted
this text is countedthis text is counted
this text is countedthis text is counted
this text is countedthis text is counted
</p>
<div style='display:none'><p>skipped display none</p></div>
<div style='visibility:hidden'><p>skipped visibility hidden</p></div>
</body>
</html>
        "#;
        let mut cursor = Cursor::new(input);
        let features = collect_statistics_for_test(&mut cursor).unwrap();
        // 40 from first <p>, 286 = 40 * 6 from the block, 5 newlines
        // 326 = 40 + 286
        assert_eq!(326, features.moz_score_all_linear);
        // 23.2361 = sqrt(40) + sqrt(286)
        assert_approx_eq!(23.2361, features.moz_score_all_sqrt, 1e-4);
        // 2.4495 = sqrt(286 - 280)
        // Note, the first <p> is ignored since it has len < 280
        assert_approx_eq!(2.4495, features.moz_score, 1e-4);
    }

    #[test]
    fn test_moz_score_features_saturated() {
        let mut p_blob = String::new();
        for _ in 0..8 {
            let mut large_p_node = String::new();
            large_p_node.push_str("<p>");
            for _ in 0..1000 {
                large_p_node.push_str("this text is counted");
            }
            large_p_node.push_str("</p>");
            p_blob.push_str(&large_p_node);
        }
        let input = format!(
            r#"<html><body><div class="something">{}</div></body></html>"#,
            p_blob
        );
        let mut cursor = Cursor::new(input);
        let features = collect_statistics_for_test(&mut cursor).unwrap();
        assert_eq!(
            *MOZ_SCORE_ALL_LINEAR_SATURATION,
            features.moz_score_all_linear
        );
        assert_approx_eq!(
            *MOZ_SCORE_ALL_SQRT_SATURATION,
            features.moz_score_all_sqrt,
            1e-4
        );
        assert_approx_eq!(*MOZ_SCORE_SATURATION, features.moz_score, 1e-4);
    }

    #[test]
    fn test_style_iter() {
        let input = "display:none;visibility:hidden   ;      opacity:  0;";
        let mut style_iter = NaiveInlineCSSStyleIterator::new(input);
        let expected = [
            ("display", "none"),
            ("visibility", "hidden"),
            ("opacity", "0"),
        ];
        for t in expected.iter() {
            let style = style_iter
                .next()
                .expect("iterator did not consume all styles");
            assert_eq!(t.0, style.0, "inline-CSS property not equal");
            assert_eq!(t.1, style.1, "inline-CSS value not equal");
        }
        assert_eq!(style_iter.next(), None, "iterator should be exhausted");
    }
}
