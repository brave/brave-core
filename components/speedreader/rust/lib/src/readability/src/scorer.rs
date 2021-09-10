use crate::{dom, util};
use html5ever::tendril::StrTendril;
use html5ever::tree_builder::TreeSink;
use html5ever::tree_builder::{ElementFlags, NodeOrText};
use html5ever::{LocalName, QualName};
use kuchiki::iter::NodeIterator;
use kuchiki::NodeData::{
    Comment, Doctype, Document, DocumentFragment, Element, ProcessingInstruction, Text,
};
use kuchiki::NodeRef as Handle;
use kuchiki::{ElementData, Sink};
use regex::Regex;
use std::cmp::Ordering;
use std::collections::{HashMap, HashSet};
use std::str::FromStr;
use url::Url;

pub static PUNCTUATIONS_REGEX: &str = r"([,]\?)";
pub static UNLIKELY_CANDIDATES: &str = "(?i)-ad-|ai2html|banner\
    |breadcrumbs|combx|comment|community|cover-wrap|disqus|extra|foot|gdpr\
    |header|legends|menu|related|remark|replies|rss|shoutbox|sidebar|skyscraper\
    |social|sponsor|supplemental|ad-break|agegate|pagination|pager|popup\
    |masthead|yom-remote";
pub static LIKELY_CANDIDATES: &str = "(?i)and|article|body|column|content|main\
    |shadow";
pub static POSITIVE_CANDIDATES: &str = "(?i)article|body|content|entry|hentry|h-entry|\
        main|page|pagination|post|text|blog|story";
pub static NEGATIVE_CANDIDATES: &str = "(?i)-ad-|hidden|^hid$| hid$| hid |\
        ^hid |banner|combx|comment|com-|contact|foot|footer|footnote|gdpr|\
        masthead|media|meta|outbrain|promo|related|scroll|share|shoutbox|\
        sidebar|skyscraper|sponsor|shopping|tags|tool|widget|ai2html|legends|\
        social|sponsor|ad-break";
pub static HTML_TAGS: &str = r"<[^>]*>";
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
static LOADABLE_IMG_SUFFIX: [&str; 4] = [".jpg", ".jpeg", ".png", ".webp"];
static PRESENTATIONAL_ATTRIBUTES: [&LocalName; 12] = [
    &local_name!("align"),
    &local_name!("background"),
    &local_name!("bgcolor"),
    &local_name!("border"),
    &local_name!("cellpadding"),
    &local_name!("cellspacing"),
    &local_name!("frame"),
    &local_name!("hspace"),
    &local_name!("rules"),
    &local_name!("style"),
    &local_name!("valign"),
    &local_name!("vspace"),
];

static DECAY_FACTOR: f32 = 3.0;

// The number of candidates to consider when choosing the "top" candidate. These
// top candidates are used in the alternative candidates part of the algorithm.
// This number is taken from the Mozilla implementation.
// https://github.com/mozilla/readability/blob/e2aea3121a9bb6e05478edc1596026c41c782779/Readability.js#L111
static NUM_TOP_CANDIDATES: usize = 5;

// The minimum score to be considered as a top candidate under strict heuristics.
static CANDIDATE_SCORE_THRESHOLD: f32 = 5.0;

