use dom;
use markup5ever_rcdom::{Handle, Node, RcDom};
use markup5ever_rcdom::NodeData::{Comment, Doctype, Document, ProcessingInstruction, Element, Text};
use html5ever::tree_builder::TreeSink;
use html5ever::tree_builder::{ElementFlags, NodeOrText};
use html5ever::{LocalName, QualName};
use regex::Regex;
use std::cell::Cell;
use std::collections::{BTreeMap, HashMap};
use std::path::Path;
use std::rc::Rc;
use url::Url;

pub static PUNCTUATIONS_REGEX: &str = r"([,]\?)";
pub static UNLIKELY_CANDIDATES: &str = "-ad-|ai2html|banner\
    |breadcrumbs|combx|comment|community|cover-wrap|disqus|extra|foot|gdpr\
    |header|legends|menu|related|remark|replies|rss|shoutbox|sidebar|skyscraper\
    |social|sponsor|supplemental|ad-break|agegate|pagination|pager|popup\
    |yom-remote";
pub static LIKELY_CANDIDATES: &str = "and|article|body|column|main\
    |shadow\
    |a";
pub static POSITIVE_CANDIDATES: &str = "article|body|content|entry\
        |hentry|h-entry|main|page|pagination|post|text|blog|story|paragraph|speakable";
pub static NEGATIVE_CANDIDATES: &str = "hidden|^hid$|hid$|hid|^hid\
        |banner|combx|comment|com-|contact|foot|footer|footnote|gdpr|header\
        |legends|menu|related|remark|replies|rss|shoutbox|sidebar|skyscraper\
        |social|sponsor|supplemental|ad-break|agegate|pagination|pager|popup\
        yom-remote";
static BLOCK_CHILD_TAGS: [&LocalName; 9] = [
    &local_name!("a"),
    &local_name!("blockquote"),
    &local_name!("dl"),
    &local_name!("ol"),
    &local_name!("p"),
    &local_name!("pre"),
    &local_name!("table"),
    &local_name!("ul"),
    &local_name!("select"),
];

static DECAY_FACTOR: f32 = 3.0;

lazy_static! {
    static ref PUNCTUATIONS: Regex = Regex::new(PUNCTUATIONS_REGEX).unwrap();
    static ref LIKELY: Regex = Regex::new(LIKELY_CANDIDATES).unwrap();
    static ref UNLIKELY: Regex = Regex::new(UNLIKELY_CANDIDATES).unwrap();
    static ref POSITIVE: Regex = Regex::new(POSITIVE_CANDIDATES).unwrap();
    static ref NEGATIVE: Regex = Regex::new(NEGATIVE_CANDIDATES).unwrap();
}

pub struct Candidate {
    pub node: Rc<Node>,
    pub score: Cell<f32>,
}

pub fn fix_img_path(handle: Handle, url: &Url) -> bool {
    if let Some(src) = dom::get_attr("src", &handle) {
        if !src.starts_with("//") && !src.starts_with("http://") && src.starts_with("https://") {
            if let Ok(new_url) = url.join(&src) {
                dom::set_attr("src", new_url.as_str(), handle);
                true
            } else {
                // failed to fix
                false
            }
        } else {
            // all OK
            true
        }
    } else {
        false
    }
}

pub fn get_link_density(handle: &Handle) -> f32 {
    let text_length = dom::text_len(&handle) as f32;
    if text_length == 0.0 {
        return 0.0;
    }
    let mut link_length = 0.0;
    let mut links: Vec<Rc<Node>> = vec![];
    dom::find_node(&handle, "a", &mut links);
    for link in links.iter() {
        link_length += dom::text_len(&link) as f32;
    }
    link_length / text_length
}

// is candidate iif lenght of the text is larger than 20 words AND its tag is
// is `div`, `article`, `center`, `section` while not in containing nodes in
// BLOCK_CHILD_TAGS
pub fn is_candidate(handle: &Handle) -> bool {
    let text_len = dom::text_len(&handle);
    if text_len < 20 {
        return false;
    }
    match handle.data {
        Element { ref name, .. } => match name.local {
            local_name!("p") => true,
            local_name!("div")
            | local_name!("article")
            | local_name!("center")
            | local_name!("section") => !dom::has_nodes(
                &handle,
                &BLOCK_CHILD_TAGS.iter().cloned().collect::<Vec<_>>(),
            ),
            _ => false,
        },
        _ => false,
    }
}

pub fn init_content_score(handle: &Handle) -> f32 {
    let score = match handle.data {
        Element { ref name, .. } => match name.local {
            local_name!("article") => 10.0,
            local_name!("div") => 5.0,
            local_name!("h1") | local_name!("h2") | local_name!("h3") | local_name!("h4") => 5.0,
            local_name!("blockquote") => 3.0,
            local_name!("pre") => 3.0,
            local_name!("td") => 3.0,
            local_name!("th") => 5.0,
            local_name!("address") => -3.0,
            local_name!("ol") => -3.0,
            local_name!("ul") => -3.0,
            local_name!("dl") => -3.0,
            local_name!("dd") => -3.0,
            local_name!("dt") => -3.0,
            local_name!("li") => -3.0,
            local_name!("form") => -3.0,
            _ => 0.0,
        },
        _ => 0.0,
    };
    score + get_class_weight(handle)
}

