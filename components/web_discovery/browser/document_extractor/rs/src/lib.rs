/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use std::collections::HashMap;

use cxx::{CxxString, CxxVector};
use html5ever::tree_builder::TreeSink;
use kuchikiki::{
    iter::{Descendants, Elements, Select},
    parse_html,
    traits::TendrilSink,
};

#[allow(unsafe_op_in_unsafe_fn)]
#[cxx::bridge(namespace = "rust_document_extractor")]
mod ffi {
    pub struct SelectAttributeRequest {
        pub sub_selector: String,
        pub key: String,
        pub attribute: String,
    }

    pub struct SelectRequest {
        pub root_selector: String,
        pub attribute_requests: Vec<SelectAttributeRequest>,
    }

    pub struct AttributeResult {
        pub key: String,
        pub attribute_values: Vec<String>,
    }

    extern "Rust" {
        fn query_element_attributes(
            html: &CxxString,
            requests: &CxxVector<SelectRequest>,
        ) -> Vec<AttributeResult>;
    }
}

use ffi::*;

const TEXT_CONTENT_ATTRIBUTE_NAME: &str = "textContent";

fn extract_attributes_from_nodes(
    attribute_requests: &[SelectAttributeRequest],
    nodes: Select<Elements<Descendants>>,
) -> HashMap<String, Vec<String>> {
    let mut result: HashMap<String, Vec<String>> = HashMap::new();
    for node in nodes {
        for attribute_request in attribute_requests {
            let sub_node = match attribute_request.sub_selector.is_empty() {
                false => match node.as_node().select_first(&attribute_request.sub_selector) {
                    Ok(e) => Some(e),
                    Err(_) => continue,
                },
                true => None,
            };
            let node_to_query = sub_node.as_ref().unwrap_or(&node).as_node();

            let attribute_value = match attribute_request.attribute == TEXT_CONTENT_ATTRIBUTE_NAME {
                true => Some(node_to_query.text_contents()),
                false => node_to_query.as_element().and_then(|element| {
                    let attributes = element.attributes.borrow();
                    attributes.get(attribute_request.attribute.as_str()).map(|v| v.to_string())
                }),
            };
            if let Some(value) = attribute_value {
                result.entry(attribute_request.key.clone()).or_default().push(value);
            }
        }
    }
    result
}

pub fn query_element_attributes(
    html: &CxxString,
    requests: &CxxVector<SelectRequest>,
) -> Vec<AttributeResult> {
    let mut sink = parse_html().one(html.to_str().unwrap_or_default());
    let mut result = Vec::new();
    let document = sink.get_document();
    for request in requests {
        if let Ok(nodes) = document.select(&request.root_selector) {
            result.extend(
                extract_attributes_from_nodes(&request.attribute_requests, nodes)
                    .into_iter()
                    .map(|(key, attribute_values)| AttributeResult { key, attribute_values }),
            );
        }
    }
    result
}
