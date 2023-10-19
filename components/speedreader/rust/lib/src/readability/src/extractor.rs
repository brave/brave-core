/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use crate::{dom, nlp, scorer, util};
use html5ever::parse_document;
use html5ever::tendril::TendrilSink;
use html5ever::tree_builder::{ElementFlags, NodeOrText, TreeSink};
use html5ever::QualName;
use kuchiki::NodeRef as Handle;
use kuchiki::Sink;
use regex::Regex;
use std::collections::{HashMap, HashSet};
use std::default::Default;
use std::io::Read;
use thiserror::Error;
use time::format_description::well_known::Rfc3339;
use time::macros::format_description;
use time::OffsetDateTime;
use url::Url;
use util::StringUtils;

static JSONLD_ARTICLE_TYPES: [&str; 19] = [
    "Article",
    "AdvertiserContentArticle",
    "NewsArticle",
    "AnalysisNewsArticle",
    "AskPublicNewsArticle",
    "BackgroundNewsArticle",
    "OpinionNewsArticle",
    "ReportageNewsArticle",
    "ReviewNewsArticle",
    "Report",
    "SatiricalArticle",
    "ScholarlyArticle",
    "MedicalScholarlyArticle",
    "SocialMediaPosting",
    "BlogPosting",
    "LiveBlogPosting",
    "DiscussionForumPosting",
    "TechArticle",
    "APIReference",
];

