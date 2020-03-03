use html5ever::rcdom::RcDom;
use html5ever::tendril::TendrilSink;
use html5ever::{parse_document, serialize};
use scorer;
use scorer::Candidate;
use std::cell::Cell;
use std::collections::BTreeMap;
use std::collections::HashMap;
use std::default::Default;
use std::io::Read;
use std::path::Path;
use url::Url;

#[derive(Debug)]
pub struct Product {
    pub title: String,
    pub content: String,
}

pub fn extract<R>(input: &mut R, url: &Url) -> Result<Product, std::io::Error>
where
    R: Read,
{
    let mut dom = parse_document(RcDom::default(), Default::default())
        .from_utf8()
        .read_from(input)?;

    extract_dom(&mut dom, url, &HashMap::new())
}

pub fn extract_dom<S: ::std::hash::BuildHasher>(
    mut dom: &mut RcDom,
    url: &Url,
    features: &HashMap<String, u32, S>,
) -> Result<Product, std::io::Error> {
    let mut title = String::new();
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

    let mut bytes = vec![];

    scorer::clean(
        &mut dom,
        Path::new(id),
        top_candidate.node.clone(),
        url,
        &title,
        features,
        &candidates,
    );

    serialize(&mut bytes, &top_candidate.node, Default::default())?;
    let content = String::from_utf8(bytes).unwrap_or_default();

    Ok(Product { title, content })
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs::File;
    use url::Url;

    #[test]
    fn test_extract_title() {
        let mut file = File::open("./tests/samples/simple_title/title.html").unwrap();
        let url = Url::parse("https://example.com").unwrap();
        let product = extract(&mut file, &url).unwrap();
        assert_eq!(product.title, "This is title");
    }
}
