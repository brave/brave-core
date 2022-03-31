use crate::html5ever::tree_builder::TreeSink;
use crate::util::count_ignore_consecutive_whitespace;
use html5ever::tendril::StrTendril;
use html5ever::tendril::TendrilSink;
use html5ever::tree_builder::{ElementFlags, NodeOrText};
use html5ever::{parse_document, ParseOpts};
use html5ever::{Attribute, LocalName, QualName};
use kuchiki::iter::NodeIterator;
use kuchiki::NodeData::{Element, Text};
use kuchiki::NodeRef as Handle;
use kuchiki::Sink;
use std::str::FromStr;

/// A small wrapper function that creates a NodeOrText from a Text handle or an Element handle.
#[inline]
pub fn node_or_text(handle: Handle) -> Option<NodeOrText<Handle>> {
    match handle.data() {
        Text(ref data) => match StrTendril::from_str(&data.borrow()) {
            Ok(tendril) => Some(NodeOrText::AppendText(tendril)),
            _ => None,
        },
        Element(_) => Some(NodeOrText::AppendNode(handle)),
        _ => None,
    }
}

/// Returns the tag name of a DOM element.
#[inline]
pub fn get_tag_name<'a>(handle: &'a Handle) -> Option<&'a LocalName> {
    match handle.data() {
        Element(ref value) => Some(&value.name.local),
        _ => None,
    }
}

/// Overrite the value of an element attribute, optionally creating the attribute if it doesn't exist.
#[inline]
pub fn set_attr<S>(attr_name: &str, attr_value: S, handle: Handle, create_if_missing: bool)
where
    S: Into<String>,
{
    if let Some(data) = handle.as_element() {
        let attrs = &mut data.attributes.borrow_mut();
        if let Some(value) = attrs.get_mut(attr_name) {
            *value = attr_value.into();
        } else {
            if create_if_missing {
                attrs.insert(attr_name, attr_value.into());
            }
        }
    }
}

/// Append a new attribute to an element. It is assumed the attribute doesn't exist. If you aren't
/// sure about that, use set_attr(..) instead.
#[inline]
pub fn append_attr(attr_name: &str, value: &str, attrs: &mut Vec<Attribute>) {
    if let Ok(value) = StrTendril::from_str(value) {
        let new_attr = Attribute {
            name: QualName::new(None, ns!(), LocalName::from(attr_name)),
            value: value,
        };
        attrs.push(new_attr);
    }
}

/// Remove an attribute from an element's list.
#[inline]
pub fn clean_attr(attr_name: &str, attrs: &mut Vec<Attribute>) {
    if let Some(index) = attrs.iter().position(|attr| {
        let name = attr.name.local.as_ref();
        name == attr_name
    }) {
        attrs.remove(index);
    }
}

/// Get HTML <head> from a document
#[inline]
pub fn document_head(dom: &Sink) -> Option<Handle> {
    let html = dom
        .document_node
        .children()
        .find(|child| get_tag_name(&child) == Some(&local_name!("html")));
    html?
        .children()
        .find(|child| get_tag_name(&child) == Some(&local_name!("head")))
}

/// Get HTML <body> from a document
#[inline]
pub fn document_body(dom: &Sink) -> Option<Handle> {
    let html = dom
        .document_node
        .children()
        .find(|child| get_tag_name(&child) == Some(&local_name!("html")));
    html?
        .children()
        .find(|child| get_tag_name(&child) == Some(&local_name!("body")))
}

/// Recursively checks if an element or any of its children have text content.
pub fn is_empty(handle: &Handle) -> bool {
    for child in handle.children() {
        match child.data() {
            Text(ref text) => {
                if text.borrow().trim().len() > 0 {
                    return false;
                }
            }
            Element(ref value) => {
                let tag_name = value.name.local.as_ref();
                match tag_name.to_lowercase().as_ref() {
                    "li" | "dt" | "dd" | "p" | "div" | "span" | "a" => {
                        if !is_empty(&child) {
                            return false;
                        }
                    }
                    _ => return false,
                }
            }
            _ => (),
        }
    }
    match get_tag_name(&handle) {
        Some(&local_name!("li"))
        | Some(&local_name!("dt"))
        | Some(&local_name!("dd"))
        | Some(&local_name!("p"))
        | Some(&local_name!("div"))
        | Some(&local_name!("span"))
        | Some(&local_name!("canvas")) => true,
        _ => false,
    }
}