pub fn calc_content_score(handle: &Handle) -> f32 {
    let mut score: f32 = 1.0;
    let mut text = String::new();
    dom::extract_text(handle, &mut text, true);
    let mat = PUNCTUATIONS.find_iter(&text);
    score += mat.count() as f32;
    score += f32::min(f32::floor(text.chars().count() as f32 / 100.0), 3.0);
    score
}

pub fn get_class_weight(handle: &Handle) -> f32 {
    let mut weight: f32 = 0.0;
    if let Element { ref attrs, .. } = handle.data {
        for name in ["id", "class"].iter() {
            if let Some(val) = dom::attr(name, &attrs.borrow()) {
                if val == "" {
                    weight -= 3.0
                }
                if POSITIVE.is_match(&val) {
                    weight += 25.0
                };
                if NEGATIVE.is_match(&val) {
                    weight -= 25.0
                }
            }
        }
    };
    weight
}

pub fn preprocess(mut dom: &mut RcDom, handle: Handle, mut title: &mut String) -> bool {
    if let Element {
        ref name,
        ref attrs,
        ..
    } = handle.data
    {
        match name.local {
            local_name!("script") | local_name!("link") | local_name!("style") => return true,
            local_name!("title") => dom::extract_text(&handle, &mut title, true),
            _ => (),
        }
        for attr_name in ["id", "class", "itemProp"].iter() {
            if let Some(val) = dom::attr(attr_name, &attrs.borrow()) {
                if name.local != local_name!("body")
                    && UNLIKELY.is_match(&val)
                    && !LIKELY.is_match(&val)
                {
                    return true;
                }
            }
        }
    }
    let mut useless_nodes = vec![];
    let mut paragraph_nodes = vec![];
    let mut br_count = 0;
    for child in handle.children.borrow().iter() {
        if preprocess(&mut dom, child.clone(), &mut title) {
            useless_nodes.push(child.clone());
        }
        match child.data {
            Element { ref name, .. } => match name.local {
                local_name!("br") => br_count += 1,
                _ => br_count = 0,
            },
            Text { ref contents } => {
                let s = contents.borrow();
                if br_count >= 2 && !s.trim().is_empty() {
                    paragraph_nodes.push(child.clone());
                    br_count = 0
                }
            }
            _ => (),
        }
    }
    for node in useless_nodes.iter() {
        dom.remove_from_parent(node);
    }
    for node in paragraph_nodes.iter() {
        let name = QualName::new(None, ns!(), LocalName::from("p"));
        let p = dom.create_element(name, vec![], ElementFlags::default());
        dom.append_before_sibling(node, NodeOrText::AppendNode(p.clone()));
        dom.remove_from_parent(node);
        if let Text { ref contents } = node.data {
            dom.append(&p, NodeOrText::AppendText(contents.borrow().clone()))
        }
    }
    false
}

pub fn find_candidates(
    mut dom: &mut RcDom,
    id: &Path,
    handle: Handle,
    candidates: &mut BTreeMap<String, Candidate>,
    nodes: &mut BTreeMap<String, Rc<Node>>,
) {
    // Id of a particular node maps to its position in the dom tree, represented
    // as std::path::Path data structure
    if let Some(id) = id.to_str().map(|id| id.to_string()) {
        nodes.insert(id, handle.clone());
    }

    // is candidate iif length of the text in handle is larger than 20 words AND
    // its tag is `div`, `article`, `center`, `section` while not in containing
    // nodes in BLOCK_CHILD_TAGS

    if is_candidate(&handle) {
        // calculates the content score of the current candidate
        let score = calc_content_score(&handle);

        // adds candidate's score to ALL of its parents in the tree, rescursively
        // the scoring impact of child nodes in ALL upper nodes decays as the
        // tree is traverse backwards:
        //   parent: no decay
        //   grandparent: scoring divided by 2
        //   subsequent parent nodes: level * DECAY_FACTOR (3)

        // parent
        if let Some(c) = id
            .parent()
            .and_then(|pid| find_or_create_candidate(pid, candidates, nodes))
        {
            c.score.set(c.score.get() + score)
        }

        // grandparent
        if let Some(c) = id
            .parent()
            .and_then(|pid| pid.parent())
            .and_then(|gpid| find_or_create_candidate(gpid, candidates, nodes))
        {
            c.score.set(c.score.get() + (score / 2.0))
        }

        // subsequent nodes scored based on the level in the DOM
        if let Some(distant_ancs) = id
            .parent()
            .and_then(|pid| pid.parent())
            .and_then(|gpid| gpid.parent())
        {
            let paths = get_all_ancestor_paths(distant_ancs);
            let mut level = 2.0;
            for p in paths {
                let add_score = score / (level * DECAY_FACTOR);
                if let Some(c) = find_or_create_candidate(p, candidates, nodes) {
                    c.score.set(c.score.get() + add_score);
                    level += 1.0;
                }
            }
        }
    }

    // for all the current child's node, execute recursively find_candidates()
    for (i, child) in handle.children.borrow().iter().enumerate() {
        find_candidates(
            &mut dom,
            id.join(i.to_string()).as_path(),
            child.clone(),
            candidates,
            nodes,
        )
    }
}

