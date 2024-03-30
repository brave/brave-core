use std::io::Read;
use std::collections::BTreeMap;
use std::path::Path;
use std::cell::Cell;
use html5ever::rcdom::{RcDom};
use html5ever::{parse_document, serialize};
use html5ever::tendril::stream::TendrilSink;
use std::default::Default;
#[cfg(feature = "reqwest")]
use std::time::Duration;
#[cfg(feature = "reqwest")]
use reqwest;
use url::Url;
use error::Error;
use dom;
use scorer;
use scorer::Candidate;

#[derive(Debug)]
pub struct Product {
    pub title:     String,
    pub content:   String,
    pub text:      String,
}

#[cfg(feature = "reqwest")]
pub fn scrape(url: &str) -> Result<Product, Error> {
    let client = reqwest::Client::builder()
        .timeout(Duration::new(30, 0))
        .build()?;
    let mut res = client.get(url)
        .send()?;
    if res.status().is_success() {
        let url = Url::parse(url)?;
        extract(&mut res, &url)
    } else {
        Err(Error::Unexpected)
    }
}

pub fn extract<R>(input: &mut R, url: &Url) -> Result<Product, Error> where R: Read {
    let mut dom = parse_document(RcDom::default(), Default::default())
        .from_utf8()
        .read_from(input)
        .unwrap();
    let mut title      = String::new();
    let mut candidates = BTreeMap::new();
    let mut nodes      = BTreeMap::new();
    let handle = dom.document.clone();
    scorer::preprocess(&mut dom, handle.clone(), &mut title);
    scorer::find_candidates(&mut dom, Path::new("/"), handle.clone(), &mut candidates, &mut nodes);
    let mut id: &str = "/";
    let mut top_candidate: &Candidate = &Candidate {
        node:  handle.clone(),
        score: Cell::new(0.0),
    };
    for (i, c) in candidates.iter() {
        let score = c.score.get() * (1.0 - scorer::get_link_density(c.node.clone()));
        c.score.set(score);
        if score <= top_candidate.score.get() {
            continue;
        }
        id            = i;
        top_candidate = c;
    }
    let mut bytes = vec![];

    let node = top_candidate.node.clone();
    scorer::clean(&mut dom, Path::new(id), node.clone(), url, &candidates);

    serialize(&mut bytes, &node, Default::default()).ok();
    let content = String::from_utf8(bytes).unwrap_or_default();

    let mut text: String = String::new();
    dom::extract_text(node.clone(), &mut text, true);
    Ok(Product { title: title, content: content, text: text })
}
