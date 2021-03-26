use crate::dom;
use html5ever::tendril::StrTendril;
use html5ever::tree_builder::TreeSink;
use html5ever::tree_builder::{ElementFlags, NodeOrText};
use html5ever::{LocalName, QualName};
use kuchiki::NodeData::{
    Comment, Doctype, Document, DocumentFragment, Element, ProcessingInstruction, Text,
};
use kuchiki::NodeRef as Handle;
use kuchiki::{ElementData, NodeRef, Sink};
use regex::Regex;
use std::cmp::Ordering;
use std::collections::HashMap;
use std::str::FromStr;
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
static ALTER_TO_DIV_EXCEPTIONS: [&LocalName; 3] = [
    //&local_name!("div"),
    &local_name!("article"),
    &local_name!("section"),
    &local_name!("p"),
];

static DECAY_FACTOR: f32 = 3.0;

lazy_static! {
    static ref PUNCTUATIONS: Regex = Regex::new(PUNCTUATIONS_REGEX).unwrap();
    static ref LIKELY: Regex = Regex::new(LIKELY_CANDIDATES).unwrap();
    static ref UNLIKELY: Regex = Regex::new(UNLIKELY_CANDIDATES).unwrap();
    static ref POSITIVE: Regex = Regex::new(POSITIVE_CANDIDATES).unwrap();
    static ref NEGATIVE: Regex = Regex::new(NEGATIVE_CANDIDATES).unwrap();
}

pub struct TopCandidate {
    pub node: Handle,
}

impl TopCandidate {
    #[inline]
    pub fn score(&self) -> f32 {
        if let Some(elem) = self.node.as_element() {
            elem.score.get()
        } else {
            0.0
        }
    }
}

impl Ord for TopCandidate {
    fn cmp(&self, other: &Self) -> Ordering {
        self.partial_cmp(other).unwrap_or(Ordering::Equal)
    }
}

impl PartialOrd for TopCandidate {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        self.score().partial_cmp(&other.score())
    }
}

impl PartialEq for TopCandidate {
    fn eq(&self, other: &Self) -> bool {
        self.score() == other.score()
    }
}

impl Eq for TopCandidate {}

#[derive(Default)]
pub struct Title {
    pub title: String,
    pub is_meta: bool,
}

/// Add https:// to the img src, if missing.
pub fn fix_img_path(data: &ElementData, url: &Url) -> Option<Url> {
    if let Some(src) = data.attributes.borrow().get(local_name!("src")) {
        if !src.starts_with("//") && !src.starts_with("http://") && src.starts_with("https://") {
            let new_url = url.join(&src);
            new_url.ok()
        } else {
            // all OK
            None
        }
    } else {
        None
    }
}

/// Returns the proportion of links an element contains with the amount of text in the subtree.
#[inline]
pub fn get_link_density(handle: &Handle) -> f32 {
    let text_length = dom::text_len(&handle) as f32;
    if text_length == 0.0 {
        return 0.0;
    }
    let mut links: Vec<Handle> = vec![];
    dom::find_node(&handle, "a", &mut links);
    let link_length = links
        .iter()
        .fold(0.0, |acc, link| acc + dom::text_len(&link) as f32);
    link_length / text_length
}

/// Returns the proportion of children an element has with the amount of text in the subtree.
#[inline]
pub fn get_text_density(handle: &Handle, tags: &[&str]) -> f32 {
    let text_length = dom::text_len(&handle) as f32;
    if text_length == 0.0 {
        return 0.0;
    }
    let nodes = dom::find_nodes_with_tag(&handle, tags);
    let children_length = nodes
        .iter()
        .fold(0.0, |acc, child| acc + dom::text_len(&child) as f32);
    children_length / text_length as f32
}

