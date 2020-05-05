use markup5ever_rcdom::NodeData::{Element, Text};
use markup5ever_rcdom::{Handle, Node};
use html5ever::tendril::StrTendril;
use html5ever::Attribute;
use html5ever::LocalName;
use std::rc::Rc;
use std::str::FromStr;

pub fn get_tag_name<'a>(handle: &'a Handle) -> Option<&'a LocalName> {
    match handle.data {
        Element { ref name, .. } => Some(&name.local),
        _ => None,
    }
}

pub fn get_attr(name: &str, handle: &Handle) -> Option<String> {
    match handle.data {
        Element { ref attrs, .. } => attr(name, &attrs.borrow()),
        _ => None,
    }
}

pub fn attr(attr_name: &str, attrs: &[Attribute]) -> Option<String> {
    for attr in attrs.iter() {
        if attr.name.local.as_ref() == attr_name {
            return Some(attr.value.to_string());
        }
    }
    None
}

pub fn set_attr(attr_name: &str, value: &str, handle: Handle) {
    if let Element { ref attrs, .. } = handle.data {
        let attrs = &mut attrs.borrow_mut();
        if let Some(index) = attrs.iter().position(|attr| {
            let name = attr.name.local.as_ref();
            name == attr_name
        }) {
            if let Ok(value) = StrTendril::from_str(value) {
                attrs[index] = Attribute {
                    name: attrs[index].name.clone(),
                    value,
                }
            }
        }
    }
}

pub fn clean_attr(attr_name: &str, attrs: &mut Vec<Attribute>) {
    if let Some(index) = attrs.iter().position(|attr| {
        let name = attr.name.local.as_ref();
        name == attr_name
    }) {
        attrs.remove(index);
    }
}

pub fn is_empty(handle: &Handle) -> bool {
    for child in handle.children.borrow().iter() {
        match child.data {
            Text { ref contents } => {
                if contents.borrow().trim().len() > 0 {
                    return false;
                }
            }
            Element { ref name, .. } => {
                let tag_name = name.local.as_ref();
                match tag_name.to_lowercase().as_ref() {
                    "li" | "dt" | "dd" | "p" | "div" => {
                        if !is_empty(child) {
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
    for child in handle.children.borrow().iter() {
        if has_link(child) {
            return true;
        }
    }
    false
}

pub fn extract_text(handle: &Handle, text: &mut String, deep: bool) {
    for child in handle.children.borrow().iter() {
        match child.data {
            Text { ref contents } => {
                text.push_str(contents.borrow().trim());
            }
            Element { .. } => {
                if deep {
                    extract_text(child, text, deep);
                }
            }
            _ => (),
        }
    }
}

pub fn text_len(handle: &Handle) -> usize {
    let mut len = 0;
    for child in handle.children.borrow().iter() {
        match child.data {
            Text { ref contents } => {
                len += contents.borrow().trim().chars().count();
            }
            Element { .. } => {
                len += text_len(child);
            }
            _ => (),
        }
    }
    len
}

pub fn find_node(handle: &Handle, tag_name: &str, nodes: &mut Vec<Rc<Node>>) {
    for child in handle.children.borrow().iter() {
        if let Element { ref name, .. } = child.data {
            let t = name.local.as_ref();
            if t.to_lowercase() == tag_name {
                nodes.push(child.clone());
            };
            find_node(child, tag_name, nodes)
        }
    }
}

pub fn count_nodes(handle: &Handle, tag_name: &LocalName) -> u32 {
    let mut count = match handle.data {
        Element { ref name, .. } if &name.local == tag_name => 1,
        _ => 0,
    };

    for child in handle.children.borrow().iter() {
        count += count_nodes(child, tag_name);
    }
    count
}

pub fn has_nodes(handle: &Handle, tag_names: &[&'static LocalName]) -> bool {
    for child in handle.children.borrow().iter() {
        if let Some(tag_name) = get_tag_name(child) {
            if tag_names.iter().any(|n| n == &tag_name) {
                return true;
            }
        }
        
        if match child.data {
            Element { .. } => has_nodes(child, tag_names),
            _ => false,
        } {
            return true;
        }
    }
    false
}

pub fn text_children_count(handle: &Handle) -> usize {
    let mut count = 0;
    for child in handle.children.borrow().iter() {
        match child.data {
            Text { ref contents } => {
                if contents.borrow().trim().len() >= 20 {
                    count += 1
                }
            }
            Element { .. } => count += text_children_count(child),
            _ => (),
        }
    }
    count
}