lazy_static! {
    static ref SEPARATORS: Regex = Regex::new(r#"\s+[\|\\/>»]\s+"#).unwrap();
    static ref END_DASH: Regex = Regex::new(r#"\s+(:?[—\-–])\s+.*$"#).unwrap();
    static ref JSONLD_SCHEMA: Regex = Regex::new(r#"^https?://schema\.org[/?\w/?]*$"#).unwrap();
}

static SHOW_ORIGINAL_DIV_ID: &str = "c93e2206-2f31-4ddc-9828-2bb8e8ed940e";
static READ_TIME_DIV_ID: &str = "da24e4ef-db57-4b9f-9fa5-548924fc9c32";
static META_DATA_AREA_DIV_ID: &str = "3bafd2b4-a87d-4471-8134-7a9cca092000";
static MAIN_CONTENT_DIV_ID: &str = "7c08a417-bf02-4241-a55e-ad5b8dc88f69";

#[derive(Debug)]
pub struct Product {
    pub meta: Meta,
    pub content: String,
}

// NOTE: Only used in document tests, but exposed publicly for callers to test
// the feature extractor.
pub fn extract<R>(input: &mut R, url: Option<&str>) -> Result<Product, std::io::Error>
where
    R: Read,
{
    let url = url
        .and_then(|url| Url::parse(url).ok())
        .unwrap_or_else(|| Url::parse("https://example.com").unwrap());

    let mut dom: Sink =
        parse_document(Sink::default(), Default::default()).from_utf8().read_from(input)?;

    extract_dom(&mut dom, &url, None, None, None, None, None, &HashMap::new())
}

#[derive(Default, Debug)]
pub struct Meta {
    pub title: String,
    pub author: Option<String>,
    pub description: Option<String>,
    pub charset: Option<String>,
    pub last_modified: Option<OffsetDateTime>,
}

impl Meta {
    /// Performs a merge of two meta structs, preferencing self except
    /// description. The shortest description will be taken.
    /// Takes ownership of both structs and returns the merged metadata.
    pub fn merge(mut self, other: Self) -> Meta {
        if self.title.is_empty() {
            self.title = other.title;
        }
        self.author = self.author.or(other.author);
        self.description = match (self.description, other.description) {
            (None, None) => None,
            (Some(x), None) | (None, Some(x)) => Some(x),
            (Some(x), Some(y)) if x.len() < y.len() => Some(x),
            (Some(_), Some(y)) => Some(y),
        };
        self.charset = self.charset.or(other.charset);
        self.last_modified = self.last_modified.or(other.last_modified);
        self
    }
}

/// This function searches the DOM for <meta> tags and JSON-LD data.
/// It looks for the title, author, time modified, and charset
/// of the article.
/// The preference of data sources is as follows:
///     (1) JSON-LD
///     (2) <meta> tags
///     (3) <title> tag
pub fn extract_metadata(dom: &Sink) -> Meta {
    let meta_nodes = dom.document_node.descendants().filter(|d| {
        d.as_element()
            .map(|e| match e.name.local {
                local_name!("script") => {
                    e.attributes.borrow().get(local_name!("type")) == Some("application/ld+json")
                }
                local_name!("meta") | local_name!("title") => true,
                _ => false,
            })
            .unwrap_or(false)
    });

    let mut meta_jsonld = Meta::default();
    for node in meta_nodes.clone().into_iter() {
        // NOTE: This unwrap is safe because the iterator only contains element types
        let data = node.as_element().unwrap();

        if data.name.local == local_name!("script")
            && data.attributes.borrow().get("type") == Some("application/ld+json")
        {
            let mut blob = String::new();
            dom::extract_text(&node, &mut blob, false);
            if try_parse_untyped_jsonld(&blob, &mut meta_jsonld).is_ok() {
                break;
            }
        }
    }

    let mut meta_tags = Meta::default();
    for node in meta_nodes.clone().into_iter() {
        // NOTE: This unwrap is safe because the iterator only contains element types
        let data = node.as_element().unwrap();

        if data.name.local != local_name!("meta") {
            continue;
        }
        let attribute = data.attributes.borrow();
        if let Some(property) =
            attribute.get(local_name!("property")).or(attribute.get(local_name!("name")))
        {
            if let Some(ref content) = attribute.get(local_name!("content")) {
                match property {
                    "dc:title"
                    | "dcterm:title"
                    | "og:title"
                    | "weibo:article:title"
                    | "weibo:webpage:title"
                    | "title"
                    | "twitter:title" => {
                        meta_tags.title = content.to_string();
                    }
                    "description"
                    | "dc:description"
                    | "dcterm:description"
                    | "og:description"
                    | "weibo:article:description"
                    | "weibo:webpage:description"
                    | "twitter:description" => {
                        if let Some(ref desc) = meta_tags.description {
                            if content.chars().count() < desc.chars().count() {
                                meta_tags.description = Some(content.to_string());
                            }
                        } else {
                            meta_tags.description = Some(content.to_string());
                        }
                    }
                    "dc:creator" | "dcterm:creator" | "author" => {
                        meta_tags.author = Some(content.to_string());
                    }
                    _ => (),
                }
            }
        } else if let Some(charset) = attribute.get(local_name!("charset")) {
            meta_tags.charset = Some(charset.to_string());
        } else if attribute
            .get(local_name!("http-equiv"))
            .map(|e| e.to_ascii_lowercase() == "content-type")
            .unwrap_or(false)
        {
            if let Some(content) = attribute.get(local_name!("content")) {
                if let Some(charset) = content.split("charset=").nth(1) {
                    meta_tags.charset = Some(charset.trim().to_string());
                }
            }
        }
    }

    let mut meta = meta_jsonld.merge(meta_tags);

    if meta.title.is_empty() {
        if let Some(handle) = meta_nodes
            .into_iter()
            .find(|d| d.as_element().unwrap().name.local == local_name!("title"))
        {
            dom::extract_text(&handle, &mut meta.title, true);
        }
    }

    // HTML decode title, author, and description
    if !meta.title.is_empty() {
        meta.title = dom::html_decode(&meta.title).unwrap_or(meta.title);
    }
    if let Some(ref author) = meta.author {
        meta.author = dom::html_decode(author).or(meta.author);
    }
    if let Some(ref description) = meta.description {
        meta.description = dom::html_decode(description).or(meta.description);
    }

    meta
}

pub fn extract_dom<S: ::std::hash::BuildHasher>(
    mut dom: &mut Sink,
    url: &Url,
    min_out_length: Option<i32>,
    theme: Option<String>,
    font_family: Option<String>,
    font_size: Option<String>,
    column_width: Option<String>,
    features: &HashMap<String, u32, S>,
) -> Result<Product, std::io::Error> {
    let handle = dom.document_node.clone();

    let mut meta = extract_metadata(dom);
    meta.title = clean_title(dom, meta.title);

    // pre-processes the DOM by removing script tags, css, links
    scorer::preprocess(&mut dom, handle.clone());

    // Normalize DOM tags
    scorer::replace_tags(&mut dom);

    // top candidate is the top scorer among the tree dom's candidates. this is
    // the subtree that will be considered for final rendering
    let top_candidate = scorer::get_top_candidate(dom, &handle, true)?;

    // Append siblings of the new root with related content.
    scorer::append_related_siblings(&mut dom, top_candidate.clone());

    scorer::clean(
        &mut dom,
        top_candidate.clone(),
        &meta.title.split_whitespace().collect::<HashSet<_>>(),
        url,
        features,
    );

    post_process(&mut dom, top_candidate.clone(), &meta);

    if let Some(min_out_length) = min_out_length {
        if (dom::text_len(&top_candidate) as i32) < min_out_length {
            return Err(std::io::Error::new(std::io::ErrorKind::InvalidInput, "Too small output"));
        }
    }

    // Calls html5ever::serialize() with IncludeNode for us.
    let mut content: String = match top_candidate.as_element() {
        Some(x) if x.name.local != local_name!("body") => {
            let name = QualName::new(None, ns!(), local_name!("body"));
            let body = dom.create_element(name, vec![], ElementFlags::default());
            let main = dom::create_element_simple(dom, "main", "", None);
            let article = dom::create_element_simple(dom, "article", "", None);

            dom.reparent_children(&top_candidate, &article);
            dom.append(&main, NodeOrText::AppendNode(article));
            dom.append(&body, NodeOrText::AppendNode(main));

            // Our CSS formats based on id="article".
            dom::set_attr("id", "article", body.clone(), true);
            body.to_string()
        }
        _ => top_candidate.to_string(),
    };

    if let Some(ref charset) = meta.charset {
        // Since we strip out the entire head, we need to include charset if one
        // was provided. Otherwise the browser will use the default encoding,
        // and surprisingly it's not utf-8 ;)
        let charset_blob = format!("<meta charset=\"{}\"/>", charset);
        content = charset_blob + &content;
    }
    if !meta.title.is_empty() {
        let title_blob = format!("<title>{}</title>", &meta.title);
        content = title_blob + &content;
    }

    if theme.is_some() || font_family.is_some() || font_size.is_some() || column_width.is_some() {
        let mut header: String = String::from("<html");
        if let Some(theme) = theme {
            header = [header, format!(" data-theme=\"{}\"", theme)].concat();
        }
        if let Some(font_family) = font_family {
            header = [header, format!(" data-font-family=\"{}\"", font_family)].concat();
        }
        if let Some(font_size) = font_size {
            header = [header, format!(" data-font-size=\"{}\"", font_size)].concat();
        }
        if let Some(column_width) = column_width {
            header = [header, format!(" data-column-width=\"{}\"", column_width)].concat();
        }
        content = [header, ">".to_string(), content, "</html>".to_string()].concat();
    }

    Ok(Product { meta, content })
}

pub fn post_process(dom: &mut Sink, root: Handle, meta: &Meta) {
    if root.first_child().is_some() {
        let meta_area = dom::create_element_simple(dom, "div", "", None);
        dom::set_attr("id", META_DATA_AREA_DIV_ID, meta_area.clone(), true);

        // Add in the title
        if !meta.title.is_empty() {
            let title_header =
                dom::create_element_simple(dom, "h1", "title metadata", Some(&meta.title));
            dom.append(&meta_area, NodeOrText::AppendNode(title_header));
        }
        // Add in the description
        if let Some(ref text) = meta.description {
            let slice_offset = if text.chars().count() > 200 {
                nlp::first_sentence_boundary(text).unwrap_or_else(|| text.len())
            } else {
                text.len()
            };
            let description = dom::create_element_simple(
                dom,
                "p",
                "subhead metadata",
                Some(&text[..slice_offset]),
            );
            dom.append(&meta_area, NodeOrText::AppendNode(description));
        }

        // Vertical split
        if meta.author.is_some() || meta.last_modified.is_some() {
            let splitter = dom::create_element_simple(dom, "hr", "", None);
            dom.append(&meta_area, NodeOrText::AppendNode(splitter));
        }

        let metadata_parent = dom::create_element_simple(dom, "div", "metadata", None);
        dom.append(&meta_area, NodeOrText::AppendNode(metadata_parent.clone()));

        // Add in the author
        if let Some(ref text) = meta.author {
            let author =
                dom::create_element_simple(dom, "p", "author", Some(&format!("By {}", text)));
            dom.append(&metadata_parent, NodeOrText::AppendNode(author));
        }

        // Add in last modified datetime
        if let Some(ref last_modified) = meta.last_modified {
            let format = format_description!(
                "[month repr:short] [day], [year] [hour repr:12]:[minute] [period]"
            );
            if let Some(formatted) = last_modified.format(format).ok() {
                let modified = dom::create_element_simple(dom, "p", "date", Some(&formatted));
                dom.append(&metadata_parent, NodeOrText::AppendNode(modified));
            }
        }

        // Add 'read time'
        {
            let read_time = dom::create_element_simple(dom, "div", "readtime", None);
            dom::set_attr("id", READ_TIME_DIV_ID, read_time.clone(), true);
            dom.append(&metadata_parent, NodeOrText::AppendNode(read_time));
        }

        // Add 'show original'
        {
            let show_original_link = dom::create_element_simple(dom, "div", "show_original", None);
            dom::set_attr("id", SHOW_ORIGINAL_DIV_ID, show_original_link.clone(), true);
            dom.append(&metadata_parent, NodeOrText::AppendNode(show_original_link));
        }

        // Vertical split
        if !meta.title.is_empty()
            || meta.description.is_some()
            || meta.author.is_some()
            || meta.last_modified.is_some()
        {
            let splitter = dom::create_element_simple(dom, "hr", "", None);
            dom.append(&meta_area, NodeOrText::AppendNode(splitter));
        }

        let content = dom::create_element_simple(dom, "div", "", None);
        dom::set_attr("id", MAIN_CONTENT_DIV_ID, content.clone(), true);
        dom.reparent_children(&root, &content);

        dom.append(&root, NodeOrText::AppendNode(meta_area));
        dom.append(&root, NodeOrText::AppendNode(content));

        // Our CSS formats based on id="article".
        dom::set_attr("id", "article", root, true);
    }
}

pub fn clean_title(dom: &Sink, title: String) -> String {
    if let Some(m) = SEPARATORS.find(&title) {
        let mut cur_title = title.substring(0, m.start());
        if cur_title.split_whitespace().count() < 3 {
            cur_title = title.substring(m.end(), title.len());
        }
        cur_title.trim().to_string()
    } else if let Some(m) = END_DASH.find(&title) {
        let trailing_title = title.substring(m.start(), title.len());
        if trailing_title.split_whitespace().count() <= 4 {
            // We have 3 distinct words and the dash. Probably the website title. Trim it
            // off.
            title.substring(0, m.start())
        } else {
            title
        }
    } else if title.contains(": ") {
        let found_matching_heading = dom
            .document_node
            .descendants()
            .filter(|d| {
                d.as_element()
                    .map(|e| e.name.local == local_name!("h1") || e.name.local == local_name!("h2"))
                    .unwrap_or(false)
            })
            .any(|handle| {
                let mut maybe_title = String::new();
                dom::extract_text(&handle, &mut maybe_title, true);
                return maybe_title.trim() == title.trim();
            });
        if found_matching_heading {
            return title;
        }
        let mut cur_title = title.substring(title.rfind(':').unwrap() + 1, title.len());

        // Less than 3 words in the title. Try first colon.
        if cur_title.split_whitespace().count() < 3 {
            cur_title = title.substring(title.find(':').unwrap() + 1, title.len());
        } else if title.substring(0, title.find(':').unwrap_or(0)).split_whitespace().count() > 5 {
            return title;
        }
        cur_title.trim().to_string()
    } else {
        title
    }
}

#[derive(Error, Debug, PartialEq)]
pub enum JsonLdError {
    #[error("Could not parse json-ld: `{0}`")]
    ParseError(String),
    #[error("Missing @context field")]
    MissingContext,
    #[error("Invalid @context: Does not match http://schema.org")]
    InvalidContext,
    #[error("Missing @type field")]
    MissingType,
    #[error("Invalid @type")]
    InvalidType,
}

impl From<serde_json::Error> for JsonLdError {
    fn from(err: serde_json::Error) -> Self {
        JsonLdError::ParseError(err.to_string())
    }
}

#[inline(never)]
fn try_parse_author(v: &serde_json::Value) -> Option<String> {
    match v {
        serde_json::Value::String(s) => {
            // Try to parse for nested JSON. Buzzfeed includes the author field as string
            // JSON blob. I have no idea why sites do this kind of stuff...
            match serde_json::from_str(s) {
                Ok(p) => try_parse_author(&p),
                Err(_) => Some(s.to_string()), // Wasn't JSON, so we can assume it's a valid author.
            }
        }
        serde_json::Value::Array(a) => {
            if !a.is_empty() {
                Some(a.iter().filter_map(|e| try_parse_author(e)).collect::<Vec<_>>().join(", "))
            } else {
                None
            }
        }
        serde_json::Value::Object(o) => {
            o.get("name").and_then(|name| name.as_str()).map(|x| x.to_string())
        }
        _ => None,
    }
}

fn try_parse_untyped_jsonld(content: &str, meta: &mut Meta) -> Result<(), JsonLdError> {
    let v: serde_json::Value = serde_json::from_str(content)?;

    fn from_json_string(v: &serde_json::Value) -> Option<String> {
        match v {
            serde_json::Value::String(s) if !s.is_empty() => Some(s.to_string()),
            _ => None,
        }
    }

    match v {
        serde_json::Value::Object(o) => {
            // Validate @context
            o.get("@context")
                .map(|c| match c {
                    serde_json::Value::String(s) => {
                        if !JSONLD_SCHEMA.is_match(s) {
                            Err(JsonLdError::InvalidContext)
                        } else {
                            Ok(())
                        }
                    }
                    _ => Err(JsonLdError::MissingContext),
                })
                .unwrap_or(Err(JsonLdError::MissingContext))?;

            // Validate @type
            o.get("@type")
                .map(|t| match t {
                    serde_json::Value::String(s) => {
                        if JSONLD_ARTICLE_TYPES.iter().find(|&&x| x == s).is_none() {
                            Err(JsonLdError::InvalidType)
                        } else {
                            Ok(())
                        }
                    }
                    _ => Err(JsonLdError::MissingType),
                })
                .unwrap_or(Err(JsonLdError::MissingType))?;

            // Get article title
            if let Some(title) =
                o.get("name").or_else(|| o.get("headline")).and_then(from_json_string)
            {
                meta.title = title;
            }

            // Get article author
            if let Some(author) = o.get("author").and_then(try_parse_author) {
                meta.author = Some(author);
            }

            // Get article description
            if let Some(description) = o.get("description").and_then(from_json_string) {
                meta.description = Some(description);
            }

            // Get article modified date
            if let Some(timestamp) =
                o.get("dateModified").or_else(|| o.get("datePublished")).and_then(from_json_string)
            {
                meta.last_modified = OffsetDateTime::parse(&timestamp, &Rfc3339).ok();
            }
        }

        _ => (),
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    fn normalize_output(input: &str) -> String {
        return input.lines().map(|line| line.trim()).filter(|line| !line.is_empty()).collect();
    }

    fn preprocess<R>(input: &mut R) -> Result<Product, std::io::Error>
    where
        R: Read,
    {
        let mut dom: Sink =
            parse_document(Sink::default(), Default::default()).from_utf8().read_from(input)?;

        let mut meta = extract_metadata(&dom);
        meta.title = clean_title(&dom, meta.title);
        let handle = dom.document_node.clone();
        scorer::preprocess(&mut dom, handle);
        scorer::replace_tags(&mut dom);
        let content = dom.document_node.to_string();
        Ok(Product { meta, content })
    }

    #[test]
    fn test_extract_title() {
        let data = r#"
        <!DOCTYPE html>
        <html>
          <head>
            <title>This is title</title>
          </head>
        </html>
        "#;
        let mut cursor = Cursor::new(data);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(product.meta.title, "This is title");
    }

    #[test]
    fn test_title_prefer_meta() {
        let data = r#"
        <head>
        <meta property="og:title" content="Title in meta tag"/>
        <meta property="hi" content="test"/>
        <title>Title in title tag</title>
        </head>
        "#;
        let mut cursor = Cursor::new(data);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(product.meta.title, "Title in meta tag");
    }

    #[test]
    fn test_description_complex() {
        // We grab the description, HTML decode it, keep the punctuation entites, but
        // delete the <b> tag. This was found on Buzzfeed.
        let data = r#"
        <head>
        <meta property="og:description" content="&lt;b&gt;An inquest into Eloise Parry&#x27;s death has been adjourned.&lt;/b&gt;"/>
        </head>
        "#;
        let mut cursor = Cursor::new(data);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(
            product.meta.description.expect("No description extracted"),
            "An inquest into Eloise Parry's death has been adjourned."
        );
    }

    #[test]
    fn test_byline_html_decode() {
        let input = r#"
        <head>
        <meta property="author" content="Geek&#039;s Guide to the Galaxy"/>
        </head>
        "#;
        let mut cursor = Cursor::new(input);
        let meta = preprocess(&mut cursor).unwrap().meta;
        assert_eq!("Geek's Guide to the Galaxy", meta.author.expect("No author extracted"),);
    }

    #[test]
    fn unwrap_noscript_img_simple() {
        let input = r#"
        <body>
          <noscript>
            <img src="https://example.com/image.png">
          </noscript>
        </body>
        "#;
        let expected = r#"
        <html><head></head>
        <body>
          <img src="https://example.com/image.png">
        </body>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn unwrap_noscript_img_delete_preceding() {
        // Based on https://www.bbc.com/news/world-australia-56307356
        let input = r#"
        <body>
          <img src="https://example.com/image.png">
          <noscript>
            <img src="https://example.com/image.png">
          </noscript>
        </body>"#;
        let expected = r#"
        <html><head></head>
        <body>
          <img src="https://example.com/image.png">
        </body>
        </html>"#;
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn unwrap_noscript_img_delete_following() {
        // Based on https://www.dutchnews.nl/features/2021/08/the-region-revolutionising-the-dutch-diet/
        let input = r#"
        <body>
          <noscript>
            <img src="https://example.com/image.png">
          </noscript>
          <img src="https://example.com/image.png">
          <p>This is the image caption</p>
        </body>"#;
        let expected = r#"
        <html><head></head>
        <body>
          <img src="https://example.com/image.png">
          <p>This is the image caption</p>
        </body>
        </html>"#;
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn unwrap_noscript_img_nested() {
        let input = r#"
        <body>
          <img src="https://example.com/image.png">
          <noscript>
            <span><img src="https://example.com/image.png"></span>
          </noscript>
        </body>
        "#;
        let expected = r#"
        <html><head></head>
        <body>
          <img src="https://example.com/image.png">
        </body>
        </html>
        "#;

        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn rewrite_divs_single_p_child_as_p() {
        let input = r#"
        <body>
          <div>
            <p>This is paragraph one!</p>
          </div>
          <div>
            <p>This is paragraph two!!</p>
          </div>
        </body>
        "#;
        let expected = r#"
        <html><head></head>
        <body>
          <p>This is paragraph one!</p>
          <p>This is paragraph two!!</p>
        </body>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn rewrite_div_phrasing_only_as_p() {
        let input = r#"
        <body>
          <div>
            Here is some text, and <a>A link</a> too!<br> <br>
          </div>
        </body>
        "#;
        let expected = r#"
        <html><head></head>
        <body>
          <p>
            Here is some text, and <a>A link</a> too!
          </p>
        </body>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn rewrite_h1s_as_h2s() {
        let input = r#"
        <body><div>
          <p>Here is some text and some more text</p>
          <h1>A random h1 that isn't the title</h1>
          <p>Even more text</p>
        </div></body>
        "#;
        let expected = r#"
        <body id="article"><div>
          <p>Here is some text and some more text</p>
          <h2>A random h1 that isn't the title</h2>
          <p>Even more text</p>
        </div></body>
        "#;

        let mut cursor = Cursor::new(input);
        let product = extract(&mut cursor, None).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn rewrite_font_as_span() {
        // <font> nodes whose children are all phrasing content are converted to <span>.
        let input = r#"
        <body>
          <font color="\#aaaaaa">Some inline content</font>
          <font color="\#bbbbbb"><a>Some inline link</a></font>
        </body>
        "#;
        let expected = r#"
        <html><head></head><body>
          <span>Some inline content</span>
          <span><a>Some inline link</a></span>
        </body></html>
        "#;

        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn rewrite_font_as_div() {
        // Based off of http://paulgraham.com/newideas.html. It makes heavy use
        // of high text density inside of <font> tags. We handle this by first
        // converting the <br> chains to <p> nodes in the preprocess pass. Then
        // we convert the <font> to a <div> since the <p> nodes are not phrasing
        // content.
        let input = r#"
        <body>
          <font color="\#aaaaaa">
          <br><br>Lots and lots of text goes here
          <br><br>And some more text goes here
          </font>
        </body>
        "#;
        let expected = r#"
        <html><head></head><body>
          <div>
            <p>Lots and lots of text goes here</p>
            <p>And some more text goes here</p>
          </div>
        </body></html>
        "#;

        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn br_chain_to_p_simple() {
        let input = r#"
        <body>
        foo<br>bar<br> <br><br>abc
        </body>
        "#;
        let expected = r#"
        <html><head></head>
        <body>
        foo<br>bar<p>abc</p>
        </body>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content),);
    }

    #[test]
    fn br_chain_to_p_include_phrasing_elements() {
        let input = r#"
        </body>
        <br><br>Some super<a href="https://baz.com">cool website</a> and more text.
        </body>
        "#;
        let expected = r#"
        <html><head></head>
        <body>
        <p>Some super<a href="https://baz.com">cool website</a> and more text.</p>
        </body>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content),);
    }

    #[test]
    fn preserve_spaces() {
        let input = r#"
        <body>
          <p>
            <strong>
              <a href="example.com/example.png">Some Link</a>
              &nbsp;
            </strong>
            this text should have a space between the link.
          </p>
        </body>
        "#;
        let expected = r#"
        <body id="article">
          <p>
            <strong>
              <a href="example.com/example.png">Some Link</a>
              &nbsp;
            </strong>
            this text should have a space between the link.
          </p>
        </body>
        "#;
        let mut cursor = Cursor::new(input);
        let product = extract(&mut cursor, None).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn remove_svg_in_figure() {
        // Test test case is based off Kotaku. Notice that if you hover over the
        // image a zoom SVG appears, which we want removed.
        // https://kotaku.com/watch-dogs-legion-will-eventually-get-60-fps-on-next-g-1846603174
        let input = r#"
        <body>
          <p>Some Text Some text some text and some more text</p>
          <figure class="element-image">
            <picture>
              <img alt="some text" src="some-image.jpg">
            </picture>
            <span class="inline-expand-image">
            <svg class="centered-icon__svg" height="22" viewBox="0 0 22 22" width="22">
                <path d="M3.4 20.2L9 14.5 7.5 13l-5.7 5.6L1 14H0v7.5l.5.5H8v-1l-4.6-.8M18.7 1.9L13 7.6 14.4 9l5.7-5.7.5 4.7h1.2V.6l-.5-.5H14v1.2l4.7.6"></path>
            </svg>
            </span>
          </figure>
        </body>"#;
        let expected = r#"
        <body id="article">
          <p>Some Text Some text some text and some more text</p>
          <figure class="element-image">
            <picture>
              <img alt="some text" src="some-image.jpg">
            </picture>
          </figure>
        </body>"#;

        let mut cursor = Cursor::new(input);
        let product = extract(&mut cursor, None).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn remove_elements_high_svg_density() {
        let input = r#"
        <body>
          <p>Some Text Some text some text and some more text</p>
          <div class="videoContainer">
            <svg class=" play">
            <use aria-hidden="false" xlink:href="\#play"></use>
            </svg>
            <svg class=" pause">
            <use aria-hidden="false" xlink:href="\#pause"></use>
            </svg>
            <figure class=" img"><img src="some-image.jpg" class="photo" alt="" height="110" width="196" loading="lazy"></figure>
          </div>
        </body>"#;
        let expected = r#"
        <body id="article">
          <p>Some Text Some text some text and some more text</p>
        </body>"#;

        let mut cursor = Cursor::new(input);
        let product = extract(&mut cursor, None).unwrap();
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }

    #[test]
    fn top_candidate_has_unlikely_tags() {
        // On the first run, the pass, "sidebar" will be ignored because usually it
        // indicates page noise. Once no other candidate is found, it will be
        // reconsidered and chosen as the top candidate on the second pass.

        // Let's create a lot of high scoring paragraphs to offset the negative
        // score incurred by the unlikely tag.
        let mut p_blob = String::new();
        for _ in 0..15 {
            let mut large_p_node = String::new();
            large_p_node.push_str("<p>");
            for _ in 0..10 {
                large_p_node.push_str("yes, this text is counted.");
            }
            large_p_node.push_str("</p>");
            p_blob.push_str(&large_p_node);
        }
        let input = format!(r#"<html><body><div class="sidebar">{}</div></body></html>"#, p_blob);
        let mut cursor = Cursor::new(input);
        let product = extract(&mut cursor, None).unwrap();
        assert!(product.content.len() > 150);
    }

    #[test]
    fn test_clean_title_colon() {
        let input = "The SoCal Weekly Digest: Welcome to our wonderful page";
        let expected = "Welcome to our wonderful page";
        let output = clean_title(&Sink::default(), input.to_string());
        assert_eq!(expected, output);
    }

    #[test]
    fn test_clean_title_separator_left() {
        let input = "Príncipe Harry asegura que su padre y hermano están \"atrapados\" en la monarquía: \"Siento compasión\" | Príncipe Carlos | Príncipe William | Meghan Markle | Duques de Sussex | Oprah Winfrey";
        let expected = "Príncipe Harry asegura que su padre y hermano están \"atrapados\" en la monarquía: \"Siento compasión\"";
        let output = clean_title(&Sink::default(), input.to_string());
        assert_eq!(expected, output);
    }

    #[test]
    fn test_clean_title_separator_right() {
        let input = "Short Title | How Cats Can Save the Planet";
        let expected = "How Cats Can Save the Planet";
        let output = clean_title(&Sink::default(), input.to_string());
        assert_eq!(expected, output);
    }

    #[test]
    fn test_clean_title_preserve_hyphen() {
        let input = "Just-released Minecraft exploit makes it easy to crash game servers";
        let output = clean_title(&Sink::default(), input.to_string());
        assert_eq!(input, output);
    }

    #[test]
    fn test_clean_title_ascii_dash_separator() {
        // A common pattern found it <title> tags is the site name being included after
        // a final dash
        let input =
            "House committee votes to approve bill that would grant DC statehood - CNNPolitics";
        let expected = "House committee votes to approve bill that would grant DC statehood";
        let output = clean_title(&Sink::default(), input.to_string());
        assert_eq!(expected, output);
    }

    #[test]
    fn test_clean_title_unicode_dash_separator() {
        // A follow up to the last test with common unicode dashes. For example &#8211;
        // converts to an en dash.
        let dashes = [
            "-", // hyphen
            "–", // en dash
            "—", // em dash
        ];
        let expected = "Coinbase from YC to IPO";
        for &d in dashes.iter() {
            let f = format!("Coinbase from YC to IPO {} Y Combinator", d);
            let output = clean_title(&Sink::default(), f.to_string());
            assert_eq!(expected, output);
        }
    }

    #[test]
    fn test_clean_title_preserve_dash() {
        // In this case, we don't want to delete the content after the " - ", because it
        // is part of the title
        let input = "Raspberry Pi 3 - All-time bestselling computer in UK";
        let output = clean_title(&Sink::default(), input.to_string());
        assert_eq!(input, output);
    }

    #[test]
    fn test_clean_title_preserve_colon() {
        let input = r#"
        <html>
          <head>
          <meta property="og:title" content="Watch Dogs: Legion Will Be Free To Play This Weekend"/>
          </head>
          <body>
            <header>
              <h1><i>Watch Dogs: Legion</i> Will Be Free To Play This Weekend</h1>
            </header>
          </body>
        </html>
        "#;
        let expected = "Watch Dogs: Legion Will Be Free To Play This Weekend";
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(expected, product.meta.title);
    }

    #[test]
    fn test_meta_variant0() {
        let input = r#"
        <html>
          <head>
            <meta charset="utf-8"/>
          </head>
          <body></body>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(product.meta.charset.expect("Expected charset"), "utf-8");
    }

    #[test]
    fn test_meta_variant1() {
        let input = r#"
        <html>
          <head>
            <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
          </head>
          <body></body>
        </html>
        "#;
        let mut cursor = Cursor::new(input);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(product.meta.charset.expect("Expected charset"), "utf-8");
    }

    #[test]
    fn test_ldjson_missing_context() {
        let mut meta = Meta::default();
        let input = "{\"@type\":\"NewsArticle\"}";
        match try_parse_untyped_jsonld(&input, &mut meta) {
            Ok(_) => assert!(false, "Expected missing context error"),
            Err(e) => assert_eq!(JsonLdError::MissingContext, e),
        }
    }

    #[test]
    fn test_ldjson_invalid_context() {
        let mut meta = Meta::default();
        let input = "{\"@context\":\"http://fake.org\", \"@type\":\"NewsArticle\"}";
        match try_parse_untyped_jsonld(&input, &mut meta) {
            Ok(_) => assert!(false, "Expected invalid context error"),
            Err(e) => assert_eq!(JsonLdError::InvalidContext, e),
        }
    }

    #[test]
    fn test_ldjson_missing_type() {
        let mut meta = Meta::default();
        let input = "{\"@context\":\"http://schema.org\"}";
        match try_parse_untyped_jsonld(&input, &mut meta) {
            Ok(_) => assert!(false, "Expected missing type error"),
            Err(e) => assert_eq!(JsonLdError::MissingType, e),
        }
    }

    #[test]
    fn test_ldjson_invalid_type() {
        let mut meta = Meta::default();
        let input = "{\"@context\":\"http://schema.org\", \"@type\":\"DefinitelyNotValid\"}";
        match try_parse_untyped_jsonld(&input, &mut meta) {
            Ok(_) => assert!(false, "Expected invalid type error"),
            Err(e) => assert_eq!(JsonLdError::InvalidType, e),
        }
    }

    #[test]
    fn test_ldjson_buzzfeed() {
        let mut meta = Meta::default();
        let input = r#"
{"@context":"http://schema.org",
"@type":"NewsArticle",
"isAccessibleForFree":true,
"mainEntityOfPage":"https://www.buzzfeed.com/markdistefano/diet-pills-burns-up",
"description":"An inquest into Eloise Parry's death has been adjourned until July.",
"headline":"Student Dies After Diet Pills She Bought Online \"Burned Her Up From Within\"",
"datePublished":"2015-04-21T09:29:39.000Z",
"dateModified":"2015-04-21T09:29:39.000Z",
"author":"[{\"@type\":\"Person\",\"name\":\"Mark Di Stefano\",\"url\":\"https://www.buzzfeed.com/markdistefano\",\"jobTitle\":\"BuzzFeed News Reporter, Australia\"}]",
"publisher":{"@type":"Organization","name":"BuzzFeed","url":"https://www.buzzfeed.com"},
"image":{"@type":"ImageObject","url":"https://img.buzzfeed.com/buzzfeed-static/static/2015-04/22/5/campaign_images/webdr03/student-dies-after-diet-pills-she-bought-online-b-2-28712-1429696299-24_dblbig.jpg?resize=1200:*","representativeOfPage":true}
}
    "#;
        try_parse_untyped_jsonld(&input, &mut meta).expect("Could not parse json ld");
        assert_eq!("Mark Di Stefano", meta.author.expect("No author extracted"));
        assert_eq!(
            "Student Dies After Diet Pills She Bought Online \"Burned Her Up From Within\"",
            meta.title
        );
        assert_eq!(
            "An inquest into Eloise Parry's death has been adjourned until July.",
            meta.description.expect("No description extracted")
        );
        assert!(meta.last_modified.is_some(), "Could not parse dateModified field");
    }
}
