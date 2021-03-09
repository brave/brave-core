use crate::dom;
use html5ever::tendril::StrTendril;
use html5ever::tendril::TendrilSink;
use html5ever::tree_builder::TreeSink;
use html5ever::tree_builder::{ElementFlags, NodeOrText};
use html5ever::{parse_document, serialize};
use html5ever::{LocalName, QualName};
use markup5ever_rcdom::RcDom;
use markup5ever_rcdom::SerializableHandle;
use scorer;
use scorer::{Candidate, Title};
use std::cell::Cell;
use std::collections::BTreeMap;
use std::collections::HashMap;
use std::default::Default;
use std::io::Read;
use std::path::Path;
use std::rc::Rc;
use std::str::FromStr;
use url::Url;

#[derive(Debug)]
pub struct Product {
    pub title: String,
    pub content: String,
}

pub fn extract<R>(input: &mut R, website: Option<&str>) -> Result<Product, std::io::Error>
where
    R: Read,
{
    let url: Url;
    if let Some(website) = website {
        url = Url::parse(website).unwrap();
    } else {
        url = Url::parse("https://example.com").unwrap();
    }
    let mut dom = parse_document(RcDom::default(), Default::default())
        .from_utf8()
        .read_from(input)?;

    extract_dom(&mut dom, &url, &HashMap::new())
}

pub fn preprocess<R>(input: &mut R) -> Result<Product, std::io::Error>
where
    R: Read,
{
    let mut dom = parse_document(RcDom::default(), Default::default())
        .from_utf8()
        .read_from(input)?;

    let mut title = Title::default();
    let handle = dom.document.clone();
    scorer::preprocess(&mut dom, handle, &mut title);
    let mut bytes = vec![];
    let document: SerializableHandle = dom.document.clone().into();
    serialize(&mut bytes, &document, serialize::SerializeOpts::default())?;
    let content = String::from_utf8(bytes).unwrap_or_default();
    Ok(Product {
        title: title.title,
        content,
    })
}

pub fn extract_dom<S: ::std::hash::BuildHasher>(
    mut dom: &mut RcDom,
    url: &Url,
    features: &HashMap<String, u32, S>,
) -> Result<Product, std::io::Error> {
    let mut title = Title::default();
    let mut candidates = BTreeMap::new();
    let mut nodes = BTreeMap::new();
    let handle = dom.document.clone();

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

    let mut id: &str = "/";

    // top candidate is the top scorer among the tree dom's candidates. this is
    // the subtree that will be considered for final rendering
    let mut top_candidate: &Candidate = &Candidate {
        node: handle,
        score: Cell::new(0.0),
    };

    // scores all candidate nodes
    for (i, c) in candidates.iter() {
        let score = c.score.get() * (1.0 - scorer::get_link_density(&c.node));
        c.score.set(score);
        if score <= top_candidate.score.get() {
            continue;
        }
        id = i;
        top_candidate = c;
    }

    scorer::clean(
        &mut dom,
        Path::new(id),
        top_candidate.node.clone(),
        url,
        &title.title,
        features,
        &candidates,
    );

    // Our CSS formats based on id="article".
    dom::set_attr("id", "article", top_candidate.node.clone(), true);
    let serialize_opts = serialize::SerializeOpts {
        traversal_scope: serialize::TraversalScope::IncludeNode,
        ..Default::default()
    };

    let name = QualName::new(None, ns!(), LocalName::from("h1"));
    let header = dom.create_element(name, vec![], ElementFlags::default());
    dom.append(
        &header,
        NodeOrText::AppendText(StrTendril::from_str(&title.title).unwrap_or_default()),
    );

    if let Some(first_child) = top_candidate.node.children.clone().borrow().iter().nth(0) {
        // Kinda hacky, but it's possible the parent is a dangling pointer if it
        // was deleted during the preprocess or cleaning stages. This ensures we
        // don't panic in append_before_sibling().
        first_child
            .parent
            .set(Some(Rc::downgrade(&top_candidate.node)));
        dom.append_before_sibling(&first_child.clone(), NodeOrText::AppendNode(header.clone()));
    }

    let document: SerializableHandle = top_candidate.node.clone().into();
    let mut bytes = vec![];
    serialize(&mut bytes, &document, serialize_opts)?;
    let content = String::from_utf8(bytes).unwrap_or_default();

    Ok(Product {
        title: title.title,
        content,
    })
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
        assert_eq!(normalize_output(expected), normalize_output(&product.content));
    }
}