lazy_static! {
    static ref PUNCTUATIONS: Regex = Regex::new(PUNCTUATIONS_REGEX).unwrap();
    static ref LIKELY: Regex = Regex::new(LIKELY_CANDIDATES).unwrap();
    static ref UNLIKELY: Regex = Regex::new(UNLIKELY_CANDIDATES).unwrap();
    static ref POSITIVE: Regex = Regex::new(POSITIVE_CANDIDATES).unwrap();
    static ref NEGATIVE: Regex = Regex::new(NEGATIVE_CANDIDATES).unwrap();
    static ref DECODED_HTML_TAGS: Regex = Regex::new(HTML_TAGS).unwrap();
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

bitflags::bitflags! {
    pub struct ImageLoadedMask: u8 {
        const SRC = 1 << 0;
        const SRCSET = 1 << 1;

        const NONE = 0;
    }
}
/// Returns true if the image src loads without JavaScript logic. Some sites like Kotaku and The
/// Atlantic try and get fancy with this. Our simple heuristic is to check if src is set something
/// reasonable or srcset is set at all. Some things we've seen in the wild are srcset being left
/// out in favor of data-srcset, and src being base64 encoded.
#[inline]
fn img_loaded_mask(data: &ElementData) -> ImageLoadedMask {
    let mut mask: ImageLoadedMask = ImageLoadedMask::NONE;
    if let Some(src) = data.attributes.borrow().get(local_name!("src")) {
        for suffix in LOADABLE_IMG_SUFFIX.iter() {
            if src.ends_with(suffix) {
                mask |= ImageLoadedMask::SRC;
                break;
            }
        }
        if !mask.contains(ImageLoadedMask::SRC) {
            // Try parsing the URL to ignore url params
            match Url::parse(src) {
                Ok(url) => {
                    for suffix in LOADABLE_IMG_SUFFIX.iter() {
                        if url.path().ends_with(suffix) {
                            mask |= ImageLoadedMask::SRC;
                            break;
                        }
                    }
                }
                _ => (),
            }
        }
    }

    if data
        .attributes
        .borrow()
        .get(local_name!("srcset"))
        .is_some()
    {
        mask |= ImageLoadedMask::SRCSET;
    }
    mask
}

/// Contains metadata about how to load a lazy loaded image.
struct LazyImage {
    local: LocalName,
    value: String,
}

/// Try and find attributes corresponding to a lazy loaded image and return the new attribute
/// metadata. Look for anything ending in *srcset (data-srcset is fairly common) or any element
/// with image suffixes.
fn try_lazy_img(
    data: &ElementData,
    mut mask: ImageLoadedMask,
) -> (ImageLoadedMask, Vec<LazyImage>) {
    let mut lazy_srcs: Vec<LazyImage> = Vec::with_capacity(2);
    for (name, value) in &data.attributes.borrow().map {
        if mask.is_all() {
            break;
        }
        if !mask.contains(ImageLoadedMask::SRCSET) {
            if name.local.as_ref().ends_with("-srcset") {
                mask |= ImageLoadedMask::SRCSET;
                lazy_srcs.push(LazyImage {
                    local: local_name!("srcset"),
                    value: value.value.clone(),
                });
            }
        }
        if !mask.contains(ImageLoadedMask::SRC) {
            if LOADABLE_IMG_SUFFIX
                .iter()
                .any(|suffix| value.value.ends_with(suffix))
            {
                mask |= ImageLoadedMask::SRC;
                lazy_srcs.push(LazyImage {
                    local: local_name!("src"),
                    value: value.value.clone(),
                });
            }
        }
    }
    (mask, lazy_srcs)
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
            local_name!("div") | local_name!("section") => 5.0,
            local_name!("h1")
            | local_name!("h2")
            | local_name!("h3")
            | local_name!("h4")
            | local_name!("th") => -5.0,
            local_name!("blockquote") => 3.0,
            local_name!("pre") => 3.0,
            local_name!("td") => 3.0,
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
    score += f32::min(mat.count() as f32, 3.0);
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

/// Do a subparse. The data inside of a noscript element is text. Send it through the parser and
/// return the result if it is one img element.
fn get_inner_img_from_noscript(handle: &Handle) -> Option<Handle> {
    let child = handle.first_child()?;
    if let Some(contents) = child.as_text() {
        let inner = dom::parse_inner(&contents.borrow())?;
        if dom::is_single_image(&inner) {
            if let Some(img) = dom::get_only_child_by_tag(&inner, &local_name!("img")) {
                return Some(img);
            } else {
                return Some(inner);
            }
        }
    }
    None
}

/// Unwrap a <noscript><img src=".."/></noscript> element to just be an img element. Sites like
/// Medium and BBC wrap img elements in a noscript on page load, and do this same process in
/// Javascript.
fn unwrap_noscript(handle: &Handle, useless_nodes: &mut Vec<Handle>) -> Option<Handle> {
    if let Some(img) = get_inner_img_from_noscript(handle) {
        for sibling in handle.preceding_siblings().elements() {
            if dom::is_single_image(&sibling.as_node()) {
                useless_nodes.push(sibling.as_node().clone());
            }
        }
        for sibling in handle.following_siblings().elements() {
            if dom::is_single_image(&sibling.as_node()) {
                useless_nodes.push(sibling.as_node().clone());
            }
        }
        return Some(img);
    }
    None
}

/// Normalizes the dom by replacing tags we aren't interested in, to reduce the
/// scoring set of elements we need to consider when cleaning and scoring.
pub fn replace_tags(dom: &mut Sink) {
    let mut replacements: Vec<(Handle, Handle)> = vec![];

    // Replace <font> elements with either <span> or <div> nodes. Since
    // Speedreader does it's own styling, <font> elements can cause problems for
    // us. Although <font> elements are inlined, there are notable sites that
    // have block elements as children. To get around this, we check its
    // children for non-phrasing content, and replace it with a div in that
    // case, or a span otherwise.
    for node in dom
        .document_node
        .descendants()
        .elements()
        .filter(|e| e.name.local == local_name!("font"))
    {
        let h = node.as_node();
        let local: LocalName;
        if dom::is_phrasing_content(&h) {
            local = local_name!("span");
        } else {
            local = local_name!("div");
        }
        let name = QualName::new(None, ns!(), local);
        let span = dom.create_element(name, vec![], ElementFlags::default());
        replacements.push((Handle::clone(&h), span));
    }
    for t in replacements {
        dom.reparent_children(&t.0, &t.1);
        dom.append_before_sibling(&t.0, NodeOrText::AppendNode(t.1));
        dom.remove_from_parent(&t.0);
    }
}

/// Prepare the DOM for the candidate and cleaning steps. Delete "noisy" nodes and do small
/// transformations.
pub fn preprocess(mut dom: &mut Sink, handle: Handle) -> bool {
    if let Some(data) = handle.as_element() {
        match data.name.local {
            local_name!("noscript")
            | local_name!("link")
            | local_name!("nav")
            | local_name!("style")
            | local_name!("title")
            | local_name!("script")
            | local_name!("meta") => return true,
            _ => (),
        }
    }
    let mut useless_nodes = vec![];
    let mut paragraph_nodes = vec![];
    let mut brs = vec![];
    for child in handle.children() {
        let pending_removal = preprocess(&mut dom, child.clone());
        if pending_removal {
            useless_nodes.push(child.clone());
        }

        // These are pre-processing steps that don't just delete nodes, but also append nodes to
        // their parent.
        match child.data() {
            Element(data) => {
                match data.name.local {
                    local_name!("br") => brs.push(child.clone()),
                    _ => {
                        if brs.len() >= 2 {
                            useless_nodes.extend(brs);
                        }
                        brs = Vec::new();
                    }
                }
                if data.name.local == local_name!("noscript") {
                    if let Some(unwrapped) = unwrap_noscript(&child, &mut useless_nodes) {
                        dom.append_before_sibling(&child, NodeOrText::AppendNode(unwrapped));
                    }
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
                if !s.trim().is_empty() {
                    if brs.len() >= 2 {
                        paragraph_nodes.push(child.clone());
                    }
                    brs = Vec::new();
                }
            }
            _ => (),
        }
    }
    for node in useless_nodes.iter() {
        dom.remove_from_parent(node);
    }

    if dom::get_tag_name(&handle) == Some(&local_name!("div")) {
        // Convert all divs whose children contains only phrasing
        // content to paragraphs. An example would look like this:
        //      <div>Here is a <a>link</a> <br></div>
        //      <p>Here is a <a>link</a> <br></p>
        let trim_whitespace = |dom: &mut Sink, p: &Handle| {
            while let Some(child) = p.last_child() {
                if !dom::is_whitespace(&child) {
                    break;
                }
                dom.remove_from_parent(&child);
            }
        };
        let mut last_p: Option<Handle> = None;
        for child in handle.children() {
            if dom::is_phrasing_content(&child) {
                if let Some(ref p) = last_p {
                    if let Some(replacement) = dom::node_or_text(child.clone()) {
                        dom.remove_from_parent(&child);
                        dom.append(&p, replacement);
                    }
                } else if !dom::is_whitespace(&child) {
                    let name = QualName::new(None, ns!(), LocalName::from("p"));
                    let p = dom.create_element(name, vec![], ElementFlags::default());
                    dom.append_before_sibling(&child, NodeOrText::AppendNode(p.clone()));
                    dom.remove_from_parent(&child);
                    if let Some(replacement) = dom::node_or_text(child.clone()) {
                        dom.append(&p, replacement);
                    }
                    last_p = Some(p);
                }
            } else if let Some(ref p) = last_p {
                trim_whitespace(dom, p);
                last_p = None;
            }
        }
        if let Some(ref p) = last_p {
            trim_whitespace(dom, p);
        }
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
        for sibling in p.preceding_siblings() {
            if !dom::is_whitespace(&sibling) {
                break;
            }
            dom.remove_from_parent(&sibling);
        }
        for sibling in p.following_siblings() {
            // If we approach another <br><br> chain, we are encroaching on another paragraph.
            if dom::get_tag_name(&sibling) == Some(&local_name!("br")) {
                if let Some(next) = sibling.next_sibling() {
                    if dom::get_tag_name(&next) == Some(&local_name!("br")) {
                        break;
                    }
                }
            }
            if paragraph_nodes.contains(&sibling) {
                break;
            }
            if !dom::is_phrasing_content(&sibling) {
                break;
            }
            dom.remove_from_parent(&sibling);
            if let Some(a) = dom::node_or_text(sibling.clone()) {
                dom.append(&p, a);
            }
        }
        while let Some(child) = p.last_child() {
            if !dom::is_whitespace(&child) {
                break;
            }
            dom.remove_from_parent(&child);
        }
    }
    false
}

/// Find the candidate with the top score to be the new root.
pub fn get_top_candidate(
    dom: &Sink,
    root: &Handle,
    strip_unlikely_tags: bool,
) -> Result<Handle, std::io::Error> {
    // Now that the dom has been processed, get a set of potential dom
    // candidates and their scoring. `is_candidate` and `score` are attributes
    // of `ElementData` that get modified.
    find_candidates(dom, root, strip_unlikely_tags);

    let top_candidate: Handle;

    // scores all candidate nodes
    let mut top_candidates: Vec<TopCandidate> = vec![];
    for elem in dom
        .document_node
        .descendants()
        .elements()
        .filter(|e| e.is_candidate.get())
    {
        let node = elem.as_node();
        let score = elem.score.get() * (1.0 - get_link_density(&node));
        elem.score.set(score);

        if top_candidates.len() < NUM_TOP_CANDIDATES {
            top_candidates.push(TopCandidate {
                node: Handle::clone(node),
            });
        } else {
            let min_index = util::min_elem_index(&top_candidates);
            let min = &mut top_candidates[min_index];
            if score > min.score() {
                *min = TopCandidate {
                    node: Handle::clone(node),
                }
            }
        }
    }
    if top_candidates.len() == 0
        || dom::is_empty(&top_candidates[0].node)
        || top_candidates[0].score() < CANDIDATE_SCORE_THRESHOLD
    {
        if strip_unlikely_tags {
            // We didn't find a candidate. We may have been too aggressive
            // deleting nodes by tag. Let's re-run a more lax version of the
            // algorithm.
            return get_top_candidate(dom, root, false);
        }
        if let Some(body) = dom::document_body(&dom) {
            top_candidate = body;
            initialize_candidate(&top_candidate);
        } else {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidInput,
                "No candidates found.",
            ));
        }
    } else {
        let max_index = util::max_elem_index(&top_candidates);
        top_candidates.swap(0, max_index);
        if let Some(new_top) = search_alternative_candidates(&top_candidates) {
            top_candidate = new_top;
        } else {
            top_candidate = Handle::clone(&top_candidates[0].node);
        }
    }
    Ok(top_candidate)
}

/// Walk the DOM and mark all candidate nodes.
pub fn find_candidates(dom: &Sink, handle: &Handle, skip_unlikely: bool) {
    // is candidate iif length of the text in handle is larger than 20 words AND
    // its tag is `div`, `article`, `center`, `section` while not in containing
    // nodes in BLOCK_CHILD_TAGS

    if skip_unlikely
        && handle
            .as_element()
            .map(|data| {
                for attr_name in ["id", "class", "itemProp"].iter() {
                    if let Some(val) = data.attributes.borrow().get(*attr_name) {
                        if data.name.local != local_name!("body")
                            && data.name.local != local_name!("main")
                            && UNLIKELY.is_match(&val)
                            && !LIKELY.is_match(&val)
                        {
                            return true;
                        }
                    }
                }
                false
            })
            .unwrap_or(false)
    {
        return;
    }

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
        find_candidates(dom, &child, skip_unlikely);
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
            let sibling_threshold = f32::max(top_elem.score.get() * 0.2, 10.0);
            for sibling in parent.children() {
                if sibling == top_candidate {
                    continue;
                }
                if let Some(elem) = sibling.as_element() {
                    // Increase consideration if the sibling shares the same
                    // class name as the top scorer.
                    let bonus = match (top_class, elem.attributes.borrow().get("class")) {
                        (Some(c0), Some(c1)) if c0 == c1 => top_elem.score.get() * 0.2,
                        _ => 0.0,
                    };

                    let mut append = false;
                    if elem.score.get() + bonus >= sibling_threshold {
                        append = true;
                    } else if Some(&local_name!("p")) == dom::get_tag_name(&sibling) {
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
    title_tokens: &HashSet<&str>,
    url: &Url,
    features: &HashMap<String, u32, S>,
) -> bool {
    let useless = match handle.data() {
        Document(_) => false,
        DocumentFragment => false,
        Doctype(_) => false,
        Text(_) => false,
        Comment(_) => true,
        Element(ref data) => {
            let delete = match data.name.local {
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
                | local_name!("textarea")
                | local_name!("input")
                | local_name!("select")
                | local_name!("button")
                | local_name!("aside") => true,
                local_name!("h1") | local_name!("h2") => {
                    // Delete remaining headings that may be duplicates of the title.
                    let mut heading = String::new();
                    dom::extract_text(&handle, &mut heading, true);
                    if heading.is_empty() {
                        return true;
                    }
                    if !title_tokens.is_empty() {
                        let heading_tokens = heading.split_whitespace().collect::<HashSet<_>>();
                        let distance = title_tokens.difference(&heading_tokens).count() as f32;
                        let similarity = 1.0 - distance / title_tokens.len() as f32;
                        if similarity >= 0.75 {
                            return true;
                        }
                    }
                    if data.name.local == local_name!("h1") {
                        // Rewrite any h1 elements as h2s. The only h1 in the
                        // DOM should be the title.
                        let name = QualName::new(None, ns!(), LocalName::from("h2"));
                        let h2 = dom.create_element(name, vec![], ElementFlags::default());
                        dom.reparent_children(&handle, &h2);
                        dom.append_before_sibling(&handle, NodeOrText::AppendNode(h2));
                        true
                    } else {
                        false
                    }
                }
                local_name!("form")
                | local_name!("table")
                | local_name!("ul")
                | local_name!("section")
                | local_name!("div") => is_useless(&handle),
                local_name!("br") => {
                    if let Some(sibling) = handle.next_sibling() {
                        if dom::get_tag_name(&sibling) == Some(&local_name!("p")) {
                            return true;
                        }
                    }
                    false
                }
                local_name!("img") => {
                    let mask = img_loaded_mask(data);
                    let (mask, lazy_srcs) = try_lazy_img(data, mask);
                    if mask == ImageLoadedMask::NONE {
                        true
                    } else {
                        for src in lazy_srcs {
                            dom::set_attr(src.local.as_ref(), src.value, handle.clone(), true);
                        }
                        false
                    }
                }
                local_name!("svg") => {
                    // If the SVG has a parent that is a <figure> it is probably
                    // an icon to resize the image. This will not format
                    // correctly in speedreader since images are styled as
                    // "display: inline".
                    for ancestor in handle.ancestors().elements() {
                        if ancestor.name.local == local_name!("figure") {
                            return true;
                        }
                    }
                    false
                }
                _ => false,
            };
            if !delete {
                // Delete style, align, and other elements that will conflict with the Speedreader
                // stylesheet.
                let mut attrs = data.attributes.borrow_mut();
                for attr in PRESENTATIONAL_ATTRIBUTES.iter() {
                    attrs.remove(*attr);
                }
            }
            delete
        }
        ProcessingInstruction(_) => unreachable!(),
    };

    if useless {
        return true;
    }
    let mut useless_nodes = vec![];
    for child in handle.children() {
        if clean(&mut dom, child.clone(), title_tokens, url, features) {
            useless_nodes.push(child.clone());
        }
    }
    for node in useless_nodes.iter() {
        dom.remove_from_parent(node);
    }
    if dom::is_empty(&handle) {
        return true;
    }
    false
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

    let svg_count = dom::count_nodes(&handle, &local_name!("svg"));
    if svg_count > 1 && content_length < 75 {
        return true;
    }

    let embed_count = dom::count_nodes(&handle, &local_name!("embed"));
    if (embed_count == 1 && content_length < 75) || embed_count > 1 {
        return true;
    }
    false
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_load_lazy_images_data_srcset() {
        // This is a truncated <img> tag taken from Kotaku.
        let input = r#"
<img src="data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==" alt="Wipe that paint outta your eyes, 60 fps is on the way."
     data-srcset="https://i.kinja-img.com/gawker-media/image/upload/c_fill,f_auto,fl_progressive,g_center,h_80,pg_1,q_80,w_80/d3xoltqrhfcqilnauamm.jpg 80w"
     data-format="jpg" data-alt="Wipe that paint outta your eyes, 60 fps is on the way."
/>"#;
        let handle = dom::parse_inner(input).unwrap();
        let elem = handle.as_element().unwrap();
        assert_eq!(elem.name.local, local_name!("img"));
        assert_eq!(img_loaded_mask(elem), ImageLoadedMask::NONE);
        let (mask, _) = try_lazy_img(elem, ImageLoadedMask::NONE);
        assert_eq!(mask, ImageLoadedMask::SRCSET);
    }

    #[test]
    fn test_loaded_image_url_params() {
        // This is a truncated <img> tag taken from Kotaku.
        let input = r#"
<img alt="Hori Parata at his Pātaua farm, the place where he was born and grew up."
     class="gu-image" itemprop="contentUrl"
     src="https://i.guim.co.uk/img/media/ff786/master/4800.jpg?width=300&amp;quality=85&amp;auto=format&amp;fit=max&amp;s=575838a657b26493e956c7f84b058080">
"#;
        let handle = dom::parse_inner(input).unwrap();
        let elem = handle.as_element().unwrap();
        assert_eq!(elem.name.local, local_name!("img"));
        assert_eq!(img_loaded_mask(elem), ImageLoadedMask::SRC);
    }

    #[test]
    fn test_load_lazy_images_src_and_srcset() {
        // look for srcset even if we loaded the src
        let input = r#"
        <img src="https://some-site/some-image.jpg"
            data-srcset="https://some-site/some-image.jpg2000x2000"/>
        "#;
        let handle = dom::parse_inner(input).unwrap();
        let elem = handle.as_element().unwrap();
        assert_eq!(img_loaded_mask(elem), ImageLoadedMask::SRC);
        let (mask, _) = try_lazy_img(elem, ImageLoadedMask::NONE);
        assert!(mask.is_all());
    }
}
