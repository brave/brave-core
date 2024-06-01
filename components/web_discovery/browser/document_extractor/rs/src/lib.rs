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

    pub struct AttributePair {
        pub key: String,
        pub value: String,
    }

    pub struct AttributeResult {
        pub root_selector: String,
        pub attribute_pairs: Vec<AttributePair>,
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
    root_selector: &str,
    attribute_requests: &[SelectAttributeRequest],
    nodes: Select<Elements<Descendants>>,
    results: &mut Vec<AttributeResult>,
) {
    for node in nodes {
        let mut attribute_map = HashMap::new();
        for attribute_request in attribute_requests {
            let sub_node = match attribute_request.sub_selector.is_empty() {
                false => match node.as_node().select_first(&attribute_request.sub_selector) {
                    Ok(e) => Some(e),
                    Err(_) => {
                        attribute_map.insert(attribute_request.key.clone(), String::new());
                        continue;
                    }
                },
                true => None,
            };
            let node_to_query = sub_node.as_ref().unwrap_or(&node).as_node();

            let attribute_value = match attribute_request.attribute == TEXT_CONTENT_ATTRIBUTE_NAME {
                true => node_to_query.text_contents(),
                false => node_to_query
                    .as_element()
                    .and_then(|element| {
                        let attributes = element.attributes.borrow();
                        attributes.get(attribute_request.attribute.as_str()).map(|v| v.to_string())
                    })
                    .unwrap_or_default(),
            };
            attribute_map.insert(attribute_request.key.clone(), attribute_value);
        }
        results.push(AttributeResult {
            root_selector: root_selector.to_string(),
            attribute_pairs: attribute_map
                .into_iter()
                .map(|(key, value)| AttributePair { key, value })
                .collect(),
        });
    }
}

pub fn query_element_attributes(
    html: &CxxString,
    requests: &CxxVector<SelectRequest>,
) -> Vec<AttributeResult> {
    let mut sink = parse_html().one(html.to_str().unwrap_or_default());
    let mut results = Vec::new();
    let document = sink.get_document();
    for request in requests {
        if let Ok(nodes) = document.select(&request.root_selector) {
            extract_attributes_from_nodes(
                &request.root_selector,
                &request.attribute_requests,
                nodes,
                &mut results,
            );
        }
    }
    results
}