/// is candidate iif length of the text is larger than 20 words AND its tag is
/// is `div`, `article`, `center`, `section` while not in containing nodes in
/// BLOCK_CHILD_TAGS
pub fn is_candidate(handle: &Handle) -> bool {
    let text_len = dom::text_len(&handle);
    if text_len < 25 {
        return false;
    }
    match handle.data() {
        Element(ref data) => match data.name.local {
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

/// Initialize an element's score based off it's tag.
pub fn init_content_score(handle: &Handle) -> f32 {
    let score = match handle.data() {
        Element(ref data) => match data.name.local {
            local_name!("article") => 10.0,
            local_name!("div") => 5.0,
            local_name!("h1") | local_name!("h2") | local_name!("h3") | local_name!("h4") => -5.0,
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

/// Calculate the "readable" content in an element.
pub fn calc_content_score(handle: &Handle) -> f32 {
    let mut score: f32 = 1.0;
    let mut text = String::new();
    dom::extract_text(handle, &mut text, true);
    let mat = PUNCTUATIONS.find_iter(&text);
    score += mat.count() as f32;
    score += f32::min(f32::floor(text.chars().count() as f32 / 100.0), 3.0);
    score
}

/// Score class and id names against a set of widely used heuristics.
pub fn get_class_weight(handle: &Handle) -> f32 {
    let mut weight: f32 = 0.0;
    if let Some(data) = handle.as_element() {
        for name in ["id", "class"].iter() {
            if let Some(val) = data.attributes.borrow().get(*name) {
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

/// Uses the <meta> elements to extract the article title.
pub fn get_metadata(data: &ElementData, title: &mut Title) {
    if title.is_meta {
        // We already grabbed a title from the metadata earlier in the parse.
        return;
    }
    if let Some(property) = data.attributes.borrow().get(local_name!("property")) {
        // TODO(keur): grab author, description, site name, etc in here.
        // For now we are just getting the title.
        match property.as_ref() {
            "dc:title"
            | "dcterm:title"
            | "og:title"
            | "weibo:article:title"
            | "weibo:webpage:title"
            | "title"
            | "twitter:title" => {
                if let Some(content) = data.attributes.borrow().get(local_name!("content")) {
                    title.title = content.to_string();
                    title.is_meta = true;
                }
            }
            _ => (),
        }
    }
}

/// Do a subparse. The data inside of a noscript element is text. Send it through the parser and
/// return the result if it is one img element.
fn get_inner_img_from_noscript(handle: &Handle) -> Option<Handle> {
    let child = handle.first_child()?;
    if let Some(contents) = child.as_text() {
        let inner = dom::parse_inner(&contents.borrow())?;
        if dom::is_single_image(&inner) {
            if let Some(data) = inner.as_element() {
                let img =
                    NodeRef::new_element(data.name.clone(), data.attributes.borrow().map.clone());
                return Some(img);
            }
        }
    }
    None
}

/// Unwrap a <noscript><img src=".."/></noscript> element to just be an img element. Sites like
/// Medium and BBC wrap img elements in a noscript on page load, and do this same process in
/// Javascript.
fn unwrap_noscript(
    handle: &Handle,
    useless_nodes: &mut Vec<Handle>,
    new_children: &mut Vec<Handle>,
) {
    if let Some(img) = get_inner_img_from_noscript(handle) {
        new_children.push(img);
        if let Some(prev) = dom::previous_element_sibling(handle) {
            if dom::is_single_image(&prev) {
                useless_nodes.push(prev);
            }
        }
    }
}

/// Prepare the DOM for the candidate and cleaning steps. Delete "noisy" nodes and do small
/// transformations.
pub fn preprocess(mut dom: &mut Sink, handle: Handle, mut title: &mut Title) -> bool {
    if let Some(data) = handle.as_element() {
        match data.name.local {
            local_name!("script")
            | local_name!("noscript")
            | local_name!("link")
            | local_name!("style") => return true,
            local_name!("title") => {
                if !title.is_meta && title.title.is_empty() {
                    dom::extract_text(&handle, &mut title.title, true);
                    title.is_meta = false;
                }
            }
            local_name!("meta") => {
                get_metadata(&data, title);
            }
            _ => (),
        }
        for attr_name in ["id", "class", "itemProp"].iter() {
            if let Some(val) = data.attributes.borrow().get(*attr_name) {
                if data.name.local != local_name!("body")
                    && UNLIKELY.is_match(&val)
                    && !LIKELY.is_match(&val)
                {
                    return true;
                }
            }
        }
    }
    let mut useless_nodes = vec![];
    let mut new_children = vec![];
    let mut paragraph_nodes = vec![];
    let mut br_count = 0;
    for child in handle.children() {
        let pending_removal = preprocess(&mut dom, child.clone(), &mut title);
        if pending_removal {
            useless_nodes.push(child.clone());
        }

        // These are pre-processing steps that don't just delete nodes, but also append nodes to
        // their parent.
        match child.data() {
            Element(data) => {
                match data.name.local {
                    local_name!("br") => br_count += 1,
                    _ => br_count = 0,
                }
                if data.name.local == local_name!("noscript") {
                    unwrap_noscript(&child, &mut useless_nodes, &mut new_children);
                } else if !pending_removal && data.name.local == local_name!("div") {
                    // This is handling for sites like mobile.slate.com, where every paragraph
                    // element is wrapped in a single div. Since this can confuse the scoring
                    // algorithm, we delete the outer divs.
                    if let Some(replacement) = dom::get_only_child_by_tag(&child, &local_name!("p"))
                    {
                        let link_density = get_link_density(&child);
                        if link_density < 0.25 {
                            dom.append_before_sibling(&child, NodeOrText::AppendNode(replacement));
                            useless_nodes.push(child.clone());
                        }
                    }
                }
            }
            Text(ref contents) => {
                let s = contents.borrow();
                if br_count >= 2 && !s.trim().is_empty() {
                    paragraph_nodes.push(child.clone());
                    br_count = 0
                }
            }
            _ => (),
        }
    }
    for node in new_children.iter() {
        dom.append(&handle, NodeOrText::AppendNode(node.clone()));
    }
    for node in useless_nodes.iter() {
        dom.remove_from_parent(node);
    }
    for node in paragraph_nodes.iter() {
        let name = QualName::new(None, ns!(), LocalName::from("p"));
        let p = dom.create_element(name, vec![], ElementFlags::default());
        dom.append_before_sibling(node, NodeOrText::AppendNode(p.clone()));
        dom.remove_from_parent(node);
        if let Some(contents) = node.as_text() {
            match StrTendril::from_str(&contents.borrow()) {
                Ok(tendril) => dom.append(&p, NodeOrText::AppendText(tendril)),
                _ => return false,
            }
        }
    }
    false
}

/// Walk the DOM and mark all candidate nodes.
pub fn find_candidates(mut dom: &mut Sink, handle: Handle) {
    // is candidate iif length of the text in handle is larger than 20 words AND
    // its tag is `div`, `article`, `center`, `section` while not in containing
    // nodes in BLOCK_CHILD_TAGS

    if is_candidate(&handle) {
        debug_assert!(handle.as_element().is_some());

        initialize_candidate(&handle);
        // calculates the content score of the current candidate
        let score = calc_content_score(&handle);

        // adds candidate's score to ALL of its parents in the tree, rescursively
        // the scoring impact of child nodes in ALL upper nodes decays as the
        // tree is traverse backwards:
        //   parent: no decay
        //   grandparent: scoring divided by 2
        //   subsequent parent nodes: level * DECAY_FACTOR (3)

        // parent
        if let Some(parent) = handle.parent().as_ref() {
            if let Some(elem) = parent.as_element() {
                initialize_candidate(parent);
                elem.score.set(elem.score.get() + score);
            }

            // grandparent
            if let Some(gparent) = parent.parent().as_ref() {
                if let Some(elem) = gparent.as_element() {
                    initialize_candidate(gparent);
                    elem.score.set(elem.score.get() + (score / 2.0));
                }

                // subsequent nodes scored based on the level in the DOM
                let mut level = 2.0;
                for ancestor in gparent.ancestors() {
                    if let Some(elem) = ancestor.as_element() {
                        initialize_candidate(&ancestor);
                        let add_score = score / (level * DECAY_FACTOR);
                        elem.score.set(elem.score.get() + add_score);
                        level += 1.0;
                    }
                }
            }
        }
    }

    // for all the current child's node, execute recursively find_candidates()
    for child in handle.children() {
        find_candidates(&mut dom, child.clone());
    }
}

#[inline]
pub fn initialize_candidate(handle: &Handle) {
    debug_assert!(handle.as_element().is_some());
    let elem = handle.as_element().unwrap();
    if !elem.is_candidate.get() {
        elem.is_candidate.set(true);
        elem.score.set(init_content_score(handle));
    }
}

/// This function expects an array of top candidates to be considered for
/// the new root of the DOM. It is assumed that the first element is the
/// candidate with the highest score.
pub fn search_alternative_candidates<'a>(top_candidates: &'a Vec<TopCandidate>) -> Option<Handle> {
    const MIN_CANDIDATES: usize = 3;

    debug_assert!(top_candidates.len() > 0);
    let top = &top_candidates[0];
    if top_candidates.len() < MIN_CANDIDATES
        || Some(&local_name!("body")) == dom::get_tag_name(&top.node)
    {
        return None;
    }
    let mut alternative_nodes = vec![];
    for i in 1..top_candidates.len() {
        let c = &top_candidates[i];
        if c.score() / top.score() >= 0.75 {
            alternative_nodes.push(c.node.clone());
        }
    }
    if alternative_nodes.len() >= MIN_CANDIDATES {
        let mut cur = top.node.clone();
        loop {
            cur = cur.parent()?.clone();
            if Some(&local_name!("body")) == dom::get_tag_name(&cur) {
                break;
            }

            let mut lists_containing_ancestor = 0;
            for alt in alternative_nodes.iter() {
                for ancestor in alt.ancestors() {
                    if cur == ancestor {
                        lists_containing_ancestor += 1;
                        break;
                    }
                }
            }
            if lists_containing_ancestor >= MIN_CANDIDATES {
                return Some(cur);
            }
        }
    }
    None
}

/// Iterates through the siblings of the top candidate and appends related content. Having the same
/// class name as the parent is a bonus, same with dense text nodes.
pub fn append_related_siblings(dom: &mut Sink, top_candidate: Handle) {
    if let Some(top_elem) = top_candidate.as_element() {
        if let Some(parent) = top_candidate.parent() {
            let mut related_siblings = vec![];
            let top_attrs = top_elem.attributes.borrow();
            let top_class = top_attrs.get("class");
            let content_bonus = top_elem.score.get() * 0.2;
            for sibling in parent.children() {
                if let Some(elem) = sibling.as_element() {
                    top_class
                        .and_then(|top_class| {
                            elem.attributes.borrow().get("class").and_then(|class| {
                                if class == top_class {
                                    Some(content_bonus)
                                } else {
                                    None
                                }
                            })
                        })
                        .unwrap_or(0.0);

                    let mut append = false;
                    if Some(&local_name!("p")) == dom::get_tag_name(&sibling) {
                        let link_density = get_link_density(&sibling);
                        let content_length = dom::text_len(&sibling);
                        if content_length > 80 && link_density < 0.25 {
                            append = true;
                        } else if content_length > 0 && content_length < 80 && link_density == 0.0 {
                            // NOTE: leaving out the condition for excluding for /\.( |$)/
                            append = true;
                        }
                    }

                    if append {
                        if ALTER_TO_DIV_EXCEPTIONS
                            .iter()
                            .any(|&tag| tag == &elem.name.local)
                        {
                            let new_elem = Handle::new_element(
                                QualName::new(None, ns!(), local_name!("div")),
                                elem.attributes.borrow().map.clone(),
                            );
                            dom.reparent_children(&sibling, &new_elem);
                            related_siblings.push(new_elem);
                        } else {
                            related_siblings.push(sibling);
                        }
                    }
                }
            }

            for sibling in related_siblings {
                top_candidate.append(sibling);
            }
        }
    }
}

/// decides whether the handle node is useless (should be dropped) or not.
pub fn clean<S: ::std::hash::BuildHasher>(
    mut dom: &mut Sink,
    handle: Handle,
    url: &Url,
    title: &str,
    features: &HashMap<String, u32, S>,
) -> bool {
    let useless = match handle.data() {
        Document(_) => false,
        DocumentFragment => false,
        Doctype(_) => false,
        Text(_) => false,
        Comment(_) => true,
        Element(ref data) => {
            match data.name.local {
                local_name!("script")
                | local_name!("link")
                | local_name!("style")
                | local_name!("noscript")
                | local_name!("meta")
                | local_name!("iframe")
                | local_name!("object")
                | local_name!("header")
                | local_name!("footer")
                | local_name!("embed")
                | local_name!("aside") => true,
                local_name!("form")
                | local_name!("table")
                | local_name!("ul")
                | local_name!("div") => is_useless(&handle),
                local_name!("img") => {
                    if let Some(fixed_url) = fix_img_path(data, url) {
                        dom::set_attr("src", fixed_url.as_str(), handle.clone(), false);
                        false
                    } else {
                        true
                    }
                }
                _ => false,
            }

            // // cleans all ids, classes and styles in node
            // dom::clean_attr("id", &mut *attrs.borrow_mut());
            // dom::clean_attr("class", &mut *attrs.borrow_mut());
            // dom::clean_attr("style", &mut *attrs.borrow_mut());
        }
        ProcessingInstruction(_) => unreachable!(),
    };

    let mut useless_nodes = vec![];
    for child in handle.children() {
        if clean(&mut dom, child.clone(), url, title, features) {
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

/// Using content score and other heuristics, determine if the handle should be marked for
/// deletion.
pub fn is_useless(handle: &Handle) -> bool {
    let tag_name = dom::get_tag_name(&handle);
    let weight = get_class_weight(&handle);
    let score = handle.as_element().map(|e| e.score.get()).unwrap_or(0.0);
    if weight + score < 0.0 {
        return true;
    }

    let content_length = dom::text_len(&handle);
    let para_count =
        dom::count_nodes(&handle, &local_name!("p")) + dom::text_children_count(&handle) as u32;

    let mut is_list = tag_name == Some(&local_name!("ul")) || tag_name == Some(&local_name!("ol"));
    if !is_list {
        let list_nodes = dom::find_nodes_with_tag(handle, &["ul", "ol"]);

        let mut list_length = 0.0;
        for node in list_nodes {
            let mut text = String::new();
            dom::extract_text(&node, &mut text, true);
            list_length += text.chars().count() as f32;
        }
        is_list = list_length / content_length as f32 > 0.9;
    }

    let input_count = dom::count_nodes(&handle, &local_name!("input"));
    if input_count as f32 > f32::floor(para_count as f32 / 3.0) {
        return true;
    }

    let link_density = get_link_density(handle);
    if weight >= 25.0 && link_density > 0.5 {
        return true;
    }

    let img_count = dom::count_nodes(&handle, &local_name!("img"));

    if !is_list {
        let li_count = dom::count_nodes(&handle, &local_name!("li")) as i32 - 100;
        if li_count > para_count as i32 {
            return true;
        }

        if weight < 25.0 && link_density > 0.2 {
            return true;
        }

        let heading_density = get_text_density(&handle, &["h1", "h2", "h3", "h4", "h5", "h6"]);
        if heading_density < 0.9 && content_length < 25 && (img_count == 0 || img_count > 2) {
            return true;
        }
    }

    let embed_count = dom::count_nodes(&handle, &local_name!("embed"));
    if (embed_count == 1 && content_length < 75) || embed_count > 1 {
        return true;
    }
    false
}
