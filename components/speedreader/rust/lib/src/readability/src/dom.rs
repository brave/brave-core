use crate::html5ever::tree_builder::TreeSink;
use html5ever::tendril::StrTendril;
use html5ever::tendril::TendrilSink;
use html5ever::{parse_document, ParseOpts};
use html5ever::{Attribute, LocalName, QualName};
use kuchiki::NodeData::{Comment, Element, Text};
use kuchiki::NodeRef as Handle;
use kuchiki::Sink;
use std::str::FromStr;

#[inline]
pub fn get_tag_name<'a>(handle: &'a Handle) -> Option<&'a LocalName> {
    match handle.data() {
        Element(ref value) => Some(&value.name.local),
        _ => None,
    }
}

#[inline]
pub fn clone_element_steal_children(
    sink: &mut Sink,
    node: &Handle,
    tag: LocalName,
) -> Option<Handle> {
    if let Some(old_elem) = node.as_element() {
        let new_elem = Handle::new_element(
            QualName::new(None, ns!(), tag),
            old_elem.attributes.borrow().map.clone(),
        );
        sink.reparent_children(node, &new_elem);
        Some(new_elem)
    } else {
        None
    }
}

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

#[inline]
pub fn clean_attr(attr_name: &str, attrs: &mut Vec<Attribute>) {
    if let Some(index) = attrs.iter().position(|attr| {
        let name = attr.name.local.as_ref();
        name == attr_name
    }) {
        attrs.remove(index);
    }
}

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
                    "li" | "dt" | "dd" | "p" | "div" => {
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
        | Some(&local_name!("canvas")) => true,
        _ => false,
    }
}

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

pub fn extract_text(handle: &Handle, text: &mut String, deep: bool) {
    for child in handle.children() {
        match child.data() {
            Text(ref contents) => {
                text.push_str(contents.borrow().trim());
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

pub fn text_len(handle: &Handle) -> usize {
    let mut len = 0;
    for child in handle.children() {
        match child.data() {
            Text(ref contents) => {
                len += contents.borrow().trim().chars().count();
            }
            Element(_) => {
                len += text_len(&child);
            }
            _ => (),
        }
    }
    len
}

#[inline]
pub fn find_nodes_with_tag(handle: &Handle, tags: &[&str], nodes: &mut Vec<Handle>) {
    for tag in tags {
        find_node(handle, tag, nodes);
    }
}

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

#[inline]
pub fn has_single_tag_inside_element(handle: &Handle, tag: &LocalName) -> Option<Handle> {
    let mut elems = handle
        .children()
        .filter(|child| child.as_element().is_some());
    if elems.clone().count() == 1 {
        let only_child = elems.nth(0)?;
        if get_tag_name(&only_child) == Some(tag) {
            return Some(only_child);
        }
    }
    None
}

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

#[inline]
pub fn parse_inner(contents: &str) -> Option<Handle> {
    let dom = parse_document(Sink::default(), ParseOpts::default()).one(contents);
    let html = dom.document_node.first_child()?;
    let body = html.first_child()?.next_sibling()?;
    body.first_child()
}

pub fn is_single_image(handle: &Handle) -> bool {
    match handle.data() {
        Element(ref data) if data.name.local == local_name!("img") => return true,
        Text(ref contents) if !contents.borrow().trim().is_empty() => return false,
        Comment(_) => (),
        _ => {
            return false;
        }
    }

    if let Some(ref first) = handle.first_child() {
        return is_single_image(first);
    } else {
        return false;
    }
}
