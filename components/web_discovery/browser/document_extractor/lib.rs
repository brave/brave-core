/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

use cxx::{CxxString, CxxVector};
use scraper::{html::Select, Html, Selector};

#[cxx::bridge(namespace = "web_discovery")]
mod ffi {
    pub struct SelectAttributeOption {
        /// An optional selector for an element within the current selected element.
        /// The attribute will be retrieved from the embedded element.
        /// If not needed, an empty string should be provided.
        pub sub_selector: String,
        /// Name of the attribute to scrape.
        pub attribute: String,
    }

    pub struct SelectAttributeRequest {
        /// Arbitrary ID used for storing the scraped result.
        pub key: String,
        /// Multiple extraction options to try ("first match" behavior).
        pub options: Vec<SelectAttributeOption>,
    }

    pub struct SelectRequest {
        /// The DOM selector for the element to scrape.
        pub root_selector: String,
        /// Scrape requests for the selected element.
        pub attribute_requests: Vec<SelectAttributeRequest>,
        /// If true, select all matching elements; if false, select only the first match.
        pub select_all: bool,
    }

    pub struct AttributePair {
        /// Arbitrary ID for the scraped result.
        pub key: String,
        /// The scraped value. Will be empty if attribute is not available.
        pub value: String,
        /// Index of the option that produced this value.
        pub option_index: u8,
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
    select_all: bool,
    results: &mut Vec<AttributeResult>,
) {
    let nodes_to_process: Vec<_> = match select_all {
        true => selection.collect(),
        false => selection.take(1).collect(),
    };

    for node in nodes_to_process {
        let mut attribute_pairs = Vec::with_capacity(attribute_requests.len());
        for attribute_request in attribute_requests {
            let mut pair = AttributePair {
                key: attribute_request.key.clone(),
                value: String::new(),
                option_index: 0,
            };

            // Try each option until one succeeds (firstMatch behavior)
            for (option_index, option) in attribute_request.options.iter().enumerate() {
                let sub_node = match option.sub_selector.is_empty() {
                    false => match Selector::parse(&option.sub_selector) {
                        Ok(selector) => match node.select(&selector).next() {
                            Some(e) => Some(e),
                            None => continue,
                        },
                        Err(_) => continue,
                    },
                    true => None,
                };
                let node_to_query = sub_node.as_ref().unwrap_or(&node);

                let attribute_value = match option.attribute == TEXT_CONTENT_ATTRIBUTE_NAME {
                    true => Some(node_to_query.text().collect::<Vec<_>>().join("")),
                    false => node_to_query.attr(option.attribute.as_str()).map(|v| v.to_string()),
                };

                if let Some(attribute_value) = attribute_value {
                    pair.value = attribute_value;
                    pair.option_index = option_index as u8;
                    break;
                }
            }
            attribute_pairs.push(pair);
        }
        results.push(AttributeResult { root_selector: root_selector.to_string(), attribute_pairs });
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
                request.select_all,
                &mut results,
            );
        }
    }
    results
}