pub fn is_whitespace(handle: &Handle) -> bool {
    match handle.data() {
        Text(ref text) => text.borrow().trim().len() == 0,
        Element(ref data) => data.name.local == local_name!("br"),
        _ => false,
    }
}

/// Checks if the tree rooted at handle contains an <a> element.
pub fn has_link(handle: &Handle) -> bool {
    match get_tag_name(&handle) {
        Some(&local_name!("a")) => return true,
        _ => (),
    }
    for child in handle.children() {
        if has_link(&child) {
            return true;
        }
    }
    false
}

/// Returns all text data for an element, optionally traversing the entire tree rooted at handle.
pub fn extract_text(handle: &Handle, text: &mut String, deep: bool) {
    debug_assert!(
        handle.as_element().is_some(),
        "extract_text() should be called on Element"
    );
    for child in handle.children() {
        match child.data() {
            Text(ref contents) => {
                if contents.borrow().trim().len() > 0 {
                    text.push_str(&contents.borrow());
                }
            }
            Element(_) => {
                if deep {
                    extract_text(&child, text, deep);
                }
            }
            _ => (),
        }
    }
}

/// Similar to extract_text(), but callers can pass in either an Element or a
/// Text variant. If maybe_include_root is provided and the node is Text, the
/// result will be returned immediately; otherwise behaves like extract_text().
#[inline]
pub fn extract_text_from_node(handle: &Handle, maybe_include_root: bool, deep: bool) -> String {
    if maybe_include_root {
        if let Some(ref contents) = handle.as_text() {
            return contents.borrow().trim().to_string();
        }
    }
    let mut text = String::new();
    extract_text(handle, &mut text, deep);
    text
}

/// Returns the length of all text in the tree rooted at handle.
pub fn text_len(handle: &Handle) -> usize {
    let mut len: usize = 0;
    for contents in handle.descendants().text_nodes() {
        len += count_ignore_consecutive_whitespace(contents.borrow().trim().chars());
    }
    len
}

/// Find all nodes with tags in the tree rooted at handle.
#[inline]
pub fn find_nodes_with_tag(handle: &Handle, tags: &[&str]) -> Vec<Handle> {
    let mut nodes = vec![];
    for tag in tags {
        find_node(handle, tag, &mut nodes);
    }
    nodes
}

/// Find all nodes with tags in the tree rooted at handle.
pub fn find_node(handle: &Handle, tag_name: &str, nodes: &mut Vec<Handle>) {
    for child in handle.children() {
        if let Some(data) = child.as_element() {
            let t = data.name.local.as_ref();
            if t.to_lowercase() == tag_name {
                nodes.push(child.clone());
            };
            find_node(&child, tag_name, nodes)
        }
    }
}

/// Count all nodes with tag "tag_name" in the tree rooted at handle.
pub fn count_nodes(handle: &Handle, tag_name: &LocalName) -> u32 {
    let mut count = match handle.data() {
        Element(ref data) if &data.name.local == tag_name => 1,
        _ => 0,
    };

    for child in handle.children() {
        count += count_nodes(&child, tag_name);
    }
    count
}

