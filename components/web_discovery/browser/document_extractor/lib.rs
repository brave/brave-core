/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use std::collections::HashMap;

use cxx::{CxxString, CxxVector};
use scraper::{html::Select, Html, Selector};

#[cxx::bridge(namespace = "web_discovery")]
mod ffi {
    pub struct SelectAttributeRequest {
        /// An optional selector for an element within the current selected element.
        /// The attribute will be retrieved from the embedded element.
        /// If not needed, an empty string should be provided.
        pub sub_selector: String,
        /// Arbitrary ID used for storing the scraped result.
        pub key: String,
        /// Name of the attribute to scrape.
        pub attribute: String,
    }

    pub struct SelectRequest {
        /// The DOM selector for the element to scrape.
        pub root_selector: String,
        /// Scrape requests for the selected element.
        pub attribute_requests: Vec<SelectAttributeRequest>,
    }

    pub struct AttributePair {
        /// Arbitrary ID for the scraped result.
        pub key: String,
        /// The scraped value. Will be empty if attribute is not available.
        pub value: String,
    }

    pub struct AttributeResult {
        /// The DOM selector for the scraped element.
        pub root_selector: String,
        /// A list of arbitrary IDs and scraped value pairs.
        pub attribute_pairs: Vec<AttributePair>,
    }

    extern "Rust" {
        /// Extracts DOM attributes from the result of a double fetch.
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
    selection: Select<'_, '_>,
    results: &mut Vec<AttributeResult>,
) {
    for node in selection {
        let mut attribute_map = HashMap::new();
        for attribute_request in attribute_requests {
            let sub_node = match attribute_request.sub_selector.is_empty() {
                false => match Selector::parse(&attribute_request.sub_selector) {
                    Ok(selector) => match node.select(&selector).next() {
                        Some(e) => Some(e),
                        None => {
                            attribute_map.insert(attribute_request.key.clone(), String::new());
                            continue;
                        }
                    },
                    Err(_) => continue,
                },
                true => None,
            };
            let node_to_query = sub_node.as_ref().unwrap_or(&node);

            let attribute_value = match attribute_request.attribute == TEXT_CONTENT_ATTRIBUTE_NAME {
                true => node_to_query.text().collect::<Vec<_>>().join(""),
                false => node_to_query
                    .attr(attribute_request.attribute.as_str())
                    .map(|v| v.to_string())
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
    let document = Html::parse_document(html.to_str().unwrap_or_default());
    let mut results = Vec::new();
    for request in requests {
        if let Ok(selector) = Selector::parse(&request.root_selector) {
            extract_attributes_from_nodes(
                &request.root_selector,
                &request.attribute_requests,
                document.select(&selector),
                &mut results,
            );
        }
    }
    results
}
