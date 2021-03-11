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
use scorer::{Candidate, Title, TopCandidate};
use std::cell::Cell;
use std::collections::BTreeMap;
use std::collections::HashMap;
use std::default::Default;
use std::io::Read;
use std::path::Path;
use std::rc::Rc;
use std::str::FromStr;
use url::Url;
use util;

const NUM_TOP_CANDIDATES: usize = 5;

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
    let mut candidates: BTreeMap<String, Rc<Candidate>> = BTreeMap::new();
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

    let mut id: &Path = Path::new("/");

    // top candidate is the top scorer among the tree dom's candidates. this is
    // the subtree that will be considered for final rendering
    let mut top_candidate: Rc<Candidate> = Rc::new(Candidate {
        node: handle,
        score: Cell::new(0.0),
    });

    // scores all candidate nodes
    let mut top_candidates: Vec<TopCandidate> = vec![];
    for (i, c) in candidates.iter() {
        let score = c.score.get() * (1.0 - scorer::get_link_density(&c.node));
        c.score.set(score);

        if top_candidates.len() < NUM_TOP_CANDIDATES {
            top_candidates.push(TopCandidate {
                candidate: c.clone(),
                id: i.to_string(),
            });
        } else {
            let min_index = util::min_elem_index(&top_candidates);
            let min = &mut top_candidates[min_index];
            if score > min.candidate.score.get() {
                *min = TopCandidate {
                    candidate: c.clone(),
                    id: i.to_string(),
                }
            }
        }
    }

    assert!(top_candidates.len() > 0);
    let max_index = util::max_elem_index(&top_candidates);
    top_candidates.swap(0, max_index);
    if let Some(pid) = scorer::search_alternative_candidates(&top_candidates, &nodes) {
        top_candidate = scorer::find_or_create_candidate(pid, &mut candidates, &nodes).unwrap();
        id = pid;
    } else {
        top_candidate = top_candidates[0].candidate.clone();
        id = Path::new(top_candidates[0].id.as_str());
    }

    scorer::clean(
        &mut dom,
        id,
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
    use markup5ever_rcdom::Node;
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
        assert_eq!(
            normalize_output(expected),
            normalize_output(&product.content)
        );
    }

    fn make_test_candidate(
        mut dom: &mut RcDom,
        path: &str,
        tag: &str,
        score: f32,
        nodes: &mut BTreeMap<String, Rc<Node>>,
        top_candidates: &mut Vec<TopCandidate>,
    ) {
        let e0 = dom::simple_create_element(&mut dom, tag);
        nodes.insert(path.to_string(), e0.clone());
        let c = Candidate::new(e0, score);
        top_candidates.push(TopCandidate {
            id: path.to_string(),
            candidate: c.clone(),
        });
    }

    #[test]
    fn alternative_candidates() {
        let mut dom = RcDom::default();
        let mut nodes = BTreeMap::new();
        let mut top_candidates: Vec<TopCandidate> = vec![];

        // First candidate. In the algorithm, this is the one with the higest score
        make_test_candidate(
            &mut dom,
            "/1/1/0/1",
            "div",
            57.1,
            &mut nodes,
            &mut top_candidates,
        );

        // Remaining alternative candidates with scores close to the top scorer.
        make_test_candidate(
            &mut dom,
            "/1/1/0",
            "h1",
            55.5,
            &mut nodes,
            &mut top_candidates,
        );
        make_test_candidate(
            &mut dom,
            "/1/1/0/2/0",
            "div",
            53.3,
            &mut nodes,
            &mut top_candidates,
        );
        make_test_candidate(
            &mut dom,
            "/1/1/0/3",
            "div",
            50.0,
            &mut nodes,
            &mut top_candidates,
        );

        let id = scorer::search_alternative_candidates(&top_candidates, &nodes).unwrap();
        assert_eq!("/1/1/0", id.to_str().unwrap());
    }
}