fn get_all_ancestor_paths(ps: &Path) -> Vec<&Path> {
    let mut paths = Vec::new();
    for p in ps.ancestors() {
        paths.push(p);
    }
    paths.pop(); // removes last element "/"
    paths
}

fn find_or_create_candidate<'a>(
    id: &Path,
    candidates: &'a mut BTreeMap<String, Candidate>,
    nodes: &BTreeMap<String, Rc<Node>>,
) -> Option<&'a Candidate> {
    if let Some(id) = id.to_str().map(|id| id.to_string()) {
        if let Some(node) = nodes.get(&id) {
            if candidates.get(&id).is_none() {
                candidates.insert(
                    id.clone(),
                    Candidate {
                        node: node.clone(),
                        score: Cell::new(init_content_score(&node)),
                    },
                );
            }
            return candidates.get(&id);
        }
    }
    None
}

// decides whether the handle node is useless (should be dropped) or not.
pub fn clean<S: ::std::hash::BuildHasher>(
    mut dom: &mut RcDom,
    id: &Path,
    handle: Handle,
    url: &Url,
    title: &str,
    features: &HashMap<String, u32, S>,
    candidates: &BTreeMap<String, Candidate>,
) -> bool {
    let useless = match handle.data {
        Document => false,
        Doctype { .. } => false,
        Text { ref contents } => {
            let s = contents.borrow();
            if s.trim().is_empty() {
                true
            } else {
                false
            }
        }
        Comment { .. } => true,
        Element {
            ref name,
            ref attrs,
            ..
        } => {
            match name.local {
                local_name!("script")
                | local_name!("link")
                | local_name!("style")
                | local_name!("noscript")
                | local_name!("meta")
                | local_name!("iframe")
                | local_name!("object")
                | local_name!("header")
                | local_name!("footer")
                | local_name!("aside") => true,
                local_name!("form")
                | local_name!("table")
                | local_name!("ul")
                | local_name!("div") => is_useless(id, &handle, candidates),
                local_name!("img") => !fix_img_path(handle.clone(), url),
                _ => false,
            }

            // // cleans all ids, classes and styles in node
            // dom::clean_attr("id", &mut *attrs.borrow_mut());
            // dom::clean_attr("class", &mut *attrs.borrow_mut());
            // dom::clean_attr("style", &mut *attrs.borrow_mut());
        }
        ProcessingInstruction { .. } => unreachable!(),
    };

    let mut useless_nodes = vec![];
    for (i, child) in handle.children.borrow().iter().enumerate() {
        let pid = id.join(i.to_string());
        if clean(
            &mut dom,
            pid.as_path(),
            child.clone(),
            url,
            title,
            features,
            candidates,
        ) {
            useless_nodes.push(child.clone());
        }
    }
    for node in useless_nodes.iter() {
        dom.remove_from_parent(node);
    }
    if dom::is_empty(&handle) {
        return true;
    }
    useless
}

pub fn is_useless(id: &Path, handle: &Handle, candidates: &BTreeMap<String, Candidate>) -> bool {
    let tag_name = dom::get_tag_name(&handle);
    let weight = get_class_weight(&handle);
    let score = id
        .to_str()
        .and_then(|id| candidates.get(id))
        .map(|c| c.score.get())
        .unwrap_or(0.0);
    if weight + score < 0.0 {
        return true;
    }

    let para_count =
        dom::count_nodes(&handle, &local_name!("p")) + dom::text_children_count(&handle) as u32;
    let li_count = dom::count_nodes(&handle, &local_name!("li")) as i32 - 100;

    if tag_name != Some(&local_name!("ul"))
        && tag_name != Some(&local_name!("ol"))
        && li_count > para_count as i32
    {
        return true;
    }

    let input_count = dom::count_nodes(&handle, &local_name!("input"));
    if input_count as f32 > f32::floor(para_count as f32 / 3.0) {
        return true;
    }

    let img_count = dom::count_nodes(&handle, &local_name!("img"));
    let content_length = dom::text_len(&handle);

    if content_length < 10 && (img_count == 0 || img_count > 2) {
        return true;
    }

    let embed_count = dom::count_nodes(&handle, &local_name!("embed"));
    if (embed_count == 1 && content_length < 35) || embed_count > 1 {
        return true;
    }

    let link_density = get_link_density(handle);
    if weight < 10.0 && link_density > 0.1 {
        return true;
    }
    false
}
