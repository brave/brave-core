use crate::dom;
use crate::scorer;
use crate::util;
use html5ever::tendril::StrTendril;
use html5ever::tree_builder::TreeSink;
use html5ever::tree_builder::{ElementFlags, NodeOrText};
use html5ever::{LocalName, QualName};
use kuchiki::Sink;
use scorer::{Candidate, Title, TopCandidate};
use std::collections::BTreeMap;
use std::collections::HashMap;
use std::default::Default;
use std::path::Path;
use std::str::FromStr;
use url::Url;

const NUM_TOP_CANDIDATES: usize = 5;

#[derive(Debug)]
pub struct Product {
    pub title: String,
    pub content: String,
}

pub fn extract_dom<S: ::std::hash::BuildHasher>(
    mut dom: &mut Sink,
    url: &Url,
    features: &HashMap<String, u32, S>,
) -> Result<Product, std::io::Error> {
    let mut title = Title::default();
    let mut candidates: BTreeMap<String, Candidate> = BTreeMap::new();
    let mut nodes = BTreeMap::new();
    let handle = dom.document_node.clone();

    // extracts title (if it exists) pre-processes the DOM by removing script
    // tags, css, links
    scorer::preprocess(&mut dom, handle.clone(), &mut title);

    // now that the dom has been preprocessed, get the set of potential dom
    // candidates and their scoring. a candidate contains the node parent of the
    // dom tree branch and its score. in practice, this function will go through
    // the dom and populate `candidates` data structure
    scorer::find_candidates(
        &mut dom,
        Path::new("/"),
        handle.clone(),
        &mut candidates,
        &mut nodes,
    );

    if candidates.iter().count() == 0 {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            "No candidates found.",
        ));
    }


    let mut id: String = "/".to_string();

    // top candidate is the top scorer among the tree dom's candidates. this is
    // the subtree that will be considered for final rendering
    let mut top_candidate = handle;

    {
        // scores all candidate nodes
        let mut top_candidates: Vec<TopCandidate> = vec![];
        for (i, c) in candidates.iter() {
            let score = c.score.get() * (1.0 - scorer::get_link_density(&c.node));
            c.score.set(score);

            if top_candidates.len() < NUM_TOP_CANDIDATES {
                top_candidates.push(TopCandidate {
                    candidate: &c,
                    id: i.to_string(),
                });
            } else {
                let min_index = util::min_elem_index(&top_candidates);
                let min = &mut top_candidates[min_index];
                if score > min.candidate.score.get() {
                    *min = TopCandidate {
                        candidate: &c,
                        id: i.to_string(),
                    }
                }
            }
        }
        debug_assert!(top_candidates.len() > 0);
        let max_index = util::max_elem_index(&top_candidates);
        top_candidates.swap(0, max_index);
        if let Some(pid) = scorer::search_alternative_candidates(&top_candidates, &nodes) {
            id = pid.to_string();
        } else {
            id = top_candidates[0].id.to_string();
        }
    }

    {
        let top = scorer::find_or_create_candidate(Path::new(id.as_str()), &mut candidates, &nodes);
        if top.is_none() {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidInput,
                "No candidates found.",
            ));
        }
        top_candidate = top.unwrap().node.clone();
    }

    scorer::clean(
        &mut dom,
        Path::new(id.as_str()),
        top_candidate.clone(),
        url,
        &title.title,
        features,
        &candidates,
    );


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

#[cfg(test)]
mod tests {
    use super::*;
    use html5ever::parse_document;
    use html5ever::tendril::TendrilSink;
    use std::io::Cursor;
    use std::io::Read;

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

    fn extract<R>(input: &mut R, url: Option<&str>) -> Result<Product, std::io::Error>
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
}
