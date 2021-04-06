use crate::dom;
use crate::scorer;
use crate::util;
use html5ever::parse_document;
use html5ever::tendril::StrTendril;
use html5ever::tendril::TendrilSink;
use html5ever::tree_builder::TreeSink;
use html5ever::tree_builder::{ElementFlags, NodeOrText};
use html5ever::{LocalName, QualName};
use kuchiki::NodeRef as Handle;
use kuchiki::Sink;
use regex::Regex;
use scorer::{Title, TopCandidate};
use std::collections::HashMap;
use std::default::Default;
use std::io::Read;
use std::str::FromStr;
use url::Url;
use util::StringUtils;

// The number of candidates to consider when choosing the "top" candidate. These
// top candidates are used in the alternative candidates part of the algorithm.
// This number is taken from the Mozilla implementation.
// https://github.com/mozilla/readability/blob/e2aea3121a9bb6e05478edc1596026c41c782779/Readability.js#L111
const NUM_TOP_CANDIDATES: usize = 5;

lazy_static! {
    static ref SEPARATORS: Regex = Regex::new(r#"\s+[\|\-\\/>»]\s+"#).unwrap();
}

#[derive(Debug)]
pub struct Product {
    pub title: String,
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

    let mut dom: Sink = parse_document(Sink::default(), Default::default())
        .from_utf8()
        .read_from(input)?;

    extract_dom(&mut dom, &url, &HashMap::new())
}

pub fn extract_dom<S: ::std::hash::BuildHasher>(
    mut dom: &mut Sink,
    url: &Url,
    features: &HashMap<String, u32, S>,
) -> Result<Product, std::io::Error> {
    let mut title = Title::default();
    let handle = dom.document_node.clone();

    // extracts title (if it exists) pre-processes the DOM by removing script
    // tags, css, links
    scorer::preprocess(&mut dom, handle.clone(), &mut title);
    title.title = clean_title(title.title);

    // now that the dom has been preprocessed, get the set of potential dom
    // candidates and their scoring. a candidate contains the node parent of the
    // dom tree branch and its score. in practice, this function will go through
    // the dom and populate `candidates` data structure
    scorer::find_candidates(&mut dom, handle);

    // top candidate is the top scorer among the tree dom's candidates. this is
    // the subtree that will be considered for final rendering
    let top_candidate: Handle;

    {
        // scores all candidate nodes
        let mut top_candidates: Vec<TopCandidate> = vec![];
        for node in dom.document_node.descendants().filter(|d| {
            d.as_element()
                .and_then(|e| Some(e.is_candidate.get()))
                .unwrap_or(false)
        }) {
            let elem = node.as_element().unwrap();
            let score = elem.score.get() * (1.0 - scorer::get_link_density(&node));
            elem.score.set(score);

            if top_candidates.len() < NUM_TOP_CANDIDATES {
                top_candidates.push(TopCandidate { node });
            } else {
                let min_index = util::min_elem_index(&top_candidates);
                let min = &mut top_candidates[min_index];
                if score > min.score() {
                    *min = TopCandidate { node }
                }
            }
        }
        if top_candidates.len() == 0 {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidInput,
                "No candidates found.",
            ));
        }
        let max_index = util::max_elem_index(&top_candidates);
        top_candidates.swap(0, max_index);
        if let Some(new_top) = scorer::search_alternative_candidates(&top_candidates) {
            top_candidate = new_top;
        } else {
            top_candidate = top_candidates[0].node.clone();
        }
    }

    // Append siblings of the new root with related content.
    scorer::append_related_siblings(&mut dom, top_candidate.clone());

    scorer::clean(&mut dom, top_candidate.clone(), url, &title.title, features);

    // Our CSS formats based on id="article".
    dom::set_attr("id", "article", top_candidate.clone(), true);

    let name = QualName::new(None, ns!(), LocalName::from("h1"));
    let header = dom.create_element(name, vec![], ElementFlags::default());
    dom.append(
        &header,
        NodeOrText::AppendText(StrTendril::from_str(&title.title).unwrap_or_default()),
    );

    if let Some(first_child) = top_candidate.first_child() {
        dom.append_before_sibling(&first_child.clone(), NodeOrText::AppendNode(header.clone()));
    }

    // Calls html5ever::serialize() with IncludeNode for us.
    let content: String = top_candidate.to_string();
    Ok(Product {
        title: title.title,
        content,
    })
}