/// Returns true if the tree rooted at handle contains any tag in "tag_names".
pub fn has_nodes(handle: &Handle, tag_names: &[&'static LocalName]) -> bool {
    for child in handle.children() {
        if let Some(tag_name) = get_tag_name(&child) {
            if tag_names.iter().any(|n| n == &tag_name) {
                return true;
            }
        }

        if match child.data() {
            Element(_) => has_nodes(&child, tag_names),
            _ => false,
        } {
            return true;
        }
    }
    false
}

/// Returns the first child of handle by tag. If handle has more than one child, don't return
/// anything. This is usually used for unwrapping divs.
#[inline]
pub fn get_only_child_by_tag(handle: &Handle, tag: &LocalName) -> Option<Handle> {
    let mut elems = handle.children().filter(|child| !is_whitespace(&child));
    if elems.clone().count() == 1 {
        let only_child = elems.nth(0)?;
        if get_tag_name(&only_child) == Some(tag) {
            return Some(only_child);
        }
    }
    None
}

/// Returns the number of text in elements in the subtree rooted at handle. A text element is only
/// considered if it has 20 or more characters.
pub fn text_children_count(handle: &Handle) -> usize {
    let mut count = 0;
    for child in handle.children() {
        match child.data() {
            Text(ref contents) => {
                if contents.borrow().trim().len() >= 20 {
                    count += 1
                }
            }
            Element(_) => count += text_children_count(&child),
            _ => (),
        }
    }
    count
}

/// Get the previous sibling of an element.
#[inline]
pub fn previous_element_sibling(handle: &Handle) -> Option<Handle> {
    let mut curr = handle.previous_sibling()?;
    loop {
        if let Some(_) = curr.as_element() {
            return Some(curr.clone());
        }
        curr = curr.previous_sibling()?;
    }
}

/// Parse HTML data encoded as text.
#[inline]
pub fn parse_inner(contents: &str) -> Option<Handle> {
    let dom = parse_document(Sink::default(), ParseOpts::default()).one(contents);
    let html = dom.document_node.first_child()?;
    let body = html.first_child()?.next_sibling()?;
    body.first_child()
}

/// Returns true if the handle contains only an image with no text content.
pub fn is_single_image(handle: &Handle) -> bool {
    if let Some(ref data) = handle.as_element() {
        if data.name.local == local_name!("img") {
            return true;
        }
        let len = text_len(&handle);
        let num_children = handle.children().elements().count();
        if num_children == 1 && len == 0 {
            return is_single_image(&handle.first_child().unwrap());
        }
    }
    false
}

/// HTML decode runs the text through an HTML parser and extracts the text.
pub fn html_decode(data: &str) -> Option<String> {
    if let Some(ref inner) = parse_inner(data) {
        let extracted = extract_text_from_node(inner, true, true);
        if !extracted.is_empty() {
            return Some(extracted);
        }
    }
    None
}

// The commented out elements qualify as phrasing content but tend to be
// removed by readability when put into paragraphs, so we ignore them here.
static PHRASING_ELEMS: [&LocalName; 39] = [
    // "canvas", "iframe", "svg", "video",
    &local_name!("abbr"),
    &local_name!("audio"),
    &local_name!("b"),
    &local_name!("bdo"),
    &local_name!("br"),
    &local_name!("button"),
    &local_name!("cite"),
    &local_name!("code"),
    &local_name!("data"),
    &local_name!("datalist"),
    &local_name!("dfn"),
    &local_name!("em"),
    &local_name!("embed"),
    &local_name!("i"),
    &local_name!("img"),
    &local_name!("input"),
    &local_name!("kbd"),
    &local_name!("label"),
    &local_name!("mark"),
    &local_name!("math"),
    &local_name!("meter"),
    &local_name!("noscript"),
    &local_name!("object"),
    &local_name!("output"),
    &local_name!("progress"),
    &local_name!("q"),
    &local_name!("ruby"),
    &local_name!("samp"),
    &local_name!("script"),
    &local_name!("select"),
    &local_name!("small"),
    &local_name!("span"),
    &local_name!("strong"),
    &local_name!("sub"),
    &local_name!("sup"),
    &local_name!("textarea"),
    &local_name!("time"),
    &local_name!("var"),
    &local_name!("wbr"),
];

/// Returns true if a handle qualifies as phrasing content.
/// https://developer.mozilla.org/en-US/docs/Web/Guide/HTML/Content_categories#Phrasing_content
pub fn is_phrasing_content(handle: &Handle) -> bool {
    match handle.data() {
        Text(_) => return true,
        Element(ref data) => {
            let tag = &data.name.local;
            if PHRASING_ELEMS.iter().any(|&t| t == tag) {
                return true;
            }

            // We can't include these in PHRASING_ELEMS because they can contain
            // non-pharsing elements inside of them, so we search recursively.
            if tag == &local_name!("a")
                || tag == &local_name!("del")
                || tag == &local_name!("ins")
                || tag == &local_name!("font")
            {
                for c in handle.children() {
                    if !is_phrasing_content(&c) {
                        return false;
                    }
                }
                true
            } else {
                false
            }
        }
        _ => false,
    }
}

#[inline]
pub fn create_element_simple(
    dom: &mut Sink,
    local_name: &str,
    classes: &str,
    content: Option<&str>,
) -> Handle {
    let name = QualName::new(None, ns!(), LocalName::from(local_name));
    let class_attr = Attribute {
        name: QualName::new(None, ns!(), local_name!("class")),
        value: StrTendril::from_str(classes).unwrap_or_default(),
    };
    let elem = dom.create_element(name, vec![class_attr], ElementFlags::default());
    if let Some(text) = content {
        dom.append(
            &elem,
            NodeOrText::AppendText(StrTendril::from_str(&text).unwrap_or_default()),
        );
    }
    elem
}