pub fn clean_title(title: String) -> String {
    if let Some(m) = SEPARATORS.find(&title) {
        let mut cur_title = title.substring(0, m.start());
        if cur_title.split_whitespace().count() < 3 {
            cur_title = title.substring(m.end(), title.len());
        }
        cur_title.trim().to_string()
    } else {
        title
            .find(": ")
            .and_then(|_| {
                let mut cur_title = title.substring(title.rfind(':').unwrap() + 1, title.len());

                // Less than 3 words in the title. Try first colon.
                if cur_title.split_whitespace().count() < 3 {
                    cur_title = title.substring(title.find(':').unwrap() + 1, title.len());
                } else if title
                    .substring(0, title.find(':').unwrap_or(0))
                    .split_whitespace()
                    .count()
                    > 5
                {
                    return None;
                }
                Some(cur_title.trim().to_string())
            })
            .unwrap_or(title)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    fn normalize_output(input: &str) -> String {
        return input
            .lines()
            .map(|line| line.trim())
            .filter(|line| !line.is_empty())
            .collect();
    }

    fn preprocess<R>(input: &mut R) -> Result<Product, std::io::Error>
    where
        R: Read,
    {
        let mut dom: Sink = parse_document(Sink::default(), Default::default())
            .from_utf8()
            .read_from(input)?;

        let mut title = Title::default();
        let handle = dom.document_node.clone();
        scorer::preprocess(&mut dom, handle, &mut title);
        let content = dom.document_node.to_string();
        Ok(Product {
            title: title.title,
            content,
        })
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
        assert_eq!(product.title, "This is title");
    }

    #[test]
    fn test_title_prefer_meta() {
        let data = r#"
        <head>
        <meta property="og:title" content="Raspberry Pi 3 - All-time bestselling computer in UK"/>
        <meta property="hi" content="test"/>
        <title>Raspberry Pi 3 - All-time bestselling computer in UK - SimplyFound</title>
        </head>
        "#;
        let mut cursor = Cursor::new(data);
        let product = preprocess(&mut cursor).unwrap();
        assert_eq!(
            product.title,
            "Raspberry Pi 3 - All-time bestselling computer in UK"
        );
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
        assert_eq!(
            normalize_output(expected),
            normalize_output(&product.content)
        );
    }

    #[test]
    fn unwrap_noscript_img_delete_preceding() {
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
        assert_eq!(
            normalize_output(expected),
            normalize_output(&product.content)
        );
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
        assert_eq!(
            normalize_output(expected),
            normalize_output(&product.content)
        );
    }

    #[test]
    fn rewrite_divs_single_p() {
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
        assert_eq!(
            normalize_output(expected),
            normalize_output(&product.content)
        );
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
        assert_eq!(
            normalize_output(expected),
            normalize_output(&product.content),
        );
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
        assert_eq!(
            normalize_output(expected),
            normalize_output(&product.content),
        );
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
          <h1></h1>
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
        assert_eq!(
            normalize_output(expected),
            normalize_output(&product.content)
        );
    }

    #[test]
    fn test_clean_title_colon() {
        let input = "The SoCal Weekly Digest: Welcome to our wonderful page";
        let expected = "Welcome to our wonderful page";
        let output = clean_title(input.to_string());
        assert_eq!(expected, output);
    }

    #[test]
    fn test_clean_title_separator_left() {
        let input = "Príncipe Harry asegura que su padre y hermano están \"atrapados\" en la monarquía: \"Siento compasión\" | Príncipe Carlos | Príncipe William | Meghan Markle | Duques de Sussex | Oprah Winfrey";
        let expected = "Príncipe Harry asegura que su padre y hermano están \"atrapados\" en la monarquía: \"Siento compasión\"";
        let output = clean_title(input.to_string());
        assert_eq!(expected, output);
    }

    #[test]
    fn test_clean_title_separator_right() {
        let input = "Short Title | How Cats Can Save the Planet";
        let expected = "How Cats Can Save the Planet";
        let output = clean_title(input.to_string());
        assert_eq!(expected, output);
    }

    #[test]
    fn test_clean_title_preserve_hyphen() {
        let input = "Just-released Minecraft exploit makes it easy to crash game servers";
        let output = clean_title(input.to_string());
        assert_eq!(input, output);
    }
}
